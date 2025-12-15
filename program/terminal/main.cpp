#include <array>
#include <iostream>
#include <thread>
#include <chrono>
#include "common/sharedMemory/nrt_process.h"
#include "common/cmdline.h"


using namespace std::chrono_literals;

int main() {
    cmdline::parser port_input;
    port_input.add<std::string>("ip", '\0', "ip address", true);
    port_input.add<int>("port", '\0', "port number", true);
    std::string ip = port_input.get<std::string>("ip");
    int port = port_input.get<int>("port");

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
      std::string cmd;
      std::cout<<"----------"<<std::endl;
      std::getline(std::cin, cmd);     
      Command cmd_;
      std::cout<<cmd<<std::endl;
      strncpy(cmd_.cmd, cmd.c_str(), sizeof(cmd_.cmd) - 1);
      cmd_.cmd[sizeof(cmd_.cmd) - 1] = '\0';  // Ensure null termination
     // cmd_.jogabsj_.position=100.0;
      //nrt_process.shared_block_->commandQueue.push(cmd_);
      std::this_thread::sleep_for(10ms);
}
    
    // 初始化RT进程（连接到共享内存）

    // 在主线程中运行NRT进程
    
    std::cout << "IPC Demo completed successfully" << std::endl;
    return 0;
}