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
#include <functional>
#include "axisParameter.h"

namespace ZrcsHardware {
const double pi = 3.14159265358979323846; 
typedef enum{
    DISABLED,
    STANDSTILL,
    HOMING,
    DISCRETE_MOTION,
    CONTINUOUS_MOTION,
    STOPPING,
    ERROR_STOP
}axisStatus;
typedef enum 
{
  GOOD                           = 0, //成功
  ERROR_AXISPOWEROFF                   = 1, //轴未使能
  ERROR_AXISPOWERON                    = 2, //轴已功能
  ERROR_AXISNOTEXIST                   = 3, //轴ID号不存在
  ERROR_POSILLEGAL                     = 4, //位置不合法
  ERROR_ACCILLEGAL                     = 5, //加/减速度不合法
  ERROR_VELILLEGAL                     = 6, //速度不合法
  ERROR_AXISHARDWARE                   = 7, //硬件错误
  ERROR_CMDPPOSOVERLIMIT               = 8, //指令位置超出正向限制
  ERROR_CMDNPOSOVERLIMIT               = 9, //指令位置超出负向限制
  ERROR_FORBIDDENPPOSMOVE              = 10, //禁止正向移动
  ERROR_FORBIDDENNPOSMOVE              = 11, //禁止负向移动
  ERROR_POSLAGOVERLIMIT                = 12, //轴跟随误差超限
  ERROR_CMDVELOVERLIMIT                = 13, //轴指令速度超出限制
  ERROR_CMDACCOVERLIMIT                = 14, //轴指令加速度超出限制
  ERROR_POSINFINITY                    = 15, //轴设定位置不合法
}AxErrorCode;
class Axis {
private:
 
  AxErrorCode axisError=GOOD;
  double targetPosition=0;
public:
  int axId;
 
  Axis(int mId): axId(mId)
  {

  }
  auto getTargetPos()->double
  {
      return targetPosition;
  }

  auto setTargetPos(double pos) -> void 
  {
      if (pos>ParaConfig->axisParas[axId].max_pos)
      {
        axisError=ERROR_CMDPPOSOVERLIMIT;
      }
      else if (pos<ParaConfig->axisParas[axId].min_pos)
      {
        axisError=ERROR_CMDNPOSOVERLIMIT;
      } 
      else
      {
        targetPosition=pos;
        setEncoderTargetPos((targetPosition + ParaConfig->axisParas[axId].pos_offset) * ParaConfig->axisParas[axId].pos_factor / (2 * pi));
      }
    
  };
  //单位是弧度
  auto actualPos()->double 
  {
     double pos = encoderActualPos();
     return (pos - ParaConfig->axisParas[axId].pos_offset) / ParaConfig->axisParas[axId].pos_factor * 2 * pi;
  }
   auto errorCode()->AxErrorCode
   {
         return axisError;
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
  

  virtual void setEncoderTargetPos(double position) = 0;
  virtual double encoderActualPos(void) = 0;

  virtual double actualVel() { return 0; }

  auto virtual actualToq() -> double { return 0; }
  // auto virtual actualCur()const->double = 0;
  // auto virtual actualAddlPos()const->double = 0;
  // auto virtual velDiff()const->double = 0;
  // auto virtual setVelDiff(double vel)->void = 0;

  auto virtual clearError() -> int { return 0;}
  auto virtual disable() -> int { return 0; }
  auto virtual enable() -> int { return 0; }
  auto virtual home() -> int { return 0; }
  auto virtual setModeOfOperation(std::uint8_t md) -> void {}
  auto virtual init() -> int { return 0; }

  virtual ~Axis(){};
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
class Sensor {

  virtual ~Sensor(){};
};

// class Controller {
// public:
//   virtual ~Controller() {};
//   virtual int init() = 0;
//   virtual int start() = 0;
//   virtual int stop() = 0;
//   virtual int close() = 0;
//   virtual int reset() = 0;
//   virtual int pause() = 0;
//   virtual int resume() = 0;
//   virtual bool isConnected() = 0;
//   virtual bool isRunning() = 0;
// };
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
