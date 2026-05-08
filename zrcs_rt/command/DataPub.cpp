#include "command/DataPub.h"


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
   
}

void DataPub::run()
{
    zrcs::AxisFeedbackData fb{};
    const size_t count = controller_->axiss.size();
    for (size_t i = 0; i < count; ++i) {
        fb.position[i]    = controller_->axiss[i]->actualPos();
        fb.cmdPosition[i] = controller_->axiss[i]->actualposCmd();
        fb.cmdVelocity[i] = controller_->axiss[i]->actualVelCmd();
        fb.velocity[i] = controller_->axiss[i]->actualVel();


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
