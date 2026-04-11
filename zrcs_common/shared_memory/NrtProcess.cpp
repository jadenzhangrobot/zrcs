#include "NrtProcess.h"

#include <iostream>
#include <thread>
#include <chrono>

NRTProcess::NRTProcess(const char* shm_name)
    : shm_name_(shm_name), shm_(nullptr), initialized_(false), shared_block_(nullptr)
{
}

bool NRTProcess::initialize()
{
    for (int retry = 0; retry < zrcs::SHM_WAIT_RETRY_COUNT; ++retry) {
        try {
            shm_ = std::make_unique<ipc::managed_shared_memory>(ipc::open_only, shm_name_);
            shared_block_ = shm_->find<SharedBlock>(zrcs::SHM_BLOCK_NAME).first;
            if (shared_block_) {
                std::cout << "[NRT Process] Attached to shared memory." << std::endl;
                initialized_ = true;
                return true;
            }
            std::cerr << "[NRT Process] SharedBlock not found, retrying ("
                      << retry + 1 << "/" << zrcs::SHM_WAIT_RETRY_COUNT << ")..." << std::endl;
        } catch (const ipc::interprocess_exception&) {
            std::cerr << "[NRT Process] Waiting for RT process ("
                      << retry + 1 << "/" << zrcs::SHM_WAIT_RETRY_COUNT << "): "
                      << "shared memory not ready" << std::endl;
        }
        shm_.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(zrcs::SHM_WAIT_RETRY_MS));
    }
    std::cerr << "[NRT Process] Failed to attach after "
              << zrcs::SHM_WAIT_RETRY_COUNT << " retries." << std::endl;
    return false;
}

SharedBlock* NRTProcess::sharedBlock() const
{
    return shared_block_;
}
