/*
 * @Description: 已定义但尚未实现的命令 — 仅保留 CmdId 和参数索引
 *
 * 这些命令在 CmdId 枚举中存在，NRT/GUI 侧可能引用它们，
 * 但 RT 侧尚无对应的 CmdNode 实现。
 * 实现后应将 CMD_RESERVE 移到对应的命令 .h 文件中改为 CMD_DEFINE。
 */
#pragma once
#include "system/CmdMeta.h"

// === 基础系统命令 ===
CMD_RESERVE(Stop, 4,
    PARAM(AxisId)
)
CMD_RESERVE(EmergStop, 5)
CMD_RESERVE(SetOverride, 7,
    PARAM(Value)
)
CMD_RESERVE(ActUnit, 8,
    PARAM(UnitId)
)
CMD_RESERVE(DeactUnit, 9,
    PARAM(UnitId)
)

// === IO 命令 ===
CMD_RESERVE(SetGO, 12,
    PARAM(ModuleIndex)
    PARAM(Value)
)
CMD_RESERVE(PulseDO, 14,
    PARAM(ModuleIndex)
    PARAM(BitPos)
    PARAM(TimeMs)
)
CMD_RESERVE(IORead, 15,
    PARAM(ModuleIndex)
    PARAM(BitPos)
    PARAM(Type)
)

// === 流程控制命令 ===
CMD_RESERVE(Wait, 16,
    PARAM(TimeMs)
)
CMD_RESERVE(WaitDI, 17,
    PARAM(ModuleIndex)
    PARAM(BitPos)
    PARAM(Value)
    PARAM(TimeoutMs)
)
CMD_RESERVE(WaitUntil, 18,
    PARAM(CondType)
    PARAM(Param1)
    PARAM(Param2)
    PARAM(TimeoutMs)
)

// === 多轴/机器人运动命令 ===
CMD_RESERVE(SearchL, 27,
    PARAM(X)
    PARAM(Y)
    PARAM(Z)
    PARAM(RX)
    PARAM(RY)
    PARAM(RZ)
    PARAM(Vel)
    PARAM(IOModule)
    PARAM(IOBit)
)
CMD_RESERVE(TriggJ, 28,
    PARAM(X)
    PARAM(Y)
    PARAM(Z)
    PARAM(RX)
    PARAM(RY)
    PARAM(RZ)
    PARAM(Vel)
    PARAM(TrigDist)
    PARAM(IOModule)
    PARAM(IOBit)
    PARAM(IOVal)
)
CMD_RESERVE(TriggL, 29,
    PARAM(X)
    PARAM(Y)
    PARAM(Z)
    PARAM(RX)
    PARAM(RY)
    PARAM(RZ)
    PARAM(Vel)
    PARAM(TrigDist)
    PARAM(IOModule)
    PARAM(IOBit)
    PARAM(IOVal)
)

// === 坐标系/参数设置命令 ===
CMD_RESERVE(SetTCP, 31,
    PARAM(X)
    PARAM(Y)
    PARAM(Z)
    PARAM(RX)
    PARAM(RY)
    PARAM(RZ)
)
CMD_RESERVE(SetPayload, 33,
    PARAM(Mass)
)
CMD_RESERVE(ConfJ, 34,
    PARAM(OnOff)
)
CMD_RESERVE(ConfL, 35,
    PARAM(OnOff)
)
CMD_RESERVE(SingArea, 36,
    PARAM(Mode)
)
CMD_RESERVE(SetPosLimit, 37,
    PARAM(AxisId)
    PARAM(Positive)
    PARAM(Negative)
)
CMD_RESERVE(SetVelLimit, 38,
    PARAM(AxisId)
    PARAM(MaxVel)
    PARAM(MaxAcc)
    PARAM(MaxJerk)
)

// === 查询命令 ===
CMD_RESERVE(GetFK, 39)
CMD_RESERVE(GetJointPos, 40)

// === 高级运动命令 ===
CMD_RESERVE(SyncMove, 41,
    PARAM(MainAxis)
    PARAM(SlaveAxis)
    PARAM(Ratio)
)
CMD_RESERVE(CamMove, 42,
    PARAM(MainAxis)
    PARAM(SlaveAxis)
    PARAM(TableId)
)
CMD_RESERVE(Probe, 43,
    PARAM(AxisId)
    PARAM(Direction)
    PARAM(Vel)
    PARAM(IOIndex)
    PARAM(BitPos)
)
CMD_RESERVE(PosCapture, 44,
    PARAM(AxisId)
    PARAM(IOIndex)
    PARAM(BitPos)
    PARAM(Edge)
)
CMD_RESERVE(PosCompare, 45,
    PARAM(AxisId)
    PARAM(Position)
    PARAM(IOModule)
    PARAM(BitPos)
    PARAM(Value)
)
CMD_RESERVE(HelixMove, 46,
    PARAM(EndX)
    PARAM(EndY)
    PARAM(CenterX)
    PARAM(CenterY)
    PARAM(ZStart)
    PARAM(ZEnd)
    PARAM(Vel)
)
CMD_RESERVE(BufMove, 47,
    PARAM(Type)
    PARAM(P1)
    PARAM(P2)
    PARAM(P3)
    PARAM(P4)
    PARAM(P5)
    PARAM(Vel)
)
// CmdId 48 (PathMove) 已在 command/PathMove.h 中通过 CMD_DEFINE 定义

// === 激光命令 ===
CMD_RESERVE(LaserSet, 49,
    PARAM(Power)
    PARAM(Freq)
    PARAM(PulseWidth)
    PARAM(Gate)
)
CMD_RESERVE(GalvoMarkL, 50,
    PARAM(X)
    PARAM(Y)
    PARAM(Vel)
    PARAM(Power)
    PARAM(Freq)
    PARAM(LaserOn)
    PARAM(PlatXId)
    PARAM(PlatYId)
    PARAM(GalvoXId)
    PARAM(GalvoYId)
    PARAM(CutoffHz)
)
CMD_RESERVE(GalvoBufMark, 51,
    PARAM(Type)
    PARAM(X)
    PARAM(Y)
    PARAM(Vel)
    PARAM(Power)
    PARAM(Freq)
    PARAM(PlatXId)
    PARAM(PlatYId)
    PARAM(GalvoXId)
    PARAM(GalvoYId)
    PARAM(CutoffHz)
)
