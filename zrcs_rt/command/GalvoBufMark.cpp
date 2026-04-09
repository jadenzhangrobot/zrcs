/// @file GalvoBufMark.cpp
/// @brief 缓冲连续联动标刻实现
#include "command/GalvoBufMark.h"

GalvoBufMark::GalvoBufMark() : segIdx_(0)
{
    std::strcpy(nodeName_, "GalvoBufMark");
}

Result GalvoBufMark::updateTrajectory() { return otg_->update(*input_, *output_); }
void GalvoBufMark::passOutputToInput() { output_->pass_to_input(*input_); }
void GalvoBufMark::applyDeltaTime(double dt) { if (otg_) otg_->delta_time = dt; }

bool GalvoBufMark::initTrajectory()
{
    return true;  // 不使用标准流程, 由 init() 接管
}

void GalvoBufMark::init()
{
    int type = static_cast<int>(command_->args[GalvoBufMarkType]);

    // type < 0: 开始执行缓冲段
    if (type < 0) {
        if (segments_.empty()) {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
            return;
        }

        // 读取轴映射和滤波参数 (从启动命令)
        platformXId_ = static_cast<int>(command_->args[GalvoBufMarkPlatXId]);
        platformYId_ = static_cast<int>(command_->args[GalvoBufMarkPlatYId]);
        galvoXId_ = static_cast<int>(command_->args[GalvoBufMarkGalvoXId]);
        galvoYId_ = static_cast<int>(command_->args[GalvoBufMarkGalvoYId]);
        cutoffHz_ = command_->args[GalvoBufMarkCutoffHz];
        if (cutoffHz_ <= 0) cutoffHz_ = 20.0;

        // 初始化滤波器和场校正
        baseDeltaTime_ = cycletime * 0.001;
        lpf_.configure(cutoffHz_, 1.0 / baseDeltaTime_);
        lpf_.reset();
        if (!fieldCorr_.isLoaded()) {
            fieldCorr_.loadIdentity(-50.0, 50.0);
        }

        segIdx_ = 0;
        setupSegment(segIdx_);
        return;
    }

    // type >= 0: 缓冲一段
    MarkSegment seg;
    seg.type = type;
    seg.targetX = command_->args[GalvoBufMarkX];
    seg.targetY = command_->args[GalvoBufMarkY];
    seg.vel = command_->args[GalvoBufMarkVel];
    seg.power = command_->args[GalvoBufMarkPower];
    seg.frequency = command_->args[GalvoBufMarkFreq];
    segments_.push_back(seg);

    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void GalvoBufMark::setupSegment(int idx)
{
    if (idx >= static_cast<int>(segments_.size())) {
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        return;
    }

    const auto& seg = segments_[idx];
    curLaserOn_ = (seg.type == 0);
    curPower_ = seg.power;
    curFrequency_ = seg.frequency;

    // 当前全局位置 = 平台 + 振镜
    double curX = controller_->axiss[platformXId_]->actualPos()
                + controller_->axiss[galvoXId_]->actualPos();
    double curY = controller_->axiss[platformYId_]->actualPos()
                + controller_->axiss[galvoYId_]->actualPos();

    double vel = seg.vel;
    if (vel <= 0) vel = 100.0;

    baseDeltaTime_ = cycletime * 0.001;
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(2, baseDeltaTime_);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(2);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(2);

    input_->current_position[0] = curX;
    input_->current_position[1] = curY;
    input_->current_velocity[0] = 0;
    input_->current_velocity[1] = 0;
    input_->current_acceleration[0] = 0;
    input_->current_acceleration[1] = 0;
    input_->target_position[0] = seg.targetX;
    input_->target_position[1] = seg.targetY;
    input_->target_velocity[0] = 0;
    input_->target_velocity[1] = 0;
    input_->target_acceleration[0] = 0;
    input_->target_acceleration[1] = 0;

    input_->max_velocity[0] = vel;
    input_->max_velocity[1] = vel;
    input_->max_acceleration[0] = controller_->axiss[platformXId_]->getMaxAcceleration();
    input_->max_acceleration[1] = controller_->axiss[platformYId_]->getMaxAcceleration();
    input_->max_jerk[0] = controller_->axiss[platformXId_]->getMaxJerk();
    input_->max_jerk[1] = controller_->axiss[platformYId_]->getMaxJerk();

    updateOverride();
}

void GalvoBufMark::applyOutput()
{
    double globalX = output_->new_position[0];
    double globalY = output_->new_position[1];

    // 低通滤波 → 平台
    double platCmdX, platCmdY;
    lpf_.update(globalX, globalY, platCmdX, platCmdY);

    // 残差 → 振镜
    double galvoCmdX = globalX - platCmdX;
    double galvoCmdY = globalY - platCmdY;

    // 平台误差补偿
    double platActualX = controller_->axiss[platformXId_]->actualPos();
    double platActualY = controller_->axiss[platformYId_]->actualPos();
    galvoCmdX += (platCmdX - platActualX);
    galvoCmdY += (platCmdY - platActualY);

    // 场畸变校正
    fieldCorr_.correct(galvoCmdX, galvoCmdY, galvoCmdX, galvoCmdY);

    // 写入轴
    controller_->axiss[platformXId_]->setAxisPositionCmd(platCmdX);
    controller_->axiss[platformYId_]->setAxisPositionCmd(platCmdY);
    controller_->axiss[galvoXId_]->setAxisPositionCmd(galvoCmdX);
    controller_->axiss[galvoYId_]->setAxisPositionCmd(galvoCmdY);

    // 激光控制
    auto* laser = controller_->laser();
    if (laser) {
        laser->setGate(curLaserOn_);
        if (curLaserOn_) {
            laser->setPower(curPower_);
            laser->setFrequency(curFrequency_);
        }
        laser->applyToHardware();
    }
}

void GalvoBufMark::run()
{
    if (!otg_) {
        ERROR_PRINT("GalvoBufMark: 段数据为空\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    updateOverride();

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working) {
        applyOutput();
        output_->pass_to_input(*input_);
    } else if (result == Result::Finished) {
        applyOutput();

        segIdx_++;
        if (segIdx_ < static_cast<int>(segments_.size())) {
            setupSegment(segIdx_);
        } else {
            // 全部段完成, 关闭激光
            auto* laser = controller_->laser();
            if (laser) {
                laser->setGate(false);
                laser->applyToHardware();
            }
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
    } else {
        ERROR_PRINT("GalvoBufMark: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

REGISTERCMD(GalvoBufMark);
