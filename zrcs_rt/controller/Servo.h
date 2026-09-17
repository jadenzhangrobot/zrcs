/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: servo hardware abstraction
 */
#pragma once

#include <cstdint>

// ── 全局作用域枚举（与全局头文件历史一致，全工程无前缀引用）──

/* Servo error code */
typedef enum {
    SERVONOERROR                = 0,
    SERVOFIELDBUSINITERROR      = 1,
    SERVOPOWERERROR             = 2,
    SERVOPOWERINGONERROR        = 3,
    SERVOERRORWHENPOWEREDON     = 4,
    SERVOPOWERINGOFFERROR       = 5
} MC_SERVO_CODE;

/* Servo control mode */
typedef enum {
    mcServoControlModePosition = 0,
    mcServoControlModeVelocity = 1,
    mcServoControlModeTorque   = 2,
} MC_SERVO_CONTROL_MODE;

/**
 * @brief Represents the CiA 402 modes of operation (Object 6060h).
 *
 * The underlying type is int8_t because the standard defines this object
 * as a SINT (Signed 8-bit integer).
 */
enum class Cia402Mode : uint8_t {
    NO_MODE_ASSIGNED = 0,
    PROFILE_POSITION = 1,
    // Value 2 is reserved
    PROFILE_VELOCITY = 3,
    PROFILE_TORQUE = 4,
    // Value 5 is reserved
    HOMING = 6,
    INTERPOLATED_POSITION = 7,
    CYCLIC_SYNCHRONOUS_POSITION = 8, // CSP
    CYCLIC_SYNCHRONOUS_VELOCITY = 9, // CSV
    CYCLIC_SYNCHRONOUS_TORQUE = 10,  // CST
    // Other values can be manufacturer-specific
};

namespace ZrcsHardware {

/**
 * @brief 绑定到 Axis 的运行时伺服参数。
 *
 * 一个 Axis 可以拥有多个 ServoPara。第 i 个 ServoPara 对应第 i 个 push 到 Axis
 * 的 Servo 对象，因此命令和反馈换算可以分别使用每个驱动器自己的模式、编码器比例
 * 和安装方向。
 */
struct ServoPara {
    uint32_t slaveId = 0;
    MC_SERVO_CONTROL_MODE mode = MC_SERVO_CONTROL_MODE::mcServoControlModePosition;
    uint64_t encoderCountPerUnit = 1;
    int direction = 1;
    double homePos = 0.0;
    double posOffset = 0.0;
    double velFactor = 1.0;
};

/// 伺服抽象接口：对上层暴露电机侧单位（圈/转速/角加速），
/// 通过 ServoPara（编码器比例、方向）在内部换算成编码器计数。
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
        Moving,    // 运动中
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

} // namespace ZrcsHardware
