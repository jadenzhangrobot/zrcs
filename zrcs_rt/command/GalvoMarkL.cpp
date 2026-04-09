/// @file GalvoMarkL.cpp
/// @brief 振镜-平台联动标刻 — 单插补器 + 后置低通滤波分解
#include "command/GalvoMarkL.h"

GalvoMarkL::GalvoMarkL()
{
    std::strcpy(nodeName_, "GalvoMarkL");
}

Result GalvoMarkL::updateTrajectory() { return otg_->update(*input_, *output_); }
void GalvoMarkL::passOutputToInput() { output_->pass_to_input(*input_); }
void GalvoMarkL::applyDeltaTime(double dt) { otg_->delta_time = dt; }

bool GalvoMarkL::initTrajectory()
{
    // 读取轴 ID 映射
    platformXId_ = static_cast<int>(command_->args[GalvoMarkLPlatXId]);
    platformYId_ = static_cast<int>(command_->args[GalvoMarkLPlatYId]);
    galvoXId_ = static_cast<int>(command_->args[GalvoMarkLGalvoXId]);
    galvoYId_ = static_cast<int>(command_->args[GalvoMarkLGalvoYId]);

    // 读取全局目标坐标和运动参数
    double targetX = command_->args[GalvoMarkLX];
    double targetY = command_->args[GalvoMarkLY];
    double vel = command_->args[GalvoMarkLVel];
    if (vel <= 0) vel = 100.0;

    // 激光参数
    laserOn_ = command_->args[GalvoMarkLLaserOn] > 0;
    power_ = command_->args[GalvoMarkLPower];
    frequency_ = command_->args[GalvoMarkLFreq];

    // 低通滤波器截止频率
    double cutoffHz = command_->args[GalvoMarkLCutoffHz];
    if (cutoffHz <= 0) cutoffHz = 20.0;

    // 当前全局位置 = 平台实际位置 + 振镜实际位置
    double curGlobalX = controller_->axiss[platformXId_]->actualPos()
                      + controller_->axiss[galvoXId_]->actualPos();
    double curGlobalY = controller_->axiss[platformYId_]->actualPos()
                      + controller_->axiss[galvoYId_]->actualPos();

    // 2-DOF Ruckig: 在全局坐标系插补
    baseDeltaTime_ = cycletime * 0.001;
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(2, baseDeltaTime_);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(2);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(2);

    input_->current_position[0] = curGlobalX;
    input_->current_position[1] = curGlobalY;
    input_->current_velocity[0] = 0;
    input_->current_velocity[1] = 0;
    input_->current_acceleration[0] = 0;
    input_->current_acceleration[1] = 0;
    input_->target_position[0] = targetX;
    input_->target_position[1] = targetY;
    input_->target_velocity[0] = 0;
    input_->target_velocity[1] = 0;
    input_->target_acceleration[0] = 0;
    input_->target_acceleration[1] = 0;

    // 全局速度/加速度限制
    input_->max_velocity[0] = vel;
    input_->max_velocity[1] = vel;
    // 加速度/jerk 取平台限制 (全局运动的瓶颈是慢轴)
    input_->max_acceleration[0] = controller_->axiss[platformXId_]->getMaxAcceleration();
    input_->max_acceleration[1] = controller_->axiss[platformYId_]->getMaxAcceleration();
    input_->max_jerk[0] = controller_->axiss[platformXId_]->getMaxJerk();
    input_->max_jerk[1] = controller_->axiss[platformYId_]->getMaxJerk();

    // 初始化低通滤波器
    double sampleHz = 1.0 / baseDeltaTime_;
    lpf_.configure(cutoffHz, sampleHz);
    lpf_.reset();

    // 场畸变校正: 使用单位校正表 (无畸变)
    // 如需加载实际校正数据, 可在此处调用 fieldCorr_.loadCalibration(...)
    if (!fieldCorr_.isLoaded()) {
        double fieldHalf = controller_->axiss[galvoXId_]->getMaxVelocity() > 0 ? 50.0 : 50.0;
        fieldCorr_.loadIdentity(-fieldHalf, fieldHalf);
    }

    return true;
}

void GalvoMarkL::applyOutput()
{
    double globalX = output_->new_position[0];
    double globalY = output_->new_position[1];

    // ② 低通滤波 → 平台指令 (低频大范围)
    double platCmdX, platCmdY;
    lpf_.update(globalX, globalY, platCmdX, platCmdY);

    // ③ 残差 → 振镜规划指令 (高频精细)
    double galvoCmdX = globalX - platCmdX;
    double galvoCmdY = globalY - platCmdY;

    // ④ 平台跟踪误差补偿 (闭环)
    double platActualX = controller_->axiss[platformXId_]->actualPos();
    double platActualY = controller_->axiss[platformYId_]->actualPos();
    galvoCmdX += (platCmdX - platActualX);
    galvoCmdY += (platCmdY - platActualY);

    // ⑤ 场畸变校正
    fieldCorr_.correct(galvoCmdX, galvoCmdY, galvoCmdX, galvoCmdY);

    // ⑥ 写入各轴
    controller_->axiss[platformXId_]->setAxisPositionCmd(platCmdX);
    controller_->axiss[platformYId_]->setAxisPositionCmd(platCmdY);
    controller_->axiss[galvoXId_]->setAxisPositionCmd(galvoCmdX);
    controller_->axiss[galvoYId_]->setAxisPositionCmd(galvoCmdY);

    // ⑦ 激光控制
    auto* laser = controller_->laser();
    if (laser) {
        if (laserOn_) {
            laser->setGate(true);
            laser->setPower(power_);
            laser->setFrequency(frequency_);
        } else {
            laser->setGate(false);
        }
        laser->applyToHardware();
    }
}

REGISTERCMD(GalvoMarkL);
