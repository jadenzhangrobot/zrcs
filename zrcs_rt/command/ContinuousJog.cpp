/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 连续点动指令
 */
#include "command/ContinuousJog.h"
#include <limits>

void ContinuousJog::init()
{
    // 从共享内存中加载手动位置数据
    input_.control_interface = ruckig::ControlInterface::Velocity;
    input_.max_acceleration[0] = controller_->axiss[shm()->jogCtrl.axisId.load()]->getMaxAcceleration();
    input_.max_jerk[0] = controller_->axiss[shm()->jogCtrl.axisId.load()]->getMaxJerk();
}

void ContinuousJog::accelerate(int axisId)
{
    if (accelerateStart_ == true)
    {
        input_.current_position[0] = controller_->axiss[axisId]->actualPos();
        input_.current_velocity[0] = lastVelocity_;
        input_.current_acceleration[0] = lastAcceleration_;
        input_.target_velocity[0] = targetVelocity_;
        input_.target_acceleration[0] = 0;
        accelerateStart_ = false;
    }

    // 倍率变化时平滑过渡到新速度
    if (input_.target_velocity[0] != targetVelocity_)
    {
        input_.target_velocity[0] = targetVelocity_;
    }

    auto status = otg_.update(input_, output_);
    if(status == Result::Working || status == Result::Finished)
    {
        auto& p = output_.new_position;
        auto& v = output_.new_velocity;
        auto& a = output_.new_acceleration;
        if (controller_ != nullptr && controller_->axiss.size() > static_cast<size_t>(axisId))
        {
            controller_->axiss[axisId]->setAxisPositionCmd(p[0]);
            lastVelocity_ = v[0];
            lastAcceleration_ = a[0];
            output_.pass_to_input(input_);
            setCurrentPosition_ = p[0];
        }
    }
}

void ContinuousJog::decelerate(int axisId)
{
    if (decelerateStart_ == true)
    {
        input_.current_position[0] = setCurrentPosition_;
        input_.current_velocity[0] = lastVelocity_;
        input_.current_acceleration[0] = lastAcceleration_;
        input_.target_velocity[0] = 0;
        input_.target_acceleration[0] = 0;
        // 使用最大加速度减速，并设置极大jerk以近似无jerk限制
        input_.max_acceleration[0] = controller_->axiss[axisId]->getMaxAcceleration();
        input_.max_jerk[0] = std::numeric_limits<double>::max();
        decelerateStart_ = false;
    }

    auto status = otg_.update(input_, output_);
    if(status == Result::Working)
    {
        auto& p = output_.new_position;
        auto& v = output_.new_velocity;
        auto& a = output_.new_acceleration;
        if (controller_ != nullptr && controller_->axiss.size() > static_cast<size_t>(axisId))
        {
            controller_->axiss[axisId]->setAxisPositionCmd(p[0]);
            lastVelocity_ = v[0];
            lastAcceleration_ = a[0];
            output_.pass_to_input(input_);
        }
    }
    if (status == Result::Finished)
    {
        // 恢复正常的加速度和jerk限制，供下次加速使用
        input_.max_acceleration[0] = controller_->axiss[axisId]->getMaxAcceleration();
        input_.max_jerk[0] = controller_->axiss[axisId]->getMaxJerk();
        decelerateStart_ = true;
        stopped_ = true;
    }
}

void ContinuousJog::run(void)
{
    int axisId = shm()->jogCtrl.axisId.load(std::memory_order_acquire);

    targetVelocity_ = shm()->overrideRatio.load(std::memory_order_acquire) * controller_->axiss[axisId]->getMaxVelocity();
    if (shm()->jogCtrl.direction.load() == false)
    {
        targetVelocity_ = -targetVelocity_;
    }

    if (shm()->jogCtrl.active.load() == true)
    {
        if (stopped_)
        {
            setCurrentPosition_ = controller_->axiss[axisId]->actualPos();
            lastVelocity_ = 0;
            lastAcceleration_ = 0;
            stopped_ = false;

            input_.max_acceleration[0] = controller_->axiss[axisId]->getMaxAcceleration();
            input_.max_jerk[0] = controller_->axiss[axisId]->getMaxJerk();
        }
        decelerateStart_ = true;
        accelerate(axisId);
    }
    else if (shm()->jogCtrl.active.load() == false)
    {
        if (stopped_)
        {
            return;
        }
        accelerateStart_ = true;
        decelerate(axisId);
    }
}

REGISTEROUTPUT(ContinuousJog);
