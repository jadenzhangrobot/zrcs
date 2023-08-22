#ifndef URKFIKNEMATIN_H
#define URKFIKNEMATIN_H
#include <boost/function/function_base.hpp>
#include <boost/mpl/assert.hpp>
#include <cmath>
#include <cstdint>
#include <math.h>
#include <memory>
#include <ostream>
#include <stdio.h>
#include <iostream>
//#include <eigen3/Eigen/Dense>
//#include <kdl/frames.hpp>
class Ur
{
 public:
   const double ZERO_THRESH = 0.00000001;
   const double PI = M_PI;
    const double d1 =  0.089159;
    const double a2 = -0.42500;
    const double a3 = -0.39225;
    const double d4 =  0.10915;
    const double d5 =  0.09465;
    const double d6 =  0.0823;
   Ur()
   {
   }
  int SIGN(double x) 
    {
      return (x > 0) - (x < 0);
    }
    void matrix_multiply(double A[], double B[], double C[])
     {
            for (int i = 0; i < 4; ++i) 
            {
                  for (int j = 0; j < 4; ++j) 
                  {
                        double sum = 0.00;
                        for (int k = 0; k < 4; ++k)
                        {
                         sum += A[i * 4 + k] * B[k * 4 + j];
                        }
                        C[i * 4 + j] = sum;
                  }
            }
     }
    int r_inverse(double* joint,double* pose, double* tatget_joint)
    {
       double x=pose[0];
       double y=pose[1];
       double z=pose[2];
       double rx=pose[3];//rx
       double ry=pose[4];//ry
       double rz=pose[5];//rz
       
     
      // double r[16]={std::cos(yaw) * std::cos(pitch), -std::cos(yaw) * std::sin(pitch), std::sin(yaw),x,
      //              std::cos(pitch) * std::sin(yaw) * std::sin(roll) + std::cos(roll) * std::sin(yaw),
      //              std::cos(yaw) * std::cos(roll) - std::sin(pitch) * std::sin(yaw) * std::sin(roll),
      //             -std::cos(yaw) * std::sin(roll) - std::cos(roll) * std::sin(pitch) * std::sin(yaw),y,
      //              std::sin(pitch) * std::sin(yaw) * std::cos(roll) - std::cos(yaw) * std::sin(roll),
      //              std::cos(yaw) * std::sin(pitch) * std::cos(roll) + std::sin(yaw) * std::sin(roll),
      //              std::cos(pitch) * std::cos(roll),z,0,0,0,1};
      
      double R[16]={cos(ry)*cos(rz),sin(rx)*sin(ry)*cos(rz)-cos(rx)*sin(rz),cos(rx)*sin(ry)*cos(rz)+sin(rx)*sin(rz),x,
                    cos(ry)*sin(rz),sin(rx)*sin(ry)*sin(rz)+cos(rx)*cos(rz),cos(rx)*sin(ry)*sin(rz)-sin(rx)*cos(rz),y,
                    -sin(ry),sin(rx)*cos(ry),cos(rx)*cos(ry),z,
                    0,0,0,1};

        // for(int i=0;i<4;i++)
        // {
        //   for(int j=0;j<4;j++)
        //   {
        //         std::cout <<R[j+i*4] << " ";
            
        //   }
        //   std::cout << std::endl;          
        // }
       

  
        double joint1[6]={0,0,0,0,0,0};                         
        double T[16];
        double target_pose[16];
        this->forward(joint1, T);
        this->matrix_multiply(T,R,target_pose);
        // for(int i=0;i<4;i++)
        // {
        //   for(int j=0;j<4;j++)
        //   {
        //         std::cout <<T[j+i*4] << " ";
            
        //   }
        //   std::cout << std::endl;
           

        // }
       

      
        double out_joint[8*6];
        int ret= this->inverse(R, out_joint,0); 
     
        for(int i=0;i<ret*6;i++)
        {
           if (out_joint[i]>=PI) {
              out_joint[i]=out_joint[i]-2*PI;
           }
        }
              double a[ret];
            for (int i=0;i<ret;i++) 
            {
                
                if((out_joint[i*6+0]>=-PI&&out_joint[i*6+0]<=PI)&&
                (out_joint[i*6+1]>=-PI&&out_joint[i*6+1]<=0)&&
                (out_joint[i*6+2]>=-2.3562&&out_joint[i*6+2]<=2.3562)&&
                (out_joint[i*6+3]>=-4&&out_joint[i*6+3]<=1.2)&&
                (out_joint[i*6+4]>=-2.3562&&out_joint[i*6+4]<=2.3562)&&
                (out_joint[i*6+5]>=-2*PI&&out_joint[i*6+5]<=2*PI))
                {
                    // if (out_joint[i*6+1]>-1.5708) {
                       
                    //   if (out_joint[i*6+2]<0&&out_joint[i*6+3]<-1.5708) {
                    //   a[i]=100000000000;
                    //   }                     
                    // }
                    
                    double a1=(out_joint[i*6+0]-joint[0])*(out_joint[i*6+0]-joint[0]);
                    double a2=(out_joint[i*6+1]-joint[1])*(out_joint[i*6+1]-joint[1]);
                    double a3=(out_joint[i*6+2]-joint[2])*(out_joint[i*6+2]-joint[2]);
                    double a4=(out_joint[i*6+3]-joint[3])*(out_joint[i*6+3]-joint[3]);
                    double a5=(out_joint[i*6+4]-joint[4])*(out_joint[i*6+4]-joint[4]);
                    double a6=(out_joint[i*6+5]-joint[5])*(out_joint[i*6+5]-joint[5]);
                    a[i]=1000*a1+100*a3+10*a5+a2+0.1*a4+0.01*a6;
                   
                    //std::cout<<"i"<<"   "<<i<<std::endl;                  
                }
                else 
                {
                    a[i]=100000000000;   
                } 
                     
            }
               
             int minIndex = 0; // 初始化最小值的位置为0
             std::uint64_t minValue = a[0]; // 初始化最小值为数组的第一个元素
              
              for (int i = 1; i < ret; i++) 
              {
                 
                  // 如果当前元素小于最小值，则更新最小值和最小值的位置
                  if (a[i] < minValue) 
                  {
                      minValue = a[i];
                      minIndex = i;
                    
                  }
              }
            
        // std::cout<<"minindex"<<"     "<<minIndex<<std::endl;   

      //  for(int i=0;i<ret;i++) 
      //     printf("%1.6f %1.6f %1.6f %1.6f %1.6f %1.6f\n", 
      //     out_joint[i*6+0], out_joint[i*6+1], out_joint[i*6+2], out_joint[i*6+3], out_joint[i*6+4], out_joint[i*6+5]);
        
        
        
        for(int i=0;i<6;i++)
        {
          tatget_joint[i]=out_joint[minIndex*6+i];
        }
        return ret;
    }
    int movec_inverse(double* joint,double* R, double* tatget_joint)
    {
      //  double x=pose[0];
      //  double y=pose[1];
      //  double z=pose[2];
      //  double rx=pose[3];//rx
      //  double ry=pose[4];//ry
      //  double rz=pose[5];//rz
       
     
      // double r[16]={std::cos(yaw) * std::cos(pitch), -std::cos(yaw) * std::sin(pitch), std::sin(yaw),x,
      //              std::cos(pitch) * std::sin(yaw) * std::sin(roll) + std::cos(roll) * std::sin(yaw),
      //              std::cos(yaw) * std::cos(roll) - std::sin(pitch) * std::sin(yaw) * std::sin(roll),
      //             -std::cos(yaw) * std::sin(roll) - std::cos(roll) * std::sin(pitch) * std::sin(yaw),y,
      //              std::sin(pitch) * std::sin(yaw) * std::cos(roll) - std::cos(yaw) * std::sin(roll),
      //              std::cos(yaw) * std::sin(pitch) * std::cos(roll) + std::sin(yaw) * std::sin(roll),
      //              std::cos(pitch) * std::cos(roll),z,0,0,0,1};
      
      // double R[16]={cos(ry)*cos(rz),sin(rx)*sin(ry)*cos(rz)-cos(rx)*sin(rz),cos(rx)*sin(ry)*cos(rz)+sin(rx)*sin(rz),x,
      //               cos(ry)*sin(rz),sin(rx)*sin(ry)*sin(rz)+cos(rx)*cos(rz),cos(rx)*sin(ry)*sin(rz)-sin(rx)*cos(rz),y,
      //               -sin(ry),sin(rx)*cos(ry),cos(rx)*cos(ry),z,
      //               0,0,0,1};

        // for(int i=0;i<4;i++)
        // {
        //   for(int j=0;j<4;j++)
        //   {
        //         std::cout <<R[j+i*4] << " ";
            
        //   }
        //   std::cout << std::endl;          
        // }
       

  
        double joint1[6]={0,0,0,0,0,0};                         
        double T[16];
        double target_pose[16];
        this->forward(joint1, T);
        this->matrix_multiply(T,R,target_pose);
        // for(int i=0;i<4;i++)
        // {
        //   for(int j=0;j<4;j++)
        //   {
        //         std::cout <<T[j+i*4] << " ";
            
        //   }
        //   std::cout << std::endl;
           

        // }
       

      
        double out_joint[8*6];
        int ret= this->inverse(R, out_joint,0); 
     
        for(int i=0;i<ret*6;i++)
        {
           if (out_joint[i]>=PI) {
              out_joint[i]=out_joint[i]-2*PI;
           }
        }
              double a[ret];
            for (int i=0;i<ret;i++) 
            {
                
                if((out_joint[i*6+0]>=-PI&&out_joint[i*6+0]<=PI)&&
                (out_joint[i*6+1]>=-PI&&out_joint[i*6+1]<=0)&&
                (out_joint[i*6+2]>=-2.3562&&out_joint[i*6+2]<=2.3562)&&
                (out_joint[i*6+3]>=-PI&&out_joint[i*6+3]<=PI)&&
                (out_joint[i*6+4]>=-2.3562&&out_joint[i*6+4]<=2.3562)&&
                (out_joint[i*6+5]>=-2*PI&&out_joint[i*6+5]<=2*PI))
                {
                 
                    double a1=(out_joint[i*6+0]-joint[0])*(out_joint[i*6+0]-joint[0]);
                    double a2=(out_joint[i*6+1]-joint[1])*(out_joint[i*6+1]-joint[1]);
                    double a3=(out_joint[i*6+2]-joint[2])*(out_joint[i*6+2]-joint[2]);
                    double a4=(out_joint[i*6+3]-joint[3])*(out_joint[i*6+3]-joint[3]);
                    double a5=(out_joint[i*6+4]-joint[4])*(out_joint[i*6+4]-joint[4]);
                    double a6=(out_joint[i*6+5]-joint[5])*(out_joint[i*6+5]-joint[5]);
                    a[i]=100*a1+a3+a5+a2+a4+a6;   
                              
                }
                else 
                {
                    a[i]=100000000000000;   
                } 
               // std::cout<<"a[i]:"<<a[i]<<std::endl;            
            }

             int minIndex = 0; // 初始化最小值的位置为0
             int minValue = a[0]; // 初始化最小值为数组的第一个元素

              for (int i = 1; i < ret; i++) 
              {
                  // 如果当前元素小于最小值，则更新最小值和最小值的位置
                  if (a[i] < minValue) 
                  {
                      minValue = a[i];
                      minIndex = i;
                  }
              }
            
         //std::cout<<"minindex"<<minIndex<<std::endl;   

       for(int i=0;i<ret;i++) 
          printf("%1.6f %1.6f %1.6f %1.6f %1.6f %1.6f\n", 
          out_joint[i*6+0], out_joint[i*6+1], out_joint[i*6+2], out_joint[i*6+3], out_joint[i*6+4], out_joint[i*6+5]);
        
        
        
        for(int i=0;i<6;i++)
        {
          tatget_joint[i]=out_joint[minIndex*6+i];
        }
        return ret;
    }
  void forward(const double* q, double* T)
  {
      double s1 = sin(*q), c1 = cos(*q); q++;
      double q23 = *q, q234 = *q, s2 = sin(*q), c2 = cos(*q); q++;
      double s3 = sin(*q), c3 = cos(*q); q23 += *q; q234 += *q; q++;
      double s4 = sin(*q), c4 = cos(*q); q234 += *q; q++;
      double s5 = sin(*q), c5 = cos(*q); q++;
      double s6 = sin(*q), c6 = cos(*q); 
      double s23 = sin(q23), c23 = cos(q23);
      double s234 = sin(q234), c234 = cos(q234);
      *T = c234*c1*s5 - c5*s1; T++;
      *T = c6*(s1*s5 + c234*c1*c5) - s234*c1*s6; T++;
      *T = -s6*(s1*s5 + c234*c1*c5) - s234*c1*c6; T++;
      *T = d6*c234*c1*s5 - a3*c23*c1 - a2*c1*c2 - d6*c5*s1 - d5*s234*c1 - d4*s1; T++;
      *T = c1*c5 + c234*s1*s5; T++;
      *T = -c6*(c1*s5 - c234*c5*s1) - s234*s1*s6; T++;
      *T = s6*(c1*s5 - c234*c5*s1) - s234*c6*s1; T++;
      *T = d6*(c1*c5 + c234*s1*s5) + d4*c1 - a3*c23*s1 - a2*c2*s1 - d5*s234*s1; T++;
      *T = -s234*s5; T++;
      *T = -c234*s6 - s234*c5*c6; T++;
      *T = s234*c5*s6 - c234*c6; T++;
      *T = d1 + a3*s23 + a2*s2 - d5*(c23*c4 - s23*s4) - d6*s5*(c23*s4 + s23*c4); T++;
      *T = 0.0; T++; *T = 0.0; T++; *T = 0.0; T++; *T = 1.0;
  } 
   void forward_all(const double* q, double* T1, double* T2, double* T3, 
                                    double* T4, double* T5, double* T6) {
    double s1 = sin(*q), c1 = cos(*q); q++; // q1
    double q23 = *q, q234 = *q, s2 = sin(*q), c2 = cos(*q); q++; // q2
    double s3 = sin(*q), c3 = cos(*q); q23 += *q; q234 += *q; q++; // q3
    q234 += *q; q++; // q4
    double s5 = sin(*q), c5 = cos(*q); q++; // q5
    double s6 = sin(*q), c6 = cos(*q); // q6
    double s23 = sin(q23), c23 = cos(q23);
    double s234 = sin(q234), c234 = cos(q234);

    if(T1 != NULL) {
      *T1 = c1; T1++;
      *T1 = 0; T1++;
      *T1 = s1; T1++;
      *T1 = 0; T1++;
      *T1 = s1; T1++;
      *T1 = 0; T1++;
      *T1 = -c1; T1++;
      *T1 = 0; T1++;
      *T1 =       0; T1++;
      *T1 = 1; T1++;
      *T1 = 0; T1++;
      *T1 =d1; T1++;
      *T1 =       0; T1++;
      *T1 = 0; T1++;
      *T1 = 0; T1++;
      *T1 = 1; T1++;
    }

    if(T2 != NULL) {
      *T2 = c1*c2; T2++;
      *T2 = -c1*s2; T2++;
      *T2 = s1; T2++;
      *T2 =a2*c1*c2; T2++;
      *T2 = c2*s1; T2++;
      *T2 = -s1*s2; T2++;
      *T2 = -c1; T2++;
      *T2 =a2*c2*s1; T2++;
      *T2 =         s2; T2++;
      *T2 = c2; T2++;
      *T2 = 0; T2++;
      *T2 =   d1 + a2*s2; T2++;
      *T2 =               0; T2++;
      *T2 = 0; T2++;
      *T2 = 0; T2++;
      *T2 =                 1; T2++;
    }

    if(T3 != NULL) {
      *T3 = c23*c1; T3++;
      *T3 = -s23*c1; T3++;
      *T3 = s1; T3++;
      *T3 =c1*(a3*c23 + a2*c2); T3++;
      *T3 = c23*s1; T3++;
      *T3 = -s23*s1; T3++;
      *T3 = -c1; T3++;
      *T3 =s1*(a3*c23 + a2*c2); T3++;
      *T3 =         s23; T3++;
      *T3 = c23; T3++;
      *T3 = 0; T3++;
      *T3 =     d1 + a3*s23 + a2*s2; T3++;
      *T3 =                    0; T3++;
      *T3 = 0; T3++;
      *T3 = 0; T3++;
      *T3 =                                     1; T3++;
    }

    if(T4 != NULL) {
      *T4 = c234*c1; T4++;
      *T4 = s1; T4++;
      *T4 = s234*c1; T4++;
      *T4 =c1*(a3*c23 + a2*c2) + d4*s1; T4++;
      *T4 = c234*s1; T4++;
      *T4 = -c1; T4++;
      *T4 = s234*s1; T4++;
      *T4 =s1*(a3*c23 + a2*c2) - d4*c1; T4++;
      *T4 =         s234; T4++;
      *T4 = 0; T4++;
      *T4 = -c234; T4++;
      *T4 =                  d1 + a3*s23 + a2*s2; T4++;
      *T4 =                         0; T4++;
      *T4 = 0; T4++;
      *T4 = 0; T4++;
      *T4 =                                                  1; T4++;
    }

    if(T5 != NULL) {
      *T5 = s1*s5 + c234*c1*c5; T5++;
      *T5 = -s234*c1; T5++;
      *T5 = c5*s1 - c234*c1*s5; T5++;
      *T5 =c1*(a3*c23 + a2*c2) + d4*s1 + d5*s234*c1; T5++;
      *T5 = c234*c5*s1 - c1*s5; T5++;
      *T5 = -s234*s1; T5++;
      *T5 = - c1*c5 - c234*s1*s5; T5++;
      *T5 =s1*(a3*c23 + a2*c2) - d4*c1 + d5*s234*s1; T5++;
      *T5 =                           s234*c5; T5++;
      *T5 = c234; T5++;
      *T5 = -s234*s5; T5++;
      *T5 =                          d1 + a3*s23 + a2*s2 - d5*c234; T5++;
      *T5 =                                                   0; T5++;
      *T5 = 0; T5++;
      *T5 = 0; T5++;
      *T5 =                                                                                 1; T5++;
    }

    if(T6 != NULL) {
      *T6 =   c6*(s1*s5 + c234*c1*c5) - s234*c1*s6; T6++;
      *T6 = - s6*(s1*s5 + c234*c1*c5) - s234*c1*c6; T6++;
      *T6 = c5*s1 - c234*c1*s5; T6++;
      *T6 =d6*(c5*s1 - c234*c1*s5) + c1*(a3*c23 + a2*c2) + d4*s1 + d5*s234*c1; T6++;
      *T6 = - c6*(c1*s5 - c234*c5*s1) - s234*s1*s6; T6++;
      *T6 = s6*(c1*s5 - c234*c5*s1) - s234*c6*s1; T6++;
      *T6 = - c1*c5 - c234*s1*s5; T6++;
      *T6 =s1*(a3*c23 + a2*c2) - d4*c1 - d6*(c1*c5 + c234*s1*s5) + d5*s234*s1; T6++;
      *T6 =                                       c234*s6 + s234*c5*c6; T6++;
      *T6 = c234*c6 - s234*c5*s6; T6++;
      *T6 = -s234*s5; T6++;
      *T6 =                                                      d1 + a3*s23 + a2*s2 - d5*c234 - d6*s234*s5; T6++;
      *T6 =                                                                                                   0; T6++;
      *T6 = 0; T6++;
      *T6 = 0; T6++;
      *T6 =                                                                                                                                            1; T6++;
    }
  }
  
   int inverse(const double* T, double* q_sols, double q6_des) 
   {
    int num_sols = 0;
    double T02 = -*T; T++; double T00 =  *T; T++; double T01 =  *T; T++; double T03 = -*T; T++; 
    double T12 = -*T; T++; double T10 =  *T; T++; double T11 =  *T; T++; double T13 = -*T; T++; 
    double T22 =  *T; T++; double T20 = -*T; T++; double T21 = -*T; T++; double T23 =  *T;

    ////////////////////////////// shoulder rotate joint (q1) //////////////////////////////
    double q1[2];
    {
      double A = d6*T12 - T13;
      double B = d6*T02 - T03;
      double R = A*A + B*B;
      if(fabs(A) < ZERO_THRESH) {
        double div;
        if(fabs(fabs(d4) - fabs(B)) < ZERO_THRESH)
          div = -SIGN(d4)*SIGN(B);
        else
          div = -d4/B;
        double arcsin = asin(div);
        if(fabs(arcsin) < ZERO_THRESH)
          arcsin = 0.0;
        if(arcsin < 0.0)
          q1[0] = arcsin + 2.0*PI;
        else
          q1[0] = arcsin;
        q1[1] = PI - arcsin;
      }
      else if(fabs(B) < ZERO_THRESH) {
        double div;
        if(fabs(fabs(d4) - fabs(A)) < ZERO_THRESH)
          div = SIGN(d4)*SIGN(A);
        else
          div = d4/A;
        double arccos = acos(div);
        q1[0] = arccos;
        q1[1] = 2.0*PI - arccos;
      }
      else if(d4*d4 > R) {
        return num_sols;
      }
      else {
        double arccos = acos(d4 / sqrt(R)) ;
        double arctan = atan2(-B, A);
        double pos = arccos + arctan;
        double neg = -arccos + arctan;
        if(fabs(pos) < ZERO_THRESH)
          pos = 0.0;
        if(fabs(neg) < ZERO_THRESH)
          neg = 0.0;
        if(pos >= 0.0)
          q1[0] = pos;
        else
          q1[0] = 2.0*PI + pos;
        if(neg >= 0.0)
          q1[1] = neg; 
        else
          q1[1] = 2.0*PI + neg;
      }
    }
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////// wrist 2 joint (q5) //////////////////////////////
    double q5[2][2];
    {
      for(int i=0;i<2;i++) {
        double numer = (T03*sin(q1[i]) - T13*cos(q1[i])-d4);
        double div;
        if(fabs(fabs(numer) - fabs(d6)) < ZERO_THRESH)
          div = SIGN(numer) * SIGN(d6);
        else
          div = numer / d6;
        double arccos = acos(div);
        q5[i][0] = arccos;
        q5[i][1] = 2.0*PI - arccos;
      }
    }
    ////////////////////////////////////////////////////////////////////////////////

    {
      for(int i=0;i<2;i++) {
        for(int j=0;j<2;j++) {
          double c1 = cos(q1[i]), s1 = sin(q1[i]);
          double c5 = cos(q5[i][j]), s5 = sin(q5[i][j]);
          double q6;
          ////////////////////////////// wrist 3 joint (q6) //////////////////////////////
          if(fabs(s5) < ZERO_THRESH)
            q6 = q6_des;
          else {
            q6 = atan2(SIGN(s5)*-(T01*s1 - T11*c1), 
                       SIGN(s5)*(T00*s1 - T10*c1));
            if(fabs(q6) < ZERO_THRESH)
              q6 = 0.0;
            if(q6 < 0.0)
              q6 += 2.0*PI;
          }
          ////////////////////////////////////////////////////////////////////////////////

          double q2[2], q3[2], q4[2];
          ///////////////////////////// RRR joints (q2,q3,q4) ////////////////////////////
          double c6 = cos(q6), s6 = sin(q6);
          double x04x = -s5*(T02*c1 + T12*s1) - c5*(s6*(T01*c1 + T11*s1) - c6*(T00*c1 + T10*s1));
          double x04y = c5*(T20*c6 - T21*s6) - T22*s5;
          double p13x = d5*(s6*(T00*c1 + T10*s1) + c6*(T01*c1 + T11*s1)) - d6*(T02*c1 + T12*s1) + 
                        T03*c1 + T13*s1;
          double p13y = T23 - d1 - d6*T22 + d5*(T21*c6 + T20*s6);

          double c3 = (p13x*p13x + p13y*p13y - a2*a2 - a3*a3) / (2.0*a2*a3);
          if(fabs(fabs(c3) - 1.0) < ZERO_THRESH)
            c3 = SIGN(c3);
          else if(fabs(c3) > 1.0) {
            // TODO NO SOLUTION
            continue;
          }
          double arccos = acos(c3);
          q3[0] = arccos;
          q3[1] = 2.0*PI - arccos;
          double denom = a2*a2 + a3*a3 + 2*a2*a3*c3;
          double s3 = sin(arccos);
          double A = (a2 + a3*c3), B = a3*s3;
          q2[0] = atan2((A*p13y - B*p13x) / denom, (A*p13x + B*p13y) / denom);
          q2[1] = atan2((A*p13y + B*p13x) / denom, (A*p13x - B*p13y) / denom);
          double c23_0 = cos(q2[0]+q3[0]);
          double s23_0 = sin(q2[0]+q3[0]);
          double c23_1 = cos(q2[1]+q3[1]);
          double s23_1 = sin(q2[1]+q3[1]);
          q4[0] = atan2(c23_0*x04y - s23_0*x04x, x04x*c23_0 + x04y*s23_0);
          q4[1] = atan2(c23_1*x04y - s23_1*x04x, x04x*c23_1 + x04y*s23_1);
          ////////////////////////////////////////////////////////////////////////////////
          for(int k=0;k<2;k++) {
            if(fabs(q2[k]) < ZERO_THRESH)
              q2[k] = 0.0;
            else if(q2[k] < 0.0) q2[k] += 2.0*PI;
            if(fabs(q4[k]) < ZERO_THRESH)
              q4[k] = 0.0;
            else if(q4[k] < 0.0) q4[k] += 2.0*PI;
            q_sols[num_sols*6+0] = q1[i];    q_sols[num_sols*6+1] = q2[k]; 
            q_sols[num_sols*6+2] = q3[k];    q_sols[num_sols*6+3] = q4[k]; 
            q_sols[num_sols*6+4] = q5[i][j]; q_sols[num_sols*6+5] = q6; 
            num_sols++;
          }

        }
      }
    }
    return num_sols;
  }




};

#endif
