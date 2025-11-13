/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef ENABLE_H_
#define ENABLE_H_
#include "common/config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <iostream>

class Enable:public zrcsSystem::CmdNode
{
   private:
      int axisId=0;
         std::strcpy(nodeName,"Enable");
      
   public:
        Enable()
        {
              
        }
    
       void init() override
       {     
          axisId=command->args[EnableAxisId];
       }

  
           
      void  run(void) override
      {                               
                     if(control->axiss.size()>axisId)            
                      {          
                                 control->axiss[axisId]->setAxisPositionCmd(control->axiss[axisId]->actualPos());            
                                 if(!control->axiss[axisId]->powerOn())
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
REGISTERCMD(Enable);
#endif