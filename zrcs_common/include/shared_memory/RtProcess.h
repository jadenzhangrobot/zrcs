#pragma once

#include <iostream>
#include <memory>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "SharedData.h"
#include "ShmConstants.h"

namespace ipc = boost::interprocess;

class RTProcess {
private:
    const char* shm_name_;
    std::unique_ptr<ipc::managed_shared_memory> shm_;
    SharedBlock* shared_block_;

public:
    explicit RTProcess(const char* shm_name = zrcs::SHM_NAME)
        : shm_name_(shm_name), shm_(nullptr), shared_block_(nullptr)
    {
    }

    ~RTProcess()
    {
        shm_.reset();
        ipc::shared_memory_object::remove(shm_name_);
    }

    bool initialize()
    {
        ipc::shared_memory_object::remove(shm_name_);
        try {
            shm_ = std::make_unique<ipc::managed_shared_memory>(
                ipc::create_only, shm_name_, zrcs::SHM_SIZE);
            shared_block_ = shm_->find_or_construct<SharedBlock>(zrcs::SHM_BLOCK_NAME)();

            std::cout << "[RT Process] Shared memory created." << std::endl;
            return true;
        } catch (const ipc::interprocess_exception& e) {
            std::cerr << "[RT Process] Initialization error: " << e.what() << std::endl;
            return false;
        }
    }

    SharedBlock* sharedBlock() const { return shared_block_; }

    // Disable copy constructor and assignment
    RTProcess(const RTProcess&) = delete;
    RTProcess& operator=(const RTProcess&) = delete;
};
