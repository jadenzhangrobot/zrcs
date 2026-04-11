#pragma once

// ============================================================
// 命令参数索引定义
// 每个枚举定义对应命令的 command_->args[] 索引
// ============================================================

// 引入共享内存布局类型（TaskScheduling、Command、SharedBlock 等）
#include "shared_memory/ShmLayout.h"

// === 基础系统命令 ===

enum { EnableAxisId };
enum { DisableAxisId };
enum { ResetAxisId };
enum { StopAxisId };
// EmergStop: 无参数
enum { SetmodeAxisId, SetmodeMode };
enum { SetOverrideValue };        // 0.0 ~ 1.0
enum { ActUnitId };
enum { DeactUnitId };
enum { SetZeroAxisId };

// === IO 命令 ===

enum { SetDOModuleIndex, SetDOBitPos, SetDOValue };
enum { SetGOModuleIndex, SetGOValue };
enum { SetAOModuleIndex, SetAOChannel, SetAOValue };
enum { PulseDOModuleIndex, PulseDOBitPos, PulseDOTimeMs };
enum { IOReadModuleIndex, IOReadBitPos, IOReadType };  // Type: 0=数字, 1=模拟

// === 流程控制命令 ===

enum { WaitTimeMs };
enum { WaitDIModuleIndex, WaitDIBitPos, WaitDIValue, WaitDITimeoutMs };
enum { WaitUntilCondType, WaitUntilParam1, WaitUntilParam2, WaitUntilTimeoutMs };

// === 单轴运动命令 ===

enum { JogabsjAxisId, JogabsjTargetPosition };
enum { JogjAxisId, JogjTargetPosition };
enum { MoveAbsAxisId, MoveAbsPosition, MoveAbsVel, MoveAbsAcc, MoveAbsJerk };
enum { MoveRelAxisId, MoveRelDistance, MoveRelVel, MoveRelAcc, MoveRelJerk };

// === 多轴/机器人运动命令 ===

enum { MoveAbsJCount, MoveAbsJJ1, MoveAbsJJ2, MoveAbsJJ3, MoveAbsJJ4, MoveAbsJJ5, MoveAbsJJ6 };
enum { MoveJX, MoveJY, MoveJZ, MoveJRX, MoveJRY, MoveJRZ, MoveJVel };
enum { MoveLX, MoveLY, MoveLZ, MoveLRX, MoveLRY, MoveLRZ, MoveLVel };
enum { MoveCViaX, MoveCViaY, MoveCViaZ, MoveCEndX, MoveCEndY, MoveCEndZ, MoveCVel };
enum { SearchLX, SearchLY, SearchLZ, SearchLRX, SearchLRY, SearchLRZ,
       SearchLVel, SearchLIOModule, SearchLIOBit };
enum { TriggJX, TriggJY, TriggJZ, TriggJRX, TriggJRY, TriggJRZ,
       TriggJVel, TriggJTrigDist, TriggJIOModule, TriggJIOBit, TriggJIOVal };
enum { TriggLX, TriggLY, TriggLZ, TriggLRX, TriggLRY, TriggLRZ,
       TriggLVel, TriggLTrigDist, TriggLIOModule, TriggLIOBit, TriggLIOVal };
// Movehome: 无参数（回零所有轴）

// === 坐标系/参数设置命令 ===

enum { SetTCPX, SetTCPY, SetTCPZ, SetTCPRX, SetTCPRY, SetTCPRZ };
enum { SetBaseX, SetBaseY, SetBaseZ, SetBaseRX, SetBaseRY, SetBaseRZ };
enum { SetPayloadMass };
enum { ConfJOnOff };              // 0=off, 1=on
enum { ConfLOnOff };              // 0=off, 1=on
enum { SingAreaMode };            // 0=Off, 1=Wrist, 2=LockAxis
enum { SetPosLimitAxisId, SetPosLimitPositive, SetPosLimitNegative };
enum { SetVelLimitAxisId, SetVelLimitMaxVel, SetVelLimitMaxAcc, SetVelLimitMaxJerk };

// === 查询命令 ===
// GetFK: 无参数
// GetJointPos: 无参数

// === 高级运动命令 ===

enum { SyncMoveMainAxis, SyncMoveSlaveAxis, SyncMoveRatio };
enum { CamMoveMainAxis, CamMoveSlaveAxis, CamMoveTableId };
enum { ProbeAxisId, ProbeDirection, ProbeVel, ProbeIOIndex, ProbeBitPos };
enum { PosCaptureAxisId, PosCaptureIOIndex, PosCaptureBitPos, PosCaptureEdge };
enum { PosCompareAxisId, PosComparePosition, PosCompareIOModule, PosCompareBitPos, PosCompareValue };
enum { HelixMoveEndX, HelixMoveEndY, HelixMoveCenterX, HelixMoveCenterY,
       HelixMoveZStart, HelixMoveZEnd, HelixMoveVel };
enum { BufMoveType, BufMoveP1, BufMoveP2, BufMoveP3, BufMoveP4, BufMoveP5, BufMoveVel };
enum { SplineMoveX, SplineMoveY, SplineMoveZ, SplineMoveVel };

// === 激光参数设置 ===

enum { LaserSetPower, LaserSetFreq, LaserSetPulseWidth, LaserSetGate };

// === 振镜-平台联动标刻 (全局坐标, 单插补器+后置滤波分解) ===

enum { GalvoMarkLX, GalvoMarkLY, GalvoMarkLVel,
       GalvoMarkLPower, GalvoMarkLFreq, GalvoMarkLLaserOn,
       GalvoMarkLPlatXId, GalvoMarkLPlatYId, GalvoMarkLGalvoXId, GalvoMarkLGalvoYId,
       GalvoMarkLCutoffHz };

// === 缓冲连续联动标刻 ===

enum { GalvoBufMarkType, GalvoBufMarkX, GalvoBufMarkY,
       GalvoBufMarkVel, GalvoBufMarkPower, GalvoBufMarkFreq,
       GalvoBufMarkPlatXId, GalvoBufMarkPlatYId, GalvoBufMarkGalvoXId, GalvoBufMarkGalvoYId,
       GalvoBufMarkCutoffHz };

// ============================================================
// 命令 ID 枚举（对应 Command::cmdId）
// NRT 侧按名称查表，RT 侧按 cmdId 快速分发，消除热路径中的 strcmp。
// 新增命令时：追加到 SENTINEL 之前，并在 RtBridge.h 的 kCmdNameToId 中注册。
// ============================================================

enum class CmdId : uint16_t {
    INVALID      = 0,
    // 基础系统命令
    Enable       = 1,
    Disable      = 2,
    Reset        = 3,
    Stop         = 4,
    EmergStop    = 5,
    Setmode      = 6,
    SetOverride  = 7,
    ActUnit      = 8,
    DeactUnit    = 9,
    SetZero      = 10,
    // IO 命令
    SetDO        = 11,
    SetGO        = 12,
    SetAO        = 13,
    PulseDO      = 14,
    IORead       = 15,
    // 流程控制命令
    Wait         = 16,
    WaitDI       = 17,
    WaitUntil    = 18,
    // 单轴运动命令
    JogabsJ      = 19,
    JogJ         = 20,
    MoveAbs      = 21,
    MoveRel      = 22,
    // 多轴/机器人运动命令
    MoveAbsJ     = 23,
    MoveJ        = 24,
    MoveL        = 25,
    MoveC        = 26,
    SearchL      = 27,
    TriggJ       = 28,
    TriggL       = 29,
    Movehome     = 30,
    // 坐标系/参数设置命令
    SetTCP       = 31,
    SetBase      = 32,
    SetPayload   = 33,
    ConfJ        = 34,
    ConfL        = 35,
    SingArea     = 36,
    SetPosLimit  = 37,
    SetVelLimit  = 38,
    // 查询命令
    GetFK        = 39,
    GetJointPos  = 40,
    // 高级运动命令
    SyncMove     = 41,
    CamMove      = 42,
    Probe        = 43,
    PosCapture   = 44,
    PosCompare   = 45,
    HelixMove    = 46,
    BufMove      = 47,
    SplineMove   = 48,
    // 激光命令
    LaserSet     = 49,
    GalvoMarkL   = 50,
    GalvoBufMark = 51,
    SENTINEL     // 边界检查，勿在此之后使用
};

// AXISMAXCOUNT 保留全局常量，与 ShmLayout.h 中的 kAxisMax 等价
inline constexpr size_t AXISMAXCOUNT = zrcs::kAxisMax;

namespace zrcs {

// cmdIdToName — RT 侧用于日志打印（数组索引，无 strcmp，O(1)）
// 返回 nullptr 表示未知 ID。
inline const char* cmdIdToName(uint16_t id) noexcept {
    static constexpr const char* kTable[] = {
        "INVALID",      // 0
        "Enable",       // 1
        "Disable",      // 2
        "Reset",        // 3
        "Stop",         // 4
        "EmergStop",    // 5
        "Setmode",      // 6
        "SetOverride",  // 7
        "ActUnit",      // 8
        "DeactUnit",    // 9
        "SetZero",      // 10
        "SetDO",        // 11
        "SetGO",        // 12
        "SetAO",        // 13
        "PulseDO",      // 14
        "IORead",       // 15
        "Wait",         // 16
        "WaitDI",       // 17
        "WaitUntil",    // 18
        "JogabsJ",      // 19
        "JogJ",         // 20
        "MoveAbs",      // 21
        "MoveRel",      // 22
        "MoveAbsJ",     // 23
        "MoveJ",        // 24
        "MoveL",        // 25
        "MoveC",        // 26
        "SearchL",      // 27
        "TriggJ",       // 28
        "TriggL",       // 29
        "Movehome",     // 30
        "SetTCP",       // 31
        "SetBase",      // 32
        "SetPayload",   // 33
        "ConfJ",        // 34
        "ConfL",        // 35
        "SingArea",     // 36
        "SetPosLimit",  // 37
        "SetVelLimit",  // 38
        "GetFK",        // 39
        "GetJointPos",  // 40
        "SyncMove",     // 41
        "CamMove",      // 42
        "Probe",        // 43
        "PosCapture",   // 44
        "PosCompare",   // 45
        "HelixMove",    // 46
        "BufMove",      // 47
        "SplineMove",   // 48
        "LaserSet",     // 49
        "GalvoMarkL",   // 50
        "GalvoBufMark", // 51
    };
    constexpr size_t kTableSize = sizeof(kTable) / sizeof(kTable[0]);
    if (id < kTableSize) return kTable[id];
    return "UNKNOWN";
}

} // namespace zrcs
