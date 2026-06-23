#include "controller/mujoco/MujocoBus.h"

#include <stdexcept>

namespace ZrcsHardware {

MujocoBus::MujocoBus(std::shared_ptr<MujocoSimulation> simulation)
    : simulation_(std::move(simulation))
{
    if (!simulation_) {
        throw std::runtime_error("MujocoBus requires a simulation instance");
    }
}

void MujocoBus::bindSharedBlock(zrcs::SharedBlock* block)
{
    block_ = block;
    if (!block_) {
        sampleProducer_.reset();
        paramConsumer_.reset();
        return;
    }
    sampleProducer_ = std::make_unique<
        zrcs::ShmSPSCProducer<zrcs::MujocoIdentSampleData, zrcs::kMujocoIdentQueueCap>>(
        block_->mujocoIdentSampleQueue);
    paramConsumer_ = std::make_unique<
        zrcs::ShmSPSCConsumer<zrcs::MujocoParamUpdate, zrcs::kMujocoParamQueueCap>>(
        block_->mujocoParamUpdateQueue);
}

void MujocoBus::send()
{
    if (paramConsumer_) {
        zrcs::MujocoParamUpdate update{};
        int applied = 0;
        while (applied < 8 && paramConsumer_->pop(update)) {
            simulation_->applyParamUpdate(update);
            ++applied;
        }
    }

    simulation_->step();

    if (sampleProducer_) {
        zrcs::MujocoIdentCommandData command{};
        const bool hasCommand = block_ &&
                                zrcs::lfl_read(block_->mujocoIdentCommand, command);
        zrcs::MujocoIdentSampleData sample{};
        sample.seq = ++sampleSeq_;
        simulation_->fillIdentificationSample(
            sample, hasCommand ? &command : nullptr, simulation_->time());
        sampleProducer_->push(sample);
    }
}

void MujocoBus::receive()
{
    simulation_->receive();
}

} // namespace ZrcsHardware
