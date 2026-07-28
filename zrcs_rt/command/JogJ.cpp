/*
 * @Description: 关节运动相对位置指令
 */
#include "command/JogJ.h"

JogJ::JogJ()
{
    std::strcpy(nodeName_, "JogJ");
     // 设置 Ruckig 轨迹规划
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);

}


bool JogJ::applyOutput()
{
    controller_->axes_[axisId_]->setAxisPositionCmd(output_->new_position[0]);
    return true;
}

bool JogJ::initTrajectory()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(JogJArg::AxisId)]);
    position_ = command_->args[static_cast<size_t>(JogJArg::TargetPosition)];

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axes_.size()))
    {
        ERROR_PRINT("JogJ: 轴索%d 超出范围\n", axisId_);
        return false;
    }

   
    input_->current_position[0] = controller_->axes_[axisId_]->actualPos();
    input_->current_velocity[0] = 0;
    input_->current_acceleration[0] = 0;
    input_->target_position[0] = position_ + controller_->axes_[axisId_]->actualPos();
    input_->target_velocity[0] = 0;
    input_->target_acceleration[0] = 0;
    input_->max_velocity[0] = controller_->axes_[axisId_]->getMaxVelocity();
    input_->max_acceleration[0] = controller_->axes_[axisId_]->getMaxAcceleration();
    input_->max_jerk[0] = controller_->axes_[axisId_]->getMaxJerk();
    return true;
}

CMD_REGISTER(JogJ);
