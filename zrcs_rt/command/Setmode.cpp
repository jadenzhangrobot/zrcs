/**
 * @file    Setmode.cpp
 * @brief   设置伺服控制模式命令实现
 */

#include "command/Setmode.h"

bool Setmode::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(SetmodeArg::AxisId)]);
    mode_    = static_cast<int>(command_->args[static_cast<size_t>(SetmodeArg::Mode)]);
    return true;
}

zrcsSystem::RunResult Setmode::run()
{
    auto cia402_mode = static_cast<Cia402Mode>(mode_);

    if (controller_->axes_.size() > static_cast<size_t>(axisId_))
    {
        controller_->axes_[axisId_]->setModeOfOperation(cia402_mode);
        return zrcsSystem::RunResult::SUCCESS;
    }
    else if (static_cast<int>(controller_->axes_.size()) == axisId_)
    {
        for (size_t i = 0; i < controller_->axes_.size(); i++)
        {
            controller_->axes_[i]->setModeOfOperation(cia402_mode);
        }
        return zrcsSystem::RunResult::SUCCESS;
    }
    else
    {
        ERROR_PRINT("Setmode: 轴索引%d 超出范围\n", axisId_);
        return zrcsSystem::RunResult::FAILED;
    }
}

bool Setmode::exit()
{
    return true;
}

CMD_REGISTER(Setmode);
