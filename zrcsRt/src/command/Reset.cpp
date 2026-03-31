/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 复位错误指令
 */
#include "command/Reset.h"

void Reset::init()
{
    axisId_ = static_cast<int>(command_->args[ResetAxisId]);
}

void Reset::run(void)
{
    if(controller_->axiss.size() > axisId_)
    {
        if(!controller_->axiss[axisId_]->resetError())
        {
            ERROR_PRINT("Reset: 轴 %d 操作失败\n", axisId_);
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        }
        else
        {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
    }
    else if(controller_->axiss.size() == axisId_)
    {
        for (int i = 0; i < axisId_; i++)
        {
            if(!controller_->axiss[i]->resetError())
            {
                ERROR_PRINT("Reset: 轴 %d 操作失败\n", i);
                setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            }
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("Reset: 轴索引 %d 超出范围(max=%zu)\n", axisId_, controller_->axiss.size());
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Reset::exit(void)
{
}

REGISTERCMD(Reset);
