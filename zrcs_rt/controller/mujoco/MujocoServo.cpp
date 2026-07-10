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
                         uint32_t slaveId)
    : simulation_(std::move(simulation)),
      slaveId_(slaveId)
{
    if (!simulation_) {
        throw std::runtime_error("MujocoServo requires a simulation instance");
    }
}

MC_SERVO_CODE MujocoServo::setPos(int32_t position)
{
    simulation_->setTargetPosition(slaveId_, encoderCountToTurns(position));
    return SERVONOERROR;
}

MC_SERVO_CODE MujocoServo::setVel(int32_t velocity)
{
    simulation_->setTargetVelocity(slaveId_, encoderCountToTurns(velocity));
    return SERVONOERROR;
}

MC_SERVO_CODE MujocoServo::setTorque(int32_t torque)
{
    torque_ = torque;
    simulation_->setTargetTorque(slaveId_, static_cast<double>(torque));
    return SERVONOERROR;
}

MC_SERVO_CODE MujocoServo::setPosInTurns(double turns)
{
    simulation_->setTargetPosition(slaveId_, turns);
    return SERVONOERROR;
}

MC_SERVO_CODE MujocoServo::setVelInTurns(double turns)
{
    simulation_->setTargetVelocity(slaveId_, turns);
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
    return turnsToEncoderCount(simulation_->position(slaveId_));
}

int32_t MujocoServo::vel()
{
    return turnsToEncoderCount(simulation_->velocity(slaveId_));
}

int32_t MujocoServo::acc()
{
    return turnsToEncoderCount(simulation_->acceleration(slaveId_));
}

int32_t MujocoServo::torque()
{
    return clampToInt32(simulation_->appliedTorque(slaveId_));
}

bool MujocoServo::readVal(int index, double& value)
{
    switch (index) {
    case 0:
        value = encoderCountToTurns(pos());
        return true;
    case 1:
        value = encoderCountToTurns(vel());
        return true;
    case 2:
        value = encoderCountToTurns(acc());
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

} // namespace ZrcsHardware
