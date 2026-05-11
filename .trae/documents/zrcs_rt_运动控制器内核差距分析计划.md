# `zrcs_rt` 与工业运动控制器内核差距分析计划

## Summary

- 目标：基于当前仓库实际代码，对 `zrcs_rt` 按“运动控制器内核”基线做差距分析，而不是按完整工业设备软件或 MES/SCADA 平台对比。
- 产出：形成一份可执行的能力矩阵，明确三类结论：
  - 已实现并可用的内核能力
  - 已在接口/共享内存/命令枚举中预留，但 RT 侧未真正落地的能力
  - 与工业运动控制器相比仍明显缺失的关键内核能力
- 成功标准：
  - 结论必须基于已存在文件与实现，不凭空猜测
  - 每个关键结论都要能追溯到具体文件
  - 输出中要明确“优先级”，便于后续决定先补哪些功能

## Current State Analysis

### 1. 已确认的系统结构

- `zrcs_rt/main.cpp`
  - 负责 RT 进程入口、项目加载、`NodeManager` 启动与共享内存 `SHUTDOWN` 退出。
- `zrcs_rt/system/NodeManager.cpp`
  - 已形成 RT 主循环：`receiveData -> InputNodes -> CmdNode -> OutputNodes -> sendData`。
  - 已具备单命令串行调度、`RUN/STOP/RESET/ERROR_STATE` 状态切换、心跳上报、命令完成跟踪。
- `zrcs_rt/controller/`
  - 已形成控制器聚合层、轴抽象层、EtherCAT/虚拟控制器/RTOS 封装。
- `zrcs_rt/model/`
  - 已具备 FK/IK/Jacobian、基坐标系/工具坐标系、奇异性指标函数。
- `zrcs_nrt/rt_bridge/RtBridge.h` 与 `zrcs_common/shared_memory/ShmLayout.h`
  - 已形成 RT/NRT 共享内存桥接、命令队列、路径队列、日志队列、状态快照通道。

### 2. 已实现的运动控制器内核能力

- 轨迹类命令已实现：
  - `Enable/Disable/Reset/SetZero/Setmode`
  - `JogJ/JogabsJ/MoveAbs/MoveAbsJ/MoveJ/MoveL/MoveC/Movehome/MoveLGalvo`
  - 证据文件：`zrcs_rt/command/CmdHead.h`
- 轨迹生成能力已具备：
  - 基于 `Ruckig` 的 jerk-limited 单轴/多轴轨迹
  - 笛卡尔直线插补、圆弧插补、平台+振镜低通分解
  - 证据文件：`zrcs_rt/command/MoveJ.cpp`、`MoveL.cpp`、`MoveC.cpp`、`MoveLGalvo.cpp`
- 基本安全/保护已具备：
  - 轴方向使能限制、软限位检查、多驱同步误差检查、轴状态机、急停接口
  - 证据文件：`zrcs_rt/controller/ControllerInterface.cpp`、`ControllerInterface.h`、`Global.h`
- 总线与实时基础已具备：
  - EtherCAT PDO/DC 同步、共享内存无锁队列、RT 心跳、RT 日志
  - 证据文件：`zrcs_rt/controller/ethercat/EthercatMaster.h`、`zrcs_common/shared_memory/ShmLayout.h`

### 3. 已预留但未完整落地的能力

- 命令枚举中已定义但 RT 侧未见对应实现或注册：
  - `IORead`
  - `SearchL`
  - `SyncMove`
  - `CamMove`
  - `Probe`
  - `PosCapture`
  - `PosCompare`
  - `HelixMove`
  - `BufMove`
  - `PathMove`
  - `LaserSet`
  - `GalvoMarkL`
  - `GalvoBufMark`
  - 证据文件：`zrcs_common/config/CmdDefine.h` 与 `zrcs_rt/command/CmdHead.h`
- 共享内存已预留但未见 RT 命令闭环：
  - `ioReadResult`
  - `probeResult`
  - `captureResult`
  - `probeTriggered`
  - `captureTriggered`
  - `pathQueue`
  - `pathMoveActive`
  - `pathMoveCfg`
  - 证据文件：`zrcs_common/shared_memory/ShmLayout.h`、`zrcs_nrt/rt_bridge/RtBridge.h`

### 4. 与工业运动控制器内核相比的主要差距

#### A. 核心运动功能差距

- 缺少真正落地的连续路径执行/多段队列消费内核
  - 虽然有 `pathQueue/pathMoveCfg/pathMoveActive`，但 RT 侧未见 `PathMove` 命令或持续消费逻辑。
  - 当前 `NodeManager` 仍是“单活动命令”串行模型，不是典型工业控制器的多段缓冲/前瞻/无缝衔接执行模型。
- 缺少实际可用的缓冲/混合运动模式
  - `MC_BUFFER_MODE` 仅见枚举，未见真正用于命令执行调度。
  - 典型工业控制器常见的 `buffered / blending / contouring` 尚未形成完整链路。
- 缺少电子齿轮/电子凸轮/主从同步运动落地
  - `SyncMove`、`CamMove` 只在枚举级存在，未见 RT 命令实现。
- 缺少在线探针/锁存/位置比较等工业采集触发功能落地
  - `Probe`、`PosCapture`、`PosCompare` 已预留结果通道，但未见 RT 执行节点。
- 缺少更完整的工艺运动类型
  - `HelixMove`、`SearchL`、`BufMove`、`GalvoMarkL`、`GalvoBufMark` 尚未落地。

#### B. 工业级鲁棒性差距

- 缺少可观测的实时周期诊断
  - 已有心跳，但未见周期抖动、超周期、deadline miss、回调耗时、总线周期健康的显式上报。
- 缺少完善的 EtherCAT 运行期健康诊断/故障恢复策略
  - 已见主站初始化与周期同步，但未见从站状态退化、working counter 异常、链路恢复、自动重配置等策略闭环。
- 缺少命令级进度/段状态反馈
  - 当前主要只有 `lastCmdSeq/lastCmdResult` 粗粒度完成态，缺少工业控制器常见的“执行中进度、当前段号、失败原因、可恢复点”。
- 缺少轨迹失败后的恢复与续跑机制
  - `ERROR_STATE` 处理更偏“停机+等待复位”，而不是工业级的“定位故障源、保留上下文、支持安全恢复”。

#### C. 运动学保护差距

- 奇异区保护未形成执行闭环
  - `RobotModel` 有 `manipulability()` 与 `isNearSingularity()`，共享内存也有 `singAreaMode`，但未见 `MoveJ/MoveL/MoveC` 执行中实际使用。
- 未见工作空间/碰撞/姿态约束的统一检查层
  - 当前主要是轴级限制，尚未看到工业机器人控制器常见的 TCP 工作空间、关节耦合约束、工具姿态限制、禁行区保护。
- 速度/加速度超限保护未完全启用
  - `Axis::cmdsProcessing()` 中速度/加速度超限逻辑存在，但关键报错与 `return false` 被注释，说明保护策略尚未完全收敛。

#### D. 安全功能差距

- 当前更像“软件停机/急停接口”，不是工业安全链路
  - 已有 `EmergencyStop` 接口、`STOP/RESET` 状态流，但未见与安全 PLC、STO、Safe Limited Speed、双通道安全输入等工业安全机制的集成。
- 缺少系统级互锁框架
  - 尚未看到“伺服使能条件、回零完成条件、门锁/气压/冷却/激光允许运行条件”等统一 interlock 框架。

## Proposed Changes

### 1. 先完成分析交付，不直接改代码

- 输出一份最终差距报告，结构分为：
  - 已实现能力
  - 预留未落地能力
  - 核心缺失能力
  - 建议优先级
- 这样做的原因：
  - 用户当前需求是“查看代码并判断没做哪些功能”，不是立刻编码补齐。

### 2. 若后续进入执行阶段，优先补齐“已预留但未落地”的核心命令链

- `zrcs_common/config/CmdDefine.h`
  - 核对命令枚举是否保留、收缩或分期实现。
- `zrcs_rt/command/`
  - 新增或补齐 `PathMove`、`Probe`、`PosCapture`、`IORead`、`SyncMove`、`CamMove` 等 RT 命令节点。
- `zrcs_rt/command/CmdHead.h`
  - 注册已补齐的命令节点。
- `zrcs_nrt/rt_bridge/RtBridge.h`
  - 继续承担结果读取、配置写入、路径推送和命令完成跟踪。
- `zrcs_nrt/zmq_server/ZmqServer.h`
  - 如需对外暴露特殊系统命令或强类型接口，再补专门路由。
- 为什么先做这批：
  - 这些能力已经有枚举和共享内存基础，增量最小，最接近工业运动控制器“应该具备但还没闭环”的部分。

### 3. 第二优先级补齐“工业级运行鲁棒性”

- `zrcs_rt/system/NodeManager.cpp`
  - 扩展命令执行状态、段状态、错误来源、恢复点记录。
- `zrcs_rt/controller/ethercat/EthercatMaster.h`
  - 增加总线健康检查、从站异常观测、恢复策略。
- `zrcs_common/shared_memory/ShmLayout.h`
  - 增加周期诊断、超时、抖动、段进度、故障原因字段。
- `zrcs_common/message/message.proto`
  - 若需要远程状态订阅，增加对应状态结构。
- 为什么做这批：
  - 工业控制器的差距不只在“有没有命令”，更在“现场异常时能不能诊断、止损、恢复”。

### 4. 第三优先级补齐“运动学保护与工艺安全”

- `zrcs_rt/model/RobotModel.h`
  - 将奇异区检测从工具函数提升为执行期约束。
- `zrcs_rt/command/MoveJ.cpp`、`MoveL.cpp`、`MoveC.cpp`
  - 在轨迹启动和执行阶段增加奇异区、工作空间、姿态约束检查。
- `zrcs_rt/controller/ControllerInterface.cpp`
  - 恢复并收敛速度/加速度超限保护策略。
- 为什么做这批：
  - 工业运动控制器不仅要“能跑”，还要“在边界区域能稳定拒绝危险轨迹”。

## Assumptions & Decisions

- 决策：本次对标基线明确采用“运动控制器内核”，不把权限、配方、工单、追溯等设备管理软件能力纳入主结论。
- 假设：`zrcs_rt` 是当前运动执行核心，`zrcs_nrt` 与 `zrcs_gui` 主要承担桥接、上位机和辅助规划，不改变本次“内核能力”判断。
- 假设：以当前仓库可见代码为准；若某些能力仅在外部闭源模块或现场 PLC 中实现，本次结论会标记为“仓库内未见”，而不是绝对不存在。
- 决策：对“已预留未落地”与“完全缺失”做严格区分，避免把架构预埋误判成已实现。

## Verification Steps

### 本轮只读验证

- 检查 RT 入口与调度链：
  - `zrcs_rt/main.cpp`
  - `zrcs_rt/system/NodeManager.cpp`
- 检查命令覆盖范围：
  - `zrcs_rt/command/CmdHead.h`
  - `zrcs_common/config/CmdDefine.h`
- 检查轴保护与状态机：
  - `zrcs_rt/controller/ControllerInterface.cpp`
  - `zrcs_rt/controller/Global.h`
- 检查共享内存与桥接预留能力：
  - `zrcs_common/shared_memory/ShmLayout.h`
  - `zrcs_nrt/rt_bridge/RtBridge.h`
- 检查运动学保护入口：
  - `zrcs_rt/model/RobotModel.h`

### 进入执行阶段后的交付验证

- 输出最终差距矩阵，保证每个“未做功能”至少有一个仓库证据支撑。
- 对每个差距项标注：
  - `已实现`
  - `预留未落地`
  - `缺失`
  - `建议优先级`
- 若用户后续要求“先补哪几个功能”，可直接从本计划中的优先级与目标文件展开。
