/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: controller hardware abstraction interfaces
 */
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "AxisConfig.h"
#include "Global.h"
#include "shared_memory/ShmLayout.h"

namespace ZrcsHardware {

class Servo
{  
public:
    enum class ServoState
    {
        Unknown,   // 尚未获取有效状态
        NotReady,  // 初始化中或未满足运行条件
        Disabled,  // 未使能
        Enabled,   // 已使能
        Stopping,  // 停止处理中
        Fault      // 故障
    };
    Servo() = default;
    virtual ~Servo() = default;


    virtual bool enable();
    virtual bool disable();

    virtual MC_SERVO_CODE setPos(int32_t pos) = 0;
    virtual MC_SERVO_CODE setVel(int32_t vel) = 0;
    virtual MC_SERVO_CODE setTorque(int32_t torque) = 0;
    virtual MC_SERVO_CODE setMode(Cia402Mode mode) = 0;

    virtual int32_t pos() = 0;
    virtual int32_t vel() = 0;
    virtual int32_t acc() = 0;
    virtual int32_t torque() = 0;

    void setServoConfig(const ServoPara& config);
    const ServoPara& servoConfig() const;
    MC_SERVO_CONTROL_MODE controlMode() const;

    virtual MC_SERVO_CODE setPosInTurns(double turns);
    virtual MC_SERVO_CODE setVelInTurns(double turns);

    virtual double posInTurns();
    virtual double velInTurns();
    virtual double accInTurns();

    virtual bool readVal(int index, double& value);
    virtual bool writeVal(int index, double value);
      virtual bool resetError();
    virtual void emergStop() = 0;
    virtual ServoState runCycle() = 0;
    virtual bool isEnabled() = 0;
    virtual bool isDisabled() = 0;

protected:
    int32_t turnsToEncoderCount(double turns) const;
    double encoderCountToTurns(int32_t encoderCount) const;

    /// 伺服参数仅在 Servo 内部使用（编码器比例、方向、模式等）。
    ServoPara servoConfig_;
};

class Axis
{
public:
    /// 轴级逻辑状态机（PLCOpen 风格），由周期循环聚合各 servo 状态推导。
    enum class AxisState
    {
        Disabled = 0,  /// 初始状态：未上电、无错误，不响应运动指令
        Standstill = 1,  /// 已上电、无错误、无运动指令执行中
        Homing = 2,  /// 回零中
        DiscreteMotion = 3,  /// 离散运动（到位停止）
        ContinuousMotion = 4,  /// 连续运动（速度模式）
        SynchronizedMotion = 5,  /// 同步运动
        Stopping = 6,  /// 停止处理中
        ErrorStop = 7  /// 错误停机，最高优先级；需复位后才能恢复
    };

private:
    AxisPara* config_;
    std::vector<std::unique_ptr<Servo>> servo_;

    uint32_t axisId_ = 0;
    std::string axisName_ = "";
    double axisPos_ = 0;
    double axisVel_ = 0;
    double axisAcc_ = 0;
    double axisJerk_ = 0;
    double axisPosCmd_ = 0;
    double lastAxisPosCmd_ = 0;
    double axisVelCmd_ = 0;
    double lastAxisVelCmd_ = 0;
    double axisTorCmd_ = 0;
    AxisState axisState_ = AxisState::Disabled;
    MC_ERROR_CODE axisError_ = MC_ERRORCODE_GOOD;

    bool powerStatus_ = false;
    bool reset_ = false;
    bool enablePositive_ = true;
    bool enableNegative_ = true;

    double zeroOffset_ = 0;

public:
    Axis(uint32_t axisId, AxisPara* config);
    Axis(uint32_t axisId, uint32_t salveId, AxisPara* config);
    virtual ~Axis();

    void pushServo(std::unique_ptr<Servo> servo);
    void pushServo(std::unique_ptr<Servo> servo, const ServoPara& config);

    size_t servoCount() const;

    MC_ERROR_CODE setAxisId(uint32_t id);
    MC_ERROR_CODE setAxisName(std::string name);

    void setAxisPositionCmd(double axisPosCmd);
    void setAxisVelocityCmd(double axisVelCmd);
    void syncCmdHistory();

    /// 电机圈数 ↔ 轴用户单位，仅使用轴参数 lead（用户单位/圈）。
    double toUserUnit(double motorTurns) const;
    double toServoUnit(double axisUnit) const;

    bool cmdsProcessing(double frequency);
    void updateMotionCmdsToServo();
    void statusSync();

    double actualPos();
    double actualVel();
    double actualAcc();
    double actualPosCmd();
    double actualVelCmd();

    AxisState getAxisState(void);
    MC_ERROR_CODE setAxisState(AxisState setState);

    void cyclerun();
    bool resetError(void);

    bool powerOn();
    bool powerOff();
    bool isPowerOn() const { return powerStatus_; }
    void setModeOfOperation();
    void setModeOfOperation(Cia402Mode mode);

    MC_ERROR_CODE getAxisError();
    MC_ERROR_CODE servoErrorToAxisError(MC_SERVO_CODE error_id);

    double getMaxVelocity();
    double getMaxAcceleration();
    double getMaxJerk();
    double getLead() const;
    double getPositiveLimit() const;
    double getNegativeLimit() const;

    void setZeroOffset(double offset);
    double getZeroOffset() const;

    void setPosLimits(double posLimit, double negLimit);
    void setVelLimits(double maxVel, double maxAcc, double maxJerk);
};

class Io
{
public:
    virtual ~Io();

    virtual bool isEmergencyStop();
    virtual void setEmergencyStop(bool value);

    virtual bool ioRead32(int index, int bitPos) = 0;
    virtual bool ioRead16(int index, int bitPos) = 0;
    virtual bool ioRead8(int index, int bitPos) = 0;
    virtual void ioWrite32(int index, int bitPos, bool value) = 0;
    virtual void ioWrite16(int index, int bitPos, bool value) = 0;
    virtual void ioWrite8(int index, int bitPos, bool value) = 0;

    virtual void aoWriteValue(int index, double value);
    virtual double aoReadValue(int index);
};

class Sensor
{
public:
    virtual ~Sensor();
};

class Rtos
{
public:
    virtual ~Rtos();
    virtual void real_task(std::function<void()> strategy) = 0;
    virtual void rtos_task_create(void) = 0;
    virtual void rtos_task_join(void) = 0;
    virtual void rtos_task_stop();
    virtual void rtos_set_periodic(int period) = 0;
    virtual std::uint64_t rtos_timer_read(void);
};

class HardwareBus
{
public:
    virtual ~HardwareBus();
    virtual void bindSharedBlock(zrcs::SharedBlock* block);
    virtual void send() = 0;
    virtual void receive() = 0;
};

} // namespace ZrcsHardware
