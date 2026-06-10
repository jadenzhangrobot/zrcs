#include "config/CmdDefine.h"
#include "motion/PathPreprocessor.h"
#include "motion/MotionPreprocessor.h"
#include "motion/VelocityPlanner3D.h"
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
    const double u2 = u * u;
    const double u3 = u2 * u;
    return {
        command_arg(command, MovePathArg::X0) +
            command_arg(command, MovePathArg::X1) * u +
            command_arg(command, MovePathArg::X2) * u2 +
            command_arg(command, MovePathArg::X3) * u3,
        command_arg(command, MovePathArg::Y0) +
            command_arg(command, MovePathArg::Y1) * u +
            command_arg(command, MovePathArg::Y2) * u2 +
            command_arg(command, MovePathArg::Y3) * u3,
        command_arg(command, MovePathArg::Z0) +
            command_arg(command, MovePathArg::Z1) * u +
            command_arg(command, MovePathArg::Z2) * u2 +
            command_arg(command, MovePathArg::Z3) * u3,
    };
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

template <typename Arg>
void assert_lookahead_commands(const std::vector<zrcs::Command>& commands,
                               CmdId expected_cmd,
                               const std::vector<Point3D>& waypoints,
                               const MotionPreprocessor::Config& cfg)
{
    assert(!commands.empty());

    Point3D previous_target = command_current_point<Arg>(commands.front());
    for (size_t i = 0; i < commands.size(); ++i)
    {
        const auto& command = commands[i];
        assert(command.cmdId == static_cast<uint16_t>(expected_cmd));

        const Point3D current = command_current_point<Arg>(command);
        const Point3D target = command_target_point<Arg>(command);
        if (i == 0)
        {
            assert(is_same_point(current, waypoints.front(), 1e-6));
        }
        else
        {
            assert(is_same_point(current, previous_target, 1e-5));
        }

        assert_quaternion_layout<Arg>(command);

        const double max_vel = command_arg(command, Arg::Vel);
        const double target_vel = command_arg(command, Arg::TargetVel);
        assert(std::isfinite(max_vel));
        assert(std::isfinite(target_vel));
        assert(max_vel > 0.0);
        assert(max_vel <= cfg.maxVel + 1e-9);
        assert(target_vel >= -1e-9);
        assert(target_vel <= max_vel + 1e-9);
        assert(is_near(command_arg(command, Arg::Sync), i == 0 ? 1.0 : 0.0));

        previous_target = target;
    }

    assert(is_same_point(previous_target, waypoints.back(), 1e-5));
    assert(is_near(command_arg(commands.back(), Arg::TargetVel), 0.0, 1e-6));
}

void assert_move_path_commands(const std::vector<zrcs::Command>& commands,
                               const std::vector<Point3D>& waypoints,
                               const MotionPreprocessor::Config& cfg)
{
    assert(!commands.empty());

    Point3D previous_target = command_curve_point(commands.front(), 0.0);
    bool has_cubic_coefficients = false;
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
        assert(std::isfinite(length));
        assert(length > 0.0);
        assert(std::isfinite(max_vel));
        assert(std::isfinite(target_vel));
        assert(max_vel > 0.0);
        assert(max_vel <= cfg.maxVel + 1e-9);
        assert(target_vel >= -1e-9);
        assert(target_vel <= max_vel + 1e-9);
        assert(is_near(command_arg(command, MovePathArg::Sync), i == 0 ? 1.0 : 0.0));

        has_cubic_coefficients = has_cubic_coefficients ||
            std::abs(command_arg(command, MovePathArg::X2)) > 1e-9 ||
            std::abs(command_arg(command, MovePathArg::X3)) > 1e-9 ||
            std::abs(command_arg(command, MovePathArg::Y2)) > 1e-9 ||
            std::abs(command_arg(command, MovePathArg::Y3)) > 1e-9 ||
            std::abs(command_arg(command, MovePathArg::Z2)) > 1e-9 ||
            std::abs(command_arg(command, MovePathArg::Z3)) > 1e-9;

        previous_target = target;
    }

    assert(has_cubic_coefficients);
    assert(is_same_point(previous_target, waypoints.back(), 1e-5));
    assert(is_near(command_arg(commands.back(), MovePathArg::TargetVel), 0.0, 1e-6));
}

template <typename Arg>
void run_motion_preprocessor_queue_test(bool galvo_mode, CmdId expected_cmd)
{
    auto block = std::make_unique<zrcs::SharedBlock>();
    RtBridge bridge(block.get());
    MotionPreprocessor preprocessor(&bridge);

    MotionPreprocessor::Config cfg;
    cfg.maxVel = 5.0;
    cfg.maxAccel = 40.0;
    cfg.maxJerk = 200.0;
    cfg.stepSize = 0.25;
    cfg.cornerTol = 0.2;
    cfg.galvoMode = galvo_mode;

    const std::vector<Point3D> waypoints = {
        {0.0, 0.0, 0.0},
        {4.0, 0.0, 0.0},
    };

    assert(preprocessor.process(waypoints, 0.1, -0.2, 0.3, cfg));
    assert(is_near(block->pathMoveCfg.maxVel.load(std::memory_order_acquire), cfg.maxVel));
    assert(is_near(block->pathMoveCfg.maxAccel.load(std::memory_order_acquire), cfg.maxAccel));
    assert(is_near(block->pathMoveCfg.maxJerk.load(std::memory_order_acquire), cfg.maxJerk));

    const auto commands = collect_commands(*block);
    assert_lookahead_commands<Arg>(commands, expected_cmd, waypoints, cfg);
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

void test_line_block_generates_single_segment()
{
    PathMoveBlock block;
    block.block_id = 42;
    block.start = {0.0, 0.0, 0.0};
    block.end = {10.0, 0.0, 0.0};
    block.feedrate = 5.0;

    PathPreprocessor fitter;
    const auto segments = fitter.fitCornerBlendSegments({block}, 0.1, 0.2);

    assert(segments.size() == 1);
    assert(segments[0].type == TrajectorySegmentType::Line);
    assert(segments[0].source_block_id == 42);
    assert(is_near(segments[0].length, 10.0));
    assert(is_near(segments[0].feedrate_limit, 5.0));
    assert(is_same_point(evaluateSegment(segments[0], 0.0), block.start));
    assert(is_same_point(evaluateSegment(segments[0], 1.0), block.end));
}

void test_corner_blend_fits_butterfly_segments()
{
    const auto raw = make_butterfly_path();
    const auto blocks = make_blocks(raw, 12.0);

    PathPreprocessor fitter;
    const auto segments = fitter.fitCornerBlendSegments(blocks, 0.1, 0.2);

    assert(!segments.empty());
    assert(segments.size() > blocks.size());
    assert_segments_are_continuous(segments);
    assert(is_same_point(evaluateSegment(segments.front(), 0.0), raw.front()));
    assert(is_same_point(evaluateSegment(segments.back(), 1.0), raw.back()));

    bool has_cubic = false;
    for (const auto& segment : segments)
    {
        assert(segment.length > 1e-9);
        assert(std::isfinite(segment.length));
        assert(std::isfinite(segment.max_curvature));
        assert(is_near(segment.feedrate_limit, 12.0));
        has_cubic = has_cubic || segment.type == TrajectorySegmentType::CubicPolynomial;
    }
    assert(has_cubic);

    show_segment_plot(raw, segments);
}

void test_velocity_lookahead_on_segments()
{
    const auto raw = make_butterfly_path();
    const auto blocks = make_blocks(raw, 5.0);

    PathPreprocessor fitter;
    auto segments = fitter.fitCornerBlendSegments(blocks, 0.1, 0.2);

    VelocityPlanner3D planner;
    planner.setConfig(20.0, 40.0, 0.0, 0.0, 1.0, 200.0);
    assert(planner.planSegments(segments));

    bool reached_nonzero_cruise = false;
    for (const auto& segment : segments)
    {
        assert(segment.is_lookahead_optimized);
        assert(std::isfinite(segment.v_max_local));
        assert(std::isfinite(segment.v_enter));
        assert(std::isfinite(segment.v_exit));
        assert(std::isfinite(segment.duration));
        assert(segment.v_max_local <= 5.0 + 1e-9);
        assert(segment.v_enter >= -1e-9);
        assert(segment.v_exit >= -1e-9);
        assert(segment.v_enter <= segment.v_max_local + 1e-9);
        assert(segment.v_exit <= segment.v_max_local + 1e-9);
        assert(segment.duration >= -1e-9);
        reached_nonzero_cruise = reached_nonzero_cruise ||
                                 segment.v_enter > 1.0 ||
                                 segment.v_exit > 1.0;
    }
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

    VelocityPlanner3D planner;
    planner.setConfig(20.0, 16.0, 0.0, 0.0, 1.0, 200.0);
    assert(planner.planSegments(segments));

    assert(is_near(segments[0].v_max_local, 2.0));
    assert(segments[0].is_lookahead_optimized);
    assert(segments[0].duration > 0.0);
}

void test_motion_preprocessor_queues_move_path_commands()
{
    auto block = std::make_unique<zrcs::SharedBlock>();
    RtBridge bridge(block.get());
    MotionPreprocessor preprocessor(&bridge);

    MotionPreprocessor::Config cfg;
    cfg.maxVel = 5.0;
    cfg.maxAccel = 40.0;
    cfg.maxJerk = 200.0;
    cfg.stepSize = 0.25;
    cfg.cornerTol = 0.2;
    cfg.galvoMode = false;

    const std::vector<Point3D> waypoints = {
        {0.0, 0.0, 0.0},
        {4.0, 0.0, 0.0},
        {4.0, 3.0, 0.0},
        {7.0, 3.0, 0.0},
    };

    assert(preprocessor.process(waypoints, 0.1, -0.2, 0.3, cfg));
    assert(is_near(block->pathMoveCfg.maxVel.load(std::memory_order_acquire), cfg.maxVel));
    assert(is_near(block->pathMoveCfg.maxAccel.load(std::memory_order_acquire), cfg.maxAccel));
    assert(is_near(block->pathMoveCfg.maxJerk.load(std::memory_order_acquire), cfg.maxJerk));

    const auto commands = collect_commands(*block);
    assert_move_path_commands(commands, waypoints, cfg);
}

void test_motion_preprocessor_queues_move_l_galvo_commands()
{
    run_motion_preprocessor_queue_test<MoveLGalvoArg>(true, CmdId::MoveLGalvo);
}

} // namespace

int main()
{
    test_line_block_generates_single_segment();
    test_corner_blend_fits_butterfly_segments();
    test_velocity_lookahead_on_segments();
    test_curvature_limits_local_velocity();
    test_motion_preprocessor_queues_move_path_commands();
    test_motion_preprocessor_queues_move_l_galvo_commands();
    std::cout << "Motion preprocessing segment test passed." << std::endl;
    return 0;
}
