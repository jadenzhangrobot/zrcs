/*
 * @Description: 通用单轴相对定位（带可选速度/加速度/加加速度参数）
 */
#include "command/MoveRel.h"

bool MoveRel::initTrajectory()
{
    axisId_ = static_cast<int>(command_->args[MoveRelAxisId]);
    double distance = command_->args[MoveRelDistance];
    double vel = command_->args[MoveRelVel];
    double acc = command_->args[MoveRelAcc];
    double jerk = command_->args[MoveRelJerk];

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("MoveRel: 轴索引 %d 超出范围\n", axisId_);
        return false;
    }

    double currentPos = controller_->axiss[axisId_]->actualPos();
    input_.current_position[0] = currentPos;
    input_.current_velocity[0] = 0;
    input_.current_acceleration[0] = 0;
    input_.target_position[0] = currentPos + distance;
    input_.target_velocity[0] = 0;
    input_.target_acceleration[0] = 0;

    input_.max_velocity[0] = vel > 0 ? vel : controller_->axiss[axisId_]->getMaxVelocity();
    input_.max_acceleration[0] = acc > 0 ? acc : controller_->axiss[axisId_]->getMaxAcceleration();
    input_.max_jerk[0] = jerk > 0 ? jerk : controller_->axiss[axisId_]->getMaxJerk();
    return true;
}

REGISTERCMD(MoveRel);
