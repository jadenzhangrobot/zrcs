/**
 * @file main.cpp
 * @brief Non-Real-Time process entry point
 * @details 信号安装 + NrtApplication 生命周期。业务装配见 app/NrtApplication。
 */

#include <csignal>
#include <cstdio>
#include <iostream>

#include <spdlog/spdlog.h>

#include "app/NrtApplication.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// 信号/控制台回调中不可调用 spdlog 等可能持锁的函数
static void signalHandler(int signum)
{
    std::fprintf(stdout, "[main] Received signal %d, shutting down...\n", signum);
    if (NrtApplication* app = NrtApplication::instance()) {
        app->requestStop();
    }
}

#ifdef _WIN32
// 关闭窗口/注销/关机：系统线程回调。
// 只做 requestStop + emergencyCleanup（内部与主线程 shutdown 互斥串行化）。
static BOOL WINAPI consoleCtrlHandler(DWORD ctrlType)
{
    switch (ctrlType) {
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        if (NrtApplication* app = NrtApplication::instance()) {
            app->emergencyCleanup();
        }
        return TRUE;
    default:
        // CTRL_C / BREAK 交给 signalHandler(SIGINT)
        return FALSE;
    }
}
#endif

int main(int argc, char** argv)
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#else
    std::signal(SIGHUP, signalHandler);
#endif

    std::cout << "ZRCS Non-Real-Time Process Started" << std::endl;

    try {
        NrtApplication app(argc, argv);
        return app.run();
    } catch (const std::exception& e) {
        spdlog::critical("Exception caught: {}", e.what());
        if (NrtApplication* app = NrtApplication::instance()) {
            app->emergencyCleanup();
        }
        return 1;
    }
}
