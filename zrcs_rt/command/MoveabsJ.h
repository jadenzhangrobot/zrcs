/*
 * @Description: 绝对关节位置运动（ABB MoveAbsJ）�?多轴同步，不经IK
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveAbsJ : public TrajectoryCmd
{

private:
    int dof_;
    std::vector<int> axisIds_;

protected:
    bool initTrajectory() override;
    
    void applyOutput() override;
  

public:
    MoveAbsJ();
};
