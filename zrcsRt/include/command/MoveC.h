/*
 * @Description: 圆弧运动（ABB MoveC）— 三点定弧，经IK解算
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/TrajectoryCmd.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
#include <memory>
#include <cmath>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveC : public TrajectoryCmd
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
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
    Result updateTrajectory() override { return otg_->update(*input_, *output_); }
    void applyOutput() override {}  // 不使用，MoveC 在 run() 中自行处理
    void passOutputToInput() override { output_->pass_to_input(*input_); }
    void applyDeltaTime(double dt) override { otg_->delta_time = dt; }

public:
    MoveC() : dof_(0), radius_(0), totalAngle_(0), zStart_(0), zEnd_(0)
    {
        std::strcpy(nodeName_, "MoveC");
    }

    void run() override;
};
