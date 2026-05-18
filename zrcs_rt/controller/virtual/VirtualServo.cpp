/*
 * @Description: virtualServo class method implementations
 */
#include "controller/virtual/VirtualServo.h"
#include "Parameter.h"

namespace ZrcsHardware {

MC_SERVO_CODE virtualServo::setPos(int32_t pos)
{
    lastPosition_=position_;
    position_ = pos;
    return MC_SERVO_CODE::SERVONOERROR;
}

MC_SERVO_CODE virtualServo::setVel(int32_t vel)
{
    velocity_ = vel;
    return MC_SERVO_CODE::SERVONOERROR;
}

MC_SERVO_CODE virtualServo::setTorque(int32_t torque)
{
    torque_ = torque;
    return MC_SERVO_CODE::SERVONOERROR;
}

MC_SERVO_CODE virtualServo::setMode(Cia402Mode mode)
{
    mode_ = mode;
    return MC_SERVO_CODE::SERVONOERROR;
}

int32_t virtualServo::pos(void)
{
    return position_;
}

int32_t virtualServo::vel(void)
{
    lastVelocity_=velocity_;
    velocity_=(position_-lastPosition_)*1000/cycletime;
    return velocity_;
}

int32_t virtualServo::acc(void)
{
    acceleration_=(velocity_-lastVelocity_)*1000/cycletime;
    return acceleration_;
}

int32_t virtualServo::torque(void)
{
    return torque_;
}

bool virtualServo::readVal(int index, double& value)
{
    // 虚拟实现：根据索引返回相应的值
    switch(index)
    {
        case 0: value = position_; return true;
        case 1: value = velocity_; return true;
        case 2: value = acceleration_; return true;
        case 3: value = torque_; return true;
        default: return false;
    }
}

bool virtualServo::writeVal(int index, double value)
{
    // 虚拟实现：根据索引设置相应的值
    switch(index)
    {
        case 0: position_ = value; return true;
        case 1: velocity_ = value; return true;
        case 2: acceleration_ = value; return true;
        case 3: torque_ = value; return true;
        default: return false;
    }
}

bool virtualServo::enable(void)
{
    return true;
}

bool virtualServo::disable(void)
{
    return true;
}

bool virtualServo::resetError(void)
{
    return true;
}

void virtualServo::runCycle(void)
{
    // 虚拟实现：运行周期
}

void virtualServo::emergStop(void)
{
    // 虚拟实现：紧急停止
    velocity_ = 0.0;
    acceleration_ = 0.0;
}

} // namespace ZrcsHardware
