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
// @param name      共享内存名称（Windows: 内核对象名；POSIX: /name）
// @param size      映射大小（字节），必须与两侧一致
// @param create    true = RT 侧：创建并零初始化；false = NRT 侧：打开已有段
// @param out_handle 仅 Windows RT 侧（create=true）时有意义：
//                   返回 CreateFileMappingA 的 HANDLE，调用方必须保持它开启
//                   直到不再需要跨进程访问为止（即进程退出或主动调用 platformShmClose）。
//                   其他情况填 nullptr 忽略。
// @return          映射起始地址；失败返回 nullptr
void* platformShmOpen(const char* name, size_t size, bool create,
                      void** out_handle = nullptr) noexcept;

// 解映射并可选删除命名共享内存段。
//
// @param addr    platformShmOpen 返回的地址（nullptr 时为空操作）
// @param size    映射大小（仅 POSIX munmap 需要，Windows 忽略）
// @param name    共享内存名称（仅 unlink=true 时使用）
// @param unlink  true = 删除内核对象（RT 析构时调用）；false = 仅解映射（NRT 析构时调用）
// @param handle  Windows RT 侧保存的 hMap 句柄（非 nullptr 时一并关闭）；其他传 nullptr
void platformShmClose(void* addr, size_t size, const char* name, bool unlink,
                      void* handle = nullptr) noexcept;

}  // namespace zrcs
