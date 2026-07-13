#pragma once

namespace NrtLogger {

/**
 * @brief 初始化 NRT 日志系统
 * @details 创建控制台 + 文件双输出 logger
 *          日志文件位于可执行文件同目录的 logs/ 下
 *          文件按 5MB 大小轮转，保留 3 个文件
 */
void init();

} // namespace NrtLogger
