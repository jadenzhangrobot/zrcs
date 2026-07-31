/*
 * @Description: 笛卡尔直线运动（ABB MoveL）�?1D 弧长参数�?+ 每周�?IK
 *               复用 Ruckig 实例，跨段速度/加速度天然连续
 */
#include "command/MoveL.h"
#include "shared_memory/ShmLayout.h"

#include <algorithm>
#include <cmath>

MoveL::MoveL() : cartDist_(0)
{
    std::strcpy(nodeName_, "MoveL");
    // 构造函数在静态初始化阶段（NRT）执行，提前预分配缓冲区
    axisIds_.reserve(zrcs::kAxisMax);
    if (!otg_)
    {
        otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
        input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
        output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
    }
}

bool MoveL::initTrajectory()
{
    // 首次进入时从模型缓存读取轴映射（零分配：RobotModel 已预缓存，axisIds_ 已预分配）
    if (!modelInited_)
    {
        auto* registry = modelRegistry_;
        if (!registry)
        {
            ERROR_PRINT("MoveL: 模型注册表未初始化\n");
            return false;
        }
        model_ = registry->getModel(0);
        if (!model_)
        {
            ERROR_PRINT("MoveL: 未找到模型(id=0)\n");
            return false;
        }
        dof_     = model_->getDof();
        axisIds_ = model_->getAxisIds();  // const ref → copy into pre-reserved buffer, zero realloc
        currentJoint_.resize(dof_);
        modelInited_ = true;
    }

    // Sync=1 时重置弧长参数空间，开始新的一组
    if (command_->args[static_cast<size_t>(MoveLArg::Sync)] == 1.0)
    {
        arcOffset_ = 0.0;
        input_->current_position[0] = 0.0;
        input_->current_velocity[0] = 0.0;
        input_->current_acceleration[0] = 0.0;
    }

    // 构建目标位姿
    targetPos_ = Eigen::Vector3d(
        command_->args[static_cast<size_t>(MoveLArg::X)],
        command_->args[static_cast<size_t>(MoveLArg::Y)],
        command_->args[static_cast<size_t>(MoveLArg::Z)]);
    targetQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MoveLArg::Q1)],  // w
        command_->args[static_cast<size_t>(MoveLArg::Q2)],  // x
        command_->args[static_cast<size_t>(MoveLArg::Q3)],  // y
        command_->args[static_cast<size_t>(MoveLArg::Q4)]); // z
    if (!targetPos_.allFinite() || !targetQuat_.coeffs().allFinite() ||
        targetQuat_.norm() < 1e-9)
    {
        ERROR_PRINT("MoveL: 目标位姿包含非法数值或零四元数\n");
        return false;
    }
    targetQuat_.normalize();

    // 起点位姿: 优先从模型 FK 计算当前关节角 → 避免 Current* 手填不一致
    if (model_)
    {
        Eigen::VectorXd q(dof_);
        for (int i = 0; i < dof_; i++)
            q(i) = controller_->axes_[axisIds_[i]]->actualPos();
        Eigen::Matrix4d T_start;
        if (model_->forwardKinematics(q, T_start))
        {
            startPos_  = T_start.block<3,1>(0,3);
            startQuat_ = Eigen::Quaterniond(T_start.block<3,3>(0,0));
            startQuat_.normalize();
        }
        else
        {
            ERROR_PRINT("MoveL: FK 计算起点失败\n");
            return false;
        }
    }
    else
    {
        // Fallback: 无模型时从 Current* 参数读取（笛卡尔轴直驱场景）
        startPos_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MoveLArg::CurrentX)],
            command_->args[static_cast<size_t>(MoveLArg::CurrentY)],
            command_->args[static_cast<size_t>(MoveLArg::CurrentZ)]);
        startQuat_ = Eigen::Quaterniond(
            command_->args[static_cast<size_t>(MoveLArg::CurrentQ1)],
            command_->args[static_cast<size_t>(MoveLArg::CurrentQ2)],
            command_->args[static_cast<size_t>(MoveLArg::CurrentQ3)],
            command_->args[static_cast<size_t>(MoveLArg::CurrentQ4)]);
    }

    // 计算线段长度
    cartDist_ = (targetPos_ - startPos_).norm();
    if (cartDist_ < 1e-6)
    {
        ERROR_PRINT("MoveL: 线段长度为零\n");
        return false;
    }

    // 读取笛卡尔标量边界条件
    double maxVel = command_->args[static_cast<size_t>(MoveLArg::Vel)];
    double tgtVel = command_->args[static_cast<size_t>(MoveLArg::TargetVel)];

    // 读取标量路径加速度/jerk 限制（复用 PathMove 配置）
    double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    double maxJerk  = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);
     
    arcOffset_=arcOffset_+cartDist_;
    // 目标和限制使用累积弧长
    input_->target_position[0]      = arcOffset_;
    input_->target_velocity[0]      = tgtVel;
    input_->max_velocity[0]         = maxVel;
    input_->max_acceleration[0]     = maxAccel;
    input_->max_jerk[0]             = maxJerk;

    return true;
}


bool MoveL::applyOutput()
{
    // otg_->update() 已由 TrajectoryCmd::runStandard()->updateTrajectory() 完成，
    // output_ 已包含当前周期的轨迹输出，此处不再重复调用 update/pass_to_input。

    double s = output_->new_position[0];

    // 线性插值得到当前笛卡尔位姿
    const double rawU = (s - (arcOffset_ - cartDist_)) / cartDist_;
    const double u = std::clamp(rawU, 0.0, 1.0);

    Eigen::Vector3d pos = startPos_ + u * (targetPos_ - startPos_);

    // 四元数球面线性插补 (SLERP) — 含最短路径 + 小角度保护
    Eigen::Quaterniond qInterp = startQuat_.slerp(u, targetQuat_);

    // 构建 4x4 目标位姿
    Eigen::Matrix4d T_target = Eigen::Matrix4d::Identity();
    T_target.block<3,3>(0,0) = qInterp.toRotationMatrix();
    T_target.block<3,1>(0,3) = pos;

    // 获取当前关节位置作为 IK 初值
    for (int i = 0; i < dof_; i++)
    {
        currentJoint_(i) = controller_->axes_[axisIds_[i]]->actualPos();
    }

    // 轨迹首拍 u≈0 时目标就是当前 FK 起点。此时直接保持当前关节角，
    // 避免在贴边/奇异/传感器微偏情况下因“重解当前位姿”失败，
    // 把整段 MoveL 在第一步就打断。
    constexpr double kHoldJointProgress = 1e-9;
    if (u <= kHoldJointProgress)
    {
        for (int i = 0; i < dof_; i++)
            controller_->axes_[axisIds_[i]]->setAxisPositionCmd(currentJoint_(i));
        return true;
    }

    // 多态 IK：根据 model.xml 配置的模型类型自动选择对应的 inverseKinematics 实现
    //   "serial"      → SerialRobot::inverseKinematics      (DLS 数值迭代)
    //   "openarm_srs" → OpenArmRobot::inverseKinematics     (Singh-Kreutz 解析解)
    //   "cartesian"   → CartesianRobot::inverseKinematics   (1:1 映射)
    Eigen::VectorXd targetJoint(dof_);
    if (!model_->inverseKinematics(T_target, currentJoint_, targetJoint))
    {
        ERROR_PRINT("MoveL: IK 求解失败 pos=(%.4f,%.4f,%.4f) "
                    "start=(%.4f,%.4f,%.4f) target=(%.4f,%.4f,%.4f) u=%.4f\n",
                    pos.x(), pos.y(), pos.z(),
                    startPos_.x(), startPos_.y(), startPos_.z(),
                    targetPos_.x(), targetPos_.y(), targetPos_.z(), u);
        return false;
    }

    // 将解算出的关节角写入各轴
    for (int i = 0; i < dof_; i++)
    {
        controller_->axes_[axisIds_[i]]->setAxisPositionCmd(targetJoint(i));
    }

    return true;
} 


CMD_REGISTER(MoveL);
