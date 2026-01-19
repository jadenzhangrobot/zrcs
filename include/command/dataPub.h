/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#pragma once
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <array>
#include <iostream>
class DataPub:public zrcsSystem::InputNode
{
   public:
        std::array<double, AXISMAXCOUNT> AxisPositon;

        void init() override
        {
           
        }
        void run() override
        {
          for (int i=0; i< control->axiss.size(); i++)
          {
             AxisPositon[i] = control->axiss[i]->actualposCmd(); 
          }
           rtStatusQueue.push(AxisPositon);         
        }    
};
REGISTERINPUT(DataPub);
