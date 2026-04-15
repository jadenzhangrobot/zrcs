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
#include "rt_bridge/RtBridge.h"
#include "shared_memory/ShmLayout.h"
#include <spdlog/spdlog.h>

class MotionPreprocessor {
public:
    struct Config {
        double maxVel    = 100.0;   // mm/s
        double maxAccel  = 500.0;   // mm/s²
        double maxJerk   = 2000.0;  // mm/s³
        double stepSize  = 1.0;     // mm，重采样步长
        double cornerTol = 0.5;     // mm，拐角偏差容限
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

        // Step 1: 路径拟合 + 重采样
        std::vector<Point3D> smoothPath;
        if (waypoints.size() >= 3) {
            smoothPath = pathFitter_.processWithSpline(waypoints, cfg.stepSize);
        } else {
            smoothPath = waypoints;  // 两点直线，无需拟合
        }

        if (smoothPath.size() < 2) {
            spdlog::error("[MotionPreprocessor] Spline produced < 2 points");
            return false;
        }

        // Step 2: 速度前瞻
        velPlanner_.clearPath();
        velPlanner_.setConfig(cfg.maxVel, cfg.maxAccel, 0.0, 0.0, cfg.cornerTol);
        for (const auto& p : smoothPath) {
            velPlanner_.addPoint(p.x, p.y, p.z);
        }
        if (!velPlanner_.plan()) {
            spdlog::error("[MotionPreprocessor] Velocity planning failed");
            return false;
        }

        // Step 3: 将每个规划路点作为 MoveL 命令发送到命令队列
        const auto& planned = velPlanner_.getPath();
        for (size_t i = 0; i < planned.size(); ++i)
        {
            double args[] = {
                planned[i].pos.x,   // X
                planned[i].pos.y,   // Y
                planned[i].pos.z,   // Z
                rx,                 // RX
                ry,                 // RY
                rz,                 // RZ
                planned[i].max_v    // Vel
            };

            auto [result, seq] = bridge_->sendCommand("MoveL", args, 7);
            if (result != RtBridge::SendResult::OK) {
                spdlog::error("[MotionPreprocessor] sendCommand MoveL failed at point {}/{}",
                              i, planned.size());
                return false;
            }
        }

        spdlog::info("[MotionPreprocessor] {} MoveL commands sent, maxVel={}, cornerTol={}",
                     planned.size(), cfg.maxVel, cfg.cornerTol);
        return true;
    }

private:
    RtBridge* bridge_;
    PathPreprocessor pathFitter_;
    VelocityPlanner3D velPlanner_;
};
