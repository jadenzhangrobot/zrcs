/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef TEST_H
#define TEST_H

#include "system/basenodeInterface.h"
#include <ruckig/ruckig.hpp>
#include <string>
#include <unistd.h>
#include "system/classfactory.h"
#include <random>
using namespace ruckig;
class Test:public ZrcsSystem::Basenode
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
            std::mt19937 gen;
            Test()
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
           
              
              // gen(rd()); // 使用 Mersenne Twister 引擎
             // std::uniform_int_distribution<int> dis(1, 10000); // 生成 1 到 100 之间的随机整数
    // 生成随机数
        
             node_status=RUNNING;                  
    }
           
      void  excuteRt(void) override
      {      
                static double t = 0;

                   static double v;
                   static double a;
                   static double x;
                    

                 ///   int random_number = dis(gen); 
                    
                  
                

                    if (t > 1000000000000) {
                    node_status = SUCCESS;
                    }
                        
                        input.current_position[0]=x;       
                        input.current_velocity[0]= v;
                        input.current_acceleration[0] =a;                               
                        input.target_position[0]=sin(t);
                        input.target_velocity[0] =0;
                        input.target_acceleration[0] =0;
                        input.max_velocity[0] =2;
                        input.max_acceleration[0] =10;
                        input.max_jerk[0] =1000;
                        //node_status=RUNNING;         
                          
                     if(otg.update(input, output) == Result::Working)            
                      {                        
                        auto& p = output.new_position;
                        //control->motors[motor_id]->setTargetPos(p[0]);                                                                                        
                        // output.pass_to_input(input);
                          v = output.new_velocity[0];
                          a = output.new_acceleration[0];
                          x = output.new_position[0];
                         rt_printf("---  %lf\n",x - sin(t));                             
                       }
                     else if(otg.update(input, output)==Result::Finished)
                      {
                        // node_status=SUCCESS;
                      }
                     else
                      {                         
                        node_status=FAILURE;                        
                      }
                        t = t + 0.001;
        
      }
      void exit(void) override
      {
             rt_printf("JogAbsj 执行成功\n");
             node_status=EXIT;
      }
     
  };

 REGISTERCMD(Test);

#endif