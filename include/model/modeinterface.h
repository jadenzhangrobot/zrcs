#ifndef MODEINTERFACE_H_
#define MODEINTERFACE_H_
#include <ruckig/ruckig.hpp>
#include <common/config/parameter.h>
using namespace ruckig;
#include <controller/ControllerInterface.h>
#include "model/KinematicsModel.h"
#include <vector>
#include <string>
#include <memory>
#include <bitset>
#include <algorithm>


using namespace ZrcsHardware;

/**
 * @brief 运动模式状态枚举
 */
enum class ModeStatus {
    IDLE = 0,        // 空闲状态
    RUNNING = 1,     // 运行状态
    PAUSED = 2,      // 暂停状态
    ERROR = 3,       // 错误状态
    COMPLETED = 4    // 完成状态
};

/**
 * @brief 运动模式类型枚举
 */
enum class ModeType {
    SINGLE_AXIS = 0,     // 单轴模式
    MULTI_AXIS = 1,      // 多轴模式
    INTERPOLATION = 2,   // 插补模式
    SYNCHRONOUS = 3      // 同步模式
};

/**
 * @brief 模式接口基类，类似机床多通道概念
 * 每个模式可以管理多个轴的协调运动
 */
template<int MaxAxisNum = 6>
class ModeInterface
{
protected:
    std::string modeName_;                    // 模式名称
    ModeType modeType_;                       // 模式类型
    ModeStatus modeStatus_;                   // 模式状态
    int activeAxisNum_;                       // 激活的轴数量
    std::bitset<MaxAxisNum> useAxisMask_;     // 使用轴的掩码，支持最多MaxAxisNum个轴
    std::vector<int> axisIds_;                // 参与运动的轴ID列表
    
    // 轨迹规划器
    std::unique_ptr<Ruckig<MaxAxisNum>> otg_;
    InputParameter<MaxAxisNum> input_;
    OutputParameter<MaxAxisNum> output_;
    
    // 运动参数
    std::vector<double> currentPosition_;     // 当前位置
    std::vector<double> currentVelocity_;     // 当前速度
    std::vector<double> currentAcceleration_; // 当前加速度
    std::vector<double> targetPosition_;      // 目标位置
    std::vector<double> targetVelocity_;      // 目标速度
    std::vector<double> maxVelocity_;         // 最大速度
    std::vector<double> maxAcceleration_;     // 最大加速度
    std::vector<double> maxJerk_;             // 最大加加速度
    
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
        
        // 初始化向量大小
        currentPosition_.resize(MaxAxisNum, 0.0);
        currentVelocity_.resize(MaxAxisNum, 0.0);
        currentAcceleration_.resize(MaxAxisNum, 0.0);
        targetPosition_.resize(MaxAxisNum, 0.0);
        targetVelocity_.resize(MaxAxisNum, 0.0);
        maxVelocity_.resize(MaxAxisNum, 1.0);
        maxAcceleration_.resize(MaxAxisNum, 1.0);
        maxJerk_.resize(MaxAxisNum, 1.0);
    }
    
    virtual ~ModeInterface() = default;
    
    /**
     * @brief 设置控制器
     * @param controller 控制器指针
     */
    void setController(Controller* controller) {
        controller_ = controller;
    }

    /**
     * @brief 设置运动学模型
     * @param model 运动学模型指针
     */
    void setKinematicsModel(std::shared_ptr<KinematicsModel> model) {
        kinematicsModel_ = model;
    }

    /**
     * @brief 添加一个轴到当前模式
     * @param axis_id 轴ID
     * @return true 添加成功
     * @return false 添加失败
     */
    bool addAxis(int axis_id)
    {
        if (activeAxisNum_ < MaxAxisNum && !useAxisMask_[axis_id])
        {
            useAxisMask_[axis_id] = 1;
            axisIds_.push_back(axis_id);
            activeAxisNum_++;
            return true;
        }
        return false;
    }

    /**
     * @brief 移除一个轴
     * @param axis_id 轴ID
     */
    void removeAxis(int axis_id)
    {
        if (useAxisMask_[axis_id])
        {
            useAxisMask_[axis_id] = 0;
            axisIds_.erase(std::remove(axisIds_.begin(), axisIds_.end(), axis_id), axisIds_.end());
            activeAxisNum_--;
        }
    }

    /**
     * @brief 设置轴的运动限制
     * @param axis_id 轴ID
     * @param max_vel 最大速度
     * @param max_acc 最大加速度
     * @param max_jerk 最大加加速度
     */
    void setAxisLimits(int axis_id, double max_vel, double max_acc, double max_jerk)
    {
        if (axis_id < MaxAxisNum)
        {
            maxVelocity_[axis_id] = max_vel;
            maxAcceleration_[axis_id] = max_acc;
            maxJerk_[axis_id] = max_jerk;
        }
    }

    /**
     * @brief 初始化，准备运动
     * @return true 初始化成功
     * @return false 初始化失败
     */
    virtual bool initialize()
    {
        if (!controller_ || axisIds_.empty())
        {
            return false;
        }
        updateCurrentState();
        for (size_t i = 0; i < axisIds_.size(); ++i)
        {
            int axis_id = axisIds_[i];
            input_.current_position[i] = currentPosition_[axis_id];
            input_.current_velocity[i] = currentVelocity_[axis_id];
            input_.current_acceleration[i] = currentAcceleration_[axis_id];

            input_.max_velocity[i] = maxVelocity_[axis_id];
            input_.max_acceleration[i] = maxAcceleration_[axis_id];
            input_.max_jerk[i] = maxJerk_[axis_id];
        }
        input_.control_interface = ControlInterface::Position;
        return true;
    }

    /**
     * @brief 设置目标位置
     * @param positions 目标位置（关节空间或笛卡尔空间）
     * @param is_cartesian 是否为笛卡尔空间坐标
     * @return true 设置成功
     * @return false 设置失败
     */
    virtual bool setTargetPositions(const std::vector<double>& positions, bool is_cartesian = false)
    {
        std::vector<double> joint_positions;
        if (is_cartesian)
        {
            if (!kinematicsModel_ || !kinematicsModel_->inverseKinematics(positions, joint_positions))
            {
                return false; // 逆运动学计算失败
            }
        }
        else
        {
            joint_positions = positions;
        }

        if (joint_positions.size() != activeAxisNum_)
        {
            return false;
        }

        for (size_t i = 0; i < activeAxisNum_; ++i)
        {
            input_.target_position[i] = joint_positions[i];
            input_.target_velocity[i] = 0.0;
        }
        return true;
    }

    /**
     * @brief 开始运动
     */
    virtual void start()
    {
        if (modeStatus_ != ModeStatus::RUNNING)
        {
            modeStatus_ = ModeStatus::RUNNING;
        }
    }

    /**
     * @brief 暂停运动
     */
    virtual void pause()
    {
        modeStatus_ = ModeStatus::PAUSED;
    }

    /**
     * @brief 停止运动
     */
    virtual void stop()
    {
        modeStatus_ = ModeStatus::IDLE;
    }

    /**
     * @brief 周期性更新函数
     */
    virtual void update()
    {
        if (modeStatus_ != ModeStatus::RUNNING)
        {
            return;
        }

        updateCurrentState();

        Result result = otg_->update(input_, output_);
        if (result == Result::Working)
        {
            for (size_t i = 0; i < activeAxisNum_; ++i)
            {
                int axis_id = axisIds_[i];
                controller_->axiss[axis_id]->setPos(output_.new_position[i]);
            }
            // 更新下一次迭代的输入
            input_.current_position = output_.new_position;
            input_.current_velocity = output_.new_velocity;
            input_.current_acceleration = output_.new_acceleration;
        }
        else if (result == Result::Finished)
        {
            modeStatus_ = ModeStatus::COMPLETED;
        }
    }

protected:
    /**
     * @brief 更新当前状态（位置、速度、加速度）
     */
    void updateCurrentState()
    {
        if (!controller_)
        {
            return;
        }
        for (size_t i = 0; i < axisIds_.size(); ++i)
        {
            int axis_id = axisIds_[i];
            currentPosition_[axis_id] = controller_->axiss[axis_id]->pos();
            currentVelocity_[axis_id] = controller_->axiss[axis_id]->vel();
            currentAcceleration_[axis_id] = controller_->axiss[axis_id]->acc();
        }
    }

};

#endif
