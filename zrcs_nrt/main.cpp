/**
 * @file main.cpp
 * @brief Non-Real-Time process entry point
 * @details Receives commands from upper computer via ZMQ and Protobuf,
 *          forwards them to real-time process via shared memory
 * @version 1.2
 * @date 2024
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <string>
#include <filesystem>
#include "NrtLogger.h"
#include "BtEngine.h"
#include "RtLogConsumer.h"
#include "zmq_server/ZmqServer.h"
#include "status_publisher/StatusPublisher.h"
#include "terminal/TerminalConsole.h"
#include "rt_bridge/RtBridge.h"
#include "shared_memory/NrtProcess.h"
#include "shared_memory/ShmLayout.h"
#include "config/ProjectConfig.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

static std::atomic<bool> g_running{true};
static ZMQServer* g_zmq_server = nullptr;

#ifdef _WIN32
static HANDLE g_rt_process = nullptr;

// Windows 控制台事件处理（捕获关闭窗口、Ctrl+C 等）
// 注意：此回调在系统线程中执行，不可调用 spdlog 等可能持锁的函数
static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        g_running = false;
        if (g_zmq_server) {
            g_zmq_server->stop();
        }
        // 关闭窗口时必须在此处终止 RT，因为之后进程可能被强杀
        if (g_rt_process) {
            TerminateProcess(g_rt_process, 0);
            WaitForSingleObject(g_rt_process, 3000);
            CloseHandle(g_rt_process);
            g_rt_process = nullptr;
        }
        return TRUE;
    default:
        return FALSE;
    }
}
#else
static pid_t g_rt_pid = -1;
#endif

static void signalHandler(int signum) {
    spdlog::warn("Received signal {}, shutting down...", signum);
    g_running = false;
    if (g_zmq_server) {
        g_zmq_server->stop();
    }
}

static void cleanupSharedMemory()
{
    // Shared memory cleanup is handled by RtProcess (the creator).
    // NRT side only detaches; nothing to unlink here.
}

// 启动 RT 子进程，返回是否成功
static bool launchRTProcess()
{
    cleanupSharedMemory();

    // 获取当前可执行文件所在目录，RT 进程应在同一目录
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string dir(exePath);
    dir = dir.substr(0, dir.find_last_of("\\/") + 1);
    std::string rtPath = dir + zrcs::kRtProcessName + ".exe";

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessA(
            rtPath.c_str(),   // 应用程序路径
            nullptr,          // 命令行
            nullptr, nullptr, // 安全属性
            FALSE,            // 不继承句柄
            0,                // 创建标志
            nullptr,          // 环境变量
            dir.c_str(),      // 工作目录
            &si, &pi))
    {
        spdlog::error("Failed to launch RT process: {} (error {})", rtPath, GetLastError());
        return false;
    }

    CloseHandle(pi.hThread);
    g_rt_process = pi.hProcess;
    spdlog::info("RT process launched (PID {}): {}", pi.dwProcessId, rtPath);
    return true;

#else
    // Linux: fork + exec
    char exePath[PATH_MAX] = {0};
    ssize_t exeLen = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (exeLen <= 0) {
        spdlog::error("Failed to resolve current executable path");
        return false;
    }
    exePath[exeLen] = '\0';

    std::filesystem::path execDir = std::filesystem::path(exePath).parent_path();
    std::string rtPath = (execDir / zrcs::kRtProcessName).string();

    g_rt_pid = fork();
    if (g_rt_pid < 0) {
        spdlog::error("Failed to fork RT process");
        return false;
    }
    if (g_rt_pid == 0) {
        // 子进程
        if (chdir(execDir.c_str()) != 0) {
            spdlog::error("Failed to change directory to {}", execDir.string());
            _exit(1);
        }
        execl(rtPath.c_str(), zrcs::kRtProcessName, nullptr);
        // execl 失败
        spdlog::error("Failed to exec RT process: {}", rtPath);
        _exit(1);
    }
    spdlog::info("RT process launched (PID {}): {}", g_rt_pid, rtPath);
    return true;
#endif
}

// 终止 RT 子进程
// bridge 非空时先通过 RtBridge 通知 RT 正常退出，超时后强杀
static void terminateRTProcess(RtBridge* bridge = nullptr)
{
    // 1. 通过 RtBridge 通知 RT 正常退出
    if (bridge) {
        spdlog::info("Sending SHUTDOWN to RT via RtBridge...");
        bridge->requestShutdown();
    }

#ifdef _WIN32
    if (g_rt_process) {
        // 2. 等待 RT 自行退出（最多 3 秒）
        DWORD waitResult = WaitForSingleObject(g_rt_process, 3000);
        if (waitResult == WAIT_OBJECT_0) {
            spdlog::info("RT process exited gracefully.");
        } else {
            // 3. 超时，强制终止
            spdlog::warn("RT process did not exit in time, force terminating...");
            TerminateProcess(g_rt_process, 0);
            WaitForSingleObject(g_rt_process, 2000);
            spdlog::info("RT process force terminated.");
        }
        CloseHandle(g_rt_process);
        g_rt_process = nullptr;
    }
#else
    if (g_rt_pid > 0) {
        // 2. 等待 RT 自行退出（最多 3 秒）
        int status;
        bool exited = false;
        for (int i = 0; i < 30; ++i) {
            pid_t ret = waitpid(g_rt_pid, &status, WNOHANG);
            if (ret == g_rt_pid) {
                exited = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (exited) {
            spdlog::info("RT process exited gracefully.");
        } else {
            // 3. 超时，强制终止
            spdlog::warn("RT process did not exit in time, sending SIGKILL...");
            kill(g_rt_pid, SIGKILL);
            waitpid(g_rt_pid, &status, 0);
            spdlog::info("RT process force terminated.");
        }
        g_rt_pid = -1;
    }
#endif

    cleanupSharedMemory();
}

int main(int argc, char **argv)
{
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef _WIN32
    // Windows 控制台设置为 UTF-8 输出，避免中文日志乱码
    SetConsoleOutputCP(CP_UTF8);
    // Windows 下必须用 SetConsoleCtrlHandler 捕获关闭窗口事件
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

    std::cout << "ZRCS Non-Real-Time Process Started" << std::endl;
    try
    {
        // 初始化日志系统
        NrtLogger::init();
        spdlog::info("ZRCS Non-Real-Time Process Started");

        // 解析项目配置
        std::string projectName = zrcs::ProjectConfig::resolve();
        if (!projectName.empty()) {
            spdlog::info("Active project: {}", projectName);
        }

        // 启动 RT 子进程（创建共享内存）
        if (!launchRTProcess()) {
            spdlog::critical("Failed to launch RT process, exiting.");
            return 1;
        }

        // 等待 RT 进程创建共享内存并 attach
        NRTProcess nrt_process;
        if (!nrt_process.initialize()) {
            spdlog::critical("Failed to initialize shared memory");
            terminateRTProcess();
            return 1;
        }

        spdlog::info("SharedBlock initialized");

        // 创建 RtBridge（NRT→RT 共享内存通信的唯一入口）
        RtBridge bridge(nrt_process.sharedBlock());

        // 启动 RT 日志消费者（从共享内存读取 RT 日志并写入 spdlog）
        RtLogConsumer rtLogConsumer(nrt_process.sharedBlock());
        rtLogConsumer.start();
        spdlog::info("RT log consumer started");

        // 初始化行为树引擎
        BTEngine bt_engine(&bridge);
        spdlog::info("BTEngine initialized");

        // 初始化 ZMQ 服务器
        ZMQServer zmq_server(&bridge, &bt_engine);
        g_zmq_server = &zmq_server;

        if (!zmq_server.initialize()) {
            spdlog::critical("Failed to initialize ZMQ server");
            terminateRTProcess();
            return 1;
        }

        zmq_server.start();
        spdlog::info("ZMQ server started, waiting for commands...");

        // 初始化状态发布器 (ZMQ PUB on port 5556)
        StatusPublisher status_publisher(&bridge);
        if (status_publisher.initialize()) {
            status_publisher.start();
            spdlog::info("Status publisher started (PUB on port 5556)");
        } else {
            spdlog::warn("Status publisher failed to initialize, continuing without it");
        }

        // 初始化终端控制台
        TerminalConsole terminal(&bridge, &bt_engine, g_running);
        if (terminal.initialize()) {
            terminal.start();
            spdlog::info("Terminal console started");
        } else {
            spdlog::warn("Terminal console not available, continuing without it");
        }

        // 主循环：监控共享内存状态
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 优雅关闭
        spdlog::info("Stopping status publisher...");
        status_publisher.stop();

        spdlog::info("Stopping RT log consumer...");
        rtLogConsumer.stop();

        spdlog::info("Shutting down terminal console...");
        terminal.stop();

        spdlog::info("Shutting down ZMQ server...");
        zmq_server.stop();
        g_zmq_server = nullptr;

        // 终止 RT 子进程（通过 RtBridge 通知正常退出）
        terminateRTProcess(&bridge);

        spdlog::info("Shutdown complete");
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Exception caught: {}", e.what());
        terminateRTProcess();
        return 1;
    }

    return 0;
}
