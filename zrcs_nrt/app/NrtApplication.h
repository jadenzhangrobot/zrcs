#pragma once
/**
 * @file NrtApplication.h
 * @brief NRT 进程生命周期：装配依赖、启动/停止工作线程与 RT 子进程。
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "shared_memory/NrtProcess.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/types.h>
#endif

class RtBridge;
class CommandService;
class TaskService;
class CommandRouter;
class BehaviorTreeService;
class BehaviorTreeCommandService;
class ZMQServer;
class StatusPublisher;
class RtLogConsumer;

namespace zrcs_nrt {
class MujocoIdentifyWorker;
}

class NrtApplication {
public:
    NrtApplication(int argc, char** argv);
    ~NrtApplication();

    NrtApplication(const NrtApplication&) = delete;
    NrtApplication& operator=(const NrtApplication&) = delete;

    int run();

    /// 请求退出主循环（信号安全：仅置标志）。
    void requestStop();

    /// 有序关闭所有子系统；线程安全、可重入（第二次调用直接返回）。
    void shutdown();

    /// Windows 关闭窗口等紧急路径：尽快停传输并强杀 RT（与 shutdown 互斥串行化）。
    void emergencyCleanup();

    static NrtApplication* instance();

private:
    bool launchRtProcess();
    void terminateRtProcess(bool notifyViaBridge);
    void killRtProcessHandle();

    int argc_;
    char** argv_;

    std::atomic<bool> running_{true};
    std::atomic<bool> shutdownComplete_{false};
    std::mutex lifecycleMutex_;
    std::mutex rtProcessMutex_;

    std::unique_ptr<zrcs::NrtProcess> nrtProcess_;
    std::unique_ptr<RtBridge> bridge_;
    std::unique_ptr<CommandService> commandService_;
    std::unique_ptr<TaskService> taskService_;
    std::unique_ptr<BehaviorTreeService> behaviorTreeService_;
    std::unique_ptr<BehaviorTreeCommandService> behaviorTreeCommandService_;
    std::unique_ptr<CommandRouter> commandRouter_;
    std::unique_ptr<zrcs_nrt::MujocoIdentifyWorker> mujocoIdentifyWorker_;
    std::unique_ptr<ZMQServer> zmqServer_;
    std::unique_ptr<StatusPublisher> statusPublisher_;
    std::unique_ptr<RtLogConsumer> rtLogConsumer_;

#ifdef _WIN32
    HANDLE rtProcess_{nullptr};
#else
    pid_t rtPid_{-1};
#endif

    static std::atomic<NrtApplication*> instance_;
};
