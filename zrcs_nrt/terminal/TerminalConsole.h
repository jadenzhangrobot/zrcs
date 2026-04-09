#pragma once

/**
 * @file terminalConsole.h
 * @brief 终端控制台，允许操作员通过 stdin 直接输入命令
 * @details 运动命令通过 RtBridge 直接发送到 RT 进程
 *          BT 命令通过 BTEngine 本地处理
 *          与 ZMQ 完全解耦，各走各的通道
 */

#include <thread>
#include <atomic>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include "rt_bridge/RtBridge.h"
#include "BtEngine.h"

class TerminalConsole {
private:
    std::thread console_thread_;
    std::atomic<bool> running_;
    RtBridge* bridge_;
    BTEngine* bt_engine_;
    std::atomic<bool>& app_running_;

public:
    TerminalConsole(RtBridge* bridge, BTEngine* bt_engine, std::atomic<bool>& app_running)
        : running_(false), bridge_(bridge),
          bt_engine_(bt_engine), app_running_(app_running) {}

    ~TerminalConsole() {
        stop();
    }

    bool initialize() {
        if (!bridge_ || !bridge_->isConnected()) {
            spdlog::error("[Terminal] RtBridge not available");
            return false;
        }
        spdlog::info("[Terminal] Initialized (direct RtBridge mode)");
        return true;
    }

    void start() {
        if (running_.exchange(true)) {
            return;
        }
        console_thread_ = std::thread(&TerminalConsole::run, this);
    }

    void stop() {
        running_ = false;
        // stdin 的 getline 是阻塞的，进程退出时线程会自然终止
        // 这里 detach 避免 join 死等
        if (console_thread_.joinable()) {
            console_thread_.detach();
        }
        spdlog::info("[Terminal] Stopped");
    }

private:
    void run() {
        printBanner();

        std::string line;
        while (running_ && app_running_) {
            std::cout << "zrcs> " << std::flush;

            if (!std::getline(std::cin, line)) {
                spdlog::info("[Terminal] stdin closed, exiting console");
                break;
            }

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

        // 运动命令（通过 RtBridge 直接发送到 RT）
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

        for (size_t i = 1; i < tokens.size(); ++i) {
            try {
                args.push_back(std::stod(tokens[i]));
            } catch (const std::exception&) {
                std::cout << "Error: invalid argument '" << tokens[i]
                          << "' (expected number)" << std::endl;
                return;
            }
        }

        auto [result, seq] = bridge_->sendCommand(cmd_name, args);
        switch (result) {
        case RtBridge::SendResult::OK:
            std::cout << "OK (seq=" << seq << ")" << std::endl;
            break;
        case RtBridge::SendResult::QUEUE_FULL:
            std::cout << "Error: command queue full" << std::endl;
            break;
        case RtBridge::SendResult::NOT_CONNECTED:
            std::cout << "Error: not connected to RT" << std::endl;
            break;
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
            "Motion Commands (sent to RT via RtBridge):\n"
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
