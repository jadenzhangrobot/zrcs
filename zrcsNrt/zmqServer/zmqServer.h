#pragma once
#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "message.pb.h"
#include "rtBridge/rtBridge.h"
#include "btEngine.h"

class ZMQServer {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    RtBridge* bridge_;
    BTEngine* bt_engine_;

    static constexpr const char* ENDPOINT = "tcp://*:5555";
    static constexpr int RECV_TIMEOUT = 1000; // ms

public:
    ZMQServer(RtBridge* bridge, BTEngine* bt_engine)
        : context_(1), socket_(nullptr), running_(false),
          bridge_(bridge), bt_engine_(bt_engine) {}

    ~ZMQServer() {
        stop();
    }

    bool initialize() {
        try {
            socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::rep);
            socket_->set(zmq::sockopt::rcvtimeo, RECV_TIMEOUT);
            socket_->set(zmq::sockopt::linger, 0);  // 关闭时不等待未发送消息
            socket_->bind(ENDPOINT);

            spdlog::info("[ZMQServer] Initialized on {}", ENDPOINT);
            return true;
        } catch (const zmq::error_t& e) {
            spdlog::error("[ZMQServer] Initialization error: {}", e.what());
            return false;
        }
    }

    void start() {
        if (running_.exchange(true)) {
            return; // Already running
        }
        server_thread_ = std::thread(&ZMQServer::run, this);
    }

    void stop() {
        if (!running_.exchange(false)) {
            return; // Already stopped
        }
        // 等待服务线程退出（recv 超时后会自然退出循环）
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        // 显式关闭 socket 和 context，释放端口
        if (socket_) {
            socket_->close();
            socket_.reset();
        }
        context_.close();

        if (bridge_->droppedCount() > 0) {
            spdlog::warn("[ZMQServer] Total dropped commands: {}", bridge_->droppedCount());
        }
        spdlog::info("[ZMQServer] Stopped");
    }

private:
    void run() {
        spdlog::info("[ZMQServer] Server thread started, listening...");
        while (running_) {
            try {
                zmq::message_t request;
                auto result = socket_->recv(request, zmq::recv_flags::none);

                if (!result) {
                    continue;
                }

                spdlog::debug("[ZMQServer] Received message, size={} bytes", request.size());

                // 先尝试解析为 TypedCommand（支持 BT 命令）
                zrcs_message::TypedCommand typed_cmd;
                if (typed_cmd.ParseFromArray(request.data(), request.size())
                    && typed_cmd.has_bt_command())
                {
                    spdlog::info("[ZMQServer] Parsed as TypedCommand with BT command, action='{}'",
                                 typed_cmd.bt_command().action());
                    handleBTCommand(typed_cmd.bt_command());
                    continue;
                }

                // 回退：解析为 MotionCommand（兼容现有协议）
                zrcs_message::MotionCommand cmd;
                if (!cmd.ParseFromArray(request.data(), request.size())) {
                    spdlog::error("[ZMQServer] Failed to parse protobuf message, size={}", request.size());
                    sendReply("ERROR: Parse failed");
                    continue;
                }

                spdlog::info("[ZMQServer] MotionCommand: cmd='{}', args_count={}",
                             cmd.command(), cmd.args_size());
                for (int i = 0; i < cmd.args_size(); ++i) {
                    spdlog::debug("[ZMQServer]   arg[{}] = {}", i, cmd.args(i));
                }

                // 通过路由层分发命令
                handleMotionCommand(cmd);

            } catch (const zmq::error_t& e) {
                if (running_ && e.num() != EAGAIN) {
                    spdlog::error("[ZMQServer] ZMQ error in run loop: {} (errno={})", e.what(), e.num());
                }
            }
        }
        spdlog::info("[ZMQServer] Server thread exiting");
    }

    /**
     * @brief 获取可执行文件所在目录
     */
    static std::filesystem::path getExeDir() {
#ifdef _WIN32
        char buf[MAX_PATH];
        GetModuleFileNameA(nullptr, buf, MAX_PATH);
        return std::filesystem::path(buf).parent_path();
#else
        return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
    }

    /**
     * @brief 将 BT XML 保存到 config/bt/ 目录
     * @return 保存的文件路径，失败返回空字符串
     */
    std::string saveBTXml(const std::string& xml_data) {
        try {
            auto bt_dir = getExeDir() / "config" / "bt";
            std::filesystem::create_directories(bt_dir);

            // 用时间戳命名，同时维护一个 current.xml 始终指向最新
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf{};
#ifdef _WIN32
            localtime_s(&tm_buf, &t);
#else
            localtime_r(&t, &tm_buf);
#endif
            char ts[64];
            std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm_buf);

            std::string filename = std::string("bt_") + ts + ".xml";
            auto filepath = bt_dir / filename;

            std::ofstream ofs(filepath, std::ios::out | std::ios::trunc);
            if (!ofs.is_open()) {
                spdlog::error("[ZMQServer] Failed to open file: {}", filepath.string());
                return "";
            }
            ofs << xml_data;
            ofs.close();

            // 覆盖写 current.xml，方便下次启动时加载最新树
            auto current_path = bt_dir / "current.xml";
            std::ofstream cur(current_path, std::ios::out | std::ios::trunc);
            if (cur.is_open()) {
                cur << xml_data;
                cur.close();
            }

            spdlog::info("[ZMQServer] BT XML saved: {} ({} bytes)", filepath.string(), xml_data.size());
            return filepath.string();
        } catch (const std::exception& e) {
            spdlog::error("[ZMQServer] Failed to save BT XML: {}", e.what());
            return "";
        }
    }

    void handleMotionCommand(const zrcs_message::MotionCommand& cmd) {
        const std::string& name = cmd.command();
        std::vector<double> args(cmd.args().begin(), cmd.args().end());

        if (name == "SYS_RUN") {
            bridge_->requestRun();
            spdlog::info("[ZMQServer] SYS_RUN: TaskScheduling -> RUN");
            sendReply("OK");
        } else if (name == "SYS_STOP") {
            bridge_->requestStop();
            spdlog::info("[ZMQServer] SYS_STOP: TaskScheduling -> STOP");
            sendReply("OK");
        } else if (name == "SYS_RESET") {
            bridge_->requestReset();
            spdlog::info("[ZMQServer] SYS_RESET: TaskScheduling -> RESET");
            sendReply("OK");
        } else if (name == "SYS_ESTOP") {
            bridge_->sendCommand("EmergStop");
            bridge_->requestStop();
            spdlog::warn("[ZMQServer] SYS_ESTOP: EmergStop + STOP");
            sendReply("OK");
        } else if (name == "SYS_JOG_START") {
            if (args.size() >= 2) {
                int axisId = static_cast<int>(args[0]);
                bool direction = args[1] > 0.0;
                bridge_->startContinuousMotion(axisId, direction);
                spdlog::info("[ZMQServer] SYS_JOG_START: axis={}, dir={}", axisId, direction);
                sendReply("OK");
            } else {
                sendReply("ERROR: SYS_JOG_START requires 2 args (axisId, direction)");
            }
        } else if (name == "SYS_JOG_STOP") {
            bridge_->stopContinuousMotion();
            spdlog::info("[ZMQServer] SYS_JOG_STOP");
            sendReply("OK");
        } else if (name == "SYS_SET_MULTIPLIER") {
            if (args.size() >= 1) {
                bridge_->setSpeedMultiplier(static_cast<uint8_t>(args[0]));
                spdlog::info("[ZMQServer] SYS_SET_MULTIPLIER: {}%", static_cast<int>(args[0]));
                sendReply("OK");
            } else {
                sendReply("ERROR: SYS_SET_MULTIPLIER requires 1 arg (percent)");
            }
        } else {
            auto [send_result, seq] = bridge_->sendCommand(name, args);
            if (send_result == RtBridge::SendResult::OK) {
                spdlog::info("[ZMQServer] Command '{}' sent via RtBridge (seq={})", name, seq);
                sendReply("OK");
            } else {
                spdlog::error("[ZMQServer] Command queue full! cmd='{}'", name);
                sendReply("ERROR: Queue full");
            }
        }
    }

    void handleBTCommand(const zrcs_message::BehaviorTreeCommand& bt_cmd) {
        const std::string& action = bt_cmd.action();
        spdlog::info("[ZMQServer] handleBTCommand: action='{}', xml_size={}", action, bt_cmd.xml_data().size());

        if (action == "LOAD") {
            // 先保存到本地文件
            saveBTXml(bt_cmd.xml_data());

            // 再加载到行为树引擎
            std::string err = bt_engine_->loadTree(bt_cmd.xml_data());
            if (err.empty()) {
                spdlog::info("[ZMQServer] BT LOAD success");
                sendReply("OK");
            } else {
                spdlog::error("[ZMQServer] BT LOAD failed: {}", err);
                sendReply("ERROR: " + err);
            }
        } else if (action == "START") {
            if (bt_engine_->start()) {
                spdlog::info("[ZMQServer] BT START success");
                sendReply("OK");
            } else {
                spdlog::error("[ZMQServer] BT START failed");
                sendReply("ERROR: Failed to start BT execution");
            }
        } else if (action == "STOP") {
            bt_engine_->stop();
            spdlog::info("[ZMQServer] BT STOP");
            sendReply("OK");
        } else if (action == "STATUS") {
            // 返回 BehaviorTreeStatus protobuf 序列化
            zrcs_message::BehaviorTreeStatus status;
            status.set_tree_state(bt_engine_->getStateString());
            status.set_current_node(bt_engine_->getCurrentNodeName());
            std::string serialized;
            status.SerializeToString(&serialized);
            spdlog::debug("[ZMQServer] BT STATUS: state='{}', node='{}'",
                          bt_engine_->getStateString(), bt_engine_->getCurrentNodeName());
            sendReplyRaw(serialized);
        } else {
            spdlog::warn("[ZMQServer] Unknown BT action: '{}'", action);
            sendReply("ERROR: Unknown BT action: " + action);
        }
    }

    void sendReply(const std::string& message) {
        try {
            zmq::message_t reply(message.size());
            memcpy(reply.data(), message.data(), message.size());
            socket_->send(reply, zmq::send_flags::none);
            spdlog::debug("[ZMQServer] Reply sent: '{}'", message);
        } catch (const zmq::error_t& e) {
            spdlog::error("[ZMQServer] Send reply error: {}", e.what());
        }
    }

    void sendReplyRaw(const std::string& data) {
        try {
            zmq::message_t reply(data.size());
            memcpy(reply.data(), data.data(), data.size());
            socket_->send(reply, zmq::send_flags::none);
            spdlog::debug("[ZMQServer] Raw reply sent, size={}", data.size());
        } catch (const zmq::error_t& e) {
            spdlog::error("[ZMQServer] Send reply error: {}", e.what());
        }
    }
};
