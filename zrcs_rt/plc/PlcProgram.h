#pragma once

// 三个 PLC 节点直接访问同一个 MatIEC 生成实例；本头文件只声明生成接口。
// 先在全局作用域加载 C 标准头，避免其声明被包含进 matiec 命名空间。
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 隔离 MatIEC 的 tm/BOOL 等类型名，C 链接保持生成物的符号名不变。
namespace matiec {
extern "C" {
#include "POUS.h"

// 实例符号 = <RESOURCE>__<PROGRAM 实例名>：ST 中 RESOURCE ZRCS + PROGRAM Demo
// => ZRCS__DEMO；程序类型名 PROGRAM plc => PLC。
extern PLC ZRCS__DEMO;
extern unsigned long long common_ticktime__;
extern TIME __CURRENT_TIME;
void config_init__(void);
void config_run__(unsigned long tick);
}
} // namespace matiec
