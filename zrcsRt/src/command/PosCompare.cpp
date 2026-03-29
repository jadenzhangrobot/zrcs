/*
 * @Description: 位置比较输出（固高 GT_SetCompare / 正运动 HW_PSWITCH）
 *               当轴到达指定位置时自动触发IO输出
 */
#include "command/PosCompare.h"

void PosCompare::init()
{
    axisId_ = static_cast<int>(command_->args[PosCompareAxisId]);
    targetPos_ = command_->args[PosComparePosition];
    ioModule_ = static_cast<int>(command_->args[PosCompareIOModule]);
    bitPos_ = static_cast<int>(command_->args[PosCompareBitPos]);
    ioValue_ = static_cast<bool>(command_->args[PosCompareValue]);
    triggered_ = false;

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
}

void PosCompare::run(void)
{
    if (triggered_)
    {
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        return;
    }

    double currentPos = controller_->axiss[axisId_]->actualPos();
    double threshold = 0.01;  // position compare tolerance

    if (std::abs(currentPos - targetPos_) <= threshold)
    {
        if (ioModule_ >= 0 &&
            ioModule_ < static_cast<int>(controller_->ios_.size()))
        {
            controller_->ios_[ioModule_]->ioWrite32(ioModule_, bitPos_, ioValue_);
        }
        triggered_ = true;
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
}

void PosCompare::exit(void) {}

REGISTERCMD(PosCompare);
