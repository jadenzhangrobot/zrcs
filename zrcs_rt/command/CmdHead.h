#pragma once
/*
 * CmdHead.h — 手动维护的命令头文件列表
 *
 * 替代原先由 tool/gen_cmd_registry.py 生成的 CmdHead_gen.h。
 * 新增命令时，在此添加对应的 #include，并同步更新 config/CmdDefine.h。
 */

// 已实现的命令（REGISTERCMD 静态注册，宏会将类名绑定到同名 CmdId）
#include "command/Disable.h"
#include "command/Enable.h"
#include "command/JogJ.h"
#include "command/JogabsJ.h"
#include "command/MoveAbs.h"
#include "command/MoveabsJ.h"
#include "command/MoveC.h"
#include "command/MoveCurve.h"
#include "command/MoveExcite.h"
#include "command/MoveJ.h"
#include "command/MoveL.h"
#include "command/MovePath.h"
#include "command/MoveV.h"
#include "command/Movehome.h"
#include "command/Reset.h"


#include "command/SetZero.h"
#include "command/Setmode.h"

// 注意：CmdId 目前没有保留槽位（不存在 CMD_RESERVE 枚举值）。

// PeriodicNode（长期节点，不走 CmdId 路由）
#include "command/ContinuousJog.h"
#include "command/DataPub.h"
