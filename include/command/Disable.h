/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#pragma once
#include <iostream>
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"

class Disable:public zrcsSystem::CmdNode
{
   private:
      int axisId=0;
      
   public:
        Disable()
        {
                                
        }
    
         void init() override
       {     
         axisId=command->args[DisableAxisId];
       }

  
           
      void  run(void) override
      {                               
                     if(control->axiss.size()>axisId)            
                      {                        
                              if(!control->axiss[axisId]->powerOff())
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
REGISTERCMD(Disable);