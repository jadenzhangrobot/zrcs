/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-06-16 09:54:19
 * @Description: 关节运动相对位置指令
 */
#ifndef MOVEJ_H
#define MOVEJ_H
#include "behaviortree_cpp_v3/action_node.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "system/basefun.h"
#include "system/centre.h"
#include <iostream>
#include <ruckig/ruckig.hpp>
#include "system/classfactory.h"
#include"../../src/slave.h"
#include"Robot_algorithm/urFIKinematin.h"
using namespace ruckig;

class BtMoveJ : public BT::SyncActionNode
{
public:
     centre& cenobj=centre::getInstance();
    BT::Optional<std::string> res,res1,res2,res3,res4,res5;
     bool condition=true;
   BtMoveJ (const std::string& name, const BT::NodeConfiguration& config) :
   BT::SyncActionNode(name, config)
  {  
            res  = this->getInput<std::string>("x");
            res1 = this->getInput<std::string>("y");
            res2 = this->getInput<std::string>("z");
            res3 = this->getInput<std::string>("rx");
            res4 = this->getInput<std::string>("ry");
            res5 = this->getInput<std::string>("rz");          
      
     // std::cout<<"--"+res.value()+" --"+res1.value()<<std::endl;
     // cenobj.cmd_queue.push("JogabsJ --motor="+res.value()+" --position="+res1.value());     
  }

  // You must override the virtual function tick()
  BT::NodeStatus tick() override
  {
    // if (condition) {
    cenobj.cmd_queue.push("MoveJ --x="+res.value()+" --y="+res1.value()+" --z="+res2.value()+" --rx="+res3.value()+" --ry="+res4.value()+" --rz="+res5.value());
       ///   condition=false;    
  //   }
    while (cenobj.rt_status!=2)
    {
        
    }
     
     return BT::NodeStatus::SUCCESS;
    //return BT::NodeStatus::RUNNING;
    //std::cout << "ApproachObject: " << this->name() << std::endl;
  }
  //  void halt() override
  //   {



  //   }
  static BT::PortsList providedPorts()
  {
    BT::PortsList ports;
    const char* description = "X-axis movement";
    ports.emplace(BT::InputPort<std::string>("x", description));
    
    const char* description1 = "Y-axis movement";
    ports.emplace(BT::InputPort<std::string>("y", description1));
    
    const char* description2 = "Z-axis movement";
    ports.emplace(BT::InputPort<std::string>("z", description2));

    const char* description3 = "X-axis rotation";
    ports.emplace(BT::InputPort<std::string>("rx", description3));

    const char* description4 = "Y-axis rotation";
    ports.emplace(BT::InputPort<std::string>("ry", description4));

    const char* description5 = "Z-axis rotation";
    ports.emplace(BT::InputPort<std::string>("rz", description5));   

    return ports;
   // const char* description = "motor_id";
    //return {BT::InputPort<std::string>("motor", description)};
  }
  
};
class MoveJ:basefun
  {
    public:
       
            centre& cenobj=centre::getInstance();
            Ruckig<JointNum> otg {0.001}; 
            InputParameter<JointNum> input;
            OutputParameter<JointNum> output;
            Ur ur;
            
           //int motor_num;
          
    MoveJ(){
         cmdline::parser cmd;
         cmd.add<double>("x", 'x', "Move to x-axis", false, 0, cmdline::range(-4.0, 4.0));
         cmd.add<double>("y", 'y', "Move to x-axis", false, 0, cmdline::range(-3.14, 3.14));
         cmd.add<double>("z", 'z', "Move to x-axis", false, 0, cmdline::range(-3.14, 3.14));
         cmd.add<double>("rx", 'r', "Move around the x-axis", false, 0, cmdline::range(-4.0, 4.0));
         cmd.add<double>("ry", 'p', "Move around the x-axis", false, 0, cmdline::range(-4.0, 4.0));
         cmd.add<double>("rz", 'b', "Move around the x-axis", false, 0, cmdline::range(-4.0, 4.0));
         
         
          if (!cenobj.nrt_cmdParam.empty()) 
          {
              std::string str=cenobj.nrt_cmdParam.front();
              cmd.parse_check(str);
              cenobj.nrt_cmdParam.pop();
          }   
          double joint[6];
          
        
          double pose[6]={cmd.get<double>("x"),cmd.get<double>("y"),cmd.get<double>("z"),cmd.get<double>("rx"),cmd.get<double>("ry"),cmd.get<double>("rz")};
          double target_joint[6];


           for(int i=0;i<JointNum;i++)
           {
             input.current_position[i]=cenobj.ec_control->motors[i]->actualPos();
             joint[i]=cenobj.ec_control->motors[i]->actualPos();
             input.current_velocity[i]= 0;
             input.current_acceleration[i] =0;
           }
            int ret= ur.r_inverse(joint,pose,target_joint);
            if (ret<=0) {
             // LOGGER_ERROR("invese failed");
            
            }
           for(int i=0;i<JointNum;i++) 
           {
              input.target_position[i]=target_joint[i];
              std::cout<<target_joint[i]<<std::endl;
              input.target_velocity[i] = 0;
              input.target_acceleration[i] =0;
              input.max_velocity[i] = 1;
              input.max_acceleration[i] = 0.5;
              input.max_jerk[i] =0.5 ;
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
                         //std::cout<<"JogabsJ motor-----"<<i<<"  "<<p[i]<<std::endl;
                       }                                                                                             
                       output.pass_to_input(input);
                       rt_flag=1;
                     }
                    else
                     {
                     // if( forcenobj.ec_control->motors[i]->setTargetPos(p[i])==cenobj.ec_control->motors[i]->act)
                    
                        rt_flag=2;
                        // cmd_frame cf;
                        // strcpy(cf.type,"MoveJ");
                        // cf.status=2;
                        // strcpy(cf.error,"success");
                        //LOGGER_INFO("MoveJ finished");                   
                     }
         
      }      
   
  };

 REGISTER(MoveJ);

#endif