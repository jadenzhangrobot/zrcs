#pragma once

#include "controller/ControllerInterface.h"
#include "controller/mujoco/MujocoSimulation.h"
#include "shared_memory/ShmLayout.h"

#include <memory>

namespace ZrcsHardware {

class MujocoBus : public HardwareBus {
public:
    explicit MujocoBus(std::shared_ptr<MujocoSimulation> simulation);

    void bindSharedBlock(zrcs::SharedBlock* block) override;
    void send() override;
    void receive() override;

private:
    std::shared_ptr<MujocoSimulation> simulation_;
    zrcs::SharedBlock* block_ = nullptr;
    std::unique_ptr<zrcs::ShmSPSCProducer<zrcs::MujocoIdentSampleData,
                                          zrcs::kMujocoIdentQueueCap>> sampleProducer_;
    std::unique_ptr<zrcs::ShmSPSCConsumer<zrcs::MujocoParamUpdate,
                                          zrcs::kMujocoParamQueueCap>> paramConsumer_;
    uint64_t sampleSeq_ = 0;
};

} // namespace ZrcsHardware
