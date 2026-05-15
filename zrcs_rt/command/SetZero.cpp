/**
 * @file    SetZero.cpp
 * @brief   设置当前位置为零点命令实现
 */

#include "command/SetZero.h"

void SetZero::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(SetZeroArg::AxisId)]);
}

void SetZero::run()
{
    if (controller_->axes_.size() > static_cast<size_t>(axisId_))
    {
        controller_->axes_[axisId_]->setZeroOffset(
            controller_->axes_[axisId_]->actualPos());
        controller_->axes_[axisId_]->setAxisPositionCmd(0.0);
        controller_->axes_[axisId_]->syncCmdHistory();
        setCmdStatus(zrcsSystem::CmdStatus::COMPLETED);
    }
    else if (static_cast<int>(controller_->axes_.size()) == axisId_)
    {
        for (size_t i = 0; i < controller_->axes_.size(); i++)
        {
            controller_->axes_[i]->setZeroOffset(
                controller_->axes_[i]->actualPos());
            controller_->axes_[i]->setAxisPositionCmd(0.0);
            controller_->axes_[i]->syncCmdHistory();
        }
        setCmdStatus(zrcsSystem::CmdStatus::COMPLETED);
    }
    else
    {
        ERROR_PRINT("SetZero: 轴索引%d 超出范围\n", axisId_);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void SetZero::exit()
{
}

CMD_REGISTER(SetZero);
