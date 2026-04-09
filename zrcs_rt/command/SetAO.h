/*
 * @Description: 设置模拟输出
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class SetAO : public zrcsSystem::CmdNode
{
private:
    int moduleIndex_;
    int channel_;
    double value_;

public:
    SetAO()
    {
        std::strcpy(nodeName_, "SetAO");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
