/*
 * @Description: 设置数字输出（单bit）
 */
#include "command/SetDO.h"

void SetDO::init()
{
    moduleIndex_ = static_cast<int>(command_->args[SetDOModuleIndex]);
    bitPos_ = static_cast<int>(command_->args[SetDOBitPos]);
    value_ = static_cast<bool>(command_->args[SetDOValue]);
}

void SetDO::run(void)
{
    if (moduleIndex_ >= 0 &&
        moduleIndex_ < static_cast<int>(controller_->ios_.size()))
    {
        controller_->ios_[moduleIndex_]->ioWrite32(moduleIndex_, bitPos_, value_);
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void SetDO::exit(void) {}

REGISTERCMD(SetDO);
