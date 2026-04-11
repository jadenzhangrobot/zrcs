#pragma once

#include <cstddef>
#include <atomic>

// 共享内存名称和配置常量，NRT/RT/GUI 统一引用
namespace zrcs {

constexpr const char* SHM_NAME        = "rtMotion";
constexpr const char* SHM_BLOCK_NAME  = "SharedBlock";
constexpr size_t      SHM_SIZE        = 4 * 1024 * 1024;  // 4MB

// NRT 等待 RT 共享内存就绪的超时设置
constexpr int SHM_WAIT_RETRY_COUNT    = 30;     // 最大重试次数
constexpr int SHM_WAIT_RETRY_MS       = 1000;   // 每次重试间隔 (ms)

// RT 进程可执行文件名
constexpr const char* RT_PROCESS_NAME = "zrcsrt";

}  // namespace zrcs

// 统一使用 C++17 的 is_always_lock_free 静态常量进行编译期检查
static_assert(std::atomic<int>::is_always_lock_free, "atomic<int> must be lock-free for cross-process safety");
static_assert(std::atomic<bool>::is_always_lock_free, "atomic<bool> must be lock-free for cross-process safety");
static_assert(std::atomic<uint64_t>::is_always_lock_free, "atomic<uint64_t> must be lock-free for cross-process safety");
static_assert(std::atomic<uint8_t>::is_always_lock_free, "atomic<uint8_t> must be lock-free for cross-process safety");
static_assert(std::atomic<double>::is_always_lock_free, "atomic<double> must be lock-free for cross-process safety");

constexpr size_t AxisMaxCount = 50;         // 最大轴数量
