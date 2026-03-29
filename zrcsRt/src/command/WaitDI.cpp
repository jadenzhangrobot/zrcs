/*
 * @Description: 等待数字输入信号达到指定值
 */
#include "command/WaitDI.h"

void WaitDI::init()
{
    moduleIndex_ = static_cast<int>(command_->args[WaitDIModuleIndex]);
    bitPos_ = static_cast<int>(command_->args[WaitDIBitPos]);
    targetValue_ = static_cast<bool>(command_->args[WaitDIValue]);
    timeoutMs_ = command_->args[WaitDITimeoutMs];
    startCount_ = nodeCount_;
}

void WaitDI::run(void)
{
    if (moduleIndex_ >= 0 &&
        moduleIndex_ < static_cast<int>(controller_->ios_.size()))
    {
        bool currentVal = controller_->ios_[moduleIndex_]->ioRead32(moduleIndex_, bitPos_);
        if (currentVal == targetValue_)
        {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
            return;
        }
    }

    // 检查超时 (timeoutMs_ <= 0 表示无限等待)
    if (timeoutMs_ > 0)
    {
        double elapsedMs = (nodeCount_ - startCount_) * cycletime;
        if (elapsedMs >= timeoutMs_)
        {
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            return;
        }
    }
    nodeCount_++;
}

void WaitDI::exit(void) {}

REGISTERCMD(WaitDI);
