/// @file GalvoBufMark.h
/// @brief 缓冲连续联动标刻 — 多段路径缓冲后不停顿连续执行
///
/// 参照 BufMove 模式: type >= 0 缓冲一段, type < 0 开始执行。
/// 每段可独立设置标刻/跳转、功率、频率。
/// 低通滤波器状态跨段保持，保证平台轨迹连续。
#pragma once

#include "config/CmdArgs.h"
#include "system/base/TrajectoryCmd.h"
#include "system/base/RtLowPassFilter.h"
#include "system/NodeFactory.h"
#include "model/GalvoFieldCorrection.h"
#include "controller/laser/LaserController.h"

#include <memory>
#include <vector>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class GalvoBufMark : public TrajectoryCmd {
private:
    struct MarkSegment {
        int type;           // 0=标刻(激光开), 1=跳转(激光关)
        double targetX;
        double targetY;
        double vel;
        double power;
        double frequency;
    };

    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;

    // 轴映射
    int platformXId_ = 0;
    int platformYId_ = 1;
    int galvoXId_ = 2;
    int galvoYId_ = 3;
    double cutoffHz_ = 20.0;

    // 缓冲段
    int segIdx_ = 0;
    std::vector<MarkSegment> segments_;

    // 当前段激光状态
    bool curLaserOn_ = false;
    double curPower_ = 0;
    double curFrequency_ = 20000;

    // 滤波器和校正 (跨段保持)
    RtLowPassFilter lpf_;
    GalvoFieldCorrection fieldCorr_;

    void setupSegment(int idx);

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    GalvoBufMark();
    void init() override;
    void run() override;
};
