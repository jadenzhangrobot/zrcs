/*
 * @Description: 脉冲数字输出（输出一段时间后自动复位）
 */
#include "command/PulseDO.h"

void PulseDO::init()
{
    moduleIndex_ = static_cast<int>(command_->args[PulseDOModuleIndex]);
    bitPos_ = static_cast<int>(command_->args[PulseDOBitPos]);
    pulseTimeMs_ = command_->args[PulseDOTimeMs];
    startCount_ = nodeCount_;

    // 立即置位输出
    if (moduleIndex_ >= 0 &&
        moduleIndex_ < static_cast<int>(controller_->ios_.size()))
    {
        controller_->ios_[moduleIndex_]->ioWrite32(moduleIndex_, bitPos_, true);
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void PulseDO::run(void)
{
    double elapsedMs = (nodeCount_ - startCount_) * cycletime;
    if (elapsedMs >= pulseTimeMs_)
    {
        // 脉冲时间到，复位输出
        controller_->ios_[moduleIndex_]->ioWrite32(moduleIndex_, bitPos_, false);
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    nodeCount_++;
}

void PulseDO::exit(void) {}

REGISTERCMD(PulseDO);
