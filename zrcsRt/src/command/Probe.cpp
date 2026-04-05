/*
 * @Description: 单轴探针触发（ABB SearchL 单轴版 / 固高探针功能）
 */
#include "command/Probe.h"

Probe::Probe() : axisId_(0), ioIndex_(0), bitPos_(0)
{
    std::strcpy(nodeName_, "Probe");
}

Result Probe::updateTrajectory() { return otg_->update(input_, output_); }
void Probe::applyOutput() { controller_->axiss[axisId_]->setAxisPositionCmd(output_.new_position[0]); }
void Probe::passOutputToInput() { output_.pass_to_input(input_); }
void Probe::applyDeltaTime(double dt) { if (otg_) otg_->delta_time = dt; }

bool Probe::initTrajectory()
{
    shm().probeTriggered().store(false, std::memory_order_release);

    axisId_ = static_cast<int>(command_->args[ProbeAxisId]);
    double direction = command_->args[ProbeDirection];
    double vel = command_->args[ProbeVel];
    ioIndex_ = static_cast<int>(command_->args[ProbeIOIndex]);
    bitPos_ = static_cast<int>(command_->args[ProbeBitPos]);

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("Probe: 轴索引 %d 超出范围\n", axisId_);
        return false;
    }

    if (vel <= 0) vel = 0.1;
    double maxTravel = (direction >= 0) ? 1000.0 : -1000.0;

    otg_ = std::make_unique<Ruckig<1>>(cycletime * 0.001);
    input_.current_position[0] = controller_->axiss[axisId_]->actualPos();
    input_.current_velocity[0] = 0;
    input_.current_acceleration[0] = 0;
    input_.target_position[0] = input_.current_position[0] + maxTravel;
    input_.target_velocity[0] = 0;
    input_.target_acceleration[0] = 0;
    input_.max_velocity[0] = controller_->axiss[axisId_]->getMaxVelocity() * vel;
    input_.max_acceleration[0] = controller_->axiss[axisId_]->getMaxAcceleration();
    input_.max_jerk[0] = controller_->axiss[axisId_]->getMaxJerk();
    return true;
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

    updateOverride();

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
        ERROR_PRINT("Probe: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

REGISTERCMD(Probe);
