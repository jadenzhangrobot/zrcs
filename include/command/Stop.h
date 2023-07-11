/*
 * @Author: zhyj0372 zhangyongjing@oetsky.com
 * @Date: 2023-06-08 15:16:36
 * @LastEditors: zhyj0372 zhangyongjing@oetsky.com
 * @LastEditTime: 2023-06-12 20:14:17
 * @FilePath: /src/motion-control/include/command/Stop.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef STOP_H
#define STOP_H
#include "behaviortree_cpp_v3/action_node.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "system/basefun.h"
#include "system/centre.h"
#include <iostream>
#include <ruckig/ruckig.hpp>
#include "system/classfactory.h"
#include"../../src/slave.h"
using namespace ruckig;
class Stop:basefun
{
    public:
            centre& cenobj=centre::getInstance();
            //Ruckig<JointNum> otg {0.001}; 
            //InputParameter<JointNum> input;
            //OutputParameter<JointNum> output;

    Stop()
    { 

          // for(int i=0;i<JointNum;i++)
          //  {
          //    input.current_position[i]=0.1;
          //    input.current_velocity[i]= 0;
          //    input.current_acceleration[i] =0;
          //  }
                     
          //  for(int i=0;i<JointNum;i++) 
          //  {
          //     input.target_position[i]=cenobj.ec_control->motors[i]->actualPos();
          //     input.target_velocity[i] = cenobj.ec_control->motors[i]->actualVel();
          //     input.target_acceleration[i] =0;
          //     input.max_velocity[i] = 1;
          //     input.max_acceleration[i] = 0.5;
          //     input.max_jerk[i] =0.5;
          //  }
          
         
    }
    void  excute_rt(void) override
      {                                   
                    
                    // if(otg.update(input, output) == Result::Working)            
                    //  {                       
                    //    auto& p = output.new_position;
                    //    for (int i=0; i<JointNum; i++) 
                    //    {
                    //      cenobj.ec_control->motors[i]->setTargetPos(p[i]);
                         
                    //    }                                                                                            
                    //    output.pass_to_input(input);
                    //    rt_flag=1;                      
                    //  }
                    // else
                    //  {
                        cmd_frame cf;
                        rt_flag=2;
                        cf.status=2;
                        strcpy(cf.error,"Stop success");
                        //LOGGER_INFO("Stop finished");
                        std::string str="MotionCtrlMsgUp";
                        cenobj.zmq_cmd.pub(str.c_str(),str.length(),ZMQ_SNDMORE);
                        cenobj.zmq_cmd.pub(&cf,sizeof(cf),0);
                   //  }
                    
      }      





}; 
REGISTER(Stop);

#endif