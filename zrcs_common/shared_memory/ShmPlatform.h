#pragma once

// ShmPlatform.h — 跨平台原始共享内存操作
//
// 替代 boost::interprocess::managed_shared_memory。
// 仅提供两个原语：打开/创建 + 关闭/删除。
// 调用方（RtProcess / NrtProcess）负责布局和构造。

#include <cstddef>

namespace zrcs {

// 打开或创建一个命名共享内存段，返回映射起始地址。
//
// @param name    共享内存名称（Windows: 内核对象名；POSIX: /name）
// @param size    映射大小（字节），必须与两侧一致
// @param create  true = RT 侧：创建并零初始化；false = NRT 侧：打开已有段
// @return        映射起始地址；失败返回 nullptr
void* platformShmOpen(const char* name, size_t size, bool create) noexcept;

// 解映射并可选删除命名共享内存段。
//
// @param addr    platformShmOpen 返回的地址（nullptr 时为空操作）
// @param size    映射大小（仅 POSIX munmap 需要，Windows 忽略）
// @param name    共享内存名称（仅 unlink=true 时使用）
// @param unlink  true = 删除内核对象（RT 析构时调用）；false = 仅解映射（NRT 析构时调用）
void platformShmClose(void* addr, size_t size, const char* name, bool unlink) noexcept;

}  // namespace zrcs
