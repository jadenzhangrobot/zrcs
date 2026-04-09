/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-05-15 17:39:38
 * @LastEditTime: 2023-05-30 16:09:28
 * @Description:
 *
 */
#pragma once

#include "pinocchio/algorithm/joint-configuration.hpp"
#include "pinocchio/algorithm/rnea-derivatives.hpp"
#include "pinocchio/parsers/urdf.hpp"
#include "system/basenode.h"
#include "system/centre.h"
#include "system/classfactory.h"
#include <ruckig/ruckig.hpp>
#include <sensor_msgs/JointState.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
using namespace pinocchio;
using namespace ruckig;

class Ureffort : public zrcs_system::Basenode {
public:
  zrcs_system::centre &cenobj = zrcs_system::centre::getInstance();
  Ruckig<6> otg{0.001};
  InputParameter<6> input;
  OutputParameter<6> output;
  Model *model = nullptr;
  Data *data = nullptr;
  ros::NodeHandle nh;
  ros::Publisher pub;
  double qp[6];
  // Data data;
  Ureffort(const std::string &node_name = "Ureffort") {}
  void init() override {
    std::unique_ptr<Model> mymodel = std::make_unique<Model>();
    // model= &_model;
    model = mymodel.release();
    const std::string urdf_filename = std::string(
        "/home/zrcs/Documents/ws_catkin/src/ur/ur_description/urdf/ur5.urdf");
    // Load the URDF model
    // Model model;
    pinocchio::urdf::buildModel(urdf_filename, *model);
    std::unique_ptr<Data> mydata = std::make_unique<Data>(*model);
    data = mydata.release();
    // // Build a data related to model
    // Data data(model);
    cmdline::parser cmd;
    cmd.add<int>("motor", 'm', "motor number", false, 0,
                 cmdline::range(000, 100));
    cmd.add<double>("position", 'p', "servo position", false, 0,
                    cmdline::range(-20.000, 20.000));
    cmd.add<double>("velocity", 'v', "servo velocity", false, 300,
                    cmdline::range(-10, 10));
    cmd.add<double>("acceleration", 'a', "servo acceleration", false, 50,
                    cmdline::range(-10, 10));

    if (!cenobj.nrt_cmdParam.empty()) {
      std::string str = cenobj.nrt_cmdParam.front();
      cmd.parse_check(str);
      cenobj.nrt_cmdParam.pop();
    }
    for (int i = 0; i < 6; i++) {
      input.current_position[i] = cenobj.ec_control->motors[i]->actualPos();
      std::cout << "JogabsJ motor-----" << i << "  "
                << input.current_position[i] << std::endl;
      input.current_velocity[i] = 0;
      input.current_acceleration[i] = 0;
    }

    for (int i = 0; i < 6; i++) {
      input.target_position[i] = cmd.get<double>("position");
      input.target_velocity[i] = 0;
      input.target_acceleration[i] = 0;
      input.max_velocity[i] = 1;
      input.max_acceleration[i] = 0.5;
      input.max_jerk[i] = 0.5;
      qp[i] = cenobj.ec_control->motors[i]->actualPos();
    }
    // input.target_position[cmd.get<int>("motor")]=cmd.get<double>("position");
    pub = nh.advertise<sensor_msgs::JointState>("effort1", 1000000000);
    node_status = RUNNING;
  }
  void excute_rt(void) override {

    // if(otg.update(input, output) == Result::Working)
    if (true) {
      // output.  // // Allocate result container
      Eigen::VectorXd q = Eigen::VectorXd::Zero(model->nq);
      Eigen::VectorXd v = Eigen::VectorXd::Zero(model->nv);
      Eigen::VectorXd a = Eigen::VectorXd::Zero(model->nv);

      static std::uint32_t count = 0;

      //  q[0]=2*sin(0.1*count);

      //  v[0]=2*cos(0.001*count)*0.1;

      //  a[0]=-2*sin(0.001*count)*0.1*0.1;

      count++; /*  */
      for (int i = 0; i < 6; i++) {
        q[i] = qp[i];
        v[i] = 0;
        a[i] = 0;
      }
      // // Computes the inverse dy*namics (RNEA) derivatives for all the joints
      // of the robot
      computeRNEADerivatives(*model, *data, q, v, a);
      // std_msgs::Float64 msg;
      //  sensor_msgs::JointState msg;
      // msg.header.stamp=ros::Time::now();
      // for(int i=0;i<JointNum;i++){

      // msg.effort={data->tau[0],data->tau[1],data->tau[2],data->tau[3],data->tau[4],data->tau[5]}
      // ;

      // }

      // pub.publish(msg);
      //  cenobj.ec_control->motors[0]->setTargetToq(data->tau[2]);
      //  cenobj.ec_control->motors[1]->setTargetToq(data->tau[1]);
      //  cenobj.ec_control->motors[2]->setTargetToq(data->tau[0]);
      for (int i = 0; i < 6; i++) {
        cenobj.ec_control->motors[i]->setTargetToq(data->tau[i]);
      }

      // std::cout << "Joint torque: " << data->tau.transpose() << std::endl;

    }
    // output.pass_to_input(input);
    //   rt_flag=1;
    //  }
    else {
      // if(
      // forcenobj.ec_control->motors[i]->setTargetPos(p[i])==cenobj.ec_control->motors[i]->act)
      node_status = SUCCESS;
    }
  }
  void exit(void) override {
    // zrcsbt::zrcs_clientResult result;
    // result.result = "error";
    // cenobj.Server.setSucceeded(result);
    ROS_INFO("finished");
  }
};
REGISTER(Ureffort);
