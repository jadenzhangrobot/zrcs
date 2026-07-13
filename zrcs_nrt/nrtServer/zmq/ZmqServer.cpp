#include "nrtServer/zmq/ZmqServer.h"

#include <cstring>
#include <vector>

#include <spdlog/spdlog.h>

#include "command/CommandRouter.h"
#include "message.pb.h"

ZMQServer::ZMQServer(CommandRouter* router)
    : context_(1)
    , socket_(nullptr)
    , router_(router)
{
}

ZMQServer::~ZMQServer()
{
    stop();
}

bool ZMQServer::initialize()
{
    std::lock_guard<std::mutex> lock(stopMutex_);
    try {
        socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::rep);
        socket_->set(zmq::sockopt::rcvtimeo, RECV_TIMEOUT);
        socket_->set(zmq::sockopt::linger, 0);
        socket_->bind(ENDPOINT);
        stopped_ = false;
        spdlog::info("[ZMQServer] Initialized on {}", ENDPOINT);
        return true;
    } catch (const zmq::error_t& e) {
        spdlog::error("[ZMQServer] Initialization error: {}", e.what());
        return false;
    }
}

void ZMQServer::start()
{
    std::lock_guard<std::mutex> lock(stopMutex_);
    if (running_.exchange(true)) {
        return;
    }
    stopped_ = false;
    server_thread_ = std::thread(&ZMQServer::run, this);
}

void ZMQServer::stop()
{
    std::lock_guard<std::mutex> lock(stopMutex_);
    if (stopped_) {
        return;
    }

    running_.store(false, std::memory_order_release);

    // join 前释放 socket 上的阻塞：rcvtimeo 会让 recv 返回，running_ 为 false 后退出循环
    if (server_thread_.joinable()) {
        // 禁止在 server 线程内 join 自己
        if (server_thread_.get_id() != std::this_thread::get_id()) {
            server_thread_.join();
        } else {
            server_thread_.detach();
        }
    }

    if (socket_) {
        try {
            socket_->close();
        } catch (...) {
        }
        socket_.reset();
    }

    try {
        context_.close();
    } catch (...) {
    }

    stopped_ = true;
    spdlog::info("[ZMQServer] Stopped");
}

void ZMQServer::run()
{
    spdlog::info("[ZMQServer] Server thread started, listening...");
    while (running_.load(std::memory_order_acquire)) {
        try {
            zmq::message_t request;
            auto result = socket_->recv(request, zmq::recv_flags::none);

            if (!result) {
                continue;
            }

            spdlog::debug("[ZMQServer] Received message, size={} bytes", request.size());

            zrcs_message::TypedCommand typedCmd;
            if (typedCmd.ParseFromArray(request.data(), request.size()) && typedCmd.has_bt_command()) {
                handleBehaviorTreeCommand(typedCmd.bt_command());
                continue;
            }

            zrcs_message::MotionCommand cmd;
            if (!cmd.ParseFromArray(request.data(), request.size())) {
                spdlog::error("[ZMQServer] Failed to parse protobuf message, size={}", request.size());
                sendReply("ERROR: Parse failed");
                continue;
            }

            spdlog::info("[ZMQServer] MotionCommand: cmd='{}', args_count={}",
                         cmd.command(),
                         cmd.args_size());
            handleMotionCommand(cmd);
        } catch (const zmq::error_t& e) {
            if (running_.load(std::memory_order_acquire) && e.num() != EAGAIN) {
                spdlog::error("[ZMQServer] ZMQ error in run loop: {} (errno={})", e.what(), e.num());
            }
        }
    }
    spdlog::info("[ZMQServer] Server thread exiting");
}

void ZMQServer::handleMotionCommand(const zrcs_message::MotionCommand& cmd)
{
    if (!router_) {
        sendReply("ERROR: Not connected");
        return;
    }
    std::vector<double> args(cmd.args().begin(), cmd.args().end());
    sendReply(router_->routeMotion(cmd.command(), args));
}

void ZMQServer::handleBehaviorTreeCommand(const zrcs_message::BehaviorTreeCommand& cmd)
{
    if (!router_) {
        sendReply("ERROR: BehaviorTree runner unavailable");
        return;
    }
    sendReply(router_->routeBehaviorTree(cmd.action(), cmd.xml_data()));
}

void ZMQServer::sendReply(const std::string& message)
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
