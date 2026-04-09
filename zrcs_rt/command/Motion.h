/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#pragma once

#include "system/base/BaseNodeInterface.h"
#include <cstdint>
#include <ruckig/ruckig.hpp>
//#include <spdlog/spdlog.h>
#include <string>
#include <unistd.h>
#include "system/classfactory.h"
#include "timer.h"
using namespace ruckig;

class Motion:public zrcsSystem::Basenode
  {
         
             int motor_id;
             struct motion{
             double velocity;
             double acceleration;
             double Tposition;
             double Cposition;
             };
             Ruckig<1> otg {0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;
             std::pmr::monotonic_buffer_resource pool;
             std::pmr::vector<motion> RtToNrt;
        public:
        Motion():pool(10000),RtToNrt(&pool)
        {
         port_input.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 1000));
         port_input.add<double>("Tposition", 'p', "servo position", false, 0, cmdline::range(-2000.000, 2000.000));
         port_input.add<double>("Cposition", 'C', "servo position", false, 0, cmdline::range(-2000.000, 2000.000));
         port_input.add<double>("velocity", 'v', "servo velocity", false, 104, cmdline::range(-1000.0, 1000.0));
         port_input.add<double>("acceleration", 'a', "servo acceleration", false, 1000, cmdline::range(-1000.0, 1000.0));
         port_input.add<double>("jerk", 'j', "servo jerk", false, 5000, cmdline::range(-1000.0, 1000.0));            
        }

     void config() override
     {
          if (!cmdParam.empty()) 
          {
              std::string str=cmdParam.front();
              port_input.parse_check(str);
              cmdParam.pop();
          }
              motion mo;
              mo.Tposition= port_input.get<double>("Tposition"); 
              mo.Cposition= port_input.get<double>("Cposition"); 
              mo.velocity=port_input.get<double>("velocity");
              mo.acceleration= port_input.get<double>("acceleration");
              motor_id=port_input.get<int>("motor"); 
              RtToNrt.push_back(mo);  

      }
       void init() override
       {          
              motion m= RtToNrt.front();
              input.current_position[0]=m.Cposition;       
              input.current_velocity[0]= 0;
              input.current_acceleration[0] =0;                               
              input.target_position[0]=m.Tposition;
              input.target_velocity[0] =0;
              input.target_acceleration[0] =0;
              input.max_velocity[0] =m.velocity;
              input.max_acceleration[0] =m.acceleration;
              input.max_jerk[0] =20000; 
              node_status=RUNNING;                                    
       }
  
      void  excuteRt(void) override
      {         
                      auto a=otg.update(input, output);
                      if(a == Result::Working)            
                      {                        
                        auto& p = output.new_position;
                        control->motors[0]->setTargetPos(p[0]);                                                                                        
                        output.pass_to_input(input);                         
                       }
                     else if(a==Result::Finished)
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
            // uint64_t a= rt_timer_read();
            // rt_printf("motion 执行成功\n");
             RtToNrt.erase(RtToNrt.begin());
             // uint64_t b= rt_timer_read();
              
             node_status=EXIT;
      }
     
  };

 REGISTERCMD(Motion);

