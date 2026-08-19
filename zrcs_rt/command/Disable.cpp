/**
 * @file    Disable.cpp
 * @brief   电机失能命令实现
 * @author  zhangyongjing
 * @date    2023-03-28
 */

#include "command/Disable.h"
#include "system/log/RtLog.h"
#include "system/node/BaseNodeInterface.h"

bool Disable::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(DisableArg::AxisId)]);
    return true;
}

zrcsSystem::RunResult Disable::run()
{
    if (controller_->axes_.size() > static_cast<size_t>(axisId_))
    {
        if (!controller_->axes_[axisId_]->powerOff())
        {
            ERROR_PRINT("Disable: 轴%d 操作失败\n", axisId_);
            return zrcsSystem::RunResult::FAILED;
        }
        INFO_PRINT("Disable: 轴%d 操作成功\n", axisId_);
        return zrcsSystem::RunResult::SUCCESS;
    }
    else if (static_cast<int>(controller_->axes_.size()) == axisId_)
    {
        bool anyFailed = false;
        for (int i = 0; i < axisId_; i++)
        {
            if (!controller_->axes_[i]->powerOff())
            {
                ERROR_PRINT("Disable: 轴%d 操作失败\n", i);
                anyFailed = true;
            }
            else
            {
                INFO_PRINT("Disable: 轴%d 操作成功\n", i);
            }
        }
        return anyFailed ? zrcsSystem::RunResult::FAILED
                         : zrcsSystem::RunResult::SUCCESS;
    }
    else
    {
        ERROR_PRINT("Disable: 轴索引%d 超出范围(max=%zu)\n",
                     axisId_, controller_->axes_.size());
        return zrcsSystem::RunResult::FAILED;
    }
}

bool Disable::exit()
{
    return true;
}

CMD_REGISTER(Disable);
