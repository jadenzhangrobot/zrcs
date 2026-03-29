/*
 * @Description: 关节运动+位置触发IO（ABB TriggJ）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
#include <memory>
#include <cmath>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class TriggJ : public zrcsSystem::CmdNode
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;
    double trigDist_;
    int ioModule_;
    int ioBit_;
    bool ioVal_;
    bool triggered_;
    Eigen::VectorXd targetJoint_;

public:
    TriggJ() : dof_(0), trigDist_(0), ioModule_(0), ioBit_(0),
               ioVal_(false), triggered_(false)
    {
        std::strcpy(nodeName_, "TriggJ");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
