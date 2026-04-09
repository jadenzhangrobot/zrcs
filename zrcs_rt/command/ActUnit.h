/*
 * @Description: 激活机械单元（使能一组关联轴）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class ActUnit : public zrcsSystem::CmdNode
{
private:
    int unitId_;

public:
    ActUnit()
    {
        std::strcpy(nodeName_, "ActUnit");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
