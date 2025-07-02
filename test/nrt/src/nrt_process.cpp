#include "nrt_process.hpp"

NRTProcess::NRTProcess(const char* shm_name) 
    : shm_name_(shm_name), shm_(nullptr), shared_block_(nullptr), initialized_(false) {
}

NRTProcess::~NRTProcess() {
    cleanup();
    delete shm_;
}

bool NRTProcess::initialize() {
    // 为防止上次异常退出，先尝试删除
    

    try {
        // 打开已存在的共享内存
        shm_ = new ipc::managed_shared_memory(ipc::open_only, shm_name_);
        shared_block_ = shm_->find<zrcsSystem::SharedBlock>("SharedBlock").first;
        if (!shared_block_) {
            std::cerr << "[RT Process] Cannot find SharedBlock. Exiting." << std::endl;
            return false;
        }
        
        std::cout << "[RT Process] Attached to shared memory. Starting control loop." << std::endl;
        initialized_ = true;
        return true;
    } catch (const ipc::interprocess_exception& e) {
        std::cerr << "[RT Process] Initialization error: " << e.what() << std::endl;
        return false;
    }
}


void NRTProcess::cleanup() {
    if (initialized_) {
        std::cout << "[NRT Process] Cleaning up shared memory." << std::endl;
        ipc::shared_memory_object::remove(shm_name_);
        initialized_ = false;
    }
}