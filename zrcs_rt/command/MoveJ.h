/*
 * @Description: 笛卡尔目�?关节路径PTP运动（ABB MoveJ）�?经IK解算
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveJ : public TrajectoryCmd
{


private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;

protected:
    bool initTrajectory() override;
   
    void applyOutput() override;
  
    void applyDeltaTime(double dt) override;

public:
    MoveJ();
};

