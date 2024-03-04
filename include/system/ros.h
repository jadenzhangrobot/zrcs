#ifndef ROS_H
#define ROS_H
#include <std_msgs/Float64.h>
#include <actionlib/server/simple_action_server.h>
#include <ros/ros.h>
#include <string>
namespace zrcs_system
{
   class Zrcstopic
   {
         public:   
         ros::NodeHandle nh;
         ros::Publisher pub;
         std::string topic_name;
     
       Zrcstopic(std::string tn)
       {
        topic_name=tn;
         pub=nh.advertise<std_msgs::Float64>(topic_name,1000000);   

       }
      int send(double data)
      {  
        std_msgs::Float64 msg; 
        msg.data=data;
        
        pub.publish(msg);
        return 1;
     }
   };
  //  typedef actionlib::SimpleActionServer<zrcsbt::zrcs_clientAction> Server;
  //  void execute(const zrcsbt::zrcs_clientActionGoalConstPtr& goal, Server* as)
  //  {
     
       

  //  }
   
  //  class zrcsbt
  //  {
  //      public:
  //      ros::NodeHandle nh;
       
  //      zrcsbt()
  //      {
  //         Server server(nh, "zrcsaction", boost::bind(&execute, _1, &server), false); 
  //         server.start();       
  //      }
        

  //  };
}

#endif