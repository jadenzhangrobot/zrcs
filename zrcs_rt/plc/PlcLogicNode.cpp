/* PlcLogicNode.cpp — PLC 扫描周期节点实现。
 *
 * 实时合规（.claude/skills/realtime/skill.md）：
 *   - run() 内无动态分配、无异常、无阻塞系统调用；
 *   - 时间用编译期常量的周期积分，不调用时钟，避免系统调用与回溯；
 *   - PLC 初始化在 onRegistered()（RT 任务启动前）完成。
 */
#include "plc/PlcLogicNode.h"

#include <cstring>
#include <stdexcept>
#include <string>

#include "plc/PlcProgram.h"
#include "system/node/NodeFactory.h"

namespace matiec {
extern "C" {
// MatIEC 定时器要求宿主提供的时间，不再维护另一份桥接时间变量。
TIME __CURRENT_TIME = {};
}
} // namespace matiec

PlcLogicNode::PlcLogicNode()
{
    std::strncpy(nodeName_, "PlcLogicNode", sizeof(nodeName_) - 1);
    nodeName_[sizeof(nodeName_) - 1] = '\0';
}

void PlcLogicNode::onRegistered()
{
    // MatIEC 的 tick 是生成任务的公共基准周期。只有与宿主周期一致时，
    // 每个 RT 扫描 tick++ 才能同时保证任务调度和 TON 等定时器的时间正确。
    // 此校验在启动 RT 线程之前执行，禁止带着不匹配的生成物进入控制循环。
    if (matiec::common_ticktime__ != static_cast<unsigned long long>(kCycleNs)) {
        throw std::runtime_error(
            "PLC task period mismatch: generated=" +
            std::to_string(matiec::common_ticktime__) + " ns, RT=" +
            std::to_string(kCycleNs) + " ns; regenerate PLC for this BUILD_MODE");
    }

    // 只在 RT 启动前初始化，首个周期的 init() 不再重复重置 PLC。
    matiec::__CURRENT_TIME = {};
    matiec::config_init__();
    tick_        = 0;
    scanTimeNs_  = 0;
}

void PlcLogicNode::init()
{
    // MatIEC 状态已经在 onRegistered() 中初始化。
}

void PlcLogicNode::run()
{
    // ── 0) RUN 门控：非 RUN 状态冻结，不推进扫描也不输出，防止越过安全停机 ──
    const bool runState = shm()->taskSched.load(std::memory_order_acquire) ==
                          zrcs::TaskScheduling::RUN;
    if (!runState) {
        return;
    }

    // 输入由 PlcInputNode 在 INPUT 阶段准备，此处只负责时间和扫描。
    // ── 1) 按宿主控制周期推进 MatIEC 时间 ──────────────────────────────
    scanTimeNs_ += kCycleNs;
    matiec::__CURRENT_TIME.tv_sec  = static_cast<int32_t>(scanTimeNs_ / kNsecPerSec);
    matiec::__CURRENT_TIME.tv_nsec = static_cast<int32_t>(scanTimeNs_ % kNsecPerSec);

    // ── 2) 直接执行 MatIEC 配置入口 ────────────────────────────────────
    matiec::config_run__(static_cast<unsigned long>(tick_++));
}

REGISTER_PERIODIC(PlcLogicNode, OUTPUT, 100);
