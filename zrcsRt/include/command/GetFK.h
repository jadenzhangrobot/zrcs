/*
 * @Description: 查询当前笛卡尔位姿（ABB CRobT）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
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
