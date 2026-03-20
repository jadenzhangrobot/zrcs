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
#else
static pid_t g_rt_pid = -1;
#endif

static void signalHandler(int signum) {
    std::cout << "\n[NRT] Received signal " << signum << ", shutting down..." << std::endl;
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
        std::cerr << "[NRT] Failed to launch RT process: " << rtPath
                  << " (error " << GetLastError() << ")" << std::endl;
        return false;
    }

    CloseHandle(pi.hThread);
    g_rt_process = pi.hProcess;
    std::cout << "[NRT] RT process launched (PID " << pi.dwProcessId << "): " << rtPath << std::endl;
    return true;

#else
    // Linux: fork + exec
    std::string rtPath = std::string("./") + zrcs::RT_PROCESS_NAME;
    g_rt_pid = fork();
    if (g_rt_pid < 0) {
        std::cerr << "[NRT] Failed to fork RT process" << std::endl;
        return false;
    }
    if (g_rt_pid == 0) {
        // 子进程
        execl(rtPath.c_str(), zrcs::RT_PROCESS_NAME, nullptr);
        // execl 失败
        std::cerr << "[NRT-child] Failed to exec RT process: " << rtPath << std::endl;
        _exit(1);
    }
    std::cout << "[NRT] RT process launched (PID " << g_rt_pid << ")" << std::endl;
    return true;
#endif
}

// 终止 RT 子进程
static void terminateRTProcess()
{
#ifdef _WIN32
    if (g_rt_process) {
        TerminateProcess(g_rt_process, 0);
        WaitForSingleObject(g_rt_process, 3000);
        CloseHandle(g_rt_process);
        g_rt_process = nullptr;
        std::cout << "[NRT] RT process terminated." << std::endl;
    }
#else
    if (g_rt_pid > 0) {
        kill(g_rt_pid, SIGTERM);
        int status;
        waitpid(g_rt_pid, &status, 0);
        g_rt_pid = -1;
        std::cout << "[NRT] RT process terminated." << std::endl;
    }
#endif
}

int main(int argc, char **argv)
{
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "ZRCS Non-Real-Time Process Started" << std::endl;
    try
    {
        // 启动 RT 子进程（创建共享内存）
        if (!launchRTProcess()) {
            std::cerr << "[NRT] Failed to launch RT process, exiting." << std::endl;
            return 1;
        }

        // 等待 RT 进程创建共享内存并 attach
        NRTProcess nrt_process;
        if (!nrt_process.initialize()) {
            std::cerr << "[NRT] Failed to initialize shared memory" << std::endl;
            terminateRTProcess();
            return 1;
        }

        std::cout << "[NRT] SharedBlock initialized" << std::endl;

        // 初始化行为树引擎
        BTEngine bt_engine(nrt_process.sharedBlock());
        std::cout << "[NRT] BTEngine initialized" << std::endl;

        // 初始化 ZMQ 服务器
        ZMQServer zmq_server(nrt_process.sharedBlock(), &bt_engine);
        g_zmq_server = &zmq_server;

        if (!zmq_server.initialize()) {
            std::cerr << "[NRT] Failed to initialize ZMQ server" << std::endl;
            terminateRTProcess();
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

        // 终止 RT 子进程
        terminateRTProcess();

        std::cout << "[NRT] Shutdown complete" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[NRT] Exception caught: " << e.what() << std::endl;
        terminateRTProcess();
        return 1;
    }

    return 0;
}
