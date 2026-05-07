/*
 * @Description: 笛卡尔直线运动（ABB MoveL）�?1D 弧长参数�?+ 每周�?IK
 *               复用 Ruckig 实例，跨段速度/加速度天然连续
 */
#include "command/MoveL.h"
#include "shared_memory/ShmLayout.h"

MoveL::MoveL() : cartDist_(0), firstSegment_(true)
{
    std::strcpy(nodeName_, "MoveL");
    // 构造函数在静态初始化阶段（NRT）执行，提前预分配缓冲区
    axisIds_.reserve(zrcs::kAxisMax);
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
        RobotModel* model = registry->getModel(0);
        if (!model)
        {
            ERROR_PRINT("MoveL: 未找到模型(id=0)\n");
            return false;
        }
        axisIds_ = model->getAxisIds();  // const ref → copy into pre-reserved buffer, zero realloc
        modelInited_ = true;
    }

    // 从命令参数构建起点和终点位姿
    startPos_ = Eigen::Vector3d(
        command_->args[static_cast<size_t>(MoveLArg::CurrentX)],
        command_->args[static_cast<size_t>(MoveLArg::CurrentY)],
        command_->args[static_cast<size_t>(MoveLArg::CurrentZ)]);
    startQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MoveLArg::CurrentQ1)],  // w
        command_->args[static_cast<size_t>(MoveLArg::CurrentQ2)],  // x
        command_->args[static_cast<size_t>(MoveLArg::CurrentQ3)],  // y
        command_->args[static_cast<size_t>(MoveLArg::CurrentQ4)]); // z

    targetPos_ = Eigen::Vector3d(
        command_->args[static_cast<size_t>(MoveLArg::X)],
        command_->args[static_cast<size_t>(MoveLArg::Y)],
        command_->args[static_cast<size_t>(MoveLArg::Z)]);
    targetQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MoveLArg::Q1)],  // w
        command_->args[static_cast<size_t>(MoveLArg::Q2)],  // x
        command_->args[static_cast<size_t>(MoveLArg::Q3)],  // y
        command_->args[static_cast<size_t>(MoveLArg::Q4)]); // z

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

    if (!otg_)
    {
        // 首段：创建 Ruckig 实例
        otg_    = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
        input_  = std::make_unique<InputParameter<DynamicDOFs>>(1);
        output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
        firstSegment_ = true;
    }

    if (firstSegment_)
    {
        // 首段：从零开始
        input_->current_position[0]     = 0;
        input_->current_velocity[0]     = 0;
        input_->current_acceleration[0] = 0;
        firstSegment_ = false;
    }
    else
    {
        // 后续段：保留上一段末尾的速度和加速度，只重置位置为 0
        // Ruckig 内部的 current_input 已经被 pass_to_input 更新
        // 这里只需要把位置归零（新段从 s=0 开始）
        input_->current_position[0] = 0;
        // current_velocity 和 current_acceleration 保持上一段末尾的值
        // 速度和加速度天然连续
    }

    // 只更新目标和限制
    input_->target_position[0]      = cartDist_;
    input_->target_velocity[0]      = tgtVel;
    input_->target_acceleration[0]  = 0;
    input_->max_velocity[0]         = maxVel;
    input_->max_acceleration[0]     = maxAccel;
    input_->max_jerk[0]             = maxJerk;

    return true;
}


void MoveL::applyOutput()
{
     updateOverride();

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working || result == Result::Finished)
    {
        double s = output_->new_position[0];

        // 线性插值得到当前笛卡尔位姿
        double u = s / cartDist_;
        if (u < 0) u = 0;
        if (u > 1) u = 1;

          Eigen::Vector3d pos = startPos_ + u * (targetPos_ - startPos_);
          controller_->axiss[axisIds_[0]]->setAxisPositionCmd(pos.x());
          controller_->axiss[axisIds_[1]]->setAxisPositionCmd(pos.y());
          controller_->axiss[axisIds_[2]]->setAxisPositionCmd(pos.z());

          // 四元数球面线性插补 (SLERP) — 含最短路径 + 小角度保护
          Eigen::Quaterniond qInterp = startQuat_.slerp(u, targetQuat_);
          if (axisIds_.size() >= 6)
          {
              Eigen::Vector3d euler = qInterp.toRotationMatrix().canonicalEulerAngles(2, 1, 0);
              controller_->axiss[axisIds_[3]]->setAxisPositionCmd(euler(2));  // rx
              controller_->axiss[axisIds_[4]]->setAxisPositionCmd(euler(1));  // ry
              controller_->axiss[axisIds_[5]]->setAxisPositionCmd(euler(0));  // rz
          }

        if (result == Result::Finished)
        {
            output_->pass_to_input(*input_);
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
        else
        {
            output_->pass_to_input(*input_);
        }
    }
    else
    {
        ERROR_PRINT(
            "MoveL invalid input: result=%d, dist=%.4f, curVel=%.4f, curAcc=%.4f, "
            "tgtVel=%.4f, tgtAcc=%.4f, maxVel=%.4f, maxAcc=%.4f, maxJerk=%.4f\n",
            static_cast<int>(result),
            cartDist_,
            input_->current_velocity[0],
            input_->current_acceleration[0],
            input_->target_velocity[0],
            input_->target_acceleration[0],
            input_->max_velocity[0],
            input_->max_acceleration[0],
            input_->max_jerk[0]
        );

        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
} 


CMD_REGISTER(MoveL);
