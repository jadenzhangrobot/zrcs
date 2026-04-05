/*
 * @Description: 螺旋线插补（固高 GT_HelixXYZ）
 *               XY平面圆弧 + Z轴直线联动
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

class HelixMove : public TrajectoryCmd
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;

    Eigen::Vector2d center_;
    double radius_;
    double startAngle_;
    double totalAngle_;
    double zStart_;
    double zEnd_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override { return otg_->update(*input_, *output_); }
    void applyOutput() override {}
    void passOutputToInput() override { output_->pass_to_input(*input_); }
    void applyDeltaTime(double dt) override { otg_->delta_time = dt; }

public:
    HelixMove() : dof_(0), radius_(0), startAngle_(0), totalAngle_(0),
                  zStart_(0), zEnd_(0)
    {
        std::strcpy(nodeName_, "HelixMove");
    }

    void run() override;
};
