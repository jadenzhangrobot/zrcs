/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 关节运动绝对位置指令
 */
#ifndef JOGABSJ_H
#define JOGABSJ_H

#include "config/cmdArgs.h"
#include "sharedMemory/sharedData.h"
#include "system/basenodeInterface.h"
#include <array>
#include <ruckig/ruckig.hpp>
#include <vector>
#include "system/nodeFactory.h"

using namespace ruckig;

class JogabsJ : public zrcsSystem::CmdNode
{
private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;            
    int axisId_;
    double position_;
    
public:
    JogabsJ() : otg_(cycletime * 0.001)
    {
        std::strcpy(nodeName_, "JogabsJ");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

#endif
