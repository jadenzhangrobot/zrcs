/*
 * @Description: 笛卡尔直线运动（ABB MoveL）— 1D 弧长参数化 + 每周期 IK
 *               复用 Ruckig 实例，跨段速度/加速度天然连续
 */
#include "command/MoveL.h"
#include "shared_memory/ShmLayout.h"

MoveL::MoveL() : dof_(0), cartDist_(0), firstSegment_(true)
{
    std::strcpy(nodeName_, "MoveL");
}

Result MoveL::updateTrajectory() { return otg_->update(*input_, *output_); }
void   MoveL::applyOutput() {}  // run() 中手动处理
void   MoveL::passOutputToInput() { output_->pass_to_input(*input_); }
void   MoveL::applyDeltaTime(double dt) { otg_->delta_time = dt; }

bool MoveL::initTrajectory()
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

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    // 从命令参数构建起点和终点位姿
    startPos_ = Eigen::Vector3d(
        command_->args[MoveLCurrentX],
        command_->args[MoveLCurrentY],
        command_->args[MoveLCurrentZ]);
    startRpy_ = Eigen::Vector3d(
        command_->args[MoveLCurrentRX],
        command_->args[MoveLCurrentRY],
        command_->args[MoveLCurrentRZ]);

    targetPos_ = Eigen::Vector3d(
        command_->args[MoveLX],
        command_->args[MoveLY],
        command_->args[MoveLZ]);
    targetRpy_ = Eigen::Vector3d(
        command_->args[MoveLRX],
        command_->args[MoveLRY],
        command_->args[MoveLRZ]);

    // 计算线段长度
    cartDist_ = (targetPos_ - startPos_).norm();
    if (cartDist_ < 1e-6)
    {
        ERROR_PRINT("MoveL: 线段长度为零\n");
        return false;
    }

    // 读取笛卡尔标量边界条件
    double maxVel = command_->args[MoveLVel];
    double tgtVel = command_->args[MoveLTargetVel];

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
        // Ruckig 内部的 current_input 已经被 pass_to_input 更新过
        // 这里只需要把位置归零（新段从 s=0 开始）
        input_->current_position[0] = 0;
        // current_velocity 和 current_acceleration 保持上一段末尾的值
        // → 速度和加速度天然连续
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

void MoveL::run()
{
    updateOverride();

    auto* registry = modelRegistry_;
    RobotModel* model = registry->getModel(0);

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working || result == Result::Finished)
    {
        double s = output_->new_position[0];

        // 线性插值得到当前笛卡尔位姿
        double u = s / cartDist_;
        if (u < 0) u = 0;
        if (u > 1) u = 1;

        Eigen::Vector3d pos = startPos_ + u * (targetPos_ - startPos_);
        Eigen::Vector3d rpy = startRpy_ + u * (targetRpy_ - startRpy_);

        Eigen::Matrix4d targetPose = RobotModel::poseFromXYZRPY(
            pos.x(), pos.y(), pos.z(),
            rpy.x(), rpy.y(), rpy.z());

        // 当前关节位置作为 IK seed
        Eigen::VectorXd currentJoint(dof_);
        for (int i = 0; i < dof_; i++)
        {
            currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
        }

        // IK 求解
        Eigen::VectorXd targetJoint(dof_);
        if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
        {
            ERROR_PRINT("MoveL: IK 求解失败 (s=%.2f)\n", s);
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            return;
        }

        // 写入关节指令
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds_[i]]->setAxisPositionCmd(targetJoint(i));
        }

        if (result == Result::Finished)
        {
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

REGISTERCMD(MoveL, 25);
