/*
 * @Description: 笛卡尔直线运动（ABB MoveL）�?1D 弧长参数�?+ 每周�?IK
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

class MoveL : public TrajectoryCmd
{
public:
    

private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;

    // 线段几何缓存
    Eigen::Vector3d startPos_;
    Eigen::Vector3d targetPos_;
    Eigen::Vector3d startRpy_;
    Eigen::Vector3d targetRpy_;
    double cartDist_;
    bool firstSegment_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    MoveL();
    void run() override;
};
