/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef DISABLE_H_
#define DISABLE_H_
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <iostream>
class Disable:public zrcsSystem::OneShotNode
{
   private:
      int axisId;
      
   public:
        Disable()
        {
           port_input.add<int>("axisId", 'm', "motor number", false, 0, cmdline::range(000, 100));                        
        }
    
         void init() override
       {     
             if(!cmdParam.empty())
             {
                   port_input.parse_check(cmdParam);
             }
            axisId=port_input.get<int>("axisId");
       }

  
           
      void  run(void) override
      {                               
                     if(control->axiss.size()>axisId)            
                      {                        
                                 if(!control->axiss[axisId]->powerOff())
                                 {
                                    SetOneShotStatus(zrcsSystem::OneShotNodeStatus::FAILED);
                                 }
                                else {
                                SetOneShotStatus(zrcsSystem::OneShotNodeStatus::EXIT);
                                }
                      }
                     else
                        {
                              SetOneShotStatus(zrcsSystem::OneShotNodeStatus::FAILED);
                        }                        
       }
                     
        
      
      void exit(void) override
      {
            
      }
};
REGISTERCMD(Disable);
#endif