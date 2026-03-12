/**
 * @file zmqClientTest.cpp
 * @brief Test program for ZMQ client - simulates upper computer sending commands
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "zmqClient.h"

int main(int argc, char **argv) 
{
    std::cout << "ZRCS ZMQ Client Test Started" << std::endl;

    ZMQClient client;
    if (!client.connect()) {
        std::cerr << "Failed to connect to ZMQ server" << std::endl;
        return 1;
    }

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 测试命令 1: MoveJ (关节运动)
    std::cout << "\n--- Test 1: MoveJ Command ---" << std::endl;
    client.sendCommand("MoveJ", {0.0, 0.0, 0.0, 0.0, 0.0, 0.0});

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 测试命令 2: MoveL (直线运动)
    std::cout << "\n--- Test 2: MoveL Command ---" << std::endl;
    client.sendCommand("MoveL", {100.0, 200.0, 300.0, 0.0, 0.0, 0.0});

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 测试命令 3: Stop (停止)
    std::cout << "\n--- Test 3: Stop Command ---" << std::endl;
    client.sendCommand("Stop");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 测试命令 4: Enable (使能)
    std::cout << "\n--- Test 4: Enable Command ---" << std::endl;
    client.sendCommand("Enable");

    std::cout << "\nAll tests completed" << std::endl;
    return 0;
}
