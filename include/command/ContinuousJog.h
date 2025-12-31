/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
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
class ContinuousJog:public zrcsSystem::OutputNode
  {

             public:
             Ruckig<1> otg {cycletime*0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             double targetVelocity=0;
             double setCurrentPosition; 
             double lastVelocity=0;
             double lastAcceleration=0;
             bool accelerateStart=true;
             bool decelerateStart=true;
            ContinuousJog()
            {

            }    
;
       void init() override
       {     
            // 从共享内存中加载手动位置数据
              input.control_interface = ruckig::ControlInterface::Velocity;                     
              input.max_acceleration[0] =control->axiss[ContinueMotion.load().axisId]->getMaxAcceleration();
              input.max_jerk[0] =control->axiss[ContinueMotion.load().axisId]->getMaxJerk();
       }
       void accelerate()
       {     
                
            if (accelerateStart==true) 
            {
              input.current_position[0]=control->axiss[ContinueMotion.load().axisId]->actualPos();       
              input.current_velocity[0]= lastVelocity;
              input.current_acceleration[0]=lastAcceleration;
              input.target_velocity[0] =targetVelocity;
              input.target_acceleration[0] =0;
              accelerateStart=false;
            }
      
              auto status  = otg.update(input, output);  
              if(status==Result::Working)            
              {                        
                auto& p = output.new_position;
                auto& v=output.new_velocity;
                auto& a=output.new_acceleration;
                if (control!=nullptr&&control->axiss.size()>ContinueMotion.load().axisId) 
                {
                  control->axiss[ContinueMotion.load().axisId]->setAxisPositionCmd(p[0]);
                  lastVelocity=v[0];
                  lastAcceleration=a[0];
                  output.pass_to_input(input);
                  setCurrentPosition=p[0];                                                                   
                } 
                
              }
              else if(status==Result::Finished)
              {
                  uniformSpeed();                        
              }
       }
       void uniformSpeed()
       {     
            setCurrentPosition=setCurrentPosition+targetVelocity*cycletime*0.001;
            lastVelocity=targetVelocity;
            lastAcceleration=0;
            control->axiss[ContinueMotion.load().axisId]->setAxisPositionCmd(setCurrentPosition);
            
       }
       void decelerate()
       {
                   
                    if (decelerateStart==true)
                    {
                      input.current_position[0]=setCurrentPosition;       
                      input.current_velocity[0]= lastVelocity;
                      input.current_acceleration[0]= lastAcceleration;
                      input.target_velocity[0] =0;
                      input.target_acceleration[0] =0;
                      decelerateStart=false;
                    }
                      auto status  = otg.update(input, output);  
                      if(status==Result::Working)            
                      {                        
                        auto& p = output.new_position;
                        auto& v=output.new_velocity;
                        auto& a=output.new_acceleration;
                        if (control!=nullptr&&control->axiss.size()>ContinueMotion.load().axisId) 
                        {
                          control->axiss[ContinueMotion.load().axisId]->setAxisPositionCmd(p[0]);
                          lastVelocity=v[0];
                          lastAcceleration=a[0];
                          output.pass_to_input(input);                                                                    
                        }                                  
                      }
                      if (status==Result::Finished)
                      {
                        decelerateStart=true;
                      }
       }

  
           
      void  run(void) override
      {                        
            
                  targetVelocity = double(MultiPlied.load()/100.0) * control->axiss[ContinueMotion.load().axisId]->getMaxVelocity();
                  if (ContinueMotion.load().direction==false) 
                  {
                      targetVelocity=-targetVelocity;
                  }
                
                  if (ContinueMotion.load().motion==true)                                                               
                  { 
                      decelerateStart=true;
                      accelerate();                       
                  }
                  else if (ContinueMotion.load().motion==false)
                  {
                      accelerateStart=true;
                      decelerate();
                  }                       
      }
  };
 REGISTEROUTPUT(ContinuousJog);

#endif