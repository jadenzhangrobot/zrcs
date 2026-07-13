#include "log/RtLogConsumer.h"

#include <cstring>

#include <spdlog/spdlog.h>

RtLogConsumer::RtLogConsumer(
    zrcs::SharedBlock* block,
    std::function<void(const zrcs::RtLogEntry&)> log_callback,
    std::chrono::milliseconds poll_interval)
    : logConsumer_(block->logQueue)
    , log_callback_(std::move(log_callback))
    , poll_interval_(poll_interval)
{
}

RtLogConsumer::~RtLogConsumer()
{
    stop();
}

void RtLogConsumer::start()
{
    if (running_.load(std::memory_order_relaxed)) return;
    running_.store(true, std::memory_order_release);
    thread_ = std::thread([this]() { run(); });
}

void RtLogConsumer::stop()
{
    running_.store(false, std::memory_order_release);
    if (thread_.joinable()) thread_.join();
}

void RtLogConsumer::run()
{
    zrcs::RtLogEntry entry;
    while (running_.load(std::memory_order_acquire))
    {
        size_t drained = 0;
        while (logConsumer_.pop(entry))
        {
            dispatch(entry);
            if (++drained >= 64) break;
        }
        if (drained == 0)
            std::this_thread::sleep_for(poll_interval_);
    }
    // 退出前排空剩余日志
    while (logConsumer_.pop(entry)) dispatch(entry);
}

void RtLogConsumer::dispatch(const zrcs::RtLogEntry& entry)
{
    char msg[sizeof(entry.message) + 1];
    std::memcpy(msg, entry.message, sizeof(entry.message));
    msg[sizeof(entry.message)] = '\0';

    char file[sizeof(entry.file) + 1];
    std::memcpy(file, entry.file, sizeof(entry.file));
    file[sizeof(entry.file)] = '\0';

    switch (entry.level)
    {
    case 0:  spdlog::info ("[RT] [{}:{}] {}", file, entry.line, msg); break;
    case 1:  spdlog::warn ("[RT] [{}:{}] {}", file, entry.line, msg); break;
    case 2:  spdlog::error("[RT] [{}:{}] {}", file, entry.line, msg); break;
    default: spdlog::info ("[RT] [L{}] [{}:{}] {}", entry.level, file, entry.line, msg); break;
    }

    if (log_callback_)
    {
        log_callback_(entry);
    }
}
