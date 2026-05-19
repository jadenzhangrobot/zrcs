#pragma once

#include "ShmLayout.h"

namespace zrcs {

class ShmClient {
public:
    explicit ShmClient(const char* name = kShmName) noexcept;
    ~ShmClient();

    bool attach() noexcept;
    void detach() noexcept;

    SharedBlock* sharedBlock() const noexcept { return block_; }
    bool isAttached() const noexcept { return block_ != nullptr; }

    ShmClient(const ShmClient&) = delete;
    ShmClient& operator=(const ShmClient&) = delete;

private:
    const char*  name_;
    void*        mapping_{nullptr};
    void*        handle_{nullptr};
    SharedBlock* block_{nullptr};
};

}  // namespace zrcs
