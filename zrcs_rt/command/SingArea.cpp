/*
 * @Description: 奇异区域处理策略（ABB SingArea）
 */
#include "command/SingArea.h"

void SingArea::init()
{
    uint8_t mode = static_cast<uint8_t>(command_->args[SingAreaMode]);
    shm()->singAreaMode.store(mode, std::memory_order_release);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void SingArea::run(void) {}
void SingArea::exit(void) {}

REGISTERCMD(SingArea);
