/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: controllr的一些抽象接口包括电机，传感器，io等
 *
 */
#ifndef CONTROLLER_INTERFACE_H
#define CONTROLLER_INTERFACE_H
#include "ParameterRead.h"
#include <functional>
#include <iostream>
namespace HWAL {
#define pi 3.14159265358979323846
enum MotorError
 { 
         Run=0,
         maxPositionExceeded,
         minPositionExceeded,
         maxPosFollowingError,
         statusWorderror
 };
class Motor {
private:
  double max_pos = 1.0;
  double min_pos = -1.0;
  double max_vel = 1.0;
  double min_vel = -1.0;
  double max_acc = 1.0;
  double min_acc = -1.0;
  double max_pos_following_error = 1.0;
  double max_vel_following_error = 1.0;
  double pos_factor = 1.0;
  double pos_offset = 0.0;
  double home_pos = 0.0;
  double vel_factor = 1.0;
  double target_vel_ = 0;
  double target_toq_ = 0;
  double offset_vel_ = 0;
  double offset_toq_ = 0;  
  MotorConfig *motorConfig;
  MotorError motorError=Run;
public:
  int motorId;
 
  Motor(int mId, MotorConfig *motorConfig_): motorId(mId), motorConfig(motorConfig_) 
  {
    pos_factor = motorConfig->motoParas[motorId].encoderBits;
    max_pos = motorConfig->motoParas[motorId].positiveLimit;
    min_pos = motorConfig->motoParas[motorId].negativeLimit;
    pos_offset = motorConfig->motoParas[motorId].PositionOffset;
  }
  auto setTargetPos(double pos) -> void 
  {
    if (pos>max_pos)
    {
       motorError=maxPositionExceeded;
    }
    if (pos<min_pos)
    {
        motorError=minPositionExceeded;
    }
    else
    {
       setEncoderTargetPos((pos + pos_offset) * pos_factor / (2 * pi));
    }
    
  };
  auto actualPos()->double 
  {
     double pos = encoderActualPos();
     return (pos - pos_offset) / pos_factor * 2 * pi;
  }
   auto getError()->MotorError
   {
         //statusWord();
         return motorError;
   }

  // auto virtual controlWord()->std::uint16_t = 0;
  // auto virtual modeOfOperation()const->std::uint8_t = 0;
  // auto virtual targetPos(double j_val)->void;
  // auto virtual targetVel()const->double = 0;
  // auto virtual targetToq()const->double = 0;
  // auto virtual offsetVel()const->double = 0;
  // auto virtual offsetCur()const->double = 0;

  auto virtual setControlWord(std::uint16_t control_word) -> void = 0;
  // auto virtual setModeOfOperation(std::uint8_t mode)->void = 0;

  // auto virtual setTargetVel(double vel)->void = 0;
  auto virtual setTargetToq(double toq) -> int { return 0; };
  // auto virtual setOffsetVel(double vel)->void = 0;
  // auto virtual setOffsetToq(double toq)->void = 0;
  // auto virtual setErrorCode(std::int32_t code)->void = 0;

  auto virtual statusWord() -> std::uint16_t = 0;
  // auto virtual modeOfDisplay()const->std::uint8_t = 0;
  auto virtual errorCode() -> uint32_t { return 0; }
  

  virtual void setEncoderTargetPos(double position) = 0;
  virtual double encoderActualPos(void) = 0;

  virtual double actualVel() { return 0; }

  auto virtual actualToq() -> double { return 0; }
  // auto virtual actualCur()const->double = 0;
  // auto virtual actualAddlPos()const->double = 0;
  // auto virtual velDiff()const->double = 0;
  // auto virtual setVelDiff(double vel)->void = 0;

  auto virtual clearError() -> int { return 0; }
  auto virtual disable() -> int { return 0; }
  auto virtual enable() -> int { return 0; }
  auto virtual home() -> int { return 0; }
  auto virtual setModeOfOperation(std::uint8_t md) -> void {}
  auto virtual init() -> int { return 0; }

  virtual ~Motor(){};
};
class Io {
public:
  virtual int Write(std::string reg, int type, int bitPos, bool value) {
    return 1;
  }
  virtual int Read(std::string reg, int type, int bitPos) { return 1; }

  virtual ~Io(){};
};
class Sensor {

  virtual ~Sensor(){};
};
class Rtos {
public:
  virtual ~Rtos(){};
  virtual void real_task(std::function<void()> strategy) = 0;
  virtual void rtos_task_create(void) = 0;

  virtual void rtos_task_join(void) = 0;

  virtual void rtos_set_perioic(int perioic) = 0;

  virtual std::uint64_t rtos_timer_read(void) {
    uint64_t time;
    return time;
  }
};
} // namespace HWAL
#endif
