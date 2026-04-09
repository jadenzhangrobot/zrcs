/*
 * @Description: 设置全局速度倍率 (0.0 ~ 1.0)
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class SetOverride : public zrcsSystem::CmdNode
{
private:
    double value_;

public:
    SetOverride()
    {
        std::strcpy(nodeName_, "SetOverride");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
