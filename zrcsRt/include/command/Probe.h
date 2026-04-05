/*
 * @Description: 单轴探针触发（ABB SearchL 单轴版 / 固高探针功能）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/TrajectoryCmd.h"
#include "system/nodeFactory.h"
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class Probe : public TrajectoryCmd
{
private:
    std::unique_ptr<Ruckig<1>> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;
    int axisId_;
    int ioIndex_;
    int bitPos_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    Probe();
    void run() override;
};
