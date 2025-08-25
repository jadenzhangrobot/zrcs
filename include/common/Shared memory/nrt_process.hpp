#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "sharedData.h"

namespace ipc = boost::interprocess;
using namespace std::chrono_literals;

class NRTProcess {
private:
    const char* shm_name_;
    ipc::managed_shared_memory* shm_;
    bool initialized_;
public:
    SharedBlock* shared_block_=nullptr;
    explicit NRTProcess(const char* shm_name = "MyMotionControlSHM"):shm_name_(shm_name), shm_(nullptr), initialized_(false),shared_block_(nullptr) 
    {

    }
    ~NRTProcess()
    {
        cleanup();
       delete shm_;
    }
    
    bool initialize()
    {       
        try {
            // 打开已存在的共享内存
            shm_ = new ipc::managed_shared_memory(ipc::open_only, shm_name_);
            shared_block_ = shm_->find<SharedBlock>("SharedBlock").first;
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
    void run()
    {

    }
    void cleanup()
    {
        if (initialized_) {
        std::cout << "[NRT Process] Cleaning up shared memory." << std::endl;
        ipc::shared_memory_object::remove(shm_name_);
        initialized_ = false;
    }
    }
    
    // 禁用拷贝构造和赋值
    NRTProcess(const NRTProcess&) = delete;
    NRTProcess& operator=(const NRTProcess&) = delete;
};