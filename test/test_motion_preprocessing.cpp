#include "config/CmdDefine.h"
#include "algorithm/path_planning/MotionPlanner.h"
#include "behavior_tree/nodes/motion/PathMoveSupport.h"
#include "rtBridge/RtBridge.h"
#include "algorithm/path_planning/LookAheadPlanner.h"
#include "shared_memory/ShmLayout.h"

#include <matplotlibcpp.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace plt = matplotlibcpp;

namespace {

bool is_near(double a, double b, double eps = 1e-6)
{
    return std::abs(a - b) <= eps;
}

bool is_same_point(const Point3D& a, const Point3D& b, double eps = 1e-6)
{
    return is_near(a.x, b.x, eps) &&
           is_near(a.y, b.y, eps) &&
           is_near(a.z, b.z, eps);
}

template <typename Arg>
double command_arg(const zrcs::Command& command, Arg arg)
{
    return command.args[static_cast<size_t>(arg)];
}

template <typename Arg>
Point3D command_current_point(const zrcs::Command& command)
{
    return {
        command_arg(command, Arg::CurrentX),
        command_arg(command, Arg::CurrentY),
        command_arg(command, Arg::CurrentZ),
    };
}

template <typename Arg>
Point3D command_target_point(const zrcs::Command& command)
{
    return {
        command_arg(command, Arg::X),
        command_arg(command, Arg::Y),
        command_arg(command, Arg::Z),
    };
}

template <typename Arg>
void assert_quaternion_layout(const zrcs::Command& command)
{
    const double cw = command_arg(command, Arg::CurrentQ1);
    const double cx = command_arg(command, Arg::CurrentQ2);
    const double cy = command_arg(command, Arg::CurrentQ3);
    const double cz = command_arg(command, Arg::CurrentQ4);
    const double tw = command_arg(command, Arg::Q1);
    const double tx = command_arg(command, Arg::Q2);
    const double ty = command_arg(command, Arg::Q3);
    const double tz = command_arg(command, Arg::Q4);

    assert(std::isfinite(cw));
    assert(std::isfinite(cx));
    assert(std::isfinite(cy));
    assert(std::isfinite(cz));
    assert(std::isfinite(tw));
    assert(std::isfinite(tx));
    assert(std::isfinite(ty));
    assert(std::isfinite(tz));

    const double current_norm = std::sqrt(cw * cw + cx * cx + cy * cy + cz * cz);
    const double target_norm = std::sqrt(tw * tw + tx * tx + ty * ty + tz * tz);
    assert(is_near(current_norm, 1.0, 1e-9));
    assert(is_near(target_norm, 1.0, 1e-9));
    assert(is_near(cw, tw, 1e-12));
    assert(is_near(cx, tx, 1e-12));
    assert(is_near(cy, ty, 1e-12));
    assert(is_near(cz, tz, 1e-12));
}

Point3D command_curve_point(const zrcs::Command& command, double u)
{
    u = std::clamp(u, 0.0, 1.0);
    const int shape = static_cast<int>(command_arg(command, MovePathArg::Shape));
    if (shape == 1) {
        const Point3D center = {
            command_arg(command, MovePathArg::P0X),
            command_arg(command, MovePathArg::P0Y),
            command_arg(command, MovePathArg::P0Z),
        };
        const Point3D basisU = {
            command_arg(command, MovePathArg::P1X),
            command_arg(command, MovePathArg::P1Y),
            command_arg(command, MovePathArg::P1Z),
        };
        const Point3D basisV = {
            command_arg(command, MovePathArg::P2X),
            command_arg(command, MovePathArg::P2Y),
            command_arg(command, MovePathArg::P2Z),
        };
        const double radius = command_arg(command, MovePathArg::Radius);
        const double theta = command_arg(command, MovePathArg::Sweep) * u;
        return pointAdd(center,
                        pointScale(pointAdd(pointScale(basisU, std::cos(theta)),
                                            pointScale(basisV, std::sin(theta))),
                                   radius));
    }

    const Point3D start = {
        command_arg(command, MovePathArg::P0X),
        command_arg(command, MovePathArg::P0Y),
        command_arg(command, MovePathArg::P0Z),
    };
    const Point3D end = {
        command_arg(command, MovePathArg::P1X),
        command_arg(command, MovePathArg::P1Y),
        command_arg(command, MovePathArg::P1Z),
    };
    return pointAdd(start, pointScale(pointSub(end, start), u));
}

void assert_curve_quaternion_layout(const zrcs::Command& command)
{
    const double sw = command_arg(command, MovePathArg::QStartW);
    const double sx = command_arg(command, MovePathArg::QStartX);
    const double sy = command_arg(command, MovePathArg::QStartY);
    const double sz = command_arg(command, MovePathArg::QStartZ);
    const double ew = command_arg(command, MovePathArg::QEndW);
    const double ex = command_arg(command, MovePathArg::QEndX);
    const double ey = command_arg(command, MovePathArg::QEndY);
    const double ez = command_arg(command, MovePathArg::QEndZ);

    assert(std::isfinite(sw));
    assert(std::isfinite(sx));
    assert(std::isfinite(sy));
    assert(std::isfinite(sz));
    assert(std::isfinite(ew));
    assert(std::isfinite(ex));
    assert(std::isfinite(ey));
    assert(std::isfinite(ez));

    const double start_norm = std::sqrt(sw * sw + sx * sx + sy * sy + sz * sz);
    const double end_norm = std::sqrt(ew * ew + ex * ex + ey * ey + ez * ez);
    assert(is_near(start_norm, 1.0, 1e-9));
    assert(is_near(end_norm, 1.0, 1e-9));
    assert(is_near(sw, ew, 1e-12));
    assert(is_near(sx, ex, 1e-12));
    assert(is_near(sy, ey, 1e-12));
    assert(is_near(sz, ez, 1e-12));
}

std::vector<zrcs::Command> collect_commands(zrcs::SharedBlock& block)
{
    zrcs::ShmSPSCConsumer<zrcs::Command, zrcs::kCmdQueueCap> consumer(block.cmdQueue);
    std::vector<zrcs::Command> commands;
    zrcs::Command command;
    while (consumer.pop(command))
    {
        commands.push_back(command);
    }
    return commands;
}

void assert_move_path_commands(const std::vector<zrcs::Command>& commands,
                               const std::vector<Point3D>& waypoints,
                               const MotionPlanner::Config& cfg)
{
    assert(!commands.empty());

    Point3D previous_target = command_curve_point(commands.front(), 0.0);
    bool has_arc_segment = false;
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const auto& command = commands[i];
        assert(command.cmdId == static_cast<uint16_t>(CmdId::MovePath));

        for (size_t arg = 0; arg <= static_cast<size_t>(MovePathArg::Sync); ++arg)
        {
            assert(std::isfinite(command.args[arg]));
        }

        const Point3D current = command_curve_point(command, 0.0);
        const Point3D target = command_curve_point(command, 1.0);
        if (i == 0)
        {
            assert(is_same_point(current, waypoints.front(), 1e-6));
        }
        else
        {
            assert(is_same_point(current, previous_target, 1e-5));
        }

        assert_curve_quaternion_layout(command);

        const double length = command_arg(command, MovePathArg::Length);
        const double max_vel = command_arg(command, MovePathArg::Vel);
        const double target_vel = command_arg(command, MovePathArg::TargetVel);
        const double target_acc = command_arg(command, MovePathArg::TargetAcc);
        assert(std::isfinite(length));
        assert(length > 0.0);
        assert(std::isfinite(max_vel));
        assert(std::isfinite(target_vel));
        assert(std::isfinite(target_acc));
        assert(max_vel > 0.0);
        assert(max_vel <= cfg.maxVel + 1e-9);
        assert(target_vel >= -1e-9);
        assert(target_vel <= max_vel + 1e-9);
        assert(std::abs(target_acc) <= cfg.maxAccel + 1e-9);
        assert(is_near(command_arg(command, MovePathArg::Sync), i == 0 ? 1.0 : 0.0));

        has_arc_segment = has_arc_segment ||
            static_cast<int>(command_arg(command, MovePathArg::Shape)) == 1;

        previous_target = target;
    }

    assert(has_arc_segment);
    assert(is_same_point(previous_target, waypoints.back(), 1e-5));
    assert(is_near(command_arg(commands.back(), MovePathArg::TargetVel), 0.0, 1e-6));
    assert(is_near(command_arg(commands.back(), MovePathArg::TargetAcc), 0.0, 1e-6));
}

bool matplotlib_available()
{
    static const bool available = []() {
        if (!Py_IsInitialized())
        {
            Py_Initialize();
        }

        PyObject* module = PyImport_ImportModule("matplotlib");
        if (module != nullptr)
        {
            Py_DECREF(module);
            return true;
        }

        PyErr_Clear();
        return false;
    }();

    return available;
}

std::string ascii_lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string matplotlib_backend()
{
    static const std::string backend = []() {
        if (!matplotlib_available())
        {
            return std::string{};
        }

        PyObject* module = PyImport_ImportModule("matplotlib");
        if (module == nullptr)
        {
            PyErr_Clear();
            return std::string{};
        }

        PyObject* get_backend = PyObject_GetAttrString(module, "get_backend");
        Py_DECREF(module);
        if (get_backend == nullptr)
        {
            PyErr_Clear();
            return std::string{};
        }

        PyObject* result = PyObject_CallObject(get_backend, nullptr);
        Py_DECREF(get_backend);
        if (result == nullptr)
        {
            PyErr_Clear();
            return std::string{};
        }

        std::string name;
        if (const char* value = PyUnicode_AsUTF8(result))
        {
            name = value;
        }
        else
        {
            PyErr_Clear();
        }
        Py_DECREF(result);
        return name;
    }();

    return backend;
}

bool backend_is_noninteractive()
{
    const std::string backend = ascii_lower(matplotlib_backend());
    return backend.find("agg") != std::string::npos ||
           backend.find("pdf") != std::string::npos ||
           backend.find("ps") != std::string::npos ||
           backend.find("svg") != std::string::npos ||
           backend.find("cairo") != std::string::npos ||
           backend.find("inline") != std::string::npos ||
           backend.find("template") != std::string::npos;
}

bool plots_enabled()
{
    if (std::getenv("ZRCS_SKIP_PLOTS") != nullptr)
    {
        return false;
    }

    if (matplotlib_available())
    {
        return true;
    }

    static bool warned = false;
    if (!warned)
    {
        std::cerr << "Skipping plots: Python module matplotlib is not installed." << std::endl;
        warned = true;
    }
    return false;
}

void warn_plot_unavailable(const char* name, const std::exception& error)
{
    std::cerr << "Skipping " << name << " plot: " << error.what() << std::endl;
}

void finish_plot(const char* name, const std::string& filename)
{
    const std::string backend = matplotlib_backend();
    if (!backend.empty() && backend_is_noninteractive())
    {
        plt::save(filename);
        std::cerr << "Saved " << name << " plot to " << filename
                  << " (matplotlib backend: " << backend << ")." << std::endl;
        return;
    }

    plt::show();
}

std::vector<Point3D> make_butterfly_path()
{
    return {
        { 0.0,  4.2, 0.0},
        {-0.8,  3.4, 0.0},
        {-2.4,  4.6, 0.0},
        {-4.5,  5.2, 0.0},
        {-6.2,  4.0, 0.0},
        {-5.2,  2.0, 0.0},
        {-3.4,  0.7, 0.0},
        {-5.5, -1.2, 0.0},
        {-4.5, -3.8, 0.0},
        {-2.4, -3.1, 0.0},
        {-0.8, -1.4, 0.0},
        { 0.0, -3.6, 0.0},
        { 0.8, -1.4, 0.0},
        { 2.4, -3.1, 0.0},
        { 4.5, -3.8, 0.0},
        { 5.5, -1.2, 0.0},
        { 3.4,  0.7, 0.0},
        { 5.2,  2.0, 0.0},
        { 6.2,  4.0, 0.0},
        { 4.5,  5.2, 0.0},
        { 2.4,  4.6, 0.0},
        { 0.8,  3.4, 0.0},
        { 0.0,  4.2, 0.0},
    };
}

std::vector<PathMoveBlock> make_blocks(const std::vector<Point3D>& points, double feedrate)
{
    std::vector<PathMoveBlock> blocks;
    blocks.reserve(points.size() > 0 ? points.size() - 1 : 0);
    for (size_t i = 1; i < points.size(); ++i)
    {
        PathMoveBlock block;
        block.block_id = static_cast<int>(i - 1);
        block.type = PathMoveType::Line;
        block.start = points[i - 1];
        block.end = points[i];
        block.feedrate = feedrate;
        blocks.push_back(block);
    }
    return blocks;
}

std::vector<Point3D> sample_segments(const std::vector<TrajectorySegment>& segments, double step)
{
    std::vector<Point3D> sampled;
    for (const auto& segment : segments)
    {
        const int count = segment.type == TrajectorySegmentType::Line
                        ? 1
                        : std::max(2, static_cast<int>(std::ceil(segment.length / step)));
        if (sampled.empty())
        {
            sampled.push_back(evaluateSegment(segment, 0.0));
        }
        for (int i = 1; i <= count; ++i)
        {
            sampled.push_back(evaluateSegment(segment, static_cast<double>(i) / count));
        }
    }
    return sampled;
}

void show_segment_plot(const std::vector<Point3D>& raw,
                       const std::vector<TrajectorySegment>& segments)
{
    if (!plots_enabled())
    {
        return;
    }

    const auto fitted = sample_segments(segments, 0.1);
    std::vector<double> raw_x;
    std::vector<double> raw_y;
    std::vector<double> fitted_x;
    std::vector<double> fitted_y;

    raw_x.reserve(raw.size());
    raw_y.reserve(raw.size());
    fitted_x.reserve(fitted.size());
    fitted_y.reserve(fitted.size());

    for (const auto& p : raw)
    {
        raw_x.push_back(p.x);
        raw_y.push_back(p.y);
    }
    for (const auto& p : fitted)
    {
        fitted_x.push_back(p.x);
        fitted_y.push_back(p.y);
    }

    try
    {
        plt::figure();
        plt::plot(raw_x, raw_y, {
            {"color", "black"},
            {"linestyle", "--"},
            {"marker", "."},
            {"linewidth", "1.0"},
            {"markersize", "8"},
            {"label", "raw outline points"},
        });
        plt::plot(fitted_x, fitted_y, {
            {"color", "blue"},
            {"linestyle", "-"},
            {"linewidth", "2.0"},
            {"label", "trajectory segments"},
        });
        plt::set_aspect_equal();
        plt::grid(true);
        plt::xlabel("X");
        plt::ylabel("Y");
        plt::title("Segment trajectory corner blend");
        plt::legend();
        finish_plot("segment trajectory", "motion_preprocessing_segments.png");
    }
    catch (const std::exception& error)
    {
        warn_plot_unavailable("segment trajectory", error);
    }
}

void show_velocity_lookahead_plot(const std::vector<TrajectorySegment>& segments)
{
    if (!plots_enabled())
    {
        return;
    }

    std::vector<double> s;
    std::vector<double> planned_v;
    std::vector<double> limit_s;
    std::vector<double> limit_v;

    double arc = 0.0;
    s.push_back(arc);
    planned_v.push_back(segments.front().v_enter);

    for (const auto& segment : segments)
    {
        limit_s.push_back(arc);
        limit_v.push_back(segment.v_max_local);
        arc += segment.length;
        limit_s.push_back(arc);
        limit_v.push_back(segment.v_max_local);

        s.push_back(arc);
        planned_v.push_back(segment.v_exit);
    }

    try
    {
        plt::figure();
        plt::plot(s, planned_v, {
            {"color", "blue"},
            {"linestyle", "-"},
            {"marker", "o"},
            {"linewidth", "2.0"},
            {"markersize", "5"},
            {"label", "planned boundary velocity"},
        });
        plt::plot(limit_s, limit_v, {
            {"color", "red"},
            {"linestyle", "--"},
            {"linewidth", "1.5"},
            {"label", "local velocity limit"},
        });
        plt::grid(true);
        plt::xlabel("Arc length S");
        plt::ylabel("Velocity");
        plt::title("Segment velocity lookahead");
        plt::legend();
        finish_plot("velocity lookahead", "motion_preprocessing_velocity.png");
    }
    catch (const std::exception& error)
    {
        warn_plot_unavailable("velocity lookahead", error);
    }
}

void assert_segments_are_continuous(const std::vector<TrajectorySegment>& segments)
{
    for (size_t i = 1; i < segments.size(); ++i)
    {
        const auto prevEnd = evaluateSegment(segments[i - 1], 1.0);
        const auto currStart = evaluateSegment(segments[i], 0.0);
        assert(is_same_point(prevEnd, currStart, 1e-5));
    }
}

void assert_segment_tangents_are_continuous(const std::vector<TrajectorySegment>& segments)
{
    for (size_t i = 1; i < segments.size(); ++i)
    {
        const Point3D prevTangent = pointNormalize(segmentTangent(segments[i - 1], 1.0));
        const Point3D currTangent = pointNormalize(segmentTangent(segments[i], 0.0));
        assert(pointLength(prevTangent) > 0.0);
        assert(pointLength(currTangent) > 0.0);
        assert(pointDot(prevTangent, currTangent) > 1.0 - 1e-5);
    }
}

void assert_no_internal_waypoint_is_kept_as_hard_corner(
    const std::vector<TrajectorySegment>& segments,
    const std::vector<Point3D>& raw)
{
    for (size_t waypoint = 1; waypoint + 1 < raw.size(); ++waypoint)
    {
        for (size_t i = 1; i < segments.size(); ++i)
        {
            const Point3D junction = evaluateSegment(segments[i], 0.0);
            assert(!is_same_point(junction, raw[waypoint], 1e-5));
        }
    }
}

double integrate_segment_speed_for_test(const TrajectorySegment& segment, double u)
{
    u = std::clamp(u, 0.0, 1.0);
    if (u <= 0.0) {
        return 0.0;
    }

    static constexpr std::array<double, 8> nodes = {
        -0.9602898564975363, -0.7966664774136267,
        -0.5255324099163290, -0.1834346424956498,
         0.1834346424956498,  0.5255324099163290,
         0.7966664774136267,  0.9602898564975363,
    };
    static constexpr std::array<double, 8> weights = {
        0.1012285362903763, 0.2223810344533745,
        0.3137066458778873, 0.3626837833783620,
        0.3626837833783620, 0.3137066458778873,
        0.2223810344533745, 0.1012285362903763,
    };

    const double half = 0.5 * u;
    const double center = 0.5 * u;
    double sum = 0.0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        const double t = center + half * nodes[i];
        sum += weights[i] * pointLength(segmentTangent(segment, t));
    }
    return half * sum;
}

double segment_arc_to_u_for_test(const TrajectorySegment& segment, double localS)
{
    localS = std::clamp(localS, 0.0, segment.length);
    if (segment.length <= 1e-9) {
        return 0.0;
    }
    if (segment.type == TrajectorySegmentType::Line ||
        segment.type == TrajectorySegmentType::CircularArc) {
        return localS / segment.length;
    }

    double lo = 0.0;
    double hi = 1.0;
    for (int iter = 0; iter < 48; ++iter) {
        const double mid = 0.5 * (lo + hi);
        if (integrate_segment_speed_for_test(segment, mid) < localS) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return 0.5 * (lo + hi);
}

Point3D sample_by_global_arc_for_test(const std::vector<TrajectorySegment>& segments, double s)
{
    double offset = 0.0;
    for (const auto& segment : segments) {
        if (s <= offset + segment.length) {
            const double u = segment_arc_to_u_for_test(segment, s - offset);
            return evaluateSegment(segment, u);
        }
        offset += segment.length;
    }
    return evaluateSegment(segments.back(), 1.0);
}

void assert_equal_arc_x_velocity_has_no_single_sample_spikes(
    const std::vector<TrajectorySegment>& segments)
{
    double totalLength = 0.0;
    for (const auto& segment : segments) {
        totalLength += segment.length;
    }

    constexpr double ds = 0.002;
    std::vector<double> vx;
    Point3D prev = sample_by_global_arc_for_test(segments, 0.0);
    for (double s = ds; s <= totalLength; s += ds) {
        const Point3D curr = sample_by_global_arc_for_test(segments, s);
        vx.push_back((curr.x - prev.x) / ds);
        prev = curr;
    }

    for (size_t i = 1; i + 1 < vx.size(); ++i) {
        const double neighborMax = std::max(std::abs(vx[i - 1]), std::abs(vx[i + 1]));
        const double spike = std::abs(vx[i]) - neighborMax;
        assert(spike < 0.05);
    }
}

void test_line_block_generates_single_segment()
{
    PathMoveBlock block;
    block.block_id = 42;
    block.start = {0.0, 0.0, 0.0};
    block.end = {10.0, 0.0, 0.0};
    block.feedrate = 5.0;

    const auto segments = MotionPlanner::buildGeometry({block}, 0.1);

    assert(segments.size() == 1);
    assert(segments[0].type == TrajectorySegmentType::Line);
    assert(segments[0].source_block_id == 42);
    assert(is_near(segments[0].length, 10.0));
    assert(is_near(segments[0].feedrate_limit, 5.0));
    assert(is_same_point(evaluateSegment(segments[0], 0.0), block.start));
    assert(is_same_point(evaluateSegment(segments[0], 1.0), block.end));
}

void test_collinear_waypoints_are_collapsed()
{
    // 近共线密化点应被折叠为更少段（LinuxCNC G64 Q / naive CAM 同类行为）
    const std::vector<Point3D> raw = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {2.0, 0.001, 0.0}, // 弦高约 0.001，容差 0.01 时应折叠
        {3.0, 0.0, 0.0},
        {3.0, 1.0, 0.0},   // 直角，不可折叠
        {4.0, 1.0, 0.0},
    };
    const auto blocks = make_blocks(raw, 5.0);

    // cornerTol 很小避免直角被大幅圆角；collinearTol=0.01 折叠近共线
    const auto segments = MotionPlanner::buildGeometry(blocks, 0.001, 0.01, 0.0005);

    assert_segments_are_continuous(segments);
    assert(is_same_point(evaluateSegment(segments.front(), 0.0), raw.front()));
    assert(is_same_point(evaluateSegment(segments.back(), 1.0), raw.back()));

    // 无折叠时 5 段直线 + 圆角；折叠后共线区变 1 段，总直线段应明显减少
    int lineCount = 0;
    for (const auto& segment : segments) {
        if (segment.type == TrajectorySegmentType::Line) {
            ++lineCount;
            // 不应残留过短微段
            assert(segment.length + 1e-12 >= 0.0005 || segment.length > 0.5);
        }
    }
    assert(lineCount <= 3);
}

void test_corner_blend_keeps_min_line_remainder()
{
    // 密集折线 + 较大 cornerTol 时，残段不得被圆角吃到亚毫米微段
    std::vector<Point3D> raw;
    raw.push_back({0.06, 0.0, 0.1});
    for (int i = 1; i <= 16; ++i) {
        const double a = 2.0 * 3.14159265358979323846 * i / 16.0;
        raw.push_back({0.06 * std::cos(a), 0.06 * std::sin(a), 0.1});
    }
    const auto blocks = make_blocks(raw, 0.15);

    const double cornerTol = 0.003;
    const double minSegLen = 0.0015;
    const auto segments = MotionPlanner::buildGeometry(blocks, cornerTol, cornerTol, minSegLen);

    assert(!segments.empty());
    assert_segments_are_continuous(segments);
    for (const auto& segment : segments) {
        if (segment.type == TrajectorySegmentType::Line) {
            assert(segment.length + 1e-9 >= minSegLen);
        }
    }
}

void test_corner_blend_fits_butterfly_segments()
{
    const auto raw = make_butterfly_path();
    const auto blocks = make_blocks(raw, 12.0);

    // butterfly 测试路径坐标量级约 ±6（任意长度单位）；cornerTol=0.25 同量级。
    const auto segments = MotionPlanner::buildGeometry(blocks, 0.25, 0.25);

    assert(!segments.empty());
    assert(segments.size() > blocks.size());
    assert_segments_are_continuous(segments);
    assert_segment_tangents_are_continuous(segments);
    assert_no_internal_waypoint_is_kept_as_hard_corner(segments, raw);
    assert_equal_arc_x_velocity_has_no_single_sample_spikes(segments);
    assert(is_same_point(evaluateSegment(segments.front(), 0.0), raw.front()));
    assert(is_same_point(evaluateSegment(segments.back(), 1.0), raw.back()));

    bool has_arc = false;
    for (const auto& segment : segments)
    {
        assert(segment.length > 1e-9);
        assert(std::isfinite(segment.length));
        assert(std::isfinite(segment.max_curvature));
        assert(is_near(segment.feedrate_limit, 12.0));
        has_arc = has_arc || segment.type == TrajectorySegmentType::CircularArc;
    }
    assert(has_arc);

    show_segment_plot(raw, segments);
}

void test_velocity_lookahead_on_segments()
{
    const auto raw = make_butterfly_path();
    const auto blocks = make_blocks(raw, 5.0);

    auto segments = MotionPlanner::buildGeometry(blocks, 0.25, 0.25, 0.05);

    LookAheadPlanner planner;
    planner.setConfig(20.0, 40.0, 0.0, 0.0, 1.0, 200.0);
    assert(planner.planSegments(segments));

    bool reached_nonzero_cruise = false;
    for (size_t i = 0; i < segments.size(); ++i)
    {
        const auto& segment = segments[i];
        assert(segment.is_lookahead_optimized);
        assert(std::isfinite(segment.v_max_local));
        assert(std::isfinite(segment.v_enter));
        assert(std::isfinite(segment.v_exit));
        assert(std::isfinite(segment.a_enter));
        assert(std::isfinite(segment.a_exit));
        assert(std::isfinite(segment.duration));
        assert(segment.v_max_local <= 5.0 + 1e-9);
        assert(segment.v_enter >= -1e-9);
        assert(segment.v_exit >= -1e-9);
        assert(segment.v_enter <= segment.v_max_local + 1e-9);
        assert(segment.v_exit <= segment.v_max_local + 1e-9);
        assert(std::abs(segment.a_enter) <= 40.0 + 1e-9);
        assert(std::abs(segment.a_exit) <= 40.0 + 1e-9);
        assert(segment.duration >= -1e-9);
        if (i > 0)
        {
            assert(is_near(segments[i - 1].v_exit, segment.v_enter, 1e-9));
            assert(is_near(segments[i - 1].a_exit, segment.a_enter, 1e-9));
        }
        reached_nonzero_cruise = reached_nonzero_cruise ||
                                 segment.v_enter > 1.0 ||
                                 segment.v_exit > 1.0;
    }
    assert(is_near(segments.front().a_enter, 0.0, 1e-9));
    assert(is_near(segments.back().a_exit, 0.0, 1e-9));
    assert(reached_nonzero_cruise);

    show_velocity_lookahead_plot(segments);
}

void test_curvature_limits_local_velocity()
{
    TrajectorySegment segment;
    segment.segment_id = 1;
    segment.type = TrajectorySegmentType::Line;
    segment.length = 10.0;
    segment.feedrate_limit = 20.0;
    segment.max_curvature = 4.0;
    segment.coeff[0][1] = 10.0;

    std::vector<TrajectorySegment> segments = {segment};

    LookAheadPlanner planner;
    planner.setConfig(20.0, 16.0, 0.0, 0.0, 1.0, 200.0);
    assert(planner.planSegments(segments));

    assert(is_near(segments[0].v_max_local, 2.0));
    assert(segments[0].is_lookahead_optimized);
    assert(segments[0].duration > 0.0);
}

void test_acceleration_lookahead_spans_segments()
{
    constexpr double maxVel = 50.0;
    constexpr double maxAccel = 10.0;
    constexpr double maxJerk = 100.0;
    std::vector<TrajectorySegment> segments;
    for (int i = 0; i < 4; ++i)
    {
        TrajectorySegment segment;
        segment.segment_id = i;
        segment.type = TrajectorySegmentType::Line;
        segment.length = 5.0;
        segment.feedrate_limit = 50.0;
        segment.coeff[0][0] = static_cast<double>(i) * segment.length;
        segment.coeff[0][1] = segment.length;
        segments.push_back(segment);
    }

    LookAheadPlanner planner;
    planner.setConfig(maxVel, maxAccel, 0.0, 0.0, 0.5, maxJerk);
    assert(planner.planSegments(segments));

    bool has_nonzero_junction_acceleration = false;
    assert(is_near(segments.front().a_enter, 0.0, 1e-9));
    assert(is_near(segments.back().a_exit, 0.0, 1e-9));
    for (size_t i = 0; i < segments.size(); ++i)
    {
        assert(std::abs(segments[i].a_enter) <= 10.0 + 1e-9);
        assert(std::abs(segments[i].a_exit) <= 10.0 + 1e-9);
        assert(segments[i].duration > 0.0);
        if (i > 0)
        {
            assert(is_near(segments[i - 1].v_exit, segments[i].v_enter, 1e-9));
            assert(is_near(segments[i - 1].a_exit, segments[i].a_enter, 1e-9));
        }
        if (i + 1 < segments.size() && std::abs(segments[i].a_exit) > 1e-6)
        {
            has_nonzero_junction_acceleration = true;
        }
    }
    assert(has_nonzero_junction_acceleration);

    auto block = std::make_unique<zrcs::SharedBlock>();
    RtBridge bridge(block.get());
    MotionPlanner::Config cfg;
    cfg.maxVel = maxVel;
    cfg.maxAccel = maxAccel;
    cfg.maxJerk = maxJerk;
    assert(zrcs_bt::queuePlannedSegments(&bridge, segments,
                                         0.0, 0.0, 0.0,
                                         0.0, 0.0, 0.0,
                                         cfg));

    const auto commands = collect_commands(*block);
    assert(commands.size() == segments.size());
    for (size_t i = 0; i < commands.size(); ++i)
    {
        assert(is_near(command_arg(commands[i], MovePathArg::TargetVel),
                       segments[i].v_exit, 1e-9));
        assert(is_near(command_arg(commands[i], MovePathArg::TargetAcc),
                       segments[i].a_exit, 1e-9));
    }
}

void test_motion_preprocessor_queues_move_path_commands()
{
    auto block = std::make_unique<zrcs::SharedBlock>();
    RtBridge bridge(block.get());
    MotionPlanner::Config cfg;
    assert(is_near(cfg.cornerTol, 0.00025));
    assert(is_near(cfg.minSegLen, 0.00005));
    cfg.maxVel = 5.0;
    cfg.maxAccel = 40.0;
    cfg.maxJerk = 200.0;
    cfg.cornerTol = 0.2;

    const std::vector<Point3D> waypoints = {
        {0.0, 0.0, 0.0},
        {4.0, 0.0, 0.0},
        {4.0, 3.0, 0.0},
        {7.0, 3.0, 0.0},
    };

    assert(zrcs_bt::queuePathFromWaypoints(&bridge, waypoints, 0.1, -0.2, 0.3,
                                           0.1, -0.2, 0.3, cfg));
    assert(is_near(block->pathMoveCfg.maxVel.load(std::memory_order_acquire), cfg.maxVel));
    assert(is_near(block->pathMoveCfg.maxAccel.load(std::memory_order_acquire), cfg.maxAccel));
    assert(is_near(block->pathMoveCfg.maxJerk.load(std::memory_order_acquire), cfg.maxJerk));

    const auto commands = collect_commands(*block);
    assert_move_path_commands(commands, waypoints, cfg);
}

void test_pointwise_xyzac_orientations_are_queued()
{
    auto block = std::make_unique<zrcs::SharedBlock>();
    RtBridge bridge(block.get());
    MotionPlanner::Config cfg;
    cfg.cornerTol = 0.0;
    cfg.minSegLen = 0.0;

    const std::vector<Point3D> waypoints = {
        {0.0, 0.0, 0.1},
        {0.01, 0.0, 0.1},
        {0.02, 0.01, 0.1},
    };
    const std::vector<PathOrientation> orientations = {
        {0.10, 0.0, -1.50},
        {0.20, 0.0, -1.00},
        {0.30, 0.0, 0.20},
    };

    assert(zrcs_bt::queuePathFromWaypoints(
        &bridge, waypoints, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        cfg, nullptr, &orientations));
    const auto commands = collect_commands(*block);
    assert(commands.size() == 2);

    auto quaternion = [](const PathOrientation& rpy) {
        const double cr = std::cos(rpy.rx * 0.5);
        const double sr = std::sin(rpy.rx * 0.5);
        const double cp = std::cos(rpy.ry * 0.5);
        const double sp = std::sin(rpy.ry * 0.5);
        const double cy = std::cos(rpy.rz * 0.5);
        const double sy = std::sin(rpy.rz * 0.5);
        return std::array<double, 4>{
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
        };
    };

    for (size_t i = 0; i < commands.size(); ++i) {
        const auto expectedStart = quaternion(orientations[i]);
        const auto expectedEnd = quaternion(orientations[i + 1]);
        const std::array<MovePathArg, 4> startArgs = {
            MovePathArg::QStartW, MovePathArg::QStartX,
            MovePathArg::QStartY, MovePathArg::QStartZ,
        };
        const std::array<MovePathArg, 4> endArgs = {
            MovePathArg::QEndW, MovePathArg::QEndX,
            MovePathArg::QEndY, MovePathArg::QEndZ,
        };
        for (size_t q = 0; q < 4; ++q) {
            assert(is_near(command_arg(commands[i], startArgs[q]), expectedStart[q]));
            assert(is_near(command_arg(commands[i], endArgs[q]), expectedEnd[q]));
        }
    }
}

} // namespace

int main()
{
    test_line_block_generates_single_segment();
    test_collinear_waypoints_are_collapsed();
    test_corner_blend_keeps_min_line_remainder();
    test_corner_blend_fits_butterfly_segments();
    test_velocity_lookahead_on_segments();
    test_curvature_limits_local_velocity();
    test_acceleration_lookahead_spans_segments();
    test_motion_preprocessor_queues_move_path_commands();
    test_pointwise_xyzac_orientations_are_queued();
    std::cout << "Motion preprocessing segment test passed." << std::endl;
    return 0;
}
