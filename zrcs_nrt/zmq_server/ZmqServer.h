#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include <zmq.hpp>

class BehaviorTreeRunner;
class CommandDispatcher;
class RtBridge;

namespace zrcs_message {
class MotionCommand;
class BehaviorTreeCommand;
}

class ZMQServer {
public:
    ZMQServer(RtBridge* bridge,
              CommandDispatcher* dispatcher,
              BehaviorTreeRunner* behaviorTreeRunner = nullptr);

    ~ZMQServer();

    bool initialize();
    void start();
    void stop();

private:
    void run();
    void handleMotionCommand(const zrcs_message::MotionCommand& cmd);
    void handleBehaviorTreeCommand(const zrcs_message::BehaviorTreeCommand& cmd);
    void sendReply(const std::string& message);
    void sendReplyRaw(const std::string& data);

    static constexpr const char* ENDPOINT = "tcp://*:5555";
    static constexpr int RECV_TIMEOUT = 100; // ms

    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    RtBridge* bridge_;
    BehaviorTreeRunner* behaviorTreeRunner_;
    CommandDispatcher* dispatcher_;
};
