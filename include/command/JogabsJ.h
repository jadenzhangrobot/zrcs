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
#include <ruckig/ruckig.hpp>
#include <string>
#include <unistd.h>
#include "system/classfactory.h"
using namespace ruckig;
class JogabsJ:public ZrcsSystem::Basenode
  {

             public:
             Ruckig<1> otg {0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             int motor_id;
             double velocity;
             double acceleration;
             double position;
             double jerk;
            JogabsJ()
            {
              port_input.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 1000));
              port_input.add<double>("position", 'p', "servo position", false, 0, cmdline::range(-2000.000, 2000.000));
              port_input.add<double>("velocity", 'v', "servo velocity", false, 10, cmdline::range(-1000.0, 1000.0));
              port_input.add<double>("acceleration", 'a', "servo acceleration", false, 10, cmdline::range(-1000.0, 1000.0));
              port_input.add<double>("jerk", 'j', "servo jerk", false, 10, cmdline::range(-1000.0, 1000.0));
            }    
;
       void config() override
       {
           if (!cmdParam.empty()) 
              {
                  std::string str=cmdParam.front();
                  port_input.parse_check(str);
                  cmdParam.pop();
              }  
              position=port_input.get<double>("position"); 
              velocity=port_input.get<double>("velocity");
              acceleration= port_input.get<double>("acceleration");
              jerk=port_input.get<double>("jerk");
              motor_id=port_input.get<int>("motor");     
       }

       void init() override
       {          
              input.current_position[0]=control->motors[motor_id]->actualPos();       
              input.current_velocity[0]= 0;
              input.current_acceleration[0] =0;                               
              input.target_position[0]=position;
              input.target_velocity[0] =0;
              input.target_acceleration[0] =0;
              input.max_velocity[0] =velocity;
              input.max_acceleration[0] =acceleration;
              input.max_jerk[0] =jerk;
              node_status=RUNNING;                          
    }
           
      void  excuteRt(void) override
      {                               
                     if(otg.update(input, output) == Result::Working)            
                      {                        
                        auto& p = output.new_position;
                        control->motors[motor_id]->setTargetPos(p[0]);                                                                                        
                        output.pass_to_input(input);
                         //rt_printf("---  %lf\n",(p[0]));                             
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
             rt_printf("JogAbsj 执行成功\n");
             node_status=EXIT;
      }
     
  };

 REGISTERCMD(JogabsJ);

#endif