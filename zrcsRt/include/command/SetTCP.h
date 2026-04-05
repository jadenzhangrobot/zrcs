/*
 * @Description: 设置工具坐标系（ABB tooldata）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
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
