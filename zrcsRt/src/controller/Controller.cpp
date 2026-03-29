/*
 * @Description: Controller class method implementations
 */
#include "controller/Controller.h"

namespace ZrcsHardware {

void Controller::sendData()
{
    for(auto& it : axiss)
    {
        it->updateMotionCmdsToServo();
    }
    if (hardwareBus_) {
        hardwareBus_->send();
    }
}

void Controller::receiveData()
{
    if (hardwareBus_) {
        hardwareBus_->receive();
    }
    for(auto& it : axiss)
    {
        it->statusSync();
        it->cyclerun();
    }
}

} // namespace ZrcsHardware
