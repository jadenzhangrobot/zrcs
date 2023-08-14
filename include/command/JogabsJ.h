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
#include <iostream>
#include <ostream>
#include <ruckig/ruckig.hpp>
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
       bool init() override
       {        
         cmdline::parser cmd;
         cmd.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 100));
         cmd.add<double>("position", 'p', "servo position", false, 0, cmdline::range(-20.000, 20.000));
         cmd.add<double>("velocity", 'v', "servo velocity", false, 300, cmdline::range(-10, 10));
         cmd.add<double>("acceleration", 'a', "servo acceleration", false, 50, cmdline::range(-10, 10));
          if (!cenobj.nrt_cmdParam.empty()) 
          {
              std::string str=cenobj.nrt_cmdParam.front();
              cmd.parse_check(str);
              cenobj.nrt_cmdParam.pop();
          }   
              input.current_position[0]=cenobj.ec_control->motors[cmd.get<int>("motor")]->actualPos();              
              input.current_velocity[0]= 0;
              input.current_acceleration[0] =0;
                               
              input.target_position[0]=cmd.get<double>("position");
              input.target_velocity[0] = 0;
              input.target_acceleration[0] =0;
              input.max_velocity[0] = 1;
              input.max_acceleration[0] = 0.5;
              input.max_jerk[0] =0.5;
              motor_id=cmd.get<int>("motor"); 
                     
           return true;
    }
  
      void  excute_rt(void) override
      {         
                                      
                     if(otg.update(input, output) == Result::Working)            
                      {                        
                        auto& p = output.new_position;
                        cenobj.ec_control->motors[motor_id]->setTargetPos(p[0]);                                                                                        
                        output.pass_to_input(input); 
                        rtnode_status=RUNNING;                  
                       }
                     else
                      {                         
                         rtnode_status=SUCCESS;          
                      }
         
      }
  };

 REGISTER(JogabsJ);

#endif