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
#include <iostream>
#include <ruckig/ruckig.hpp>
#include <vector>
#include "system/nodeFactory.h"

using namespace ruckig;
class ContinuousJog:public zrcsSystem::PersistentNode
  {

             public:
             Ruckig<1> otg {0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             int axisId=0;
             double lastVelocity=0;
             double lastAcceleration=0;
             std::pmr::vector<double> po;
             std::pmr::vector<double> ve;
             std::pmr::vector<double> aa;
               int flog=0;
            ContinuousJog()
            {

            }    
;
       void init() override
       {     
            // 从共享内存中加载手动位置数据
              input.control_interface = ruckig::ControlInterface::Velocity;        
              input.max_acceleration[0] =100;
              input.max_jerk[0] =100;
       }
       void accelerate()
       {
          
             if (flog==0) {
              input.current_position[0]=control->axiss[axisId]->actualPos();       
              input.current_velocity[0]= control->axiss[axisId]->actualVel();
              input.current_acceleration[0]=control->axiss[axisId]->actualAcc();
              input.target_velocity[0] =1;
              input.target_acceleration[0] =0;
              flog=1;
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
                      else if(status==Result::Finished)
                      {
                          uniformSpeed();                        
                      }
       }
       void uniformSpeed()
       {
           
           control->axiss[axisId]->setAxisPositionCmd(control->axiss[axisId]->actualPos()+0.001);

       }
       void decelerate()
       {
                    static int flog_=0;
                    if (flog_==0) {
                      input.current_position[0]=control->axiss[axisId]->actualPos();       
                      input.current_velocity[0]= control->axiss[axisId]->actualVel();
                      input.current_acceleration[0]=control->axiss[axisId]->actualAcc();
                      input.target_velocity[0] =0;
                      input.target_acceleration[0] =0;
                      flog_=1;
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
                         
                          //aa.push_back(a[0]);                                              
                        }                                  
                      }
                      if (status==Result::Finished) {
                        flog_=0;
                      }
       }

  
           
      void  run(void) override
      {        
                          po.push_back(control->axiss[axisId]->actualPos());
                          ve.push_back(control->axiss[axisId]->actualVel());
                  SingleAxisMotion sam = rtProcess->shared_block_->manualPosition.load();
                    if (sam.SteppingDistance==1)                                                               
                    { 
                        accelerate();
                        
                    }
                    else if (sam.SteppingDistance==0)
                    {
                        flog=0;
                        decelerate();
                    }
                        
      }
      void exit(void) override
      {
           
      }
     
  };

 REGISTERNODE(ContinuousJog);

#endif