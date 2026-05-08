#pragma once
#include "system/node/BaseNodeInterface.h"
#include <memory>
#include "system/node/NodeFactory.h"  // IWYU pragma: keep — provides REGISTERINPUT macro

class DataPub : public zrcsSystem::InputNode
{
public:
    void init() override;
    void run() override;
    DataPub() ;
    ~DataPub() override = default;
protected:
    void onRegistered() override;
private:

    std::unique_ptr<zrcs::ShmSPSCProducer<zrcs::AxisFeedbackData, zrcs::kLogQueueCap>> producer_;
};
