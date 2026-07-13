#pragma once
/**
 * @file ZmqServer.h
 * @brief ZMQ REP 控制面：解码 protobuf → CommandRouter → 回复字符串。
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <zmq.hpp>

class CommandRouter;

namespace zrcs_message {
class MotionCommand;
class BehaviorTreeCommand;
}

class ZMQServer {
public:
    explicit ZMQServer(CommandRouter* router);

    ~ZMQServer();

    bool initialize();
    void start();
    /// 线程安全、可重入：多次调用只清理一次。
    void stop();

private:
    void run();
    void handleMotionCommand(const zrcs_message::MotionCommand& cmd);
    void handleBehaviorTreeCommand(const zrcs_message::BehaviorTreeCommand& cmd);
    void sendReply(const std::string& message);

    static constexpr const char* ENDPOINT = "tcp://*:5555";
    static constexpr int RECV_TIMEOUT = 100; // ms

    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    CommandRouter* router_;
    std::mutex stopMutex_;
    bool stopped_ = true;
};
