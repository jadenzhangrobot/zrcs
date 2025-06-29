#pragma once

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "shared_data.hpp"

namespace ipc = boost::interprocess;
using namespace std::chrono_literals;

class RTProcess {
private:
    const char* shm_name_;
    ipc::managed_shared_memory* shm_;
    SharedBlock* shared_block_;
    bool initialized_;
    bool running_;
    
    // 运动控制状态
    Status rt_status_;
    double target_position_;
    uint64_t status_updates_;

public:
    RTProcess(const char* shm_name) 
    : shm_name_(shm_name), shm_(nullptr), shared_block_(nullptr)

{
    
}

    ~RTProcess()
    {
        stop();
        delete shm_;
    }
    
    bool initialize()
    {
         ipc::shared_memory_object::remove(shm_name_);
        try {
            // 创建共享内存和 SharedBlock 对象
            shm_ = new ipc::managed_shared_memory(ipc::create_only, shm_name_, 65536);
            shared_block_ = shm_->find_or_construct<SharedBlock>("SharedBlock")();
            
            std::cout << "[NRT Process] Shared memory created. Starting simulation." << std::endl;
            initialized_ = true;
            return true;
        } catch (const ipc::interprocess_exception& e) {
            std::cerr << "[NRT Process] Initialization error: " << e.what() << std::endl;
            return false;
        }
    }
    void stop()
    {
         running_ = false;
    }
    
    // 禁用拷贝构造和赋值
    RTProcess(const RTProcess&) = delete;
    RTProcess& operator=(const RTProcess&) = delete;
};