#pragma once

// NrtProcess.h — NRT 进程侧：创建共享内存段，等待 RT 初始化
//
// 职责：
//   1. 通过 platformShmOpen(create=true) 创建 kShmTotalSize 字节的共享内存段（清零）
//   2. 写入 ShmHeader 的 version 和 sizeof_block（但不写 magic）
//   3. 等待 RT 子进程 open 段、构造 SharedBlock 并写入 magic
//   4. 校验 magic == kShmMagic 后开始使用 SharedBlock
//   5. 析构时 platformShmClose(unlink=true)（NRT 是创建方，负责销毁段）

#include "ShmLayout.h"

namespace zrcs {

class NrtProcess {
public:
    explicit NrtProcess(const char* name = kShmName) noexcept;
    ~NrtProcess();

    // 创建共享内存段并写入 ABI 头部（不写 magic）。失败时返回 false。
    bool initialize() noexcept;

    // 等待 RT 子进程写入 magic（SharedBlock 就绪）。超时返回 false。
    bool waitForRt() noexcept;

    SharedBlock* sharedBlock() const noexcept { return block_; }

    NrtProcess(const NrtProcess&) = delete;
    NrtProcess& operator=(const NrtProcess&) = delete;

private:
    const char*  name_;
    void*        mapping_{nullptr};
    void*        handle_{nullptr};
    SharedBlock* block_{nullptr};
};

}  // namespace zrcs

// 向后兼容别名
using NRTProcess = zrcs::NrtProcess;
