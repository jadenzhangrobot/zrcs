/*
 * @Description: 直线运动配置监控开关（ABB ConfL）
 */
#include "command/ConfL.h"

void ConfL::init()
{
    bool enabled = static_cast<bool>(command_->args[ConfLOnOff]);
    shm()->confLEnabled.store(enabled, std::memory_order_release);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void ConfL::run(void) {}
void ConfL::exit(void) {}

REGISTERCMD(ConfL);
