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
#include "system/nodeFactory.h"

using namespace ruckig;
class ContinuousJog:public zrcsSystem::OutputPlcNode
  {

             public:
             Ruckig<1> otg {cycletime*0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             int axisId=0;
             double maxVelocity=4;
             double targetVelocity=0;
             double setCurrentPosition; 
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
              input.max_acceleration[0] =20;
              input.max_jerk[0] =250;
       }
       void accelerate()
       {     
                
            if (accelerateStart==true) 
            {
              input.current_position[0]=control->axiss[axisId]->actualPos();       
              input.current_velocity[0]= control->axiss[axisId]->actualVel();
              input.current_acceleration[0]=control->axiss[axisId]->actualAcc();
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
                if (control!=nullptr&&control->axiss.size()>axisId) 
                {
                  control->axiss[axisId]->setAxisPositionCmd(p[0]);
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
            control->axiss[axisId]->setAxisPositionCmd(setCurrentPosition);
            setCurrentPosition=setCurrentPosition+targetVelocity*cycletime*0.001;
       }
       void decelerate()
       {
                   
                    if (decelerateStart==true)
                    {
                      input.current_position[0]=setCurrentPosition;       
                      input.current_velocity[0]= control->axiss[axisId]->actualVel();
                      input.current_acceleration[0]=control->axiss[axisId]->actualAcc();
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
                        if (control!=nullptr&&control->axiss.size()>axisId) 
                        {
                          control->axiss[axisId]->setAxisPositionCmd(p[0]);
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
                  SingleAxisMotion sam = rtProcess->shared_block_->manualPosition.load();
                  targetVelocity = double(sam.Multiplied)/100.0 * maxVelocity;
                  if (sam.direction==false) 
                  {
                      targetVelocity=-targetVelocity;
                  }
                
                  if (sam.motion==true)                                                               
                  { 
                      decelerateStart=true;
                      accelerate();                       
                  }
                  else if (sam.motion==false)
                  {
                      accelerateStart=true;
                      decelerate();
                  }                       
      }
      void exit(void) override
      {
           
      }
     
  };
 REGISTEROUTPUTPLCNODE(ContinuousJog);

#endif