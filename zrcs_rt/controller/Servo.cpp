#include "controller/Servo.h"

#include "system/log/RtLog.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace ZrcsHardware {

// ── Servo ──────────────────────────────────────────────────────────────────

bool Servo::enable()
{
    return true;
}

bool Servo::disable()
{
    return true;
}

void Servo::setServoConfig(const ServoPara& config)
{
    servoConfig_ = config;
    if (servoConfig_.encoderCountPerUnit == 0)
    {
        servoConfig_.encoderCountPerUnit = 1;
    }
}

const ServoPara& Servo::servoConfig() const
{
    return servoConfig_;
}

MC_SERVO_CONTROL_MODE Servo::controlMode() const
{
    return servoConfig_.mode;
}

MC_SERVO_CODE Servo::setPosInTurns(double turns)
{
    return setPos(turnsToEncoderCount(turns));
}

MC_SERVO_CODE Servo::setVelInTurns(double turns)
{
    return setVel(turnsToEncoderCount(turns));
}

double Servo::posInTurns()
{
    return encoderCountToTurns(pos());
}

double Servo::velInTurns()
{
    return encoderCountToTurns(vel());
}

double Servo::accInTurns()
{
    return encoderCountToTurns(acc());
}

bool Servo::readVal(int /*index*/, double& /*value*/)
{
    return false;
}

bool Servo::writeVal(int /*index*/, double /*value*/)
{
    return false;
}

bool Servo::resetError()
{
    return true;
}

int32_t Servo::turnsToEncoderCount(double turns) const
{
    // 工业常用路径：电机侧单位 → 编码器计数
    //   counts = round(turns * encoderCountPerUnit) * direction
    // - isfinite：避免 NaN/Inf 转整数 UB
    // - 先按 int32 量级限幅再 round：避免 double→整数超范围 UB
    // - std::round：half away from zero（优于截断）
    // - direction 对应 CiA402 Polarity
    if (!std::isfinite(turns))
    {
        // 非法指令不静默给随机 count；0 仅作防 UB 兜底，上层应避免传入非有限值
        return 0;
    }

    const int64_t countsPerUnit =
        static_cast<int64_t>(servoConfig_.encoderCountPerUnit);
    if (countsPerUnit <= 0)
    {
        return 0;
    }

    const double product = turns * static_cast<double>(countsPerUnit);
    if (!std::isfinite(product))
    {
        return 0;
    }

    // direction: +1 / -1
    const double signedProduct =product * static_cast<double>(servoConfig_.direction);

    // 在 double 域先饱和到 int32 范围，再 round，避免超大浮点转整数 UB
    constexpr double kMax = static_cast<double>(std::numeric_limits<int32_t>::max());
    constexpr double kMin =static_cast<double>(std::numeric_limits<int32_t>::min());
    const double limited = std::clamp(signedProduct, kMin, kMax);
    return static_cast<int32_t>(std::round(limited));
}

double Servo::encoderCountToTurns(int32_t encoderCount) const
{
    // 反馈：counts → 电机侧单位；direction 与指令路径一致
    const double countsPerUnit = static_cast<double>(servoConfig_.encoderCountPerUnit);
    if (countsPerUnit == 0.0)
    {
        return 0.0;
    }
    return (static_cast<double>(encoderCount) * static_cast<double>(servoConfig_.direction))/ countsPerUnit;
}

} // namespace ZrcsHardware
