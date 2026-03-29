/*
 * @Description: 多轴回零命令
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class Movehome : public zrcsSystem::CmdNode
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;

public:
    Movehome() : dof_(0)
    {
        std::strcpy(nodeName_, "Movehome");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
