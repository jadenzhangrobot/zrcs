/*
 * @Description: 关节运动绝对位置指令
 */
#include "command/JogabsJ.h"

JogabsJ::JogabsJ() : otg_(cycletime * 0.001)
{
    std::strcpy(nodeName_, "JogabsJ");
}

Result JogabsJ::updateTrajectory() { return otg_.update(input_, output_); }
void JogabsJ::applyOutput() { controller_->axiss[axisId_]->setAxisPositionCmd(output_.new_position[0]); }
void JogabsJ::passOutputToInput() { output_.pass_to_input(input_); }
void JogabsJ::applyDeltaTime(double dt) { otg_.delta_time = dt; }

bool JogabsJ::initTrajectory()
{
    axisId_ = static_cast<int>(command_->args[JogabsJAxisId]);
    position_ = command_->args[JogabsJTargetPosition];

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("JogabsJ: 轴索引 %d 超出范围\n", axisId_);
        return false;
    }

    input_.current_position[0] = controller_->axiss[axisId_]->actualPos();
    input_.current_velocity[0] = 0;
    input_.current_acceleration[0] = 0;
    input_.target_position[0] = position_;
    input_.target_velocity[0] = 0;
    input_.target_acceleration[0] = 0;
    input_.max_velocity[0] = controller_->axiss[axisId_]->getMaxVelocity();
    input_.max_acceleration[0] = controller_->axiss[axisId_]->getMaxAcceleration();
    input_.max_jerk[0] = controller_->axiss[axisId_]->getMaxJerk();
    return true;
}

REGISTERCMD(JogabsJ, 19);
