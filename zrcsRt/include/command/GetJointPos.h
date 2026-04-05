/*
 * @Description: 查询当前关节位置（ABB CJointT / 固高 GT_GetPos）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"

class GetJointPos : public zrcsSystem::CmdNode
{
public:
    GetJointPos()
    {
        std::strcpy(nodeName_, "GetJointPos");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
