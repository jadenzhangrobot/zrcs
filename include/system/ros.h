#ifndef ROS_H
#define ROS_H
#include <std_msgs/Float64.h>
#include <ros/ros.h>
namespace zrcs_system
{
   class Zrcstopic
   {
         public:   
         ros::NodeHandle nh;
         ros::Publisher pub;
     
       Zrcstopic()
       {
         nh.advertise<std_msgs::Float64>("command_error",1000000);   

       }
       int send(std_msgs::Float64 data)
      {   
        pub.publish(data);
        return 1;
     }
   };
  
}

#endif