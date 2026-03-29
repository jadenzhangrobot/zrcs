/*
 * @Description: 紧急停止命令 — 立即断使能所有轴
 */
#include "command/EmergStop.h"

void EmergStop::init() {}

void EmergStop::run(void)
{
    for (size_t i = 0; i < controller_->axiss.size(); i++)
    {
        controller_->axiss[i]->powerOff();
    }
    shm().taskScheduling().store(TaskScheduling::STOP, std::memory_order_release);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void EmergStop::exit(void) {}

REGISTERCMD(EmergStop);
