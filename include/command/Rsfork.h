#ifndef RSFORK_H_
#define RSFORK_H_
#include "behaviortree_cpp_v3/action_node.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "system/basefun.h"
#include "system/centre.h"
#include <cstring>
#include <iostream>
#include <ruckig/ruckig.hpp>
#include "system/classfactory.h"
#include"../../src/slave.h"
#include <cmath>
#include "system/zmq.h"
#include "Robot_algorithm/sforkFIkinematin.h"
using namespace ruckig;

class BtRsfork : public BT::SyncActionNode
{
public:
     centre& cenobj=centre::getInstance();
     BT::Optional<std::string> res ;
     BT::Optional<std::string> res1;
     bool condition=true;
   BtRsfork(const std::string& name, const BT::NodeConfiguration& config) :
   BT::SyncActionNode(name, config)
  {
     
       res = this->getInput<std::string>("motorp1");
       res1 = this->getInput<std::string>("motorp2"); 
      
     // std::cout<<"--"+res.value()+" --"+res1.value()<<std::endl;
     // cenobj.cmd_queue.push("JogabsJ --motor="+res.value()+" --position="+res1.value());     
  }

  // You must override the virtual function tick()
  BT::NodeStatus tick() override
  {
    // if (condition) {
    cenobj.cmd_queue.push("Sfork --motorp1="+res.value()+" --motorp2="+res1.value());
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
   // const char* description = "motorp1";
    ports.emplace(BT::InputPort<std::string>("motorp1"));
    
    //const char* description1 = "motorp1";
    ports.emplace(BT::InputPort<std::string>("motorp2"));
    
    // Add multiple InputPorts
   // ports.emplace("position", BT::InputPort<int>("position"));
    // Add OutputPort

    return ports;
   // const char* description = "motor_id";
    //return {BT::InputPort<std::string>("motor", description)};
  }
  
};
class Rsfork:basefun
  {
    public:
       
            centre& cenobj=centre::getInstance();
            Ruckig<2> otg {0.001}; 
            InputParameter<2> input;
            OutputParameter<2> output;
            sforkFIkinematin sfkin;       
    Rsfork(){
         cmdline::parser cmd;
         cmd.add<int>("angledir", 'r', "servo1 position", false, 0, cmdline::range(-100, 100));
         cmd.add<int>("lengthdir", 'd', "servo2 position", false, 0, cmdline::range(-20, 20));
       
          if (!cenobj.nrt_cmdParam.empty()) 
          {
              std::string str=cenobj.nrt_cmdParam.front();
              cmd.parse_check(str);
              cenobj.nrt_cmdParam.pop();
          }   
           
           for(int i=0;i<2;i++)
           {
             input.current_position[i]=cenobj.ec_control->motors[i]->actualPos();
             input.current_velocity[i]= 0;
             input.current_acceleration[i] =0;
           }
                   
           for(int i=0;i<2;i++) 
           {
              input.target_position[i]=cenobj.ec_control->motors[i]->actualPos();
              input.target_velocity[i] = 0;
              input.target_acceleration[i] =0;
              input.max_velocity[i] = 0.02;
              input.max_acceleration[i] = 0.1;
              input.max_jerk[i] =0.1;
           }
            int angledir=cmd.get<int>("angledir");
            int lendir=cmd.get<int>("lengthdir");
            if(angledir==0)
            {
             
              struct position po=sfkin.Forward_kinematics(cenobj.ec_control->motors[0]->actualPos(), cenobj.ec_control->motors[1]->actualPos());
              if (lendir>0) {
                   struct angle an=sfkin.Inverse_kinematics(po.angle, 1.5);
                     input.target_position[0]=an.first_angle;
                     input.target_position[1]=an.second_angle;  
                             
              }
              else if(lendir<0)
              {
                     struct angle an=sfkin.Inverse_kinematics(po.angle, 0.3);
                     input.target_position[0]=an.first_angle;
                     input.target_position[1]=an.second_angle;       
              }
              else
              {
                     struct angle an=sfkin.Inverse_kinematics(po.angle, po.len);
                     input.target_position[0]=an.first_angle;
                     input.target_position[1]=an.second_angle;
              }
              
            }
            else if (angledir>0) {
                struct position po=sfkin.Forward_kinematics(cenobj.ec_control->motors[0]->actualPos(), cenobj.ec_control->motors[1]->actualPos());
                   std::cout<<po.len<<std::endl;   
                   struct angle an=sfkin.Inverse_kinematics(80, po.len);
                     input.target_position[0]=an.first_angle;
                     input.target_position[1]=an.second_angle;               
                std::cout<<an.first_angle<<" "<<an.second_angle<<std::endl;   
                    
            }
            else if (angledir<0) {
                struct position po=sfkin.Forward_kinematics(cenobj.ec_control->motors[0]->actualPos(), cenobj.ec_control->motors[1]->actualPos());
           
                    struct angle an=sfkin.Inverse_kinematics(-80, po.len);
                     input.target_position[0]=an.first_angle;
                     input.target_position[1]=an.second_angle;               
             
                    
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
                        cmd_frame cf;
                        strcpy(cf.type,"Rsfork");
                        cf.status=2;
                        strcpy(cf.error,"success");
                        std::string str="MotionCtrlMsgCmd";
                        cenobj.zmq_cmd.pub(str.c_str(),str.length(),ZMQ_SNDMORE);
                        cenobj.zmq_cmd.pub(&cf,sizeof(cf),0);     
                     }

                    
      }      
   
  };

 REGISTER(Rsfork);

#endif