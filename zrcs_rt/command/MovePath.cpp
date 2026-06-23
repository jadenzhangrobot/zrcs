#include "command/MovePath.h"
#include "shared_memory/ShmLayout.h"

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

Eigen::Vector3d MovePath::evaluate(double u) const
{
    if (isArc_)
    {
        const double theta = arcSweep_ * u;
        return arcCenter_ + arcRadius_ * (std::cos(theta) * arcU_ + std::sin(theta) * arcV_);
    }

    const double u2 = u * u;
    const double u3 = u2 * u;

    return {
        coeff_[0][0] + coeff_[0][1] * u + coeff_[0][2] * u2 + coeff_[0][3] * u3,
        coeff_[1][0] + coeff_[1][1] * u + coeff_[1][2] * u2 + coeff_[1][3] * u3,
        coeff_[2][0] + coeff_[2][1] * u + coeff_[2][2] * u2 + coeff_[2][3] * u3,
    };
}

Eigen::Vector3d MovePath::evaluateDerivative(double u) const
{
    if (isArc_)
    {
        const double theta = arcSweep_ * u;
        return arcRadius_ * arcSweep_ * (-std::sin(theta) * arcU_ + std::cos(theta) * arcV_);
    }

    const double u2 = u * u;

    return {
        coeff_[0][1] + 2.0 * coeff_[0][2] * u + 3.0 * coeff_[0][3] * u2,
        coeff_[1][1] + 2.0 * coeff_[1][2] * u + 3.0 * coeff_[1][3] * u2,
        coeff_[2][1] + 2.0 * coeff_[2][2] * u + 3.0 * coeff_[2][3] * u2,
    };
}

double MovePath::integrateSpeed(double u) const
{
    u = std::clamp(u, 0.0, 1.0);
    if (u <= 0.0)
    {
        return 0.0;
    }

    static constexpr std::array<double, 8> nodes = {
        -0.9602898564975363, -0.7966664774136267,
        -0.5255324099163290, -0.1834346424956498,
         0.1834346424956498,  0.5255324099163290,
         0.7966664774136267,  0.9602898564975363,
    };
    static constexpr std::array<double, 8> weights = {
        0.1012285362903763, 0.2223810344533745,
        0.3137066458778873, 0.3626837833783620,
        0.3626837833783620, 0.3137066458778873,
        0.2223810344533745, 0.1012285362903763,
    };

    const double half = 0.5 * u;
    const double center = 0.5 * u;
    double sum = 0.0;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const double t = center + half * nodes[i];
        sum += weights[i] * evaluateDerivative(t).norm();
    }
    return half * sum;
}

void MovePath::buildArcLengthLut()
{
    for (size_t i = 0; i < kArcLutSize; ++i)
    {
        const double u = static_cast<double>(i) / static_cast<double>(kArcLutSize - 1);
        arcLut_[i] = integrateSpeed(u);
    }
    arcLut_.back() = geometryLength_;
}

double MovePath::arcLengthToParameter(double localS) const
{
    localS = std::clamp(localS, 0.0, geometryLength_);
    if (geometryLength_ <= 1e-9)
    {
        return 0.0;
    }

    auto upper = std::lower_bound(arcLut_.begin(), arcLut_.end(), localS);
    if (upper == arcLut_.begin())
    {
        return 0.0;
    }
    if (upper == arcLut_.end())
    {
        return 1.0;
    }

    const size_t hi = static_cast<size_t>(upper - arcLut_.begin());
    const size_t lo = hi - 1;
    const double s0 = arcLut_[lo];
    const double s1 = arcLut_[hi];
    const double span = s1 - s0;
    const double ratio = span > 1e-12 ? (localS - s0) / span : 0.0;
    const double u0 = static_cast<double>(lo) / static_cast<double>(kArcLutSize - 1);
    const double u1 = static_cast<double>(hi) / static_cast<double>(kArcLutSize - 1);
    double u = u0 + ratio * (u1 - u0);
    double loU = u0;
    double hiU = u1;

    // 牛顿精修：用真实�? s(u)=integrateSpeed(u) 消掉分段线性插值的导数跳变
    for (int iter = 0; iter < 2; ++iter)
    {
        const double currentS = integrateSpeed(u);
        const double error = currentS - localS;
        if (std::abs(error) <= 1e-10)
        {
            break;
        }

        if (currentS > localS)
        {
            hiU = u;
        }
        else
        {
            loU = u;
        }

        const double speed = evaluateDerivative(u).norm();
        if (speed <= 1e-12)
        {
            u = 0.5 * (loU + hiU);
            break;
        }

        const double nextU = u - error / speed;
        if (std::isfinite(nextU) && nextU > loU && nextU < hiU)
        {
            u = nextU;
        }
        else
        {
            u = 0.5 * (loU + hiU);
        }
    }

    return std::clamp(u, u0, u1);
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
    isLinear_ = !isArc_;

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
    }
    else
    {
        startPos_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P0X)],
            command_->args[static_cast<size_t>(MovePathArg::P0Y)],
            command_->args[static_cast<size_t>(MovePathArg::P0Z)]);
        targetPos_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P1X)],
            command_->args[static_cast<size_t>(MovePathArg::P1Y)],
            command_->args[static_cast<size_t>(MovePathArg::P1Z)]);
        for (int axis = 0; axis < 3; ++axis)
        {
            coeff_[axis][0] = startPos_(axis);
            coeff_[axis][1] = targetPos_(axis) - startPos_(axis);
            coeff_[axis][2] = 0.0;
            coeff_[axis][3] = 0.0;
        }
    }

    startPos_ = evaluate(0.0);
    targetPos_ = evaluate(1.0);

    startQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MovePathArg::QStartW)],
        command_->args[static_cast<size_t>(MovePathArg::QStartX)],
        command_->args[static_cast<size_t>(MovePathArg::QStartY)],
        command_->args[static_cast<size_t>(MovePathArg::QStartZ)]);
    endQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MovePathArg::QEndW)],
        command_->args[static_cast<size_t>(MovePathArg::QEndX)],
        command_->args[static_cast<size_t>(MovePathArg::QEndY)],
        command_->args[static_cast<size_t>(MovePathArg::QEndZ)]);

    const double startNorm = startQuat_.norm();
    const double endNorm = endQuat_.norm();
    if (!std::isfinite(startNorm) || !std::isfinite(endNorm) ||
        startNorm < 1e-12 || endNorm < 1e-12)
    {
        ERROR_PRINT("MovePath: quaternion is invalid\n");
        return false;
    }
    startQuat_.normalize();
    endQuat_.normalize();

    const double commandLength = command_->args[static_cast<size_t>(MovePathArg::Length)];
    if (!std::isfinite(commandLength) || commandLength < 1e-6)
    {
        ERROR_PRINT("MovePath: path length is zero or invalid\n");
        return false;
    }
    pathLength_ = commandLength;

    // 高斯-勒让德积分计算曲线真实弧长
    if (isLinear_)
    {
        geometryLength_ = (targetPos_ - startPos_).norm();
    }
    else if (isArc_)
    {
        geometryLength_ = std::abs(arcRadius_ * arcSweep_);
    }
    else
    {
        geometryLength_ = integrateSpeed(1.0);
    }
    if (!std::isfinite(geometryLength_) || geometryLength_ < 1e-6)
    {
        ERROR_PRINT("MovePath: polynomial arc length is zero or invalid\n");
        return false;
    }

    if (!isLinear_ && !isArc_)
    {
        buildArcLengthLut();
    }

    const double maxVel = command_->args[static_cast<size_t>(MovePathArg::Vel)];
    const double targetVel = command_->args[static_cast<size_t>(MovePathArg::TargetVel)];
    const double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    const double maxJerk = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);

    if (!std::isfinite(maxVel) || maxVel <= 0.0 ||
        !std::isfinite(targetVel) || targetVel < 0.0 ||
        !std::isfinite(maxAccel) || maxAccel <= 0.0 ||
        !std::isfinite(maxJerk) || maxJerk <= 0.0)
    {
        ERROR_PRINT("MovePath: velocity, acceleration or jerk limit is invalid\n");
        return false;
    }

    arcOffset_ = arcOffset_ + geometryLength_;
    input_->target_position[0] = arcOffset_;
    input_->target_velocity[0] = targetVel;
    input_->target_acceleration[0] = 0.0;
    input_->max_velocity[0] = maxVel;
    input_->max_acceleration[0] = maxAccel;
    input_->max_jerk[0] = maxJerk;

    return true;
}

void MovePath::applyOutput()
{
    // otg_->update() 已由 TrajectoryCmd::runStandard()->updateTrajectory() 完成，
    // output_ 已包含当前周期的轨迹输出，此处不再重复调用 update/pass_to_input。

    double s = output_->new_position[0];
    const double geometryS = s - (arcOffset_ - geometryLength_);
    const double u = (isLinear_ || isArc_)
                   ? (geometryLength_ > 1e-12 ? geometryS / geometryLength_ : 0.0)
                   : arcLengthToParameter(geometryS);

    const Eigen::Vector3d pos = isLinear_
                              ? startPos_ + u * (targetPos_ - startPos_)
                              : evaluate(u);

    Eigen::Vector3d pathDir = Eigen::Vector3d::Zero();
    if (isLinear_)
    {
        if (geometryLength_ > 1e-12)
        {
            pathDir = (targetPos_ - startPos_) / geometryLength_;
        }
    }
    else
    {
        const Eigen::Vector3d du = evaluateDerivative(u);
        const double duNorm = du.norm();
        if (duNorm > 1e-12)
        {
            pathDir = du / duNorm;
        }
    }
    const double pathVelocity = output_->new_velocity[0];

    controller_->axes_[axisIds_[0]]->setAxisPositionCmd(pos.x());
    controller_->axes_[axisIds_[1]]->setAxisPositionCmd(pos.y());
    controller_->axes_[axisIds_[2]]->setAxisPositionCmd(pos.z());
    controller_->axes_[axisIds_[0]]->setAxisVelocityCmd(pathVelocity * pathDir.x());
    controller_->axes_[axisIds_[1]]->setAxisVelocityCmd(pathVelocity * pathDir.y());
    controller_->axes_[axisIds_[2]]->setAxisVelocityCmd(pathVelocity * pathDir.z());

    // // 四元数球面线性插补 (SLERP) — 跳过相同四元数，避�? slerp(q,q) 0/0 数值噪声
    // Eigen::Quaterniond qInterp;
    // if (startQuat_.dot(endQuat_) >= 1.0 - 1e-12) {
    //     qInterp = startQuat_;
    // } else {
    //     qInterp = startQuat_.slerp(u, endQuat_);
    // }
    // if (axisIds_.size() >= 6)
    // {
    //     Eigen::Vector3d euler = qInterp.toRotationMatrix().canonicalEulerAngles(2, 1, 0);
    //     controller_->axes_[axisIds_[3]]->setAxisPositionCmd(euler(2));  // rx
    //     controller_->axes_[axisIds_[4]]->setAxisPositionCmd(euler(1));  // ry
    //     controller_->axes_[axisIds_[5]]->setAxisPositionCmd(euler(0));  // rz
    // }
}

CMD_REGISTER(MovePath);
