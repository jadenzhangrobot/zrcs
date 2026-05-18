/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#pragma once

#include "system/node/BaseNodeInterface.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <ruckig/ruckig.hpp>
class MoveSine : public zrcsSystem::CmdNode {
public:
  
  double currentPosition[6];
  double amplitude;
  double frequency;
  MoveSine()
  {
        port_input.add<double>("amplitude", 'a', "servo position", false, 1,
                           cmdline::range(-20000.000, 20000.000));
        port_input.add<double>("frequency", 'f', "servo velocity", false, 1,
                           cmdline::range(-101.0000, 101.0000));

     

  }


     void config() override
       {
          if (!cmdParam.empty()) 
          {
              std::string str=cmdParam.front();
              port_input.parse_check(str);
              cmdParam.pop();
          }   
       
       }

  bool init() override {


    for (int i = 0; i < control->motors.size(); i++) {
      currentPosition[i] = control->motors[i]->actualPos();
    }
    amplitude = port_input.get<double>("amplitude");
    frequency = port_input.get<double>("frequency");
    node_status = RUNNING;
    return true;
  }

  void excuteRt(void) override {

    static double t = 0;

    double targetPosition[6];

    for (int i = 0; i < control->motors.size(); i++) {
      targetPosition[i] = currentPosition[i] + amplitude * sin(frequency * t);

      control->motors[i]->setTargetPos(targetPosition[i]);
    }
    t = t + 0.01;
   

    if (t > 1000000000000) {
      node_status = SUCCESS;
    }
  }
  bool exit()override{ return true; }
};

CMD_REGISTER(MoveSine);

