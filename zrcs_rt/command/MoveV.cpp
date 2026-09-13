#include "command/MoveV.h"

#include "config/CmdDefine.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"

#include <cmath>
#include <cstring>

MoveV::MoveV()
{
    std::strcpy(nodeName_, "MoveV");
}

bool MoveV::prepare()
{
    if (prepared_)
        return true;

    if (!controller_ || !modelRegistry_)
    {
        ERROR_PRINT("MoveV: controller/model registry is unavailable during prepare\n");
        return false;
    }

    RobotModel* model = modelRegistry_->getModel(0);
    if (!model)
    {
        ERROR_PRINT("MoveV: model id=0 was not found\n");
        return false;
    }

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();
    if (dof_ <= 0 || dof_ > static_cast<int>(zrcs::kCmdArgsMax -
                                             static_cast<size_t>(MoveVArg::V1)) ||
        axisIds_.size() != static_cast<size_t>(dof_))
    {
        ERROR_PRINT("MoveV: invalid model DOF=%d or axis mapping size=%zu\n",
                    dof_, axisIds_.size());
        return false;
    }

    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[static_cast<size_t>(i)];
        if (axisId < 0 || axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            ERROR_PRINT("MoveV: axis id=%d is out of range during prepare\n", axisId);
            return false;
        }
    }

    otg_ = std::make_unique<ruckig::Ruckig<ruckig::DynamicDOFs>>(
        dof_, cycletime * 0.001);
    input_ = std::make_unique<ruckig::InputParameter<ruckig::DynamicDOFs>>(dof_);
    output_ = std::make_unique<ruckig::OutputParameter<ruckig::DynamicDOFs>>(dof_);
    input_->control_interface = ruckig::ControlInterface::Velocity;
    input_->synchronization = ruckig::Synchronization::None;
    prepared_ = true;
    return true;
}

bool MoveV::initTrajectory()
{
    if (!prepared_ || !command_ || !controller_ || !otg_ || !input_ || !output_)
    {
        ERROR_PRINT("MoveV: command or prepared resources are unavailable\n");
        return false;
    }

    const double countValue = command_->args[static_cast<size_t>(MoveVArg::Count)];
    if (!std::isfinite(countValue) || countValue < 0.0 ||
        std::floor(countValue) != countValue)
    {
        ERROR_PRINT("MoveV: Count must be a non-negative integer, got %.6f\n",
                    countValue);
        return false;
    }
    if (countValue > static_cast<double>(dof_))
    {
        ERROR_PRINT("MoveV: Count=%.0f exceeds model DOF=%d\n",
                    countValue, dof_);
        return false;
    }

    // Count=0 is the default form used by existing typed command nodes.
    const int requestedDof = countValue == 0.0 ? dof_ : static_cast<int>(countValue);
    if (requestedDof != dof_)
    {
        ERROR_PRINT("MoveV: Count=%d does not match model DOF=%d\n",
                    requestedDof, dof_);
        return false;
    }

    // Force a fresh calculation from this session's sampled axis state.
    otg_->reset();

    zeroTarget_ = true;
    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[static_cast<size_t>(i)];
        if (axisId < 0 || axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            ERROR_PRINT("MoveV: axis id=%d is out of range\n", axisId);
            return false;
        }

        auto* axis = controller_->axes_[axisId].get();
        const double targetVelocity = command_->args[
            static_cast<size_t>(MoveVArg::V1) + static_cast<size_t>(i)];
        const double maxVelocity = axis->getMaxVelocity();
        const double maxAcceleration = axis->getMaxAcceleration();
        const double maxJerk = axis->getMaxJerk();
        const double position = axis->actualPos();
        const double velocity = axis->actualVel();
        const double acceleration = axis->actualAcc();

        if (!std::isfinite(targetVelocity) || !std::isfinite(maxVelocity) ||
            !std::isfinite(maxAcceleration) || !std::isfinite(maxJerk) ||
            !std::isfinite(position) || !std::isfinite(velocity) ||
            !std::isfinite(acceleration))
        {
            ERROR_PRINT("MoveV: non-finite input on axis id=%d\n", axisId);
            return false;
        }
        if (maxVelocity < 0.0 || maxAcceleration <= 0.0 || maxJerk <= 0.0)
        {
            ERROR_PRINT("MoveV: invalid limits on axis id=%d (v=%.6f a=%.6f j=%.6f)\n",
                        axisId, maxVelocity, maxAcceleration, maxJerk);
            return false;
        }
        if (std::abs(targetVelocity) > maxVelocity)
        {
            ERROR_PRINT("MoveV: target velocity %.6f exceeds axis %d limit %.6f\n",
                        targetVelocity, axisId, maxVelocity);
            return false;
        }

        input_->current_position[static_cast<size_t>(i)] = position;
        input_->current_velocity[static_cast<size_t>(i)] = velocity;
        input_->current_acceleration[static_cast<size_t>(i)] = acceleration;
        // Position is ignored by the velocity interface, but initialise it to
        // a meaningful finite value for diagnostics and input validation.
        input_->target_position[static_cast<size_t>(i)] = position;
        input_->target_velocity[static_cast<size_t>(i)] = targetVelocity;
        input_->target_acceleration[static_cast<size_t>(i)] = 0.0;
        input_->max_velocity[static_cast<size_t>(i)] = maxVelocity;
        input_->max_acceleration[static_cast<size_t>(i)] = maxAcceleration;
        input_->max_jerk[static_cast<size_t>(i)] = maxJerk;

        if (targetVelocity != 0.0)
            zeroTarget_ = false;
    }

    return true;
}

bool MoveV::applyOutput()
{
    if (!output_ || output_->new_position.size() != static_cast<size_t>(dof_))
        return false;

    for (int i = 0; i < dof_; ++i)
    {
        const double position = output_->new_position[static_cast<size_t>(i)];
        const int axisId = axisIds_[static_cast<size_t>(i)];
        if (!std::isfinite(position) || axisId < 0 ||
            axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            return false;
        }
        // Axis::cmdsProcessing derives the velocity from this position
        // command, so writing position is required for the RT control path.
        controller_->axes_[axisId]->setAxisPositionCmd(position);
    }
    return true;
}

zrcsSystem::RunResult MoveV::run()
{
    updateOverride();

    // 本工程 Ruckig 以 throw_error=false 实例化，非法输入返回负值 Result，
    // 由下方 result < 0 处理，不在 RT 路径使用异常。
    const ruckig::Result result = updateTrajectory();

    if (result < 0)
    {
        ERROR_PRINT("MoveV: velocity trajectory failed, result=%d\n",
                    static_cast<int>(result));
        return zrcsSystem::RunResult::FAILED;
    }
    if (!applyOutput())
    {
        ERROR_PRINT("MoveV: failed to apply trajectory output\n");
        return zrcsSystem::RunResult::FAILED;
    }

    passOutputToInput();

    // A non-zero velocity is a continuous command. It deliberately remains
    // active even when Ruckig reports Finished for an already-matched speed.
    if (zeroTarget_ && result == ruckig::Result::Finished)
        return zrcsSystem::RunResult::SUCCESS;
    return zrcsSystem::RunResult::EXECUTING;
}

REGISTERCMD(MoveV);
