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
    //  Prepare our context and publisher
   
    double timestamp = 0.0;
    while (true) {   
        nlohmann::json j;
        double radius = 1.0;
        double theta = timestamp * 2.0; // 经度角
        double phi = timestamp * 1.0;  // 纬度角
        j["x"] = radius * sin(phi) * cos(theta);
        j["y"] = radius * sin(phi) * sin(theta);
        j["z"] = radius * cos(phi);
        j["timestamp"] = timestamp;
        timestamp += 0.1;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}