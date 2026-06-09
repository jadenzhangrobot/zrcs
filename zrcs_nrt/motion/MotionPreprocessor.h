#pragma once

#include "motion/PathPreprocessor.h"
#include "motion/VelocityPlanner3D.h"
#include "rtBridge/RtBridge.h"
#include "shared_memory/ShmLayout.h"

#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <vector>

class MotionPreprocessor {
public:
    struct Config {
        double maxVel = 10.0;
        double maxAccel = 20.0;
        double maxJerk = 30.0;
        double stepSize = 1.0;
        double cornerTol = 0.5;
        bool galvoMode = false;
    };

    explicit MotionPreprocessor(RtBridge* bridge)
        : bridge_(bridge)
    {
    }

    bool process(const std::vector<Point3D>& waypoints,
                 double rx,
                 double ry,
                 double rz,
                 const Config& cfg)
    {
        if (waypoints.size() < 2) {
            spdlog::error("[MotionPreprocessor] Need at least 2 waypoints");
            return false;
        }
        if (!bridge_) {
            spdlog::error("[MotionPreprocessor] RtBridge is null");
            return false;
        }

        bridge_->setPathMoveConfig(cfg.maxVel, cfg.maxAccel, cfg.maxJerk);

        auto blocks = makeBlocksFromWaypoints(waypoints, rx, ry, rz, cfg.maxVel);
        auto segments = pathFitter_.fitCornerBlendSegments(blocks, cfg.stepSize, cfg.cornerTol);
        if (segments.empty()) {
            spdlog::error("[MotionPreprocessor] Path fitting produced no trajectory segments");
            return false;
        }

        velPlanner_.setConfig(cfg.maxVel, cfg.maxAccel, 0.0, 0.0, cfg.cornerTol, cfg.maxJerk);
        if (!velPlanner_.planSegments(segments)) {
            spdlog::error("[MotionPreprocessor] Segment velocity planning failed");
            return false;
        }

        if (!sendSegmentsAsMoveL(segments, rx, ry, rz, cfg)) {
            return false;
        }

        spdlog::info("[MotionPreprocessor] {} trajectory segments planned, maxVel={}, cornerTol={}",
                     segments.size(), cfg.maxVel, cfg.cornerTol);
        return true;
    }

private:
    struct RuntimeMove {
        Point3D start;
        Point3D end;
        double maxVel = 0.0;
        double targetVel = 0.0;
    };

    static std::vector<PathMoveBlock> makeBlocksFromWaypoints(const std::vector<Point3D>& waypoints,
                                                              double rx,
                                                              double ry,
                                                              double rz,
                                                              double feedrate)
    {
        std::vector<PathMoveBlock> blocks;
        blocks.reserve(waypoints.size() - 1);
        for (size_t i = 1; i < waypoints.size(); ++i) {
            PathMoveBlock block;
            block.block_id = static_cast<int>(i - 1);
            block.type = PathMoveType::Line;
            block.start = waypoints[i - 1];
            block.end = waypoints[i];
            block.rx = rx;
            block.ry = ry;
            block.rz = rz;
            block.feedrate = feedrate;
            blocks.push_back(block);
        }
        return blocks;
    }

    std::vector<RuntimeMove> buildRuntimeMoves(const std::vector<TrajectorySegment>& segments,
                                               double sampleStep) const
    {
        std::vector<RuntimeMove> moves;
        if (segments.empty()) {
            return moves;
        }

        Point3D current = evaluateSegment(segments.front(), 0.0);
        for (const auto& segment : segments) {
            const int sampleCount = segment.type == TrajectorySegmentType::Line
                                  ? 1
                                  : std::max(1, static_cast<int>(std::ceil(segment.length / std::max(sampleStep, 1e-6))));
            for (int i = 1; i <= sampleCount; ++i) {
                const double u = static_cast<double>(i) / sampleCount;
                const Point3D next = evaluateSegment(segment, u);
                RuntimeMove move;
                move.start = current;
                move.end = next;
                move.maxVel = segment.v_max_local > 0.0 ? segment.v_max_local : segment.feedrate_limit;
                move.targetVel = segment.v_enter + (segment.v_exit - segment.v_enter) * u;
                moves.push_back(move);
                current = next;
            }
        }
        return moves;
    }

    bool sendSegmentsAsMoveL(const std::vector<TrajectorySegment>& segments,
                             double rx,
                             double ry,
                             double rz,
                             const Config& cfg)
    {
        const auto moves = buildRuntimeMoves(segments, cfg.stepSize);
        for (size_t i = 0; i < moves.size(); ++i) {
            const auto& move = moves[i];
            const double segmentMaxVel = move.maxVel > 0.0 ? move.maxVel : cfg.maxVel;

            double args[] = {
                move.start.x,
                move.start.y,
                move.start.z,
                rx,
                ry,
                rz,
                move.end.x,
                move.end.y,
                move.end.z,
                rx,
                ry,
                rz,
                segmentMaxVel,
                move.targetVel,
                (i == 0) ? 1.0 : 0.0,
            };

            auto [result, seq] = bridge_->sendCommand(cfg.galvoMode ? "MoveLGalvo" : "MoveL", args, 15);
            (void)seq;
            if (result != RtBridge::SendResult::OK) {
                spdlog::error("[MotionPreprocessor] sendCommand MoveL failed at segment {}/{}",
                              i + 1, moves.size());
                return false;
            }
        }
        return true;
    }

    RtBridge* bridge_;
    PathPreprocessor pathFitter_;
    VelocityPlanner3D velPlanner_;
};
