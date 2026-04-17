/*
 * @Description: 振镜-平台联动直线运动（MoveLGalvo）
 *               与 MoveL 使用相同的 Ruckig 1D 弧长插补方案，
 *               输出通过 IIR LPF 分解为平台低频分量和振镜高频偏移量。
 */
#include "command/MoveLGalvo.h"
#include "shared_memory/ShmLayout.h"

MoveLGalvo::MoveLGalvo()
    : cartDist_(0), firstSegment_(true),
      lpfConfigured_(false), lastCutoffHz_(0.0)
{
    std::strcpy(nodeName_, "MoveLGalvo");
}

Result MoveLGalvo::updateTrajectory() { return otg_->update(*input_, *output_); }
void   MoveLGalvo::applyOutput() {}   // run() 中手动处理
void   MoveLGalvo::passOutputToInput() { output_->pass_to_input(*input_); }
void   MoveLGalvo::applyDeltaTime(double dt) { otg_->delta_time = dt; }

bool MoveLGalvo::initTrajectory()
{
    // 从命令参数构建起点和终点位置
    startPos_ = Eigen::Vector3d(
        command_->args[MoveLGalvoCurrentX],
        command_->args[MoveLGalvoCurrentY],
        command_->args[MoveLGalvoCurrentZ]);

    targetPos_ = Eigen::Vector3d(
        command_->args[MoveLGalvoX],
        command_->args[MoveLGalvoY],
        command_->args[MoveLGalvoZ]);

    cartDist_ = (targetPos_ - startPos_).norm();
    if (cartDist_ < 1e-6)
    {
        ERROR_PRINT("MoveLGalvo: 线段长度为零\n");
        return false;
    }

    double maxVel   = command_->args[MoveLGalvoVel];
    double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    double maxJerk  = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);

    if (!otg_)
    {
        otg_    = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
        input_  = std::make_unique<InputParameter<DynamicDOFs>>(1);
        output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
        firstSegment_ = true;
    }

    if (firstSegment_)
    {
        input_->current_position[0]     = 0;
        input_->current_velocity[0]     = 0;
        input_->current_acceleration[0] = 0;
        firstSegment_ = false;
    }
    else
    {
        // 后续段：位置归零，速度/加速度由 pass_to_input 保持连续
        input_->current_position[0] = 0;
    }

    input_->target_position[0]     = cartDist_;
    input_->target_velocity[0]     = command_->args[MoveLGalvoTargetVel];
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

void MoveLGalvo::run()
{
    updateOverride();

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working || result == Result::Finished)
    {
        double s = output_->new_position[0];

        // 线性插值得到当前全局笛卡尔位置
        double u = s / cartDist_;
        if (u < 0.0) u = 0.0;
        if (u > 1.0) u = 1.0;

        Eigen::Vector3d pos = startPos_ + u * (targetPos_ - startPos_);

        // LPF 分解：平台跟踪低频分量，振镜补偿高频残差
        double platX, platY;
        lpf_.update(pos.x(), pos.y(), platX, platY);
        double galvoX = pos.x() - platX;
        double galvoY = pos.y() - platY;

        // 读取轴 ID（运行时可由 NRT 侧动态配置）
        int pXId = shm()->galvoCfg.platXId.load(std::memory_order_acquire);
        int pYId = shm()->galvoCfg.platYId.load(std::memory_order_acquire);
        int gXId = shm()->galvoCfg.galvoXId.load(std::memory_order_acquire);
        int gYId = shm()->galvoCfg.galvoYId.load(std::memory_order_acquire);

        controller_->axiss[pXId]->setAxisPositionCmd(platX);
        controller_->axiss[pYId]->setAxisPositionCmd(platY);
        controller_->axiss[gXId]->setAxisPositionCmd(galvoX);
        controller_->axiss[gYId]->setAxisPositionCmd(galvoY);

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
            "MoveLGalvo invalid input: result=%d, dist=%.4f, curVel=%.4f, "
            "tgtVel=%.4f, maxVel=%.4f, maxAcc=%.4f, maxJerk=%.4f\n",
            static_cast<int>(result),
            cartDist_,
            input_->current_velocity[0],
            input_->target_velocity[0],
            input_->max_velocity[0],
            input_->max_acceleration[0],
            input_->max_jerk[0]
        );
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

REGISTERCMD(MoveLGalvo, 52);
