#pragma once

// RtProcess.h — RT 进程侧：创建并拥有共享内存段
//
// 职责：
//   1. 通过 platformShmOpen 创建 kShmTotalSize 字节的共享内存段（清零）
//   2. 在固定偏移 kSharedBlockOffset 处 placement-new SharedBlock
//   3. 写入 ShmHeader（version、sizeof_block），最后以 release 语义写入 magic
//      → magic 作为 NRT 侧的 "RT 已初始化完成" 信号
//   4. 析构时 placement-delete SharedBlock，platformShmClose(unlink=true)

#include "ShmLayout.h"

namespace zrcs {

class RtProcess {
public:
    explicit RtProcess(const char* name = kShmName) noexcept;
    ~RtProcess();

    // 创建共享内存段并构造 SharedBlock。失败时返回 false（调用方应终止进程）。
    bool initialize() noexcept;

    SharedBlock* sharedBlock() const noexcept { return block_; }

    RtProcess(const RtProcess&) = delete;
    RtProcess& operator=(const RtProcess&) = delete;

private:
    const char*  name_;
    void*        mapping_{nullptr};
    SharedBlock* block_{nullptr};
};

}  // namespace zrcs

// 向后兼容别名：现有代码可继续使用 RTProcess（不带命名空间限定）
using RTProcess = zrcs::RtProcess;
