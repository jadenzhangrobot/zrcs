/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef JOGABSJ_H
#define JOGABSJ_H

#include "system/basenode.h"
#include "system/centre.h"
#include <cstring>
#include <iostream>
#include <ostream>
#include <ruckig/ruckig.hpp>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include "system/classfactory.h"
using namespace ruckig;

class JogabsJ:public zrcs_system::Basenode
  {
    public:
             zrcs_system::centre& cenobj=zrcs_system::centre::getInstance();
             Ruckig<1> otg {0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             int motor_id;
      JogabsJ()
      {
          node_name="JogabsJ";
          motor_id=0;
      }

       void init() override
       {             
         
         port_input.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 100));
         port_input.add<double>("position", 'p', "servo position", false, 0, cmdline::range(-20.000, 20.000));
         port_input.add<double>("velocity", 'v', "servo velocity", false, 1, cmdline::range(-10.0, 10.0));
         port_input.add<double>("acceleration", 'a', "servo acceleration", false, 0.5, cmdline::range(-10.0, 10.0));
         port_input.add<double>("jerk", 'j', "servo jerk", false, 0.5, cmdline::range(-10.0, 10.0));
          if (!cenobj.nrt_cmdParam.empty()) 
          {
              std::string str=cenobj.nrt_cmdParam.front();
              port_input.parse_check(str);
              cenobj.nrt_cmdParam.pop();
          }   
              input.current_position[0]=cenobj.ec_control->motors[port_input.get<int>("motor")]->actualPos();              
              input.current_velocity[0]= 0;
              input.current_acceleration[0] =0;                               
              input.target_position[0]=port_input.get<double>("position");
              input.target_velocity[0] =0;
              input.target_acceleration[0] =0;
              input.max_velocity[0] =port_input.get<double>("velocity");
              input.max_acceleration[0] =port_input.get<double>("acceleration");
              input.max_jerk[0] =port_input.get<double>("jerk");
              motor_id=port_input.get<int>("motor");
              node_status=RUNNING;                 
          
    }
  
      void  excute_rt(void) override
      {         
                                      
                     if(otg.update(input, output) == Result::Working)            
                      {                        
                        auto& p = output.new_position;
                        cenobj.ec_control->motors[motor_id]->setTargetPos(p[0]);                                                                                        
                        output.pass_to_input(input);                              
                       }
                     else if(otg.update(input, output)==Result::Finished)
                      {
                        node_status=SUCCESS; 
                      }
                     else
                      {                         
                        node_status=FAILURE;                        
                      }
         
      }
      void exit(void) override
      {
         spdlog::info(node_name+"  finished");
      }
     
  };

 REGISTER(JogabsJ);

#endif