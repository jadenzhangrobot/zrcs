/*
 * @Description: 通用单轴相对定位（带可选速度/加速度/加加速度参数）
 */
#include "command/MoveRel.h"

void MoveRel::init()
{
    axisId_ = static_cast<int>(command_->args[MoveRelAxisId]);
    double distance = command_->args[MoveRelDistance];
    double vel = command_->args[MoveRelVel];
    double acc = command_->args[MoveRelAcc];
    double jerk = command_->args[MoveRelJerk];

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("MoveRel: 轴索引 %d 超出范围\n", axisId_);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    double currentPos = controller_->axiss[axisId_]->actualPos();
    input_.current_position[0] = currentPos;
    input_.current_velocity[0] = 0;
    input_.current_acceleration[0] = 0;
    input_.target_position[0] = currentPos + distance;
    input_.target_velocity[0] = 0;
    input_.target_acceleration[0] = 0;

    double override = shm().overrideRatio().load(std::memory_order_acquire);
    input_.max_velocity[0] = (vel > 0 ? vel : controller_->axiss[axisId_]->getMaxVelocity()) * override;
    input_.max_acceleration[0] = acc > 0 ? acc : controller_->axiss[axisId_]->getMaxAcceleration();
    input_.max_jerk[0] = jerk > 0 ? jerk : controller_->axiss[axisId_]->getMaxJerk();
}

void MoveRel::run(void)
{
    auto result = otg_.update(input_, output_);
    if (result == Result::Working)
    {
        controller_->axiss[axisId_]->setAxisPositionCmd(output_.new_position[0]);
        output_.pass_to_input(input_);
    }
    else if (result == Result::Finished)
    {
        controller_->axiss[axisId_]->setAxisPositionCmd(output_.new_position[0]);
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("MoveRel: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void MoveRel::exit(void) {}

REGISTERCMD(MoveRel);
