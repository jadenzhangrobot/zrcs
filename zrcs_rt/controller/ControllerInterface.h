/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: controller hardware abstraction interfaces
 */
#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "AxisConfig.h"
#include "Global.h"
#include "config/Parameter.h"
#include "shared_memory/ShmLayout.h"

namespace ZrcsHardware {

class Servo
{
public:
  Servo() = default;
  virtual ~Servo() = default;

  virtual bool enable()
  {
    return true;
  }
  virtual bool disable()
  {
    return true;
  }

  virtual MC_SERVO_CODE setPos(int32_t pos)=0;
  virtual MC_SERVO_CODE setVel(int32_t vel) = 0;
  virtual MC_SERVO_CODE setTorque(int32_t torque) = 0;
  virtual MC_SERVO_CODE setMode(Cia402Mode mode) = 0;

  virtual int32_t pos()=0;
  virtual int32_t vel()=0;
  virtual int32_t acc()=0;
  virtual int32_t torque() = 0;

  void setServoConfig(const ServoPara& config)
  {
    servoConfig_ = config;
    if (servoConfig_.encoderCountPerUnit == 0) 
    {
      servoConfig_.encoderCountPerUnit = 1;
    }
  }

  virtual MC_SERVO_CODE setPosInTurns(double turns)
  {
    return setPos(turnsToEncoderCount(turns));
  }

  virtual MC_SERVO_CODE setVelInTurns(double turns)
  {
    return setVel(turnsToEncoderCount(turns));
  }

  double posInTurns()
  {
    return encoderCountToTurns(pos());
  }

  double velInTurns()
  {
    return encoderCountToTurns(vel());
  }

  double accInTurns()
  {
    return encoderCountToTurns(acc());
  }

  virtual bool readVal(int index, double& value) { return false; }
  virtual bool writeVal(int index, double value) { return false; }

  virtual bool resetError()
  {
    return true;
  }
  virtual void emergStop()=0;
  virtual void runCycle()=0;

protected:
  int32_t turnsToEncoderCount(double turns) const
  {
    const double encoderCount =turns * static_cast<double>(servoConfig_.encoderCountPerUnit) *static_cast<double>(servoConfig_.direction);
    const double maxValue = static_cast<double>(std::numeric_limits<int32_t>::max());
    const double minValue = static_cast<double>(std::numeric_limits<int32_t>::min());

    if (encoderCount > maxValue) 
    {
      return std::numeric_limits<int32_t>::max();
    }
    if (encoderCount < minValue) 
    {
      return std::numeric_limits<int32_t>::min();
    }
    return static_cast<int32_t>(std::llround(encoderCount));
  }

  double encoderCountToTurns(int32_t encoderCount) const
  {
    return (static_cast<double>(encoderCount) *
           static_cast<double>(servoConfig_.direction)) /
           static_cast<double>(servoConfig_.encoderCountPerUnit);
  }

private:
  ServoPara servoConfig_;
};

class Axis {
private:
  AxisPara *config_;
  std::vector<std::unique_ptr<Servo>> servo_;
  std::vector<ServoPara> servoConfig_;

  uint32_t axisId_=0;
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
  MC_AXIS_STATES axisState_=MC_AXIS_STATES::mcStandstill;
  MC_ERROR_CODE axisError_=MC_ERRORCODE_GOOD;

  bool powerOn_=false;
  bool powerStatus_=false;
  bool reset_=false;
  bool enablePositive_=true;
  bool enableNegative_=true;

public:
  Axis(uint32_t axisId,AxisPara *config): axisId_(axisId),config_(config)
  {
  }
  Axis(uint32_t axisId,uint32_t salveId,AxisPara *config): Axis(axisId, config)
  {
      (void)salveId;
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

  void pushServo(std::unique_ptr<Servo> servo, const ServoPara& config)
  {
    servo->setServoConfig(config);
    servo_.push_back(std::move(servo));
    servoConfig_.push_back(config);
  }

  size_t servoCount() const
  {
    return servo_.size();
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
   void setAxisVelocityCmd(double axisVelCmd)
   {
       axisVelCmd_=axisVelCmd;
   }
   void syncCmdHistory()
   {
         lastAxisPosCmd_ = axisPosCmd_;
         lastAxisVelCmd_ = 0.0;
   }

  double toUserUnit(double motorTurns)
  {
    return toUserUnit(motorTurns, defaultServoConfig());
  }

  double toServoUnit(double axisUnit)
  {
    return toServoUnit(axisUnit, defaultServoConfig());
  }

  double toUserUnit(double motorTurns, const ServoPara& config) const
  {
    (void)config;
    return motorTurns * config_->lead;
  }

  double toServoUnit(double axisUnit, const ServoPara& config) const
  {
    (void)config;
    return config_->lead == 0.0 ? 0.0 : axisUnit / config_->lead;
  }

  bool cmdsProcessing(double frequency);
  void updateMotionCmdsToServo();
  void statusSync();
  
  double actualPos()
  {
    return axisPos_ - zeroOffset_;
  }
  double actualVel()
  {
    return axisVel_;
  }
  double actualAcc()
  {
    return axisAcc_;
  }
  double actualPosCmd()
  {
    return axisPosCmd_;
  }
  double actualVelCmd()
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
  double getLead() const
  {
    return config_->lead;
  }
  double getPositiveLimit() const
  {
    return config_->posPositiveLimit;
  }
  double getNegativeLimit() const
  {
    return config_->posNegativeLimit;
  }

  void setZeroOffset(double offset) 
  { 
    zeroOffset_ = offset+zeroOffset_;
   }
  double getZeroOffset() const 
  { return zeroOffset_; }

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
  const ServoPara& defaultServoConfig() const
  {
    static const ServoPara fallback{};
    return servoConfig_.empty() ? fallback : servoConfig_.front();
  }
  double zeroOffset_ = 0;
};

class Io {
public:
  virtual bool isEmergencyStop() { return false;}
  virtual void setEmergencyStop(bool value) { (void)value; }

  virtual bool ioRead32(int index, int bitPos) = 0;
  virtual bool ioRead16(int index, int bitPos) = 0;
  virtual bool ioRead8(int index, int bitPos) = 0;
  virtual void ioWrite32(int index, int bitPos, bool value) = 0;
  virtual void ioWrite16(int index, int bitPos, bool value) = 0;
  virtual void ioWrite8(int index, int bitPos, bool value) = 0;

  virtual void aoWriteValue(int index, double value) { (void)index; (void)value; }
  virtual double aoReadValue(int index) { (void)index; return 0; }
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
    virtual void bindSharedBlock(zrcs::SharedBlock* block) { (void)block; }
    virtual void send() = 0;
    virtual void receive() = 0;
};

} 
