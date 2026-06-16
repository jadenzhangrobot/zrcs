#pragma once

#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include "system/node/NodeFactory.h"
#include "system/node/TrajectoryCmd.h"

#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MovePath : public zrcsSystem::TrajectoryCmd
{
private:
    static constexpr size_t kArcLutSize = 128;

    std::vector<int> axisIds_;
    bool modelInited_ = false;

    double coeff_[3][4] = {};
    Eigen::Vector3d startPos_;
    Eigen::Vector3d targetPos_;
    Eigen::Quaterniond startQuat_;
    Eigen::Quaterniond endQuat_;
    double pathLength_ = 0.0;
    double geometryLength_ = 0.0;
    double arcOffset_ = 0.0;
    bool isLinear_ = true;
    std::array<double, kArcLutSize> arcLut_ = {};

    Eigen::Vector3d evaluate(double u) const;
    Eigen::Vector3d evaluateDerivative(double u) const;
    double integrateSpeed(double u) const;
    void buildArcLengthLut();
    double arcLengthToParameter(double localS) const;

protected:
    bool initTrajectory() override;
    void applyOutput() override;

public:
    MovePath();
};
