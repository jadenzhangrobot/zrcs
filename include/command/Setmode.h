/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-29 13:54:20
 * @LastEditTime: 2023-04-20 15:48:24
 * @Description: 设置电机模式的指令
 * 
 */
#ifndef MODE_H_
#define MODE_H_
#include "system/basefun.h"
#include "system/centre.h"
#include <iostream>
class Setmode:basefun
{
   private:
      int motor_num;
      int motor_id;
      int motor_mode;
   public:
        centre& cenobj=centre::getInstance();
        Setmode()
        {         
            cmdline::parser cmd;
            cmd.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 100)); 
            cmd.add<int>("mode", 'o', "motor mode", false, 4, cmdline::range(0, 10));        
            if (!cenobj.nrt_cmdParam.empty()) 
            {
                std::string str=cenobj.nrt_cmdParam.front();  
                cmd.parse_check(str);
                cenobj.nrt_cmdParam.pop();
            }
            motor_num=cenobj.ec_control->motors.size();    
            motor_id=cmd.get<int>("motor");
            motor_mode=cmd.get<int>("mode");
        }
        void  excute_rt(void) override
        {            
            if(motor_id==motor_num)
            {
                for(int i=0;i<motor_num;i++)
                {
                  cenobj.ec_control->motors[i]->mode(motor_mode);
                }
            }  
            else
            {      
                 cenobj.ec_control->motors[motor_id]->mode(motor_mode);             
            }                  
             rt_flag=0;
        }


};
 REGISTER(Setmode);
#endif