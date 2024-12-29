/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef DATAPUB
#define DATAPUB
#include "system/basenodeInterface.h"
#include "system/centre.h"
#include "system/classfactory.h"
#include <iostream>
class DataPub:public zrcsSystem::Basenode
{
   private:
      int motor_id;
      
   public:
        DataPub()
        {
          
        }
      void config()override
      {
          
            
      }
         void init() override
         {                 
               node_status=RUNNING;                 
         }
        void  excuteRt(void) override
        {    
             
           for (int i=0; i<control->motors.size(); i++)
           {
          
              Motor m;              
              m.feedbackPosition=control->motors[0]->actualPos();
              m.setPosition=control->motors[0]->getTargetPos();
              if(motorFeedback.size()<10000) 
               {
                  motorFeedback.write(m);
               }
           }
                                     
                                
        }
      void exit(void) override
      {
             rt_printf("Flyingshot 执行成功\n");
             node_status=EXIT;
      }
     
};
REGISTERNODE(DataPub);
#endif