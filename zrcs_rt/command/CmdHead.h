#pragma once
/*
 * CmdHead.h — 手动维护的命令头文件列表
 *
 * 替代原先由 tool/gen_cmd_registry.py 生成的 CmdHead_gen.h。
 * 新增命令时，在此添加对应的 #include，并同步更新 system/CmdIds.h。
 */

// 已实现的命令（CMD_DEFINE，含 REGISTERCMD 静态注册）
#include "command/Disable.h"
#include "command/Enable.h"
#include "command/JogJ.h"
#include "command/JogabsJ.h"
#include "command/MoveAbs.h"
#include "command/MoveabsJ.h"
#include "command/MoveC.h"
#include "command/MoveJ.h"
#include "command/MoveL.h"
#include "command/MoveLGalvo.h"
#include "command/MoveRel.h"
#include "command/Movehome.h"
#include "command/PathMove.h"
#include "command/Reset.h"
#include "command/SetAO.h"
#include "command/SetBase.h"
#include "command/SetDO.h"
#include "command/SetZero.h"
#include "command/Setmode.h"

// 保留 ID（CMD_RESERVE，未实现，仅占 CmdId 槽位）
#include "command/CmdReserved.h"

// OutputNode / InputNode（不走 CmdId 路由）
#include "command/ContinuousJog.h"
#include "command/DataPub.h"
