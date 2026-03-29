/*
 * @Description: 绝对关节位置运动（ABB MoveAbsJ）— 多轴同步，不经IK
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveAbsJ : public zrcsSystem::CmdNode
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;

public:
    MoveAbsJ() : dof_(0)
    {
        std::strcpy(nodeName_, "MoveAbsJ");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
