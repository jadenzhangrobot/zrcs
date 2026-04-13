/*
 * @Description: 设置模拟输出
 */
#pragma once
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"

CMD_DEFINE(SetAO, 13,
    PARAM(ModuleIndex)
    PARAM(Channel)
    PARAM(Value)
)

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
