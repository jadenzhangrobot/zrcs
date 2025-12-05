/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef JOGJ_H
#define JOG_H

#include "common/config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include <array>
#include <ruckig/ruckig.hpp>
#include <string>
#include <vector>
#include "system/nodeFactory.h"

using namespace ruckig;
class JogJ:public zrcsSystem::CmdNode
  {

             public:
             Ruckig<1> otg {cycletime*0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             int axisId;
             double position;
            JogJ()
            {
                std::strcpy(nodeName,"JogJ");
            }    
;
       void init() override
       { 
             
              axisId=command->args[JogjAxisId];       
              position=command->args[JogjTargetPosition];
              input.current_position[0]=control->axiss[axisId]->actualPos();       
              input.current_velocity[0]= 0;
              input.current_acceleration[0] =0;                               
              input.target_position[0]=position+control->axiss[axisId]->actualPos();
              input.target_velocity[0] =0;
              input.target_acceleration[0] =0;
              input.max_velocity[0] =control->axiss[axisId]->getMaxVelocity();
              input.max_acceleration[0] =control->axiss[axisId]->getMaxAcceleration();
              input.max_jerk[0] =control->axiss[axisId]->getMaxJerk();
       }

  
           
      void  run(void) override
      {                               
                     if(otg.update(input, output) == Result::Working)            
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
                     else if(otg.update(input, output)==Result::Finished)
                      {
                        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
                      }
                     else
                      {                         
                        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
                      }
        
      }
      void exit(void) override
      {
            
      }
     
  };

 REGISTERCMD(JogJ);

#endif