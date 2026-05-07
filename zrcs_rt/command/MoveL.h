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

private:
    std::vector<int> axisIds_;      // 构造函数中预分配，RT 只读
    bool modelInited_ = false;

    // 线段几何缓存
    Eigen::Vector3d startPos_;
    Eigen::Vector3d targetPos_;
    Eigen::Quaterniond startQuat_;
    Eigen::Quaterniond targetQuat_;
    double cartDist_;

protected:
    bool initTrajectory() override;
   
    void applyOutput() override;
  

public:
    MoveL();
};
