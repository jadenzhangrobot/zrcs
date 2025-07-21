#ifndef VIRTUALSERVO_H
#define VIRTUALSERVO_H
#include "controller/ControllerInterface.h"


namespace ZrcsHardware 
{
    class virtualServo : public Servo
    {
    private:
        double position_;      // 当前位置
        double velocity_;      // 当前速度
        double acceleration_;  // 当前加速度
        double torque_;        // 当前扭矩
        bool powerStatus_;     // 电源状态
        Cia402Mode mode_;      // 控制模式
        
    public:
        virtualServo(int slaveId) : position_(0.0), velocity_(0.0), acceleration_(0.0), 

                        torque_(0.0), powerStatus_(false), mode_(Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION) {}
        
        virtual ~virtualServo() {}
        
        // 必须实现的纯虚函数
        virtual MC_SERVO_CODE setPower(bool powerStatus) override {
            powerStatus_ = powerStatus;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setPos(double pos) override {
            position_ = pos;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setVel(double vel) override {
            velocity_ = vel;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setTorque(double torque) override {
            torque_ = torque;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setMode(Cia402Mode mode) override {
            mode_ = mode;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual double pos(void) override {
            return position_;
        }
        
        virtual double vel(void) override {
            return velocity_;
        }
        
        virtual double acc(void) override {
            return acceleration_;
        }
        
        virtual double torque(void) override {
            return torque_;
        }
        
        virtual bool readVal(int index, double& value) override {
            // 虚拟实现：根据索引返回相应的值
            switch(index) {
                case 0: value = position_; return true;
                case 1: value = velocity_; return true;
                case 2: value = acceleration_; return true;
                case 3: value = torque_; return true;
                default: return false;
            }
        }
        
        virtual bool writeVal(int index, double value) override {
            // 虚拟实现：根据索引设置相应的值
            switch(index) {
                case 0: position_ = value; return true;
                case 1: velocity_ = value; return true;
                case 2: acceleration_ = value; return true;
                case 3: torque_ = value; return true;
                default: return false;
            }
        }
        
        virtual MC_SERVO_CODE resetError(bool& isDone) override {
            // 虚拟实现：总是成功重置错误
            isDone = true;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual void runCycle() override {
            // 虚拟实现：模拟运行周期
            // 在实际实现中，这里会执行伺服控制循环
        }
        
        virtual void emergStop(void) override {
            // 虚拟实现：紧急停止
            velocity_ = 0.0;
            acceleration_ = 0.0;
            powerStatus_ = false;
        }
    };
}

#endif