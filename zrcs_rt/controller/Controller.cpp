/**
 * @file Controller.cpp
 * @brief Controller 类方法实现 — RT 周期的数据收发与轴状态同步
 */
#include "controller/Controller.h"

namespace ZrcsHardware {

void Controller::sendData()
{
    for (auto& axis : axes_) 
    {
        if (!axis->cmdsProcessing(1000.0 / cycletime))
        {
            axis->setAxisState(Axis::AxisState::ErrorStop);
            continue;
        }
        axis->updateMotionCmdsToServo();
    }
#if defined(REALTIME) || defined(SIMULATION)
    if (hardwareBus_)
    {
        hardwareBus_->send();
    }
#endif
}

void Controller::receiveData()
{
#if defined(REALTIME) || defined(SIMULATION)
    if (hardwareBus_)
    {
        hardwareBus_->receive();
    }
#endif
    for (auto& axis : axes_)
    {
        axis->cyclerun();
    }
}

} // namespace ZrcsHardware
