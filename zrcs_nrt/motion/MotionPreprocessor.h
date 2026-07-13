#pragma once

#include "command/RtBridge.h"
#include "config/CmdDefine.h"
#include "motion/PathPreprocessor.h"
#include "motion/TrajectoryTypes.h"
#include "motion/VelocityPlanner3D.h"
#include "shared_memory/ShmLayout.h"

#include <array>
#include <vector>

class MotionPreprocessor {
public:
    static_assert(static_cast<size_t>(MovePathArg::Sync) + 1 <= zrcs::kCmdArgsMax,
                  "MovePath args must fit Command::args");

    struct Config {
        double maxVel = 10.0;
        double maxAccel = 20.0;
        double maxJerk = 30.0;
        double cornerTol = 0.5;
        bool galvoMode = false;
    };

    explicit MotionPreprocessor(RtBridge* bridge);

    bool process(const std::vector<Point3D>& waypoints,
                 double rx,
                 double ry,
                 double rz,
                 const Config& cfg);

private:
    struct QuaternionArgs {
        double w = 1.0;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    static QuaternionArgs rpyToQuaternion(double rx, double ry, double rz);

    template <typename Arg>
    static void fillMoveArgs(std::array<double, zrcs::kCmdArgsMax>& args,
                             const Point3D& start,
                             const Point3D& end,
                             const QuaternionArgs& quat,
                             double maxVel,
                             double targetVel,
                             double sync);

    static double segmentVelocityLimit(const TrajectorySegment& segment, double fallback);
    static bool containsNonLineSegment(const std::vector<TrajectorySegment>& segments);
    static void fillMovePathArgs(std::array<double, zrcs::kCmdArgsMax>& args,
                                 const TrajectorySegment& segment,
                                 const QuaternionArgs& quat,
                                 double maxVel,
                                 double sync);
    static bool argsAreFinite(const std::array<double, zrcs::kCmdArgsMax>& args, size_t count);
    static std::vector<PathMoveBlock> makeBlocksFromWaypoints(const std::vector<Point3D>& waypoints,
                                                              double rx,
                                                              double ry,
                                                              double rz,
                                                              double feedrate);

    bool sendSegmentsAsMoveL(const std::vector<TrajectorySegment>& segments,
                             double rx,
                             double ry,
                             double rz,
                             const Config& cfg);
    bool sendSegmentsAsMovePath(const std::vector<TrajectorySegment>& segments,
                                double rx,
                                double ry,
                                double rz,
                                const Config& cfg);
    bool sendSegmentsAsCommands(const std::vector<TrajectorySegment>& segments,
                                double rx,
                                double ry,
                                double rz,
                                const Config& cfg);

    RtBridge* bridge_;
    PathPreprocessor pathFitter_;
    VelocityPlanner3D velPlanner_;
};

template <typename Arg>
void MotionPreprocessor::fillMoveArgs(std::array<double, zrcs::kCmdArgsMax>& args,
                                      const Point3D& start,
                                      const Point3D& end,
                                      const QuaternionArgs& quat,
                                      double maxVel,
                                      double targetVel,
                                      double sync)
{
    args[static_cast<size_t>(Arg::CurrentX)] = start.x;
    args[static_cast<size_t>(Arg::CurrentY)] = start.y;
    args[static_cast<size_t>(Arg::CurrentZ)] = start.z;
    args[static_cast<size_t>(Arg::CurrentQ1)] = quat.w;
    args[static_cast<size_t>(Arg::CurrentQ2)] = quat.x;
    args[static_cast<size_t>(Arg::CurrentQ3)] = quat.y;
    args[static_cast<size_t>(Arg::CurrentQ4)] = quat.z;
    args[static_cast<size_t>(Arg::X)] = end.x;
    args[static_cast<size_t>(Arg::Y)] = end.y;
    args[static_cast<size_t>(Arg::Z)] = end.z;
    args[static_cast<size_t>(Arg::Q1)] = quat.w;
    args[static_cast<size_t>(Arg::Q2)] = quat.x;
    args[static_cast<size_t>(Arg::Q3)] = quat.y;
    args[static_cast<size_t>(Arg::Q4)] = quat.z;
    args[static_cast<size_t>(Arg::Vel)] = maxVel;
    args[static_cast<size_t>(Arg::TargetVel)] = targetVel;
    args[static_cast<size_t>(Arg::Sync)] = sync;
}
