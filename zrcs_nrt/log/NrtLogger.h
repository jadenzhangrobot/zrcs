#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace NrtLogger {

/**
 * @brief 获取可执行文件所在目录
 */
inline std::filesystem::path getExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
#else
    return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
}

/**
 * @brief 初始化 NRT 日志系统
 * @details 创建控制台 + 文件双输出 logger
 *          日志文件位于可执行文件同目录的 logs/ 下
 *          文件按 5MB 大小轮转，保留 3 个文件
 */
inline void init() {
    try {
        auto log_dir = getExeDir() / "logs";
        std::filesystem::create_directories(log_dir);
        auto log_path = (log_dir / "nrt.log").string();

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_path, 5 * 1024 * 1024, 3);
        file_sink->set_level(spdlog::level::trace);

        auto logger = std::make_shared<spdlog::logger>("nrt",
            spdlog::sinks_init_list{console_sink, file_sink});
        logger->set_level(spdlog::level::trace);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        logger->flush_on(spdlog::level::debug);

        spdlog::set_default_logger(logger);
        spdlog::info("NRT logger initialized, log file: {}", log_path);
    } catch (const spdlog::spdlog_ex& ex) {
        // fallback to stderr if logger init fails
        fprintf(stderr, "[NrtLogger] Init failed: %s\n", ex.what());
    }
}

} // namespace NrtLogger
