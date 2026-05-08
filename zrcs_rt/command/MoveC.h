/*
 * @Description: 圆弧运动（ABB MoveC）�?三点定弧，经IK解算
 */
#pragma once
#include "system/node/TrajectoryCmd.h"
#include "system/node/NodeFactory.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <cmath>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveC : public TrajectoryCmd
{

private:
    int dof_;
    std::vector<int> axisIds_;

    Eigen::Vector3d center_;
    Eigen::Vector3d axis_;
    double radius_;
    double totalAngle_;
    Eigen::Vector3d startPos_;
    Eigen::Vector3d startRadial_;
    Eigen::Vector3d biNormal_;
    double zStart_;
    double zEnd_;

protected:
    bool initTrajectory() override;
    
    void applyOutput() override;
  

public:
    MoveC();
    void run() override;
};
