/// @file StatusPublisher.h
/// @brief 状态发布器 —— 将 RT 轴反馈数据通过 ZMQ PUB 推送给 GUI

#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <zmq.hpp>

#include "shared_memory/ShmLayout.h"

class RtBridge;

/**
 * @brief 状态发布器 (ZMQ PUB)
 * @details 从共享内存读取 RT 轴位置数据，通过 ZMQ PUB socket 发布给 GUI
 *          发布频率: 100Hz (10ms)，数据格式: SystemStatus protobuf
 */
class StatusPublisher
{
public:
    /// @param bridge RT 桥接指针，用于读取共享内存中的轴反馈
    explicit StatusPublisher(RtBridge* bridge);

    /// @brief 析构，自动停止发布线程
    ~StatusPublisher();

    /// @brief 初始化 ZMQ PUB socket 并绑定端口
    /// @return true 初始化成功；false 绑定失败（端口占用等）
    bool initialize();

    /// @brief 启动发布线程（幂等，重复调用安全）
    void start();

    /// @brief 停止发布线程并关闭 socket（幂等，可在析构外主动调用）
    void stop();

    /// @brief 投递一条 RT 日志，将随下一次状态发布一并发送给 GUI
    void enqueueRtLog(const zrcs::RtLogEntry& entry);

private:
    static constexpr const char* PUB_ENDPOINT = "tcp://*:5556";
    static constexpr int PUB_INTERVAL_MS = 10;
    static constexpr size_t kMaxPendingRtLogs = 256;

    std::vector<zrcs::RtLogEntry> drainPendingRtLogs();
    void run();

    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> pub_socket_;
    RtBridge* bridge_;
    std::thread pub_thread_;
    std::atomic<bool> running_;
    std::mutex log_mutex_;
    std::deque<zrcs::RtLogEntry> pending_logs_;
};
