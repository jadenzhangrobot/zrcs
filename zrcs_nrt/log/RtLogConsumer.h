#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

#include "shared_memory/ShmLayout.h"

/**
 * @brief NRT 侧 RT 日志消费者
 *
 * 从共享内存 logQueue 消费 RT 进程日志，通过 spdlog 输出。
 * 独立线程运行，默认 10ms 轮询间隔。
 */
class RtLogConsumer {
public:
    explicit RtLogConsumer(
        zrcs::SharedBlock* block,
        std::function<void(const zrcs::RtLogEntry&)> log_callback = {},
        std::chrono::milliseconds poll_interval = std::chrono::milliseconds(10));

    ~RtLogConsumer();

    RtLogConsumer(const RtLogConsumer&) = delete;
    RtLogConsumer& operator=(const RtLogConsumer&) = delete;

    void start();
    void stop();

private:
    void run();
    void dispatch(const zrcs::RtLogEntry& entry);

    zrcs::ShmSPSCConsumer<zrcs::RtLogEntry, zrcs::kLogQueueCap> logConsumer_;
    std::function<void(const zrcs::RtLogEntry&)> log_callback_;
    std::chrono::milliseconds poll_interval_;
    std::atomic<bool>         running_{false};
    std::thread               thread_;
};
