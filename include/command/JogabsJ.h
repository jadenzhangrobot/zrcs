/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-06-10 15:24:43
 * @Description: 关节运动绝对位置指令
 */
#ifndef JOGABSJ_H
#define JOGABSJ_H

#include "system/basenode.h"
#include "system/centre.h"
#include <ruckig/ruckig.hpp>
#include "system/classfactory.h"
using namespace ruckig;

class JogabsJ:zrcs_system::Basenode
  {
    public:
            zrcs_system::centre& cenobj=zrcs_system::centre::getInstance();
            Ruckig<6> otg {0.001}; 
            InputParameter<6> input;
            OutputParameter<6> output;       
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
           
           for(int i=0;i<6;i++)
           {
             input.current_position[i]=cenobj.ec_control->motors[i]->actualPos();
             std::cout<<"JogabsJ motor-----"<<i<<"  "<< input.current_position[i]<<std::endl;
             input.current_velocity[i]= 0;
             input.current_acceleration[i] =0;
           }
                     
           for(int i=0;i<6;i++) 
           {
              input.target_position[i]=cenobj.ec_control->motors[i]->actualPos();
              input.target_velocity[i] = 0;
              input.target_acceleration[i] =0;
              input.max_velocity[i] = 1;
              input.max_acceleration[i] = 0.5;
              input.max_jerk[i] =0.5 ;
           }
           input.target_position[cmd.get<int>("motor")]=cmd.get<double>("position");
           return true;
    }
  
      void  excute_rt(void) override
      {         
                                      
                    if(otg.update(input, output) == Result::Working)            
                     { 
                       
                       auto& p = output.new_position;
                       for (int i=0; i<6; i++) 
                       {
                         cenobj.ec_control->motors[i]->setTargetPos(p[i]);
                         std::cout<<"JogabsJ motor-----"<<i<<"  "<<p[i]<<std::endl;
                       }    
                                                                                         
                       output.pass_to_input(input);
                       
                     }
                    else
                     {
                         rtnode_status=SUCCESS;          
                     }
         
      }
  };

 REGISTER(JogabsJ);

#endif