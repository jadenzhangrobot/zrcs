#pragma once

// NrtProcess.h — NRT 进程侧：附加到 RT 创建的共享内存段
//
// 职责：
//   1. 通过 platformShmOpen(create=false) 尝试打开段
//   2. 校验 ShmHeader：magic == kShmMagic（RT 已完成初始化）
//                      version == kShmVersion（ABI 版本匹配）
//                      sizeof_block == sizeof(SharedBlock)（布局一致）
//   3. 重试最多 kAttachRetries 次（间隔 kAttachRetryMs ms），等待 RT 启动
//   4. 析构时 platformShmClose(unlink=false)（NRT 不负责删除段，RT 析构时删除）

#include "ShmLayout.h"

namespace zrcs {

class NrtProcess {
public:
    explicit NrtProcess(const char* name = kShmName) noexcept;
    ~NrtProcess();

    // 尝试附加，超时或版本不匹配时返回 false。
    bool initialize() noexcept;

    SharedBlock* sharedBlock() const noexcept { return block_; }

    NrtProcess(const NrtProcess&) = delete;
    NrtProcess& operator=(const NrtProcess&) = delete;

private:
    // 单次尝试：打开段 + 校验头部。返回 true 表示成功附加。
    bool tryAttach() noexcept;

    const char*  name_;
    void*        mapping_{nullptr};
    SharedBlock* block_{nullptr};
};

}  // namespace zrcs

// 向后兼容别名
using NRTProcess = zrcs::NrtProcess;
