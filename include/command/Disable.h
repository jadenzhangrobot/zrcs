/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-29 18:37:34
 * @LastEditTime: 2023-06-06 14:24:28
 * @Description: 电机失能指令
 * 
 */

#ifndef DISABLE_H_
#define DISABLE_H_
#include "system/basefun.h"
#include "system/centre.h"
#include <iostream>
class Disable:basefun
{
   private:
      int motor_num;
      int motor_id;
   public:
        centre& cenobj=centre::getInstance();
        Disable()
        {         
            cmdline::parser cmd;
            cmd.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 100));         
            if (!cenobj.nrt_cmdParam.empty()) 
            {
                std::string str=cenobj.nrt_cmdParam.front();
                cmd.parse_check(str);
                cenobj.nrt_cmdParam.pop();
            }
            motor_num=cenobj.ec_control->motors.size();
            motor_id=cmd.get<int>("motor");
        }
        void  excute_rt(void) override
        {
             
            if(motor_id==motor_num)
            {
                for(int i=0;i<motor_num;i++)
                {
                    cenobj.ec_control->motors[i]->disable();
                }

            }
            else
            {  
                  cenobj.ec_control->motors[motor_id]->disable();                                       
            }
                   
             rt_flag=0;
        }


};

 REGISTER(Disable);
#endif