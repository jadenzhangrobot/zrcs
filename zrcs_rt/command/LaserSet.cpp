/// @file LaserSet.cpp
/// @brief 设置激光参数实现
#include "command/LaserSet.h"

void LaserSet::init()
{
    power_ = command_->args[LaserSetPower];
    frequency_ = command_->args[LaserSetFreq];
    pulseWidth_ = command_->args[LaserSetPulseWidth];
    gate_ = command_->args[LaserSetGate] > 0;
}

void LaserSet::run()
{
    auto* laser = controller_->laser();
    if (!laser) {
        ERROR_PRINT("LaserSet: 激光控制器未初始化\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    laser->setPower(power_);
    laser->setFrequency(frequency_);
    laser->setPulseWidth(pulseWidth_);
    laser->setGate(gate_);
    laser->applyToHardware();

    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

REGISTERCMD(LaserSet);
