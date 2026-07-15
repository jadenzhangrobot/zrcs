#pragma once
/**
 * @file StatusCollector.h
 * @brief 从 RtBridge / BehaviorTreeService 采集状态并写入 StatusStore 的线程。
 */

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "shared_memory/ShmLayout.h"

class RtBridge;
class StatusStore;
class BehaviorTreeService;

class StatusCollector {
public:
    StatusCollector(RtBridge* bridge,
                    StatusStore* store,
                    BehaviorTreeService* behaviorTree = nullptr,
                    std::chrono::milliseconds interval = std::chrono::milliseconds(10));
    ~StatusCollector();

    StatusCollector(const StatusCollector&) = delete;
    StatusCollector& operator=(const StatusCollector&) = delete;

    void start();
    void stop();

private:
    void run();
    static std::string taskStateToString(zrcs::TaskScheduling ts);

    RtBridge* bridge_;
    StatusStore* store_;
    BehaviorTreeService* behaviorTree_;
    std::chrono::milliseconds interval_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};
