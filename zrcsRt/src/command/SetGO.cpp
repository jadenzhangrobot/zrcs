/*
 * @Description: 设置组输出（多bit整数）
 */
#include "command/SetGO.h"

void SetGO::init()
{
    moduleIndex_ = static_cast<int>(command_->args[SetGOModuleIndex]);
    value_ = static_cast<int>(command_->args[SetGOValue]);
}

void SetGO::run(void)
{
    if (moduleIndex_ >= 0 &&
        moduleIndex_ < static_cast<int>(controller_->ios_.size()))
    {
        // 逐bit写入组输出值
        for (int bit = 0; bit < 32; bit++)
        {
            bool bitVal = (value_ >> bit) & 1;
            controller_->ios_[moduleIndex_]->ioWrite32(moduleIndex_, bit, bitVal);
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("SetGO: IO模块索引 %d 超出范围(max=%zu)\n", moduleIndex_, controller_->ios_.size());
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void SetGO::exit(void) {}

REGISTERCMD(SetGO);
