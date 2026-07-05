#pragma once

#include "controller/ControllerInterface.h"
#include "controller/mujoco/MujocoSimulation.h"

#include <cstdint>
#include <memory>

namespace ZrcsHardware {

class MujocoServo : public Servo {
public:
    MujocoServo(std::shared_ptr<MujocoSimulation> simulation,
                uint32_t slaveId);
    ~MujocoServo() override = default;

    MC_SERVO_CODE setPos(int32_t pos) override;
    MC_SERVO_CODE setVel(int32_t vel) override;
    MC_SERVO_CODE setTorque(int32_t torque) override;
    MC_SERVO_CODE setMode(Cia402Mode mode) override;

    MC_SERVO_CODE setPosInTurns(double turns) override;
    MC_SERVO_CODE setVelInTurns(double turns) override;

    int32_t pos() override;
    int32_t vel() override;
    int32_t acc() override;
    int32_t torque() override;

    bool readVal(int index, double& value) override;
    bool writeVal(int index, double value) override;
    bool enable() override;
    bool disable() override;
    bool resetError() override;
    void runCycle() override;
    void emergStop() override;

private:
    std::shared_ptr<MujocoSimulation> simulation_;
    uint32_t slaveId_;
    Cia402Mode mode_ = Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION;
    int32_t torque_ = 0;
};

} // namespace ZrcsHardware
