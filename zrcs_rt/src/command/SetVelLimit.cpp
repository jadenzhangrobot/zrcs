/*
 * @Description: 动态设置速度限制（固高 GT_SetAxisPrm）
 */
#include "command/SetVelLimit.h"

void SetVelLimit::init()
{
    int axisId = static_cast<int>(command_->args[SetVelLimitAxisId]);
    double maxVel = command_->args[SetVelLimitMaxVel];
    double maxAcc = command_->args[SetVelLimitMaxAcc];
    double maxJerk = command_->args[SetVelLimitMaxJerk];

    if (axisId < 0 || axisId >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("SetVelLimit: 轴索引 %d 超出范围\n", axisId);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    controller_->axiss[axisId]->setVelLimits(maxVel, maxAcc, maxJerk);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void SetVelLimit::run(void) {}
void SetVelLimit::exit(void) {}

REGISTERCMD(SetVelLimit);
