/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef MOVESINE_H
#define MOVESINE_H

#include "ros/publisher.h"
#include "system/basenode.h"
#include "system/centre.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <ruckig/ruckig.hpp>
#include "system/classfactory.h"
#include "system/cmdline.h"
#include <std_msgs/Float64.h>
class MoveSine:public zrcs_system::Basenode
  {
    public:
            zrcs_system::centre& cenobj=zrcs_system::centre::getInstance();         
            double  current_position[6];
            double  amplitude;
            double  frequency;
             
            ros::NodeHandle& nh; 
           ros::Publisher& pub; 
       bool init() override
       {        
         cmdline::parser cmd;
         cmd.add<double>("amplitude", 'a', "servo position", false, 1, cmdline::range(-20.000, 20.000));
         cmd.add<double>("frequency", 'f', "servo velocity", false, 1, cmdline::range(-101.0000, 101.0000));
       
          if (!cenobj.nrt_cmdParam.empty()) 
          {
              std::string str=cenobj.nrt_cmdParam.front();
              cmd.parse_check(str);
              cenobj.nrt_cmdParam.pop();
          }  
              for (int i=0; i<6; i++) {
               current_position[i]=cenobj.ec_control->motors[i]->actualPos();    
              }   
               amplitude=cmd.get<double>("amplitude");
               frequency=cmd.get<double>("frequency"); 
              ros::NodeHandle nh_;
               nh=nh_;
             ros::Publisher pub_; 
             pub=pub_;
            
             nh.advertise<std_msgs::Float64>("command_error",1000000);     
            return true;
    }
  
      void  excute_rt(void) override
      {         

                         
                                   
                        static double t=0;
                      
                        double target_position[6] ;                       
                        
                        for (int i=0;i<6;i++){
                            target_position[i]=current_position[i]+amplitude*sin(frequency*t);
                           
                            cenobj.ec_control->motors[i]->setTargetPos(target_position[i]);  
                        }
                        t=t+0.01;
                          std_msgs::Float64 msg;
                          msg.data=target_position[0]-cenobj.ec_control->motors[0]->actualPos();
                          
                    pub.publish(msg);
      }
  };

 REGISTER(MoveSine);

#endif