#pragma once

#include <zmq.hpp>
#include <iostream>
#include <string>
#include "message.pb.h"

class ZMQClient {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::string endpoint_;

public:
    explicit ZMQClient(const std::string& endpoint = "tcp://localhost:5555")
        : context_(1), socket_(nullptr), endpoint_(endpoint) {}

    ~ZMQClient() {
        if (socket_) {
            socket_->close();
        }
    }

    bool connect() {
        try {
            socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::req);
            socket_->set(zmq::sockopt::rcvtimeo, 5000); // 5s timeout
            socket_->connect(endpoint_);
            std::cout << "[ZMQClient] Connected to " << endpoint_ << std::endl;
            return true;
        } catch (const zmq::error_t& e) {
            std::cerr << "[ZMQClient] Connection error: " << e.what() << std::endl;
            return false;
        }
    }

    bool sendCommand(const std::string& command, const std::vector<double>& args = {}) {
        try {
            zrcs_message::MotionCommand cmd;
            cmd.set_command(command);
            for (double arg : args) {
                cmd.add_args(arg);
            }

            std::string serialized;
            if (!cmd.SerializeToString(&serialized)) {
                std::cerr << "[ZMQClient] Failed to serialize command" << std::endl;
                return false;
            }

            // 发送命令
            zmq::message_t request(serialized.size());
            memcpy(request.data(), serialized.data(), serialized.size());
            socket_->send(request, zmq::send_flags::none);

            // 接收回复
            zmq::message_t reply;
            auto result = socket_->recv(reply, zmq::recv_flags::none);
            
            if (result) {
                std::string response(static_cast<char*>(reply.data()), reply.size());
                std::cout << "[ZMQClient] Response: " << response << std::endl;
                return response == "OK";
            }
            return false;

        } catch (const zmq::error_t& e) {
            std::cerr << "[ZMQClient] Send error: " << e.what() << std::endl;
            return false;
        }
    }
};
