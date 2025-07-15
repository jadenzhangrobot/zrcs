#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "../include/common/Shared memory/sharedData.h"

namespace ipc = boost::interprocess;
using namespace std::chrono_literals;

class NRTProcess {
private:
    const char* shm_name_;
    ipc::managed_shared_memory* shm_;
    bool initialized_;
public:
    SharedBlock* shared_block_=nullptr;
    explicit NRTProcess(const char* shm_name = "MyMotionControlSHM");
    ~NRTProcess();
    
    bool initialize();
    void run();
    void cleanup();
    
    // 禁用拷贝构造和赋值
    NRTProcess(const NRTProcess&) = delete;
    NRTProcess& operator=(const NRTProcess&) = delete;
};