/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 数据发布节点
 */
#pragma once
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"
#include <array>

class DataPub : public zrcsSystem::InputNode
{
public:
    std::array<double, AXISMAXCOUNT> axisPosition_;

    void init() override
    {
    }
    
    void run() override
    {
        for (int i = 0; i < controller_->axiss.size(); i++)
        {
            axisPosition_[i] = controller_->axiss[i]->actualposCmd(); 
        }
        shm().statusQueue().push(axisPosition_);         
    }    
};

REGISTERINPUT(DataPub);
