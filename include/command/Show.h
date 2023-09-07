/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-04-07 14:18:25
 * @LastEditTime: 2023-04-24 14:38:54
 * @Description: 显示信息指令
 * 
 */

#ifndef SHOW_H
#define SHOW_H
#include "system/basenode.h"
#include "system/centre.h"
#include <iostream>
class Show:public zrcs_system::Basenode
{    

         zrcs_system::centre& cenobj=zrcs_system::centre::getInstance();
       bool init() override
       {                 
            return true;
       }
  
      void  excute_rt(void) override
      {      
            for (int i =0;i<6;i++) {
           
            std::cout<<"motorid"<<"  "<<i<<"    "<<cenobj.ec_control->motors[i]->actualPos()<<std::endl;
            }
            
                         
            rtnode_status=SUCCESS;                                               
      }







};
REGISTER(Show);
#endif