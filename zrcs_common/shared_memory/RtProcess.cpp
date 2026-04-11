#include "RtProcess.h"

#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace ipc = boost::interprocess;

RTProcess::RTProcess(const char* shm_name)
    : shm_name_(shm_name), shm_(nullptr), shared_block_(nullptr)
{
}

RTProcess::~RTProcess()
{
    shm_.reset();
    ipc::shared_memory_object::remove(shm_name_);
}

bool RTProcess::initialize()
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

SharedBlock* RTProcess::sharedBlock() const
{
    return shared_block_;
}
