/*
 * @Description: 命令元数据声明宏
 *
 * 在命令 .h 中使用 CMD_DEFINE 声明 CmdId 和参数列表。
 * C++ 编译时这些宏展开为空；Python 代码生成器 (tool/gen_cmd_registry.py)
 * 通过正则匹配提取元数据，自动生成 CmdRegistry_gen.h / CmdHead_gen.h /
 * CmdNameToId_gen.h / message_gen.proto。
 *
 * 用法：
 *   CMD_DEFINE(Enable, 1, PARAM(AxisId))
 *   CMD_DEFINE(MoveJ, 24, PARAM(X) PARAM(Y) PARAM(Z) PARAM(RX) PARAM(RY) PARAM(RZ) PARAM(Vel))
 *   CMD_DEFINE(Movehome, 30)          // 无参数
 *   CMD_RESERVE(Stop, 4)              // 保留 ID（未实现的命令）
 *   CMD_RESERVE(EmergStop, 5, PARAM(xxx))   // 保留 ID + 参数
 */
#pragma once

// PARAM — C++ 编译时为空，仅供代码生成器解析
#define PARAM(name)

// CMD_DEFINE — 已实现的命令
// 生成器解析格式: CMD_DEFINE(className, cmdId, PARAM(p1) PARAM(p2) ...)
// 生成 CmdId 枚举项 + 参数索引枚举 + CmdHead_gen.h #include + CmdNameToId 映射
#define CMD_DEFINE(className, id, ...)

// CMD_RESERVE — 保留但未实现的命令（只生成 CmdId 枚举项 + 参数索引 + CmdNameToId，
//               不生成 CmdHead_gen.h #include）
#define CMD_RESERVE(className, id, ...)
