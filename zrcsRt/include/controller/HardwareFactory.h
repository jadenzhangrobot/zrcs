#ifndef HARDWARE_FACTORY_H
#define HARDWARE_FACTORY_H

#include <memory>
#include <string>
#include "controller/Controller.h"
#include "controller/axisConfig.h"

namespace ZrcsHardware {

class HardwareFactory {
public:
    static std::unique_ptr<Controller> createController(const std::string& projectName = "");
};

} // namespace ZrcsHardware

#endif // HARDWARE_FACTORY_H
