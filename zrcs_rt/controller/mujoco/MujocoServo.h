#pragma once

#include "controller/Servo.h"
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

    // 仿真无编码器：直接返回 MuJoCo 浮点状态，绕开 int32 编码器计数往返量化。
    // 否则反馈分辨率会受 servo.xml 的 encoderCountPerUnit 影响（=1 时量化到整单位，反馈跳变）。
    double posInTurns() override { return simulation_->position(slaveId_); }
    double velInTurns() override { return simulation_->velocity(slaveId_); }
    double accInTurns() override { return simulation_->acceleration(slaveId_); }

    int32_t pos() override;
    int32_t vel() override;
    int32_t acc() override;
    int32_t torque() override;

    bool readVal(int index, double& value) override;
    bool writeVal(int index, double value) override;
    bool enable() override;
    bool disable() override;
    bool isEnabled() override { return enabled_; }
    bool isDisabled() override { return !enabled_; }
    bool resetError() override;
    Servo::ServoState runCycle() override;
    void emergStop() override;

private:
    std::shared_ptr<MujocoSimulation> simulation_;
    uint32_t slaveId_;
    Cia402Mode mode_ = Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION;
    int32_t torque_ = 0;
    bool enabled_ = false;
};

} // namespace ZrcsHardware
