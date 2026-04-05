/*
 * @Description: 直线搜索运动（ABB SearchL）— 运动中检测IO后停止记录位置
 */
#pragma once
#include "config/cmdArgs.h"
#include "command/TrajectoryCmd.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
#include <memory>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class SearchL : public TrajectoryCmd
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;
    int ioModule_;
    int ioBit_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override { return otg_->update(*input_, *output_); }
    void applyOutput() override;
    void passOutputToInput() override { output_->pass_to_input(*input_); }
    void applyDeltaTime(double dt) override { otg_->delta_time = dt; }

public:
    SearchL() : dof_(0), ioModule_(0), ioBit_(0)
    {
        std::strcpy(nodeName_, "SearchL");
    }

    void run() override;
};
