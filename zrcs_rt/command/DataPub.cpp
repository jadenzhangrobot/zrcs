#include "command/DataPub.h"

#include <algorithm>

DataPub::DataPub()
{
    std::strcpy(nodeName_, "DataPub");
}

void DataPub::onRegistered()
{
    producer_ = std::make_unique<zrcs::ShmSPSCProducer<zrcs::AxisFeedbackData, zrcs::kLogQueueCap>>(
        shm()->axisFeedbackQueue);
}

void DataPub::init()
{
    sequence_ = 0;
    simulationTimeNs_ = 0;
    publishFeedback();
}

void DataPub::run()
{
    constexpr uint64_t kNanosecondsPerMillisecond = 1'000'000;
    const uint64_t stepNs = static_cast<uint64_t>(
        static_cast<double>(cycletime) * static_cast<double>(kNanosecondsPerMillisecond));
    ++sequence_;
    simulationTimeNs_ += stepNs;
    publishFeedback();
}

void DataPub::publishFeedback()
{
    zrcs::AxisFeedbackData fb{};
    const size_t count = std::min(controller_->axes_.size(), zrcs::kAxisMax);

    fb.sequence = sequence_;
    fb.simulationTimeNs = simulationTimeNs_;
    fb.axisCount = static_cast<uint32_t>(count);

    for (size_t i = 0; i < count; ++i) {
        fb.position[i]    = controller_->axes_[i]->actualPos();
        fb.cmdPosition[i] = controller_->axes_[i]->actualPosCmd();
        fb.cmdVelocity[i] = controller_->axes_[i]->actualVelCmd();
        fb.velocity[i] = controller_->axes_[i]->actualVel();
        fb.servoEnabled[i] = controller_->axes_[i]->isPowerOn() ? 1 : 0;
        // fb.torque[i] — Axis 暂无 torque 接口，默认 0
    }
    if (producer_) {
        producer_->push(fb);
    }

    // 同时写 LFL（兼容 RtBridge::readLatestAxisPositions）
    zrcs::JointPosData pos{};
    for (size_t i = 0; i < count; ++i) {
        pos.pos[i] = fb.position[i];
    }
    zrcs::lfl_write(shm()->axisPositions, pos);
}

REGISTERINPUT(DataPub);
