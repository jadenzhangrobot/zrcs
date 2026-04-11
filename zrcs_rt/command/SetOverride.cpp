/*
 * @Description: 设置全局速度倍率 (0.0 ~ 1.0)
 */
#include "command/SetOverride.h"

void SetOverride::init()
{
    value_ = command_->args[SetOverrideValue];
    if (value_ < 0.0) value_ = 0.0;
    if (value_ > 1.0) value_ = 1.0;
}

void SetOverride::run(void)
{
    shm()->overrideRatio.store(value_, std::memory_order_release);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void SetOverride::exit(void) {}

REGISTERCMD(SetOverride);
