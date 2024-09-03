/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef ENABLE_H_
#define ENABLE_H_
#include "system/basenode.h"
#include "system/centre.h"
#include <iostream>
class Enable:zrcs_system::Basenode
{
   private:
      int motor_id;
   public:
        Enable(const std::string& node_name="Enable")
        {
         

    
        }
      void init() override
       {   
           port_input.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 100));
        
          if (!CmdParam->empty()) 
          {
              std::string str=CmdParam->front();
              port_input.parse_check(str);
              CmdParam->pop();
          }   
                    
              motor_id=port_input.get<int>("motor");
              node_status=RUNNING;                 
          }


        void  excute_rt(void) override
        {            
                static int SleepCount=0;
             
			          if (SleepCount>50)
			          {
                  if(Control->motors[motor_id]->enable()==5)
                  {                      
                     node_status=SUCCESS;
                   } 
                  else if (Control->motors[motor_id]->enable()<0) {
                        node_status=FAILURE;
                  } 
                  SleepCount=0;
                }            
                SleepCount++;                                 
        }
      void exit(void) override
      {
           std::cout<<"Enable 执行成功"<<std::endl;
      }



};
 REGISTER(Enable);
 
#endif