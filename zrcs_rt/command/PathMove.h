/*
 * @Description: 弧长参数化笛卡尔路径运动 — NRT 预处理路点经 SHM 传入，
 *               RT 侧用 1D Ruckig 在弧长 s 上做 S 曲线插补 + IK
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <cmath>
#include <algorithm>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>


using namespace ruckig;

class PathMove : public TrajectoryCmd
{
public:
    CMD_DEFINE(48, PARAM(Vel))

private:
    std::unique_ptr<Ruckig<DynamicDOFs>>          otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>>   input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>>  output_;

    int dof_;
    std::vector<int> axisIds_;

    // 路径缓冲（initTrajectory 时从 SHM pathQueue 批量排空）
    static constexpr size_t kMaxPts = 256;
    struct PathEntry {
        double x, y, z;
        double rx, ry, rz;
        double s;       // 累计弧长 (mm)
        double maxVel;  // 速度前瞻限制 (mm/s)
    };
    PathEntry pathBuf_[kMaxPts];
    size_t    pathLen_;
    double    totalArc_;

    /// 二分查找 + 线性插值：弧长 s → 笛卡尔位姿
    void lookupPath(double s, double& x, double& y, double& z,
                    double& rx, double& ry, double& rz) const;

    /// 二分查找 + 线性插值：弧长 s → 速度限制
    double lookupMaxVel(double s) const;

protected:
    bool   initTrajectory() override;
    Result updateTrajectory() override;
    void   applyOutput() override;
    void   passOutputToInput() override;
    void   applyDeltaTime(double dt) override;

public:
    PathMove();
    void run() override;
};
