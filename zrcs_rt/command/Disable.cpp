/**
 * @file    Disable.cpp
 * @brief   电机失能命令实现
 * @author  zhangyongjing
 * @date    2023-03-28
 */

#include "command/Disable.h"

void Disable::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(DisableArg::AxisId)]);
}

void Disable::run()
{
    if (controller_->axes_.size() > static_cast<size_t>(axisId_))
    {
        if (!controller_->axes_[axisId_]->powerOff())
        {
            ERROR_PRINT("Disable: 轴%d 操作失败\n", axisId_);
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        }
        else
        {
            setCmdStatus(zrcsSystem::CmdStatus::COMPLETED);
        }
    }
    else if (static_cast<int>(controller_->axes_.size()) == axisId_)
    {
        for (int i = 0; i < axisId_; i++)
        {
            if (!controller_->axes_[i]->powerOff())
            {
                ERROR_PRINT("Disable: 轴%d 操作失败\n", i);
                setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            }
        }
        setCmdStatus(zrcsSystem::CmdStatus::COMPLETED);
    }
    else
    {
        ERROR_PRINT("Disable: 轴索引%d 超出范围(max=%zu)\n",
                     axisId_, controller_->axes_.size());
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Disable::exit()
{
}

CMD_REGISTER(Disable);
