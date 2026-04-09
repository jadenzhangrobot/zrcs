/*
 * @Description: 奇异区域处理策略（ABB SingArea）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class SingArea : public zrcsSystem::CmdNode
{
public:
    SingArea()
    {
        std::strcpy(nodeName_, "SingArea");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
