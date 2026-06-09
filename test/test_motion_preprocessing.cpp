#include "motion/PathPreprocessor.h"
#include "motion/VelocityPlanner3D.h"

#include <matplot/matplot.h>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

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

bool plots_enabled()
{
    return std::getenv("ZRCS_SKIP_PLOTS") == nullptr;
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

    matplot::figure();
    matplot::plot(raw_x, raw_y, "k.--")->line_width(1.0).marker_size(8);
    matplot::hold(matplot::on);
    matplot::plot(fitted_x, fitted_y, "b-")->line_width(2.0);
    matplot::axis(matplot::equal);
    matplot::grid(matplot::on);
    matplot::xlabel("X");
    matplot::ylabel("Y");
    matplot::title("Segment trajectory corner blend");
    matplot::legend({"raw outline points", "trajectory segments"});
    matplot::show();
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

    matplot::figure();
    matplot::plot(s, planned_v, "b-o")->line_width(2.0).marker_size(5);
    matplot::hold(matplot::on);
    matplot::plot(limit_s, limit_v, "r--")->line_width(1.5);
    matplot::grid(matplot::on);
    matplot::xlabel("Arc length S");
    matplot::ylabel("Velocity");
    matplot::title("Segment velocity lookahead");
    matplot::legend({"planned boundary velocity", "local velocity limit"});
    matplot::show();
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

} // namespace

int main()
{
    test_line_block_generates_single_segment();
    test_corner_blend_fits_butterfly_segments();
    test_velocity_lookahead_on_segments();
    test_curvature_limits_local_velocity();
    std::cout << "Motion preprocessing segment test passed." << std::endl;
    return 0;
}
