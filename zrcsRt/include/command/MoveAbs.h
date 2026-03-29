/*
 * @Description: 通用单轴绝对定位（带可选速度/加速度/加加速度参数）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveAbs : public zrcsSystem::CmdNode
{
private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;
    int axisId_;

public:
    MoveAbs() : otg_(cycletime * 0.001)
    {
        std::strcpy(nodeName_, "MoveAbs");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
