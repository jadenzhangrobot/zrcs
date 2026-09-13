/*
 * @Description: 笛卡尔直线运动（ABB MoveL）�?1D 弧长参数�?+ 每周�?IK
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

class MoveL : public zrcsSystem::TrajectoryCmd
{

private:
    std::vector<int> axisIds_;      // Filled during prepare(), read-only in RT.
    int dof_ = 0;
    RobotModel* model_ = nullptr;   // Cached during prepare(), read-only in RT.
    bool prepared_ = false;
    Eigen::VectorXd targetJoint_;
    Eigen::VectorXd currentJoint_;  // Pre-sized IK seed.

    // 线段几何缓存
    Eigen::Vector3d startPos_;
    Eigen::Vector3d targetPos_;
    Eigen::Quaterniond startQuat_;
    Eigen::Quaterniond targetQuat_;
    double cartDist_;
    double arcOffset_ = 0.0;  // 当前段起点的弧长，从 Ruckig 状态接力或 Sync 归零

protected:
    bool initTrajectory() override;
   
    bool applyOutput() override;
  

public:
    MoveL();
    bool prepare() override;
};
