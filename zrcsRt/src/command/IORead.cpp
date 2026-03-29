/*
 * @Description: 读取数字/模拟输入
 */
#include "command/IORead.h"

void IORead::init()
{
    moduleIndex_ = static_cast<int>(command_->args[IOReadModuleIndex]);
    bitPos_ = static_cast<int>(command_->args[IOReadBitPos]);
    type_ = static_cast<int>(command_->args[IOReadType]);
}

void IORead::run(void)
{
    if (moduleIndex_ >= 0 &&
        moduleIndex_ < static_cast<int>(controller_->ios_.size()))
    {
        bool val = controller_->ios_[moduleIndex_]->ioRead32(moduleIndex_, bitPos_);
        shm().ioReadResult().store(val ? 1 : 0, std::memory_order_release);
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void IORead::exit(void) {}

REGISTERCMD(IORead);
