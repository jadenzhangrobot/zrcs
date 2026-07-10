#pragma once
#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include "message.pb.h"
#include "behaviorTree/BehaviorTreeRunner.h"
#include "command/RtBridge.h"
#include "command/CommandDispatcher.h"

class ZMQServer {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    RtBridge* bridge_;
    BehaviorTreeRunner* behaviorTreeRunner_;
    CommandDispatcher* dispatcher_;

    static constexpr const char* ENDPOINT = "tcp://*:5555";
    static constexpr int RECV_TIMEOUT = 100; // ms

public:
    ZMQServer(RtBridge* bridge,
              CommandDispatcher* dispatcher,
              BehaviorTreeRunner* behaviorTreeRunner = nullptr)
        : context_(1), socket_(nullptr), running_(false),
          bridge_(bridge), behaviorTreeRunner_(behaviorTreeRunner),
          dispatcher_(dispatcher) {}

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
        const bool wasRunning = running_.exchange(false);
        const bool hasThread = server_thread_.joinable();
        const bool hasSocket = static_cast<bool>(socket_);
        if (!wasRunning && !hasThread && !hasSocket) {
            return; // Already stopped
        }
        if (server_thread_.joinable()) 
        {
            server_thread_.join();
        }
        if (socket_) {
            socket_->close();
            socket_.reset();
        }
        context_.close();

        if (bridge_ && bridge_->droppedCount() > 0) {
            spdlog::warn("[ZMQServer] Total dropped commands: {}", bridge_->droppedCount());
        }
        if (wasRunning || hasThread) {
            spdlog::info("[ZMQServer] Stopped");
        }
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

                zrcs_message::TypedCommand typedCmd;
                if (typedCmd.ParseFromArray(request.data(), request.size()) && typedCmd.has_bt_command())
                {
                    handleBehaviorTreeCommand(typedCmd.bt_command());
                    continue;
                }

                zrcs_message::MotionCommand cmd;
                if (!cmd.ParseFromArray(request.data(), request.size()))
                {
                    spdlog::error("[ZMQServer] Failed to parse protobuf message, size={}", request.size());
                    sendReply("ERROR: Parse failed");
                    continue;
                }

                spdlog::info("[ZMQServer] MotionCommand: cmd='{}', args_count={}",
                             cmd.command(), cmd.args_size());
                for (int i = 0; i < cmd.args_size(); ++i) {
                    spdlog::debug("[ZMQServer]   arg[{}] = {}", i, cmd.args(i));
                }

                handleMotionCommand(cmd);

            } catch (const zmq::error_t& e) {
                if (running_ && e.num() != EAGAIN) {
                    spdlog::error("[ZMQServer] ZMQ error in run loop: {} (errno={})", e.what(), e.num());
                }
            }
        }
        spdlog::info("[ZMQServer] Server thread exiting");
    }

    // 命令语义分发交给 CommandDispatcher（领域层），本类只做传输编解码。
    void handleMotionCommand(const zrcs_message::MotionCommand& cmd) {
        std::vector<double> args(cmd.args().begin(), cmd.args().end());
        sendReply(dispatcher_->dispatch(cmd.command(), args));
    }

    void handleBehaviorTreeCommand(const zrcs_message::BehaviorTreeCommand& cmd) {
        if (!behaviorTreeRunner_)
        {
            sendReply("ERROR: BehaviorTree runner unavailable");
            return;
        }

        const std::string action = cmd.action();
        if (action == "LOAD")
        {
            if (cmd.xml_data().empty())
            {
                sendReply("ERROR: LOAD requires xml_data");
                return;
            }

            std::string error;
            if (behaviorTreeRunner_->loadFromXml(cmd.xml_data(), error))
            {
                spdlog::info("[ZMQServer] Behavior tree loaded");
                sendReply("OK");
            }
            else
            {
                spdlog::error("[ZMQServer] Behavior tree load failed: {}", error);
                sendReply("ERROR: " + error);
            }
            return;
        }

        if (action == "START")
        {
            std::string error;
            if (behaviorTreeRunner_->start(error))
            {
                spdlog::info("[ZMQServer] Behavior tree started");
                sendReply("OK");
            }
            else
            {
                sendReply("ERROR: " + error);
            }
            return;
        }

        if (action == "STOP")
        {
            behaviorTreeRunner_->stop("Stopped by command");
            spdlog::info("[ZMQServer] Behavior tree stopped");
            sendReply("OK");
            return;
        }

        if (action == "STATUS")
        {
            const auto status = behaviorTreeRunner_->status();
            std::string reply = "STATE=" + status.treeState + ";NODE=" + status.currentNode + ";MSG=" + status.message;
            sendReply(reply);
            return;
        }

        sendReply("ERROR: Unknown BT action");
    }

    void sendReply(const std::string& message) 
    {
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
