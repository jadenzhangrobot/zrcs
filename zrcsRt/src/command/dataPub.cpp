/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 数据发布节点
 */
#include "command/dataPub.h"

void DataPub::init()
{
}

void DataPub::run()
{
    for (int i = 0; i < controller_->axiss.size(); i++)
    {
        axisPosition_[i] = controller_->axiss[i]->actualposCmd();
    }
    shm().statusQueue().push(axisPosition_);
}

REGISTERINPUT(DataPub);
