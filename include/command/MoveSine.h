/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef MOVESINE_H
#define MOVESINE_H
#include "system/basenode.h"
#include "system/centre.h"
#include "system/classfactory.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <ruckig/ruckig.hpp>
class MoveSine : public zrcs_system::Basenode {
public:
  
  double current_position[6];
  double amplitude;
  double frequency;

  void init() override {

    port_input.add<double>("amplitude", 'a', "servo position", false, 1,
                           cmdline::range(-20.000, 20.000));
    port_input.add<double>("frequency", 'f', "servo velocity", false, 1,
                           cmdline::range(-101.0000, 101.0000));

   if (!CmdParam->empty()) 
          {
              std::string str=CmdParam->front();
              port_input.parse_check(str);
              CmdParam->pop();
          }   
    for (int i = 0; i < 1; i++) {
      current_position[i] = Control->motors[i]->actualPos();
    }
    amplitude = port_input.get<double>("amplitude");
    frequency = port_input.get<double>("frequency");
    node_status = RUNNING;
  }

  void excute_rt(void) override {

    static double t = 0;

    double target_position[6];

    for (int i = 0; i < 1; i++) {
      target_position[i] = current_position[i] + amplitude * sin(frequency * t);

      Control->motors[i]->setTargetPos(target_position[i]);
    }
    t = t + 0.01;
   

    if (t > 1000000000000) {
      node_status = SUCCESS;
    }
  }
};

REGISTER(MoveSine);

#endif