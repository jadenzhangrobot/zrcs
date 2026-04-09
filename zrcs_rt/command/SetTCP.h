/*
 * @Description: 设置工具坐标系（ABB tooldata）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <Eigen/Dense>

class SetTCP : public zrcsSystem::CmdNode
{
public:
    SetTCP()
    {
        std::strcpy(nodeName_, "SetTCP");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
