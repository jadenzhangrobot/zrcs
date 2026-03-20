#pragma once
#include <iostream>
#include <memory>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "sharedData.h"
#include "shmConstants.h"

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
        try {
            shm_ = std::make_unique<ipc::managed_shared_memory>(ipc::open_only, shm_name_);
            shared_block_ = shm_->find<SharedBlock>(zrcs::SHM_BLOCK_NAME).first;
            if (!shared_block_) {
                std::cerr << "[NRT Process] Cannot find SharedBlock. Exiting." << std::endl;
                return false;
            }

            std::cout << "[NRT Process] Attached to shared memory." << std::endl;
            initialized_ = true;
            return true;
        } catch (const ipc::interprocess_exception& e) {
            std::cerr << "[NRT Process] Initialization error: " << e.what() << std::endl;
            return false;
        }
    }

    SharedBlock* sharedBlock() const { return shared_block_; }

    // 禁用拷贝构造和赋值
    NRTProcess(const NRTProcess&) = delete;
    NRTProcess& operator=(const NRTProcess&) = delete;
};
