/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#include "command/JogJ.h"

void JogJ::init()
{
    axisId_ = static_cast<int>(command_->args[JogjAxisId]);
    position_ = command_->args[JogjTargetPosition];
    input_.current_position[0] = controller_->axiss[axisId_]->actualPos();
    input_.current_velocity[0] = 0;
    input_.current_acceleration[0] = 0;
    input_.target_position[0] = position_ + controller_->axiss[axisId_]->actualPos();
    input_.target_velocity[0] = 0;
    input_.target_acceleration[0] = 0;
    input_.max_velocity[0] = controller_->axiss[axisId_]->getMaxVelocity();
    input_.max_acceleration[0] = controller_->axiss[axisId_]->getMaxAcceleration();
    input_.max_jerk[0] = controller_->axiss[axisId_]->getMaxJerk();
}

void JogJ::run(void)
{
    if(otg_.update(input_, output_) == Result::Working)
    {
        auto& p = output_.new_position;
        auto& v = output_.new_velocity;
        auto& a = output_.new_acceleration;
        if (controller_ != nullptr && controller_->axiss.size() > axisId_)
        {
            controller_->axiss[axisId_]->setAxisPositionCmd(p[0]);
            output_.pass_to_input(input_);
        }
    }
    else if(otg_.update(input_, output_) == Result::Finished)
    {
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("JogJ: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void JogJ::exit(void)
{
}

REGISTERCMD(JogJ);
