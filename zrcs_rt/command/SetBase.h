/*
 * @Description: 设置基坐标系/工件坐标系（ABB wobjdata / ZMC BASE）
 */
#pragma once
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <Eigen/Dense>

class SetBase : public zrcsSystem::CmdNode
{
public:
    CMD_DEFINE(32, PARAM(X) PARAM(Y) PARAM(Z) PARAM(RX) PARAM(RY) PARAM(RZ))

    SetBase()
    {
        std::strcpy(nodeName_, "SetBase");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

