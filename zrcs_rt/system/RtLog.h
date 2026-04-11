#pragma once

// RtLog.h — RT 侧实时日志
//
// g_logQueue 指向进程本地的 ShmSPSCProducer，由 NodeManager::run() 初始化。
// RT 线程通过 INFO_PRINT / WARN_PRINT / ERROR_PRINT 宏写日志（栈上格式化，无动态分配）。
// 队列满时静默丢弃（不阻塞实时循环）。

#include <cstring>
#include <cstdint>
#include <chrono>
#include "shared_memory/ShmLayout.h"

#ifdef REALTIME
#include <cobalt/stdio.h>
#define RT_PRINTF rt_printf
#else
#include <cstdio>  // IWYU pragma: keep — printf is used via RT_PRINTF macro
#define RT_PRINTF printf
#endif

namespace zrcs {
namespace rtlog {

enum Level : uint8_t { INFO = 0, WARN = 1, ERR = 2 };

// 进程本地生产者指针（不在共享内存中）
inline ShmSPSCProducer<RtLogEntry, kLogQueueCap>* g_logQueue = nullptr;

inline void setLogQueue(ShmSPSCProducer<RtLogEntry, kLogQueueCap>* p) {
    g_logQueue = p;
}

inline const char* extractFilename(const char* path) {
    const char* name = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') name = p + 1;
    }
    return name;
}

inline uint64_t nowMicroseconds() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

inline void logPush(Level level, const char* file, uint16_t line, const char* msg) {
    if (!g_logQueue) return;

    RtLogEntry entry;
    entry.timestamp_us = nowMicroseconds();
    entry.level        = static_cast<uint8_t>(level);
    entry._pad[0]      = 0;
    entry.line         = line;

    const char* fname = extractFilename(file);
    std::strncpy(entry.file, fname, sizeof(entry.file) - 1);
    entry.file[sizeof(entry.file) - 1] = '\0';

    std::strncpy(entry.message, msg, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    g_logQueue->push(entry);  // 队列满时静默丢弃
}

} // namespace rtlog
} // namespace zrcs

// ─────────────────────────────────────────────────────────────────────────────
// 日志宏（与原有签名兼容）
// ─────────────────────────────────────────────────────────────────────────────

#define RT_LOG_IMPL_(level, fmt, ...) \
    do { \
        char rt_log_buf_[192]; \
        std::snprintf(rt_log_buf_, sizeof(rt_log_buf_), fmt, ##__VA_ARGS__); \
        if (zrcs::rtlog::g_logQueue) { \
            zrcs::rtlog::logPush(level, __FILE__, \
                static_cast<uint16_t>(__LINE__), rt_log_buf_); \
        } else { \
            RT_PRINTF("%s", rt_log_buf_); \
        } \
    } while (0)

#define INFO_PRINT(fmt, ...) \
    RT_LOG_IMPL_(zrcs::rtlog::Level::INFO, fmt, ##__VA_ARGS__)

#define WARN_PRINT(fmt, ...) \
    RT_LOG_IMPL_(zrcs::rtlog::Level::WARN, "[WARN] %s:%d:%s(): " fmt, \
                 __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define ERROR_PRINT(fmt, ...) \
    RT_LOG_IMPL_(zrcs::rtlog::Level::ERR, "[ERROR] %s:%d:%s(): " fmt, \
                 __FILE__, __LINE__, __func__, ##__VA_ARGS__)
