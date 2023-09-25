/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-05-09 19:21:11
 * @LastEditTime: 2023-08-09 09:06:29
 * @Description: UR机械臂在gazeobo中仿真环境的通讯接口封装
 * 
 */
#ifndef CYBERGAZEBO_H
#define CYBERGAZEBO_H
#include "../controller_interface.h"
#include "sensor_msgs/JointState.h"
#include "ros/init.h"
#include <actionlib/client/simple_action_client.h>
#include <control_msgs/FollowJointTrajectoryAction.h>
#include <cstddef>
#include <std_msgs/Float64.h>
#include <ros/ros.h>
#include <sensor_msgs/JointState.h>

namespace controller
{

    class Cybergazebo
    {
        private:
            ros::NodeHandle nh; 
            ros::Subscriber sub;
            ros::Publisher  pub[6];
        public:
             inline static std::array<std::atomic<double>,4> send_postiton;
             inline static std::array<std::atomic<double>,4> receive_postiton;
             inline static std::array<std::atomic<double>,4> receive_effort;
            Cybergazebo(/* args */)
            {
                 
               
            }  
            ~Cybergazebo()
            {

            }
            void init()
            {          
               
                for(int i=0;i<4;i++)
                 {
                      std::string joint_name="/arm/joint"+std::to_string(i+1);

                      pub[i]=nh.advertise<std_msgs::Float64>(joint_name+"_position_controller/command",1000000000);
                      
                 }
                  sub=nh.subscribe("/arm/joint_states",1,&Cybergazebo::callfunc,this);               

            }
            static Cybergazebo &getInstance()
            {
                static Cybergazebo motor_instance;
                return motor_instance;
            }     
            void callfunc(const sensor_msgs::JointStateConstPtr& msg)
            {

                    receive_postiton[0].store(msg->position[0]);
                    receive_postiton[1].store(msg->position[1]);
                    receive_postiton[2].store(msg->position[2]);
                    receive_postiton[3].store(msg->position[3]);
                   
                   
                    receive_effort[0].store(msg->effort[0]);
                    receive_effort[1].store(msg->effort[1]);
                    receive_effort[2].store(msg->effort[2]);
                    receive_effort[3].store(msg->effort[3]);
                
            }
            void send_position()
            {
                for(int i=0;i<4;i++)
                {
                    std_msgs::Float64 msg;
                    msg.data=send_postiton[i].load();
                    pub[i].publish(msg);
                }
            }
             
         
    };


    class CyberMotor:Motor
    {
        int motor_id;
        Cybergazebo& cybergazebo=Cybergazebo::getInstance();
        public:
        CyberMotor(int id):motor_id(id)
        {

        }
        int setTargetPos (double position) override
        {
             cybergazebo.send_postiton[motor_id].store(position);
             return 1;
        }
        double actualPos(void) override
        {
            double position=cybergazebo.receive_postiton[motor_id].load();
            return position;
        }
        int  setTargetToq(double toq) override
        {            
            cybergazebo.send_postiton[motor_id].store(toq);
            return 1;
        }
         double actualToq(void) override
         {
              double toq=cybergazebo.receive_effort[motor_id].load();
              return toq;
         }             

    };
 class CybergazeboTransceive:Transceive
  {
    private:
         Cybergazebo& cybergazebo=Cybergazebo::getInstance();
    public:
         auto init()->int override
        {
                 
            cybergazebo.init();
            return 1;
        }
        auto send(void)->void override
        {
            cybergazebo.send_position();

        }
        auto  receive()->void override
        {
          
        }
  };
}

#endif
