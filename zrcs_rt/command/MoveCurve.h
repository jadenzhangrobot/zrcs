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

class MoveCurve : public zrcsSystem::TrajectoryCmd
{
private:
    static constexpr size_t kArcLutSize = 64;

    std::vector<int> axisIds_;
    bool modelInited_ = false;

    double coeff_[3][4] = {};
    Eigen::Quaterniond startQuat_;
    Eigen::Quaterniond endQuat_;
    double curveLength_ = 0.0;
    double arcStart_ = 0.0;
    std::array<double, kArcLutSize> arcLut_ = {};

    Eigen::Vector3d evaluate(double u) const;
    void buildArcLengthLut();
    double arcLengthToParameter(double localS) const;

protected:
    bool initTrajectory() override;
    bool applyOutput() override;

public:
    MoveCurve();
};
