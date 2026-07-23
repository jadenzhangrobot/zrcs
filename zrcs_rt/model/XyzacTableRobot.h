#pragma once

#include "RobotModel.h"

// XYZAC table-table kinematics. The path position is expressed in the work
// frame; A and C rotate the work frame while XYZ position the world-space TCP.
class XyzacTableRobot : public RobotModel
{
public:
    XyzacTableRobot(const std::string& name,
                    const std::vector<ModelJoint>& joints,
                    const Eigen::Matrix4d& pivotTf,
                    const Eigen::Matrix4d& tableOffsetTf)
        : RobotModel(name, "xyzac_table", 5)
    {
        joints_ = joints;
        baseTf_ = pivotTf;
        toolTf_ = tableOffsetTf;
    }

    bool forwardKinematics(const Eigen::VectorXd& jointPos,
                           Eigen::Matrix4d& toolPose) const override;

    bool inverseKinematics(const Eigen::Matrix4d& toolPose,
                           const Eigen::VectorXd& currentJointPos,
                           Eigen::VectorXd& targetJointPos) const override;

private:
    Eigen::Matrix4d tablePose(double a, double c) const;
};
