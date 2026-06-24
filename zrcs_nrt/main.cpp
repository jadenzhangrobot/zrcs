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
#include <cmath>
#include <vector>
#include "log/NrtLogger.h"
#include "log/RtLogConsumer.h"
#include "zmq_server/ZmqServer.h"
#include "statusPublisher/StatusPublisher.h"
#include "rtBridge/RtBridge.h"
#include "shared_memory/NrtProcess.h"
#include "shared_memory/ShmLayout.h"
#include "config/ProjectConfig.h"
#include "motion/MotionPreprocessor.h"
#include "behavior_tree/BehaviorTreeRunner.h"
#include "mujoco_identify/MujocoIdentifyWorker.h"

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

// 统一信号处理（Ctrl+C / SIGTERM），两平台共用
// 注意：信号处理器和 consoleCtrlHandler 中不可调用 spdlog 等可能持锁的函数
static void signalHandler(int signum) {
    std::fprintf(stdout, "[main] Received signal %d, shutting down...\n", signum);
    g_running = false;
    if (g_zmq_server) {
        g_zmq_server->stop();
    }
}

#ifdef _WIN32
static HANDLE g_rt_process = nullptr;

// Windows 控制台事件处理——仅处理关闭窗口/注销/关机等 std::signal 无法捕获的事件
// Ctrl+C 由 signalHandler(SIGINT) 统一处理，控制台返回 FALSE 让默认行为触发 SIGINT
// 注意：此回调在系统线程中执行，不可调用 spdlog 等可能持锁的函数
static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        // 关闭窗口/注销/关机：进程即将被系统强杀（~5s 超时），必须在此处清理 RT
        g_running = false;
        if (g_zmq_server) {
            g_zmq_server->stop();
        }
        if (g_rt_process) {
            // 先给 RT 500ms 自行退出，超时再强杀
            WaitForSingleObject(g_rt_process, 500);
            TerminateProcess(g_rt_process, 0);
            WaitForSingleObject(g_rt_process, 3000);
            CloseHandle(g_rt_process);
            g_rt_process = nullptr;
        }
        return TRUE;
    default:
        // CTRL_C_EVENT / CTRL_BREAK_EVENT 不处理，让默认行为触发 SIGINT 走 signalHandler
        return FALSE;
    }
}
#else
static pid_t g_rt_pid = -1;
#endif


// 启动 RT 子进程，返回是否成功
static bool launchRTProcess()
{

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

}

static bool runMotionPreprocessorDemo(MotionPreprocessor& motionPreprocessor)
{
    constexpr double visualScale = 50.0;
    MotionPreprocessor::Config cfg;
    cfg.maxVel = 80.0 * visualScale;
    cfg.maxAccel = 300.0 * visualScale;
    cfg.maxJerk = 8000.0 * visualScale;
    cfg.cornerTol = 0.1 * visualScale;
    cfg.galvoMode = false;

    // 蝴蝶形点位 — 先注释，用共线点测试弧长累加
    auto wp = [](double x, double y, double z) {
        return Point3D{x * visualScale, y * visualScale, z * visualScale};
    };
    const std::vector<Point3D> waypoints = {
       wp( 0.0,  4.2, 0.0),
       wp(-0.8,  3.4, 0.0),
       wp(-2.4,  4.6, 0.0),
       wp(-4.5,  5.2, 0.0),
       wp(-6.2,  4.0, 0.0),
       wp(-5.2,  2.0, 0.0),
       wp(-3.4,  0.7, 0.0),
       wp(-5.5, -1.2, 0.0),
       wp(-4.5, -3.8, 0.0),
       wp(-2.4, -3.1, 0.0),
       wp(-0.8, -1.4, 0.0),
       wp( 0.0, -3.6, 0.0),
       wp( 0.8, -1.4, 0.0),
       wp( 2.4, -3.1, 0.0),
       wp( 4.5, -3.8, 0.0),
       wp( 5.5, -1.2, 0.0),
       wp( 3.4,  0.7, 0.0),
       wp( 5.2,  2.0, 0.0),
       wp( 6.2,  4.0, 0.0),
       wp( 4.5,  5.2, 0.0),
       wp( 2.4,  4.6, 0.0),
       wp( 0.8,  3.4, 0.0),
       wp( 0.0,  4.2, 0.0),
    };

    // 共线测试点：沿 X 轴等距排列
    // const std::vector<Point3D> waypoints = {
    //     {0.0, 0.0, 0.0},
    //     {1.0, 0.0, 0.0},
    //     {2.0, 0.0, 0.0},
    //     {3.0, 0.0, 0.0},
    //     {4.0, 0.0, 0.0},
    //     {5.0, 0.0, 0.0},
    // };

    spdlog::info("[MotionPreprocessorDemo] Sending {} waypoints through real RtBridge", waypoints.size());
    if (!motionPreprocessor.process(waypoints, 0.1, -0.2, 0.3, cfg)) {
        spdlog::error("[MotionPreprocessorDemo] Failed to queue demo path");
        return false;
    }

    spdlog::info("[MotionPreprocessorDemo] Demo path queued to RT cmdQueue");
    return true;
}

int main(int argc, char **argv)
{
    // std::signal 两平台共用，统一处理 Ctrl+C / SIGTERM
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef _WIN32
    // Windows 控制台设置为 UTF-8 输出，避免中文日志乱码
    SetConsoleOutputCP(CP_UTF8);
    // 额外注册控制台事件处理器，捕获关闭窗口等 std::signal 无法处理的事件
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#else
    // Linux：关闭终端窗口时内核发送 SIGHUP，需要优雅清理 RT 子进程
    std::signal(SIGHUP, signalHandler);
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

        // NRT 侧创建共享内存段（RT 子进程将 open 并初始化 SharedBlock）
        NRTProcess nrt_process;
        if (!nrt_process.initialize()) {
            spdlog::critical("Failed to initialize shared memory");
            return 1;
        }

        // 启动 RT 子进程（打开已创建的共享内存）
        if (!launchRTProcess()) {
            spdlog::critical("Failed to launch RT process, exiting.");
            return 1;
        }

        // 等待 RT 子进程写入 magic（SharedBlock 就绪）
        if (!nrt_process.waitForRt()) {
            spdlog::critical("RT process did not initialize shared memory");
            terminateRTProcess();
            return 1;
        }

        spdlog::info("SharedBlock initialized");

        // 创建 RtBridge（NRT→RT 共享内存通信的唯一入口）
        RtBridge bridge(nrt_process.sharedBlock());
        BehaviorTreeRunner behaviorTreeRunner(&bridge);
        auto mujocoIdentifyOptions = zrcs_nrt::parseMujocoIdentifyOptions(argc, argv);
        zrcs_nrt::MujocoIdentifyWorker mujocoIdentifyWorker(
            nrt_process.sharedBlock(), mujocoIdentifyOptions);
        mujocoIdentifyWorker.start();

        // 初始化 ZMQ 服务器
        ZMQServer zmq_server(&bridge, &behaviorTreeRunner);
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

        // 启动 RT 日志消费者：写入 NRT 日志，并转发给状态发布器供 GUI 分流显示。
        RtLogConsumer rtLogConsumer(
            nrt_process.sharedBlock(),
            [&status_publisher](const zrcs::RtLogEntry& entry) {
                status_publisher.enqueueRtLog(entry);
            });
        rtLogConsumer.start();
        spdlog::info("RT log consumer started");

        // 初始化运动预处理器
       MotionPreprocessor motion_preprocessor(&bridge);
       (void)runMotionPreprocessorDemo(motion_preprocessor);

        // 主循环：监控共享内存状态
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 优雅关闭
        spdlog::info("Stopping MuJoCo identification worker...");
        mujocoIdentifyWorker.stop();

        spdlog::info("Stopping status publisher...");
        status_publisher.stop();

        spdlog::info("Stopping RT log consumer...");
        rtLogConsumer.stop();

        spdlog::info("Stopping behavior tree runner...");
        behaviorTreeRunner.stop("Process shutdown");

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
