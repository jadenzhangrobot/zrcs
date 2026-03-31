/*
 * @Description: Controller class method implementations
 */
#include "controller/Controller.h"

namespace ZrcsHardware {

void Controller::sendData()
{
    try {
        for(auto& it : axiss)
        {
            it->updateMotionCmdsToServo();
        }
        if (hardwareBus_) {
            hardwareBus_->send();
        }
    } catch (const std::exception& e) {
        ERROR_PRINT("Controller::sendData 异常: %s\n", e.what());
    }
}

void Controller::receiveData()
{
    try {
        if (hardwareBus_) {
            hardwareBus_->receive();
        }
        for(auto& it : axiss)
        {
            it->statusSync();
            it->cyclerun();
        }
    } catch (const std::exception& e) {
        ERROR_PRINT("Controller::receiveData 异常: %s\n", e.what());
    }
}

} // namespace ZrcsHardware
