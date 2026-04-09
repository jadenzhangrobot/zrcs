/*
 * @Description: 动态设置速度限制（固高 GT_SetAxisPrm）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class SetVelLimit : public zrcsSystem::CmdNode
{
public:
    SetVelLimit()
    {
        std::strcpy(nodeName_, "SetVelLimit");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
