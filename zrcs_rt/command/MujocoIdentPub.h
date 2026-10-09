#pragma once

#if defined(SIMULATION)

#include "system/node/BaseNodeInterface.h"
#include "controller/mujoco/MujocoSimulation.h"
#include "shared_memory/ShmLayout.h"
#include <cstdint>
#include <memory>
#include "system/node/NodeFactory.h"  // IWYU pragma: keep — provides REGISTEROUTPUT macro

/**
 * @brief MuJoCo 仿真 GUI 识别通道节点（对齐 DataPub 的数据发布模式）。
 *
 * 每周期：消费 NRT 侧（MujocoIdentifyWorker）下发的 MujocoParamUpdate 并应用到仿真，
 * 再把带时间戳/序号的识别采样 MujocoIdentSampleData 写入 mujocoIdentSampleQueue。
 * 仿真步进（simulation_->step）仍由传输层 MujocoBus::send 负责，本节点只做数据交换。
 */
class MujocoIdentPub : public zrcsSystem::PeriodicNode
{
public:
    MujocoIdentPub();
    ~MujocoIdentPub() override = default;
    void init() override;
    void run() override;
protected:
    void onRegistered() override;
private:
    std::shared_ptr<ZrcsHardware::MujocoSimulation> sim_;
    std::unique_ptr<zrcs::ShmSPSCProducer<zrcs::MujocoIdentSampleData,
                                          zrcs::kMujocoIdentQueueCap>> sampleProducer_;
    std::unique_ptr<zrcs::ShmSPSCConsumer<zrcs::MujocoParamUpdate,
                                          zrcs::kMujocoParamQueueCap>> paramConsumer_;
    uint64_t sampleSeq_ = 0;
};

#endif // SIMULATION
