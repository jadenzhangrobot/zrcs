#ifndef _LOGUTILS_H_
#define _LOGUTILS_H_

#include <sstream>
#include <iostream>
#include <iomanip>
#include <memory>

#include <log4cplus/loglevel.h>
#include <log4cplus/ndc.h>
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/log4cplus.h>
#include <log4cplus/consoleappender.h>
 
using namespace log4cplus;
using namespace log4cplus::helpers;
 
#define PATH_SIZE 128 

//日志初始化，传入日志的保存路径  
#define LOGGER_INIT(path)	\
{ \
	CLogUtils::instance()->init(path); \
}
 
/// 记录追踪日志
#define LOGGER_TRACE(msg)   \
{ \
	CLogUtils::instance(); \
	LOG4CPLUS_TRACE(CLogUtils::_logger, msg); \
}
/// 记录调试日志
#define LOGGER_DEBUG(msg)   \
{ \
	CLogUtils::instance(); \
	LOG4CPLUS_DEBUG(CLogUtils::_logger, msg); \
}
/// 记录信息日志
#define LOGGER_INFO(msg)    \
{ \
	CLogUtils::instance();\
	LOG4CPLUS_INFO(CLogUtils::_logger, msg);\
}
/// 记录告警日志
#define LOGGER_WARNING(msg) \
{ \
	CLogUtils::instance(); \
	LOG4CPLUS_WARNING(CLogUtils::_logger, msg); \
}
/// 记录错误日志
#define LOGGER_ERROR(msg)   \
{ \
	CLogUtils::instance(); \
	LOG4CPLUS_ERROR(CLogUtils::_logger, msg); \
}
/// 记录致命日志
#define LOGGER_FATAL(msg)  \
{ \
	CLogUtils::instance(); \
	LOG4CPLUS_FATAL(CLogUtils::_logger, msg); \
}
 
using namespace std;
class CLogUtils
{
public:
	static CLogUtils* instance();
	void init(const char* cLogPath);
	static Logger _logger;
private:
	CLogUtils();
	virtual ~CLogUtils();
 
	static CLogUtils* m_lpCLogUtils;
};
 
#endif // _LOGUTILS_H_