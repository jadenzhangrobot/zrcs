# Trajectory Fitting

This motion preprocessing layer uses a segment-level interface between path fitting
and velocity lookahead.

## Data Model

`Point3D` is geometry only:

```cpp
struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};
```

Programmed velocity belongs to `PathMoveBlock`:

```cpp
struct PathMoveBlock {
    int block_id = 0;
    PathMoveType type = PathMoveType::Line;
    Point3D start;
    Point3D end;
    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;
    double feedrate = 0.0; // m/s, <= 0 falls back to global maxVel
};
```

**Units (controller SI):** position **m**, velocity **m/s**, acceleration **m/s²**, orientation **rad**.  
G-code programs use standard G21 (**mm**) / G20 (**inch**); `NcParser` converts position to meters.  
**F feed:** this project treats F as program-length-unit **per second** (G21 → mm/s → m/s), not mm/min.

Path fitting outputs `TrajectorySegment`. The fitting stage fills geometry fields
such as `length`, `coeff`, `max_curvature`, and `feedrate_limit`. Velocity
lookahead then fills `v_max_local`, `v_enter`, `v_exit`, `duration`, and related
dynamic fields.

## Fitting Flow

`PathPreprocessor::fitCornerBlendSegments()` accepts line move blocks:

```cpp
std::vector<TrajectorySegment> segments =
    fitter.fitCornerBlendSegments(blocks, sampleStep, cornerTol);
```

The current implementation keeps long line portions as `Line` segments. At active
corners, it inserts a local clamped cubic B-spline transition and splits that
transition at its internal knot into two `CubicPolynomial` segments. Each segment
uses `u in [0, 1]`:

```cpp
p_axis(u) = c0 + c1*u + c2*u^2 + c3*u^3
```

The first three axes are `x/y/z`; the remaining axes are reserved for pose.

## Velocity Lookahead

`VelocityPlanner3D::planSegments()` mutates the same segment vector:

```cpp
VelocityPlanner3D planner;
planner.setConfig(maxVel, maxAccel, startVel, endVel, cornerTol, maxJerk);
planner.planSegments(segments);
```

For each segment, local velocity is limited by:

- global maximum velocity,
- programmed `feedrate_limit`,
- curvature limit `sqrt(maxAccel / max_curvature)`.

The planner then performs backward and forward scans over segment length and fills
boundary velocities:

```cpp
segment.v_enter;
segment.v_exit;
segment.duration;
```

## Plotting Test

`test_motion_preprocessing` draws two matplotlib-cpp figures:

- fitted segment outline sampled from `TrajectorySegment`,
- velocity lookahead profile over arc length.

Plotting is optional. Install Python `matplotlib` to show these figures. If it
is not installed, the test skips plotting and still runs the assertions. With a
non-interactive backend such as `Agg`, the figures are saved as
`motion_preprocessing_segments.png` and `motion_preprocessing_velocity.png`;
set `ZRCS_SKIP_PLOTS=1` to skip plotting explicitly.
