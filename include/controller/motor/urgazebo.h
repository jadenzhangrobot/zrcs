/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-05-09 19:21:11
 * @LastEditTime: 2023-08-09 09:06:29
 * @Description: UR机械臂在gazeobo中仿真环境的通讯接口封装
 * 
 */
#ifndef URGAZEBO_H
#define URGAZEBO_H
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

    class  Urgazebo
    {
        private:
           
            ros::Subscriber sub;
            ros::Publisher  pub[6];
        public:
             inline static std::array<std::atomic<double>,6> send_postiton;
             inline static std::array<std::atomic<double>,6> receive_postiton;
             inline static std::array<std::atomic<double>,6> receive_effort;
            Urgazebo(/* args */)
            {
                 
               
            }  
            ~Urgazebo()
            {

            }
            void init()
            {          
                 ros::NodeHandle nh; 
                for(int i=0;i<6;i++)
                 {
                      std::string joint_name="joint"+std::to_string(i+1);

                      pub[i]=nh.advertise<std_msgs::Float64>(joint_name+"_position_controller/command",1000000000);
                      
                 }
                  sub=nh.subscribe("/joint_states",1,&Urgazebo::callfunc,this);               

            }
            static Urgazebo &getInstance()
            {
                static Urgazebo motor_instance;
                return motor_instance;
            }     
            void callfunc(const sensor_msgs::JointStateConstPtr& msg)
            {

                    receive_postiton[2].store(msg->position[0]);
                    receive_postiton[1].store(msg->position[1]);
                    receive_postiton[0].store(msg->position[2]);
                    receive_postiton[3].store(msg->position[3]);
                    receive_postiton[4].store(msg->position[4]);
                    receive_postiton[5].store(msg->position[5]);
                   
                    receive_effort[2].store(msg->effort[0]);
                    receive_effort[1].store(msg->effort[1]);
                    receive_effort[0].store(msg->effort[2]);
                    receive_effort[3].store(msg->effort[3]);
                    receive_effort[4].store(msg->effort[4]);
                    receive_effort[5].store(msg->effort[5]);
                
            }
            void send_position()
            {
                for(int i=0;i<6;i++)
                {
                    std_msgs::Float64 msg;
                    msg.data=send_postiton[i].load();
                    pub[i].publish(msg);
                }
            }
             
         
    };


    class UrgazeboMotor:Motor
    {
        int motor_id;
        Urgazebo& urgazebo=Urgazebo::getInstance();
        public:
        UrgazeboMotor(int id):motor_id(id)
        {

        }
        int setTargetPos (double position) override
        {
             urgazebo.send_postiton[motor_id].store(position);
             return 1;
        }
        double actualPos(void) override
        {
            double position=urgazebo.receive_postiton[motor_id].load();
            return position;
        }
        int  setTargetToq(double toq) override
        {            
            urgazebo.send_postiton[motor_id].store(toq);
            return 1;
        }
         double actualToq(void) override
         {
              double toq=urgazebo.receive_effort[motor_id].load();
              return toq;
         }             

    };
 class UrgazeboTransceive:Transceive
  {
    private:
         Urgazebo& urgazebo=Urgazebo::getInstance();
    public:
         auto init()->int override
        {
                 
            //urgazebo.init();
            return 1;
        }
        auto send(void)->void override
        {
            urgazebo.send_position();

        }
        auto  receive()->void override
        {
          
        }
  };
}

#endif
