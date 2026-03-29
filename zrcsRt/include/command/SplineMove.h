/*
 * @Description: 样条曲线插补（正运动 MSPLINE）
 *               经过指定路径点的平滑样条曲线
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

class SplineMove : public zrcsSystem::CmdNode
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;

public:
    SplineMove() : dof_(0)
    {
        std::strcpy(nodeName_, "SplineMove");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
