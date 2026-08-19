#include "app/NrtApplication.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <thread>

#include <spdlog/spdlog.h>

#include "algorithm/dynamics_identification/MujocoIdentifyWorker.h"
#include "config/ProjectConfig.h"
#include "core/ServiceContainer.h"
#include "log/NrtLogger.h"
#include "rtBridge/RtBridge.h"
#include "shared_memory/ShmLayout.h"

#ifndef _WIN32
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

std::atomic<NrtApplication*> NrtApplication::instance_{nullptr};

NrtApplication::NrtApplication(int argc, char** argv)
    : argc_(argc)
    , argv_(argv)
{
    instance_.store(this, std::memory_order_release);
}

NrtApplication::~NrtApplication()
{
    shutdown();

    if (instance_.load(std::memory_order_acquire) == this) {
        instance_.store(nullptr, std::memory_order_release);
    }
}

NrtApplication* NrtApplication::instance()
{
    return instance_.load(std::memory_order_acquire);
}

void NrtApplication::requestStop()
{
    running_.store(false, std::memory_order_release);
}

// =============================================================================
// RT 进程管理（与 NrtServiceContainer 无关）
// =============================================================================

void NrtApplication::killRtProcessHandle()
{
    std::lock_guard<std::mutex> lock(rtProcessMutex_);
#ifdef _WIN32
    if (!rtProcess_) {
        return;
    }
    WaitForSingleObject(rtProcess_, 500);
    TerminateProcess(rtProcess_, 0);
    WaitForSingleObject(rtProcess_, 3000);
    CloseHandle(rtProcess_);
    rtProcess_ = nullptr;
#else
    if (rtPid_ <= 0) {
        return;
    }
    kill(rtPid_, SIGKILL);
    int status = 0;
    waitpid(rtPid_, &status, 0);
    rtPid_ = -1;
#endif
}

void NrtApplication::emergencyCleanup()
{
    requestStop();

    std::lock_guard<std::mutex> lock(lifecycleMutex_);
    if (shutdownComplete_.load(std::memory_order_acquire)) {
        return;
    }

    // 尽可能快：停 ZMQ 发布链路，强杀 RT
    if (services_) {
        services_->shutdown();
    }

    killRtProcessHandle();
}

void NrtApplication::shutdown()
{
    requestStop();

    std::lock_guard<std::mutex> lock(lifecycleMutex_);
    if (shutdownComplete_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    // 有序关闭所有服务
    if (services_) {
        services_->shutdown();
    }

    terminateRtProcess(/*notifyViaBridge=*/true);
}

bool NrtApplication::launchRtProcess()
{
    std::lock_guard<std::mutex> lock(rtProcessMutex_);
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string dir(exePath);
    dir = dir.substr(0, dir.find_last_of("\\/") + 1);
    const std::string rtPath = dir + zrcs::kRtProcessName + ".exe";

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessA(rtPath.c_str(),
                        nullptr, nullptr, nullptr, FALSE, 0,
                        nullptr, dir.c_str(), &si, &pi)) {
        spdlog::error("Failed to launch RT process: {} (error {})", rtPath, GetLastError());
        return false;
    }

    CloseHandle(pi.hThread);
    rtProcess_ = pi.hProcess;
    spdlog::info("RT process launched (PID {}): {}", pi.dwProcessId, rtPath);
    return true;
#else
    char exePath[PATH_MAX] = {0};
    const ssize_t exeLen = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (exeLen <= 0) {
        spdlog::error("Failed to resolve current executable path");
        return false;
    }
    exePath[exeLen] = '\0';

    const std::filesystem::path execDir = std::filesystem::path(exePath).parent_path();
    const std::string rtPath = (execDir / zrcs::kRtProcessName).string();

    rtPid_ = fork();
    if (rtPid_ < 0) {
        spdlog::error("Failed to fork RT process");
        return false;
    }
    if (rtPid_ == 0) {
        if (chdir(execDir.c_str()) != 0) {
            _exit(1);
        }
        execl(rtPath.c_str(), zrcs::kRtProcessName, nullptr);
        _exit(1);
    }
    spdlog::info("RT process launched (PID {}): {}", rtPid_, rtPath);
    return true;
#endif
}

void NrtApplication::terminateRtProcess(bool notifyViaBridge)
{
    if (notifyViaBridge && services_ && services_->bridge()) {
        spdlog::info("Sending SHUTDOWN to RT via RtBridge...");
        services_->bridge()->requestShutdown();
    }

    std::lock_guard<std::mutex> lock(rtProcessMutex_);
#ifdef _WIN32
    if (!rtProcess_) {
        return;
    }
    const DWORD waitResult = WaitForSingleObject(rtProcess_, 3000);
    if (waitResult == WAIT_OBJECT_0) {
        spdlog::info("RT process exited gracefully.");
    } else {
        spdlog::warn("RT process did not exit in time, force terminating...");
        TerminateProcess(rtProcess_, 0);
        WaitForSingleObject(rtProcess_, 2000);
        spdlog::info("RT process force terminated.");
    }
    CloseHandle(rtProcess_);
    rtProcess_ = nullptr;
#else
    if (rtPid_ <= 0) {
        return;
    }
    int status = 0;
    bool exited = false;
    for (int i = 0; i < 30; ++i) {
        const pid_t ret = waitpid(rtPid_, &status, WNOHANG);
        if (ret == rtPid_) {
            exited = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (exited) {
        spdlog::info("RT process exited gracefully.");
    } else {
        spdlog::warn("RT process did not exit in time, sending SIGKILL...");
        kill(rtPid_, SIGKILL);
        waitpid(rtPid_, &status, 0);
        spdlog::info("RT process force terminated.");
    }
    rtPid_ = -1;
#endif
}

// =============================================================================
// 主入口
// =============================================================================

int NrtApplication::run()
{
    NrtLogger::init();
    spdlog::info("ZRCS Non-Real-Time Process Started");

    const std::string projectName = zrcs::ProjectConfig::resolve();
    if (!projectName.empty()) {
        spdlog::info("Active project: {}", projectName);
    }

    // 1. 共享内存初始化
    nrtProcess_ = std::make_unique<zrcs::NrtProcess>();
    if (!nrtProcess_->initialize()) {
        spdlog::critical("Failed to initialize shared memory");
        return 1;
    }

    // 2. 启动 RT 子进程
    if (!launchRtProcess()) {
        spdlog::critical("Failed to launch RT process, exiting.");
        return 1;
    }

    if (!nrtProcess_->waitForRt()) {
        spdlog::critical("RT process did not initialize shared memory");
        terminateRtProcess(/*notifyViaBridge=*/false);
        return 1;
    }

    spdlog::info("SharedBlock initialized");

    // 3. 创建服务容器（依赖注入在构造阶段一次性完成）
    const auto mujocoOptions = zrcs_nrt::parseMujocoIdentifyOptions(argc_, argv_);
    services_ = std::make_unique<NrtServiceContainer>(
        nrtProcess_->sharedBlock(), mujocoOptions);

    // 4. 初始化（ZMQ bind 等 I/O 操作）
    if (!services_->initialize()) {
        spdlog::critical("Failed to initialize services");
        terminateRtProcess(/*notifyViaBridge=*/false);
        return 1;
    }

    // 5. 启动所有后台线程
    services_->startAll();

    // 6. 主循环
    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::info("Shutting down NRT application...");
    shutdown();
    spdlog::info("Shutdown complete");
    return 0;
}
