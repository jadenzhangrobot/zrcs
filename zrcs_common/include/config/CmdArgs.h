#pragma once

// ============================================================
// 命令参数索引定义
// 每个枚举定义对应命令的 command_->args[] 索引
// ============================================================

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

// ============================================================
// 任务调度状态
// ============================================================

enum class TaskScheduling
{
    RUN,
    ERROR_STATE,
    STOP,            // 停止
    RESET,           // 复位
    START,
    SHUTDOWN         // NRT 通知 RT 正常退出
};
