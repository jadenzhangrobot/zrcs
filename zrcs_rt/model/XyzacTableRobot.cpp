#include "model/XyzacTableRobot.h"

#include <cmath>

namespace {

double unwrapNear(double angle, double reference)
{
    constexpr double kTwoPi = 2.0 * M_PI;
    return angle + kTwoPi * std::round((reference - angle) / kTwoPi);
}

Eigen::Matrix3d xyzacRotation(double a, double c)
{
    return (Eigen::AngleAxisd(a, Eigen::Vector3d::UnitX()) *
            Eigen::AngleAxisd(c, Eigen::Vector3d::UnitZ())).toRotationMatrix();
}

bool extractXyzacAngles(const Eigen::Matrix3d& rotation, double& a, double& c)
{
    a = std::atan2(-rotation(1, 2), rotation(2, 2));
    c = std::atan2(-rotation(0, 1), rotation(0, 0));
    return (xyzacRotation(a, c) - rotation).norm() <= 1e-8;
}

} // namespace

Eigen::Matrix4d XyzacTableRobot::tablePose(double a, double c) const
{
    Eigen::Matrix4d rotateA = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d rotateC = Eigen::Matrix4d::Identity();
    rotateA.block<3, 3>(0, 0) =
        Eigen::AngleAxisd(a, Eigen::Vector3d::UnitX()).toRotationMatrix();
    rotateC.block<3, 3>(0, 0) =
        Eigen::AngleAxisd(c, Eigen::Vector3d::UnitZ()).toRotationMatrix();
    return baseTf_ * rotateA * toolTf_ * rotateC;
}

bool XyzacTableRobot::forwardKinematics(const Eigen::VectorXd& jointPos,
                                        Eigen::Matrix4d& toolPose) const
{
    if (jointPos.size() != 5 || joints_.size() != 5)
    {
        return false;
    }

    const double a = jointPos(3) + joints_[3].offset;
    const double c = jointPos(4) + joints_[4].offset;
    const Eigen::Vector3d tipWorld(
        jointPos(0) + joints_[0].offset,
        jointPos(1) + joints_[1].offset,
        jointPos(2) + joints_[2].offset);

    const Eigen::Vector4d pointWorkHomogeneous =
        tablePose(a, c).inverse() * tipWorld.homogeneous();
    const Eigen::Vector3d pointWork = pointWorkHomogeneous.head<3>();

    // The rotation block is the path protocol's A/C encoding. The physical
    // spindle stays fixed; only the work-frame point is transformed by RTCP.
    toolPose = Eigen::Matrix4d::Identity();
    toolPose.block<3, 3>(0, 0) = xyzacRotation(a, c);
    toolPose.block<3, 1>(0, 3) = pointWork;
    return toolPose.allFinite();
}

bool XyzacTableRobot::inverseKinematics(const Eigen::Matrix4d& toolPose,
                                        const Eigen::VectorXd& currentJointPos,
                                        Eigen::VectorXd& targetJointPos) const
{
    if (currentJointPos.size() != 5 || joints_.size() != 5 ||
        !toolPose.allFinite())
    {
        return false;
    }

    double a = 0.0;
    double c = 0.0;
    if (!extractXyzacAngles(toolPose.block<3, 3>(0, 0), a, c))
    {
        return false;
    }

    a = unwrapNear(a - joints_[3].offset, currentJointPos(3));
    c = unwrapNear(c - joints_[4].offset, currentJointPos(4));
    const double modelA = a + joints_[3].offset;
    const double modelC = c + joints_[4].offset;
    const Eigen::Vector4d tipWorldHomogeneous =
        tablePose(modelA, modelC) * toolPose.block<3, 1>(0, 3).homogeneous();
    const Eigen::Vector3d tipWorld = tipWorldHomogeneous.head<3>();

    targetJointPos.resize(5);
    targetJointPos(0) = tipWorld.x() - joints_[0].offset;
    targetJointPos(1) = tipWorld.y() - joints_[1].offset;
    targetJointPos(2) = tipWorld.z() - joints_[2].offset;
    targetJointPos(3) = a;
    targetJointPos(4) = c;
    return targetJointPos.allFinite();
}
