/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Description: Axis class method implementations
 */
#include "controller/ControllerInterface.h"
#include "system/log/RtLog.h"
#include <cmath>
#include <cstdint>

namespace ZrcsHardware {

double Axis::fixOverFlow(double x)
{
    x += overflowCount_ * INT32_MAX * 2.0;
    if (x >= INT32_MAX)
    {
      x -= INT32_MAX * 2.0;
      overflowCount_ -= 1;
    }
    else if (x <= - INT32_MAX)
    {
      x += INT32_MAX * 2.0;
      overflowCount_ += 1;
    }
    return x;
}

bool Axis::cmdsProcessing(double frequency)
{
    // Check motion direction and limits based on command differences
    double vel_cmd = (axisPosCmd_ - lastAxisPosCmd_) * frequency;
    double acc_cmd = (vel_cmd - lastAxisVelCmd_) * frequency;
    axisVelCmd_ = vel_cmd;  // 存储 double 精度指令速度（无编码器量化噪声）

    if(vel_cmd > 0 && !enablePositive_)
    {
      axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_POSITIVE;
      ERROR_PRINT("轴%d: 正方向禁止运动\n", axisId_);
      return false;
    } else if(vel_cmd < 0 && !enableNegative_)
    {
      axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_NEGATIVE;
      ERROR_PRINT("轴%d: 负方向禁止运动\n", axisId_);
      return false;
    }

    if (std::abs(vel_cmd) > config_->maxVel)
    {
      //axisError_ = MC_ERRORCODE_CMDVELOVERLIMIT;
      //ERROR_PRINT("轴%d: 速度超限 vel=%.4f, max=%.4f\n", axisId_, vel_cmd, config_->maxVel);
     // return false;
    }

    if (std::abs(acc_cmd) > config_->maxAcc)
    {
      //axisError_ = MC_ERRORCODE_CMDACCOVERLIMIT;
      //ERROR_PRINT("轴%d: 加速度超限 acc=%.4f, max=%.4f\n", axisId_, acc_cmd, config_->maxAcc);
      //return false;
    }

    double rawPosCmd = axisPosCmd_ + zeroOffset_;
    if(rawPosCmd > config_->posPositiveLimit && vel_cmd > 0)
    {
      axisError_ = MC_ERRORCODE_CMDPPOSOVERLIMIT;
      ERROR_PRINT("轴%d: 正限位超限 pos=%.4f, limit=%.4f\n", axisId_, rawPosCmd, config_->posPositiveLimit);
      return false;
    }

    if(rawPosCmd < config_->posNegativeLimit && vel_cmd < 0)
    {
      axisError_ = MC_ERRORCODE_CMDNPOSOVERLIMIT;
      ERROR_PRINT("轴%d: 负限位超限 pos=%.4f, limit=%.4f\n", axisId_, rawPosCmd, config_->posNegativeLimit);
      return false;
    }

    // Update history for next cycle
    lastAxisPosCmd_ = axisPosCmd_;
    lastAxisVelCmd_ = vel_cmd;

    return true;
}

void Axis::updateMotionCmdsToServo()
{
    if (config_->mode== mcServoControlModePosition)
    {
      for (auto& servo : servo_)
      {
        auto ret = servo->setPos(toEncoderUnit(axisPosCmd_ + zeroOffset_));
        if (ret != SERVONOERROR) {
            WARN_PRINT("轴%d: setPos 伺服错误 code=%d\n", axisId_, static_cast<int>(ret));
        }
      }
    }
    if (config_->mode == mcServoControlModeVelocity)
    {
      for (auto& servo : servo_)
      {
        auto ret = servo->setVel(toEncoderUnit(axisVelCmd_));
        if (ret != SERVONOERROR) {
            WARN_PRINT("轴%d: setVel 伺服错误 code=%d\n", axisId_, static_cast<int>(ret));
        }
      }
    }
}

void Axis::statusSync()
{
    double lastAxisPos_ = 0.0;
    for (int i=0;i<servo_.size();i++)
    {

       axisPos_ = toUserUnit(servo_[i]->pos() - overflowCount_ * INT32_MAX * 2.0);
       axisVel_ = toUserUnit(servo_[i]->vel());
       axisAcc_ = toUserUnit(servo_[i]->acc());
      // 计算位置差值
      if (i!=0)
      {
          double posDiff = axisPos_ - lastAxisPos_;
          if (std::abs(posDiff) > config_->maxPosDiff)
          {
             ERROR_PRINT("轴%d: 多驱同步误差过大 posDiff=%.4f, limit=%.4f\n", axisId_, posDiff, config_->maxPosDiff);
             axisError_=MC_ERRORCODE_MULTI_DRIVE_SYNC_ERROR;
          }
      }
      lastAxisPos_=axisPos_;
    }

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
        {
          axisState_ = setState;
          return MC_ERRORCODE_GOOD;
        }
        break;
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
          {
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
          }
          break;
        default:
          ERROR_PRINT("轴%d: 状态转换非法 Stopping->%d\n", axisId_, setState);
          return MC_ERRORCODE_INVALIDSTATESTIPPING;
          break;
      }
      break;
    case mcErrorStop:
      switch (setState)
      {
        case mcErrorStop:
        case mcDisabled:
        case mcStandstill:
          {
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
          }
          break;
        default:
          ERROR_PRINT("轴%d: 状态转换非法 ErrorStop->%d\n", axisId_, setState);
          return MC_ERRORCODE_INVALIDSATATESTOP;
          break;
      }
      break;
    case mcDisabled:
      switch (setState)
      {
        case mcDisabled:
        case mcErrorStop:
        case mcStandstill:
          {
            axisState_ = setState;
            return MC_ERRORCODE_GOOD;
          }
          break;
        default:
          ERROR_PRINT("轴%d: 状态转换非法 Disabled->%d\n", axisId_, setState);
          return MC_ERRORCODE_INVALIDSTATEDISABLE;
          break;
      }
      break;
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
       if(!servo->resetError())
       {
         ERROR_PRINT("轴%d: 伺服复位失败\n", axisId_);
         return false;
       }
     }
     return true;
}

bool Axis::powerOn()
{
    for (size_t i = 0; i < servo_.size(); i++)
    {
      if(!servo_[i]->enable())
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
         ERROR_PRINT("轴%d: 伺服%zu 失能失败\n", axisId_, i);
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

} // namespace ZrcsHardware
