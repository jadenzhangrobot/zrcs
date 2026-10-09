#pragma once

#include "system/node/BaseNodeInterface.h"

// INPUT 阶段：采集 RT 变量，写入 PLC 输入；变量映射集中在 run() 中。
class PlcInputNode : public zrcsSystem::PeriodicNode {
public:
    PlcInputNode();
    void init() override;
    void run() override;
};
