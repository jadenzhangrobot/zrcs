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
                setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            }
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Reset::exit(void)
{
}

REGISTERCMD(Reset);
