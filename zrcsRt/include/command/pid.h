/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef  PID_H
#define PID_H

#include "system/base/basenodeInterface.h"
#include <cstdint>
#include <ruckig/ruckig.hpp>
#include <string>
#include <unistd.h>
#include "system/classfactory.h"
#include <random>
using namespace ruckig;
class PIDController {
private:
    double Kp, Ki, Kd;
    double prevError;
    double integral;

public:
    PIDController(double Kp, double Ki, double Kd) : Kp(Kp), Ki(Ki), Kd(Kd), prevError(0), integral(0) {}

    double calculate(double setpoint, double processVariable) 
    {
        double error = setpoint - processVariable;

        double p = Kp * error;
        integral += error;
        double i = Ki * integral;
        double d = Kd * (error - prevError);

        double output = p + i + d;

        prevError = error;

        return output;
    }
};
class Pid:public zrcsSystem::Basenode
  {

             public:
             Ruckig<1> otg {0.001}; 
             InputParameter<1> input;
             OutputParameter<1> output;            
             int motor_id;
             double velocity;
             double acceleration;
             double position;
             double jerk;
             PIDController pid;
              double processVariable=10;
            Pid():pid(0.5, 0.01, 0.1)
            {
              port_input.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 1000));
              port_input.add<double>("position", 'p', "servo position", false, 0, cmdline::range(-2000.000, 2000.000));
              port_input.add<double>("velocity", 'v', "servo velocity", false, 10, cmdline::range(-1000.0, 1000.0));
              port_input.add<double>("acceleration", 'a', "servo acceleration", false, 10, cmdline::range(-1000.0, 1000.0));
              port_input.add<double>("jerk", 'j', "servo jerk", false, 10, cmdline::range(-1000.0, 1000.0));
              
            }    
;
       void config() override
       {
           if (!cmdParam.empty()) 
              {
                  std::string str=cmdParam.front();
                  port_input.parse_check(str);
                  cmdParam.pop();
              }  
              position=port_input.get<double>("position"); 
              velocity=port_input.get<double>("velocity");
              acceleration= port_input.get<double>("acceleration");
              jerk=port_input.get<double>("jerk");
              motor_id=port_input.get<int>("motor");     
       }

       void init() override
       {    
           
              
              // gen(rd()); // 使用 Mersenne Twister 引擎
             // std::uniform_int_distribution<int> dis(1, 10000); // 生成 1 到 100 之间的随机整数
    // 生成随机数
        
             node_status=RUNNING;                  
    }
           
      void  excuteRt(void) override
      {      
                    static uint64_t t=0; 
            
                     double controlOutput = pid.calculate(199+sin(t), processVariable);
                     processVariable += controlOutput;
                     t=t+0.001;
                     rt_printf("---  %llf\n",controlOutput);  

                     
        
      }
      void exit(void) override
      {
             rt_printf("JogAbsj 执行成功\n");
             node_status=EXIT;
      }
     
  };

 REGISTERCMD(Pid);

#endif