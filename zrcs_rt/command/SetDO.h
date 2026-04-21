/*
 * @Description: 设置数字输出（单bit）
 */
#pragma once
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"

class SetDO : public zrcsSystem::CmdNode
{
public:
    CMD_DEFINE(11, PARAM(ModuleIndex) PARAM(BitPos) PARAM(Value))

    SetDO()
    {
        std::strcpy(nodeName_, "SetDO");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;

private:
    int moduleIndex_;
    int bitPos_;
    bool value_;
};

