/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef FLYINGSHOT_H
#define FLYINGSHOT_H
#include "system/basenodeInterface.h"
#include "system/centre.h"
#include "system/classfactory.h"
#include <iostream>
class Flyingshot:public zrcsSystem::Basenode
{
   private:
      int motor_id;
      
   public:
        Flyingshot()
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
            
                //int result=control->Ios[0]->Read("IoOutput2", 16, 8);
               //  if (result==1)
               //  {
               //      //rt_printf("11111\n");
               //  }
               //  else if(result==0)
               //  {
               //        node_status=FAILURE;                    
               //  }
               //  else {
                     
               //  }                      
        }
      void exit(void) override
      {
             rt_printf("Flyingshot 执行成功\n");
             node_status=EXIT;
      }
     
};
REGISTERNODE(Flyingshot);
#endif