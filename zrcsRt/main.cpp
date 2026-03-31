/**
 * @file main.cpp
 * @author zhangyongjing (6499894200@qq.com)
 * @brief Real-Time process entry point
 * @version 1.1
 * @date 2024-11-13
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "sharedMemory/sharedData.h"
#include "controller/ControllerInterface.h"
#include "system/nodeManager.h"
#include "system/rtLog.h"
#include "config/projectConfig.h"
#include <thread>
#include <iostream>
#include "command/Cmdhead.h"

#ifdef __linux__
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#define ZRCS_PLATFORM_WINDOWS
#endif

int main(int argc, char **argv)
{
#ifdef __linux__
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        fprintf(stderr, "Error: mlockall failed: %s\n", strerror(errno));
        // 在生产环境中，实时性无法保证时应退出
        // exit(EXIT_FAILURE);
    }
    printf("Memory successfully locked.\n");
#endif

#ifdef ZRCS_PLATFORM_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "[RT] Running on Windows platform (no memory locking)" << std::endl;
#endif

    try
    {
        std::string projectName = zrcs::ProjectConfig::resolve();
        if (!projectName.empty()) {
            std::cout << "[RT] Active project: " << projectName << std::endl;
        }

        zrcsSystem::NodeManager nodeManager(projectName);
        nodeManager.run();

        // 检测共享内存中的 SHUTDOWN 信号
        auto* sharedBlock = nodeManager.rtProcess()->sharedBlock();
        while (true) {
            if (sharedBlock->cmd.load(std::memory_order_acquire) == TaskScheduling::SHUTDOWN) {
                INFO_PRINT("[RT] 收到 SHUTDOWN 信号, 正在退出...\n");
                std::cout << "[RT] Received SHUTDOWN from NRT, exiting..." << std::endl;
                nodeManager.stop();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception& e)
    {
        ERROR_PRINT("[RT] 致命异常: %s\n", e.what());
        std::cerr << "[RT] Exception caught: " << e.what() << std::endl;
    }
    return 0;
}
