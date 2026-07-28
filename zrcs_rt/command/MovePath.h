#pragma once

#include "system/node/TrajectoryCmd.h"

#include <Eigen/Dense>
#include <memory>
#include <ruckig/ruckig.hpp>
#include <vector>

using namespace ruckig;

class RobotModel;

/// 路径段运动：直线 / 圆弧。
/// Shape=0 直线，Shape=1 圆弧；弧长均有闭式解。
/// 三次多项式曲线见 MoveCurve。
class MovePath : public zrcsSystem::TrajectoryCmd
{
private:
    std::vector<int> axisIds_;
    RobotModel* model_ = nullptr;
    int dof_ = 0;
    bool modelInited_ = false;
    bool rtcp5Axis_ = false;
    bool jointTargetValid_ = false;

    Eigen::Vector3d startPos_;
    Eigen::Vector3d targetPos_;
    Eigen::Vector3d arcCenter_;
    Eigen::Vector3d arcU_;
    Eigen::Vector3d arcV_;
    double geometryLength_ = 0.0;
    double arcOffset_ = 0.0;
    double arcRadius_ = 0.0;
    double arcSweep_ = 0.0;
    bool isArc_ = false;

    Eigen::Quaterniond startQuat_;
    Eigen::Quaterniond endQuat_;
    Eigen::VectorXd ikSeed_;
    Eigen::VectorXd targetJoint_;
    Eigen::VectorXd lastJointTarget_;
    double startA_ = 0.0;
    double startC_ = 0.0;
    double endA_ = 0.0;
    double endC_ = 0.0;

    Eigen::Vector3d evaluateArc(double u) const;
    bool initializeRtcp();
    bool solveRtcp(const Eigen::Vector3d& pos, double u);


protected:
    bool initTrajectory() override;
    bool applyOutput() override;

public:
    MovePath();
};
