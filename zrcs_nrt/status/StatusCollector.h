#pragma once
/**
 * @file StatusCollector.h
 * @brief 唯一从 RtBridge 采集轴反馈与任务状态并写入 StatusStore 的线程。
 */

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "shared_memory/ShmLayout.h"

class RtBridge;
class StatusStore;

class StatusCollector {
public:
    StatusCollector(RtBridge* bridge,
                    StatusStore* store,
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
    std::chrono::milliseconds interval_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};
