/*
 * @Description: 设置当前位置为零点
 */
#include "command/SetZero.h"

void SetZero::init()
{
    axisId_ = static_cast<int>(command_->args[SetZeroAxisId]);
}

void SetZero::run(void)
{
    if (controller_->axiss.size() > axisId_)
    {
        controller_->axiss[axisId_]->setZeroOffset(
            controller_->axiss[axisId_]->actualPos());
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else if (controller_->axiss.size() == axisId_)
    {
        for (size_t i = 0; i < controller_->axiss.size(); i++)
        {
            controller_->axiss[i]->setZeroOffset(
                controller_->axiss[i]->actualPos());
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("SetZero: 轴索引 %d 超出范围\n", axisId_);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void SetZero::exit(void) {}

REGISTERCMD(SetZero, 10);
