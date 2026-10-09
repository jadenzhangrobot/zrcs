#pragma once

#include "system/node/BaseNodeInterface.h"

// OUTPUT 阶段：在 PlcLogicNode 扫描后读取 PLC 输出，映射到 RT 变量/IO。
class PlcOutputNode : public zrcsSystem::PeriodicNode {
public:
    PlcOutputNode();
    void init() override;
    void run() override;

protected:
    void onRegistered() override;

private:
    bool lastRunning_ = false;
};
