#pragma once

#include <cstddef>

// 共享内存名称和配置常量，NRT/RT/GUI 统一引用
namespace zrcs {

constexpr const char* SHM_NAME        = "rtMotion";
constexpr const char* SHM_BLOCK_NAME  = "SharedBlock";
constexpr size_t      SHM_SIZE        = 4 * 1024 * 1024;  // 4MB

// RT 进程等待共享内存就绪的超时设置
constexpr int SHM_WAIT_RETRY_COUNT    = 30;     // 最大重试次数
constexpr int SHM_WAIT_RETRY_MS       = 1000;   // 每次重试间隔 (ms)

}  // namespace zrcs
