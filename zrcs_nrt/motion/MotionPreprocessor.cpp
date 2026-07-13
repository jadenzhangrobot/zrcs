#include "motion/MotionPreprocessor.h"

#include <algorithm>
#include <cmath>

#include <spdlog/spdlog.h>

#include "config/CmdDefine.h"

MotionPreprocessor::MotionPreprocessor(RtBridge* bridge)
    : bridge_(bridge)
{
}

bool MotionPreprocessor::process(const std::vector<Point3D>& waypoints,
                                 double rx,
                                 double ry,
                                 double rz,
                                 const Config& cfg)
{
    if (waypoints.size() < 2)
    {
        spdlog::error("[MotionPreprocessor] Need at least 2 waypoints");
        return false;
    }
    if (!bridge_)
    {
        spdlog::error("[MotionPreprocessor] RtBridge is null");
        return false;
    }

    bridge_->setPathMoveConfig(cfg.maxVel, cfg.maxAccel, cfg.maxJerk);

    auto blocks = makeBlocksFromWaypoints(waypoints, rx, ry, rz, cfg.maxVel);
    auto segments = pathFitter_.fitCornerBlendSegments(blocks, cfg.cornerTol);
    if (segments.empty())
    {
        spdlog::error("[MotionPreprocessor] Path fitting produced no trajectory segments");
        return false;
    }

    velPlanner_.setConfig(cfg.maxVel, cfg.maxAccel, 0.0, 0.0, cfg.cornerTol, cfg.maxJerk);
    if (!velPlanner_.planSegments(segments))
    {
        spdlog::error("[MotionPreprocessor] Segment velocity planning failed");
        return false;
    }

    if (!sendSegmentsAsCommands(segments, rx, ry, rz, cfg))
    {
        return false;
    }

    spdlog::info("[MotionPreprocessor] {} trajectory segments planned, maxVel={}, cornerTol={}",
                 segments.size(), cfg.maxVel, cfg.cornerTol);
    return true;
}

MotionPreprocessor::QuaternionArgs MotionPreprocessor::rpyToQuaternion(double rx, double ry, double rz)
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
    if (std::isfinite(norm) && norm > 0.0)
    {
        q.w /= norm;
        q.x /= norm;
        q.y /= norm;
        q.z /= norm;
    }
    return q;
}

double MotionPreprocessor::segmentVelocityLimit(const TrajectorySegment& segment, double fallback)
{
    double limit = segment.v_max_local > 0.0 ? segment.v_max_local : segment.feedrate_limit;
    if (limit <= 0.0 || !std::isfinite(limit))
    {
        limit = fallback;
    }
    return std::min(limit, fallback);
}

bool MotionPreprocessor::containsNonLineSegment(const std::vector<TrajectorySegment>& segments)
{
    return std::any_of(segments.begin(), segments.end(), [](const TrajectorySegment& segment) {
        return segment.type != TrajectorySegmentType::Line;
    });
}

void MotionPreprocessor::fillMovePathArgs(std::array<double, zrcs::kCmdArgsMax>& args,
                                          const TrajectorySegment& segment,
                                          const QuaternionArgs& quat,
                                          double maxVel,
                                          double sync)
{
    if (segment.type == TrajectorySegmentType::CircularArc)
    {
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
    }
    else
    {
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

bool MotionPreprocessor::argsAreFinite(const std::array<double, zrcs::kCmdArgsMax>& args, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (!std::isfinite(args[i]))
        {
            return false;
        }
    }
    return true;
}

std::vector<PathMoveBlock> MotionPreprocessor::makeBlocksFromWaypoints(
    const std::vector<Point3D>& waypoints,
    double rx,
    double ry,
    double rz,
    double feedrate)
{
    std::vector<PathMoveBlock> blocks;
    blocks.reserve(waypoints.size() - 1);
    for (size_t i = 1; i < waypoints.size(); ++i)
    {
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

bool MotionPreprocessor::sendSegmentsAsMoveL(const std::vector<TrajectorySegment>& segments,
                                             double rx,
                                             double ry,
                                             double rz,
                                             const Config& cfg)
{
    const QuaternionArgs quat = rpyToQuaternion(rx, ry, rz);
    const char* commandName = cfg.galvoMode ? "MoveLGalvo" : "MoveL";
    const size_t argCount = cfg.galvoMode
        ? static_cast<size_t>(MoveLGalvoArg::Sync) + 1
        : static_cast<size_t>(MoveLArg::Sync) + 1;

    for (size_t i = 0; i < segments.size(); ++i)
    {
        const auto& segment = segments[i];
        if (segment.type != TrajectorySegmentType::Line)
        {
            spdlog::error("[MotionPreprocessor] {} cannot execute non-line segment without sampling",
                          commandName);
            return false;
        }

        const Point3D start = evaluateSegment(segment, 0.0);
        const Point3D end = evaluateSegment(segment, 1.0);
        const double segmentMaxVel = segmentVelocityLimit(segment, cfg.maxVel);
        std::array<double, zrcs::kCmdArgsMax> args{};
        if (cfg.galvoMode)
        {
            fillMoveArgs<MoveLGalvoArg>(args, start, end, quat, segmentMaxVel,
                                        segment.v_exit, (i == 0) ? 1.0 : 0.0);
        }
        else
        {
            fillMoveArgs<MoveLArg>(args, start, end, quat, segmentMaxVel,
                                   segment.v_exit, (i == 0) ? 1.0 : 0.0);
        }

        if (!argsAreFinite(args, argCount))
        {
            spdlog::error("[MotionPreprocessor] Non-finite MoveL args at segment {}/{}",
                          i + 1, segments.size());
            return false;
        }

        auto [result, seq] = bridge_->sendCommand(commandName, args.data(), argCount);
        (void)seq;
        if (result != RtBridge::SendResult::OK)
        {
            spdlog::error("[MotionPreprocessor] sendCommand MoveL failed at segment {}/{}",
                          i + 1, segments.size());
            return false;
        }
    }
    return true;
}

bool MotionPreprocessor::sendSegmentsAsMovePath(const std::vector<TrajectorySegment>& segments,
                                                double rx,
                                                double ry,
                                                double rz,
                                                const Config& cfg)
{
    const QuaternionArgs quat = rpyToQuaternion(rx, ry, rz);
    const size_t argCount = static_cast<size_t>(MovePathArg::Sync) + 1;

    for (size_t i = 0; i < segments.size(); ++i)
    {
        const auto& segment = segments[i];
        std::array<double, zrcs::kCmdArgsMax> args{};
        const double maxVel = segmentVelocityLimit(segment, cfg.maxVel);
        fillMovePathArgs(args, segment, quat, maxVel, (i == 0) ? 1.0 : 0.0);

        if (!argsAreFinite(args, argCount))
        {
            spdlog::error("[MotionPreprocessor] Non-finite MovePath args at segment {}/{}",
                          i + 1, segments.size());
            return false;
        }

        auto [result, seq] = bridge_->sendCommand("MovePath", args.data(), argCount);
        (void)seq;
        if (result != RtBridge::SendResult::OK)
        {
            spdlog::error("[MotionPreprocessor] sendCommand MovePath failed at segment {}/{}",
                          i + 1, segments.size());
            return false;
        }
    }
    return true;
}

bool MotionPreprocessor::sendSegmentsAsCommands(const std::vector<TrajectorySegment>& segments,
                                                double rx,
                                                double ry,
                                                double rz,
                                                const Config& cfg)
{
    if (!cfg.galvoMode)
    {
        return sendSegmentsAsMovePath(segments, rx, ry, rz, cfg);
    }
    if (cfg.galvoMode && containsNonLineSegment(segments))
    {
        spdlog::error("[MotionPreprocessor] MoveLGalvo cannot execute blended arc segments without sampling");
        return false;
    }
    return sendSegmentsAsMoveL(segments, rx, ry, rz, cfg);
}
