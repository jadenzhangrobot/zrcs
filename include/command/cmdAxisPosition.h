/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#pragma once
#include "common/sharedMemory/sharedData.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <array>
#include <iostream>
class DataPub:public zrcsSystem::InputNode
{
   public:
        std::array<double, 100> AxisPositon;
        void execute() override
        {
            std::array<double, 100> AxisPositon;
            for(int i=0; i< control->axiss.size(); i++)
            {
                AxisPositon[i] = control->axiss[i]->actualPos();  
            }
               AxisPositon[1] = control->axiss[0]->actualposCmd(); 
            rtStatusQueue.push(AxisPositon);           
        }    
};
REGISTERINPUT(DataPub);
