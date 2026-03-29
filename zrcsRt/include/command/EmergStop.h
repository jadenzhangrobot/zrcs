/*
 * @Description: 紧急停止命令 — 立即断使能所有轴
 */
#pragma once
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"

class EmergStop : public zrcsSystem::CmdNode
{
public:
    EmergStop()
    {
        std::strcpy(nodeName_, "EmergStop");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
