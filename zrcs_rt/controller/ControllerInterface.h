/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: controllr的一些抽象接口包括电机，传感器，io等
 *
 */
#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "AxisConfig.h"
#include "Global.h"
#include "config/Parameter.h"
#include "shared_memory/ShmLayout.h"
namespace ZrcsHardware {
class Servo
{
public:
  Servo()
  {}
  virtual~Servo() = default;

  virtual bool enable()
  {
    return true;
  }
  virtual bool disable()
  {
    return true;
  }
  //virtual MC_SERVO_CODE setPower(bool powerStatus)=0;
  virtual MC_SERVO_CODE setPos(int32_t pos)=0;
  virtual MC_SERVO_CODE setVel(int32_t vel) = 0;
  virtual MC_SERVO_CODE setTorque(int32_t torque) = 0;
  virtual MC_SERVO_CODE setMode(Cia402Mode mode) = 0;

  virtual int32_t pos()=0;
  virtual int32_t vel()=0;
  virtual int32_t acc()=0;
  virtual int32_t torque() = 0;

  virtual bool readVal(int index, double& value) { return false; }
  virtual bool writeVal(int index, double value) { return false; }
  

  virtual bool resetError()
  {
    return true;
  }
  virtual void emergStop()=0;
  virtual void runCycle()=0;
};

class Axis {
private:

  AxisPara *config_;
  std::vector<std::unique_ptr<Servo>> servo_;

  // 与 servo_ 平行存储：servoConfig_[i] 描述 servo_[i]。
  // Axis 负责逻辑轴限位和命令状态，每个伺服保留自己的模式与编码器比例，
  // 因此一个轴可以驱动多个物理电机。
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
  int32_t overflowCount_=0;
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

  /**
   * @brief 绑定一个物理/虚拟伺服及其单驱参数。
   *
   * 对双驱轴，HardwareFactory 会按 axis.xml 中列出的每个 slaveId 调用一次。
   * 插入顺序会被保留，后续命令下发和反馈换算都按这个顺序匹配 servoConfig_。
   */
  void pushServo(std::unique_ptr<Servo> servo, const ServoPara& config)
  {
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


  double toUserUnit(double x)
  {
    return toUserUnit(x, defaultServoConfig());
  }

  int32_t toEncoderUnit(double x)
  {
    return toEncoderUnit(x, defaultServoConfig());
  }
  double toUserUnit(double x, const ServoPara& config)
  {
    // 将编码器计数转换回运动命令使用的用户单位。direction 是伺服相对逻辑轴
    // 正方向的符号：反装驱动器写 -1，反馈乘同一个符号后回到统一轴坐标。
    return (x * config.direction) / config.encoderCountPerUnit;
  }

  int32_t toEncoderUnit(double x, const ServoPara& config)
  {
    // 先按逻辑轴正方向处理编码器溢出，再乘以伺服方向。这样双驱轴一正一反时，
    // 两个伺服共享同一条逻辑轴连续位置，不会因为反向安装而各自维护一套溢出状态。
    return (int32_t)(fixOverFlow(x * config.encoderCountPerUnit) * config.direction);
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

  // --- 零点偏移 ---
  void setZeroOffset(double offset) 
  { 
    zeroOffset_ = offset+zeroOffset_;
   }
  double getZeroOffset() const 
  { return zeroOffset_; }

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
  const ServoPara& defaultServoConfig() const
  {
    // 旧调用点如果没有显式传入 ServoPara，就沿用第一个驱动器的比例。
    // 在还没有添加伺服前使用安全回退配置，避免除零或空引用。
    static const ServoPara fallback{};
    return servoConfig_.empty() ? fallback : servoConfig_.front();
  }
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

  /// 模拟量输出: 写入指定通道的模拟值 (如 DAC 电压)
  virtual void aoWriteValue(int index, double value) {}
  /// 模拟量输入: 读取指定通道的模拟值 (如 ADC 电压)
  virtual double aoReadValue(int index) { return 0; }
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
