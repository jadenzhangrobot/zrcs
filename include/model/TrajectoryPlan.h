#ifndef TRAJECTORYPLAN_H
#define TRAJECTORYPLAN_H
#include <ruckig/ruckig.hpp>
#include "system/basenodeInterface.h"
using namespace ruckig;
class TraPlan:public zrcsSystem::Basenode
{
      TraPlan()
      {
             Ruckig<6> otg {0.001}; 
             InputParameter<6> input;
             OutputParameter<6> output;      
      }
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

#endif