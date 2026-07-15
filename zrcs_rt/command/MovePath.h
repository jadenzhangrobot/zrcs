#pragma once

#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include "system/node/NodeFactory.h"
#include "system/node/TrajectoryCmd.h"

#include <Eigen/Dense>
#include <memory>
#include <ruckig/ruckig.hpp>
#include <vector>

using namespace ruckig;

/// 路径段运动：直线 / 圆弧。
/// Shape=0 直线，Shape=1 圆弧；弧长均有闭式解。
/// 三次多项式曲线见 MoveCurve。
class MovePath : public zrcsSystem::TrajectoryCmd
{
private:
    std::vector<int> axisIds_;
    bool modelInited_ = false;

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

    Eigen::Vector3d evaluateArc(double u) const;
    Eigen::Vector3d evaluateArcDerivative(double u) const;

protected:
    bool initTrajectory() override;
    void applyOutput() override;

public:
    MovePath();
};
