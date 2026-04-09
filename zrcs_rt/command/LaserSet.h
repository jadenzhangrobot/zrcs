/// @file LaserSet.h
/// @brief 设置激光参数 (功率/频率/脉宽/开关)
#pragma once

#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "controller/laser/LaserController.h"

class LaserSet : public zrcsSystem::CmdNode {
private:
    double power_ = 0;
    double frequency_ = 20000;
    double pulseWidth_ = 100;
    bool gate_ = false;

public:
    LaserSet()
    {
        std::strcpy(nodeName_, "LaserSet");
    }

    void init() override;
    void run(void) override;
    void exit(void) override {}
};
