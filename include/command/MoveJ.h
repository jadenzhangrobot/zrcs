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
#include "model/urFIKinematin.h"
#include "system/basenode.h"
#include "system/centre.h"
#include "system/classfactory.h"
#include <iostream>
#include <ruckig/ruckig.hpp>
using namespace ruckig;
class MoveJ : public zrcs_system::Basenode {
public:
  zrcs_system::centre &cenobj = zrcs_system::centre::getInstance();
  Ruckig<6> otg{0.001};
  InputParameter<6> input;
  OutputParameter<6> output;
  Ur ur;
  // int motor_num;
  void init() override {
    cmdline::parser cmd;
    cmd.add<double>("x", 'x', "Move to x-axis", false, 0,
                    cmdline::range(-4.0, 4.0));
    cmd.add<double>("y", 'y', "Move to x-axis", false, 0,
                    cmdline::range(-3.14, 3.14));
    cmd.add<double>("z", 'z', "Move to x-axis", false, 0,
                    cmdline::range(-3.14, 3.14));
    cmd.add<double>("rx", 'r', "Move around the x-axis", false, 0,
                    cmdline::range(-4.0, 4.0));
    cmd.add<double>("ry", 'p', "Move around the x-axis", false, 0,
                    cmdline::range(-4.0, 4.0));
    cmd.add<double>("rz", 'b', "Move around the x-axis", false, 0,
                    cmdline::range(-4.0, 4.0));

    if (!cenobj.nrt_cmdParam.empty()) {
      std::string str = cenobj.nrt_cmdParam.front();
      cmd.parse_check(str);
      cenobj.nrt_cmdParam.pop();
    }
    double joint[6];

    double pose[6] = {cmd.get<double>("x"),  cmd.get<double>("y"),
                      cmd.get<double>("z"),  cmd.get<double>("rx"),
                      cmd.get<double>("ry"), cmd.get<double>("rz")};
    double target_joint[6];

    for (int i = 0; i < 6; i++) {
      input.current_position[i] = cenobj.ec_control->motors[i]->actualPos();
      joint[i] = cenobj.ec_control->motors[i]->actualPos();
      input.current_velocity[i] = 0;
      input.current_acceleration[i] = 0;
    }
    int ret = ur.r_inverse(joint, pose, target_joint);
    if (ret <= 0) {
      // LOGGER_ERROR("invese failed");
    }
    for (int i = 0; i < 6; i++) {
      input.target_position[i] = target_joint[i];
      std::cout << "errror          "<<target_joint[i]-joint[i] << std::endl;
      input.target_velocity[i] = 0;
      input.target_acceleration[i] = 0;
      input.max_velocity[i] = 1;
      input.max_acceleration[i] = 0.5;
      input.max_jerk[i] = 0.5;
    }
    node_status = RUNNING;
  }

  void excute_rt(void) override {

    if (otg.update(input, output) == Result::Working) {

      auto &p = output.new_position;
      for (int i = 0; i < 6; i++) {
        cenobj.ec_control->motors[i]->setTargetPos(p[i]);     
      }
      
      output.pass_to_input(input);
      node_status = RUNNING;
    } else {
      
      node_status = SUCCESS;
    }
  }
   void exit(void) override { ROS_INFO(" MoveJ finished"); }
};

REGISTER(MoveJ);

#endif