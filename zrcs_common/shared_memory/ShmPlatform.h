#pragma once

// ShmPlatform.h — 跨平台共享内存操作（基于 Boost.Interprocess）
//
// 使用 boost::interprocess::shared_memory_object + mapped_region 实现。
// 仅提供两个原语：打开/创建 + 关闭/删除。
// 调用方（RtProcess / NrtProcess）负责布局和构造。

#include <cstddef>

namespace zrcs {

// 打开或创建一个命名共享内存段，返回映射起始地址。
//
// @param name      共享内存名称（Windows: 内核对象名；POSIX: /name）
// @param size      映射大小（字节），必须与两侧一致
// @param create    true = NRT 侧：创建并零初始化；false = RT 侧：打开已有段
// @param out_handle 返回内部 ShmHandle*（堆分配的 Boost 对象包装），调用方必须
//                   保持它存活直到不再需要共享内存为止，析构时传给 platformShmClose。
//                   nullptr 时忽略（但映射会在函数返回后立即失效，仅用于 unlink 场景）。
// @return          映射起始地址；失败返回 nullptr
void* platformShmOpen(const char* name, size_t size, bool create,
                      void** out_handle = nullptr) noexcept;

// 解映射并可选删除命名共享内存段。
//
// @param addr    platformShmOpen 返回的地址（nullptr 时为空操作）
// @param size    映射大小（仅 POSIX munmap 需要，Windows 忽略）
// @param name    共享内存名称（仅 unlink=true 时使用）
// @param unlink  true = 删除内核对象（NRT 析构时调用）；false = 仅解映射（RT 析构时调用）
// @param handle  platformShmOpen 返回的 ShmHandle*（非 nullptr 时一并销毁）
void platformShmClose(void* addr, size_t size, const char* name, bool unlink,
                      void* handle = nullptr) noexcept;

}  // namespace zrcs
