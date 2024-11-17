/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef ENABLE_H_
#define ENABLE_H_
#include "system/basenodeInterface.h"
#include "system/centre.h"
#include "system/classfactory.h"
#include <iostream>
class Enable:public zrcsSystem::Basenode
{
   private:
      int motor_id;
      
   public:
        Enable()
        {
           port_input.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 100));                        
        }
      void config()override
      {
          
          if (!cmdParam.empty()) 
          {
              std::string str=cmdParam.front();
              port_input.parse_check(str);
              cmdParam.pop();            
          } 
           motor_id=port_input.get<int>("motor");      
      }
         void init() override
         {                 
               node_status=RUNNING;                 
         }
        void  excuteRt(void) override
        {    
          static int SleepCount=0;            
		    if (SleepCount>50)
		    {
                  if(control->motors[motor_id]->enable()==5)
                  {                      
                        node_status=SUCCESS;
                  } 
                  else if (control->motors[motor_id]->enable()<0) 
                  {
                        node_status=FAILURE;
                  } 
                  SleepCount=0;
          }            
                SleepCount++;                                 
        }
      void exit(void) override
      {
             rt_printf("Enable 执行成功\n");
             node_status=EXIT;
      }
     
};
REGISTERCMD(Enable);
#endif