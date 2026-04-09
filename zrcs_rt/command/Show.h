/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-04-07 14:18:25
 * @LastEditTime: 2023-04-24 14:38:54
 * @Description: 显示信息指令
 * 
 */

#pragma once

#include "system/base/BaseNodeInterface.h"
#include "system/centre.h"
#include <iostream>
class Show:public zrcs_system::Basenode
{    
        int MotorId;
        public:
        Show(const std::string& node_name="Show")
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
                    
              MotorId=port_input.get<int>("motor");
              node_status=RUNNING;                 
          }


        void  excute_rt(void) override
        {            
                rt_printf("position---  %lf\n",Control->motors[MotorId]->actualPos());
                node_status=SUCCESS;                        
        }
      void exit(void) override
      {
           std::cout<<"Show 执行成功"<<std::endl;
      }








};
REGISTER(Show);
