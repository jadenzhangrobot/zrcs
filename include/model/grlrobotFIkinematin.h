/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 10:25:59
 * @LastEditTime: 2023-04-20 15:53:07
 * @Description: 挂轨四轴机器人算法
 * 
 */

#ifndef GLRRBOTFIKINEMATIN_H
#define GLRRBOTFIKINEMATIN_H

#include <iomanip>
#include <iostream>
#include <eigen3/Eigen/Dense>
#include <cmath>
#include <kdl/joint.hpp>
#include <math.h>
#include "sensor_msgs/JointState.h"
#include <memory>
#include <kdl/chain.hpp>
#include <kdl/frames.hpp>
#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/chainiksolverpos_nr_jl.hpp>
#include <kdl/segment.hpp>
#include <kdl/chainfksolver.hpp>
#include <kdl/chainiksolver.hpp>
#include <kdl/chainiksolverpos_lma.hpp>
#include <kdl/chainiksolverpos_nr.hpp>
#include <kdl/chainiksolverpos_nr_jl.hpp>
#include <kdl/chainfksolverpos_recursive.hpp>
#include "system/centre.h"
#include <trac_ik/trac_ik.hpp>
class glrrobot
{
     private:
         double eps=1E-5;
         int maxiter=500;
         double eps_joints=1E-15;
         KDL::JntArray lower_joint_limits,upper_joint_limits;   
         KDL::Chain chain;
         KDL::ChainFkSolverPos_recursive FKSolver; 
         
         centre& cenobj=centre::getInstance();
         double xx_=0;
    public:
          
         glrrobot(void):FKSolver(chain)  
         {
             //joint0
            chain.addSegment(
            KDL::Segment(KDL::Joint(KDL::Joint::RotZ),
                KDL::Frame::DH(0, -M_PI_2, -0.08, 0)));
	          // joint1
	         chain.addSegment(
			     KDL::Segment(KDL::Joint(KDL::Joint::RotZ),
					  KDL::Frame::DH(-0.35, 0, -0.05, 0)));
	       
           // joint 2
          chain.addSegment(
              KDL::Segment(KDL::Joint(KDL::Joint::RotZ),
                  KDL::Frame::DH(-0.75, 0, 0.05, 0)));
          //joint 3
          chain.addSegment(
              KDL::Segment(KDL::Joint(KDL::Joint::RotZ),
                  KDL::Frame::DH(-0.37, 0, -0.05, 0)));   
           
            lower_joint_limits.resize(4);
            upper_joint_limits.resize(4);
            for(int i=0;i<4;i++)
            {
                 lower_joint_limits(i)=-M_PI;
                 upper_joint_limits(i)=M_PI;
            }

         }
        

       /**
        * @description: 机器人正解
        * @param {double} j1
        * @param {double} j2
        * @param {double} j3
        * @param {double} j4
        * @return {*}
        */       
       void Forward_kinematics(double j1,double j2,double j3,double j4)
       {
            KDL::Frame end_effector_pose;
            KDL::JntArray jointAngles = KDL::JntArray(4);
            jointAngles(0) = j1;       
            jointAngles(1) = j2;        
            jointAngles(2) = j3;       
            jointAngles(3) = j4;   
            double roll, pitch, yaw, x ,y ,z;
            end_effector_pose.M.GetRPY(roll, pitch, yaw);

            // Eigen::Matrix3d R;
            // for (int i = 0; i < 3; i++)
            // {
            //   for (int j = 0; j < 3; j++) 
            //   {
            //       R(i,j)= end_effector_pose(i, j);
                
            //   }
		
            // }
            //R.eulerAngles(roll, pitch, yaw);
            std::cout<<roll<<std::endl;
            std::cout<<pitch<<std::endl;
            std::cout<<yaw<<std::endl;
                             
       }

     
    // Eigen::Vector3f 
      /**
       * @description: 机器人逆解
       * @param {double} x 
       * @param {double} y
       * @param {double} z
       * @param {double} rx
       * @param {double} ry
       * @param {double} rz
       * @return {*}
       */       
      KDL::JntArray inverse_kinematics( double x,double y,double z,double rx,double ry,double rz)
      {
           // x=1-1*cos(rz)+x;
           // y=-1*sin(rz)+y;
        
            KDL::Frame end_effector_pose;
            KDL::JntArray jointAngles = KDL::JntArray(4);
                for(int i=0;i<4;i++)
                {
                    
                    
                    jointAngles(i) = cenobj.ec_control->motors[i+1]->actualPos();
                }
        
            FKSolver.JntToCart(jointAngles,end_effector_pose);

        //double L=std::sqrt(end_effector_pose(1,3)*end_effector_pose(1,3)+end_effector_pose(2,3)*end_effector_pose(2,3));
           //  x=L-L*cos(rz)+x;
         //    y=-L*sin(rz)+y;
        
           // KDL::Frame target_pose = KDL::Frame(KDL::Rotation::EulerZYX(rz, ry, rx)) * KDL::Frame(KDL::Vector(x, y, z));
           //// KDL::Frame target_pose(KDL::Rotation::RPY(rx, ry ,rz), KDL::Vector(x, y, z));
            KDL::Frame target_pose(KDL::Rotation::EulerZYX(rz, ry, rx), KDL::Vector(x, y, z));
            
            KDL::JntArray Joint=KDL::JntArray(4);
           // KDL::JntArray result=KDL::JntArray(4);
            KDL::JntArray _Joint=KDL::JntArray(8);
            KDL::Frame  target_pose1=end_effector_pose*target_pose;
             // target_pose=target_pose;
                for (int i = 0; i < 4; i++){
                            for (int j = 0; j < 4; j++) {
                                double a = target_pose(i, j);
                                if (a < 0.0001 && a > -0.001) {
                                    a = 0.0;
                                }
                                std::cout << std::setprecision(4) << a << "\t\t";
                            }
                            std::cout << std::endl;
                        }
             TRAC_IK::TRAC_IK ik_solver_ =TRAC_IK::TRAC_IK(chain,lower_joint_limits,upper_joint_limits,0.005,1e-5,TRAC_IK::Speed);
             int error = ik_solver_.CartToJnt(jointAngles,target_pose1, Joint);
               
             for(int i=0;i<4;i++)
             {
                 _Joint(i)=Joint(i);

             }
             
             for(int i=0;i<4;i++)
             {
                 _Joint(i+4)=jointAngles(i);       
               
             }
            
             
            if (error >= 0) 
            {
                // 打印机器人关节角度
                for (int i = 0; i < 8; i++) 
                {
                  std::cout << "Joint " << i+1 << ": " << _Joint(i) << std::endl;
                }
               
            }
            else 
            {
                std::cout << "Failed to calculate inverse kinematics!" <<error<< std::endl;
               
            }
                
             return _Joint;
           
      }
};
#endif