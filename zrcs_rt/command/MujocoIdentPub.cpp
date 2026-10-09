#include "command/MujocoIdentPub.h"

#if defined(SIMULATION)

#include <cstring>

MujocoIdentPub::MujocoIdentPub()
{
    std::strcpy(nodeName_, "MujocoIdentPub");
}

void MujocoIdentPub::onRegistered()
{
    sim_ = controller_ ? controller_->mujocoSimulation() : nullptr;
    if (!sim_) {
        return;
    }
    sampleProducer_ = std::make_unique<
        zrcs::ShmSPSCProducer<zrcs::MujocoIdentSampleData, zrcs::kMujocoIdentQueueCap>>(
        shm()->mujocoIdentSampleQueue);
    paramConsumer_ = std::make_unique<
        zrcs::ShmSPSCConsumer<zrcs::MujocoParamUpdate, zrcs::kMujocoParamQueueCap>>(
        shm()->mujocoParamUpdateQueue);
}

void MujocoIdentPub::init()
{
    // 通道已在 onRegistered() 建立，无一次性初始化。
}

void MujocoIdentPub::run()
{
    if (!sim_ || !sampleProducer_) {
        return;
    }

    // 消费 NRT 侧下发的动力学参数更新并应用到仿真（在 step 之前，与旧版本序一致）。
    if (paramConsumer_) {
        zrcs::MujocoParamUpdate update{};
        int applied = 0;
        while (applied < 8 && paramConsumer_->pop(update)) {
            sim_->applyParamUpdate(update);
            ++applied;
        }
    }

    // 发布识别采样：带仿真时间戳与递增序号，供 NRT 侧时间对齐。
    zrcs::MujocoIdentCommandData command{};
    const bool hasCommand = zrcs::lfl_read(shm()->mujocoIdentCommand, command);
    zrcs::MujocoIdentSampleData sample{};
    sample.seq = ++sampleSeq_;
    sim_->fillIdentificationSample(
        sample, hasCommand ? &command : nullptr, sim_->time());
    sampleProducer_->push(sample);
}

REGISTER_PERIODIC(MujocoIdentPub, OUTPUT, 90);

#endif // SIMULATION
