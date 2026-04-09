/*
 * @Description: HardwareFactory class method implementations
 */
#include "controller/HardwareFactory.h"
#include "config/ProjectConfig.h"

#ifdef REALTIME
#include "controller/ethercat/EthercatMaster.h"
#include "controller/ethercat/EthercatMotor.h"
#include "controller/rtos/Xenomai.h"
#endif

#ifdef SIMULATION
#include "controller/virtual/CoppeliaSim.h"
#endif

#ifdef STANDARD
#include "controller/virtual/VirtualServo.h"
#endif

#ifndef REALTIME
#include "controller/rtos/Linux.h"
#endif

namespace ZrcsHardware {

std::unique_ptr<Controller> HardwareFactory::createController(const std::string& projectName) {
    auto config = std::make_unique<AxisConfig>(zrcs::ProjectConfig::prefixedFilename(projectName, "axis.xml"));
    std::shared_ptr<Rtos> rtos;
    std::unique_ptr<HardwareBus> bus = nullptr;

#ifdef REALTIME
    rtos = std::make_shared<xenomai>();
    auto ethercatMaster = std::make_unique<EthercatMaster>(zrcs::ProjectConfig::prefixedFilename(projectName, "ethercat.xml"));
    void* masterPtr = ethercatMaster.get();
    bus = std::move(ethercatMaster);
#else
    rtos = std::make_shared<Nativelinux>();
    void* masterPtr = nullptr;
#endif

    // 在 config 被 move 之前，先用它创建所有 Axis
    std::vector<std::unique_ptr<Axis>> axes;
    for (auto it = config->axisParas.begin(); it != config->axisParas.end(); ++it) {
        auto axis = std::make_unique<Axis>(it->axisId, it->slaveId, new AxisPara(*it));

#ifdef REALTIME
        if (masterPtr) {
            axis->pushServo(std::make_unique<EthercatMotor>(it->slaveId, static_cast<EthercatMaster*>(masterPtr)));
        }
#endif

#ifdef SIMULATION
        axis->pushServo(std::make_unique<Coppeliasim>(it->slaveId));
#endif

#ifdef STANDARD
        axis->pushServo(std::make_unique<virtualServo>(it->slaveId));
#endif
        axes.push_back(std::move(axis));
    }

    auto controller = std::make_unique<Controller>(std::move(config), rtos, std::move(bus));

    for (auto& axis : axes) {
        controller->addAxis(std::move(axis));
    }

    return controller;
}

} // namespace ZrcsHardware
