/*
 * @Description: 停用机械单元（失能一组关联轴）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"

class DeactUnit : public zrcsSystem::CmdNode
{
private:
    int unitId_;

public:
    DeactUnit()
    {
        std::strcpy(nodeName_, "DeactUnit");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
