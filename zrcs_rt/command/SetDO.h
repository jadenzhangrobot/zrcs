/*
 * @Description: 设置数字输出（单bit）
 */
#pragma once
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"

CMD_DEFINE(SetDO, 11,
    PARAM(ModuleIndex)
    PARAM(BitPos)
    PARAM(Value)
)

class SetDO : public zrcsSystem::CmdNode
{
private:
    int moduleIndex_;
    int bitPos_;
    bool value_;

public:
    SetDO()
    {
        std::strcpy(nodeName_, "SetDO");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
