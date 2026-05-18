/**
 * @file    Reset.cpp
 * @brief   复位错误命令实现
 * @author  zhangyongjing
 * @date    2023-03-28
 */

#include "command/Reset.h"

bool Reset::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(ResetArg::AxisId)]);
    return true;
}

zrcsSystem::RunResult Reset::run()
{
    if (controller_->axes_.size() > static_cast<size_t>(axisId_))
    {
        if (!controller_->axes_[axisId_]->resetError())
        {
            ERROR_PRINT("Reset: 轴%d 操作失败\n", axisId_);
            return zrcsSystem::RunResult::FAILED;
        }
        else
        {
            controller_->axes_[axisId_]->setAxisPositionCmd(
                controller_->axes_[axisId_]->actualPos());
            controller_->axes_[axisId_]->syncCmdHistory();
        }
        return zrcsSystem::RunResult::SUCCESS;
    }
    else if (static_cast<int>(controller_->axes_.size()) == axisId_)
    {
        for (int i = 0; i < axisId_; i++)
        {
            if (!controller_->axes_[i]->resetError())
            {
                ERROR_PRINT("Reset: 轴%d 操作失败\n", i);
            }
            else
            {
                controller_->axes_[i]->setAxisPositionCmd(
                    controller_->axes_[i]->actualPos());
                controller_->axes_[i]->syncCmdHistory();
            }
        }
        return zrcsSystem::RunResult::SUCCESS;
    }
    else
    {
        ERROR_PRINT("Reset: 轴索引%d 超出范围(max=%zu)\n",
                     axisId_, controller_->axes_.size());
        return zrcsSystem::RunResult::FAILED;
    }
}

bool Reset::exit()
{
    return true;
}

CMD_REGISTER(Reset);
