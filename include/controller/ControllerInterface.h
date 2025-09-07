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
#include <cstdint>
#include <functional>
#include <memory>
#include "axisConfig.h"
#include "global.h"
#include "common/config/parameter.h"
namespace ZrcsHardware {
class Servo
{
public:
  Servo()
  {}
  virtual~Servo() = default;

  virtual MC_SERVO_CODE setPower(bool powerStatus)=0;
  virtual MC_SERVO_CODE setPos(int32_t pos)=0;
  virtual MC_SERVO_CODE setVel(int32_t vel) { return SERVONOERROR; }
  virtual MC_SERVO_CODE setTorque(int32_t torque) { return SERVONOERROR; }
  virtual MC_SERVO_CODE setMode(Cia402Mode mode) { return SERVONOERROR; }

  virtual int32_t pos(void)=0;
  virtual int32_t vel(void)=0;
  virtual int32_t acc(void) { return 0.0; }
  virtual int32_t torque(void) { return 0.0; }

  virtual bool readVal(int index, double& value) { return false; }
  virtual bool writeVal(int index, double value) { return false; }
  

  virtual MC_SERVO_CODE resetError(bool& isDone)=0;

  virtual void runCycle(void)=0;
  virtual void emergStop(void)=0;
};

class Axis {
private:

  AxisPara *config_;
  Servo* servo_;

  uint32_t axisId_=0;
  uint32_t slaveId_=0;
  std::string axisName_="";
  double axisPos_=0;
  double axisVel_=0;
  double axisAcc_=0;
  double axisJerk_=0;
  double axisPosCmd_=0;
  double axisVelCmd_=0;
  double axisTorCmd_=0;
  double overflowCount_=0;
  MC_AXIS_STATES axisState_=MC_AXIS_STATES::mcStandstill;
  MC_ERROR_CODE axisError_=MC_ERRORCODE_GOOD;

  bool powerOn_;
  bool powerStatus_;
  bool reset_;
  bool enablePositive_;
  bool enableNegative_;
public:

  Axis(uint32_t axisId,uint32_t salveId,AxisPara *config,Servo* servo): axisId_(axisId),slaveId_(salveId),config_(config),servo_(servo)
  {
     
  }
  virtual ~Axis()
  {
      if (servo_ != nullptr) 
      {

          delete servo_;
          servo_ = nullptr;
      }
  };
  MC_ERROR_CODE setAxisId(uint32_t id)
  {
      axisId_=id;
      return MC_ERRORCODE_GOOD;
  }

  MC_ERROR_CODE setAxisName(std::string name)
  {
      axisName_=name;
      return MC_ERRORCODE_GOOD;
  }

   void setAxisPositionCmd(double axisPosCmd)
   {
         axisPosCmd_=axisPosCmd;  
   }


  double toUserUnit(double x)
  {
    return x / config_->encoder_count_per_unit_;
  }

  int32_t toEncoderUnit(double x)
  {
    return (int32_t)fixOverFlow(x * config_->encoder_count_per_unit_);
  }

  double fixOverFlow(double x)
  {
    x += overflowCount_ * INT32_MAX * 2.0;
    if (x >= INT32_MAX)
    {
      x -= INT32_MAX * 2.0;
      overflowCount_ -= 1.0;
    }
    else if (x <= - INT32_MAX)
    {
      x += INT32_MAX * 2.0;
      overflowCount_ += 1.0;
    }
    return x;
  }
  /** 对给到轴的位置，速度进行检查
   */
  bool cmdsProcessing(double frequency)
  {
    // Check motion direction and limits
    double vel_cmd = (axisPosCmd_ - axisPos_) * frequency;
    double acc_cmd = (axisVelCmd_ - axisVel_) * frequency;

    if(vel_cmd > 0 && !enablePositive_)
    {
      axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_POSITIVE;
      return false;
    } else if(vel_cmd < 0 && !enableNegative_)
    {
      axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_NEGATIVE;
      return false;
    }

    if (config_->max_vel_ <vel_cmd&&vel_cmd>config_->min_vel_)
    {
      axisError_ = MC_ERRORCODE_CMDVELOVERLIMIT;
      return false;
    }

    // if (config_->sw_acc_limit_ && std::abs(acc_cmd) > config_->acc_limit_)
    // {
    //   axisError_ = MC_ERRORCODE_CMDACCOVERLIMIT;
    //   return false;
    // }

    // if(config_->sw_range_limit_ && axisPosCmd_ > config_->pos_positive_limit_ && vel_cmd > 0)
    // {
    //   axisError_ = MC_ERRORCODE_CMDPPOSOVERLIMIT;
    //   return false;
    // }

    // if(config_->sw_range_limit_ && axisPosCmd_ < config_->pos_negative_limit_ && vel_cmd < 0)
    // {
    //   axisError_ = MC_ERRORCODE_CMDNPOSOVERLIMIT;
    //   return false;
    // }
    return true;
  }
/**
 * @brief 把轴的数据更新给具体的伺服电机
 * 
 */
  void updateMotionCmdsToServo()
  {
    if (config_->mode_ == mcServoControlModePosition)
      servo_->setPos(toEncoderUnit(axisPosCmd_));
    if (config_->mode_ == mcServoControlModeVelocity)
      servo_->setVel(toEncoderUnit(axisVelCmd_));
  }
  /**
   * @brief 将伺服电机的数据更新给轴，更新轴的位置和速度
   * 
   */
  void statusSync()
  {
    // Update servo state to axis
    axisPos_ = toUserUnit(servo_->pos() - overflowCount_ * INT32_MAX * 2.0);
    axisVel_ = toUserUnit(servo_->vel());
    axisAcc_ = toUserUnit(servo_->acc());

  }
  
  auto actualPos()->double 
  {
    return axisPos_;
  }
  auto actualVel()->double
  {
    return axisVel_;
  }
  auto actualAcc()->double
  {
    return axisAcc_;
  }
  auto actualposCmd()->double
  {
    return axisPosCmd_;
  }
  auto actualVelCmd()->double
  {
    return axisVelCmd_;
  }
  MC_AXIS_STATES getAxisState(void)
  {
    return axisState_;
  }
  MC_ERROR_CODE setAxisState(MC_AXIS_STATES setState)
  {
    switch (axisState_)
    {
    case mcStandstill:
    case mcHoming:
    case mcDiscreteMotion:
    case mcContinuousMotion:
      switch (setState)
      {
      case mcDisabled:
      case mcErrorStop:
        {
          axisState_ = setState;
          return MC_ERRORCODE_GOOD;
        }
        break;
      default:
        break;
      }
      break;
    case mcStopping:
      switch (setState)
      {
        case mcStopping:
        case mcDisabled:
        case mcErrorStop:
        case mcStandstill:
          {
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
          }
          break;
        default:
          return MC_ERRORCODE_INVALIDSTATESTIPPING;
          break;
      }
      break;
    case mcErrorStop:
      switch (setState)
      {
        case mcErrorStop:
        case mcDisabled:
        case mcStandstill:
          {
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
          }
          break;
        default:
          return MC_ERRORCODE_INVALIDSATATESTOP;
          break;
      }
      break;
    case mcDisabled:
      switch (setState)
      {
        case mcDisabled:
        case mcErrorStop:
        case mcStandstill:
          {
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
          }
          break;
        default:
          return MC_ERRORCODE_INVALIDSTATEDISABLE;
          break;
      }
      break;    
    default:
      break;
    }
    return MC_ERRORCODE_GOOD;
  }
  void setPower(bool power_on, bool enable_positive, bool enable_negative)
  {
    powerOn_ = power_on;
    enablePositive_ = enable_positive;
    enableNegative_ = enable_negative;
  }

  void resetError(bool reset)
  {
    reset_ = reset;
  }

  bool powerOn()
  {
    return powerStatus_;
  }

  bool powerTriggered()
  {
    return powerOn_;
  }

  MC_ERROR_CODE getAxisError()
  {
    return axisError_;
  }

  MC_ERROR_CODE servoErrorToAxisError(MC_SERVO_CODE error_id)
  {
    return static_cast<MC_ERROR_CODE>(0x60 + error_id);
  }

  
};
class Io {
public:
  /// 急停相关接口
  virtual bool isEmergencyStop() { return false;}  // 读取急停状态
  virtual void setEmergencyStop(bool value) {}      // 设置急停状态

  virtual bool ioRead32(int index, int bitPos) = 0;                  // 32位IO读取
  virtual bool ioRead16(int index, int bitPos) = 0;                  // 16位IO读取
  virtual bool ioRead8(int index, int bitPos) = 0;                   // 8位IO读取
  virtual void ioWrite32(int index, int bitPos, bool value) = 0;            // 32位IO写
  virtual void ioWrite16(int index, int bitPos, bool value) = 0;            // 16位IO写
  virtual void ioWrite8(int index, int bitPos, bool value) = 0;              // 8位IO写
  virtual ~Io(){};
};
class Sensor 
{

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
} 
#endif
