/*
 * CmdMeta.h — 命令注册宏
 *
 * CMD_DEFINE 放在命令类体 public 区域，展开为:
 *   - kCmdId: 命令 ID 常量，供 CMD_REGISTER 宏使用
 *   - 匿名参数索引枚举：在类方法内可直接用参数名作下标访问 command_->args[]
 *
 * 用法（在类 public 区域）：
 *   CMD_DEFINE(1, PARAM(AxisId))
 *   CMD_DEFINE(25, PARAM(CurrentX) PARAM(CurrentY) PARAM(Z) PARAM(Vel))
 *   CMD_DEFINE(30)                    // 无参数
 *
 * CMD_REGISTER 放在命令 .cpp 文件底部，将命令注册到 NodeFactory:
 *   CMD_REGISTER(Enable)
 *
 * CMD_RESERVE 保留 ID 占位，展开为空（仅文档用途）:
 *   CMD_RESERVE(Stop, 4)
 */
#pragma once

#include "system/NodeFactory.h"

// PARAM(name) — 在 CMD_DEFINE 枚举中展开为一个枚举成员
#define PARAM(name) name,

// CMD_DEFINE(id, PARAM(p1) PARAM(p2) ...) — 放在类体 public 区域
// 展开为 kCmdId 常量 + 匿名参数索引枚举
#define CMD_DEFINE(id, ...)                        \
    static constexpr uint16_t kCmdId = (id);      \
    enum : int { __VA_ARGS__ };

// CMD_REGISTER(ClassName) — 放在 .cpp 底部，触发静态注册
#define CMD_REGISTER(ClassName)                                              \
    static zrcsSystem::RegisterNodeById<ClassName> _reg_##ClassName(        \
        ClassName::kCmdId);

// CMD_RESERVE — 保留但未实现的命令，展开为空
#define CMD_RESERVE(className, id, ...)
