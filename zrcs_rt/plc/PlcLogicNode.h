/* PlcLogicNode.h — PLC 扫描周期节点（逻辑/联锁层）
 *
 * 在 RT 循环内直接运行 MatIEC 编译的 IEC 61131-3 逻辑。
 * 继承 PeriodicNode（Phase::OUTPUT），经 REGISTER_PERIODIC 注册，由 NodeManager
 * 在输出相位（每周期一次）调用 execute()（同 ContinuousJog 的落点），不开新线程、
 * 不改调度器。
 *
 * 每周期流程：
 *   PlcInputNode(INPUT, 100) 写入输入
 *   -> PlcLogicNode(OUTPUT, 100) 推进时间并执行 config_run__(tick)
 *   -> PlcOutputNode(OUTPUT, 110) 读取输出。
 *
 * 安全策略（docs/matiec-rt-integration.md §7）：
 *   - 仅在 TaskScheduling::RUN 下推进扫描与输出；
 *   - 其余状态（STOP/ERROR/SHUTDOWN/IDLE）冻结，避免 PLC 越过安全停机；
 *   - PLC 是逻辑/联锁层，轨迹激活时轨迹层优先，本节点不写轴位置指令。
 */
#pragma once

#include <cstdint>

#include "system/node/BaseNodeInterface.h"
#include "shared_memory/ShmLayout.h"   // zrcs::TaskScheduling

class PlcLogicNode : public zrcsSystem::PeriodicNode {
public:
    PlcLogicNode();
    ~PlcLogicNode() override = default;

    /// 调度器的首次周期入口；PLC 已在 onRegistered() 中初始化。
    void init() override;

    /// 每周期：推进时间 -> config_run__（RUN 门控）。
    void run() override;

protected:
    /// 注册时清空 PLC 状态（RT 启动前执行）。
    void onRegistered() override;

private:
    // ── 编译期常量 ───────────────────────────────────────────
    static_assert(ZRCS_CYCLE_TIME_MS > 0, "PLC cycle must be positive");
    static_assert(cycletime == ZRCS_CYCLE_TIME_MS, "RT cycle definitions must agree");
    static constexpr int64_t     kCycleNs    = static_cast<int64_t>(ZRCS_CYCLE_TIME_MS) * 1000000;
    static constexpr int64_t     kNsecPerSec = 1000000000LL;

    // ── 成员 ───────────────────────────────────────────────
    uint64_t tick_          = 0;   // PLC 扫描计数
    int64_t  scanTimeNs_    = 0;   // 单调时间戳（ns，由 tick 积分，永不回溯）
};
