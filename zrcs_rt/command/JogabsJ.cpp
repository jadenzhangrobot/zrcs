/*
 * @Description: 关节运动绝对位置指令
 */
#include "command/JogabsJ.h"

JogabsJ::JogabsJ()
{
    std::strcpy(nodeName_, "JogabsJ");
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
}

void JogabsJ::applyOutput() 
{ 
    controller_->axes_[axisId_]->setAxisPositionCmd(output_->new_position[0]); 
}

bool JogabsJ::initTrajectory()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(JogabsJArg::AxisId)]);
    position_ = command_->args[static_cast<size_t>(JogabsJArg::TargetPosition)];

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axes_.size()))
    {
        ERROR_PRINT("JogabsJ: 轴索 ?%d 超出范围\n", axisId_);
        return false;
    }

input_->current_position[0] = controller_->axes_[axisId_]->actualPos();
    input_->current_velocity[0] = 0;
    input_->current_acceleration[0] = 0;
    input_->target_position[0] = position_;
    input_->target_velocity[0] = 0;
    input_->target_acceleration[0] = 0;
    input_->max_velocity[0] = controller_->axes_[axisId_]->getMaxVelocity();
    input_->max_acceleration[0] = controller_->axes_[axisId_]->getMaxAcceleration();
    input_->max_jerk[0] = controller_->axes_[axisId_]->getMaxJerk();
    return true;
}

CMD_REGISTER(JogabsJ);
