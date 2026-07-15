#include "command/MovePath.h"
#include "shared_memory/ShmLayout.h"

#include <algorithm>
#include <cmath>
#include <cstring>

MovePath::MovePath()
{
    std::strcpy(nodeName_, "MovePath");
    axisIds_.reserve(zrcs::kAxisMax);
    if (!otg_)
    {
        otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
        input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
        output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
    }
}

Eigen::Vector3d MovePath::evaluateArc(double u) const
{
    const double theta = arcSweep_ * u;
    return arcCenter_ + arcRadius_ * (std::cos(theta) * arcU_ + std::sin(theta) * arcV_);
}

Eigen::Vector3d MovePath::evaluateArcDerivative(double u) const
{
    const double theta = arcSweep_ * u;
    return arcRadius_ * arcSweep_ * (-std::sin(theta) * arcU_ + std::cos(theta) * arcV_);
}

bool MovePath::initTrajectory()
{
    if (!modelInited_)
    {
        auto* registry = modelRegistry_;
        if (!registry)
        {
            ERROR_PRINT("MovePath: model registry is not initialized\n");
            return false;
        }
        RobotModel* model = registry->getModel(0);
        if (!model)
        {
            ERROR_PRINT("MovePath: model id=0 not found\n");
            return false;
        }
        axisIds_ = model->getAxisIds();
        if (axisIds_.size() < 3)
        {
            ERROR_PRINT("MovePath: model must expose at least 3 axes\n");
            return false;
        }
        modelInited_ = true;
    }

    if (command_->args[static_cast<size_t>(MovePathArg::Sync)] == 1.0)
    {
        arcOffset_ = 0.0;
        input_->current_position[0] = 0.0;
        input_->current_velocity[0] = 0.0;
        input_->current_acceleration[0] = 0.0;
    }

    const int shape = static_cast<int>(command_->args[static_cast<size_t>(MovePathArg::Shape)]);
    isArc_ = shape == 1;

    if (isArc_)
    {
        arcCenter_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P0X)],
            command_->args[static_cast<size_t>(MovePathArg::P0Y)],
            command_->args[static_cast<size_t>(MovePathArg::P0Z)]);
        arcU_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P1X)],
            command_->args[static_cast<size_t>(MovePathArg::P1Y)],
            command_->args[static_cast<size_t>(MovePathArg::P1Z)]);
        arcV_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P2X)],
            command_->args[static_cast<size_t>(MovePathArg::P2Y)],
            command_->args[static_cast<size_t>(MovePathArg::P2Z)]);
        arcRadius_ = command_->args[static_cast<size_t>(MovePathArg::Radius)];
        arcSweep_ = command_->args[static_cast<size_t>(MovePathArg::Sweep)];

        const double uNorm = arcU_.norm();
        const double vNorm = arcV_.norm();
        if (!std::isfinite(arcRadius_) || !std::isfinite(arcSweep_) ||
            arcRadius_ <= 1e-9 || std::abs(arcSweep_) <= 1e-9 ||
            uNorm <= 1e-9 || vNorm <= 1e-9)
        {
            ERROR_PRINT("MovePath: circular arc args are invalid\n");
            return false;
        }
        arcU_ /= uNorm;
        arcV_ /= vNorm;

        startPos_ = evaluateArc(0.0);
        targetPos_ = evaluateArc(1.0);
        geometryLength_ = std::abs(arcRadius_ * arcSweep_);
    }
    else
    {
        // Shape=0：直线；其它 shape 当前协议不支持，按直线处理
        startPos_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P0X)],
            command_->args[static_cast<size_t>(MovePathArg::P0Y)],
            command_->args[static_cast<size_t>(MovePathArg::P0Z)]);
        targetPos_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P1X)],
            command_->args[static_cast<size_t>(MovePathArg::P1Y)],
            command_->args[static_cast<size_t>(MovePathArg::P1Z)]);
        geometryLength_ = (targetPos_ - startPos_).norm();
    }

    const double commandLength = command_->args[static_cast<size_t>(MovePathArg::Length)];
    if (!std::isfinite(commandLength) || commandLength < 1e-6)
    {
        ERROR_PRINT("MovePath: path length is zero or invalid\n");
        return false;
    }
    if (!std::isfinite(geometryLength_) || geometryLength_ < 1e-6)
    {
        ERROR_PRINT("MovePath: geometry length is zero or invalid\n");
        return false;
    }

    const double maxVel = command_->args[static_cast<size_t>(MovePathArg::Vel)];
    const double targetVel = command_->args[static_cast<size_t>(MovePathArg::TargetVel)];
    const double targetAcc = command_->args[static_cast<size_t>(MovePathArg::TargetAcc)];
    const double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    const double maxJerk = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);

    if (!std::isfinite(maxVel) || maxVel <= 0.0 ||
        !std::isfinite(targetVel) || targetVel < 0.0 ||
        !std::isfinite(maxAccel) || maxAccel <= 0.0 ||
        !std::isfinite(targetAcc) || std::abs(targetAcc) > maxAccel ||
        !std::isfinite(maxJerk) || maxJerk <= 0.0)
    {
        ERROR_PRINT("MovePath: velocity, acceleration or jerk limit is invalid\n");
        return false;
    }

    arcOffset_ = arcOffset_ + geometryLength_;
    input_->target_position[0] = arcOffset_;
    input_->target_velocity[0] = targetVel;
    input_->target_acceleration[0] = targetAcc;
    input_->max_velocity[0] = maxVel;
    input_->min_velocity = std::vector<double>{0.0};
    input_->max_acceleration[0] = maxAccel;
    input_->max_jerk[0] = maxJerk;

    return true;
}

void MovePath::applyOutput()
{
    // otg_->update() 已由 TrajectoryCmd::run() -> updateTrajectory() 完成
    const double s = output_->new_position[0];
    const double geometryS = s - (arcOffset_ - geometryLength_);
    const double u = geometryLength_ > 1e-13
                         ? geometryS / geometryLength_
                         : 0.0;

    Eigen::Vector3d pos;
    Eigen::Vector3d pathDir = Eigen::Vector3d::Zero();
    if (isArc_)
    {
        pos = evaluateArc(u);
        const Eigen::Vector3d du = evaluateArcDerivative(u);
        const double duNorm = du.norm();
        if (duNorm > 1e-12)
        {
            pathDir = du / duNorm;
        }
    }
    else
    {
        pos = startPos_ + u * (targetPos_ - startPos_);
        if (geometryLength_ > 1e-12)
        {
            pathDir = (targetPos_ - startPos_) / geometryLength_;
        }
    }

    const double pathVelocity = output_->new_velocity[0];

    controller_->axes_[axisIds_[0]]->setAxisPositionCmd(pos.x());
    controller_->axes_[axisIds_[1]]->setAxisPositionCmd(pos.y());
    controller_->axes_[axisIds_[2]]->setAxisPositionCmd(pos.z());
    controller_->axes_[axisIds_[0]]->setAxisVelocityCmd(pathVelocity * pathDir.x());
    controller_->axes_[axisIds_[1]]->setAxisVelocityCmd(pathVelocity * pathDir.y());
    controller_->axes_[axisIds_[2]]->setAxisVelocityCmd(pathVelocity * pathDir.z());
    controller_->axes_[axisIds_[3]]->setAxisPositionCmd(output_->new_velocity[0]);
}

CMD_REGISTER(MovePath);
