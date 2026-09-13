/*
 * @Description: 多轴回零命令
 */
#include "command/Movehome.h"

Movehome::Movehome() : dof_(0) { std::strcpy(nodeName_, "Movehome"); }

bool Movehome::prepare()
{
    if (prepared_)
    {
        return true;
    }
    if (!controller_)
    {
        ERROR_PRINT("Movehome: controller is unavailable during prepare\n");
        return false;
    }

    dof_ = static_cast<int>(controller_->axes_.size());
    if (dof_ <= 0)
    {
        ERROR_PRINT("Movehome: 自由度无�?dof=%d)\n", dof_);
        return false;
    }

    for (int i = 0; i < dof_; ++i)
    {
        if (!controller_->axes_[static_cast<size_t>(i)])
        {
            ERROR_PRINT("Movehome: axis %d is unavailable during prepare\n", i);
            return false;
        }
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);
    prepared_ = true;
    return true;
}

bool Movehome::initTrajectory()
{
    if (!prepared_ || !otg_ || !input_ || !output_)
    {
        ERROR_PRINT("Movehome: prepared resources are unavailable\n");
        return false;
    }
    otg_->reset();

    for (int i = 0; i < dof_; i++)
    {
        input_->current_position[i] = controller_->axes_[i]->actualPos();
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = 0;
        input_->target_velocity[i] = 0;
        input_->target_acceleration[i] = 0;
        input_->max_velocity[i] = controller_->axes_[i]->getMaxVelocity();
        input_->max_acceleration[i] = controller_->axes_[i]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axes_[i]->getMaxJerk();
    }
    return true;
}

bool Movehome::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axes_[i]->setAxisPositionCmd(output_->new_position[i]);
    }
    return true;
}

REGISTERCMD(Movehome);
