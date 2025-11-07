#pragma once
#include <zmq.hpp>
#include "nlohmann/json.hpp"
class ZmqServer
{
public:
    ZmqServer()
    {
        try {
            context_ = zmq::context_t(1);
            publisher = zmq::socket_t(context_, zmq::socket_type::pub);
            publisher.bind("tcp://*:8888");
        } catch (const zmq::error_t& e) {
            throw std::runtime_error(std::string("ZMQ 初始化失败: ") + e.what());
        }
    }
    ~ZmqServer();
    void send(const nlohmann::json& message)
    {
        try {
            std::string message_str = message.dump();   
            publisher.send(zmq::buffer(message_str));
        } catch (const zmq::error_t& e) {
            throw std::runtime_error(std::string("ZMQ 发送失败: ") + e.what());
        }
    }
private:
    zmq::context_t context_;
    zmq::socket_t publisher;
};