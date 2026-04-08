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
#include <cmath>
#include <cstdint>
#include <functional>
#include "axisConfig.h"
#include "global.h"
#include "config/parameter.h"
#include "sharedMemory/sharedData.h"
namespace ZrcsHardware {
class Servo
{
public:
  Servo()
  {}
  virtual~Servo() = default;

  virtual bool enable(void)
  {
    return true;
  }
  virtual bool disable(void)
  {
    return true;
  }
  //virtual MC_SERVO_CODE setPower(bool powerStatus)=0;
  virtual MC_SERVO_CODE setPos(int32_t pos)=0;
  virtual MC_SERVO_CODE setVel(int32_t vel) = 0;
  virtual MC_SERVO_CODE setTorque(int32_t torque) = 0;
  virtual MC_SERVO_CODE setMode(Cia402Mode mode) = 0;

  virtual int32_t pos(void)=0;
  virtual int32_t vel(void)=0;
  virtual int32_t acc(void)=0;
  virtual int32_t torque(void) = 0;

  virtual bool readVal(int index, double& value) { return false; }
  virtual bool writeVal(int index, double value) { return false; }
  

  virtual bool resetError()
  {
    return true;
  }
  virtual void emergStop(void)=0;
  virtual void runCycle(void)=0;
};

class Axis {
private:

  AxisPara *config_;
  std::vector<std::unique_ptr<Servo>> servo_;

  uint32_t axisId_=0;
  uint32_t slaveId_=0;
  std::string axisName_="";
  double axisPos_=0;
  double axisVel_=0;
  double axisAcc_=0;
  double axisJerk_=0;
  double axisPosCmd_=0;
  double lastAxisPosCmd_=0;
  double axisVelCmd_=0;
  double lastAxisVelCmd_=0;
  double axisTorCmd_=0;
  int32_t overflowCount_=0;
  MC_AXIS_STATES axisState_=MC_AXIS_STATES::mcStandstill;
  MC_ERROR_CODE axisError_=MC_ERRORCODE_GOOD;

  bool powerOn_=false;
  bool powerStatus_=false;
  bool reset_=false;
  bool enablePositive_=true;
  bool enableNegative_=true;
public:

  Axis(uint32_t axisId,uint32_t salveId,AxisPara *config): axisId_(axisId),slaveId_(salveId),config_(config)
  {
     
  }
  virtual ~Axis()
  {
      servo_.clear();
      delete config_;
  };
  void pushServo(std::unique_ptr<Servo> servo)
  {
    servo_.push_back(std::move(servo));
  }
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

   void syncCmdHistory()
   {
         lastAxisPosCmd_ = axisPosCmd_;
         lastAxisVelCmd_ = 0.0;
   }


  double toUserUnit(double x)
  {
    return x / config_->encoderCountPerUnit;
  }

  int32_t toEncoderUnit(double x)
  {
    return (int32_t)fixOverFlow(x * config_->encoderCountPerUnit);
  }

  double fixOverFlow(double x);
  /** 对给到轴的位置，速度进行检查
   */
  bool cmdsProcessing(double frequency);
/**
 * @brief 把轴的数据更新给具体的伺服电机
 *
 */
  void updateMotionCmdsToServo();
  /**
   * @brief 将伺服电机的数据更新给轴，更新轴的位置和速度
   *
   */
  void statusSync();
  
  auto actualPos()->double
  {
    return axisPos_ - zeroOffset_;
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
  MC_ERROR_CODE setAxisState(MC_AXIS_STATES setState);

  MC_ERROR_CODE cyclerun();
  bool resetError(void);

  bool powerOn();

  bool powerOff();
  void setModeOfOperation();
  MC_ERROR_CODE getAxisError()
  {
    return axisError_;
  }

  MC_ERROR_CODE servoErrorToAxisError(MC_SERVO_CODE error_id)
  {
    return static_cast<MC_ERROR_CODE>(0x60 + error_id);
  }

  double getMaxVelocity()
  {
    return config_->maxVel;
  }
  double getMaxAcceleration()
  {
    return config_->maxAcc;
  }
  double getMaxJerk()
  {
    return config_->maxJerk;
  }

  // --- 零点偏移 ---
  void setZeroOffset(double offset) 
  { zeroOffset_ = offset+zeroOffset_; }
  double getZeroOffset() const { return zeroOffset_; }

  // --- 动态限位修改 ---
  void setPosLimits(double posLimit, double negLimit)
  {
    config_->posPositiveLimit = posLimit;
    config_->posNegativeLimit = negLimit;
  }

  void setVelLimits(double maxVel, double maxAcc, double maxJerk)
  {
    config_->maxVel = maxVel;
    config_->maxAcc = maxAcc;
    config_->maxJerk = maxJerk;
  }

  void setModeOfOperation(Cia402Mode mode);

private:
  double zeroOffset_ = 0;  // 用户零点偏移
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

  virtual void rtos_task_stop() { rtos_task_join(); }

  virtual void rtos_set_perioic(int perioic) = 0;

  virtual std::uint64_t rtos_timer_read(void) {
    uint64_t time = 0;
    return time;
  }
};

class HardwareBus {
public:
    virtual ~HardwareBus() = default;
    virtual void send() = 0;
    virtual void receive() = 0;
};
} 
#endif
