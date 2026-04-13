/*
 * @Description: 设置模拟输出
 */
#include "command/SetAO.h"

void SetAO::init()
{
    moduleIndex_ = static_cast<int>(command_->args[SetAOModuleIndex]);
    channel_ = static_cast<int>(command_->args[SetAOChannel]);
    value_ = command_->args[SetAOValue];
}

void SetAO::run(void)
{
    if (moduleIndex_ >= 0 &&
        moduleIndex_ < static_cast<int>(controller_->ios_.size()))
    {
        // 模拟输出通过 writeVal 接口（预留扩展）
        // 暂使用 ioWrite32 写入通道值的整数近似
        controller_->ios_[moduleIndex_]->ioWrite32(moduleIndex_, channel_,
                                                    static_cast<bool>(value_ > 0));
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("SetAO: IO模块索引 %d 超出范围(max=%zu)\n", moduleIndex_, controller_->ios_.size());
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void SetAO::exit(void) {}

REGISTERCMD(SetAO, 13);
