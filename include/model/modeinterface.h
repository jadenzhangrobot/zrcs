#ifndef MODEINTERFACE_H_
#define MODEINTERFACE_H_
#include <ruckig/ruckig.hpp>
#include <common/config/parameter.h>
#include "controller/Controller.h"
#include <controller/ControllerInterface.h>

#include <vector>
#include <string>
#include <memory>
#include <bitset>
#include "common/config/parameter.h"

using namespace ruckig;
using namespace ZrcsHardware;
/**
 * @brief 模式接口基类，类似机床多通道概念
 * 每个模式可以管理多个轴的协调运动
 */
template<int AxisNum>    
class ModeInterface 
{
protected:  
    std::string modeName_;                    // 模式名称
    std::bitset<AxisNum> useAxisMask_;     // 使用轴的掩码，支持最多MaxAxisNum个轴
    std::vector<int> axisIds_;                // 参与运动的轴ID列表
    
    // 轨迹规划器
    Ruckig<AxisNum> otg_{cycletime * 0.001};
         // 最大加加速度
    
    // 控制器接口
    Controller* controller_;
    // 运动学模型
    
public:
    /**
     * @brief 构造函数
     * @param name 模式名
     * @param type 模式类型
     */
    ModeInterface(const std::string& name, Controller* controller) : modeName_(name), controller_(controller)
    {

    }  
    virtual ~ModeInterface() = default;
    
    virtual bool forwardKinematicsSolution(const std::vector<double>& PositionPosture, std::vector<double>& axisPosition)
    {
       
    }
    virtual bool inverseKinematicsSolution(const std::vector<double>& axisPosition, std::vector<double>& PositionPosture)
    {
       
    }

};

#endif
