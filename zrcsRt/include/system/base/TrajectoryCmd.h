/**
 * @Description: 轨迹规划基类 — 封装 Ruckig 生命周期与动态倍率（delta_time 时间缩放）
 *
 * 子类只需实现 initTrajectory() 设置 Ruckig 参数，并在 run() 中调用
 * updateOverride() 实现每周期的动态倍率。标准流程的命令可直接使用
 * runStandard() 代替手写 run()。
 */
#pragma once
#include "system/base/basenodeInterface.h"
#include "config/parameter.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class TrajectoryCmd : public zrcsSystem::CmdNode
{
protected:
    double baseDeltaTime_;  // 原始周期 = cycletime * 0.001

    // --- 子类必须实现 ---

    /// 设置 Ruckig 参数（current/target/max_velocity 等），返回 false 表示失败
    virtual bool initTrajectory() = 0;

    /// 设置 otg.delta_time（子类持有具体的 Ruckig 对象，需转发）
    virtual void applyDeltaTime(double dt) = 0;

    /// 调用 otg.update()，返回 Result
    virtual Result updateTrajectory() = 0;

    /// 将 Ruckig output 的位置写入轴位置指令
    virtual void applyOutput() = 0;

    /// output.pass_to_input(input)
    virtual void passOutputToInput() = 0;

    // --- 基类提供的辅助方法 ---

    /// 读取 overrideRatio 并更新 delta_time，每个 run() 周期调用
    void updateOverride()
    {
        double override = shm().overrideRatio().load(std::memory_order_acquire);
        applyDeltaTime(baseDeltaTime_ * override);
    }

    /// 标准 run() 流程：updateOverride → updateTrajectory → applyOutput
    /// 适用于不需要额外逻辑的简单运动命令
    void runStandard()
    {
        updateOverride();

        auto result = updateTrajectory();
        if (result == Result::Working)
        {
            applyOutput();
            passOutputToInput();
        }
        else if (result == Result::Finished)
        {
            applyOutput();
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
        else
        {
            ERROR_PRINT("%s: 轨迹规划失败\n", nodeName_);
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        }
    }

public:
    TrajectoryCmd() : baseDeltaTime_(cycletime * 0.001) {}

    void init() override
    {
        if (!initTrajectory())
        {
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            return;
        }
        // 应用初始倍率
        updateOverride();
    }

    void run() override
    {
        runStandard();
    }

    void exit() override {}
};
