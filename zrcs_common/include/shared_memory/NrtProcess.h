#pragma once
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "SharedData.h"
#include "ShmConstants.h"

namespace ipc = boost::interprocess;

class NRTProcess {
private:
    const char* shm_name_;
    std::unique_ptr<ipc::managed_shared_memory> shm_;
    bool initialized_;
    SharedBlock* shared_block_;

public:
    explicit NRTProcess(const char* shm_name = zrcs::SHM_NAME)
        : shm_name_(shm_name), shm_(nullptr), initialized_(false), shared_block_(nullptr)
    {
    }

    ~NRTProcess() = default;

    bool initialize()
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

    SharedBlock* sharedBlock() const { return shared_block_; }

    // 禁用拷贝构造和赋值
    NRTProcess(const NRTProcess&) = delete;
    NRTProcess& operator=(const NRTProcess&) = delete;
};
