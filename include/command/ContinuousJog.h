/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 连续点动指令
 */
#ifndef CONTINUOUSJOG_H
#define CONTINUOUSJOG_H
#include "system/basenodeInterface.h"
#include <array>
#include <ruckig/ruckig.hpp>
#include <vector>
#include "system/nodeCommunication.h"
#include "system/nodeFactory.h"

using namespace ruckig;

class ContinuousJog : public zrcsSystem::OutputNode
{
private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;            
    double targetVelocity_;
    double setCurrentPosition_; 
    double lastVelocity_;
    double lastAcceleration_;
    bool accelerateStart_;
    bool decelerateStart_;
    
public:
    ContinuousJog() : otg_(cycletime * 0.001), 
                      targetVelocity_(0), 
                      setCurrentPosition_(0),
                      lastVelocity_(0), 
                      lastAcceleration_(0),
                      accelerateStart_(true), 
                      decelerateStart_(true)
    {
    }

    void init() override
    {     
        // 从共享内存中加载手动位置数据
        input_.control_interface = ruckig::ControlInterface::Velocity;                     
        input_.max_acceleration[0] = controller_->axiss[ContinueMotion.load().axisId]->getMaxAcceleration();
        input_.max_jerk[0] = controller_->axiss[ContinueMotion.load().axisId]->getMaxJerk();
    }
    
    void accelerate()
    {     
        if (accelerateStart_ == true) 
        {
            input_.current_position[0] = controller_->axiss[ContinueMotion.load().axisId]->actualPos();       
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
            if (controller_ != nullptr && controller_->axiss.size() > ContinueMotion.load().axisId) 
            {
                controller_->axiss[ContinueMotion.load().axisId]->setAxisPositionCmd(p[0]);
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
    
    void uniformSpeed()
    {     
        setCurrentPosition_ = setCurrentPosition_ + targetVelocity_ * cycletime * 0.001;
        lastVelocity_ = targetVelocity_;
        lastAcceleration_ = 0;
        controller_->axiss[ContinueMotion.load().axisId]->setAxisPositionCmd(setCurrentPosition_);
    }
    
    void decelerate()
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
            if (controller_ != nullptr && controller_->axiss.size() > ContinueMotion.load().axisId) 
            {
                controller_->axiss[ContinueMotion.load().axisId]->setAxisPositionCmd(p[0]);
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

    void run(void) override
    {                        
        targetVelocity_ = double(MultiPlied.load() / 100.0) * controller_->axiss[ContinueMotion.load().axisId]->getMaxVelocity();
        if (ContinueMotion.load().direction == false) 
        {
            targetVelocity_ = -targetVelocity_;
        }
        
        if (ContinueMotion.load().motion == true)                                                               
        { 
            decelerateStart_ = true;
            accelerate();                       
        }
        else if (ContinueMotion.load().motion == false)
        {
            accelerateStart_ = true;
            decelerate();
        }                       
    }
};

REGISTEROUTPUT(ContinuousJog);

#endif
