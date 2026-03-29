/*
 * @Description: 笛卡尔目标+关节路径PTP运动（ABB MoveJ）— 经IK解算
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
#include <memory>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveJ : public zrcsSystem::CmdNode
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;

public:
    MoveJ() : dof_(0)
    {
        std::strcpy(nodeName_, "MoveJ");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
