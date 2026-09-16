#pragma once

#include "controller/ControllerInterface.h"
#include <cstdint>
#include <iomanip>
#include <vector>
#include <memory_resource>


namespace ZrcsHardware 
{
    class virtualServo : public Servo
    {
    private:
        int32_t position_=0;      // 当前位置
        int32_t lastPosition_=0;      // 上个周期位置
        int32_t velocity_=0;      // 当前速度
        int32_t lastVelocity_=0;      // 上个周期速度
        int32_t acceleration_=0;  // 当前加速度
        int32_t torque_=0;        // 当前扭矩
        Cia402Mode mode_;      // 控制模式
        bool enabled_ = false;      // 使能状态

    public:
        virtualServo(int slaveId) : position_(0.0),lastPosition_(0.0), velocity_(0.0), acceleration_(0.0), torque_(0.0), mode_(Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION)
        {

        }

        virtual ~virtualServo() {}


        virtual MC_SERVO_CODE setPos(int32_t pos) override;
        virtual MC_SERVO_CODE setVel(int32_t vel) override;
        virtual MC_SERVO_CODE setTorque(int32_t torque) override;
        virtual MC_SERVO_CODE setMode(Cia402Mode mode) override;
        virtual int32_t pos(void) override;
        virtual int32_t vel(void) override;
        virtual int32_t acc(void) override;
        virtual int32_t torque(void) override;
        virtual bool readVal(int index, double& value) override;
        virtual bool writeVal(int index, double value) override;
        virtual bool enable(void) override;
        virtual bool disable(void) override;
        virtual bool isEnabled(void) override { return enabled_; }
        virtual bool isDisabled(void) override { return !enabled_; }
        virtual bool resetError(void) override;
        virtual Servo::ServoState runCycle(void) override;
        virtual void emergStop(void) override;
    };
}
