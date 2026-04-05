/*
 * @Description: 关节运动相对位置指令
 */
#include "command/JogJ.h"

bool JogJ::initTrajectory()
{
    axisId_ = static_cast<int>(command_->args[JogjAxisId]);
    position_ = command_->args[JogjTargetPosition];

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("JogJ: 轴索引 %d 超出范围\n", axisId_);
        return false;
    }

    input_.current_position[0] = controller_->axiss[axisId_]->actualPos();
    input_.current_velocity[0] = 0;
    input_.current_acceleration[0] = 0;
    input_.target_position[0] = position_ + controller_->axiss[axisId_]->actualPos();
    input_.target_velocity[0] = 0;
    input_.target_acceleration[0] = 0;
    input_.max_velocity[0] = controller_->axiss[axisId_]->getMaxVelocity();
    input_.max_acceleration[0] = controller_->axiss[axisId_]->getMaxAcceleration();
    input_.max_jerk[0] = controller_->axiss[axisId_]->getMaxJerk();
    return true;
}

REGISTERCMD(JogJ);
