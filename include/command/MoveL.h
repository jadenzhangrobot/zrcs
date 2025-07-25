/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-06-16 09:54:19
 * @Description: 关节运动相对位置指令
 */
#ifndef MOVEL_H
#define MOVEL_H
#include "behaviortree_cpp_v3/basic_types.h"
#include "model/urFIKinematin.h"
#include "system/basenode.h"
#include "system/centre.h"
#include "system/classfactory.h"
#include <boost/concept/detail/has_constraints.hpp>
#include <cmath>

#include <ruckig/ruckig.hpp>
using namespace ruckig;
class MoveL : public zrcs_system::Basenode {
public:
  zrcs_system::centre &cenobj = zrcs_system::centre::getInstance();
  Ruckig<1> otg{0.001};
  InputParameter<1> input;
  OutputParameter<1> output;
  Ur ur;
  double L;
  double T[16];
  double joint[6];
  double pose[6];
  // int motor_num;
  void init() override {
    cmdline::parser cmd;
    cmd.add<double>("x", 'x', "Move to x-axis", false, 0,
                    cmdline::range(-4.0, 4.0));
    cmd.add<double>("y", 'y', "Move to x-axis", false, 0,
                    cmdline::range(-3.14, 3.14));
    cmd.add<double>("z", 'z', "Move to x-axis", false, 0,
                    cmdline::range(-3.14, 3.14));
    cmd.add<double>("rx", 'r', "Move around the x-axis", false, 3.1415926,
                    cmdline::range(-4.0, 4.0));
    cmd.add<double>("ry", 'p', "Move around the x-axis", false, 0,
                    cmdline::range(-4.0, 4.0));
    cmd.add<double>("rz", 'b', "Move around the x-axis", false, 1.5708,
                    cmdline::range(-4.0, 4.0));

    if (!cenobj.nrt_cmdParam.empty()) {
      std::string str = cenobj.nrt_cmdParam.front();
      cmd.parse_check(str);
      cenobj.nrt_cmdParam.pop();
    }

    pose[0] = cmd.get<double>("x");
    pose[1] = cmd.get<double>("y");
    pose[2] = cmd.get<double>("z");
    pose[3] = cmd.get<double>("rx");
    pose[4] = cmd.get<double>("ry");
    pose[5] = cmd.get<double>("rz");

    for (int i = 0; i < 6; i++) {
      joint[i] = cenobj.ec_control->motors[i]->actualPos();
    }
    ur.forward(joint, T);
    L = std::abs((pose[0] - T[3]) * (pose[0] - T[3]) +
                 (pose[1] - T[7]) * (pose[1] - T[7]) +
                 (pose[2] - T[11]) * (pose[2] - T[11]));

    input.current_position[0] = 0;
    input.current_velocity[0] = 0;
    input.current_acceleration[0] = 0;

    input.target_position[0] = L;
    input.target_velocity[0] = 0;
    input.target_acceleration[0] = 0;
    input.max_velocity[0] = 0.3;
    input.max_acceleration[0] = 0.5;
    input.max_jerk[0] = 0.5;
    node_status = RUNNING;
  }

  void excute_rt(void) override {

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
        joint[i] = cenobj.ec_control->motors[i]->actualPos();
      }
      int ret = ur.r_inverse(joint, pose_, target_joint);

      for (int i = 0; i < 6; i++) {

        cenobj.ec_control->motors[i]->setTargetPos(target_joint[i]);
      
      }
      output.pass_to_input(input);
      node_status = RUNNING;
    } else {
      node_status = SUCCESS;
    }
  }
  void exit(void) override { ROS_INFO(" MoveL finished"); }
};

REGISTER(MoveL);

#endif