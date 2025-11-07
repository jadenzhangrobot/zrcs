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
#include "system/nodeFactory.h"
#include <iostream>
class DataPub:public zrcsSystem::Basenode
{
   private:
      int motor_id;
      
   public:
        DataPub()
        {
          
        }
        void{
         
        }
     
};
REGISTERNODE(DataPub);
#endif