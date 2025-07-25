/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef JOGABSJ_H
#define JOGABSJ_H

#include "system/basenodeInterface.h"
#include <array>
#include <ruckig/ruckig.hpp>
#include <string>
#include <vector>
#include "system/nodeFactory.h"

using namespace ruckig;
class JogabsJ:public zrcsSystem::OneShotNode
  {

             public:
             Ruckig<1> otg {0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             int axisId=1;
             double velocity;
             double acceleration;
             double position;
             double jerk;
             std::pmr::vector<double> po;
             std::pmr::vector<double> ve;
             std::pmr::vector<double> aa;
            JogabsJ()
            {
              port_input.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 1000));
              port_input.add<double>("position", 'p', "servo position", false, 0, cmdline::range(-2000.000, 2000.000));
              port_input.add<double>("velocity", 'v', "servo velocity", false, 10, cmdline::range(-1000.0, 1000.0));
              port_input.add<double>("acceleration", 'a', "servo acceleration", false, 10, cmdline::range(-1000.0, 1000.0));
              port_input.add<double>("jerk", 'j', "servo jerk", false, 10, cmdline::range(-1000.0, 1000.0));
            }    
;
       void init() override
       {     
             if(!cmdParam.empty())
             {
                   port_input.parse_check(cmdParam);
             }            
              position=port_input.get<double>("position"); 
              velocity=port_input.get<double>("velocity");
              acceleration= port_input.get<double>("acceleration");
              jerk=port_input.get<double>("jerk");
              axisId=port_input.get<int>("motor");

              input.current_position[0]=0;       
              input.current_velocity[0]= 0;
              input.current_acceleration[0] =0;                               
              input.target_position[0]=position;
              input.target_velocity[0] =0;
              input.target_acceleration[0] =0;
              input.max_velocity[0] =velocity;
              input.max_acceleration[0] =acceleration;
              input.max_jerk[0] =jerk;
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
                          po.push_back(p[0]);
                          ve.push_back(v[0]);
                          aa.push_back(a[0]);                        
                        }
                                                
                       }
                     else if(otg.update(input, output)==Result::Finished)
                      {
                        po.clear();
                        ve.clear();
                        aa.clear();
                        SetOneShotStatus(zrcsSystem::OneShotNodeStatus::EXIT);
                      }
                     else
                      {                         
                        SetOneShotStatus(zrcsSystem::OneShotNodeStatus::FAILED);
                      }
        
      }
      void exit(void) override
      {
            
      }
     
  };

 REGISTERCMD(JogabsJ);

#endif