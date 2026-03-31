#pragma once

#include <atomic>
#include <thread>
#include <chrono>
#include <cstring>
#include <spdlog/spdlog.h>
#include "sharedMemory/sharedData.h"

/**
 * @brief NRT 侧 RT 日志消费者
 *
 * 从共享内存 logQueue 中读取 RT 进程日志，通过 spdlog 输出到控制台和文件。
 * 独立线程运行，默认 10ms 轮询间隔。
 */
class RtLogConsumer {
public:
    explicit RtLogConsumer(SharedBlock* block,
                           std::chrono::milliseconds poll_interval = std::chrono::milliseconds(10))
        : block_(block), poll_interval_(poll_interval) {}

    ~RtLogConsumer() { stop(); }

    RtLogConsumer(const RtLogConsumer&) = delete;
    RtLogConsumer& operator=(const RtLogConsumer&) = delete;

    void start() {
        if (running_.load(std::memory_order_relaxed)) return;
        running_.store(true, std::memory_order_release);
        thread_ = std::thread([this]() { run(); });
    }

    void stop() {
        running_.store(false, std::memory_order_release);
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    void run() {
        RtLogEntry entry;
        while (running_.load(std::memory_order_acquire)) {
            size_t drained = 0;
            while (block_->logQueue.pop(entry)) {
                dispatch(entry);
                ++drained;
                if (drained >= 64) break;
            }
            if (drained == 0) {
                std::this_thread::sleep_for(poll_interval_);
            }
        }
        // 退出前排空剩余日志
        while (block_->logQueue.pop(entry)) {
            dispatch(entry);
        }
    }

    void dispatch(const RtLogEntry& entry) {
        char msg[sizeof(entry.message) + 1];
        std::memcpy(msg, entry.message, sizeof(entry.message));
        msg[sizeof(entry.message)] = '\0';

        char file[sizeof(entry.file) + 1];
        std::memcpy(file, entry.file, sizeof(entry.file));
        file[sizeof(entry.file)] = '\0';

        switch (entry.level) {
        case 0:
            spdlog::info("[RT] [{}:{}] {}", file, entry.line, msg);
            break;
        case 1:
            spdlog::warn("[RT] [{}:{}] {}", file, entry.line, msg);
            break;
        case 2:
            spdlog::error("[RT] [{}:{}] {}", file, entry.line, msg);
            break;
        default:
            spdlog::info("[RT] [L{}] [{}:{}] {}", entry.level, file, entry.line, msg);
            break;
        }
    }

    SharedBlock*                block_;
    std::chrono::milliseconds   poll_interval_;
    std::atomic<bool>           running_{false};
    std::thread                 thread_;
};
