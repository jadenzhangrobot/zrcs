/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-06-16 09:54:19
 * @Description: 直线运动指令
 */
#ifndef GO1_H
#define GO1_H

#include "system/base/BaseNodeInterface.h"
#include "model/ModeInterface.h"
#include <array>
#include <ruckig/ruckig.hpp>
#include <string>
#include <vector>
#include "system/nodeFactory.h"
#include <cmath>
#include <zmq.hpp>

using namespace ruckig;
class GO1 : public zrcsSystem::CmdNode 
{
    double x;
    double y;
    double z;
    double a;
    double b;
    double c;
    double f;
    Ruckig<1> otg {cycletime*0.001}; 
    InputParameter<1> input;
    OutputParameter<1> output;    
public:
  GO1()
  {
     std::strcpy(nodeName,"GO1");
  }
  
  void init() override
  {  
              x=command->args[G01X];       
              y=command->args[G01Y];       
              z=command->args[G01Z];       
              a=command->args[G01A];       
              b=command->args[GO1B];       
              c=command->args[GO1C];       
              f=command->args[GO1F];       
              axisId=command->args[JogabsjAxisId];       
              position=command->args[JogabsjTargetPosition];
              input.current_position[0]=control->axiss[axisId]->actualPos();       
              input.current_velocity[0]= 0;
              input.current_acceleration[0] =0;                               
              input.target_position[0]=position;
              input.target_velocity[0] =0;
              input.target_acceleration[0] =0;
              input.max_velocity[0] =control->axiss[axisId]->getMaxVelocity();
              input.max_acceleration[0] =control->axiss[axisId]->getMaxAcceleration();
              input.max_jerk[0] =control->axiss[axisId]->getMaxJerk();  
  }

   void run(void) override
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
REGISTERCMD(GO1)

#endif