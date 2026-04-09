/// @file LaserController.h
/// @brief 激光器实时控制封装 — 管理激光开关、功率、频率和脉宽
#pragma once

#include "controller/ControllerInterface.h"

namespace ZrcsHardware {

/// 激光器配置参数 (从 laser.xml 解析)
struct LaserParam {
    int ioModuleIndex = 0;   // IO 模块在 controller->ios_ 中的索引
    int gateIndex = 0;       // 门控信号 DO 的 PDO 偏移索引
    int gateBit = 0;         // 门控信号的 bit 位
    int powerIndex = 1;      // 功率 AO 的 PDO 偏移索引
    int freqIndex = 2;       // 频率 AO 的 PDO 偏移索引
    int pulseWidthIndex = 3; // 脉宽 AO 的 PDO 偏移索引
    double powerMin = 0;     // 功率范围最小值
    double powerMax = 100;   // 功率范围最大值
    double freqMin = 1000;   // 频率范围最小值 (Hz)
    double freqMax = 200000; // 频率范围���大值 (Hz)
    double pwMin = 5;        // 脉宽范围最小值 (ns)
    double pwMax = 500;      // 脉宽范围最大值 (ns)
};

/// @brief 激光器实时控制器
///
/// 封装激光器的 DO (门控) 和 AO (功率/频率/脉宽) 通道。
/// 所有方法 RT 安全: 仅更新内部缓存，applyToHardware() 一次性写入 IO。
class LaserController {
public:
    LaserController(Io* io, const LaserParam& param)
        : io_(io), param_(param)
    {}

    /// 设置激光门控 (开/关)
    void setGate(bool on) { gateOn_ = on; }

    /// 设置功率百分比 [0, 100]
    void setPower(double percent)
    {
        if (percent < param_.powerMin) percent = param_.powerMin;
        if (percent > param_.powerMax) percent = param_.powerMax;
        power_ = percent;
    }

    /// 设置脉冲频率 (Hz)
    void setFrequency(double hz)
    {
        if (hz < param_.freqMin) hz = param_.freqMin;
        if (hz > param_.freqMax) hz = param_.freqMax;
        frequency_ = hz;
    }

    /// 设置脉冲宽度 (ns)
    void setPulseWidth(double ns)
    {
        if (ns < param_.pwMin) ns = param_.pwMin;
        if (ns > param_.pwMax) ns = param_.pwMax;
        pulseWidth_ = ns;
    }

    /// 将缓存的激光参数一次性写入 IO 硬件
    void applyToHardware()
    {
        if (!io_) return;
        io_->ioWrite8(param_.gateIndex, param_.gateBit, gateOn_);
        io_->aoWriteValue(param_.powerIndex, power_);
        io_->aoWriteValue(param_.freqIndex, frequency_);
        io_->aoWriteValue(param_.pulseWidthIndex, pulseWidth_);
    }

    bool isGateOn() const { return gateOn_; }
    double power() const { return power_; }
    double frequency() const { return frequency_; }
    double pulseWidth() const { return pulseWidth_; }

private:
    Io* io_ = nullptr;
    LaserParam param_;

    bool gateOn_ = false;
    double power_ = 0;
    double frequency_ = 20000;
    double pulseWidth_ = 100;
};

} // namespace ZrcsHardware
