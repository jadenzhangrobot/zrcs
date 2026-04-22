/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#include "command/Enable.h"

void Enable::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(EnableArg::AxisId)]);
}

void Enable::run(void)
{
    if(controller_->axiss.size() > axisId_)
    {
        controller_->axiss[axisId_]->setModeOfOperation();
        double p = controller_->axiss[axisId_]->actualPos();
        controller_->axiss[axisId_]->setAxisPositionCmd(controller_->axiss[axisId_]->actualPos());
        controller_->axiss[axisId_]->syncCmdHistory();
        if(!controller_->axiss[axisId_]->powerOn())
        {
            ERROR_PRINT("�?%d 使能失败\n", axisId_);
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
            controller_->axiss[i]->setModeOfOperation();
            double p = controller_->axiss[i]->actualPos();
            controller_->axiss[i]->setAxisPositionCmd(controller_->axiss[i]->actualPos());
            controller_->axiss[i]->syncCmdHistory();
            if(!controller_->axiss[i]->powerOn())
            {
                ERROR_PRINT("�?%d 使能失败\n", i);
                setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            }
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("使能轴超过限制\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Enable::exit(void)
{
}

CMD_REGISTER(Enable);
