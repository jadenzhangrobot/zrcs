#include "controller/Axis.h"

#include "system/log/RtLog.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ZrcsHardware {

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
    // 软限位安全网（轴级命令整形，不改写上层规划器内部状态）：
    // 1) 每周期用剩余距离 s 计算制动速度上限 vCap=√(2a·s)；
    // 2) 再用 |Δv|≤a·dt 做相邻周期速度斜坡，避免速度跳变；
    // 3) 用限速后的 v 重写本拍 axisPosCmd_，把“减速”落实成“少走一点”；
    // 4) 贴边只锁超限方向，允许反向退出；边沿告警；不置 axisError_。
    //
    // 远/近不是显式 if 分支，而是由 v 与 vCap 的大小关系自然体现：
    //   - 远处：vCap 很大，min(v,vCap) 不起作用 → 基本不干预上层速度；
    //   - 开始减速：当 当前速度 > vCap 时，距离已不够从容刹停 → 压速度；
    //   - 贴边：s<=0 或本拍走到限位 → 锁方向、速度清零。
    if (frequency <= 0.0 || !std::isfinite(frequency))
    {
        ERROR_PRINT("axis%d: invalid cmdsProcessing frequency=%.6f\n", axisId_, frequency);
        return false;
    }

    const double dt = 1.0 / frequency;
    const double aMax = std::max(0.0, config_->maxAcc);
    const double vMax = std::max(0.0, config_->maxVel);

    // 上层本拍期望速度（由位置命令差分得到）；后续可能被软限位改写。
    double vel_cmd = (axisPosCmd_ - lastAxisPosCmd_) * frequency;
    // 机械坐标 = 用户命令坐标 + 零点偏移；软限位按机械行程判断。
    const double lastRaw = lastAxisPosCmd_ + zeroOffset_;

    // 软限位：仅 posPositiveLimit > posNegativeLimit 时有效。
    // 两者均为默认 0 或反向配置时，跳过全部软限位逻辑，避免误锁轴。
    if (config_->posPositiveLimit > config_->posNegativeLimit)
    {
    // —— 正软限位 ——
    // remPos = 到正限位的剩余距离 s。
    // remPos<=0 时只禁止继续正向，不得清零反向速度（否则无法退出限位）。
    {
        const double remPos = config_->posPositiveLimit - lastRaw;
        if (remPos <= 0.0)
        {
            // [贴边/已越界] 锁正向；若上层仍给正向速度，钉在限位并清零。
            if (enablePositive_)
            {
                WARN_PRINT("axis%d: soft +limit reached pos=%.4f limit=%.4f\n",
                           axisId_, lastRaw, config_->posPositiveLimit);
            }
            enablePositive_ = false;
            if (vel_cmd > 0.0)
            {
                axisPosCmd_ = config_->posPositiveLimit - zeroOffset_;
                vel_cmd = 0.0;
            }
            // vel_cmd < 0：放行反向退出，不改写 axisPosCmd_
        }
        else if (vel_cmd > 0.0)
        {
            // [朝正限位运动] remPos>0：还没贴边，进入距离限速逻辑。
            if (aMax > 0.0)
            {
                // vCap：以 aMax 匀减速、刚好在限位刹停时的最高允许速度。
                // s 大 → vCap 大（远处）；s 小 → vCap 小（近处开始收速度）。
                double vCap = std::sqrt(2.0 * aMax * remPos);

                // 加速度门：本拍速度相对上一拍输出，变化量不超过 aMax·dt。
                // 即使要减速，也不能一拍从高速直接掉到 0。
                double vUp = lastAxisVelCmd_ + aMax * dt;
                double vDn = lastAxisVelCmd_ - aMax * dt;
                double v = std::clamp(vel_cmd, vDn, vUp);

                // 距离门：真正决定“远处不管 / 开始减速”的一步。
                //   - 若 v <= vCap：剩余距离还够，vCap 不起作用 → 远处，保持斜坡后速度；
                //   - 若 v >  vCap：已进入制动包络 → 开始把速度压到 vCap 以下。
                v = std::min(v, vCap);
                if (vMax > 0.0)
                {
                    v = std::min(v, vMax);
                }
                // 正限位侧只处理正向接近，不允许借道变成反向。
                v = std::max(0.0, v);

                // 用限速后的 v 积分本拍机械位置；必要时钳到正限位。
                // 这里不是另发“减速命令”，而是把目标位置改成“少走一点”。
                double nextRaw = lastRaw + v * dt;
                if (nextRaw > config_->posPositiveLimit)
                {
                    nextRaw = config_->posPositiveLimit;
                }
                // 机械坐标 → 用户坐标后写回命令；再反算本拍真实速度，保证 pos/vel 一致。
                axisPosCmd_ = nextRaw - zeroOffset_;
                vel_cmd = (axisPosCmd_ - lastAxisPosCmd_) * frequency;

                if (nextRaw >= config_->posPositiveLimit)
                {
                    // [本拍刚好到边] 锁正向并清零速度。
                    if (enablePositive_)
                    {
                        WARN_PRINT("axis%d: soft +limit reached pos=%.4f limit=%.4f\n",
                                   axisId_, nextRaw, config_->posPositiveLimit);
                    }
                    enablePositive_ = false;
                    vel_cmd = 0.0;
                }
            }
            else
            {
                // maxAcc=0：无法做平滑制动，只能在本拍将越界时硬钳位置。
                double nextRaw = lastRaw + vel_cmd * dt;
                if (nextRaw > config_->posPositiveLimit)
                {
                    nextRaw = config_->posPositiveLimit;
                    axisPosCmd_ = nextRaw - zeroOffset_;
                    vel_cmd = (axisPosCmd_ - lastAxisPosCmd_) * frequency;
                    if (enablePositive_)
                    {
                        WARN_PRINT("axis%d: soft +limit clamp (no maxAcc) pos=%.4f\n",
                                   axisId_, nextRaw);
                    }
                    enablePositive_ = false;
                }
            }
        }
        // vel_cmd<=0 且 remPos>0：不朝正限位走，正侧无需干预。
    }

    // —— 负软限位（与正侧对称）：只禁止继续负向 ——
    // remNeg = 到负限位的剩余距离 s；远/近判定同样由 |v| 与 vCap 比较自然形成。
    {
        const double remNeg = lastRaw - config_->posNegativeLimit;
        if (remNeg <= 0.0)
        {
            // [贴边/已越界] 锁负向；若上层仍给负向速度，钉在限位并清零。
            if (enableNegative_)
            {
                WARN_PRINT("axis%d: soft -limit reached pos=%.4f limit=%.4f\n",axisId_, lastRaw, config_->posNegativeLimit);
            }
            enableNegative_ = false;
            if (vel_cmd < 0.0)
            {
                axisPosCmd_ = config_->posNegativeLimit - zeroOffset_;
                vel_cmd = 0.0;
            }
            // vel_cmd > 0：放行反向（向正）退出
        }
        else if (vel_cmd < 0.0)
        {
            // [朝负限位运动] remNeg>0：还没贴边，进入距离限速逻辑。
            if (aMax > 0.0)
            {
                // 负向速度上限幅值：vCap=√(2a·s)；远处 vCap 大，近处收紧。
                double vCap = std::sqrt(2.0 * aMax * remNeg);

                // 加速度门：|Δv|≤aMax·dt。
                double vUp = lastAxisVelCmd_ + aMax * dt;
                double vDn = lastAxisVelCmd_ - aMax * dt;
                double v = std::clamp(vel_cmd, vDn, vUp);

                // 距离门：负向取 max(v, -vCap)。
                //   - |v| <= vCap：远处，基本不干预；
                //   - |v| >  vCap：开始减速，把速度往 -vCap 方向收。
                v = std::max(v, -vCap);
                if (vMax > 0.0)
                {
                    v = std::max(v, -vMax);
                }
                // 负限位侧只处理负向接近，不允许借道变成正向。
                v = std::min(0.0, v);

                // 限速后的 v 积分位置，并在必要时钳到负限位。
                double nextRaw = lastRaw + v * dt;
                if (nextRaw < config_->posNegativeLimit)
                {
                    nextRaw = config_->posNegativeLimit;
                }
                axisPosCmd_ = nextRaw - zeroOffset_;
                vel_cmd = (axisPosCmd_ - lastAxisPosCmd_) * frequency;

                if (nextRaw <= config_->posNegativeLimit)
                {
                    // [本拍刚好到边] 锁负向并清零速度。
                    if (enableNegative_)
                    {
                        WARN_PRINT("axis%d: soft -limit reached pos=%.4f limit=%.4f\n",
                                   axisId_, nextRaw, config_->posNegativeLimit);
                    }
                    enableNegative_ = false;
                    vel_cmd = 0.0;
                }
            }
            else
            {
                // maxAcc=0：无平滑制动，越界时硬钳位置。
                double nextRaw = lastRaw + vel_cmd * dt;
                if (nextRaw < config_->posNegativeLimit)
                {
                    nextRaw = config_->posNegativeLimit;
                    axisPosCmd_ = nextRaw - zeroOffset_;
                    vel_cmd = (axisPosCmd_ - lastAxisPosCmd_) * frequency;
                    if (enableNegative_)
                    {
                        WARN_PRINT("axis%d: soft -limit clamp (no maxAcc) pos=%.4f\n",
                                   axisId_, nextRaw);
                    }
                    enableNegative_ = false;
                }
            }
        }
        // vel_cmd>=0 且 remNeg>0：不朝负限位走，负侧无需干预。
    }

    } // if (posPositiveLimit > posNegativeLimit) — 软限位配置有效

    // 方向锁：仅拦截被锁方向；反向请求在此之前已保留。
    if (vel_cmd > 0.0 && !enablePositive_)
    {
        axisPosCmd_ = lastAxisPosCmd_;
        vel_cmd = 0.0;
    }
    if (vel_cmd < 0.0 && !enableNegative_)
    {
        axisPosCmd_ = lastAxisPosCmd_;
        vel_cmd = 0.0;
    }

    // 反向运动自动解锁对侧，便于退出限位后恢复双向运动。
    if (vel_cmd < 0.0)
    {
        enablePositive_ = true;
    }
    if (vel_cmd > 0.0)
    {
        enableNegative_ = true;
    }

    axisVelCmd_ = vel_cmd;
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
                setAxisState(AxisState::ErrorStop);
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

Axis::AxisState Axis::getAxisState(void)
{
    return axisState_;
}

MC_ERROR_CODE Axis::setAxisState(Axis::AxisState setState)
{
    switch (axisState_)
    {
    case AxisState::Standstill:
    case AxisState::Homing:
    case AxisState::DiscreteMotion:
    case AxisState::ContinuousMotion:
    case AxisState::Stopping:
        switch (setState)
        {
        case AxisState::Stopping:
        case AxisState::Disabled:
        case AxisState::ErrorStop:
        case AxisState::Standstill:
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
        default:
            ERROR_PRINT("axis%d: invalid transition Stopping->%d\n", axisId_, setState);
            return MC_ERRORCODE_INVALIDSTATESTIPPING;
        }
    case AxisState::ErrorStop:
        switch (setState)
        {
        case AxisState::ErrorStop:
        case AxisState::Disabled:
        case AxisState::Standstill:
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
        default:
            ERROR_PRINT("axis%d: invalid transition ErrorStop->%d\n", axisId_, setState);
            return MC_ERRORCODE_INVALIDSATATESTOP;
        }
    case AxisState::Disabled:
        switch (setState)
        {
        case AxisState::Disabled:
        case AxisState::ErrorStop:
        case AxisState::Standstill:
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

void Axis::cyclerun()
{
    bool allEnabled = true;
    for (auto& servo : servo_)
    {
        // 周期驱动各伺服，聚合其真实状态推导轴级使能与故障。
        Servo::ServoState state = servo->runCycle();
        allEnabled = allEnabled && (state == Servo::ServoState::Enabled);
        if (state == Servo::ServoState::Fault)
        {
            setAxisState(Axis::AxisState::ErrorStop);
        }
    }
    // 用设备上报的真实使能状态同步轴级上电标志，避免驱动器中途
    // 掉使能后 powerStatus_ 仍停留在 true 与设备脱节。
    powerStatus_ = allEnabled;
}

bool Axis::resetError(void)
{
    for (size_t i = 0; i < servo_.size(); i++)
    {
        if (!servo_[i]->resetError())
        {
            ERROR_PRINT("axis%d: servo%zu reset failed\n", axisId_, i);
            return false;
        }
    }
    setAxisState(Axis::AxisState::Standstill);
    return true;
}

bool Axis::powerOn()
{
    for (size_t i = 0; i < servo_.size(); i++)
    {
        if (!servo_[i]->isEnabled())
        {
            if (!servo_[i]->enable())
            {
                ERROR_PRINT("axis%d: servo%zu enable failed\n", axisId_, i);
                return false;
            }
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

} // namespace ZrcsHardware
