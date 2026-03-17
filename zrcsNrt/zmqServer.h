#pragma once

#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <iostream>
#include <memory>
#include "message.pb.h"
#include "sharedMemory/sharedData.h"

class ZMQServer {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    SharedBlock* shared_block_;
    std::atomic<uint64_t> dropped_count_{0};

    static constexpr const char* ENDPOINT = "tcp://*:5555";
    static constexpr int RECV_TIMEOUT = 1000; // ms

public:
    explicit ZMQServer(SharedBlock* shared_block)
        : context_(1), socket_(nullptr), running_(false), shared_block_(shared_block) {}

    ~ZMQServer() {
        stop();
    }

    bool initialize() {
        try {
            socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::rep);
            socket_->set(zmq::sockopt::rcvtimeo, RECV_TIMEOUT);
            socket_->set(zmq::sockopt::linger, 0);  // 关闭时不等待未发送消息
            socket_->bind(ENDPOINT);

            std::cout << "[ZMQServer] Initialized on " << ENDPOINT << std::endl;
            return true;
        } catch (const zmq::error_t& e) {
            std::cerr << "[ZMQServer] Initialization error: " << e.what() << std::endl;
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

        if (dropped_count_ > 0) {
            std::cout << "[ZMQServer] Total dropped commands: " << dropped_count_.load() << std::endl;
        }
        std::cout << "[ZMQServer] Stopped" << std::endl;
    }

    uint64_t droppedCount() const { return dropped_count_.load(); }

private:
    void run() {
        while (running_) {
            try {
                zmq::message_t request;
                auto result = socket_->recv(request, zmq::recv_flags::none);

                if (!result) {
                    continue;
                }

                // 反序列化 Protobuf 消息
                zrcs_message::MotionCommand cmd;
                if (!cmd.ParseFromArray(request.data(), request.size())) {
                    std::cerr << "[ZMQServer] Failed to parse protobuf message" << std::endl;
                    sendReply("ERROR: Parse failed");
                    continue;
                }

                // 转换为共享内存命令格式
                Command shm_cmd{};
                strncpy(shm_cmd.cmd, cmd.command().c_str(), sizeof(shm_cmd.cmd) - 1);
                shm_cmd.cmd[sizeof(shm_cmd.cmd) - 1] = '\0';

                // 复制参数（受 MAX_CMD_ARGS 限制）
                for (size_t i = 0; i < static_cast<size_t>(cmd.args_size()) && i < MAX_CMD_ARGS; ++i) {
                    shm_cmd.args[i] = cmd.args(i);
                }

                // 推送到共享内存队列
                if (shared_block_->commandQueue.push(shm_cmd)) {
                    std::cout << "[ZMQServer] Command received: " << cmd.command() << std::endl;
                    sendReply("OK");
                } else {
                    ++dropped_count_;
                    std::cerr << "[ZMQServer] Command queue full (dropped: "
                              << dropped_count_.load() << ")" << std::endl;
                    sendReply("ERROR: Queue full");
                }

            } catch (const zmq::error_t& e) {
                if (running_ && e.num() != EAGAIN) {
                    std::cerr << "[ZMQServer] Error: " << e.what() << std::endl;
                }
            }
        }
    }

    void sendReply(const std::string& message) {
        try {
            zmq::message_t reply(message.size());
            memcpy(reply.data(), message.data(), message.size());
            socket_->send(reply, zmq::send_flags::none);
        } catch (const zmq::error_t& e) {
            std::cerr << "[ZMQServer] Send reply error: " << e.what() << std::endl;
        }
    }
};
