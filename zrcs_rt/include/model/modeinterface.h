/**
 * @brief 模式接口基类，类似机床多通道概念
 * 每个模式可以管理多个轴的协调运动
 *
 * 改造要点:
 * 1. 去掉模板参数 <int AxisNum> → 运行时 DOF (Ruckig DynamicDOFs)
 * 2. 持有 RobotModel* (可为 nullptr，表示不需要运动学)
 * 3. 提供 FK/IK 委托方法
 */
#ifndef MODEINTERFACE_H_
#define MODEINTERFACE_H_

#include <ruckig/ruckig.hpp>
#include "controller/Controller.h"
#include <controller/ControllerInterface.h>
#include "model/robotModel.h"

#include <vector>
#include <string>
#include <memory>
#include "config/Parameter.h"
#include "shared_memory/SharedData.h"

using namespace ruckig;
using namespace ZrcsHardware;

class ModeInterface
{
protected:
    std::string modeName_;
    std::vector<int> axisIds_;        // 参与运动的轴ID列表
    int dof_;

    // 轨迹规划器 (运行时DOF)
    Ruckig<DynamicDOFs> otg_;
    InputParameter<DynamicDOFs> input_;
    OutputParameter<DynamicDOFs> output_;

    // 控制器接口
    Controller* controller_;

    // 运动学模型 (可为 nullptr)
    RobotModel* model_;

public:
    /**
     * @brief 构造函数
     * @param name 模式名
     * @param controller 控制器指针
     * @param model 运动学模型指针 (可为 nullptr)
     * @param dof 自由度数
     */
    ModeInterface(const std::string& name, Controller* controller,
                  RobotModel* model, int dof)
        : modeName_(name),
          controller_(controller),
          model_(model),
          dof_(dof),
          otg_(dof, cycletime * 0.001),
          input_(dof),
          output_(dof)
    {
        if (model_)
        {
            axisIds_ = model_->getAxisIds();
        }
    }

    virtual ~ModeInterface() = default;

    /**
     * @brief 正运动学 (委托给模型)
     */
    bool forwardKinematics(const Eigen::VectorXd& jointPos,
                            Eigen::Matrix4d& toolPose) const;

    /**
     * @brief 逆运动学 (委托给模型)
     */
    bool inverseKinematics(const Eigen::Matrix4d& toolPose,
                            const Eigen::VectorXd& currentJointPos,
                            Eigen::VectorXd& targetJointPos) const;

    /**
     * @brief Jacobian (委托给模型)
     */
    bool jacobian(const Eigen::VectorXd& jointPos,
                   Eigen::MatrixXd& J) const;

    int getDof() const { return dof_; }
    const std::string& getModeName() const { return modeName_; }
    const std::vector<int>& getAxisIds() const { return axisIds_; }
    RobotModel* getModel() const { return model_; }
    Controller* getController() const { return controller_; }
};

#endif // MODEINTERFACE_H_
