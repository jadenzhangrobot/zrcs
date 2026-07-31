#include "app/NrtApplication.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <thread>

#include <spdlog/spdlog.h>

#include "algorithm/dynamics_identification/MujocoIdentifyWorker.h"
#include "behavior_tree/core/BehaviorTreeService.h"
#include "command/BehaviorTreeCommandService.h"
#include "command/CommandRouter.h"
#include "command/CommandService.h"
#include "command/TaskService.h"
#include "config/ProjectConfig.h"
#include "log/NrtLogger.h"
#include "log/RtLogConsumer.h"
#include "status/StatusPublisher.h"
#include "nrtServer/zmq/ZmqServer.h"
#include "rtBridge/RtBridge.h"
#include "shared_memory/NrtProcess.h"
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
    // 与控制台紧急路径串行化：只清理一次
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
    // 控制台回调线程：尽快停传输 + 强杀 RT；与主线程 shutdown 互斥
    requestStop();

    std::lock_guard<std::mutex> lock(lifecycleMutex_);
    if (shutdownComplete_.load(std::memory_order_acquire)) {
        return;
    }

    if (zmqServer_) {
        zmqServer_->stop();
    }
    if (statusPublisher_) {
        statusPublisher_->stop();
    }

    killRtProcessHandle();

    // 标记部分紧急清理已做；主线程 shutdown 仍会尝试有序 stop 其余组件
    // 但 zmq/publisher/rt 已是 no-op
}

void NrtApplication::shutdown()
{
    requestStop();

    std::lock_guard<std::mutex> lock(lifecycleMutex_);
    if (shutdownComplete_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    // 以下 stop 自身线程安全可重入
    if (mujocoIdentifyWorker_) {
        mujocoIdentifyWorker_->stop();
    }
    if (statusPublisher_) {
        statusPublisher_->stop();
    }
    if (rtLogConsumer_) {
        rtLogConsumer_->stop();
    }
    if (behaviorTreeService_) {
        behaviorTreeService_->stop("Process shutdown");
    }
    if (zmqServer_) {
        zmqServer_->stop();
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
                        nullptr,
                        nullptr,
                        nullptr,
                        FALSE,
                        0,
                        nullptr,
                        dir.c_str(),
                        &si,
                        &pi)) {
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
    // 若紧急路径已杀句柄，此处只尝试优雅通知（bridge 可能仍有效）
    if (notifyViaBridge && bridge_) {
        spdlog::info("Sending SHUTDOWN to RT via RtBridge...");
        bridge_->requestShutdown();
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

int NrtApplication::run()
{
    NrtLogger::init();
    spdlog::info("ZRCS Non-Real-Time Process Started");

    const std::string projectName = zrcs::ProjectConfig::resolve();
    if (!projectName.empty()) {
        spdlog::info("Active project: {}", projectName);
    }

    nrtProcess_ = std::make_unique<zrcs::NrtProcess>();
    if (!nrtProcess_->initialize()) {
        spdlog::critical("Failed to initialize shared memory");
        return 1;
    }

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

    bridge_ = std::make_unique<RtBridge>(nrtProcess_->sharedBlock());
    commandService_ = std::make_unique<CommandService>(bridge_.get());
    taskService_ = std::make_unique<TaskService>(bridge_.get(), commandService_.get());
    behaviorTreeService_ = std::make_unique<BehaviorTreeService>(
        bridge_.get(), commandService_.get(), taskService_.get());
    behaviorTreeCommandService_ =
        std::make_unique<BehaviorTreeCommandService>(behaviorTreeService_.get());
    commandRouter_ = std::make_unique<CommandRouter>(
        commandService_.get(), taskService_.get(), behaviorTreeCommandService_.get());

    const auto mujocoIdentifyOptions = zrcs_nrt::parseMujocoIdentifyOptions(argc_, argv_);
    mujocoIdentifyWorker_ = std::make_unique<zrcs_nrt::MujocoIdentifyWorker>(
        nrtProcess_->sharedBlock(), mujocoIdentifyOptions);
    mujocoIdentifyWorker_->start();

    zmqServer_ = std::make_unique<ZMQServer>(commandRouter_.get());
    if (!zmqServer_->initialize()) {
        spdlog::critical("Failed to initialize ZMQ server");
        terminateRtProcess(/*notifyViaBridge=*/false);
        return 1;
    }

    zmqServer_->start();
    spdlog::info("ZMQ server started, waiting for commands...");

    statusPublisher_ = std::make_unique<StatusPublisher>(
        bridge_.get(), behaviorTreeService_.get());
    if (statusPublisher_->initialize()) {
        statusPublisher_->start();
        spdlog::info("Status publisher started (PUB on port 5556)");
    } else {
        spdlog::warn("Status publisher failed to initialize, continuing without it");
    }

    rtLogConsumer_ = std::make_unique<RtLogConsumer>(
        nrtProcess_->sharedBlock(),
        [this](const zrcs::RtLogEntry& entry) {
            if (statusPublisher_) {
                statusPublisher_->enqueueRtLog(entry);
            }
        });
    rtLogConsumer_->start();
    spdlog::info("RT log consumer started");

    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    spdlog::info("Shutting down NRT application...");
    shutdown();
    spdlog::info("Shutdown complete");
    return 0;
}
