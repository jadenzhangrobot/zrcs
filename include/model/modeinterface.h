#ifndef MODEINTERFACE_H_
#define MODEINTERFACE_H_
#include <ruckig/ruckig.hpp>
#include <common/config/parameter.h>
#include "controller/Controller.h"
#include <controller/ControllerInterface.h>
#include "model/KinematicsModel.h"
#include <vector>
#include <string>
#include <memory>
#include <bitset>
#include <algorithm>

using namespace ruckig;
using namespace ZrcsHardware;
/**
 * @brief 模式接口基类，类似机床多通道概念
 * 每个模式可以管理多个轴的协调运动
 */
template<int MaxAxisNum>
class ModeInterface 
{
protected:
    std::string modeName_;                    // 模式名称
    std::bitset<MaxAxisNum> useAxisMask_;     // 使用轴的掩码，支持最多MaxAxisNum个轴
    std::vector<int> axisIds_;                // 参与运动的轴ID列表
    
    // 轨迹规划器
    std::unique_ptr<Ruckig<MaxAxisNum>> otg_;
    InputParameter<MaxAxisNum> input_;
    OutputParameter<MaxAxisNum> output_;
         // 最大加加速度
    
    // 控制器接口
    Controller* controller_;
    // 运动学模型
    std::shared_ptr<KinematicsModel> kinematicsModel_;
public:
    /**
     * @brief 构造函数
     * @param name 模式名称
     * @param type 模式类型
     */
    ModeInterface(const std::string& name = "DefaultMode", ModeType type = ModeType::MULTI_AXIS)
        : modeName_(name), modeType_(type), modeStatus_(ModeStatus::IDLE), 
          activeAxisNum_(0), controller_(nullptr), kinematicsModel_(nullptr)
    {
        otg_ = std::make_unique<Ruckig<MaxAxisNum>>(cycletime * 0.001);
        
    
    }
    
    virtual ~ModeInterface() = default;

};

#endif
