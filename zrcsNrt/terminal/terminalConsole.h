#pragma once

/**
 * @file terminalConsole.h
 * @brief 终端控制台，允许操作员通过 stdin 直接输入命令
 * @details 运动命令通过 ZMQ REQ 回环发送到本地 ZMQ Server（保持 SPSC 队列单生产者）
 *          NRT 本地命令（bt、help、quit）直接在本线程处理
 */

#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include "message.pb.h"
#include "btEngine.h"

class TerminalConsole {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::thread console_thread_;
    std::atomic<bool> running_;
    BTEngine* bt_engine_;
    std::atomic<bool>& app_running_;

    static constexpr const char* ZMQ_ENDPOINT = "tcp://localhost:5555";
    static constexpr int RECV_TIMEOUT = 5000; // ms

public:
    TerminalConsole(BTEngine* bt_engine, std::atomic<bool>& app_running)
        : context_(1), socket_(nullptr), running_(false),
          bt_engine_(bt_engine), app_running_(app_running) {}

    ~TerminalConsole() {
        stop();
    }

    bool initialize() {
        try {
            socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::req);
            socket_->set(zmq::sockopt::rcvtimeo, RECV_TIMEOUT);
            socket_->set(zmq::sockopt::linger, 0);
            socket_->connect(ZMQ_ENDPOINT);
            spdlog::info("[Terminal] Initialized, connected to {}", ZMQ_ENDPOINT);
            return true;
        } catch (const zmq::error_t& e) {
            spdlog::error("[Terminal] Initialization error: {}", e.what());
            return false;
        }
    }

    void start() {
        if (running_.exchange(true)) {
            return;
        }
        console_thread_ = std::thread(&TerminalConsole::run, this);
    }

    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        // stdin 的 getline 是阻塞的，进程退出时线程会自然终止
        // 这里 detach 避免 join 死等
        if (console_thread_.joinable()) {
            console_thread_.detach();
        }
        if (socket_) {
            socket_->close();
            socket_.reset();
        }
        context_.close();
        spdlog::info("[Terminal] Stopped");
    }

private:
    void run() {
        printBanner();

        std::string line;
        while (running_ && app_running_) {
            std::cout << "zrcs> " << std::flush;

            if (!std::getline(std::cin, line)) {
                // EOF 或 stdin 关闭
                spdlog::info("[Terminal] stdin closed, exiting console");
                break;
            }

            // 去除首尾空白
            auto trimmed = trim(line);
            if (trimmed.empty()) {
                continue;
            }

            handleLine(trimmed);
        }
        spdlog::info("[Terminal] Console thread exiting");
    }

    void handleLine(const std::string& line) {
        auto tokens = tokenize(line);
        if (tokens.empty()) return;

        const auto& cmd = tokens[0];

        // 系统命令
        if (cmd == "help" || cmd == "h" || cmd == "?") {
            printHelp();
            return;
        }
        if (cmd == "quit" || cmd == "exit") {
            std::cout << "Shutting down..." << std::endl;
            app_running_ = false;
            running_ = false;
            return;
        }

        // BT 命令（本地处理）
        if (cmd == "bt") {
            handleBTCommand(tokens);
            return;
        }

        // 运动命令（通过 ZMQ 发送）
        handleMotionCommand(tokens);
    }

    void handleBTCommand(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            std::cout << "Usage: bt <status|start|stop|load <file>>" << std::endl;
            return;
        }

        const auto& action = tokens[1];

        if (action == "status") {
            std::cout << "BT State: " << bt_engine_->getStateString()
                      << ", Active Node: " << bt_engine_->getCurrentNodeName()
                      << std::endl;
        } else if (action == "start") {
            if (bt_engine_->start()) {
                std::cout << "BT started" << std::endl;
            } else {
                std::cout << "BT start failed (no tree loaded or already running)" << std::endl;
            }
        } else if (action == "stop") {
            bt_engine_->stop();
            std::cout << "BT stopped" << std::endl;
        } else if (action == "load") {
            if (tokens.size() < 3) {
                std::cout << "Usage: bt load <filepath>" << std::endl;
                return;
            }
            // 读取 XML 文件
            std::ifstream ifs(tokens[2]);
            if (!ifs.is_open()) {
                std::cout << "Error: cannot open file: " << tokens[2] << std::endl;
                return;
            }
            std::string xml((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
            ifs.close();

            std::string err = bt_engine_->loadTree(xml);
            if (err.empty()) {
                std::cout << "BT tree loaded (" << xml.size() << " bytes)" << std::endl;
            } else {
                std::cout << "BT load error: " << err << std::endl;
            }
        } else {
            std::cout << "Unknown bt action: " << action << std::endl;
            std::cout << "Usage: bt <status|start|stop|load <file>>" << std::endl;
        }
    }

    void handleMotionCommand(const std::vector<std::string>& tokens) {
        const auto& cmd_name = tokens[0];
        std::vector<double> args;

        // 解析剩余 token 为 double 参数
        for (size_t i = 1; i < tokens.size(); ++i) {
            try {
                args.push_back(std::stod(tokens[i]));
            } catch (const std::exception&) {
                std::cout << "Error: invalid argument '" << tokens[i]
                          << "' (expected number)" << std::endl;
                return;
            }
        }

        std::string reply = sendMotionCommand(cmd_name, args);
        if (!reply.empty()) {
            std::cout << "Reply: " << reply << std::endl;
        }
    }

    /**
     * @brief 通过 ZMQ REQ 发送 MotionCommand 到本地 ZMQ Server
     * @return 服务器回复字符串，超时返回空
     */
    std::string sendMotionCommand(const std::string& cmd,
                                  const std::vector<double>& args) {
        if (!socket_) {
            std::cout << "Error: ZMQ socket not available" << std::endl;
            return "";
        }

        try {
            // 构造 MotionCommand protobuf
            zrcs_message::MotionCommand motion_cmd;
            motion_cmd.set_command(cmd);
            for (double arg : args) {
                motion_cmd.add_args(arg);
            }

            // 序列化并发送
            std::string serialized;
            motion_cmd.SerializeToString(&serialized);

            zmq::message_t request(serialized.size());
            memcpy(request.data(), serialized.data(), serialized.size());
            socket_->send(request, zmq::send_flags::none);

            spdlog::debug("[Terminal] Sent command '{}' with {} args", cmd, args.size());

            // 接收回复
            zmq::message_t reply;
            auto result = socket_->recv(reply, zmq::recv_flags::none);
            if (!result) {
                std::cout << "Error: reply timeout" << std::endl;
                reconnect();
                return "";
            }

            return std::string(static_cast<char*>(reply.data()), reply.size());

        } catch (const zmq::error_t& e) {
            std::cout << "ZMQ error: " << e.what() << std::endl;
            spdlog::error("[Terminal] ZMQ error: {}", e.what());
            reconnect();
            return "";
        }
    }

    /**
     * @brief 重建 ZMQ REQ socket（超时或错误后恢复）
     */
    void reconnect() {
        try {
            if (socket_) {
                socket_->close();
            }
            socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::req);
            socket_->set(zmq::sockopt::rcvtimeo, RECV_TIMEOUT);
            socket_->set(zmq::sockopt::linger, 0);
            socket_->connect(ZMQ_ENDPOINT);
            spdlog::info("[Terminal] Reconnected to {}", ZMQ_ENDPOINT);
        } catch (const zmq::error_t& e) {
            spdlog::error("[Terminal] Reconnect failed: {}", e.what());
            socket_.reset();
        }
    }

    void printBanner() {
        std::cout << "\n"
                  << "========================================\n"
                  << "  ZRCS Terminal Console\n"
                  << "  Type 'help' for available commands\n"
                  << "========================================\n"
                  << std::endl;
    }

    void printHelp() {
        std::cout << "\n"
            "Motion Commands (sent to RT via ZMQ):\n"
            "  Enable [axisId]                Enable motor\n"
            "  Disable [axisId]               Disable motor\n"
            "  Reset [axisId]                 Reset axis error\n"
            "  Stop                           Emergency stop\n"
            "  MoveJ <a1> <a2> <a3> ...       Joint move\n"
            "  MoveL <x> <y> <z> ...          Linear move\n"
            "  MoveC <args...>                Circular move\n"
            "  JogJ <axisId> <position>       Jog joint\n"
            "  JogabsJ <axisId> <position>    Jog joint absolute\n"
            "  ContinuousJog <axisId> <dir>   Continuous jog\n"
            "  Show                           Show status\n"
            "  <command> [args...]             Any RT command\n"
            "\n"
            "BT Commands:\n"
            "  bt status                      Show behavior tree state\n"
            "  bt start                       Start behavior tree\n"
            "  bt stop                        Stop behavior tree\n"
            "  bt load <filepath>             Load BT from XML file\n"
            "\n"
            "System Commands:\n"
            "  help                           Show this message\n"
            "  quit / exit                    Shutdown NRT process\n"
            << std::endl;
    }

    // ---- 工具函数 ----

    static std::vector<std::string> tokenize(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }
};
