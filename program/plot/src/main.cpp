#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "zmqServer.h"
#include "common/sharedMemory/nrt_process.h"
int main() 
{

     NRTProcess nrt_process("rtMotion");
    
    // 初始化NRT进程（创建共享内存）
    if (!nrt_process.initialize()) 
    {
        std::cerr << "Failed to initialize NRT process" << std::endl;
        return 1;
    }
    ZmqServer zs;
    //  Prepare our context and publisher
   
    double timestamp = 0.0;
    while (true) {   
        nlohmann::json j;
        
        j["axis-1"] = radius * sin(phi) * cos(theta);
        j["axis-2"] = radius * sin(phi) * sin(theta);
        j["axis-3"] = radius * cos(phi);
        j["timestamp"] = timestamp;
        timestamp += 0.001;
        zs.send(j);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}