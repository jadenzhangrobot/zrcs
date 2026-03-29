/*
 * @Description: 单轴探针触发（ABB SearchL 单轴版 / 固高探针功能）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class Probe : public zrcsSystem::CmdNode
{
private:
    std::unique_ptr<Ruckig<1>> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;
    int axisId_;
    int ioIndex_;
    int bitPos_;

public:
    Probe() : axisId_(0), ioIndex_(0), bitPos_(0)
    {
        std::strcpy(nodeName_, "Probe");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
