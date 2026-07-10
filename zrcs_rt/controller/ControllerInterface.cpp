#include "controller/ControllerInterface.h"

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
    const double signedProduct =
        product * static_cast<double>(servoConfig_.direction);

    // 在 double 域先饱和到 int32 范围，再 round，避免超大浮点转整数 UB
    constexpr double kMax =
        static_cast<double>(std::numeric_limits<int32_t>::max());
    constexpr double kMin =
        static_cast<double>(std::numeric_limits<int32_t>::min());
    const double limited = std::clamp(signedProduct, kMin, kMax);

    return static_cast<int32_t>(std::round(limited));
}

double Servo::encoderCountToTurns(int32_t encoderCount) const
{
    // 反馈：counts → 电机侧单位；direction 与指令路径一致
    const double countsPerUnit =
        static_cast<double>(servoConfig_.encoderCountPerUnit);
    if (countsPerUnit == 0.0)
    {
        return 0.0;
    }
    return (static_cast<double>(encoderCount)
            * static_cast<double>(servoConfig_.direction))
           / countsPerUnit;
}

// ── Axis ───────────────────────────────────────────────────────────────────

Axis::Axis(uint32_t axisId, AxisPara* config)
    : config_(config)
    , axisId_(axisId)
{
}

Axis::Axis(uint32_t axisId, uint32_t salveId, AxisPara* config)
    : Axis(axisId, config)
{
    (void)salveId;
}

Axis::~Axis()
{
    servo_.clear();
    delete config_;
}

void Axis::pushServo(std::unique_ptr<Servo> servo)
{
    servo_.push_back(std::move(servo));
}

void Axis::pushServo(std::unique_ptr<Servo> servo, const ServoPara& config)
{
    // 伺服参数注入 Servo 内部，Axis 不再另存一份。
    servo->setServoConfig(config);
    servo_.push_back(std::move(servo));
}

size_t Axis::servoCount() const
{
    return servo_.size();
}

MC_ERROR_CODE Axis::setAxisId(uint32_t id)
{
    axisId_ = id;
    return MC_ERRORCODE_GOOD;
}

MC_ERROR_CODE Axis::setAxisName(std::string name)
{
    axisName_ = name;
    return MC_ERRORCODE_GOOD;
}

void Axis::setAxisPositionCmd(double axisPosCmd)
{
    axisPosCmd_ = axisPosCmd;
}

void Axis::setAxisVelocityCmd(double axisVelCmd)
{
    axisVelCmd_ = axisVelCmd;
}

void Axis::syncCmdHistory()
{
    lastAxisPosCmd_ = axisPosCmd_;
    lastAxisVelCmd_ = 0.0;
}

double Axis::toUserUnit(double motorTurns) const
{
    return motorTurns * config_->lead;
}

double Axis::toServoUnit(double axisUnit) const
{
    return config_->lead == 0.0 ? 0.0 : axisUnit / config_->lead;
}

bool Axis::cmdsProcessing(double frequency)
{
    double vel_cmd = (axisPosCmd_ - lastAxisPosCmd_) * frequency;
    double acc_cmd = (vel_cmd - lastAxisVelCmd_) * frequency;
    axisVelCmd_ = vel_cmd;

    if (vel_cmd > 0 && !enablePositive_)
    {
        axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_POSITIVE;
        ERROR_PRINT("axis%d: positive direction disabled\n", axisId_);
        return false;
    }
    if (vel_cmd < 0 && !enableNegative_)
    {
        axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_NEGATIVE;
        ERROR_PRINT("axis%d: negative direction disabled\n", axisId_);
        return false;
    }

    (void)acc_cmd;

    double rawPosCmd = axisPosCmd_ + zeroOffset_;
    if (rawPosCmd > config_->posPositiveLimit && vel_cmd > 0)
    {
        axisError_ = MC_ERRORCODE_CMDPPOSOVERLIMIT;
        ERROR_PRINT("axis%d: positive limit exceeded pos=%.4f, limit=%.4f\n",
                    axisId_, rawPosCmd, config_->posPositiveLimit);
        return false;
    }

    if (rawPosCmd < config_->posNegativeLimit && vel_cmd < 0)
    {
        axisError_ = MC_ERRORCODE_CMDNPOSOVERLIMIT;
        ERROR_PRINT("axis%d: negative limit exceeded pos=%.4f, limit=%.4f\n",
                    axisId_, rawPosCmd, config_->posNegativeLimit);
        return false;
    }

    lastAxisPosCmd_ = axisPosCmd_;
    lastAxisVelCmd_ = vel_cmd;
    return true;
}

void Axis::updateMotionCmdsToServo()
{
    for (auto& servo : servo_)
    {
        const auto mode = servo->controlMode();
        if (mode == mcServoControlModePosition)
        {
            auto ret = servo->setPosInTurns(toServoUnit(axisPosCmd_ + zeroOffset_));
            if (ret != SERVONOERROR)
            {
                WARN_PRINT("axis%d: setPos servo error code=%d\n",
                           axisId_, static_cast<int>(ret));
            }
        }
        else if (mode == mcServoControlModeVelocity)
        {
            auto ret = servo->setVelInTurns(toServoUnit(axisVelCmd_));
            if (ret != SERVONOERROR)
            {
                WARN_PRINT("axis%d: setVel servo error code=%d\n",
                           axisId_, static_cast<int>(ret));
            }
        }
    }
}

void Axis::statusSync()
{
    double lastAxisPos = 0.0;
    for (size_t i = 0; i < servo_.size(); ++i)
    {
        axisPos_ = toUserUnit(servo_[i]->posInTurns());
        axisVel_ = toUserUnit(servo_[i]->velInTurns());
        axisAcc_ = toUserUnit(servo_[i]->accInTurns());

        if (i != 0)
        {
            double posDiff = axisPos_ - lastAxisPos;
            if (std::abs(posDiff) > config_->maxPosDiff)
            {
                ERROR_PRINT("axis%d: multi-drive sync error posDiff=%.4f, limit=%.4f\n",
                            axisId_, posDiff, config_->maxPosDiff);
                axisError_ = MC_ERRORCODE_MULTI_DRIVE_SYNC_ERROR;
                setAxisState(mcErrorStop);
            }
        }
        lastAxisPos = axisPos_;
    }
}

double Axis::actualPos()
{
    return axisPos_ - zeroOffset_;
}

double Axis::actualVel()
{
    return axisVel_;
}

double Axis::actualAcc()
{
    return axisAcc_;
}

double Axis::actualPosCmd()
{
    return axisPosCmd_;
}

double Axis::actualVelCmd()
{
    return axisVelCmd_;
}

MC_AXIS_STATES Axis::getAxisState(void)
{
    return axisState_;
}

MC_ERROR_CODE Axis::setAxisState(MC_AXIS_STATES setState)
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
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
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
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
        default:
            ERROR_PRINT("axis%d: invalid transition Stopping->%d\n", axisId_, setState);
            return MC_ERRORCODE_INVALIDSTATESTIPPING;
        }
    case mcErrorStop:
        switch (setState)
        {
        case mcErrorStop:
        case mcDisabled:
        case mcStandstill:
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
        default:
            ERROR_PRINT("axis%d: invalid transition ErrorStop->%d\n", axisId_, setState);
            return MC_ERRORCODE_INVALIDSATATESTOP;
        }
    case mcDisabled:
        switch (setState)
        {
        case mcDisabled:
        case mcErrorStop:
        case mcStandstill:
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
        default:
            ERROR_PRINT("axis%d: invalid transition Disabled->%d\n", axisId_, setState);
            return MC_ERRORCODE_INVALIDSTATEDISABLE;
        }
    default:
        break;
    }
    return MC_ERRORCODE_GOOD;
}

MC_ERROR_CODE Axis::cyclerun()
{
    for (auto& servo : servo_)
    {
        servo->runCycle();
    }
    return MC_ERRORCODE_GOOD;
}

bool Axis::resetError(void)
{
    for (auto& servo : servo_)
    {
        if (!servo->resetError())
        {
            ERROR_PRINT("axis%d: servo reset failed\n", axisId_);
            return false;
        }
    }

    // 软件错误码与 ErrorStop 状态一并清除，否则调度层会立刻再次进入 ERROR_STATE。
    axisError_ = MC_ERRORCODE_GOOD;
    if (axisState_ == mcErrorStop)
    {
        setAxisState(mcStandstill);
    }
    return true;
}

bool Axis::powerOn()
{
    for (auto& servo : servo_)
    {
        if (!servo->enable())
        {
            return false;
        }
    }
    return true;
}

bool Axis::powerOff()
{
    for (size_t i = 0; i < servo_.size(); i++)
    {
        if (!servo_[i]->disable())
        {
            ERROR_PRINT("axis%d: servo%zu disable failed\n", axisId_, i);
            return false;
        }
    }
    return true;
}

void Axis::setModeOfOperation()
{
    for (auto& servo : servo_)
    {
        servo->setMode(Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION);
    }
}

void Axis::setModeOfOperation(Cia402Mode mode)
{
    for (auto& servo : servo_)
    {
        servo->setMode(mode);
    }
}

MC_ERROR_CODE Axis::getAxisError()
{
    return axisError_;
}

MC_ERROR_CODE Axis::servoErrorToAxisError(MC_SERVO_CODE error_id)
{
    return static_cast<MC_ERROR_CODE>(0x60 + error_id);
}

double Axis::getMaxVelocity()
{
    return config_->maxVel;
}

double Axis::getMaxAcceleration()
{
    return config_->maxAcc;
}

double Axis::getMaxJerk()
{
    return config_->maxJerk;
}

double Axis::getLead() const
{
    return config_->lead;
}

double Axis::getPositiveLimit() const
{
    return config_->posPositiveLimit;
}

double Axis::getNegativeLimit() const
{
    return config_->posNegativeLimit;
}

void Axis::setZeroOffset(double offset)
{
    zeroOffset_ = offset + zeroOffset_;
}

double Axis::getZeroOffset() const
{
    return zeroOffset_;
}

void Axis::setPosLimits(double posLimit, double negLimit)
{
    config_->posPositiveLimit = posLimit;
    config_->posNegativeLimit = negLimit;
}

void Axis::setVelLimits(double maxVel, double maxAcc, double maxJerk)
{
    config_->maxVel = maxVel;
    config_->maxAcc = maxAcc;
    config_->maxJerk = maxJerk;
}

// ── Io ─────────────────────────────────────────────────────────────────────

Io::~Io() = default;

bool Io::isEmergencyStop()
{
    return false;
}

void Io::setEmergencyStop(bool /*value*/)
{
}

void Io::aoWriteValue(int /*index*/, double /*value*/)
{
}

double Io::aoReadValue(int /*index*/)
{
    return 0;
}

// ── Sensor ─────────────────────────────────────────────────────────────────

Sensor::~Sensor() = default;

// ── Rtos ───────────────────────────────────────────────────────────────────

Rtos::~Rtos() = default;

void Rtos::rtos_task_stop()
{
    rtos_task_join();
}

std::uint64_t Rtos::rtos_timer_read(void)
{
    return 0;
}

// ── HardwareBus ────────────────────────────────────────────────────────────

HardwareBus::~HardwareBus() = default;

void HardwareBus::bindSharedBlock(zrcs::SharedBlock* /*block*/)
{
}

} // namespace ZrcsHardware
