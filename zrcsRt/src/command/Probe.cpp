/*
 * @Description: 单轴探针触发（ABB SearchL 单轴版 / 固高探针功能）
 */
#include "command/Probe.h"

void Probe::init()
{
    shm().probeTriggered().store(false, std::memory_order_release);

    axisId_ = static_cast<int>(command_->args[ProbeAxisId]);
    double direction = command_->args[ProbeDirection];
    double vel = command_->args[ProbeVel];
    ioIndex_ = static_cast<int>(command_->args[ProbeIOIndex]);
    bitPos_ = static_cast<int>(command_->args[ProbeBitPos]);

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    if (vel <= 0) vel = 0.1;
    double maxTravel = (direction >= 0) ? 1000.0 : -1000.0;

    double override = shm().overrideRatio().load(std::memory_order_acquire);

    otg_ = std::make_unique<Ruckig<1>>(cycletime * 0.001);
    input_.current_position[0] = controller_->axiss[axisId_]->actualPos();
    input_.current_velocity[0] = 0;
    input_.current_acceleration[0] = 0;
    input_.target_position[0] = input_.current_position[0] + maxTravel;
    input_.target_velocity[0] = 0;
    input_.target_acceleration[0] = 0;
    input_.max_velocity[0] = controller_->axiss[axisId_]->getMaxVelocity() * override * vel;
    input_.max_acceleration[0] = controller_->axiss[axisId_]->getMaxAcceleration();
    input_.max_jerk[0] = controller_->axiss[axisId_]->getMaxJerk();
}

void Probe::run(void)
{
    // check IO trigger
    if (ioIndex_ >= 0 &&
        ioIndex_ < static_cast<int>(controller_->ios_.size()))
    {
        if (controller_->ios_[ioIndex_]->ioRead32(ioIndex_, bitPos_))
        {
            double pos = controller_->axiss[axisId_]->actualPos();
            shm().probeResult()[0] = pos;
            shm().probeTriggered().store(true, std::memory_order_release);

            controller_->axiss[axisId_]->setAxisPositionCmd(pos);
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
            return;
        }
    }

    auto result = otg_->update(input_, output_);
    if (result == Result::Working)
    {
        controller_->axiss[axisId_]->setAxisPositionCmd(output_.new_position[0]);
        output_.pass_to_input(input_);
    }
    else if (result == Result::Finished)
    {
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void Probe::exit(void) {}

REGISTERCMD(Probe);
