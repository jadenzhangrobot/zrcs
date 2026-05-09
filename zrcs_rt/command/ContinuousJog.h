/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-08-03 06:50:30
 * @Description: 连续点动指令
 */
#pragma once

#include "system/node/BaseNodeInterface.h"
#include <array>
#include <ruckig/ruckig.hpp>
#include <vector>
#include "system/node/NodeFactory.h"

using namespace ruckig;

class ContinuousJog : public zrcsSystem::OutputNode
{
private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;            
    double targetVelocity_;
    double setCurrentPosition_; 
    double lastVelocity_;
    double lastAcceleration_;
    bool accelerateStart_;
    bool decelerateStart_;
    bool stopped_;          // 减速完成后进入停止状态，不再写位置指令
    
public:
    ContinuousJog() : otg_(cycletime * 0.001),
                      targetVelocity_(0),
                      setCurrentPosition_(0),
                      lastVelocity_(0),
                      lastAcceleration_(0),
                      accelerateStart_(true),
                      decelerateStart_(true),
                      stopped_(true)
    {
    }

    void init() override;
    void accelerate(int axisId);
    void decelerate(int axisId);
    void run() override;
};

