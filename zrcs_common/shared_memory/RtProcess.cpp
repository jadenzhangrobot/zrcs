#include "RtProcess.h"
#include "ShmPlatform.h"

#include <cstdio>
#include <new>
#include <thread>
#include <chrono>

namespace zrcs {

RtProcess::RtProcess(const char* name) noexcept
    : name_(name)
{
}

RtProcess::~RtProcess()
{
    if (block_) {
        block_->~SharedBlock();
        block_ = nullptr;
    }
    // RT 不负责 unlink，NRT（创建方）负责
    platformShmClose(mapping_, kShmTotalSize, name_, /*unlink=*/false, handle_);
    mapping_ = nullptr;
    handle_  = nullptr;
}

bool RtProcess::initialize() noexcept
{
    // 重试打开 NRT 已创建的共享内存段
    for (int retry = 0; retry < kAttachRetries; ++retry) {
        mapping_ = platformShmOpen(name_, kShmTotalSize, /*create=*/false, &handle_);
        if (mapping_) break;
        std::fprintf(stdout, "[RtProcess] Waiting for shared memory '%s' (%d/%d)...\n",
                     name_, retry + 1, kAttachRetries);
        std::this_thread::sleep_for(std::chrono::milliseconds(kAttachRetryMs));
    }
    if (!mapping_) {
        std::fprintf(stderr, "[RtProcess] Failed to open shared memory '%s'\n", name_);
        return false;
    }

    // 校验 NRT 写入的 ABI 头部
    const auto* hdr = static_cast<const ShmHeader*>(mapping_);
    if (hdr->version != kShmVersion) {
        std::fprintf(stderr,
            "[RtProcess] ABI version mismatch: expected %u, got %u.\n",
            kShmVersion, hdr->version);
        return false;
    }
    if (hdr->sizeof_block != static_cast<uint32_t>(sizeof(SharedBlock))) {
        std::fprintf(stderr,
            "[RtProcess] SharedBlock size mismatch: expected %u, got %u.\n",
            static_cast<unsigned>(sizeof(SharedBlock)), hdr->sizeof_block);
        return false;
    }

    // 构造 SharedBlock（placement-new）
    void* block_addr = static_cast<char*>(mapping_) + kSharedBlockOffset;
    block_ = new (block_addr) SharedBlock{};

    // 最后以 release 语义写入 magic，通知 NRT "RT 初始化完成"
    auto* hdr_mut = static_cast<ShmHeader*>(mapping_);
    std::atomic_thread_fence(std::memory_order_release);
    hdr_mut->magic.store(kShmMagic, std::memory_order_release);

    std::fprintf(stdout, "[RtProcess] SharedBlock initialized in '%s' (v%u, block=%u bytes)\n",
                 name_, kShmVersion, static_cast<unsigned>(sizeof(SharedBlock)));
    return true;
}

}  // namespace zrcs
