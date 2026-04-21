/*
 * @Description: 设置数字输出（单bit）
 */
#include "command/SetDO.h"

void SetDO::init()
{
    moduleIndex_ = static_cast<int>(command_->args[ModuleIndex]);
    bitPos_ = static_cast<int>(command_->args[BitPos]);
    value_ = static_cast<bool>(command_->args[Value]);
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
        ERROR_PRINT("SetDO: IO模块索引 %d 超出范围(max=%zu)\n", moduleIndex_, controller_->ios_.size());
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void SetDO::exit(void) {}

CMD_REGISTER(SetDO);
