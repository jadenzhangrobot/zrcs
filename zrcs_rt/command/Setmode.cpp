/*
 * @Description: 设置伺服控制模式命令
 */
#include "command/Setmode.h"

void Setmode::init()
{
    axisId_ = static_cast<int>(command_->args[SetmodeAxisId]);
    mode_ = static_cast<int>(command_->args[SetmodeMode]);
}

void Setmode::run(void)
{
    auto cia402Mode = static_cast<Cia402Mode>(mode_);

    if (controller_->axiss.size() > axisId_)
    {
        controller_->axiss[axisId_]->setModeOfOperation(cia402Mode);
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else if (controller_->axiss.size() == axisId_)
    {
        for (size_t i = 0; i < controller_->axiss.size(); i++)
        {
            controller_->axiss[i]->setModeOfOperation(cia402Mode);
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("Setmode: 轴索引 %d 超出范围\n", axisId_);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Setmode::exit(void) {}

REGISTERCMD(Setmode, 6);
