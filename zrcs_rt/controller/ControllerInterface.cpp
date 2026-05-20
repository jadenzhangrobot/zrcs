#include "controller/ControllerInterface.h"

#include "system/log/RtLog.h"

#include <cmath>
#include <cstdint>

namespace ZrcsHardware {

double Axis::fixOverFlow(double x)
{
    x += overflowCount_ * INT32_MAX * 2.0;
    if (x >= INT32_MAX) {
        x -= INT32_MAX * 2.0;
        overflowCount_ -= 1;
    } else if (x <= -INT32_MAX) {
        x += INT32_MAX * 2.0;
        overflowCount_ += 1;
    }
    return x;
}

bool Axis::cmdsProcessing(double frequency)
{
    double vel_cmd = (axisPosCmd_ - lastAxisPosCmd_) * frequency;
    double acc_cmd = (vel_cmd - lastAxisVelCmd_) * frequency;
    axisVelCmd_ = vel_cmd;

    if (vel_cmd > 0 && !enablePositive_) {
        axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_POSITIVE;
        ERROR_PRINT("axis%d: positive direction disabled\n", axisId_);
        return false;
    }
    if (vel_cmd < 0 && !enableNegative_) {
        axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_NEGATIVE;
        ERROR_PRINT("axis%d: negative direction disabled\n", axisId_);
        return false;
    }

    (void)acc_cmd;

    double rawPosCmd = axisPosCmd_ + zeroOffset_;
    if (rawPosCmd > config_->posPositiveLimit && vel_cmd > 0) {
        axisError_ = MC_ERRORCODE_CMDPPOSOVERLIMIT;
        ERROR_PRINT("axis%d: positive limit exceeded pos=%.4f, limit=%.4f\n",
                    axisId_, rawPosCmd, config_->posPositiveLimit);
        return false;
    }

    if (rawPosCmd < config_->posNegativeLimit && vel_cmd < 0) {
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
    // 将逻辑轴命令广播到该轴绑定的所有伺服。每个驱动器可以使用自己的模式和
    // 编码器比例，因此无需复制 Axis 对象也能支持双驱轴。
    for (size_t i = 0; i < servo_.size(); ++i) {
        const auto& cfg = i < servoConfig_.size() ? servoConfig_[i] : defaultServoConfig();
        if (cfg.mode == mcServoControlModePosition) {
            auto ret = servo_[i]->setPos(toEncoderUnit(axisPosCmd_ + zeroOffset_, cfg));
            if (ret != SERVONOERROR) {
                WARN_PRINT("axis%d: setPos servo error code=%d\n",
                           axisId_, static_cast<int>(ret));
            }
        } else if (cfg.mode == mcServoControlModeVelocity) {
            auto ret = servo_[i]->setVel(toEncoderUnit(axisVelCmd_, cfg));
            if (ret != SERVONOERROR) {
                WARN_PRINT("axis%d: setVel servo error code=%d\n",
                           axisId_, static_cast<int>(ret));
            }
        }
    }
}

void Axis::statusSync()
{
    double lastAxisPos = 0.0;
    for (size_t i = 0; i < servo_.size(); ++i) {
        const auto& cfg = i < servoConfig_.size() ? servoConfig_[i] : defaultServoConfig();

        // 反馈使用和命令下发相同的单驱比例换算。axisPos_ 最终保存最后一个伺服
        // 的位置，这样保持旧单驱行为，同时仍能检查多驱之间的位置差。
        axisPos_ = toUserUnit(servo_[i]->pos() - overflowCount_ * INT32_MAX * 2.0, cfg);
        axisVel_ = toUserUnit(servo_[i]->vel(), cfg);
        axisAcc_ = toUserUnit(servo_[i]->acc(), cfg);

        if (i != 0) {
            // maxPosDiff 属于逻辑轴，因为它描述的是同一机械轴上多个驱动器之间
            // 允许的同步误差。
            double posDiff = axisPos_ - lastAxisPos;
            if (std::abs(posDiff) > config_->maxPosDiff) {
                ERROR_PRINT("axis%d: multi-drive sync error posDiff=%.4f, limit=%.4f\n",
                            axisId_, posDiff, config_->maxPosDiff);
                axisError_ = MC_ERRORCODE_MULTI_DRIVE_SYNC_ERROR;
            }
        }
        lastAxisPos = axisPos_;
    }
}

MC_ERROR_CODE Axis::setAxisState(MC_AXIS_STATES setState)
{
    switch (axisState_) {
    case mcStandstill:
    case mcHoming:
    case mcDiscreteMotion:
    case mcContinuousMotion:
        switch (setState) {
        case mcDisabled:
        case mcErrorStop:
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
        default:
            break;
        }
        break;
    case mcStopping:
        switch (setState) {
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
        switch (setState) {
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
        switch (setState) {
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
    for (auto& servo : servo_) {
        servo->runCycle();
    }
    return MC_ERRORCODE_GOOD;
}

bool Axis::resetError(void)
{
    for (auto& servo : servo_) {
        if (!servo->resetError()) {
            ERROR_PRINT("axis%d: servo reset failed\n", axisId_);
            return false;
        }
    }
    return true;
}

bool Axis::powerOn()
{
    for (auto& servo : servo_) {
        if (!servo->enable()) {
            return false;
        }
    }
    return true;
}

bool Axis::powerOff()
{
    for (size_t i = 0; i < servo_.size(); i++) {
        if (!servo_[i]->disable()) {
            ERROR_PRINT("axis%d: servo%zu disable failed\n", axisId_, i);
            return false;
        }
    }
    return true;
}

void Axis::setModeOfOperation()
{
    for (auto& servo : servo_) {
        servo->setMode(Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION);
    }
}

void Axis::setModeOfOperation(Cia402Mode mode)
{
    for (auto& servo : servo_) {
        servo->setMode(mode);
    }
}

} // namespace ZrcsHardware
