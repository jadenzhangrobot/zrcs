/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 10:25:59
 * @LastEditTime: 2023-08-09 05:32:23
 * @Description: 
 * 
 * Copyright (c) 2023 by ${git_name_email}, All Rights Reserved. 
 */
#include "ros/rate.h"
#include <ros/ros.h>
#include <unistd.h>
#include "slave.h"
#include "system/centre.h"
#include "command/Cmdhead.h"

int main(int argc,char** argv)
{
      
    #ifdef Ros
      ros::init(argc,argv,"motion_control"); 
    #endif
       static zrcs_system::centre& ct=zrcs_system::centre::getInstance();
      //ct.registerController(GazeboController());
      //ct.registerController(InnfosController());
       ct.registerController(UrgazeboController());
      //ct.registerController(GlrbusController());
        ct.init();
            
     
  #ifdef Ros
   ros::spin();
  #else
   pause();
  #endif
  return 0;
}
