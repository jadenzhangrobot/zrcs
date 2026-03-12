/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机失能指令
 */
#pragma once
#include <iostream>
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"

class Disable : public zrcsSystem::CmdNode
{
private:
    int axisId_;
      
public:
    Disable()
    {
        std::strcpy(nodeName_, "Disable");               
    }

    void init() override
    {     
        axisId_ = static_cast<int>(command_->args[DisableAxisId]);
    }

    void run(void) override
    {                               
        if(controller_->axiss.size() > axisId_)            
        {                        
            if(!controller_->axiss[axisId_]->powerOff())
            {
                setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            }
            else 
            {
                setCmdStatus(zrcsSystem::CmdStatus::EXIT);
            }
        }
        else if(controller_->axiss.size() == axisId_)
        {
            for (int i = 0; i < axisId_; i++) 
            {
                if(!controller_->axiss[i]->powerOff())
                {
                    setCmdStatus(zrcsSystem::CmdStatus::FAILED);
                }
            }
            setCmdStatus(zrcsSystem::CmdStatus::EXIT); 
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

REGISTERCMD(Disable);
