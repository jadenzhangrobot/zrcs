#pragma once
#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <string>
#include <spdlog/spdlog.h>
#include "message.pb.h"
#include "rt_bridge/RtBridge.h"

class ZMQServer {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    RtBridge* bridge_;

    static constexpr const char* ENDPOINT = "tcp://*:5555";
    static constexpr int RECV_TIMEOUT = 1000; // ms

public:
    ZMQServer(RtBridge* bridge)
        : context_(1), socket_(nullptr), running_(false),
          bridge_(bridge) {}

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
        // 先关闭 socket，中断阻塞中的 recv，使服务线程立即退出
        if (socket_) {
            socket_->close();
            socket_.reset();
        }
        // 等待服务线程退出（此时 recv 已被中断，不会等超时）
        if (server_thread_.joinable()) {
            server_thread_.join();
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

                // 解析为 MotionCommand
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
        } else if (name == "SYS_SET_ORIGIN") {
            // SetZero all axes: pass axisId = axisCount as sentinel
            double sentinel = static_cast<double>(bridge_->axisCount());
            auto [send_result, seq] = bridge_->sendCommand("SetZero", std::vector<double>{sentinel});
            if (send_result == RtBridge::SendResult::OK) {
                spdlog::info("[ZMQServer] SYS_SET_ORIGIN -> SetZero(all) seq={}", seq);
                sendReply("OK");
            } else {
                spdlog::error("[ZMQServer] SYS_SET_ORIGIN failed");
                sendReply("ERROR: Queue full");
            }
        } else if (name == "SYS_SET_AXIS_ORIGIN") {
            if (args.size() >= 1) {
                auto [send_result, seq] = bridge_->sendCommand("SetZero", std::vector<double>(args));
                if (send_result == RtBridge::SendResult::OK) {
                    spdlog::info("[ZMQServer] SYS_SET_AXIS_ORIGIN -> SetZero(axis={}) seq={}", static_cast<int>(args[0]), seq);
                    sendReply("OK");
                } else {
                    spdlog::error("[ZMQServer] SYS_SET_AXIS_ORIGIN failed");
                    sendReply("ERROR: Queue full");
                }
            } else {
                sendReply("ERROR: SYS_SET_AXIS_ORIGIN requires 1 arg (axisId)");
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
