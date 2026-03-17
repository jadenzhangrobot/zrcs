/**
 * @file main.cpp
 * @brief Non-Real-Time process entry point
 * @details Receives commands from upper computer via ZMQ and Protobuf,
 *          forwards them to real-time process via shared memory
 * @version 1.1
 * @date 2024
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "sharedMemory/sharedData.h"
#include "sharedMemory/shmConstants.h"
#include "zmqServer.h"

namespace ipc = boost::interprocess;

static std::atomic<bool> g_running{true};
static ZMQServer* g_zmq_server = nullptr;

static void signalHandler(int signum) {
    std::cout << "\n[NRT] Received signal " << signum << ", shutting down..." << std::endl;
    g_running = false;
    if (g_zmq_server) {
        g_zmq_server->stop();
    }
}

int main(int argc, char **argv)
{
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "ZRCS Non-Real-Time Process Started" << std::endl;
    try
    {
        // NRT 负责创建共享内存（RT 侧只做 find）
        ipc::managed_shared_memory shm(
            ipc::open_or_create,
            zrcs::SHM_NAME,
            zrcs::SHM_SIZE
        );

        // 创建 SharedBlock 并零初始化
        SharedBlock* shared_block = shm.find_or_construct<SharedBlock>(zrcs::SHM_BLOCK_NAME)();

        if (!shared_block) {
            std::cerr << "[NRT] Failed to create/find SharedBlock" << std::endl;
            return 1;
        }

        std::cout << "[NRT] SharedBlock initialized" << std::endl;

        // 初始化 ZMQ 服务器
        ZMQServer zmq_server(shared_block);
        g_zmq_server = &zmq_server;

        if (!zmq_server.initialize()) {
            std::cerr << "[NRT] Failed to initialize ZMQ server" << std::endl;
            return 1;
        }

        zmq_server.start();
        std::cout << "[NRT] ZMQ server started, waiting for commands..." << std::endl;

        // 主循环：监控共享内存状态
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 优雅关闭
        std::cout << "[NRT] Shutting down ZMQ server..." << std::endl;
        zmq_server.stop();
        g_zmq_server = nullptr;
        std::cout << "[NRT] Shutdown complete" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[NRT] Exception caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
