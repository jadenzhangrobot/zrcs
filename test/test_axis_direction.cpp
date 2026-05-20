#include "controller/ControllerInterface.h"

#include <cassert>
#include <cstdint>
#include <memory>

namespace {

class TestServo : public ZrcsHardware::Servo {
public:
    MC_SERVO_CODE setPos(int32_t pos) override
    {
        position_ = pos;
        return SERVONOERROR;
    }

    MC_SERVO_CODE setVel(int32_t vel) override
    {
        velocity_ = vel;
        return SERVONOERROR;
    }

    MC_SERVO_CODE setTorque(int32_t torque) override
    {
        torque_ = torque;
        return SERVONOERROR;
    }

    MC_SERVO_CODE setMode(Cia402Mode mode) override
    {
        mode_ = mode;
        return SERVONOERROR;
    }

    int32_t pos() override { return position_; }
    int32_t vel() override { return velocity_; }
    int32_t acc() override { return acceleration_; }
    int32_t torque() override { return torque_; }
    void emergStop() override { velocity_ = 0; }
    void runCycle() override {}

private:
    int32_t position_ = 0;
    int32_t velocity_ = 0;
    int32_t acceleration_ = 0;
    int32_t torque_ = 0;
    Cia402Mode mode_ = Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION;
};

} // namespace

int main()
{
    ZrcsHardware::AxisPara axisConfig;
    axisConfig.axisId = 0;
    axisConfig.axisName = "dual";
    axisConfig.maxPosDiff = 0.001;

    ZrcsHardware::Axis axis(0, new ZrcsHardware::AxisPara(axisConfig));

    ZrcsHardware::ServoPara forwardServo;
    forwardServo.slaveId = 0;
    forwardServo.encoderCountPerUnit = 1000;
    forwardServo.direction = 1;

    ZrcsHardware::ServoPara reverseServo;
    reverseServo.slaveId = 1;
    reverseServo.encoderCountPerUnit = 1000;
    reverseServo.direction = -1;

    axis.pushServo(std::make_unique<TestServo>(), forwardServo);
    axis.pushServo(std::make_unique<TestServo>(), reverseServo);

    assert(axis.toEncoderUnit(2.5, forwardServo) == 2500);
    assert(axis.toEncoderUnit(2.5, reverseServo) == -2500);
    assert(axis.toUserUnit(2500, forwardServo) == 2.5);
    assert(axis.toUserUnit(-2500, reverseServo) == 2.5);

    axis.setAxisPositionCmd(2.5);
    axis.updateMotionCmdsToServo();
    axis.statusSync();

    assert(axis.actualPos() == 2.5);
    assert(axis.getAxisError() == MC_ERRORCODE_GOOD);

    return 0;
}
