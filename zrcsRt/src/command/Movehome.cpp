/*
 * @Description: 多轴回零命令
 */
#include "command/Movehome.h"

void Movehome::init()
{
    dof_ = static_cast<int>(controller_->axiss.size());
    if (dof_ <= 0)
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    for (int i = 0; i < dof_; i++)
    {
        input_->current_position[i] = controller_->axiss[i]->actualPos();
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = 0;
        input_->target_velocity[i] = 0;
        input_->target_acceleration[i] = 0;
        input_->max_velocity[i] = controller_->axiss[i]->getMaxVelocity();
        input_->max_acceleration[i] = controller_->axiss[i]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[i]->getMaxJerk();
    }
}

void Movehome::run(void)
{
    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[i]->setAxisPositionCmd(output_->new_position[i]);
        }
        output_->pass_to_input(*input_);
    }
    else if (result == Result::Finished)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[i]->setAxisPositionCmd(output_->new_position[i]);
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Movehome::exit(void) {}

REGISTERCMD(Movehome);
