/*
 * @Description: 查询当前笛卡尔位姿（ABB CRobT）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <Eigen/Dense>

class GetFK : public zrcsSystem::CmdNode
{
public:
    GetFK()
    {
        std::strcpy(nodeName_, "GetFK");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
