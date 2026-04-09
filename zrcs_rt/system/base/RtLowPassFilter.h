/// @file RtLowPassFilter.h
/// @brief RT 安全的一阶 IIR 低通滤波器 (2通道 XY)
///
/// 用于振镜-平台联动中将全局插补轨迹分解为低频平台分量和高频振镜分量。
/// 所有操作 O(1)、无动态分配、无阻塞，满足实时约束。
#pragma once

#include <cmath>

class RtLowPassFilter {
public:
    /// 配置滤波器参数
    /// @param cutoffHz 截止频率 (Hz)，通常设为平台伺服带宽的 30~50%
    /// @param sampleHz 采样频率 (Hz)，通常 = 1000 (1ms RT 周期)
    void configure(double cutoffHz, double sampleHz)
    {
        double rc = 1.0 / (2.0 * M_PI * cutoffHz);
        double dt = 1.0 / sampleHz;
        alpha_ = dt / (rc + dt);
        initialized_ = false;
    }

    /// 每周期调用: 输入全局坐标，输出滤波后的低频分量
    void update(double inX, double inY, double& outX, double& outY)
    {
        if (!initialized_) {
            stateX_ = inX;
            stateY_ = inY;
            initialized_ = true;
        }
        stateX_ += alpha_ * (inX - stateX_);
        stateY_ += alpha_ * (inY - stateY_);
        outX = stateX_;
        outY = stateY_;
    }

    /// 重置滤波器状态 (新轨迹开始时调用)
    void reset() { initialized_ = false; }

    bool isInitialized() const { return initialized_; }

private:
    double alpha_ = 0.02;   // 滤波系数: dt / (RC + dt)
    double stateX_ = 0;
    double stateY_ = 0;
    bool initialized_ = false;
};
