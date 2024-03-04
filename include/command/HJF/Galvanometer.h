/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef GALVANOMETER_H
#define GALVANOMETER_H

#include "system/basenode.h"
#include <ruckig/ruckig.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <unistd.h>
#include "system/classfactory.h"

class Galvanometer:public zrcs_system::Basenode
  {
    public:  
         int motor_id;
         float x;
         float y;
      explicit Galvanometer(const std::string& node_name="Galvanometer")
      {
          motor_id=0;
      }

       void init() override
       {   
         port_input.add<float>("axis-x", 'x', "servo position", false, 0, cmdline::range(-20.000, 20.000));
         port_input.add<float>("axis-y", 'y', "servo velocity", false, 0, cmdline::range(-10.0, 10.0));
          if (!CmdParam->empty()) 
          {
              std::string str=CmdParam->front();
              port_input.parse_check(str);
              CmdParam->pop();
          }  
            port_input.get<float>("axis-x");
            port_input.get<float>("axis-y");
            node_status=RUNNING; 
       }
  
      void  excute_rt(void) override
      {      

          if (Control->motors[motor_id]->Set2DGalvanometerPositon(x,y)) 
          {
                    node_status=SUCCESS;
          }
          else {
                    node_status=FAILURE;
          }
                           
      }
      void exit(void) override
      {
           // ROS_INFO("JogabsJ finished");
      }
     
  };

 REGISTER(Galvanometer);

#endif