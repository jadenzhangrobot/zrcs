#include "RtProcess.h"
#include "ShmPlatform.h"

#include <cstdio>
#include <new>

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
    platformShmClose(mapping_, kShmTotalSize, name_, /*unlink=*/true);
    mapping_ = nullptr;
}

bool RtProcess::initialize() noexcept
{
    // 清理旧残留段（正常情况 RT 是首先启动的一方）
    platformShmClose(nullptr, 0, name_, /*unlink=*/true);

    mapping_ = platformShmOpen(name_, kShmTotalSize, /*create=*/true);
    if (!mapping_) {
        std::fprintf(stderr, "[RtProcess] Failed to create shared memory '%s'\n", name_);
        return false;
    }

    // 构造 SharedBlock（placement-new）
    void* block_addr = static_cast<char*>(mapping_) + kSharedBlockOffset;
    block_ = new (block_addr) SharedBlock{};

    // 填写 ABI 头部（magic 最后写入，充当初始化完成信号）
    auto* hdr = static_cast<ShmHeader*>(mapping_);
    hdr->version      = kShmVersion;
    hdr->sizeof_block = static_cast<uint32_t>(sizeof(SharedBlock));
    // release fence：保证 SharedBlock 构造和 header 字段完全可见后再写 magic
    std::atomic_thread_fence(std::memory_order_release);
    hdr->magic.store(kShmMagic, std::memory_order_release);

    std::fprintf(stdout, "[RtProcess] Shared memory '%s' created (v%u, block=%u bytes)\n",
                 name_, kShmVersion, static_cast<unsigned>(sizeof(SharedBlock)));
    return true;
}

}  // namespace zrcs
