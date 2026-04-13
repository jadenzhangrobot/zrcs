#pragma once

// RtProcess.h — RT 进程侧：打开 NRT 创建的共享内存段并初始化 SharedBlock
//
// 职责：
//   1. 通过 platformShmOpen(create=false) 打开 NRT 已创建的段
//   2. 校验 ShmHeader 的 version 和 sizeof_block
//   3. 在固定偏移 kSharedBlockOffset 处 placement-new SharedBlock
//   4. 最后以 release 语义写入 magic → 通知 NRT 初始化完成
//   5. 析构时 placement-delete SharedBlock，platformShmClose(unlink=false)
//     （RT 不负责删除段，NRT 析构时删除）

#include "ShmLayout.h"

namespace zrcs {

class RtProcess {
public:
    explicit RtProcess(const char* name = kShmName) noexcept;
    ~RtProcess();

    // 打开共享内存段并构造 SharedBlock。失败时返回 false（调用方应终止进程）。
    bool initialize() noexcept;

    SharedBlock* sharedBlock() const noexcept { return block_; }

    RtProcess(const RtProcess&) = delete;
    RtProcess& operator=(const RtProcess&) = delete;

private:
    const char*  name_;
    void*        mapping_{nullptr};
    void*        handle_{nullptr};
    SharedBlock* block_{nullptr};
};

}  // namespace zrcs

// 向后兼容别名：现有代码可继续使用 RTProcess（不带命名空间限定）
using RTProcess = zrcs::RtProcess;
