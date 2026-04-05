/*
 * @Description: 缓冲区连续插补（固高 GT_CrdData / 正运动连续插补）
 *               预装多段轨迹到缓冲区，不停顿连续执行
 */
#pragma once
#include "config/cmdArgs.h"
#include "command/TrajectoryCmd.h"
#include "system/nodeFactory.h"
#include <memory>
#include <vector>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class BufMove : public TrajectoryCmd
{
private:
    struct Segment
    {
        int type;         // 0=line, 1=arc, 2=spline
        double params[6]; // P1~P5 + Vel
        double vel;
    };

    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;
    int segIdx_;
    std::vector<Segment> segments_;

    void setupSegment(int idx);

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override { return otg_->update(*input_, *output_); }
    void applyOutput() override;
    void passOutputToInput() override { output_->pass_to_input(*input_); }
    void applyDeltaTime(double dt) override { if (otg_) otg_->delta_time = dt; }

public:
    BufMove() : dof_(0), segIdx_(0)
    {
        std::strcpy(nodeName_, "BufMove");
    }

    void init() override;
    void run() override;
};
