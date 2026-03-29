/*
 * @Description: 绝对关节位置运动（ABB MoveAbsJ）— 多轴同步，不经IK
 */
#include "command/MoveabsJ.h"

void MoveAbsJ::init()
{
    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    dof_ = static_cast<int>(command_->args[MoveAbsJCount]);
    if (dof_ <= 0) dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double override = shm().overrideRatio().load(std::memory_order_acquire);

    for (int i = 0; i < dof_; i++)
    {
        int axisId = axisIds_[i];
        input_->current_position[i] = controller_->axiss[axisId]->actualPos();
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = command_->args[MoveAbsJJ1 + i];
        input_->target_velocity[i] = 0;
        input_->target_acceleration[i] = 0;
        input_->max_velocity[i] = controller_->axiss[axisId]->getMaxVelocity() * override;
        input_->max_acceleration[i] = controller_->axiss[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[axisId]->getMaxJerk();
    }
}

void MoveAbsJ::run(void)
{
    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
        }
        output_->pass_to_input(*input_);
    }
    else if (result == Result::Finished)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void MoveAbsJ::exit(void) {}

REGISTERCMD(MoveAbsJ);
