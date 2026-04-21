/*
 * @Description: 绝对关节位置运动（ABB MoveAbsJ）— 多轴同步，不经IK
 */
#include "command/MoveabsJ.h"

MoveAbsJ::MoveAbsJ() : dof_(0) { std::strcpy(nodeName_, "MoveAbsJ"); }
Result MoveAbsJ::updateTrajectory() { return otg_->update(*input_, *output_); }
void MoveAbsJ::passOutputToInput() { output_->pass_to_input(*input_); }
void MoveAbsJ::applyDeltaTime(double dt) { otg_->delta_time = dt; }

bool MoveAbsJ::initTrajectory()
{
    auto* registry = modelRegistry_;
    if (!registry)
    {
        ERROR_PRINT("MoveAbsJ: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("MoveAbsJ: 未找到模型(id=0)\n");
        return false;
    }

    dof_ = static_cast<int>(command_->args[Count]);
    if (dof_ <= 0) dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    for (int i = 0; i < dof_; i++)
    {
        int axisId = axisIds_[i];
        input_->current_position[i] = controller_->axiss[axisId]->actualPos();
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = command_->args[J1 + i];
        input_->target_velocity[i] = 0;
        input_->target_acceleration[i] = 0;
        input_->max_velocity[i] = controller_->axiss[axisId]->getMaxVelocity();
        input_->max_acceleration[i] = controller_->axiss[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[axisId]->getMaxJerk();
    }
    return true;
}

void MoveAbsJ::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
}

CMD_REGISTER(MoveAbsJ);
