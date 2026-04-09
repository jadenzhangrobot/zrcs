/*
 * @Description: 动态设置软限位（固高 GT_SetSoftLimit）
 */
#include "command/SetPosLimit.h"

void SetPosLimit::init()
{
    int axisId = static_cast<int>(command_->args[SetPosLimitAxisId]);
    double posLimit = command_->args[SetPosLimitPositive];
    double negLimit = command_->args[SetPosLimitNegative];

    if (axisId < 0 || axisId >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("SetPosLimit: 轴索引 %d 超出范围\n", axisId);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    controller_->axiss[axisId]->setPosLimits(posLimit, negLimit);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void SetPosLimit::run(void) {}
void SetPosLimit::exit(void) {}

REGISTERCMD(SetPosLimit);
