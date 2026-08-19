#pragma once
/**
 * @file StatusPublisher.h
 * @brief 单一线程：从 RT 共享内存采集状态 → protobuf 序列化 → ZMQ PUB 发布给 GUI。
 *
 * 替代原先 StatusCollector + StatusStore + StatusPublisher 三件套的架构。
 * 线程模型：run() 在一个独立线程里循环，每 10ms 做一轮采集 + 序列化 + 发送。
 *
 * RT 日志由 RtLogConsumer 通过 enqueueRtLog() 推入内部队列，run() 周期搬空。
 */

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include <zmq.hpp>
#include "shared_memory/ShmLayout.h"
#include "status/StatusCollector.h"

class RtBridge;
class BehaviorTreeService;

class StatusPublisher {
public:
    /// @param bridge         RT 共享内存桥（非拥有）
    /// @param behaviorTree   行为树服务（非拥有，可为空）
    StatusPublisher(RtBridge* bridge, BehaviorTreeService* behaviorTree = nullptr);
    ~StatusPublisher();

    bool initialize();
    void start();
    /// 线程安全、可重入：多次调用只清理一次。
    void stop();

    /// 追加一条 RT 日志到待发布队列（由 RtLogConsumer 回调调用，线程安全）。
    void enqueueRtLog(const zrcs::RtLogEntry& entry);

private:
    void run();

    static constexpr const char* PUB_ENDPOINT = "tcp://*:5556";
    static constexpr int PUB_INTERVAL_MS = 10;
    static constexpr size_t kMaxPendingLogs = 256;

    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> pub_socket_;
    StatusCollector collector_;          ///< 数据采集（与序列化/发送分离）

    /// RT 日志待发布队列（RtLogConsumer 回调写入，run() 搬空）。
    mutable std::mutex logMutex_;
    std::deque<zrcs::RtLogEntry> pendingLogs_;

    std::thread pub_thread_;
    std::atomic<bool> running_{false};
    std::mutex stopMutex_;
    bool stopped_ = true;
};
