/*
 * @Description: Three-point circular motion with per-cycle inverse kinematics.
 */
#include "command/MoveC.h"

MoveC::MoveC() : radius_(0.0), totalAngle_(0.0), zStart_(0.0), zEnd_(0.0)
{
    std::strcpy(nodeName_, "MoveC");
}

bool MoveC::prepare()
{
    if (prepared_)
    {
        return true;
    }
    if (!controller_ || !modelRegistry_)
    {
        ERROR_PRINT("MoveC: controller/model registry is unavailable during prepare\n");
        return false;
    }

    model_ = modelRegistry_->getModel(0);
    if (!model_)
    {
        ERROR_PRINT("MoveC: model id=0 not found during prepare\n");
        return false;
    }

    dof_ = model_->getDof();
    axisIds_ = model_->getAxisIds();
    if (dof_ <= 0 || axisIds_.size() != static_cast<size_t>(dof_))
    {
        ERROR_PRINT("MoveC: invalid model DOF or axis mapping\n");
        return false;
    }
    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[static_cast<size_t>(i)];
        if (axisId < 0 || axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            ERROR_PRINT("MoveC: axis id=%d is out of range during prepare\n", axisId);
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

bool MoveC::applyOutput()
{
    return true;
}

bool MoveC::initTrajectory()
{
    if (!prepared_ || !command_ || !model_ || !otg_ || !input_ || !output_)
    {
        ERROR_PRINT("MoveC: command or prepared resources are unavailable\n");
        return false;
    }
    otg_->reset();

    for (int i = 0; i < dof_; ++i)
    {
        currentJoint_(i) = controller_->axes_[axisIds_[i]]->actualPos();
    }

    Eigen::Matrix4d startPose;
    if (!model_->forwardKinematics(currentJoint_, startPose))
    {
        ERROR_PRINT("MoveC: failed to calculate the start pose with FK\n");
        return false;
    }

    const Eigen::Vector3d p0 = startPose.block<3, 1>(0, 3);
    const Eigen::Vector3d p1(
        command_->args[static_cast<size_t>(MoveCArg::ViaX)],
        command_->args[static_cast<size_t>(MoveCArg::ViaY)],
        command_->args[static_cast<size_t>(MoveCArg::ViaZ)]);
    const Eigen::Vector3d p2(
        command_->args[static_cast<size_t>(MoveCArg::EndX)],
        command_->args[static_cast<size_t>(MoveCArg::EndY)],
        command_->args[static_cast<size_t>(MoveCArg::EndZ)]);

    const Eigen::Vector3d a = p1 - p0;
    const Eigen::Vector3d b = p2 - p0;
    const Eigen::Vector3d normal = a.cross(b);
    if (normal.squaredNorm() < 1e-12)
    {
        ERROR_PRINT("MoveC: the three points are collinear\n");
        return false;
    }

    const Eigen::Vector3d d = p1 - p0;
    const Eigen::Vector3d e = p2 - p0;
    const double dd = d.dot(d);
    const double ee = e.dot(e);
    const double de = d.dot(e);
    const double denominator = 2.0 * (dd * ee - de * de);
    if (std::abs(denominator) < 1e-12)
    {
        ERROR_PRINT("MoveC: the three points are collinear\n");
        return false;
    }

    const double centerD = (dd * ee - ee * de) / denominator;
    const double centerE = (ee * dd - dd * de) / denominator;
    center_ = p0 + centerD * d + centerE * e;
    radius_ = (p0 - center_).norm();
    axis_ = normal.normalized();
    startRadial_ = (p0 - center_).normalized();
    biNormal_ = axis_.cross(startRadial_);

    const Eigen::Vector3d r1 = p1 - center_;
    const Eigen::Vector3d r2 = p2 - center_;
    double angle1 = std::atan2(r1.dot(biNormal_), r1.dot(startRadial_));
    double angle2 = std::atan2(r2.dot(biNormal_), r2.dot(startRadial_));
    if (angle1 < 0.0)
    {
        angle1 += 2.0 * M_PI;
    }
    if (angle2 < 0.0)
    {
        angle2 += 2.0 * M_PI;
    }
    if (angle2 < angle1)
    {
        angle2 += 2.0 * M_PI;
    }
    totalAngle_ = angle2;

    startPos_ = p0;
    zStart_ = p0.z();
    zEnd_ = p2.z();

    double velScale = command_->args[static_cast<size_t>(MoveCArg::Vel)];
    if (velScale <= 0.0)
    {
        velScale = 1.0;
    }
    const double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    const double maxJerk = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);

    input_->current_position[0] = 0.0;
    input_->current_velocity[0] = 0.0;
    input_->current_acceleration[0] = 0.0;
    input_->target_position[0] = totalAngle_;
    input_->target_velocity[0] = 0.0;
    input_->target_acceleration[0] = 0.0;
    input_->max_velocity[0] = 2.0 * velScale;
    input_->max_acceleration[0] = maxAccel > 0.0 ? maxAccel : 4.0;
    input_->max_jerk[0] = maxJerk > 0.0 ? maxJerk : 20.0;
    return true;
}

zrcsSystem::RunResult MoveC::run()
{
    updateOverride();
    const auto result = otg_->update(*input_, *output_);
    if (result != Result::Working && result != Result::Finished)
    {
        ERROR_PRINT("MoveC: trajectory planning failed, result=%d\n",
                    static_cast<int>(result));
        return zrcsSystem::RunResult::FAILED;
    }

    const double theta = output_->new_position[0];
    const Eigen::Vector3d pos = center_ + radius_ *
        (std::cos(theta) * startRadial_ + std::sin(theta) * biNormal_);

    for (int i = 0; i < dof_; ++i)
    {
        currentJoint_(i) = controller_->axes_[axisIds_[i]]->actualPos();
    }

    Eigen::Matrix4d targetPose;
    if (!model_->forwardKinematics(currentJoint_, targetPose))
    {
        ERROR_PRINT("MoveC: FK failed while preserving orientation\n");
        return zrcsSystem::RunResult::FAILED;
    }
    targetPose.block<3, 1>(0, 3) = pos;

    if (!model_->inverseKinematics(targetPose, currentJoint_, targetJoint_))
    {
        ERROR_PRINT("MoveC: IK failed\n");
        return zrcsSystem::RunResult::FAILED;
    }
    for (int i = 0; i < dof_; ++i)
    {
        controller_->axes_[axisIds_[i]]->setAxisPositionCmd(targetJoint_(i));
    }

    if (result == Result::Finished)
    {
        return zrcsSystem::RunResult::SUCCESS;
    }
    output_->pass_to_input(*input_);
    return zrcsSystem::RunResult::EXECUTING;
}

REGISTERCMD(MoveC);
