#ifndef SFORK_H_
#define SFORK_H_
#include "system/basefun.h"
#include "system/centre.h"
#include <cmath>
#include "Robot_algorithm/sforkFIkinematin.h"
using namespace ruckig;

class BtSfork : public BT::SyncActionNode
{
public:
     centre& cenobj=centre::getInstance();
     BT::Optional<std::string> res ;
     BT::Optional<std::string> res1;
     bool condition=true;
   BtSfork(const std::string& name, const BT::NodeConfiguration& config) :
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
class Sfork:basefun
  {
    public:
       
            centre& cenobj=centre::getInstance();
            Ruckig<2> otg {0.001}; 
            InputParameter<2> input;
            OutputParameter<2> output;
            sforkFIkinematin  sforkfi;     
    Sfork(){
         cmdline::parser cmd;
         cmd.add<double>("angle", 'r', "servo1 position", false, 0, cmdline::range(-100.0, 100.0));
         cmd.add<double>("length", 'd', "servo2 position", false, 0, cmdline::range(-20.000, 20.000));
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
              input.max_velocity[i] = 0.52;
              input.max_acceleration[i] = 0.2;
              input.max_jerk[i] =0.1;
           }
            double angle=cmd.get<double>("angle");
            double len=cmd.get<double>("length");
            struct angle an= sforkfi.Inverse_kinematics(angle,len);
           input.target_position[0]=an.first_angle;
           input.target_position[1]=an.second_angle;
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
                        strcpy(cf.type,"Sfork");
                        cf.status=2;
                        strcpy(cf.error,"success");
                       // LOGGER_INFO("Sfork finished");
                        std::string str="MotionCtrlMsgCmd";
                        cenobj.zmq_cmd.pub(str.c_str(),str.length(),ZMQ_SNDMORE);
                        cenobj.zmq_cmd.pub(&cf,sizeof(cf),0);     
                     }
                    
      }      
   
  };

 REGISTER(Sfork);

#endif