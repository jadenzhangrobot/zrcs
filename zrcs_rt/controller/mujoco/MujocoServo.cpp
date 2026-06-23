#include "controller/mujoco/MujocoServo.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace ZrcsHardware {
namespace {

int32_t clampToInt32(double value)
{
    const double hi = static_cast<double>(std::numeric_limits<int32_t>::max());
    const double lo = static_cast<double>(std::numeric_limits<int32_t>::min());
    if (value > hi) {
        return std::numeric_limits<int32_t>::max();
    }
    if (value < lo) {
        return std::numeric_limits<int32_t>::min();
    }
    return static_cast<int32_t>(std::llround(value));
}

} // namespace

MujocoServo::MujocoServo(std::shared_ptr<MujocoSimulation> simulation,
                         uint32_t slaveId,
                         ServoPara servoConfig)
    : simulation_(std::move(simulation)),
      slaveId_(slaveId),
      servoConfig_(std::move(servoConfig))
{
    if (!simulation_) {
        throw std::runtime_error("MujocoServo requires a simulation instance");
    }
    if (servoConfig_.encoderCountPerUnit == 0) {
        servoConfig_.encoderCountPerUnit = 1;
    }
}

MC_SERVO_CODE MujocoServo::setPos(int32_t position)
{
    simulation_->setTargetPosition(slaveId_, encoderToUser(position));
    return SERVONOERROR;
}

MC_SERVO_CODE MujocoServo::setVel(int32_t velocity)
{
    simulation_->setTargetVelocity(slaveId_, encoderToUser(velocity));
    return SERVONOERROR;
}

MC_SERVO_CODE MujocoServo::setTorque(int32_t torque)
{
    torque_ = torque;
    simulation_->setTargetTorque(slaveId_, static_cast<double>(torque));
    return SERVONOERROR;
}

MC_SERVO_CODE MujocoServo::setMode(Cia402Mode mode)
{
    mode_ = mode;
    simulation_->setMode(slaveId_, mode);
    return SERVONOERROR;
}

int32_t MujocoServo::pos()
{
    return userToEncoder(simulation_->position(slaveId_));
}

int32_t MujocoServo::vel()
{
    return userToEncoder(simulation_->velocity(slaveId_));
}

int32_t MujocoServo::acc()
{
    return userToEncoder(simulation_->acceleration(slaveId_));
}

int32_t MujocoServo::torque()
{
    return userToEncoder(simulation_->appliedTorque(slaveId_));
}

bool MujocoServo::readVal(int index, double& value)
{
    switch (index) {
    case 0:
        value = pos();
        return true;
    case 1:
        value = vel();
        return true;
    case 2:
        value = acc();
        return true;
    case 3:
        value = torque();
        return true;
    default:
        return false;
    }
}

bool MujocoServo::writeVal(int index, double value)
{
    switch (index) {
    case 0:
        return setPos(clampToInt32(value)) == SERVONOERROR;
    case 1:
        return setVel(clampToInt32(value)) == SERVONOERROR;
    case 2:
        return true;
    case 3:
        return setTorque(clampToInt32(value)) == SERVONOERROR;
    default:
        return false;
    }
}

bool MujocoServo::enable()
{
    simulation_->setEnabled(slaveId_, true);
    return true;
}

bool MujocoServo::disable()
{
    simulation_->setEnabled(slaveId_, false);
    simulation_->holdCurrentPosition(slaveId_);
    return true;
}

bool MujocoServo::resetError()
{
    return true;
}

void MujocoServo::runCycle()
{
}

void MujocoServo::emergStop()
{
    simulation_->holdCurrentPosition(slaveId_);
    simulation_->setTargetVelocity(slaveId_, 0.0);
    torque_ = 0;
    simulation_->setTargetTorque(slaveId_, 0.0);
}

double MujocoServo::encoderToUser(int32_t value) const
{
    return (static_cast<double>(value) * servoConfig_.direction) /
           static_cast<double>(servoConfig_.encoderCountPerUnit);
}

int32_t MujocoServo::userToEncoder(double value) const
{
    return clampToInt32(value *
                        static_cast<double>(servoConfig_.encoderCountPerUnit) *
                        servoConfig_.direction);
}

} // namespace ZrcsHardware
