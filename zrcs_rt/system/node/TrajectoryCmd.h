/**
 * @Description: 轨迹规划基类 — 封装 Ruckig 生命周期与动态倍率（delta_time 时间缩放）
 *
 * 子类只需实现 initTrajectory() 设置 Ruckig 参数，并在 run() 中调用
 * updateOverride() 实现每周期的动态倍率。标准流程的命令可直接使用
 * runStandard() 代替手写 run()。
 */
#pragma once
#include "system/node/BaseNodeInterface.h"
#include "config/Parameter.h"
#include <ruckig/ruckig.hpp>
#include <memory>

using namespace ruckig;

class TrajectoryCmd : public zrcsSystem::CmdNode
{
protected:
    double baseDeltaTime_;  // 原始周期 = cycletime * 0.001
    std::unique_ptr<Ruckig<DynamicDOFs>>          otg_;    // 子类在 initTrajectory() 中 make_unique
    std::unique_ptr<InputParameter<DynamicDOFs>>  input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;


    /// 设置 Ruckig 参数（current/target/max_velocity 等），返回 false 表示失败
    virtual bool initTrajectory() = 0;

    /// 设置 otg.delta_time（子类持有具体的 Ruckig 对象，需转发）
    void applyDeltaTime(double dt);

    /// 调用 otg.update()，返回 Result
   Result updateTrajectory() ;

    /// 将 Ruckig output 的位置写入轴位置指令
    virtual void applyOutput() = 0;

    /// output.pass_to_input(input)
     void passOutputToInput() ;

    // --- 基类提供的辅助方法 ---

    /// 读取 overrideRatio 并更新 delta_time，每个 run() 周期调用
    void updateOverride();

    /// 标准 run() 流程：updateOverride → updateTrajectory → applyOutput
    void runStandard();

public:
    TrajectoryCmd();

    void init() override;
    void run() override;
    void exit() override;
};
