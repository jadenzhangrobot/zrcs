#pragma once
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"  // IWYU pragma: keep — provides REGISTERINPUT macro

class DataPub : public zrcsSystem::InputNode
{
public:
    void init() override;
    void run() override;
};
