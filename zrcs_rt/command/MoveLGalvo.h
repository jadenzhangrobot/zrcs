/*
 * @Description: 振镜-平台联动直线运动 �?1D 弧长参数�?+ IIR 低通滤波分�?
 *
 * 在全局坐标系中规划直线轨迹（与 MoveL 相同�?Ruckig 1D 弧长方案），
 * 每周期将插补结果通过一�?IIR 低通滤波器（LPF）分解为�?
 *   platform_pos = LPF(global_pos)          �?平台 X/Y 轴（低频，大行程�?
 *   galvo_offset = global_pos - platform_pos �?振镜 X/Y 轴（高频，小行程�?
 *
 * �?ID 和截止频率从共享内存 galvoCfg 中读取，�?NRT �?setGalvoConfig() 配置�?
 * 多段连续路径�?Ruckig 状态和 LPF 状态均保持连续（同一实例复用）�?
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/base/RtLowPassFilter.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>


using namespace ruckig;

class MoveLGalvo : public TrajectoryCmd
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>>         otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>>output_;

    // 线段几何缓存
    Eigen::Vector3d startPos_;
    Eigen::Vector3d targetPos_;
    double          cartDist_;
    bool            firstSegment_;

    // LPF 状态跨段持久（同一实例复用，不重置�?
    RtLowPassFilter lpf_;
    bool            lpfConfigured_;
    double          lastCutoffHz_;

protected:
    bool   initTrajectory()   override;
    Result updateTrajectory() override;
    void   applyOutput()      override;
    void   passOutputToInput()override;
    void   applyDeltaTime(double dt) override;

public:
    MoveLGalvo();
    void run() override;
};
