/*
 * @Description: 延时等待命令
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"

class Wait : public zrcsSystem::CmdNode
{
private:
    double waitTimeMs_;
    uint64_t startCount_;

public:
    Wait()
    {
        std::strcpy(nodeName_, "Wait");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
