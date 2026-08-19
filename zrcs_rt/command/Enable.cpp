/*
 * @file    Enable.cpp
 * @brief   电机使能命令实现
 * @author  zhangyongjing
 * @date    2023-03-28
 */
#include "command/Enable.h"

bool Enable::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(EnableArg::AxisId)]);
    return true;
}

zrcsSystem::RunResult Enable::run()
{
    if (controller_->axes_.size() > static_cast<size_t>(axisId_))
    {
        controller_->axes_[axisId_]->setModeOfOperation();
        controller_->axes_[axisId_]->setAxisPositionCmd(controller_->axes_[axisId_]->actualPos());
        controller_->axes_[axisId_]->syncCmdHistory();
        if (!controller_->axes_[axisId_]->powerOn())
        {
            ERROR_PRINT("Enable: 轴%d 使能失败\n", axisId_);
            return zrcsSystem::RunResult::FAILED;
        }
        return zrcsSystem::RunResult::SUCCESS;
    }
    else if (static_cast<int>(controller_->axes_.size()) == axisId_)
    {
        bool anyFailed = false;
        for (int i = 0; i < axisId_; i++)
        {
            controller_->axes_[i]->setModeOfOperation();
            controller_->axes_[i]->setAxisPositionCmd(controller_->axes_[i]->actualPos());
            controller_->axes_[i]->syncCmdHistory();
            if (!controller_->axes_[i]->powerOn())
            {
                ERROR_PRINT("Enable: 轴%d 使能失败\n", i);
                anyFailed = true;
            }
        }
        return anyFailed ? zrcsSystem::RunResult::FAILED
                         : zrcsSystem::RunResult::SUCCESS;
    }
    else
    {
        ERROR_PRINT("Enable: 轴索引%d 超出范围\n", axisId_);
        return zrcsSystem::RunResult::FAILED;
    }
}

bool Enable::exit()
{
    return true;
}

CMD_REGISTER(Enable);
