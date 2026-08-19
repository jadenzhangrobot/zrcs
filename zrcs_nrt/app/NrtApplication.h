#pragma once
/**
 * @file NrtApplication.h
 * @brief NRT 进程生命周期：RT 子进程管理 + 主循环。
 *
 * 服务装配委托给 NrtServiceContainer，本类只负责：
 *  - RT 子进程启动/终止
 *  - 共享内存初始化
 *  - 服务容器构造与启动
 *  - 主事件循环
 *  - 信号/控制台紧急处理
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

class NrtServiceContainer;

namespace zrcs_nrt {
struct MujocoIdentifyOptions;
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

    /// 有序关闭所有子系统；线程安全、可重入。
    void shutdown();

    /// 紧急路径：尽快停传输并强杀 RT（与 shutdown 互斥串行化）。
    void emergencyCleanup();

    /// 全局单例，供信号处理器访问。
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

    // RT 进程管理
    std::unique_ptr<zrcs::NrtProcess> nrtProcess_;
#ifdef _WIN32
    HANDLE rtProcess_{nullptr};
#else
    pid_t rtPid_{-1};
#endif

    // 服务容器（拥有全部 NRT 服务）
    std::unique_ptr<NrtServiceContainer> services_;

    static std::atomic<NrtApplication*> instance_;
};
