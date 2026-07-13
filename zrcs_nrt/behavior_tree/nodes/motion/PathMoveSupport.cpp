#include "behavior_tree/nodes/motion/PathMoveSupport.h"

#include <array>
#include <cmath>

#include <spdlog/spdlog.h>

#include "config/CmdDefine.h"
#include "rtBridge/RtBridge.h"
#include "shared_memory/ShmLayout.h"

namespace zrcs_bt {
namespace {

static_assert(static_cast<size_t>(MovePathArg::Sync) + 1 <= zrcs::kCmdArgsMax,
              "MovePath args must fit Command::args");

struct QuaternionArgs {
    double w = 1.0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

QuaternionArgs rpyToQuaternion(double rx, double ry, double rz)
{
    const double cr = std::cos(rx * 0.5);
    const double sr = std::sin(rx * 0.5);
    const double cp = std::cos(ry * 0.5);
    const double sp = std::sin(ry * 0.5);
    const double cy = std::cos(rz * 0.5);
    const double sy = std::sin(rz * 0.5);

    QuaternionArgs q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;

    const double norm = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (std::isfinite(norm) && norm > 0.0) {
        q.w /= norm;
        q.x /= norm;
        q.y /= norm;
        q.z /= norm;
    }
    return q;
}

double segmentVelocityLimit(const TrajectorySegment& segment, double fallback)
{
    double limit = segment.v_max_local > 0.0 ? segment.v_max_local : segment.feedrate_limit;
    if (limit <= 0.0 || !std::isfinite(limit)) {
        limit = fallback;
    }
    return std::min(limit, fallback);
}

void fillMovePathArgs(std::array<double, zrcs::kCmdArgsMax>& args,
                      const TrajectorySegment& segment,
                      const QuaternionArgs& quat,
                      double maxVel,
                      double sync)
{
    if (segment.type == TrajectorySegmentType::CircularArc) {
        args[static_cast<size_t>(MovePathArg::Shape)] = 1.0;
        args[static_cast<size_t>(MovePathArg::P0X)] = segment.arc_center.x;
        args[static_cast<size_t>(MovePathArg::P0Y)] = segment.arc_center.y;
        args[static_cast<size_t>(MovePathArg::P0Z)] = segment.arc_center.z;
        args[static_cast<size_t>(MovePathArg::P1X)] = segment.arc_u.x;
        args[static_cast<size_t>(MovePathArg::P1Y)] = segment.arc_u.y;
        args[static_cast<size_t>(MovePathArg::P1Z)] = segment.arc_u.z;
        args[static_cast<size_t>(MovePathArg::P2X)] = segment.arc_v.x;
        args[static_cast<size_t>(MovePathArg::P2Y)] = segment.arc_v.y;
        args[static_cast<size_t>(MovePathArg::P2Z)] = segment.arc_v.z;
        args[static_cast<size_t>(MovePathArg::Radius)] = segment.arc_radius;
        args[static_cast<size_t>(MovePathArg::Sweep)] = segment.arc_sweep;
    } else {
        const Point3D start = evaluateSegment(segment, 0.0);
        const Point3D end = evaluateSegment(segment, 1.0);
        args[static_cast<size_t>(MovePathArg::Shape)] = 0.0;
        args[static_cast<size_t>(MovePathArg::P0X)] = start.x;
        args[static_cast<size_t>(MovePathArg::P0Y)] = start.y;
        args[static_cast<size_t>(MovePathArg::P0Z)] = start.z;
        args[static_cast<size_t>(MovePathArg::P1X)] = end.x;
        args[static_cast<size_t>(MovePathArg::P1Y)] = end.y;
        args[static_cast<size_t>(MovePathArg::P1Z)] = end.z;
    }
    args[static_cast<size_t>(MovePathArg::QStartW)] = quat.w;
    args[static_cast<size_t>(MovePathArg::QStartX)] = quat.x;
    args[static_cast<size_t>(MovePathArg::QStartY)] = quat.y;
    args[static_cast<size_t>(MovePathArg::QStartZ)] = quat.z;
    args[static_cast<size_t>(MovePathArg::QEndW)] = quat.w;
    args[static_cast<size_t>(MovePathArg::QEndX)] = quat.x;
    args[static_cast<size_t>(MovePathArg::QEndY)] = quat.y;
    args[static_cast<size_t>(MovePathArg::QEndZ)] = quat.z;
    args[static_cast<size_t>(MovePathArg::Length)] = segment.length;
    args[static_cast<size_t>(MovePathArg::Vel)] = maxVel;
    args[static_cast<size_t>(MovePathArg::TargetVel)] = segment.v_exit;
    args[static_cast<size_t>(MovePathArg::Sync)] = sync;
}

bool argsAreFinite(const std::array<double, zrcs::kCmdArgsMax>& args, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (!std::isfinite(args[i])) {
            return false;
        }
    }
    return true;
}

bool sendSegmentsAsMovePath(RtBridge* bridge,
                            const std::vector<TrajectorySegment>& segments,
                            double rx,
                            double ry,
                            double rz,
                            const MotionPlanner::Config& cfg)
{
    const QuaternionArgs quat = rpyToQuaternion(rx, ry, rz);
    const size_t argCount = static_cast<size_t>(MovePathArg::Sync) + 1;

    for (size_t i = 0; i < segments.size(); ++i) {
        const auto& segment = segments[i];
        std::array<double, zrcs::kCmdArgsMax> args{};
        const double maxVel = segmentVelocityLimit(segment, cfg.maxVel);
        fillMovePathArgs(args, segment, quat, maxVel, (i == 0) ? 1.0 : 0.0);

        if (!argsAreFinite(args, argCount)) {
            spdlog::error("[PathMove] Non-finite MovePath args at segment {}/{}",
                          i + 1, segments.size());
            return false;
        }

        auto [result, seq] = bridge->sendCommand("MovePath", args.data(), argCount);
        (void)seq;
        if (result != RtBridge::SendResult::OK) {
            spdlog::error("[PathMove] sendCommand MovePath failed at segment {}/{}",
                          i + 1, segments.size());
            return false;
        }
    }
    return true;
}

} // namespace

bool queuePlannedSegments(RtBridge* bridge,
                          const std::vector<TrajectorySegment>& segments,
                          double rx,
                          double ry,
                          double rz,
                          const MotionPlanner::Config& cfg)
{
    if (!bridge) {
        spdlog::error("[PathMove] RtBridge is null");
        return false;
    }
    bridge->setPathMoveConfig(cfg.maxVel, cfg.maxAccel, cfg.maxJerk);
    return sendSegmentsAsMovePath(bridge, segments, rx, ry, rz, cfg);
}

bool queuePathFromWaypoints(RtBridge* bridge,
                            const std::vector<Point3D>& waypoints,
                            double rx,
                            double ry,
                            double rz,
                            const MotionPlanner::Config& cfg)
{
    if (!bridge) {
        spdlog::error("[PathMove] RtBridge is null");
        return false;
    }

    MotionPlanner planner;
    std::vector<TrajectorySegment> segments;
    std::string error;
    if (!planner.plan(waypoints, rx, ry, rz, cfg, segments, &error)) {
        spdlog::error("[PathMove] {}", error);
        return false;
    }

    if (!queuePlannedSegments(bridge, segments, rx, ry, rz, cfg)) {
        return false;
    }

    spdlog::info("[PathMove] {} trajectory segments planned, maxVel={}, cornerTol={}",
                 segments.size(), cfg.maxVel, cfg.cornerTol);
    return true;
}

} // namespace zrcs_bt
