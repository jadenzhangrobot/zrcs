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
class ContinuousJog:public zrcsSystem::PersistentNode
  {

             public:
             Ruckig<1> otg {0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             int axisId;
             double velocity;
             double acceleration;
             double position;
             double jerk;
             std::pmr::vector<double> po;
             std::pmr::vector<double> ve;
             std::pmr::vector<double> aa;
            ContinuousJog()
            {

            }    
;
       void init() override
       {     
            SingleAxisMotion  sam;
           if  (rtProcess->shared_block_->manualPositionQueue.pop(sam))
            { 
              axisId=sam.axisId;   
              input.current_position[0]=control->axiss[axisId]->actualPos();       
              input.current_velocity[0]= control->axiss[axisId]->actualVel();
              input.current_acceleration[0]=control->axiss[axisId]->actualAcc();                               
              input.target_position[0]=sam.position;
              input.target_velocity[0] =sam.velocity;
              input.target_acceleration[0] =sam.acceleration;
              input.max_velocity[0] =2;
              input.max_acceleration[0] =1;
              input.max_jerk[0] =0.5;
            }
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
                          std::cout<<"p--- "<<p[0]<<std::endl;
                          po.push_back(p[0]);
                          ve.push_back(v[0]);
                          aa.push_back(a[0]);                        
                        }
                                                
                       }
                     else if(otg.update(input, output)==Result::Finished)
                      {
                        //po.clear();
                       // ve.clear();
                       // aa.clear();
                       SetPersistentStatus(zrcsSystem::PersistentNodeStatus::RTEXIT);
                        //SetOneShotStatus(zrcsSystem::OneShotNodeStatus::EXIT);
                      }
                     else if(otg.update(input, output)==Result::ErrorInvalidInput) 
                      {
                        SetPersistentStatus(zrcsSystem::PersistentNodeStatus::RTINIT);
                      }
                     else
                      { 
                        std::cout<<"status--- "<<otg.update(input, output)<<std::endl;                     
                        //SetPersistentStatus(zrcsSystem::PersistentNodeStatus::FAILED);
                      }
        
      }
      void exit(void) override
      {
           
      }
     
  };

 REGISTERNODE(ContinuousJog);

#endif