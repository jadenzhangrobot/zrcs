/**
 * @file main.cpp
 * @brief Non-Real-Time process entry point
 * @details Receives commands from upper computer via ZMQ and Protobuf,
 *          forwards them to real-time process via shared memory
 * @version 1.0
 * @date 2024
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "sharedMemory/sharedData.h"
#include "zmqServer.h"
#include "motion/PathPreprocessor.h"
namespace ipc = boost::interprocess;

int main(int argc, char **argv) 
{
     std::vector<Point3D> rawPoints = {
    {50.00, 65.00, 0}, {54.45, 64.12, 0}, {58.20, 61.35, 0}, {60.15, 57.12, 0},
    {59.80, 52.45, 0}, {64.12, 50.12, 0}, {70.50, 52.35, 0}, {78.20, 58.12, 0},
    {85.45, 62.45, 0}, {88.12, 55.20, 0}, {85.35, 45.15, 0}, {78.12, 38.45, 0},
    {68.50, 35.12, 0}, {62.15, 30.45, 0}, {65.45, 20.12, 0}, {72.12, 12.35, 0},
    {80.50, 8.12, 0},  {75.35, 2.45, 0},  {65.12, 5.12, 0},  {55.45, 12.35, 0},
    {50.00, 22.00, 0}, {44.55, 12.35, 0}, {34.88, 5.12, 0},  {24.65, 2.45, 0},
    {19.50, 8.12, 0},  {27.88, 12.35, 0}, {34.55, 20.12, 0}, {37.85, 30.45, 0},
    {31.50, 35.12, 0}, {21.88, 38.45, 0}, {14.65, 45.15, 0}, {11.88, 55.20, 0},
    {14.55, 62.45, 0}, {21.80, 58.12, 0}, {29.50, 52.35, 0}, {35.88, 50.12, 0},
    {40.20, 52.45, 0}, {39.85, 57.12, 0}, {41.80, 61.35, 0}, {45.55, 64.12, 0},
    {50.00, 65.00, 0}
};

    // 2. 实例化预处理器
    PathPreprocessor processor;

    // 3. 执行三次样条拟合与重采样
    // 建议 stepSize 设为 0.5mm 到 1.0mm，具体取决于你的加工精度需求
    double stepSize = 0.5; 
    std::vector<Point3D> smoothPath = processor.processWithSpline(rawPoints, stepSize);
    VelocityPlanner3D planner;
    planner.setConfig(100.0, 500.0, 0.0, 0.0);

    // 场景：
    // 1. 地面直线加速 (0,0,0) -> (50,0,0)
    // 2. 开始爬坡 (50,0,0) -> (100,0,50) [Z轴上升]
    // 3. 坡顶急转弯 (100,0,50) -> (100,50,50) [Z轴不变，XY平面转弯]
    for (const auto& pt : smoothPath) {
    planner.addPoint(pt.x, pt.y, pt.z);
}
  
    if (planner.plan()) {
        planner.printReport("3D_planning_report.txt", rawPoints);
    }

    std::cout << "ZRCS Non-Real-Time Process Started" << std::endl;   
    try 
    {
        // 打开或创建共享内存
        ipc::managed_shared_memory shm(
            ipc::open_or_create,
            "MyMotionControlSHM",
            65536  // 64KB
        );

        // 查找或创建 SharedBlock
        SharedBlock* shared_block = shm.find_or_construct<SharedBlock>("SharedBlock")();
        
        if (!shared_block) {
            std::cerr << "[NRT] Failed to create/find SharedBlock" << std::endl;
            return 1;
        }

        std::cout << "[NRT] SharedBlock initialized" << std::endl;

        // 初始化 ZMQ 服务器
        ZMQServer zmq_server(shared_block);
        if (!zmq_server.initialize()) {
            std::cerr << "[NRT] Failed to initialize ZMQ server" << std::endl;
            return 1;
        }

        zmq_server.start();
        std::cout << "[NRT] ZMQ server started, waiting for commands..." << std::endl;

        // 主循环：监控共享内存状态
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 可选：定期输出心跳或状态信息
            // std::cout << "[NRT] Heartbeat: " << shared_block->heartBeat << std::endl;
        }
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "[NRT] Exception caught: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
