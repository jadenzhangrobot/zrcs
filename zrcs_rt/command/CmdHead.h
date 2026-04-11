#pragma once

// === 基础系统命令 ===
#include "Enable.h"
#include "Disable.h"
#include "Reset.h"
#include "Stop.h"
#include "EmergStop.h"
#include "Setmode.h"
#include "SetOverride.h"
#include "ActUnit.h"
#include "DeactUnit.h"

// === IO 命令 ===
#include "SetDO.h"
#include "SetGO.h"
#include "SetAO.h"
#include "PulseDO.h"
#include "IORead.h"

// === 流程控制命令 ===
#include "Wait.h"
#include "WaitDI.h"
#include "WaitUntil.h"

// === 单轴运动命令 ===
#include "JogabsJ.h"
#include "JogJ.h"
#include "MoveAbs.h"
#include "MoveRel.h"
#include "ContinuousJog.h"
#include "Movehome.h"

// === 多轴/机器人运动命令 ===
#include "MoveabsJ.h"
#include "MoveJ.h"
#include "MoveL.h"
#include "MoveC.h"
#include "SearchL.h"
#include "TriggJ.h"
#include "TriggL.h"

// === 坐标系/参数设置命令 ===
#include "SetZero.h"
#include "SetTCP.h"
#include "SetBase.h"
#include "SetPayload.h"
#include "ConfJ.h"
#include "ConfL.h"
#include "SingArea.h"
#include "SetPosLimit.h"
#include "SetVelLimit.h"

// === 查询命令 ===
#include "GetFK.h"
#include "GetJointPos.h"

// === 高级运动命令（固高/正运动特色） ===
#include "SyncMove.h"
#include "CamMove.h"
#include "Probe.h"
#include "PosCapture.h"
#include "PosCompare.h"
#include "HelixMove.h"
#include "BufMove.h"
#include "SplineMove.h"

// === 数据节点 ===
#include "DataCollection.h"
#include "DataPub.h"

