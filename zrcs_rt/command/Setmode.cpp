/**
 * @file    Setmode.cpp
 * @brief   设置伺服控制模式命令实现
 */

#include "command/Setmode.h"

void Setmode::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(SetmodeArg::AxisId)]);
    mode_    = static_cast<int>(command_->args[static_cast<size_t>(SetmodeArg::Mode)]);
}

void Setmode::run()
{
    auto cia402_mode = static_cast<Cia402Mode>(mode_);

    if (controller_->axes_.size() > static_cast<size_t>(axisId_))
    {
        controller_->axes_[axisId_]->setModeOfOperation(cia402_mode);
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else if (static_cast<int>(controller_->axes_.size()) == axisId_)
    {
        for (size_t i = 0; i < controller_->axes_.size(); i++)
        {
            controller_->axes_[i]->setModeOfOperation(cia402_mode);
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("Setmode: 轴索引%d 超出范围\n", axisId_);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Setmode::exit()
{
}

CMD_REGISTER(Setmode);
