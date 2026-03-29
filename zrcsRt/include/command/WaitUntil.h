/*
 * @Description: 等待条件满足（通用条件等待）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <cmath>

class WaitUntil : public zrcsSystem::CmdNode
{
private:
    int condType_;    // 0=轴到位, 1=IO条件
    double param1_;
    double param2_;
    double timeoutMs_;
    uint64_t startCount_;

public:
    WaitUntil()
    {
        std::strcpy(nodeName_, "WaitUntil");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};
