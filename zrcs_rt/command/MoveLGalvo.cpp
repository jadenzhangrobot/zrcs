/*
 * @Description: 振镜-平台联动直线运动（MoveLGalvo）
 *               MoveL 使用相同的 Ruckig 1D 弧长插补方案
 *               输出通过 IIR LPF 分解为平台低频分量和振镜高频偏移量
 */
#include "command/MoveLGalvo.h"
#include "shared_memory/ShmLayout.h"

MoveLGalvo::MoveLGalvo() : cartDist_(0),
      lpfConfigured_(false), lastCutoffHz_(0.0)
{
    std::strcpy(nodeName_, "MoveLGalvo");
    // 设置 Ruckig 轨迹规划
    if (!otg_)
    {
        otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
        input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
        output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
    }
    input_->duration_discretization = DurationDiscretization::Discrete;
}



bool MoveLGalvo::initTrajectory()
{
    // 从命令参数构建起点和终点位置
    startPos_ = Eigen::Vector3d(
        command_->args[static_cast<size_t>(MoveLGalvoArg::CurrentX)],
        command_->args[static_cast<size_t>(MoveLGalvoArg::CurrentY)],
        command_->args[static_cast<size_t>(MoveLGalvoArg::CurrentZ)]);

    targetPos_ = Eigen::Vector3d(
        command_->args[static_cast<size_t>(MoveLGalvoArg::X)],
        command_->args[static_cast<size_t>(MoveLGalvoArg::Y)],
        command_->args[static_cast<size_t>(MoveLGalvoArg::Z)]);

    cartDist_ = (targetPos_ - startPos_).norm();
    if (cartDist_ < 1e-6)
    {
        ERROR_PRINT("MoveLGalvo: 线段长度为零\n");
        return false;
    }

    // Ruckig 弧长参数化起始状态
    input_->current_position[0]      = 0.0;
    input_->current_velocity[0]      = command_->args[static_cast<size_t>(MoveLGalvoArg::CurrentVel)];
    input_->current_acceleration[0]  = command_->args[static_cast<size_t>(MoveLGalvoArg::CurrentAcc)];

    double maxVel   = command_->args[static_cast<size_t>(MoveLGalvoArg::Vel)];
    double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    double maxJerk  = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);

    input_->target_position[0]     = cartDist_;
    input_->target_velocity[0]     = command_->args[static_cast<size_t>(MoveLGalvoArg::TargetVel)];
    input_->target_acceleration[0] = 0;
    input_->max_velocity[0]        = maxVel;
    input_->max_acceleration[0]    = maxAccel;
    input_->max_jerk[0]            = maxJerk;

    // 配置 LPF（仅在截止频率变化时重新配置，避免重置状态）
    double cutoffHz = shm()->galvoCfg.cutoffHz.load(std::memory_order_acquire);
    if (!lpfConfigured_ || cutoffHz != lastCutoffHz_)
    {
        double sampleHz = 1000.0 / cycletime;  // cycletime 单位 ms
        lpf_.configure(cutoffHz, sampleHz);
        lastCutoffHz_  = cutoffHz;
        lpfConfigured_ = true;
    }
    
    return true;
}

void MoveLGalvo::applyOutput()
{
        double s = output_->new_position[0];

        // 线性插值得到当前全局笛卡尔位置
        double u = s / cartDist_;
        //u = std::clamp(u, 0.0, 1.0);

        Eigen::Vector3d pos = startPos_ + u * (targetPos_ - startPos_);

        // LPF 分解：平台跟踪低频分量，振镜补偿高频残差
        double platX, platY;
        lpf_.update(pos.x(), pos.y(), platX, platY);
        double galvoX = pos.x() - platX;
        double galvoY = pos.y() - platY;

        // 读取ID（运行时可由 NRT 侧动态配置）
        int pXId = shm()->galvoCfg.platXId.load(std::memory_order_acquire);
        int pYId = shm()->galvoCfg.platYId.load(std::memory_order_acquire);
        int gXId = shm()->galvoCfg.galvoXId.load(std::memory_order_acquire);
        int gYId = shm()->galvoCfg.galvoYId.load(std::memory_order_acquire);

        controller_->axes_[pXId]->setAxisPositionCmd(platX);
        controller_->axes_[pYId]->setAxisPositionCmd(platY);
        controller_->axes_[gXId]->setAxisPositionCmd(galvoX);
        controller_->axes_[gYId]->setAxisPositionCmd(galvoY);

 }

CMD_REGISTER(MoveLGalvo);
