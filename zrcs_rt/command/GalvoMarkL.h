/// @file GalvoMarkL.h
/// @brief 振镜-平台联动标刻命令 — 单插补器 + 后置低通滤波分解
///
/// 核心算法: 单个 Ruckig 在全局 2-DOF 空间插补，每周期对输出做低通滤波
/// 分解为平台低频分量和振镜高频残差，振镜额外补偿平台跟踪误差。
#pragma once

#include "config/CmdArgs.h"
#include "system/base/TrajectoryCmd.h"
#include "system/base/RtLowPassFilter.h"
#include "system/NodeFactory.h"
#include "model/GalvoFieldCorrection.h"
#include "controller/laser/LaserController.h"

#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class GalvoMarkL : public TrajectoryCmd {
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;

    // 轴 ID 映射
    int platformXId_ = 0;
    int platformYId_ = 1;
    int galvoXId_ = 2;
    int galvoYId_ = 3;

    // 后置滤波器: 将全局轨迹分解为平台(低频) + 振镜(高频)
    RtLowPassFilter lpf_;

    // 场畸变校正 (可选)
    GalvoFieldCorrection fieldCorr_;

    // 激光参数
    bool laserOn_ = false;
    double power_ = 0;
    double frequency_ = 20000;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    GalvoMarkL();
};
