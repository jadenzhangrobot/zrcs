/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 10:25:59
 * @LastEditTime: 2023-05-26 11:43:56
 * @Description: 
 * 
 */
#ifndef TRAJECTORY_H
#define TRAJECTORY_H
#include "ros/init.h"
#include "ros/rate.h"
#include "sensor_msgs/JointState.h"
#include <kdl/jntarray.hpp>
#include <ruckig/ruckig.hpp>
#include <memory>
#include <shared_mutex>
#include "motor.h"
#include <iostream>
#include <thread>

using namespace ruckig;
class trajectory 
{
      public:
            Ruckig<1> otg {0.01}; 
            InputParameter<5> input;
            OutputParameter<5> output;
            bool flag=false;
           
    trajectory()
    {
        
           
    }
    ~trajectory()
    {
       
        

    }
    
    
    void set_trajectory(KDL::JntArray p)
         {
                        // for(int i=0;i<5;i++)
                        // {
                        // std::cout<<"set-joint"<<p(i)<<std::endl;
                        // }
          sensor_msgs::JointState j_angel=motor::getInstance().get_position();             
          // input.current_position = {j_angel.position[0],j_angel.position[1],j_angel.position[2]-1.5708,j_angel.position[3],j_angel.position[4]};
           input.current_position = {p(5),p(6),p(7),p(8),p(9)};
           input.current_velocity = {0.0,0,0,0,0};
           input.current_acceleration = {0.0,0,0,0,0};
           
           input.target_position = {p(0),p(1),p(2),p(3),p(4)};
           input.target_velocity = {0.0};
           input.target_acceleration = {0.0};
           input.max_velocity = {0.3,0.3,0.3,0.3,0.3};
           input.max_acceleration = {1,1,1,1,1};
           input.max_jerk = {1,1,1,1,1};
         }
        void interpolation()
          {
            
               flag=true;
               while(flag)  
               {        
                      
                    if(otg.update(input, output) == Result::Working)            
                     { 
                        //flag=false;
                        auto& p = output.new_position;
                      
                        motor::getInstance().set_positon(p[0],p[1],p[2],p[3],p[4]);
                                                  
                       output.pass_to_input(input);
                     }
                    else
                     {
                          flag=false;
                     }
                     
                     std::this_thread::sleep_for(std::chrono::milliseconds(10));
                     
                    
                    
               }
             
               
             
          }
    



};
#endif