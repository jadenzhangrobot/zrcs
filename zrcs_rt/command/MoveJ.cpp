/*
 * @Description: Cartesian target joint-space PTP motion (ABB MoveJ style).
 */
#include "command/MoveJ.h"

#include <cmath>

MoveJ::MoveJ()
{
    std::strcpy(nodeName_, "MoveJ");
}

bool MoveJ::prepare()
{
    if (prepared_)
    {
        return true;
    }
    if (!controller_ || !modelRegistry_)
    {
        ERROR_PRINT("MoveJ: controller/model registry is unavailable during prepare\n");
        return false;
    }

    model_ = modelRegistry_->getModel(0);
    if (!model_)
    {
        ERROR_PRINT("MoveJ: model id=0 not found during prepare\n");
        return false;
    }

    dof_ = model_->getDof();
    axisIds_ = model_->getAxisIds();
    if (dof_ <= 0 || axisIds_.size() != static_cast<size_t>(dof_))
    {
        ERROR_PRINT("MoveJ: invalid model DOF or axis mapping\n");
        return false;
    }
    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[static_cast<size_t>(i)];
        if (axisId < 0 || axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            ERROR_PRINT("MoveJ: axis id=%d is out of range during prepare\n", axisId);
            return false;
        }
    }

    currentJoint_.resize(dof_);
    targetJoint_.resize(dof_);
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);
    prepared_ = true;
    return true;
}

bool MoveJ::initTrajectory()
{
    if (!prepared_ || !command_ || !model_ || !otg_ || !input_ || !output_)
    {
        ERROR_PRINT("MoveJ: command or prepared resources are unavailable\n");
        return false;
    }
    otg_->reset();

    const Eigen::Matrix4d targetPose = RobotModel::poseFromXYZRPY(
        command_->args[static_cast<size_t>(MoveJArg::X)],
        command_->args[static_cast<size_t>(MoveJArg::Y)],
        command_->args[static_cast<size_t>(MoveJArg::Z)],
        command_->args[static_cast<size_t>(MoveJArg::RX)],
        command_->args[static_cast<size_t>(MoveJArg::RY)],
        command_->args[static_cast<size_t>(MoveJArg::RZ)]);

    for (int i = 0; i < dof_; ++i)
    {
        currentJoint_(i) = controller_->axes_[axisIds_[i]]->actualPos();
    }

    if (!model_->inverseKinematics(targetPose, currentJoint_, targetJoint_))
    {
        ERROR_PRINT("MoveJ: IK failed for target=(%.4f,%.4f,%.4f; %.4f,%.4f,%.4f)\n",
                    targetPose(0, 3), targetPose(1, 3), targetPose(2, 3),
                    command_->args[static_cast<size_t>(MoveJArg::RX)],
                    command_->args[static_cast<size_t>(MoveJArg::RY)],
                    command_->args[static_cast<size_t>(MoveJArg::RZ)]);
        return false;
    }

    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[i];
        const double lower = controller_->axes_[axisId]->getNegativeLimit();
        const double upper = controller_->axes_[axisId]->getPositiveLimit();
        if (!std::isfinite(targetJoint_(i)) || targetJoint_(i) < lower - 1e-8 ||
            targetJoint_(i) > upper + 1e-8)
        {
            ERROR_PRINT("MoveJ: axis%d IK target %.6f exceeds [%.6f, %.6f]\n",
                        axisId, targetJoint_(i), lower, upper);
            return false;
        }
    }

    double velScale = command_->args[static_cast<size_t>(MoveJArg::Vel)];
    if (velScale <= 0.0)
    {
        velScale = 1.0;
    }

    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[i];
        input_->current_position[i] = currentJoint_(i);
        input_->current_velocity[i] = 0.0;
        input_->current_acceleration[i] = 0.0;
        input_->target_position[i] = targetJoint_(i);
        input_->target_velocity[i] = 0.0;
        input_->target_acceleration[i] = 0.0;
        input_->max_velocity[i] = controller_->axes_[axisId]->getMaxVelocity() * velScale;
        input_->max_acceleration[i] = controller_->axes_[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axes_[axisId]->getMaxJerk();
    }
    return true;
}

bool MoveJ::applyOutput()
{
    for (int i = 0; i < dof_; ++i)
    {
        controller_->axes_[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
    return true;
}

REGISTERCMD(MoveJ);
