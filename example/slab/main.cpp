#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
#include "../../include/common/Shared memory/nrt_process.hpp"


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
int sw=0;
int sp=0;
std::thread th=std::thread([&]()
{ 
    SingleAxisMotion sam;
    sam.axisId=0;
    sam.position=1;
    std::string cmd="ContinuousJog";
    Command cmd_;
    
    strncpy(cmd_.cmd, cmd.c_str(), sizeof(cmd_.cmd) - 1);
    cmd_.cmd[sizeof(cmd_.cmd) - 1] = '\0';  // Ensure null termination
    nrt_process.shared_block_->commandQueue.push(cmd_);
    while (true) 
    {
      if (sw==1&&sp==0) 
      {
             
        sam.velocity=2;
        sam.acceleration=0;
        nrt_process.shared_block_->manualPositionQueue.push(sam);
       // std::cout<<"--------------"<<std::endl;
         sam.position++;
      }
      if((sp==1)&&(sw==1))
      {
        std::cout<<"*************"<<std::endl;
        SingleAxisMotion sam1;
        sam1.position=sam.position;
        sam1.axisId=0;
        sam1.velocity=0;
        sam1.acceleration=0;
        nrt_process.shared_block_->manualPositionQueue.push(sam1);
        // std::string cmd="RemoveNode ContinuousJog";
        // Command cmd_;
        // strncpy(cmd_.cmd, cmd.c_str(), sizeof(cmd_.cmd) - 1);
        // cmd_.cmd[sizeof(cmd_.cmd) - 1] = '\0';  // Ensure null termination
        // nrt_process.shared_block_->commandQueue.push(cmd_);
        sp=0;
        sw=0;
      }
       std::this_thread::sleep_for(3000ms);
    }

});

    
    // 创建RT进程对象
while (true)
{

    //  nrt_process.shared_block_->registers.sysControl.store(control_flags);
      std::string cmd;
      std::getline(std::cin, cmd);
      if (cmd=="start") 
      {
          sw=1;
      }
      if (cmd=="stop")
      {
          sp=1;
      }
}



    // 初始化RT进程（连接到共享内存）

    // 在主线程中运行NRT进程
    th.join();
    std::cout << "IPC Demo completed successfully" << std::endl;
    return 0;
}