/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 连续点动指令
 */
#include "command/ContinuousJog.h"

void ContinuousJog::init()
{
    // 从共享内存中加载手动位置数据
    input_.control_interface = ruckig::ControlInterface::Velocity;
    input_.max_acceleration[0] = controller_->axiss[shm().continueMotion().axisId.load()]->getMaxAcceleration();
    input_.max_jerk[0] = controller_->axiss[shm().continueMotion().axisId.load()]->getMaxJerk();
}

void ContinuousJog::accelerate()
{
    if (accelerateStart_ == true)
    {
        input_.current_position[0] = controller_->axiss[shm().continueMotion().axisId.load()]->actualPos();
        input_.current_velocity[0] = lastVelocity_;
        input_.current_acceleration[0] = lastAcceleration_;
        input_.target_velocity[0] = targetVelocity_;
        input_.target_acceleration[0] = 0;
        accelerateStart_ = false;
    }

    auto status = otg_.update(input_, output_);
    if(status == Result::Working)
    {
        auto& p = output_.new_position;
        auto& v = output_.new_velocity;
        auto& a = output_.new_acceleration;
        if (controller_ != nullptr && controller_->axiss.size() > shm().continueMotion().axisId.load())
        {
            controller_->axiss[shm().continueMotion().axisId.load()]->setAxisPositionCmd(p[0]);
            lastVelocity_ = v[0];
            lastAcceleration_ = a[0];
            output_.pass_to_input(input_);
            setCurrentPosition_ = p[0];
        }
    }
    else if(status == Result::Finished)
    {
        uniformSpeed();
    }
}

void ContinuousJog::uniformSpeed()
{
    setCurrentPosition_ = setCurrentPosition_ + targetVelocity_ * cycletime * 0.001;
    lastVelocity_ = targetVelocity_;
    lastAcceleration_ = 0;
    controller_->axiss[shm().continueMotion().axisId.load()]->setAxisPositionCmd(setCurrentPosition_);
}

void ContinuousJog::decelerate()
{
    if (decelerateStart_ == true)
    {
        input_.current_position[0] = setCurrentPosition_;
        input_.current_velocity[0] = lastVelocity_;
        input_.current_acceleration[0] = lastAcceleration_;
        input_.target_velocity[0] = 0;
        input_.target_acceleration[0] = 0;
        decelerateStart_ = false;
    }

    auto status = otg_.update(input_, output_);
    if(status == Result::Working)
    {
        auto& p = output_.new_position;
        auto& v = output_.new_velocity;
        auto& a = output_.new_acceleration;
        if (controller_ != nullptr && controller_->axiss.size() > shm().continueMotion().axisId.load())
        {
            controller_->axiss[shm().continueMotion().axisId.load()]->setAxisPositionCmd(p[0]);
            lastVelocity_ = v[0];
            lastAcceleration_ = a[0];
            output_.pass_to_input(input_);
        }
    }
    if (status == Result::Finished)
    {
        decelerateStart_ = true;
    }
}

void ContinuousJog::run(void)
{
    targetVelocity_ = shm().overrideRatio().load(std::memory_order_acquire) * controller_->axiss[shm().continueMotion().axisId.load()]->getMaxVelocity();
    if (shm().continueMotion().direction.load() == false)
    {
        targetVelocity_ = -targetVelocity_;
    }

    if (shm().continueMotion().motion.load() == true)
    {
        decelerateStart_ = true;
        accelerate();
    }
    else if (shm().continueMotion().motion.load() == false)
    {
        accelerateStart_ = true;
        decelerate();
    }
}

REGISTEROUTPUT(ContinuousJog);
