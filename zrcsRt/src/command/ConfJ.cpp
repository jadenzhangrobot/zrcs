/*
 * @Description: 关节配置监控开关（ABB ConfJ）
 */
#include "command/ConfJ.h"

void ConfJ::init()
{
    bool enabled = static_cast<bool>(command_->args[ConfJOnOff]);
    shm().confJEnabled().store(enabled, std::memory_order_release);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void ConfJ::run(void) {}
void ConfJ::exit(void) {}

REGISTERCMD(ConfJ);
