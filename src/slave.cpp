/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-22 17:25:32
 * @LastEditTime: 2023-08-09 05:33:31
 * @Description: 控制器注册文件
 * 
 * Copyright (c) 2023 by ${git_name_email}, All Rights Reserved. 
 */

 #include "slave.h"
 #include "controller/controller_interface.h"
 //#include "controller/rtos/xenomai.h"
 #include "system/centre.h"
 #include "controller/motor/rawsocketbus.h"
 #include "controller/motor/urgazebo.h"
 #include <memory>
 /**
  * @description: 注册一个gazebo控制器
  * @return {*}
  */
//  std::unique_ptr<controller::Controller> GazeboController(void)
//  {
//     std::unique_ptr<controller::Controller> ct(new controller::Controller);
//     for(int i=0;i<JointNum;i++)
//     {
//         std::unique_ptr<controller::Motor> cm((controller::Motor*)(new controller::GazeboMotor(i)));
//         ct->motors.push_back(std::move(cm));
//     }   
//      ct->transceiver.reset((controller::Transceive*)(new controller::GazeboTransceive()));

//      ct->rtos_.reset((controller::Rtos*)(new controller::linux()));
//      return ct;
//  }

 /**
  * @description: 注册一个infos的机器人控制器
  * @return {*}
  */
//  std::unique_ptr<controller::Controller> InnfosController(void)
//  {
//     std::unique_ptr<controller::Controller> ct(new controller::Controller);
//     for(int i=0;i<JointNum;i++)
//     {
//         std::unique_ptr<controller::Motor> cm((controller::Motor*)(new controller::InnfosMotor(i)));
//         ct->motors.push_back(std::move(cm));
//     }

//      ct->transceiver.reset((controller::Transceive*)(new controller::InnfosTransceive()));

//      ct->rtos_.reset((controller::Rtos*)(new controller::linux()));

//     return ct;
//  }
 /**
  * @description: 注册一个ur在仿真环境gazebo中的控制器
  * @return {*}
  */
 std::unique_ptr<controller::Controller> UrgazeboController(void)
 {
    //使用ur在gazebo中的控制器
    std::unique_ptr<controller::Controller> ct(new controller::Controller);
    for(int i=0;i<JointNum;i++)
    {
        std::unique_ptr<controller::Motor> cm((controller::Motor*)(new controller::UrgazeboMotor(i)));
        ct->motors.push_back(std::move(cm));
    }
     //发送接受函数，为了兼容总线协议
     ct->transceiver.reset((controller::Transceive*)(new controller::UrgazeboTransceive()));
      //使用linux操作系统
     ct->rtos_.reset((controller::Rtos*)(new controller::Nativelinux()));

    return ct;
 }

    /**
    * @description: 注册一个Glrbus真实环境中的控制器
    * @return {*}
    */
//  std::unique_ptr<controller::Controller> GlrbusController(void)
//  {
//     //使用ur在gazebo中的控制器
//     std::unique_ptr<controller::Controller> ct(new controller::Controller);
//     for(int i=0;i<JointNum;i++)
//     {
//         std::unique_ptr<controller::Motor> cm((controller::Motor*)(new controller::Glrrobot(i)));
//         ct->motors.push_back(std::move(cm));
//     }
//      //发送接受函数，为了兼容总线协议
//      ct->transceiver.reset((controller::Transceive*)(new controller::GlrrobotTransceive()));
//       //使用linux操作系统
//      ct->rtos_.reset((controller::Rtos*)(new controller::linux()));

//     return ct;
//  }


//   std::unique_ptr<controller::Controller> GlrbusController(void)
//  {
//     //使用ur在gazebo中的控制器
//     std::unique_ptr<controller::Controller> ct(new controller::Controller);
//     for(int i=0;i<JointNum;i++)
//     {
//         std::unique_ptr<controller::Motor> cm((controller::Motor*)(new controller::Glrrobot(i)));
//         ct->motors.push_back(std::move(cm));
//     }
//      //发送接受函数，为了兼容总线协议
//      ct->transceiver.reset((controller::Transceive*)(new controller::GlrrobotTransceive()));
//       //使用linux操作系统
//      //ct->rtos_.reset((controller::Rtos*)(new controller::Preempt_rt()));
//      ct->rtos_.reset((controller::Rtos*)(new controller::xenomai()));

//     return ct;
//  }
