/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Description: Axis class method implementations
 */
#include "controller/ControllerInterface.h"
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

    if(vel_cmd > 0 && !enablePositive_)
    {
      axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_POSITIVE;
      return false;
    } else if(vel_cmd < 0 && !enableNegative_)
    {
      axisError_ = MC_ERRORCODE_INVALID_DIRTCTION_NEGATIVE;
      return false;
    }

    if (std::abs(vel_cmd) > config_->maxVel)
    {
      axisError_ = MC_ERRORCODE_CMDVELOVERLIMIT;
      return false;
    }

    if (std::abs(acc_cmd) > config_->maxAcc)
    {
      axisError_ = MC_ERRORCODE_CMDACCOVERLIMIT;
      return false;
    }

    if(axisPosCmd_ > config_->posPositiveLimit && vel_cmd > 0)
    {
      axisError_ = MC_ERRORCODE_CMDPPOSOVERLIMIT;
      return false;
    }

    if(axisPosCmd_ < config_->posNegativeLimit && vel_cmd < 0)
    {
      axisError_ = MC_ERRORCODE_CMDNPOSOVERLIMIT;
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
        servo->setPos(toEncoderUnit(axisPosCmd_));
      }
    }
    if (config_->mode == mcServoControlModeVelocity)
    {
      for (auto& servo : servo_)
      {
        servo->setVel(toEncoderUnit(axisVelCmd_));
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
         return false;
       }
     }
     return true;
}

bool Axis::powerOn()
{
    for (auto& servo : servo_)
    {
      if(servo->enable())
      {
        return true;
      }

    }
    return false;
}

bool Axis::powerOff()
{
    for (auto& servo : servo_)
    {
      if (!servo->disable())
      {
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
