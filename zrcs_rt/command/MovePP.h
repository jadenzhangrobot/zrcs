/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-20 16:02:55
 * @LastEditTime: 2023-05-09 08:41:59
 * @Description: 挂轨机器人姿态变化指令
 * 
 */
#pragma once

#include "system/basefun.h"
#include "system/centre.h"
#include <ruckig/ruckig.hpp>
#include "controller/motor/gazebo.h"
#include "system/classfactory.h"
#include "Robot_algorithm/grlrobotFIkinematin.h"
#include"../../src/slave.h"
using namespace ruckig;
class Movepp:basefun
{
    public:
            centre& cenobj=centre::getInstance();
            Ruckig<5> otg {0.001}; 
            InputParameter<5> input;
            OutputParameter<5> output;      
            glrrobot glr;
    Movepp(){
            
         cmdline::parser cmd;
         cmd.add<double>("x", 'x', "Move to x-axis", false, 0, cmdline::range(-3.14, 3.14));
         cmd.add<double>("y", 'y', "Move to x-axis", false, 0, cmdline::range(-3.14, 3.14));
         cmd.add<double>("z", 'z', "Move to x-axis", false, 0, cmdline::range(-3.14, 3.14));
         cmd.add<double>("rx", 'r', "Move around the x-axis", false, 0, cmdline::range(-3.14, 3.14));
         cmd.add<double>("ry", 'p', "Move around the x-axis", false, 0, cmdline::range(-3.14, 3.14));
         cmd.add<double>("rz", 'b', "Move around the x-axis", false, 0, cmdline::range(-3.14, 3.14));
         
         if (!cenobj.nrt_cmdParam.empty()) 
         {
                std::string str=cenobj.nrt_cmdParam.front();
                cmd.parse_check(str);
                cenobj.nrt_cmdParam.pop();
        }
             
             KDL::JntArray j =KDL::JntArray(8);
             double p0 =cenobj.ec_control->motors[0]->actualPos();
             j =glr.inverse_kinematics(cmd.get<double>("x"),cmd.get<double>("y"),cmd.get<double>("z"),cmd.get<double>("rx"),cmd.get<double>("ry"),cmd.get<double>("rz"));
             input.current_position = {p0,j(4),j(5),j(6),j(7)};
             input.current_velocity = {0,0.0,0,0,0};
             input.current_acceleration = {0,0.0,0,0,0};           
          
                input.target_position={p0,j(0),j(1),j(2),j(3)};
                input.target_velocity = {0,0,0,0,0};
                input.target_acceleration= {0,0,0,0,0};
                input.max_velocity = {1,1,1,1,1};
                input.max_acceleration = {1,1,1,1,1};
                input.max_jerk =  {1,1,1,1,1};    
           
             // input.target_position[0]=p0;
    }
     
        void  excute_rt(void) override
        {

              
                    if(otg.update(input, output) == Result::Working)            
                     { 
                       auto& p = output.new_position;
                            
                     
                       for (int i=1; i<JointNum; i++) 
                       {
                        
                         cenobj.ec_control->motors[i]->setTargetPos(p[i]);
                       }    
                                                                         
                       output.pass_to_input(input);
                       rt_flag=1;
                     }
                    else
                     {
                        rt_flag=2;
                        std::cout<<"Movepp finished"<<std::endl;                          
                     }


        }



};
class BtMovepp : public BT::SyncActionNode
{
public:
     centre& cenobj=centre::getInstance();
     BT::Optional<std::string> res,res1,res2,res3,res4,res5;
     BtMovepp(const std::string& name, const BT::NodeConfiguration& config) :
     BT::SyncActionNode(name, config)
  {
     
            res  = this->getInput<std::string>("mx");
            res1 = this->getInput<std::string>("my");
            res2 = this->getInput<std::string>("mz");
            res3 = this->getInput<std::string>("rx");
            res4 = this->getInput<std::string>("ry");
            res5 = this->getInput<std::string>("rz");                 
     // std::cout<<"--"+res.value()+" --"+res1.value()<<std::endl;
     // cenobj.cmd_queue.push("JogabsJ --motor="+res.value()+" --position="+res1.value());     
  }

  // You must override the virtual function tick()
  BT::NodeStatus tick() override
  {    
          cenobj.cmd_queue.push("Movepp --x="+res.value()+" --y="+res1.value()+" --z="+res2.value()+" --rx="+res3.value()+" --ry="+res4.value()+" --rz="+res5.value());
         // std::cout <<"Movepp --x="+res.value()+" --y="+res1.value()+" --z="+res2.value()+" --rx="+res3.value()+" --ry="+res4.value()+" --rz="+res5.value()<< std::endl;
          while (cenobj.rt_status!=2)
          {
              
          }
     
         return BT::NodeStatus::SUCCESS;
    //std::cout << "ApproachObject: " << this->name() << std::endl;
  }
 static BT::PortsList providedPorts()
  {
    BT::PortsList ports;
    const char* description = "X-axis movement";
    ports.emplace(BT::InputPort<std::string>("mx", description));
    
    const char* description1 = "X-axis movement";
    ports.emplace(BT::InputPort<std::string>("my", description1));
    
    const char* description2 = "Z-axis movement";
    ports.emplace(BT::InputPort<std::string>("mz", description2));

    const char* description3 = "X-axis rotation";
    ports.emplace(BT::InputPort<std::string>("rx", description3));

    const char* description4 = "Y-axis rotation";
    ports.emplace(BT::InputPort<std::string>("ry", description4));

    const char* description5 = "Z-axis rotation";
    ports.emplace(BT::InputPort<std::string>("rz", description5));
   
    return ports;
  
  }
  
};

REGISTER(Movepp);
