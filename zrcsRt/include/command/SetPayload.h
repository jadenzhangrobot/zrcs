/*
 * @Description: 设置末端负载（ABB GripLoad）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"

class SetPayload : public zrcsSystem::CmdNode
{
public:
    SetPayload()
    {
        std::strcpy(nodeName_, "SetPayload");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
