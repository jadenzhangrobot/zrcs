/*
 * @Description: 直线搜索运动（ABB SearchL）— 运动中检测IO后停止记录位置
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

class SearchL : public zrcsSystem::CmdNode
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;
    int ioModule_;
    int ioBit_;

public:
    SearchL() : dof_(0), ioModule_(0), ioBit_(0)
    {
        std::strcpy(nodeName_, "SearchL");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
