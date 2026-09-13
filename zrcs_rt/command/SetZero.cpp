/**
 * @file    SetZero.cpp
 * @brief   设置当前位置为零点命令实现
 */

#include "command/SetZero.h"

bool SetZero::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(SetZeroArg::AxisId)]);
    return true;
}

zrcsSystem::RunResult SetZero::run()
{
    if (controller_->axes_.size() > static_cast<size_t>(axisId_))
    {
        controller_->axes_[axisId_]->setZeroOffset(
            controller_->axes_[axisId_]->actualPos());
        controller_->axes_[axisId_]->setAxisPositionCmd(0.0);
        controller_->axes_[axisId_]->syncCmdHistory();
        INFO_PRINT("SetZero: 轴%d 操作成功\n", axisId_);
        return zrcsSystem::RunResult::SUCCESS;
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
        INFO_PRINT("SetZero: 所有轴操作成功\n");
        return zrcsSystem::RunResult::SUCCESS;
    }
    else
    {
        ERROR_PRINT("SetZero: 轴索引%d 超出范围\n", axisId_);
        return zrcsSystem::RunResult::FAILED;
    }
}

bool SetZero::exit()
{
    return true;
}

REGISTERCMD(SetZero);
