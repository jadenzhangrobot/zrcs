/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 10:25:59
 * @LastEditTime: 2023-06-08 14:45:15
 * @Description: gazebo控制器
 * 
 */
#ifndef GAZEBO_H
#define GAZEBO_H
#include "controller/controller_interface.h"
#include "ros/init.h"
#include "ros/rate.h"
#include "ros/ros.h"
#include "sensor_msgs/JointState.h"
#include <thread>
#include "ros/rate.h"
#include <atomic>
#include <queue>
#include <atomic>
#include <array>
namespace controller
{
   
  class Gazebo
  {

    private:
        ros::NodeHandle nh;
        ros::Subscriber sub;
        ros::Publisher  pub;
    public:
            inline static std::array<std::atomic<double>,5> send_postiton;
            inline static std::array<std::atomic<double>,5> receive_postiton;
        Gazebo(void)
        {
           
            pub=nh.advertise<sensor_msgs::JointState>("/robot1/joint/pos_cmd",100000000);
            sub=nh.subscribe("/joint_positon",1,&Gazebo::callfunc,this);     

        }
        
        ~Gazebo()
        {

        }
         
          static Gazebo &getInstance()
          {
              static Gazebo motor_instance;
              return motor_instance;
          }     
          void callfunc(const sensor_msgs::JointStateConstPtr& msg)
          {
             for(int i=0;i<5;i++)
              {
                
                receive_postiton[i].store(msg->position[i]);

              }
          }
               
          void send_positon()
          {
              sensor_msgs::JointState p;
              p.position.resize(5);
               for(int i=0;i<5;i++)
               {
                p.position[i]=send_postiton[i].load();
               }         
              pub.publish(p); 

          }      
  };

  class GazeboMotor:Motor
  {
    private:
       int motor_id;
       Gazebo &gazebo=Gazebo::getInstance();
    public:
       GazeboMotor(int mn):motor_id(mn)
       {
         
       }         
       auto setTargetPos(double j_val)->int override
       {
         gazebo.send_postiton[motor_id].store(j_val);
         return 1;         
       }
        double  actualPos() override
        { 
          return gazebo.receive_postiton[motor_id].load();
        }
        auto enable()->int  override
        {
            return 1;
        }
        auto  disable()->int override
        {
            return 1;
        }
        auto mode(std::uint8_t md)->int override
        {
               
               return 1;

        }
        auto  clearError()->int override
        {               
            return 1;
        }
         
      
  };
  class GazeboTransceive:Transceive
  {
    private:
        Gazebo &gazebo=Gazebo::getInstance();
    public:
        auto send(void)->void override
        {
            gazebo.send_positon();
        }
        auto  receive()->void override
        {
          
        }
  };
}
#endif