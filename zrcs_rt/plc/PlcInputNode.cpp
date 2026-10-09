#include "plc/PlcInputNode.h"

#include <cstring>
#include <type_traits>

#include "plc/PlcProgram.h"
#include "system/node/NodeFactory.h"

namespace {

constexpr std::size_t kAxisMapMax =
    std::extent_v<decltype(matiec::ZRCS__DEMO.I_AXISPOS.value.table)>;
static_assert(kAxisMapMax ==
              std::extent_v<decltype(matiec::ZRCS__DEMO.I_AXISENABLED.value.table)>,
              "PLC position and enable inputs must have matching lengths");

} // namespace

PlcInputNode::PlcInputNode()
{
    std::strncpy(nodeName_, "PlcInputNode", sizeof(nodeName_) - 1);
    nodeName_[sizeof(nodeName_) - 1] = '\0';
}

void PlcInputNode::init()
{
    // PLC 实例由 PlcLogicNode 在 RT 启动前统一初始化。
}

void PlcInputNode::run()
{
    if (shm()->taskSched.load(std::memory_order_acquire) != zrcs::TaskScheduling::RUN) {
        return;
    }

    // 在此添加 RT 变量 -> PLC 输入的映射，扫描前采样，保留 LREAL 与 FORCE 语义。
    for (std::size_t i = 0; i < kAxisMapMax; ++i) {
        auto* axis = i < controller_->axes_.size() ? controller_->axes_[i].get() : nullptr;
        const double position = axis ? axis->actualPos() : 0.0;
        const bool enabled = axis && axis->isPowerOn() &&
            axis->getAxisState() != ZrcsHardware::Axis::AxisState::ErrorStop &&
            axis->getAxisError() == MC_ERRORCODE_GOOD;
        __SET_VAR(matiec::ZRCS__DEMO., I_AXISPOS, .table[i], position);
        __SET_VAR(matiec::ZRCS__DEMO., I_AXISENABLED, .table[i], enabled);
    }
}

REGISTER_PERIODIC(PlcInputNode, INPUT, 100);
