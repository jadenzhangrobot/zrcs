/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-24 18:39:03
 * @LastEditTime: 2023-05-17 11:17:38
 * @Description: 电机回原点指令
 * 
 */

#ifndef MOVEHOME_H_
#define MOVEHOME_H_
#include "behaviortree_cpp_v3/action_node.h"
#include "system/basefun.h"
#include "system/centre.h"
#include <iostream>
#include <ruckig/ruckig.hpp>
#include "system/classfactory.h"
#include"../../src/slave.h"
using namespace ruckig;
class Movehome:basefun
{
      public:
       
            centre& cenobj=centre::getInstance();
            Ruckig<JointNum> otg {0.001}; 
            InputParameter<JointNum> input;
            OutputParameter<JointNum> output;
  //  protected:
           //int motor_num;
          
            Movehome()
            {
                  
                  cmdline::parser cmd;        
                  
                  for(int i=0;i<JointNum;i++)
                  {
                    input.current_position[i]=cenobj.ec_control->motors[i]->actualPos();
                    input.current_velocity[i] =0 ;
                    input.current_acceleration[i] = 0;
                  }
                  
                          
                  for(int i=0;i<JointNum;i++)
                  {
                      input.target_position[i]=0;
                      input.target_velocity[i] =0;
                      input.target_acceleration[i] = 0;
                      input.max_velocity[i] = 0.1;
                      input.max_acceleration[i] = 0.1;
                      input.max_jerk[i] = 0.1;
                  }
                  
            }
  
      void  excute_rt(void) override
      {                          
                    if(otg.update(input, output) == Result::Working)            
                     { 
                       auto& p = output.new_position;
                       for (int i=0; i<JointNum; i++) 
                       {
                         cenobj.ec_control->motors[i]->setTargetPos(p[i]);
                       }                                                                                        
                       output.pass_to_input(input);
                        rt_flag=1; 
                     }
                    else
                     {
                        rt_flag=2; 
                        std::cout<<"movehome finished"<<std::endl;                    
                     }        
      }      
   
  };
class BtMovehome : public BT::SyncActionNode
{
public:
   centre& cenobj=centre::getInstance();
   bool condition=true;
   BtMovehome(const std::string& name, const BT::NodeConfiguration& config) :
   BT::SyncActionNode(name, config)
  {
     
  }

  // You must override the virtual function tick()
  BT::NodeStatus tick() override
  {
    
    cenobj.cmd_queue.push("Movehome");     
   
    while (cenobj.rt_status!=2)
    {
       
    }
  
      return BT::NodeStatus::SUCCESS;
  }
  
  static BT::PortsList providedPorts()
  {
    BT::PortsList ports;   
    return ports;
  }
  
};

 REGISTER(Movehome);
#endif