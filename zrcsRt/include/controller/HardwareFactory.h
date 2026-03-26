#ifndef HARDWARE_FACTORY_H
#define HARDWARE_FACTORY_H

#include <memory>
#include "controller/Controller.h"
#include "controller/axisConfig.h"

#ifdef REALTIME
#include "controller/ethercat/EthercatMaster.h"
#include "controller/ethercat/EthercatMotor.h"
#include "controller/rtos/xenomai.h"
#endif

#ifdef SIMULATION
#include "controller/virtual/Coppeliasim.h"
#endif

#ifdef STANDARD
#include "controller/virtual/virtualServo.h"
#endif

#ifndef REALTIME
#include "controller/rtos/linux.h"
#endif

namespace ZrcsHardware {

class HardwareFactory {
public:
    static std::unique_ptr<Controller> createController() {
        auto config = std::make_unique<AxisConfig>("axis.xml");
        std::shared_ptr<Rtos> rtos;
        std::unique_ptr<HardwareBus> bus = nullptr;

#ifdef REALTIME
        rtos = std::make_shared<xenomai>();
        auto ethercatMaster = std::make_unique<EthercatMaster>();
        void* masterPtr = ethercatMaster.get();
        bus = std::move(ethercatMaster);
#else
        rtos = std::make_shared<Nativelinux>();
        void* masterPtr = nullptr;
#endif

        auto controller = std::make_unique<Controller>(std::move(config), rtos, std::move(bus));

        AxisConfig tempConfig("axis.xml");
        
        for (auto it = tempConfig.axisParas.begin(); it != tempConfig.axisParas.end(); ++it) {
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
            controller->addAxis(std::move(axis));
        }

        return controller;
    }
};

} // namespace ZrcsHardware

#endif // HARDWARE_FACTORY_H
