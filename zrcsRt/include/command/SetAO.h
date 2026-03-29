/*
 * @Description: 设置模拟输出
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"

class SetAO : public zrcsSystem::CmdNode
{
private:
    int moduleIndex_;
    int channel_;
    double value_;

public:
    SetAO()
    {
        std::strcpy(nodeName_, "SetAO");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
