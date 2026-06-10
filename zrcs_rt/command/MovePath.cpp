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
    u = std::clamp(u, 0.0, 1.0);
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
    u = std::clamp(u, 0.0, 1.0);
    const double u2 = u * u;

    return {
        coeff_[0][1] + 2.0 * coeff_[0][2] * u + 3.0 * coeff_[0][3] * u2,
        coeff_[1][1] + 2.0 * coeff_[1][2] * u + 3.0 * coeff_[1][3] * u2,
        coeff_[2][1] + 2.0 * coeff_[2][2] * u + 3.0 * coeff_[2][3] * u2,
    };
}

Eigen::Vector3d MovePath::evaluateSecondDerivative(double u) const
{
    u = std::clamp(u, 0.0, 1.0);

    return {
        2.0 * coeff_[0][2] + 6.0 * coeff_[0][3] * u,
        2.0 * coeff_[1][2] + 6.0 * coeff_[1][3] * u,
        2.0 * coeff_[2][2] + 6.0 * coeff_[2][3] * u,
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

double MovePath::integrateSpeedBetween(double u0, double u1) const
{
    u0 = std::clamp(u0, 0.0, 1.0);
    u1 = std::clamp(u1, 0.0, 1.0);
    if (std::abs(u1 - u0) <= 1e-12)
    {
        return 0.0;
    }

    const double sign = u1 >= u0 ? 1.0 : -1.0;
    const double from = sign > 0.0 ? u0 : u1;
    const double to = sign > 0.0 ? u1 : u0;

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

    const double half = 0.5 * (to - from);
    const double center = 0.5 * (from + to);
    double sum = 0.0;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const double t = center + half * nodes[i];
        sum += weights[i] * evaluateDerivative(t).norm();
    }
    return sign * half * sum;
}

double MovePath::arcLengthAt(double u) const
{
    return integrateSpeed(u);
}

void MovePath::buildArcLengthLut()
{
    for (size_t i = 0; i < kArcLutSize; ++i)
    {
        const double u = static_cast<double>(i) / static_cast<double>(kArcLutSize - 1);
        arcLut_[i] = arcLengthAt(u);
    }
    arcLut_.back() = pathLength_;
}

double MovePath::arcLengthToParameter(double localS) const
{
    localS = std::clamp(localS, 0.0, pathLength_);
    if (pathLength_ <= 1e-9)
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

    for (int iter = 0; iter < 4; ++iter)
    {
        const double speed = evaluateDerivative(u).norm();
        if (speed <= 1e-12)
        {
            break;
        }

        const double error = arcLengthAt(u) - localS;
        if (std::abs(error) <= 1e-9)
        {
            break;
        }
        u = std::clamp(u - error / speed, 0.0, 1.0);
    }

    return u;
}

double MovePath::advanceParameterByArc(double localS)
{
    localS = std::clamp(localS, 0.0, pathLength_);
    if (pathLength_ <= 1e-9)
    {
        currentU_ = 0.0;
        lastLocalS_ = 0.0;
        parameterInited_ = true;
        return currentU_;
    }

    if (localS <= 1e-9)
    {
        currentU_ = 0.0;
        lastLocalS_ = localS;
        parameterInited_ = true;
        return currentU_;
    }
    if (pathLength_ - localS <= 1e-9)
    {
        currentU_ = 1.0;
        lastLocalS_ = localS;
        parameterInited_ = true;
        return currentU_;
    }

    if (!parameterInited_)
    {
        currentU_ = arcLengthToParameter(localS);
        lastLocalS_ = localS;
        parameterInited_ = true;
        return currentU_;
    }

    const double deltaS = localS - lastLocalS_;
    if (!std::isfinite(deltaS))
    {
        currentU_ = arcLengthToParameter(localS);
        lastLocalS_ = localS;
        return currentU_;
    }
    if (std::abs(deltaS) <= 1e-12)
    {
        lastLocalS_ = localS;
        return currentU_;
    }

    const double maxLocalStep = std::max(pathLength_ * 0.25, 1e-6);
    if (std::abs(deltaS) > maxLocalStep)
    {
        currentU_ = arcLengthToParameter(localS);
        lastLocalS_ = localS;
        return currentU_;
    }

    const double u0 = std::clamp(currentU_, 0.0, 1.0);
    const Eigen::Vector3d d1 = evaluateDerivative(u0);
    const double speed = d1.norm();
    if (!std::isfinite(speed) || speed <= 1e-12)
    {
        currentU_ = arcLengthToParameter(localS);
        lastLocalS_ = localS;
        return currentU_;
    }

    const Eigen::Vector3d d2 = evaluateSecondDerivative(u0);
    const double speed4 = speed * speed * speed * speed;
    const double secondOrder = speed4 > 1e-24
                             ? -d1.dot(d2) * deltaS * deltaS / (2.0 * speed4)
                             : 0.0;
    double u = u0 + deltaS / speed + secondOrder;

    const double lo = deltaS >= 0.0 ? u0 : 0.0;
    const double hi = deltaS >= 0.0 ? 1.0 : u0;
    u = std::clamp(u, lo, hi);

    for (int iter = 0; iter < 4; ++iter)
    {
        const double speedAtU = evaluateDerivative(u).norm();
        if (!std::isfinite(speedAtU) || speedAtU <= 1e-12)
        {
            break;
        }

        const double error = integrateSpeedBetween(u0, u) - deltaS;
        if (std::abs(error) <= 1e-10)
        {
            break;
        }
        u = std::clamp(u - error / speedAtU, lo, hi);
    }

    if (!std::isfinite(u))
    {
        currentU_ = arcLengthToParameter(localS);
    }
    else
    {
        currentU_ = u;
    }
    lastLocalS_ = localS;
    return currentU_;
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
        input_->current_position[0] = 0.0;
        input_->current_velocity[0] = 0.0;
        input_->current_acceleration[0] = 0.0;
    }

    coeff_[0][0] = command_->args[static_cast<size_t>(MovePathArg::X0)];
    coeff_[0][1] = command_->args[static_cast<size_t>(MovePathArg::X1)];
    coeff_[0][2] = command_->args[static_cast<size_t>(MovePathArg::X2)];
    coeff_[0][3] = command_->args[static_cast<size_t>(MovePathArg::X3)];
    coeff_[1][0] = command_->args[static_cast<size_t>(MovePathArg::Y0)];
    coeff_[1][1] = command_->args[static_cast<size_t>(MovePathArg::Y1)];
    coeff_[1][2] = command_->args[static_cast<size_t>(MovePathArg::Y2)];
    coeff_[1][3] = command_->args[static_cast<size_t>(MovePathArg::Y3)];
    coeff_[2][0] = command_->args[static_cast<size_t>(MovePathArg::Z0)];
    coeff_[2][1] = command_->args[static_cast<size_t>(MovePathArg::Z1)];
    coeff_[2][2] = command_->args[static_cast<size_t>(MovePathArg::Z2)];
    coeff_[2][3] = command_->args[static_cast<size_t>(MovePathArg::Z3)];

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

    const double rawLength = integrateSpeed(1.0);
    if (!std::isfinite(rawLength) || rawLength < 1e-9)
    {
        ERROR_PRINT("MovePath: polynomial arc length is zero or invalid\n");
        return false;
    }
    pathLength_ = rawLength;

    buildArcLengthLut();

    arcStart_ = input_->current_position[0];
    currentU_ = 0.0;
    lastLocalS_ = 0.0;
    parameterInited_ = true;

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

    input_->target_position[0] = arcStart_ + pathLength_;
    input_->target_velocity[0] = targetVel;
    input_->target_acceleration[0] = 0.0;
    input_->max_velocity[0] = maxVel;
    input_->max_acceleration[0] = maxAccel;
    input_->max_jerk[0] = maxJerk;

    return true;
}

void MovePath::applyOutput()
{
    const double localS = output_->new_position[0] - arcStart_;
    const double u = advanceParameterByArc(localS);
    const Eigen::Vector3d pos = evaluate(u);

    controller_->axes_[axisIds_[0]]->setAxisPositionCmd(pos.x());
    controller_->axes_[axisIds_[1]]->setAxisPositionCmd(pos.y());
    controller_->axes_[axisIds_[2]]->setAxisPositionCmd(pos.z());

    Eigen::Quaterniond qInterp = startQuat_.slerp(u, endQuat_);
    if (axisIds_.size() >= 6)
    {
        Eigen::Vector3d euler = qInterp.toRotationMatrix().canonicalEulerAngles(2, 1, 0);
        controller_->axes_[axisIds_[3]]->setAxisPositionCmd(euler(2));
        controller_->axes_[axisIds_[4]]->setAxisPositionCmd(euler(1));
        controller_->axes_[axisIds_[5]]->setAxisPositionCmd(euler(0));
    }
}

CMD_REGISTER(MovePath);
