#include "command/MoveCurve.h"
#include "shared_memory/ShmLayout.h"

#include <cmath>
#include <cstring>

MoveCurve::MoveCurve()
{
    std::strcpy(nodeName_, "MoveCurve");
}

bool MoveCurve::prepare()
{
    if (prepared_)
    {
        return true;
    }
    if (!controller_ || !modelRegistry_)
    {
        ERROR_PRINT("MoveCurve: controller/model registry is unavailable during prepare\n");
        return false;
    }

    RobotModel* model = modelRegistry_->getModel(0);
    if (!model)
    {
        ERROR_PRINT("MoveCurve: model id=0 not found during prepare\n");
        return false;
    }

    axisIds_ = model->getAxisIds();
    if (axisIds_.size() < 3)
    {
        ERROR_PRINT("MoveCurve: model must expose at least 3 axes\n");
        return false;
    }
    for (size_t i = 0; i < axisIds_.size(); ++i)
    {
        const int axisId = axisIds_[i];
        if (axisId < 0 || axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            ERROR_PRINT("MoveCurve: axisIds_[%zu]=%d is unavailable during prepare\n",
                        i, axisId);
            return false;
        }
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
    prepared_ = true;
    return true;
}

Eigen::Vector3d MoveCurve::evaluate(double u) const
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

void MoveCurve::buildArcLengthLut()
{
    arcLut_[0] = 0.0;

    double cumulative = 0.0;
    Eigen::Vector3d previous = evaluate(0.0);
    for (size_t i = 1; i < kArcLutSize; ++i)
    {
        const double u = static_cast<double>(i) / static_cast<double>(kArcLutSize - 1);
        const Eigen::Vector3d next = evaluate(u);
        cumulative += (next - previous).norm();
        arcLut_[i] = cumulative;
        previous = next;
    }

    if (cumulative > 1e-9)
    {
        const double scale = curveLength_ / cumulative;
        for (double& s : arcLut_)
        {
            s *= scale;
        }
    }
    arcLut_.back() = curveLength_;
}

double MoveCurve::arcLengthToParameter(double localS) const
{
    localS = std::clamp(localS, 0.0, curveLength_);
    if (curveLength_ <= 1e-9)
    {
        return 0.0;
    }

    const auto upper = std::lower_bound(arcLut_.begin(), arcLut_.end(), localS);
    if (upper == arcLut_.begin())
    {
        return 0.0;
    }
    if (upper == arcLut_.end())
    {
        return 1.0;
    }

    const size_t high = static_cast<size_t>(upper - arcLut_.begin());
    const size_t low = high - 1;
    const double s0 = arcLut_[low];
    const double s1 = arcLut_[high];
    const double span = s1 - s0;
    const double ratio = span > 1e-12 ? (localS - s0) / span : 0.0;
    const double u0 = static_cast<double>(low) / static_cast<double>(kArcLutSize - 1);
    const double u1 = static_cast<double>(high) / static_cast<double>(kArcLutSize - 1);
    return u0 + ratio * (u1 - u0);
}

bool MoveCurve::initTrajectory()
{
    if (!prepared_ || !command_ || !otg_ || !input_ || !output_)
    {
        ERROR_PRINT("MoveCurve: command or prepared resources are unavailable\n");
        return false;
    }

    if (command_->args[static_cast<size_t>(MoveCurveArg::Sync)] == 1.0)
    {
        input_->current_position[0] = 0.0;
        input_->current_velocity[0] = 0.0;
        input_->current_acceleration[0] = 0.0;
    }

    coeff_[0][0] = command_->args[static_cast<size_t>(MoveCurveArg::X0)];
    coeff_[0][1] = command_->args[static_cast<size_t>(MoveCurveArg::X1)];
    coeff_[0][2] = command_->args[static_cast<size_t>(MoveCurveArg::X2)];
    coeff_[0][3] = command_->args[static_cast<size_t>(MoveCurveArg::X3)];
    coeff_[1][0] = command_->args[static_cast<size_t>(MoveCurveArg::Y0)];
    coeff_[1][1] = command_->args[static_cast<size_t>(MoveCurveArg::Y1)];
    coeff_[1][2] = command_->args[static_cast<size_t>(MoveCurveArg::Y2)];
    coeff_[1][3] = command_->args[static_cast<size_t>(MoveCurveArg::Y3)];
    coeff_[2][0] = command_->args[static_cast<size_t>(MoveCurveArg::Z0)];
    coeff_[2][1] = command_->args[static_cast<size_t>(MoveCurveArg::Z1)];
    coeff_[2][2] = command_->args[static_cast<size_t>(MoveCurveArg::Z2)];
    coeff_[2][3] = command_->args[static_cast<size_t>(MoveCurveArg::Z3)];

    startQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MoveCurveArg::QStartW)],
        command_->args[static_cast<size_t>(MoveCurveArg::QStartX)],
        command_->args[static_cast<size_t>(MoveCurveArg::QStartY)],
        command_->args[static_cast<size_t>(MoveCurveArg::QStartZ)]);
    endQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MoveCurveArg::QEndW)],
        command_->args[static_cast<size_t>(MoveCurveArg::QEndX)],
        command_->args[static_cast<size_t>(MoveCurveArg::QEndY)],
        command_->args[static_cast<size_t>(MoveCurveArg::QEndZ)]);

    const double startNorm = startQuat_.norm();
    const double endNorm = endQuat_.norm();
    if (!std::isfinite(startNorm) || !std::isfinite(endNorm) ||
        startNorm < 1e-12 || endNorm < 1e-12)
    {
        ERROR_PRINT("MoveCurve: quaternion is invalid\n");
        return false;
    }
    startQuat_.normalize();
    endQuat_.normalize();

    curveLength_ = command_->args[static_cast<size_t>(MoveCurveArg::Length)];
    if (!std::isfinite(curveLength_) || curveLength_ < 1e-6)
    {
        ERROR_PRINT("MoveCurve: curve length is zero or invalid\n");
        return false;
    }
    buildArcLengthLut();
    arcStart_ = input_->current_position[0];

    const double maxVel = command_->args[static_cast<size_t>(MoveCurveArg::Vel)];
    const double targetVel = command_->args[static_cast<size_t>(MoveCurveArg::TargetVel)];
    const double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    const double maxJerk = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);
    if (!std::isfinite(maxVel) || maxVel <= 0.0 ||
        !std::isfinite(targetVel) || targetVel < 0.0 ||
        !std::isfinite(maxAccel) || maxAccel <= 0.0 ||
        !std::isfinite(maxJerk) || maxJerk <= 0.0)
    {
        ERROR_PRINT("MoveCurve: velocity, acceleration or jerk limit is invalid\n");
        return false;
    }

    input_->target_position[0] = arcStart_ + curveLength_;
    input_->target_velocity[0] = targetVel;
    input_->target_acceleration[0] = 0.0;
    input_->max_velocity[0] = maxVel;
    input_->max_acceleration[0] = maxAccel;
    input_->max_jerk[0] = maxJerk;
    return true;
}

bool MoveCurve::applyOutput()
{
    const double localS = output_->new_position[0] - arcStart_;
    const double u = arcLengthToParameter(localS);
    const Eigen::Vector3d pos = evaluate(u);

    controller_->axes_[axisIds_[0]]->setAxisPositionCmd(pos.x());
    controller_->axes_[axisIds_[1]]->setAxisPositionCmd(pos.y());
    controller_->axes_[axisIds_[2]]->setAxisPositionCmd(pos.z());

    const Eigen::Quaterniond orientation = startQuat_.slerp(u, endQuat_);
    if (axisIds_.size() >= 6)
    {
        const Eigen::Vector3d euler =
            orientation.toRotationMatrix().canonicalEulerAngles(2, 1, 0);
        controller_->axes_[axisIds_[3]]->setAxisPositionCmd(euler(2));
        controller_->axes_[axisIds_[4]]->setAxisPositionCmd(euler(1));
        controller_->axes_[axisIds_[5]]->setAxisPositionCmd(euler(0));
    }
    return true;
}

REGISTERCMD(MoveCurve);
