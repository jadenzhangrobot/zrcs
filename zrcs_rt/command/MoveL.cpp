/*
 * @Description: Cartesian linear motion with a scalar arc-length trajectory.
 */
#include "command/MoveL.h"
#include "shared_memory/ShmLayout.h"

#include <algorithm>
#include <cmath>

MoveL::MoveL() : cartDist_(0.0)
{
    std::strcpy(nodeName_, "MoveL");
}

bool MoveL::prepare()
{
    if (prepared_)
    {
        return true;
    }
    if (!controller_ || !modelRegistry_)
    {
        ERROR_PRINT("MoveL: controller/model registry is unavailable during prepare\n");
        return false;
    }

    model_ = modelRegistry_->getModel(0);
    if (!model_)
    {
        ERROR_PRINT("MoveL: model id=0 not found during prepare\n");
        return false;
    }

    dof_ = model_->getDof();
    axisIds_ = model_->getAxisIds();
    if (dof_ <= 0 || axisIds_.size() != static_cast<size_t>(dof_))
    {
        ERROR_PRINT("MoveL: invalid model DOF or axis mapping\n");
        return false;
    }
    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[static_cast<size_t>(i)];
        if (axisId < 0 || axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            ERROR_PRINT("MoveL: axis id=%d is out of range during prepare\n", axisId);
            return false;
        }
    }

    currentJoint_.resize(dof_);
    targetJoint_.resize(dof_);
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
    prepared_ = true;
    return true;
}

bool MoveL::initTrajectory()
{
    if (!prepared_ || !command_ || !model_ || !otg_ || !input_ || !output_)
    {
        ERROR_PRINT("MoveL: command or prepared resources are unavailable\n");
        return false;
    }

    // Sync starts a new path group. Otherwise the scalar Ruckig state is
    // intentionally carried into the next segment.
    if (command_->args[static_cast<size_t>(MoveLArg::Sync)] == 1.0)
    {
        arcOffset_ = 0.0;
        input_->current_position[0] = 0.0;
        input_->current_velocity[0] = 0.0;
        input_->current_acceleration[0] = 0.0;
    }

    targetPos_ = Eigen::Vector3d(
        command_->args[static_cast<size_t>(MoveLArg::X)],
        command_->args[static_cast<size_t>(MoveLArg::Y)],
        command_->args[static_cast<size_t>(MoveLArg::Z)]);
    targetQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MoveLArg::Q1)],
        command_->args[static_cast<size_t>(MoveLArg::Q2)],
        command_->args[static_cast<size_t>(MoveLArg::Q3)],
        command_->args[static_cast<size_t>(MoveLArg::Q4)]);
    if (!targetPos_.allFinite() || !targetQuat_.coeffs().allFinite() ||
        targetQuat_.norm() < 1e-9)
    {
        ERROR_PRINT("MoveL: target pose contains invalid values or a zero quaternion\n");
        return false;
    }
    targetQuat_.normalize();

    for (int i = 0; i < dof_; ++i)
    {
        currentJoint_(i) = controller_->axes_[axisIds_[i]]->actualPos();
    }

    Eigen::Matrix4d startPose;
    if (!model_->forwardKinematics(currentJoint_, startPose))
    {
        ERROR_PRINT("MoveL: failed to calculate the start pose with FK\n");
        return false;
    }
    startPos_ = startPose.block<3, 1>(0, 3);
    startQuat_ = Eigen::Quaterniond(startPose.block<3, 3>(0, 0));
    startQuat_.normalize();

    cartDist_ = (targetPos_ - startPos_).norm();
    if (cartDist_ < 1e-6)
    {
        ERROR_PRINT("MoveL: segment length is zero\n");
        return false;
    }

    const double maxVel = command_->args[static_cast<size_t>(MoveLArg::Vel)];
    const double targetVel = command_->args[static_cast<size_t>(MoveLArg::TargetVel)];
    const double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    const double maxJerk = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);

    arcOffset_ += cartDist_;
    input_->target_position[0] = arcOffset_;
    input_->target_velocity[0] = targetVel;
    input_->max_velocity[0] = maxVel;
    input_->max_acceleration[0] = maxAccel;
    input_->max_jerk[0] = maxJerk;
    return true;
}

bool MoveL::applyOutput()
{
    const double s = output_->new_position[0];
    const double rawU = (s - (arcOffset_ - cartDist_)) / cartDist_;
    const double u = std::clamp(rawU, 0.0, 1.0);

    const Eigen::Vector3d pos = startPos_ + u * (targetPos_ - startPos_);
    const Eigen::Quaterniond orientation = startQuat_.slerp(u, targetQuat_);
    Eigen::Matrix4d targetPose = Eigen::Matrix4d::Identity();
    targetPose.block<3, 3>(0, 0) = orientation.toRotationMatrix();
    targetPose.block<3, 1>(0, 3) = pos;

    for (int i = 0; i < dof_; ++i)
    {
        currentJoint_(i) = controller_->axes_[axisIds_[i]]->actualPos();
    }

    constexpr double kHoldJointProgress = 1e-9;
    if (u <= kHoldJointProgress)
    {
        for (int i = 0; i < dof_; ++i)
        {
            controller_->axes_[axisIds_[i]]->setAxisPositionCmd(currentJoint_(i));
        }
        return true;
    }

    if (!model_->inverseKinematics(targetPose, currentJoint_, targetJoint_))
    {
        ERROR_PRINT("MoveL: IK failed at pos=(%.4f,%.4f,%.4f), u=%.4f\n",
                    pos.x(), pos.y(), pos.z(), u);
        return false;
    }

    for (int i = 0; i < dof_; ++i)
    {
        controller_->axes_[axisIds_[i]]->setAxisPositionCmd(targetJoint_(i));
    }
    return true;
}

REGISTERCMD(MoveL);
