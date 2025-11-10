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
        std::array<double, 100> axisPosition;
        while(nrt_process.shared_block_->cmdAxisPositionQueue.pop(axisPosition))
        {
            j["axis-1"] = axisPosition[0];
            j["axis-2"] = axisPosition[1];
            j["axis-3"] = axisPosition[2];
            j["timestamp"] = timestamp;
            timestamp += 0.001;
            zs.send(j);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}