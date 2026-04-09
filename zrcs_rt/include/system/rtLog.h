
#pragma once

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <chrono>
#include "shared_memory/SharedData.h"

#ifdef REALTIME
#include <cobalt/stdio.h>
#define RT_PRINTF rt_printf
#else
#define RT_PRINTF printf
#endif

namespace zrcs {
namespace rtlog {

// 日志级别
enum Level : uint8_t { INFO = 0, WARN = 1, ERR = 2 };

// 全局日志队列指针，RT 进程初始化时设置
// RT 是单线程实时循环，无需 atomic
inline SPSCRingBuffer<RtLogEntry, LOG_BUFFER_SIZE>* g_logQueue = nullptr;

inline void setLogQueue(SPSCRingBuffer<RtLogEntry, LOG_BUFFER_SIZE>* q) {
    g_logQueue = q;
}

// 从完整路径提取文件名
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

// 核心日志写入 -- RT-safe: 栈上格式化 + atomic store
inline void logPush(Level level, const char* file, uint16_t line,
                    const char* msg)
{
    if (!g_logQueue) return;

    RtLogEntry entry;
    entry.timestamp_us = nowMicroseconds();
    entry.level = static_cast<uint8_t>(level);
    entry.padding[0] = 0;
    entry.line = line;

    const char* fname = extractFilename(file);
    std::strncpy(entry.file, fname, sizeof(entry.file) - 1);
    entry.file[sizeof(entry.file) - 1] = '\0';

    std::strncpy(entry.message, msg, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    g_logQueue->push(entry); // 队列满时静默丢弃
}

} // namespace rtlog
} // namespace zrcs

// ===================================================================
// 兼容宏 -- 保持与原有 INFO_PRINT/WARN_PRINT/ERROR_PRINT 相同签名
// 有 logQueue 时写入共享内存; 无 logQueue 时降级为 printf
// ===================================================================

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
