#ifndef SPDLOG_H_
#define SPDLOG_H_
#include <iostream>
#include <cstring>
#include <sstream>
#include "spdlog/spdlog.h" 
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/logger.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/pattern_formatter.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace zrcs_system { 
using namespace std;
 
class Zrcslog
{
public:
	
	 Zrcslog()
	{
	
		//my_logger = spdlog::basic_logger_mt("basic_logger","../log/log.txt");	//创建basic_logger，注意该函数创建的是支持多线程的文件输出
		//spdlog::set_pattern("[%Y-%m-%d %T][thread %t][%l]%v");   //设置logger的输出格式
          // 创建文件日志器，将日志写入文件
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("../log/log.txt", true);
    
    // 创建控制台日志器，将日志输出到控制台
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    
    // 创建多重日志器，将日志同时写入文件和输出到控制台
    auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{file_sink, console_sink});
    
    // 设置日志级别
    logger->set_level(spdlog::level::debug);
    
    // 设置日志格式
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    
    // 将多重日志器设置为默认日志器
    spdlog::set_default_logger(logger);
	}
 
	
    ~Zrcslog()
	{
        spdlog::drop("multi_sink");   //logger使用完成后，要执行drop操作，否则不能循环创建同一类型的logger
	}
};
}

#endif