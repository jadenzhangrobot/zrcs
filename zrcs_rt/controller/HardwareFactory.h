#pragma once

#include <memory>
#include <string>
#include "controller/Controller.h"
#include "controller/AxisConfig.h"

namespace ZrcsHardware {

class HardwareFactory {
public:
    static std::unique_ptr<Controller> createController(const std::string& projectName = "");
};

} // namespace ZrcsHardware

