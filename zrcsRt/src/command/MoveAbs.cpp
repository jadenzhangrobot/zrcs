/*
 * @Description: 通用单轴绝对定位（带可选速度/加速度/加加速度参数）
 */
#include "command/MoveAbs.h"

bool MoveAbs::initTrajectory()
{
    axisId_ = static_cast<int>(command_->args[MoveAbsAxisId]);
    double position = command_->args[MoveAbsPosition];
    double vel = command_->args[MoveAbsVel];
    double acc = command_->args[MoveAbsAcc];
    double jerk = command_->args[MoveAbsJerk];

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("MoveAbs: 轴索引 %d 超出范围\n", axisId_);
        return false;
    }

    input_.current_position[0] = controller_->axiss[axisId_]->actualPos();
    input_.current_velocity[0] = 0;
    input_.current_acceleration[0] = 0;
    input_.target_position[0] = position;
    input_.target_velocity[0] = 0;
    input_.target_acceleration[0] = 0;

    // 倍率不再缩放 max_velocity，由 delta_time 时间缩放统一处理
    input_.max_velocity[0] = vel > 0 ? vel : controller_->axiss[axisId_]->getMaxVelocity();
    input_.max_acceleration[0] = acc > 0 ? acc : controller_->axiss[axisId_]->getMaxAcceleration();
    input_.max_jerk[0] = jerk > 0 ? jerk : controller_->axiss[axisId_]->getMaxJerk();
    return true;
}

REGISTERCMD(MoveAbs);
