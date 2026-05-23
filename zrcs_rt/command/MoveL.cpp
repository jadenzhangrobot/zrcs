/*
 * @Description: 笛卡尔直线运动（ABB MoveL）�?1D 弧长参数�?+ 每周�?IK
 *               复用 Ruckig 实例，跨段速度/加速度天然连续
 */
#include "command/MoveL.h"
#include "shared_memory/ShmLayout.h"

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
     
    arcOffset_=arcOffset_+cartDist_;
    // 目标和限制使用累积弧长
    input_->target_position[0]      = arcOffset_;
    input_->target_velocity[0]      = tgtVel;
    input_->max_velocity[0]         = maxVel;
    input_->max_acceleration[0]     = maxAccel;
    input_->max_jerk[0]             = maxJerk;

    return true;
}


void MoveL::applyOutput()
{
    // otg_->update() 已由 TrajectoryCmd::runStandard()->updateTrajectory() 完成，
    // output_ 已包含当前周期的轨迹输出，此处不再重复调用 update/pass_to_input。

    double s = output_->new_position[0];
    const double pathVelocity = output_->new_velocity[0];

    // 线性插值得到当前笛卡尔位姿
    double u = (s - arcOffset_) / cartDist_;
    //u = std::clamp(u, 0.0, 1.0);

    Eigen::Vector3d pos = startPos_ + u * (targetPos_ - startPos_);
    const Eigen::Vector3d pathDir = (targetPos_ - startPos_) / cartDist_;

    controller_->axes_[axisIds_[0]]->setAxisPositionCmd(pos.x());
    controller_->axes_[axisIds_[1]]->setAxisPositionCmd(pos.y());
    controller_->axes_[axisIds_[2]]->setAxisPositionCmd(pos.z());

    // 速度前馈：将弧长速度按路径方向分解到各平动轴，减少伺服跟踪滞后
   // controller_->axes_[axisIds_[0]]->setAxisVelocityCmd(pathVelocity * pathDir.x());
    //controller_->axes_[axisIds_[1]]->setAxisVelocityCmd(pathVelocity * pathDir.y());
    //controller_->axes_[axisIds_[2]]->setAxisVelocityCmd(pathVelocity * pathDir.z());

    // 四元数球面线性插补 (SLERP) — 含最短路径 + 小角度保护
    Eigen::Quaterniond qInterp = startQuat_.slerp(u, targetQuat_);
    if (axisIds_.size() >= 6)
    {
        Eigen::Vector3d euler = qInterp.toRotationMatrix().canonicalEulerAngles(2, 1, 0);
        controller_->axes_[axisIds_[3]]->setAxisPositionCmd(euler(2));  // rx
        controller_->axes_[axisIds_[4]]->setAxisPositionCmd(euler(1));  // ry
        controller_->axes_[axisIds_[5]]->setAxisPositionCmd(euler(0));  // rz
        //controller_->axes_[axisIds_[3]]->setAxisVelocityCmd(0.0);
       // controller_->axes_[axisIds_[4]]->setAxisVelocityCmd(0.0);
       // controller_->axes_[axisIds_[5]]->setAxisVelocityCmd(0.0);
    }
} 


CMD_REGISTER(MoveL);
