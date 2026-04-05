/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#ifndef ENABLE_H_
#define ENABLE_H_
#include "config/cmdArgs.h"
#include "sharedMemory/sharedData.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"

class Enable : public zrcsSystem::CmdNode
{
private:
    int axisId_;

public:
    Enable()
    {
        std::strcpy(nodeName_, "Enable");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

#endif
