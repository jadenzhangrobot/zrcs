/*
 * @Description: 受控停止命令
 */
#include "command/Stop.h"

void Stop::init()
{
    axisId_ = static_cast<int>(command_->args[StopAxisId]);
}

void Stop::run(void)
{
    if (controller_->axiss.size() > axisId_)
    {
        // 将目标位置设为当前位置，使轴停在原地
        controller_->axiss[axisId_]->setAxisPositionCmd(
            controller_->axiss[axisId_]->actualPos());
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else if (controller_->axiss.size() == axisId_)
    {
        // 停止所有轴
        for (size_t i = 0; i < controller_->axiss.size(); i++)
        {
            controller_->axiss[i]->setAxisPositionCmd(
                controller_->axiss[i]->actualPos());
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("Stop: 轴索引 %d 超出范围(max=%zu)\n", axisId_, controller_->axiss.size());
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Stop::exit(void) {}

REGISTERCMD(Stop);
