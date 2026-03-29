#ifndef HARDWARE_FACTORY_H
#define HARDWARE_FACTORY_H

#include <memory>
#include "controller/Controller.h"
#include "controller/axisConfig.h"

namespace ZrcsHardware {

class HardwareFactory {
public:
    static std::unique_ptr<Controller> createController();
};

} // namespace ZrcsHardware

#endif // HARDWARE_FACTORY_H
