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


namespace zrcs_system { 
using namespace std;
 
class MY_LOG
{
public:
	
	
	std::shared_ptr<spdlog::logger> my_logger;   //创建的logger指针
	string log_file_name;    //log文件名

 
	 MY_LOG()
	{
	
		my_logger = spdlog::basic_logger_mt("basic_logger","../log/log.txt");	//创建basic_logger，注意该函数创建的是支持多线程的文件输出
		spdlog::set_pattern("[%Y-%m-%d %T][thread %t][%l]%v");   //设置logger的输出格式

	}
 
	
    ~MY_LOG()
	{
        spdlog::drop("basic_logger");   //logger使用完成后，要执行drop操作，否则不能循环创建同一类型的logger
	}
};
  //inline static MY_LOG Log;
}

#endif