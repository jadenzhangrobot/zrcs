#include "NrtProcess.h"
#include "ShmPlatform.h"

#include <cstdio>
#include <thread>
#include <chrono>

namespace zrcs {

NrtProcess::NrtProcess(const char* name) noexcept
    : name_(name)
{
}

NrtProcess::~NrtProcess()
{
    platformShmClose(mapping_, kShmTotalSize, name_, /*unlink=*/false);
    mapping_ = nullptr;
    block_   = nullptr;
}

bool NrtProcess::tryAttach() noexcept
{
    void* addr = platformShmOpen(name_, kShmTotalSize, /*create=*/false);
    if (!addr) return false;  // 段尚未创建，调用方重试

    const auto* hdr = static_cast<const ShmHeader*>(addr);

    // acquire 读 magic：若 RT 尚未完成 SharedBlock 构造，magic 仍为 0
    if (hdr->magic.load(std::memory_order_acquire) != kShmMagic) {
        platformShmClose(addr, kShmTotalSize, name_, /*unlink=*/false);
        return false;
    }

    // 版本校验
    if (hdr->version != kShmVersion) {
        std::fprintf(stderr,
            "[NrtProcess] ABI version mismatch: expected %u, got %u. "
            "Please rebuild both RT and NRT from the same source.\n",
            kShmVersion, hdr->version);
        platformShmClose(addr, kShmTotalSize, name_, /*unlink=*/false);
        return false;
    }

    // 布局大小校验
    if (hdr->sizeof_block != static_cast<uint32_t>(sizeof(SharedBlock))) {
        std::fprintf(stderr,
            "[NrtProcess] SharedBlock size mismatch: expected %u, got %u.\n",
            static_cast<unsigned>(sizeof(SharedBlock)), hdr->sizeof_block);
        platformShmClose(addr, kShmTotalSize, name_, /*unlink=*/false);
        return false;
    }

    mapping_ = addr;
    block_   = reinterpret_cast<SharedBlock*>(static_cast<char*>(addr) + kSharedBlockOffset);
    return true;
}

bool NrtProcess::initialize() noexcept
{
    for (int retry = 0; retry < kAttachRetries; ++retry) {
        if (tryAttach()) {
            std::fprintf(stdout, "[NrtProcess] Attached to shared memory '%s'\n", name_);
            return true;
        }
        std::fprintf(stdout, "[NrtProcess] Waiting for RT process (%d/%d)...\n",
                     retry + 1, kAttachRetries);
        std::this_thread::sleep_for(std::chrono::milliseconds(kAttachRetryMs));
    }
    std::fprintf(stderr, "[NrtProcess] Failed to attach after %d retries.\n", kAttachRetries);
    return false;
}

}  // namespace zrcs
