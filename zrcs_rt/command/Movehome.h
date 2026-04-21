/*
 * @Description: 多轴回零命令
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class Movehome : public TrajectoryCmd
{
public:
    CMD_DEFINE(30)

private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    Movehome();
};

