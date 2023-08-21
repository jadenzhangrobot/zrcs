/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-06-16 09:54:19
 * @Description: 关节运动相对位置指令
 */
#ifndef MOVEC_H
#define MOVEC_H
#include "behaviortree_cpp_v3/action_node.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "system/basenode.h"
#include "system/centre.h"
#include <boost/function/function_base.hpp>
#include <cmath>

#include <iostream>
#include <ostream>
#include <ruckig/ruckig.hpp>
#include "system/classfactory.h"
#include"../../src/slave.h"
#include"model/urFIKinematin.h"
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/src/Core/Matrix.h>


using namespace ruckig;

// class BtMoveJ : public BT::SyncActionNode
// {
// public:
//      zrcs_system::centre& cenobj= zrcs_system::centre::getInstance();
//     BT::Optional<std::string> res,res1,res2,res3,res4,res5;
//      bool condition=true;
//    BtMoveJ (const std::string& name, const BT::NodeConfiguration& config) :
//    BT::SyncActionNode(name, config)
//   {  
//             res  = this->getInput<std::string>("x");
//             res1 = this->getInput<std::string>("y");
//             res2 = this->getInput<std::string>("z");
//             res3 = this->getInput<std::string>("rx");
//             res4 = this->getInput<std::string>("ry");
//             res5 = this->getInput<std::string>("rz");          
      
//      // std::cout<<"--"+res.value()+" --"+res1.value()<<std::endl;
//      // cenobj.cmd_queue.push("JogabsJ --motor="+res.value()+" --position="+res1.value());     
//   }

//   // You must override the virtual function tick()
//   BT::NodeStatus tick() override
//   {
//     // if (condition) {
//     cenobj.cmd_queue.push("MoveJ --x="+res.value()+" --y="+res1.value()+" --z="+res2.value()+" --rx="+res3.value()+" --ry="+res4.value()+" --rz="+res5.value());
//        ///   condition=false;    
//   //   }
//     while (cenobj.rt_status!=2)
//     {
        
//     }
     
//      return BT::NodeStatus::SUCCESS;
//     //return BT::NodeStatus::RUNNING;
//     //std::cout << "ApproachObject: " << this->name() << std::endl;
//   }
//   //  void halt() override
//   //   {



//   //   }
//   static BT::PortsList providedPorts()
//   {
//     BT::PortsList ports;
//     const char* description = "X-axis movement";
//     ports.emplace(BT::InputPort<std::string>("x", description));
    
//     const char* description1 = "Y-axis movement";
//     ports.emplace(BT::InputPort<std::string>("y", description1));
    
//     const char* description2 = "Z-axis movement";
//     ports.emplace(BT::InputPort<std::string>("z", description2));

//     const char* description3 = "X-axis rotation";
//     ports.emplace(BT::InputPort<std::string>("rx", description3));

//     const char* description4 = "Y-axis rotation";
//     ports.emplace(BT::InputPort<std::string>("ry", description4));

//     const char* description5 = "Z-axis rotation";
//     ports.emplace(BT::InputPort<std::string>("rz", description5));   

//     return ports;
//    // const char* description = "motor_id";
//     //return {BT::InputPort<std::string>("motor", description)};
//   }
  
// };
 class MoveC:public zrcs_system::Basenode
  {
    public:
       
            zrcs_system::centre& cenobj=zrcs_system::centre::getInstance();
            Ruckig<1> otg {0.001}; 
            InputParameter<1> input;
            OutputParameter<1> output;
            Ur ur;
            double L;
            double T[16];
            double joint[6];
           // double pose[6];
            // double p[3];
            // double p0[3];
            // double p1[3];
            // Eigen::Matrix<double,4,4> R;
            // Eigen::Matrix<double,4,4> initpose;
            double wx;
            double wy;
            double wz;
            double wxy;
            double wxz;
            double wyz;
            Eigen::Vector3d W_;
            Eigen::Matrix<double,3,1> center_xyz;
            struct PT3
            {
              double x, y, z;
            };
      int solveCenterPointOfCircle(std::vector<PT3> pd, double centerpoint[])
        {
          double a1, b1, c1, d1;
          double a2, b2, c2, d2;
          double a3, b3, c3, d3;

          double x1 = pd[0].x, y1 = pd[0].y, z1 = pd[0].z;
          double x2 = pd[1].x, y2 = pd[1].y, z2 = pd[1].z;
          double x3 = pd[2].x, y3 = pd[2].y, z3 = pd[2].z;

          a1 = (y1*z2 - y2*z1 - y1*z3 + y3*z1 + y2*z3 - y3*z2);
          b1 = -(x1*z2 - x2*z1 - x1*z3 + x3*z1 + x2*z3 - x3*z2);
          c1 = (x1*y2 - x2*y1 - x1*y3 + x3*y1 + x2*y3 - x3*y2);
          d1 = -(x1*y2*z3 - x1*y3*z2 - x2*y1*z3 + x2*y3*z1 + x3*y1*z2 - x3*y2*z1);

          a2 = 2 * (x2 - x1);
          b2 = 2 * (y2 - y1);
          c2 = 2 * (z2 - z1);
          d2 = x1 * x1 + y1 * y1 + z1 * z1 - x2 * x2 - y2 * y2 - z2 * z2;

          a3 = 2 * (x3 - x1);
          b3 = 2 * (y3 - y1);
          c3 = 2 * (z3 - z1);
          d3 = x1 * x1 + y1 * y1 + z1 * z1 - x3 * x3 - y3 * y3 - z3 * z3;

          centerpoint[0] = -(b1*c2*d3 - b1*c3*d2 - b2*c1*d3 + b2*c3*d1 + b3*c1*d2 - b3*c2*d1)
            /(a1*b2*c3 - a1*b3*c2 - a2*b1*c3 + a2*b3*c1 + a3*b1*c2 - a3*b2*c1);
          centerpoint[1] =  (a1*c2*d3 - a1*c3*d2 - a2*c1*d3 + a2*c3*d1 + a3*c1*d2 - a3*c2*d1)
            /(a1*b2*c3 - a1*b3*c2 - a2*b1*c3 + a2*b3*c1 + a3*b1*c2 - a3*b2*c1);
          centerpoint[2] = -(a1*b2*d3 - a1*b3*d2 - a2*b1*d3 + a2*b3*d1 + a3*b1*d2 - a3*b2*d1)
            /(a1*b2*c3 - a1*b3*c2 - a2*b1*c3 + a2*b3*c1 + a3*b1*c2 - a3*b2*c1);

          return 0;
        }        
      bool init() override
      {
         cmdline::parser cmd;
         cmd.add<double>("x0", 'x', " x0-axis", false, 0, cmdline::range(-4.0, 4.0));
         cmd.add<double>("y0", 'y', " y0-axis", false, 1, cmdline::range(-3.14, 3.14));
         cmd.add<double>("z0", 'z', " z0-axis", false, 0, cmdline::range(-3.14, 3.14));
         cmd.add<double>("x1", 'r', " x1-axis", false, 0, cmdline::range(-4.0, 4.0));
         cmd.add<double>("y1", 'p', " y1=-axis", false, 0, cmdline::range(-4.0, 4.0));
         cmd.add<double>("z1", 'b', " z1-axis", false, 1, cmdline::range(-4.0, 4.0));
         
         
          if (!cenobj.nrt_cmdParam.empty()) 
          {
              std::string str=cenobj.nrt_cmdParam.front();
              cmd.parse_check(str);
              cenobj.nrt_cmdParam.pop();
          }   
          
       
          //  pose[0]=0;
          //  pose[1]=0;
          //  pose[2]=0;
          //  pose[3]=3.1415926;
          //  pose[4]=0;
          //  pose[5]=cmd.get<double>("rz");
                  
           for(int i=0;i<JointNum;i++)
           {
             joint[i]=cenobj.ec_control->motors[i]->actualPos();
           }
           ur.forward(joint, T);
          // L=std::abs((pose[0]-T[3])*(pose[0]-T[3])+(pose[1]-T[7])*(pose[1]-T[7])+(pose[2]-T[11])*(pose[2]-T[11]));

          double x0=T[3];double y0=T[7];double z0=T[11];
          //double x0=0.5;double y0=1;double z0=0;
          double xm=cmd.get<double>("x0"); double ym=cmd.get<double>("y0");double zm=cmd.get<double>("z0");
          double xf=cmd.get<double>("x1"); double yf=cmd.get<double>("y1");double zf=cmd.get<double>("z1");
        //   Eigen::Matrix<double,3,3> matrix1;
        //   matrix1 <<x0,y0,z0,
        //             xm,ym,zm,
        //             xf,yf,zf;
        //  Eigen::Matrix<double,3,1> con_;
        //  con_ <<1,1,1;
        //  Eigen::Matrix<double,3,1> ABC=matrix1.inverse()*con_;

        //   Eigen::Matrix<double,3,3> matrix2;
        //   matrix2<< 2*(xm-x0),2*(ym-y0),2*(zm-z0),
        //             2*(xf-x0),2*(yf-y0),2*(zf-z0),
        //             ABC(0,0),ABC(1,0),ABC(2,0);
                     
        //   Eigen::Matrix<double,3,1> vector;
        //   vector<<-(x0*x0)-(y0*y0)-(z0*z0)+xm*xm+ym*ym+zm*zm,-(x0*x0)-(y0*y0)-(z0*z0)+xf*xf+yf*yf+zf*zf,1;
      
        //   //求圆心坐标xyz         
          
            struct PT3 P1={x0,y0,z0};
            struct PT3 P2={xm,ym,zm};
            struct PT3 P3={xf,yf,zf};
            std::vector<PT3> v;
            v.push_back(P1);
            v.push_back(P2);
            v.push_back(P3);
           // double pp[3];
            solveCenterPointOfCircle(v,center_xyz.data());

          Eigen::Vector3d  p0w(x0-center_xyz(0,0),y0-center_xyz(1,0),z0-center_xyz(2,0));
          Eigen::Vector3d  pFw(xf-center_xyz(0,0),yf-center_xyz(1,0),zf-center_xyz(2,0));
          //计算2个向量之间的夹角
          double angel=std::acos(p0w.normalized().dot(pFw.normalized()));


        // 计算叉乘
         Eigen::Vector3d W = p0w.cross(pFw).normalized();
         
           W_=W;
        //  wx=W_(0);
        //  wy=W_(1);
        //  wz=W_(2);
        //  wxy=wx+wy;
        //  wxz=wx+wz;
        //  wyz=wy+wz;
        
        // R<<cos(a)+wx*wx*(1-cos(a)),wxy*(1-cos(a)-wz*sin(a)),wxz*(1-cos(a))+wy*sin(a),0,
        //    wxy*(1-cos(a))+wz*sin(a),cos(a)+(wy*wy)*(1-cos(a)),wyz*(1-cos(a))-wx*sin(a),0,
        //    wxz*(1-cos(a))-wy*sin(a),wyz*(1-cos(a))+wx*sin(a),cos(a)+wz*wz*(1-cos(a)),0,
        //    1,1,1,1;
           std::cout<<"XYZ"<<"   "<<center_xyz<<std::endl;
           std::cout<<"p0w"<<"   "<<p0w<<std::endl;
           std::cout<<"pFw"<<"   "<<pFw<<std::endl;
           std::cout<<"www"<<"   "<<W_<<std::endl;
           std::cout<<"angle"<<"   "<<angel<<std::endl;
        
            input.current_position[0]=0;              
            input.current_velocity[0]= 0;
            input.current_acceleration[0] =0;
                              
            input.target_position[0]=angel;
            input.target_velocity[0] = 0;
            input.target_acceleration[0] =0;
            input.max_velocity[0] = 0.2;
            input.max_acceleration[0] = 0.1;
            input.max_jerk[0] =0.1;
         
            return  true;
    }
  
      void  excute_rt(void) override
      {         
                                      
                    if(otg.update(input, output) == Result::Working)            
                     { 
                       
                       auto& p = output.new_position;
                       //double a=p[0];
                       //Eigen::Vector3d axis(1.0, 0.0, 0.0); // 旋转轴向量
                      double angle = p[0]; // 旋转角度（以弧度表示）

    // 使用AngleAxis类创建旋转对象
                        Eigen::AngleAxisd rotation(angle,W_);

    // 将旋转对象转换为旋转矩阵
                      Eigen::Matrix3d rotationMatrix = rotation.toRotationMatrix();
                      // Eigen::Matrix<double,3,3> R;
                      // R<<cos(a)+wx*wx*(1-cos(a)),wxy*(1-cos(a)-wz*sin(a)),wxz*(1-cos(a))+wy*sin(a),
                      //   wxy*(1-cos(a))+wz*sin(a),cos(a)+(wy*wy)*(1-cos(a)),wyz*(1-cos(a))-wx*sin(a),
                      //   wxz*(1-cos(a))-wy*sin(a),wyz*(1-cos(a))+wx*sin(a),cos(a)+wz*wz*(1-cos(a));
                       Eigen::Vector3d xyz(T[3]-center_xyz(0,0),T[7]-center_xyz.coeffByOuterInner(1,0),T[11]-center_xyz(2.0));
                       Eigen::Matrix<double,4,4> qcmatrix;
                    
                       qcmatrix<< rotationMatrix,center_xyz,
                                  0,0,0,1;
                        
                       double pose_[6];
                       Eigen::Matrix<double,4,1> xyz_;
                       xyz_<<xyz,1;
                       Eigen::Matrix<double,4,1> txyz;
                       txyz =qcmatrix*xyz_;
                     
                       pose_[0]=txyz(0,0);
                       pose_[1]=txyz(1,0);
                       pose_[2]=txyz(2,0);
                       pose_[3]=3.1415926;
                       pose_[4]=0;
                       pose_[5]=1.5708;
                       //double joint[6];
                      //  double pose_[16];
                      //  double a=p[0];
                      
                     // this->matrix_multiply(R, T, pose_);
                     
                       double target_joint[6];
                       int ret= ur.r_inverse(joint,pose_,target_joint);
                       for (int i=0; i<JointNum; i++) 
                       {
                        
                         cenobj.ec_control->motors[i]->setTargetPos(target_joint[i]);
                         //std::cout<<"JogabsJ motor-----"<<i<<"  "<<p[i]<<std::endl;
                       }                                                                                             
                       output.pass_to_input(input);
                       rtnode_status=RUNNING;
                     }
                    else
                     {
                     // if( forcenobj.ec_control->motors[i]->setTargetPos(p[i])==cenobj.ec_control->motors[i]->act)                    
                      rtnode_status=SUCCESS;     
                                    
                     }
         
      }      
   
  };

 REGISTER(MoveC);

#endif