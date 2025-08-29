#include <array>
#include <iostream>
#include <thread>
#include <chrono>
#include "../../include/common/Shared memory/nrt_process.h"

using namespace std::chrono_literals;

int main() {
    std::cout << "Starting IPC Demo with C++ Classes" << std::endl;
    
    // 创建NRT进程对象
    NRTProcess nrt_process("rtMotion");
    
    // 初始化NRT进程（创建共享内存）
    if (!nrt_process.initialize()) 
    {
        std::cerr << "Failed to initialize NRT process" << std::endl;
        return 1;
    }
   

// 移除未使用的变量 control

    
    // 创建RT进程对象
while (true)
{
      std::array<bool, 100> control_flags={};
      control_flags[5] = true;
    //  nrt_process.shared_block_->registers.sysControl.store(control_flags);
      std::string cmd;
      std::cout<<"----------"<<std::endl;
      std::getline(std::cin, cmd);     
      Command cmd_;
      std::cout<<cmd<<std::endl;
      strncpy(cmd_.cmd, cmd.c_str(), sizeof(cmd_.cmd) - 1);
      cmd_.cmd[sizeof(cmd_.cmd) - 1] = '\0';  // Ensure null termination
      nrt_process.shared_block_->commandQueue.push(cmd_);
      std::this_thread::sleep_for(10ms);
}
    
    // 初始化RT进程（连接到共享内存）

    // 在主线程中运行NRT进程
    
    std::cout << "IPC Demo completed successfully" << std::endl;
    return 0;
}