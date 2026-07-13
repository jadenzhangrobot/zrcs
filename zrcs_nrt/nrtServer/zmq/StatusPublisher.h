#pragma once
/**
 * @file StatusPublisher.h
 * @brief 从 StatusStore 读取快照，经 ZMQ PUB 发布给 GUI。
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include <zmq.hpp>

class StatusStore;

class StatusPublisher {
public:
    explicit StatusPublisher(StatusStore* store);
    ~StatusPublisher();

    bool initialize();
    void start();
    /// 线程安全、可重入：多次调用只清理一次。
    void stop();

private:
    void run();

    static constexpr const char* PUB_ENDPOINT = "tcp://*:5556";
    static constexpr int PUB_INTERVAL_MS = 10;

    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> pub_socket_;
    StatusStore* store_;
    std::thread pub_thread_;
    std::atomic<bool> running_{false};
    std::mutex stopMutex_;
    bool stopped_ = true;
};
