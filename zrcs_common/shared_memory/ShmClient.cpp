#include "ShmClient.h"

#include "ShmPlatform.h"

#include <cstdio>

namespace zrcs {

ShmClient::ShmClient(const char* name) noexcept
    : name_(name)
{
}

ShmClient::~ShmClient()
{
    detach();
}

bool ShmClient::attach() noexcept
{
    if (block_) {
        return true;
    }

    mapping_ = platformShmOpen(name_, kShmTotalSize, /*create=*/false, &handle_);
    if (!mapping_) {
        return false;
    }

    auto* hdr = static_cast<ShmHeader*>(mapping_);
    if (hdr->version != kShmVersion ||
        hdr->sizeof_block != static_cast<uint32_t>(sizeof(SharedBlock)) ||
        hdr->magic.load(std::memory_order_acquire) != kShmMagic) {
        std::fprintf(stderr,
            "[ShmClient] Shared memory '%s' is not ready or ABI mismatched "
            "(version=%u, block=%u, magic=0x%08x)\n",
            name_,
            hdr->version,
            hdr->sizeof_block,
            hdr->magic.load(std::memory_order_relaxed));
        detach();
        return false;
    }

    block_ = reinterpret_cast<SharedBlock*>(
        static_cast<char*>(mapping_) + kSharedBlockOffset);
    return true;
}

void ShmClient::detach() noexcept
{
    block_ = nullptr;
    platformShmClose(mapping_, kShmTotalSize, name_, /*unlink=*/false, handle_);
    mapping_ = nullptr;
    handle_ = nullptr;
}

}  // namespace zrcs
