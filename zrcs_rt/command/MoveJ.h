/*
 * @Description: 笛卡尔目�?关节路径PTP运动（ABB MoveJ）�?经IK解算
 */
#pragma once
#include "system/node/TrajectoryCmd.h"
#include "system/node/NodeFactory.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveJ : public zrcsSystem::TrajectoryCmd
{

private:
    int dof_;
    std::vector<int> axisIds_;

protected:
    bool initTrajectory() override;
   
    void applyOutput() override;
  

public:
    MoveJ();
};

