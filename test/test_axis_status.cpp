#include "controller/virtual/VirtualServo.h"

#include <cassert>
#include <memory>

using namespace ZrcsHardware;

class FeedbackServo : public virtualServo {
public:
    FeedbackServo() : virtualServo(0) {}
    ServoStatus feedback = ServoStatus::Disabled;
    bool acceptReset = true;
    bool enableOnCycle = false;
    int cycles = 0;

    ServoStatus getStatus() override { return feedback; }
    bool resetError() override { return acceptReset; }
    void runCycle() override
    {
        ++cycles;
        if (enableOnCycle)
            feedback = ServoStatus::Enabled;
    }
};

int main()
{
    // 仿真后端也通过统一接口提供实际使能状态。
    virtualServo simulated(0);
    assert(simulated.getStatus() == ServoStatus::Disabled);
    simulated.enable();
    assert(simulated.getStatus() == ServoStatus::Enabled);
    simulated.disable();
    assert(simulated.getStatus() == ServoStatus::Disabled);

    Axis axis(0, new AxisPara{});
    axis.cyclerun();
    assert(!axis.isPowerOn());
    assert(axis.getAxisState() == Axis::AxisState::Disabled);

    auto first = std::make_unique<FeedbackServo>();
    auto second = std::make_unique<FeedbackServo>();
    auto* a = first.get();
    auto* b = second.get();
    axis.pushServo(std::move(first), ServoPara{});
    axis.pushServo(std::move(second), ServoPara{});

    // 请求受理不代表反馈已使能；多驱轴须等待最后一个驱动。
    assert(axis.powerOn());
    axis.cyclerun();
    assert(!axis.isPowerOn());
    a->feedback = ServoStatus::Enabled;
    b->feedback = ServoStatus::NotReady;
    axis.cyclerun();
    assert(!axis.isPowerOn());
    assert(axis.getAxisState() == Axis::AxisState::Disabled);

    // runCycle 中的变化要等下一次状态采集才确认。
    b->enableOnCycle = true;
    axis.cyclerun();
    assert(!axis.isPowerOn());
    axis.cyclerun();
    assert(axis.isPowerOn());
    assert(axis.getAxisState() == Axis::AxisState::Standstill);
    b->enableOnCycle = false;

    b->feedback = ServoStatus::Disabled;
    axis.cyclerun();
    assert(!axis.isPowerOn());
    assert(axis.getAxisState() == Axis::AxisState::Disabled);

    // 任一驱动故障优先，故障循环中其他伺服仍然得到周期调用。
    a->feedback = ServoStatus::Fault;
    const int previousCycles = b->cycles;
    axis.cyclerun();
    assert(b->cycles == previousCycles + 1);
    assert(axis.getAxisState() == Axis::AxisState::ErrorStop);
    assert(axis.getAxisError() == MC_ERRORCODE_AXISHARDWARE);
    a->feedback = b->feedback = ServoStatus::Enabled;
    axis.cyclerun();
    assert(axis.isPowerOn());
    assert(axis.getAxisState() == Axis::AxisState::ErrorStop);

    // 复位未受理时，不允许反馈恢复自动清错。
    b->acceptReset = false;
    assert(!axis.resetError());
    axis.cyclerun();
    assert(axis.getAxisState() == Axis::AxisState::ErrorStop);
    b->acceptReset = true;
    assert(axis.resetError());
    a->feedback = ServoStatus::Fault;
    axis.cyclerun();
    assert(axis.getAxisState() == Axis::AxisState::ErrorStop);
    a->feedback = ServoStatus::Unknown;
    axis.cyclerun();
    assert(axis.getAxisState() == Axis::AxisState::ErrorStop);
    a->feedback = ServoStatus::Enabled;
    axis.cyclerun();
    assert(axis.getAxisError() == MC_ERRORCODE_GOOD);
    assert(axis.getAxisState() == Axis::AxisState::Standstill);

    // 已存在的软件错误停止也不能被正常伺服反馈覆盖。
    axis.setAxisState(Axis::AxisState::ErrorStop);
    axis.cyclerun();
    assert(axis.getAxisState() == Axis::AxisState::ErrorStop);
    assert(axis.resetError());
    axis.cyclerun();
    assert(axis.getAxisState() == Axis::AxisState::Standstill);

    b->feedback = ServoStatus::QuickStop;
    axis.cyclerun();
    assert(!axis.isPowerOn());
    assert(axis.getAxisState() == Axis::AxisState::ErrorStop);
}
