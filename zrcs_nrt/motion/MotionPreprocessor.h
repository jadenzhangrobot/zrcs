/**
 * @brief NRT 侧路径预处理器
 *
 * 接收一组离散笛卡尔目标点，执行：
 *   1. PathPreprocessor  — 三次样条拟合 + 弧长重采样
 *   2. VelocityPlanner3D — 速度前瞻（拐角限速 + 前后向扫描）
 *   3. 将结果路点（含 maxVel）批量写入 SHM pathQueue
 *
 * RT 侧 PathMove（OutputNode）从 pathQueue 消费路点，
 * 用 Ruckig 1D 弧长参数化插补 + IK 输出关节指令。
 */
#pragma once

#include "motion/PathPreprocessor.h"
#include "motion/VelocityPlanner3D.h"
#include "rtBridge/RtBridge.h"
#include "shared_memory/ShmLayout.h"
#include <spdlog/spdlog.h>

class MotionPreprocessor {
public:
    struct Config {
        double maxVel    = 10.0;    // mm/s（与轴配置 motion/maxVel 一致）
        double maxAccel  = 20.0;    // mm/s²（与轴配置 motion/maxAcc 一致）
        double maxJerk   = 30.0;    // mm/s³（与轴配置 motion/maxJerk 一致）
        double stepSize  = 1.0;     // mm，重采样步长（仅用于角点 Bezier 采样，不影响 MoveL 段数）
        double cornerTol = 0.5;     // mm，拐角偏差容限
        bool   galvoMode = false;   // true: 使用 MoveLGalvo（振镜-平台分解）而非 MoveL
    };

    explicit MotionPreprocessor(RtBridge* bridge)
        : bridge_(bridge) {}

    /**
     * @brief 处理一组离散目标点
     * @param waypoints 离散笛卡尔位置序列
     * @param rx, ry, rz 姿态（简化：各段相同）
     * @param cfg 运动参数配置
     * @return true 成功写入路径队列
     */
    bool process(const std::vector<Point3D>& waypoints,
                 double rx, double ry, double rz,
                 const Config& cfg)
    {
        if (waypoints.size() < 2) {
            spdlog::error("[MotionPreprocessor] Need at least 2 waypoints");
            return false;
        }

        // 将运动参数同步到 RT 侧，确保 Ruckig 使用与速度前瞻相同的限制
        bridge_->setPathMoveConfig(cfg.maxVel, cfg.maxAccel, cfg.maxJerk);

        // Step 1: 直接对原始路点做速度前瞻（不密集重采样）
        // 每个 waypoint 间距足够长，Ruckig 可以完成完整 S 型轨迹
        // Bezier 角点混合由 VelocityPlanner 的拐角限速公式处理
        velPlanner_.clearPath();
        velPlanner_.setConfig(cfg.maxVel, cfg.maxAccel, 0.0, 0.0, cfg.cornerTol, cfg.maxJerk);
        for (const auto& p : waypoints) {
            velPlanner_.addPoint(p.x, p.y, p.z);
        }
        if (!velPlanner_.plan()) {
            spdlog::error("[MotionPreprocessor] Velocity planning failed");
            return false;
        }

        // Step 2: 每对相邻路点发一条 MoveL（段数 = waypoints.size()-1）
        const auto& planned = velPlanner_.getPath();
        for (size_t i = 1; i < planned.size(); ++i)
        {
            const auto& start = planned[i - 1];
            const auto& end = planned[i];
            // 用前后向扫描所允许的峰值速度，而非仅取端点速度的最大值
            // 以免把中间本可加速的长线段限制在拐角低速
            double segmentMaxVel = cfg.maxVel;

            double args[] = {
                start.pos.x,           // CurrentX
                start.pos.y,           // CurrentY
                start.pos.z,           // CurrentZ
                rx,                    // CurrentRX
                ry,                    // CurrentRY
                rz,                    // CurrentRZ
                end.pos.x,             // X
                end.pos.y,             // Y
                end.pos.z,             // Z
                rx,                    // RX
                ry,                    // RY
                rz,                    // RZ
                segmentMaxVel,         // Vel
                start.velocity,        // CurrentVel
                end.velocity           // TargetVel
            };

            auto [result, seq] = bridge_->sendCommand(cfg.galvoMode ? "MoveLGalvo" : "MoveL", args, 15);
            if (result != RtBridge::SendResult::OK) {
                spdlog::error("[MotionPreprocessor] sendCommand MoveL failed at segment {}/{}",
                              i, planned.size() - 1);
                return false;
            }
        }

        spdlog::info("[MotionPreprocessor] {} MoveL segments sent, maxVel={}, cornerTol={}",
                     planned.size() - 1, cfg.maxVel, cfg.cornerTol);
        return true;
    }

private:
    RtBridge* bridge_;
    PathPreprocessor pathFitter_;
    VelocityPlanner3D velPlanner_;
};
