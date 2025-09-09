/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-06-16 09:54:19
 * @Description: 直线运动指令
 */
#ifndef MOVEL_H
#define MOVEL_H

#include "system/basenodeInterface.h"
#include "model/urFIKinematin.h"
#include <array>
#include <ruckig/ruckig.hpp>
#include <string>
#include <vector>
#include "system/nodeFactory.h"
#include <cmath>

using namespace ruckig;
class G01 : public zrcsSystem::RtCmdNode {
public:
  Ruckig<1> otg {cycletime*0.001};
  InputParameter<1> input;
  OutputParameter<1> output;
  Ur ur;
  double L;
  double T[16];
  double joint[6];
  double pose[6];
  double x, y, z, rx, ry, rz;
  double velocity;
  double acceleration;
  double jerk;
  
  G01()
  {
    port_input.add<double>("x", 'x', "Move to x-axis", false, 0, cmdline::range(-4.0, 4.0));
    port_input.add<double>("y", 'y', "Move to y-axis", false, 0, cmdline::range(-4.0, 4.0));
    port_input.add<double>("z", 'z', "Move to z-axis", false, 0, cmdline::range(-4.0, 4.0));
    port_input.add<double>("rx", 'r', "Move around the x-axis", false, 3.1415926, cmdline::range(-4.0, 4.0));
    port_input.add<double>("ry", 'p', "Move around the y-axis", false, 0, cmdline::range(-4.0, 4.0));
    port_input.add<double>("rz", 'b', "Move around the z-axis", false, 1.5708, cmdline::range(-4.0, 4.0));
    port_input.add<double>("velocity", 'v', "linear velocity", false, 0.3, cmdline::range(0.0, 1.0));
    port_input.add<double>("acceleration", 'a', "linear acceleration", false, 0.5, cmdline::range(0.0, 2.0));
    port_input.add<double>("jerk", 'j', "linear jerk", false, 0.5, cmdline::range(0.0, 2.0));
   }
  ;
  
  void init() override
   {
     if(!cmdParam.empty())
     {
       port_input.parse_check(cmdParam);
     }
     
     x = port_input.get<double>("x");
     y = port_input.get<double>("y");
     z = port_input.get<double>("z");
     rx = port_input.get<double>("rx");
     ry = port_input.get<double>("ry");
     rz = port_input.get<double>("rz");
     velocity = port_input.get<double>("velocity");
     acceleration = port_input.get<double>("acceleration");
     jerk = port_input.get<double>("jerk");
     
     pose[0] = x;
     pose[1] = y;
     pose[2] = z;
     pose[3] = rx;
     pose[4] = ry;
     pose[5] = rz;
     
     for (int i = 0; i < 6; i++) {
       joint[i] = control->axiss[i]->actualPos();
     }
     
     ur.forward(joint, T);
     L = std::sqrt((pose[0] - T[3]) * (pose[0] - T[3]) +
                   (pose[1] - T[7]) * (pose[1] - T[7]) +
                   (pose[2] - T[11]) * (pose[2] - T[11]));

     input.current_position[0] = 0;
     input.current_velocity[0] = 0;
     input.current_acceleration[0] = 0;

     input.target_position[0] = L;
     input.target_velocity[0] = 0;
     input.target_acceleration[0] = 0;
     input.max_velocity[0] = velocity;
     input.max_acceleration[0] = acceleration;
     input.max_jerk[0] = jerk;
  }

  void run(void) override {
    if (otg.update(input, output) == Result::Working) {
      auto &p = output.new_position;
      double pose_[6];
      pose_[0] = T[3] + (pose[0] - T[3]) * (p[0] / L);
      pose_[1] = T[7] + (pose[1] - T[7]) * (p[0] / L);
      pose_[2] = T[11] + (pose[2] - T[11]) * (p[0] / L);
      pose_[3] = pose[3];
      pose_[4] = pose[4];
      pose_[5] = pose[5];
      double target_joint[6];
      for (int i = 0; i < 6; i++) {
        joint[i] = control->axiss[i]->actualPos();
      }
      int ret = ur.r_inverse(joint, pose_, target_joint);

      for (int i = 0; i < 6; i++) {
        if (control != nullptr && control->axiss.size() > i) {
          control->axiss[i]->setAxisPositionCmd(target_joint[i]);
        }
      }
      output.pass_to_input(input);
    }
    else if(otg.update(input, output) == Result::Finished) {
      SetOneShotStatus(zrcsSystem::OneShotNodeStatus::EXIT);
    }
    else {
      SetOneShotStatus(zrcsSystem::OneShotNodeStatus::FAILED);
    }
  }
  void exit(void) override { 
     // MoveL finished
   }
};

REGISTERRTCMD(G01)

#endif