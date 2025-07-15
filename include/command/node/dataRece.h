/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef DATARCE
#define DATARCE
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <iostream>
class DataRece:public zrcsSystem::PersistentNode
{
   private:
      int motor_id;
      
   public:
        DataRece()
        {
          
        }
     
         void init() override
         {                 
               std::cout<<"DataRece init"<<std::endl;
         }
        void  run(void) override
        {    
              std::cout<<"DataRECE run"<<std::endl;            
        }
      void exit(void) override
      {
             //rt_printf("Flyingshot 执行成功\n");
            // node_status=EXIT;
      }
     
};
REGISTERNODE(DataRece);
#endif