#include "algorithm/path_planning/MotionPlanner.h"
#include "core/nc_parser/NcParser.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

bool near(double actual, double expected, double tolerance = 1e-10)
{
    return std::abs(actual - expected) <= tolerance;
}

void test_rotary_words_are_parsed_as_degrees()
{
    const auto result = NcParser{}.parseText(
        "G21 G90\n"
        "G1 X1 Y2 Z3 A10 B20 C30 F5\n"
        "G1 X2 A15 C35\n"
        "G91\n"
        "G1 X1 A2 C-3\n");

    assert(result.error.empty());
    assert(result.waypoints.size() == 3);
    assert(result.orientations.size() == result.waypoints.size());
    assert(near(result.orientations[0].rx, 10.0 * kPi / 180.0));
    assert(near(result.orientations[0].ry, 20.0 * kPi / 180.0));
    assert(near(result.orientations[0].rz, 30.0 * kPi / 180.0));
    assert(near(result.orientations[1].rx, 15.0 * kPi / 180.0));
    assert(near(result.orientations[1].rz, 35.0 * kPi / 180.0));
    assert(near(result.orientations[2].rx, 17.0 * kPi / 180.0));
    assert(near(result.orientations[2].rz, 32.0 * kPi / 180.0));
}

void test_butterfly_is_pointwise_xyzac_on_sphere()
{
    const std::string path =
        std::string(ZRCS_SOURCE_DIR) + "/config/5axis/program/maple.nc";
    const auto result = NcParser{}.parseFile(path);
    assert(result.error.empty());
    assert(result.waypoints.size() == 309);
    assert(result.orientations.size() == result.waypoints.size());
    assert(result.segmentFeedrates.size() + 1 == result.waypoints.size());

    double maxNormalError = 0.0;
    assert(near(result.waypoints.front().z, 0.170));
    assert(near(result.orientations.front().rx, 0.0));
    assert(near(result.orientations.back().rz, 2.0 * kPi, 1e-8));

    // Skip home/safe approach and safe/home retract points.
    for (size_t i = 2; i + 2 < result.waypoints.size(); ++i) {
        const auto& point = result.waypoints[i];
        const auto& orientation = result.orientations[i];
        const double nx = point.x / 0.040;
        const double ny = point.y / 0.040;
        const double nz = (point.z - 0.070) / 0.040;

        const double xAfterC = nx * std::cos(orientation.rz) -
                               ny * std::sin(orientation.rz);
        const double yAfterC = nx * std::sin(orientation.rz) +
                               ny * std::cos(orientation.rz);
        const double yAfterA = yAfterC * std::cos(orientation.rx) -
                               nz * std::sin(orientation.rx);
        const double zAfterA = yAfterC * std::sin(orientation.rx) +
                               nz * std::cos(orientation.rx);
        const double normalError = std::sqrt(
            xAfterC * xAfterC + yAfterA * yAfterA +
            (zAfterA - 1.0) * (zAfterA - 1.0));
        maxNormalError = std::max(maxNormalError, normalError);
    }
    assert(maxNormalError < 2e-8);

    // Verify the complete table-table IK envelope, including approach/retract.
    for (size_t i = 0; i < result.waypoints.size(); ++i) {
        const auto& point = result.waypoints[i];
        const auto& orientation = result.orientations[i];
        const double xAfterC = point.x * std::cos(orientation.rz) -
                               point.y * std::sin(orientation.rz);
        const double yAfterC = point.x * std::sin(orientation.rz) +
                               point.y * std::cos(orientation.rz);
        const double jointX = xAfterC;
        const double jointY = yAfterC * std::cos(orientation.rx) -
                              (point.z + 0.050) * std::sin(orientation.rx);
        const double jointZ = 0.380 + yAfterC * std::sin(orientation.rx) +
                              (point.z + 0.050) * std::cos(orientation.rx) - 0.600;
        assert(jointX >= -0.400 && jointX <= 0.400);
        assert(jointY >= -0.300 && jointY <= 0.300);
        assert(jointZ >= -0.180 && jointZ <= 0.400);
        assert(orientation.rx >= -1.920 && orientation.rx <= 1.920);
        assert(orientation.rz >= -6.2832 && orientation.rz <= 6.2832);
    }

    // Pointwise A/C is linearly interpolated by RT. Check the complete cutting
    // edges, not only their exact spherical endpoints.
    double maxSurfaceError = 0.0;
    double maxInterpolatedNormalError = 0.0;
    for (size_t i = 3; i + 2 < result.waypoints.size(); ++i) {
        const auto& start = result.waypoints[i - 1];
        const auto& end = result.waypoints[i];
        const auto& startOrientation = result.orientations[i - 1];
        const auto& endOrientation = result.orientations[i];
        for (int sample = 0; sample <= 10; ++sample) {
            const double t = static_cast<double>(sample) / 10.0;
            const Point3D point{
                start.x + (end.x - start.x) * t,
                start.y + (end.y - start.y) * t,
                start.z + (end.z - start.z) * t,
            };
            const PathOrientation orientation{
                startOrientation.rx + (endOrientation.rx - startOrientation.rx) * t,
                0.0,
                startOrientation.rz + (endOrientation.rz - startOrientation.rz) * t,
            };
            const Point3D radial{point.x, point.y, point.z - 0.070};
            const double radialLength = pointLength(radial);
            maxSurfaceError = std::max(maxSurfaceError,
                                       std::abs(radialLength - 0.040));
            const Point3D normal = pointScale(radial, 1.0 / radialLength);
            const double xAfterC = normal.x * std::cos(orientation.rz) -
                                   normal.y * std::sin(orientation.rz);
            const double yAfterC = normal.x * std::sin(orientation.rz) +
                                   normal.y * std::cos(orientation.rz);
            const double yAfterA = yAfterC * std::cos(orientation.rx) -
                                   normal.z * std::sin(orientation.rx);
            const double zAfterA = yAfterC * std::sin(orientation.rx) +
                                   normal.z * std::cos(orientation.rx);
            maxInterpolatedNormalError = std::max(
                maxInterpolatedNormalError,
                std::sqrt(xAfterC * xAfterC + yAfterA * yAfterA +
                          (zAfterA - 1.0) * (zAfterA - 1.0)));
        }
    }
    assert(maxSurfaceError < 1e-5);              // < 0.010 mm
    assert(maxInterpolatedNormalError < 7e-4);   // < 0.041 deg

    MotionPlanner::Config cfg;
    cfg.maxVel = 0.05;
    cfg.maxAccel = 0.5;
    cfg.maxJerk = 5.0;
    cfg.cornerTol = 0.0;
    cfg.minSegLen = 0.0;

    MotionPlanner planner;
    std::vector<TrajectorySegment> segments;
    std::string error;
    assert(planner.plan(result.waypoints, 0.0, 0.0, 0.0, cfg, segments,
                        &error, &result.segmentFeedrates));
    assert(segments.size() + 1 == result.waypoints.size());
}

void test_sphere_hex_grid_is_continuous_xyzac()
{
    const std::string path =
        std::string(ZRCS_SOURCE_DIR) + "/config/5axis/program/sphere_hex_grid.nc";
    const auto result = NcParser{}.parseFile(path);
    assert(result.error.empty());
    assert(result.waypoints.size() == 6647);
    assert(result.orientations.size() == result.waypoints.size());
    assert(result.segmentFeedrates.size() + 1 == result.waypoints.size());

    const size_t firstCut = 2;
    const size_t lastCut = result.waypoints.size() - 3;
    double maxPolarAngle = 0.0;
    double maxSurfaceError = 0.0;
    double maxNormalError = 0.0;
    double maxRotaryVelocityA = 0.0;
    double maxRotaryVelocityC = 0.0;

    for (size_t i = firstCut; i <= lastCut; ++i) {
        const auto& point = result.waypoints[i];
        const auto& orientation = result.orientations[i];
        const Point3D radial{point.x, point.y, point.z - 0.070};
        const double radialLength = pointLength(radial);
        const Point3D normal = pointScale(radial, 1.0 / radialLength);
        maxSurfaceError = std::max(maxSurfaceError,
                                   std::abs(radialLength - 0.040));
        maxPolarAngle = std::max(
            maxPolarAngle,
            std::acos(std::clamp(normal.z, -1.0, 1.0)));

        const double xAfterC = normal.x * std::cos(orientation.rz) -
                               normal.y * std::sin(orientation.rz);
        const double yAfterC = normal.x * std::sin(orientation.rz) +
                               normal.y * std::cos(orientation.rz);
        const double yAfterA = yAfterC * std::cos(orientation.rx) -
                               normal.z * std::sin(orientation.rx);
        const double zAfterA = yAfterC * std::sin(orientation.rx) +
                               normal.z * std::cos(orientation.rx);
        maxNormalError = std::max(
            maxNormalError,
            std::sqrt(xAfterC * xAfterC + yAfterA * yAfterA +
                      (zAfterA - 1.0) * (zAfterA - 1.0)));

        const double jointX = point.x * std::cos(orientation.rz) -
                              point.y * std::sin(orientation.rz);
        const double rotatedY = point.x * std::sin(orientation.rz) +
                                point.y * std::cos(orientation.rz);
        const double jointY = rotatedY * std::cos(orientation.rx) -
                              (point.z + 0.050) * std::sin(orientation.rx);
        const double jointZ = 0.380 + rotatedY * std::sin(orientation.rx) +
                              (point.z + 0.050) * std::cos(orientation.rx) - 0.600;
        assert(jointX >= -0.400 && jointX <= 0.400);
        assert(jointY >= -0.300 && jointY <= 0.300);
        assert(jointZ >= -0.180 && jointZ <= 0.400);
        assert(orientation.rx >= -1.920 && orientation.rx <= 1.920);
        assert(orientation.rz >= -6.2832 && orientation.rz <= 6.2832);

        if (i == firstCut) {
            continue;
        }

        const double segmentLength = pointDistance(result.waypoints[i - 1], point);
        assert(segmentLength > 1e-9);
        assert(segmentLength <= 0.000501);
        const double feed = result.segmentFeedrates[i - 1];
        assert(near(feed, 0.005));
        const double duration = segmentLength / feed;
        maxRotaryVelocityA = std::max(
            maxRotaryVelocityA,
            std::abs(orientation.rx - result.orientations[i - 1].rx) / duration);
        maxRotaryVelocityC = std::max(
            maxRotaryVelocityC,
            std::abs(orientation.rz - result.orientations[i - 1].rz) / duration);

        const auto& start = result.waypoints[i - 1];
        const auto& startOrientation = result.orientations[i - 1];
        const Point3D midpoint{
            0.5 * (start.x + point.x),
            0.5 * (start.y + point.y),
            0.5 * (start.z + point.z),
        };
        const PathOrientation midpointOrientation{
            0.5 * (startOrientation.rx + orientation.rx),
            0.0,
            0.5 * (startOrientation.rz + orientation.rz),
        };
        const Point3D midpointRadial{midpoint.x, midpoint.y, midpoint.z - 0.070};
        const double midpointRadius = pointLength(midpointRadial);
        maxSurfaceError = std::max(maxSurfaceError,
                                   std::abs(midpointRadius - 0.040));
        const Point3D midpointNormal = pointScale(midpointRadial, 1.0 / midpointRadius);
        const double midpointXAfterC =
            midpointNormal.x * std::cos(midpointOrientation.rz) -
            midpointNormal.y * std::sin(midpointOrientation.rz);
        const double midpointYAfterC =
            midpointNormal.x * std::sin(midpointOrientation.rz) +
            midpointNormal.y * std::cos(midpointOrientation.rz);
        const double midpointYAfterA =
            midpointYAfterC * std::cos(midpointOrientation.rx) -
            midpointNormal.z * std::sin(midpointOrientation.rx);
        const double midpointZAfterA =
            midpointYAfterC * std::sin(midpointOrientation.rx) +
            midpointNormal.z * std::cos(midpointOrientation.rx);
        maxNormalError = std::max(
            maxNormalError,
            std::sqrt(midpointXAfterC * midpointXAfterC +
                      midpointYAfterA * midpointYAfterA +
                      (midpointZAfterA - 1.0) * (midpointZAfterA - 1.0)));
    }

    assert(maxPolarAngle > 84.0 * kPi / 180.0);
    assert(maxPolarAngle <= 85.0 * kPi / 180.0);
    assert(maxSurfaceError < 1e-5);            // < 0.010 mm
    assert(maxNormalError < 7e-4);             // < 0.041 deg
    assert(maxRotaryVelocityA <= 1.047 + 1e-3);
    assert(maxRotaryVelocityC <= 1.571 + 1e-3);
}

} // namespace

int main()
{
    test_rotary_words_are_parsed_as_degrees();
    test_butterfly_is_pointwise_xyzac_on_sphere();
    test_sphere_hex_grid_is_continuous_xyzac();
    return 0;
}
