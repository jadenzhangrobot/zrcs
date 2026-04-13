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
    block_ = nullptr;
    // NRT 是创建方，负责 unlink
    platformShmClose(mapping_, kShmTotalSize, name_, /*unlink=*/true, handle_);
    mapping_ = nullptr;
    handle_  = nullptr;
}

bool NrtProcess::initialize() noexcept
{
    // 创建共享内存段（清零）
    mapping_ = platformShmOpen(name_, kShmTotalSize, /*create=*/true, &handle_);
    if (!mapping_) {
        std::fprintf(stderr, "[NrtProcess] Failed to create shared memory '%s'\n", name_);
        return false;
    }

    // 写入 ABI 头部（magic 留 0，等 RT 写入）
    auto* hdr = static_cast<ShmHeader*>(mapping_);
    hdr->version      = kShmVersion;
    hdr->sizeof_block = static_cast<uint32_t>(sizeof(SharedBlock));

    std::fprintf(stdout, "[NrtProcess] Shared memory '%s' created (v%u, block=%u bytes)\n",
                 name_, kShmVersion, static_cast<unsigned>(sizeof(SharedBlock)));
    return true;
}

bool NrtProcess::waitForRt() noexcept
{
    if (!mapping_) return false;

    auto* hdr = static_cast<ShmHeader*>(mapping_);

    for (int retry = 0; retry < kAttachRetries; ++retry) {
        const uint32_t magic_val = hdr->magic.load(std::memory_order_acquire);
        if (magic_val == kShmMagic) {
            block_ = reinterpret_cast<SharedBlock*>(
                static_cast<char*>(mapping_) + kSharedBlockOffset);
            std::fprintf(stdout, "[NrtProcess] RT initialized shared memory '%s'\n", name_);
            return true;
        }
        std::fprintf(stdout, "[NrtProcess] Waiting for RT process (%d/%d)...\n",
                     retry + 1, kAttachRetries);
        std::this_thread::sleep_for(std::chrono::milliseconds(kAttachRetryMs));
    }

    std::fprintf(stderr, "[NrtProcess] RT did not initialize after %d retries.\n", kAttachRetries);
    return false;
}

}  // namespace zrcs
