/*
 * @Description: 通用单轴相对定位（带可选速度/加速度/加加速度参数）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveRel : public zrcsSystem::CmdNode
{
private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;
    int axisId_;

public:
    MoveRel() : otg_(cycletime * 0.001)
    {
        std::strcpy(nodeName_, "MoveRel");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
