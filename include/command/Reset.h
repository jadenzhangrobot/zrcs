/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef RESET_H_
#define RESET_H_
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <iostream>
class Reset:public zrcsSystem::CmdNode
{
   private:
      int axisId;
      
   public:
        Reset()
        {
           port_input.add<int>("axis", 'm', "motor number", false, 0, cmdline::range(000, 100));                        
        }
    
         void init() override
       {     
             if(!cmdParam.empty())
             {
                   port_input.parse_check(cmdParam);
             }
            axisId=port_input.get<int>("axis");
       }

  
           
      void  run(void) override
      {                               
                     if(control->axiss.size()>axisId)            
                      {                        
                                 if(!control->axiss[axisId]->resetError())
                                 {
                                    setCmdStatus(zrcsSystem::CmdStatus::FAILED);
                                 }
                                 else 
                                 {
                                  setCmdStatus(zrcsSystem::CmdStatus::EXIT);
                                 }
                      }
                     else
                        {
                              setCmdStatus(zrcsSystem::CmdStatus::FAILED);
                        }                        
       }
                     
        
      
      void exit(void) override
      {
            
      }
};
REGISTERCMD(Reset);
#endif