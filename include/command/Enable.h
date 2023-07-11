/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef ENABLE_H_
#define ENABLE_H_
#include "system/basefun.h"
#include "system/centre.h"
#include <iostream>
class Enable:basefun
{
   private:
      int motor_num;
      int motor_id;
   public:
        centre& cenobj=centre::getInstance();
        Enable()
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
                    cenobj.ec_control->motors[i]->enable();
                }
            }
            else
            {  
                  cenobj.ec_control->motors[motor_id]->enable();                      
            }                   
                        rt_flag=2;
                        cmd_frame cf;
                        strcpy(cf.type,"Enable");
                        cf.status=2;
                        strcpy(cf.error,"success");
                       // LOGGER_INFO("Sfork finished");
                        std::string str="MotionCtrlMsgCmd";
                        cenobj.zmq_cmd.pub(str.c_str(),str.length(),ZMQ_SNDMORE);
                        cenobj.zmq_cmd.pub(&cf,sizeof(cf),0);
        }


};
 REGISTER(Enable);
 
#endif