#include "command/DataPub.h"

void DataPub::init()
{
    producer_ = new zrcs::ShmSPSCProducer<zrcs::AxisFeedbackData, zrcs::kLogQueueCap>(
        shm()->axisFeedbackQueue);
}

void DataPub::run()
{
    zrcs::AxisFeedbackData fb{};
    const size_t count = controller_->axiss.size();
    for (size_t i = 0; i < count; ++i) {
        fb.position[i]    = controller_->axiss[i]->actualPos();
        fb.cmdPosition[i] = controller_->axiss[i]->actualposCmd();
        fb.velocity[i]    = controller_->axiss[i]->actualVel();
        // fb.torque[i] — Axis 暂无 torque 接口，默认 0
    }
    producer_->push(fb);

    // 同时写 LFL（兼容 RtBridge::readLatestAxisPositions）
    zrcs::JointPosData pos{};
    for (size_t i = 0; i < count; ++i) {
        pos.pos[i] = fb.position[i];
    }
    zrcs::lfl_write(shm()->axisPositions, pos);
}

REGISTERINPUT(DataPub);
