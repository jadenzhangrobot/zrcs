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
#include "system/basenodeInterface.h"
#include "system/centre.h"
#include <iostream>
class Disable:zrcs_system::Basenode
{
   private:
      int motor_id;
   public:
        Disable()
        {
         
    
        }
         void init() override
       {   
           port_input.add<int>("motor", 'm', "motor number", false, 0, cmdline::range(000, 100));
        
          if (!cmdParam->empty()) 
          {
              std::string str=cmdParam->front();
              port_input.parse_check(str);
              cmdParam->pop();
          }   
                    
              motor_id=port_input.get<int>("motor");
              node_status=RUNNING;                 
          }


        void  excuteRt(void) override
        {            
           
                  if(control->motors[motor_id]->disable()==0)
                  {
                      
                     node_status=SUCCESS;
                   } 
                  else {
                      node_status=FAILURE;
                   }                     
                                                  
        }
      void exit(void) override
      {
           //std::cout<<"Disable 执行成功"<<std::endl;
      }



};

 REGISTERCMD(Disable);
#endif