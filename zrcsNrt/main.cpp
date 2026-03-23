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
#include "nrtLogger.h"
#include "btEngine.h"
#include "zmqServer/zmqServer.h"
#include "sharedMemory/nrt_process.h"
#include "sharedMemory/shmConstants.h"

#ifdef _WIN32
#include <windows.h>
#else
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

// 启动 RT 子进程，返回是否成功
static bool launchRTProcess()
{
    // 获取当前可执行文件所在目录，RT 进程应在同一目录
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string dir(exePath);
    dir = dir.substr(0, dir.find_last_of("\\/") + 1);
    std::string rtPath = dir + zrcs::RT_PROCESS_NAME + ".exe";

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
    std::string rtPath = std::string("./") + zrcs::RT_PROCESS_NAME;
    g_rt_pid = fork();
    if (g_rt_pid < 0) {
        spdlog::error("Failed to fork RT process");
        return false;
    }
    if (g_rt_pid == 0) {
        // 子进程
        execl(rtPath.c_str(), zrcs::RT_PROCESS_NAME, nullptr);
        // execl 失败
        spdlog::error("Failed to exec RT process: {}", rtPath);
        _exit(1);
    }
    spdlog::info("RT process launched (PID {})", g_rt_pid);
    return true;
#endif
}

// 终止 RT 子进程
// shared_block 非空时先通过共享内存通知 RT 正常退出，超时后强杀
static void terminateRTProcess(SharedBlock* shared_block = nullptr)
{
    // 1. 通过共享内存通知 RT 正常退出
    if (shared_block) {
        spdlog::info("Sending SHUTDOWN to RT via shared memory...");
        shared_block->cmd.store(TaskScheduling::SHUTDOWN, std::memory_order_release);
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
}

int main(int argc, char **argv)
{
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef _WIN32
    // Windows 下必须用 SetConsoleCtrlHandler 捕获关闭窗口事件
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

    std::cout << "ZRCS Non-Real-Time Process Started" << std::endl;
    try
    {
        // 初始化日志系统
        NrtLogger::init();
        spdlog::info("ZRCS Non-Real-Time Process Started");

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

        // 初始化行为树引擎
        BTEngine bt_engine(nrt_process.sharedBlock());
        spdlog::info("BTEngine initialized");

        // 初始化 ZMQ 服务器
        ZMQServer zmq_server(nrt_process.sharedBlock(), &bt_engine);
        g_zmq_server = &zmq_server;

        if (!zmq_server.initialize()) {
            spdlog::critical("Failed to initialize ZMQ server");
            terminateRTProcess();
            return 1;
        }

        zmq_server.start();
        spdlog::info("ZMQ server started, waiting for commands...");

        // 主循环：监控共享内存状态
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 优雅关闭
        spdlog::info("Shutting down ZMQ server...");
        zmq_server.stop();
        g_zmq_server = nullptr;

        // 终止 RT 子进程（通过共享内存通知正常退出）
        terminateRTProcess(nrt_process.sharedBlock());

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
