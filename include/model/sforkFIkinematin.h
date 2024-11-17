#ifndef SFORKFIKNEMATIN_H
#define SFORKFIKNEMATIN_H
#include <cmath>
#include <memory>
   struct angle
    {
          double first_angle;
          double second_angle;
    };
    struct position
    {
          double angle;
          double len;
    };
 class sforkFIkinematin
 {
    public:
     double L=0.190;
     double M=0.185;
   
    sforkFIkinematin ()
    {


    }
    struct position Forward_kinematics(double first_angle,double second_angle)
    {
            position result  ;
            double a=(first_angle+second_angle+3.14)/2;
            double b=(first_angle-second_angle+2.46)/2;
            double L1=L*std::cos(b);
            double L2=L*std::sin(b);
            double L=sqrt(std::pow(M,2)-std::pow(L2,2));
            double len=4*L1+8*L;
            result.angle=b;
            result.len=len;
            return result;

    }
     struct angle Inverse_kinematics(double angle,double len)
    {

        double radian =angle*3.1415926/180;

            double Y=len*std::cos(radian );
            double X=len*std::sin(radian );

            
             double L4=std::pow(L,4);
             double L2=std::pow(L,2);
             double M2=std::pow(M,2);
    
             double X2=std::pow(X,2);
             double Y2=std::pow(Y,2);

             
           double b=std::acos((4*sqrt(60*L4-60*L2*M2+L2*X2+L2*Y2)-L*sqrt(X2+Y2))/(30*L2));

           double a=std::atan2(Y, X);
        
            struct angle result  ;
           //std::shared_ptr<double [2]> result(new double[2]);
           result.first_angle=a+b-2.8;
           result.second_angle=a-b-0.34;
          return result;
    }


 };

#endif