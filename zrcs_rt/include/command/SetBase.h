/*
 * @Description: 设置基坐标系/工件坐标系（ABB wobjdata / ZMC BASE）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
#include <Eigen/Dense>

class SetBase : public zrcsSystem::CmdNode
{
public:
    SetBase()
    {
        std::strcpy(nodeName_, "SetBase");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
