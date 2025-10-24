#ifndef VIRTUALSERVO_H
#define VIRTUALSERVO_H
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
        std::pmr::vector<double> positionCommand_;
      
    public:
        virtualServo(int slaveId) : position_(0.0),lastPosition_(0.0), velocity_(0.0), acceleration_(0.0), torque_(0.0), mode_(Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION)
        {
                
        }
        
        virtual ~virtualServo() {}
        
     
        virtual MC_SERVO_CODE setPos(int32_t pos) override
        {
            lastPosition_=position_;
            position_ = pos;
           
            positionCommand_.push_back(pos);
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setVel(int32_t vel) override 
        {
            velocity_ = vel;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setTorque(int32_t torque) override 
        {
            torque_ = torque;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setMode(Cia402Mode mode) override
        {
            mode_ = mode;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual int32_t pos(void) override 
        {
            return position_;
        }
        
        virtual int32_t vel(void) override
        {
            lastVelocity_=velocity_;
            velocity_=(position_-lastPosition_)*1000;           
            return velocity_;
        }
        
        virtual int32_t acc(void) override
        {
            acceleration_=(velocity_-lastVelocity_)*1000;
            return acceleration_;
        }
        
        virtual int32_t torque(void) override
        {
            return torque_;
        }
        
        virtual bool readVal(int index, double& value) override 
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
        
        virtual bool writeVal(int index, double value) override 
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
        virtual bool enable(void) override
        {
            return true;
        }
        virtual bool disable(void) override
        {
            return true;
        }
        virtual bool resetError(void) override
        {
            return true;
        }
        virtual void runCycle(void) override
        {
            // 虚拟实现：运行周期
        }
        
        
        virtual void emergStop(void) override
        {
            // 虚拟实现：紧急停止
            velocity_ = 0.0;
            acceleration_ = 0.0;
        }
    };
}
#endif