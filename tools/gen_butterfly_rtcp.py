#!/usr/bin/env python3
"""Generate the pointwise XYZAC butterfly toolpath used by maple.nc."""

import math
from pathlib import Path


SPHERE_RADIUS = 40.0
SPHERE_CENTER_Z = 70.0
SAFE_RADIUS = 60.0
HOME_Z = 170.0
PATTERN_SCALE = 3.0
MAX_PLANAR_STEP = 0.5
FEED = 5.0

# Clockwise outline. With C = atan2(X, Y), this keeps C continuous from 0 to 360.
BUTTERFLY_CONTROL_POINTS = [
    (0.0, 4.2),
    (0.8, 3.4),
    (2.4, 4.6),
    (4.5, 5.2),
    (6.2, 4.0),
    (5.2, 2.0),
    (3.4, 0.7),
    (5.5, -1.2),
    (4.5, -3.8),
    (2.4, -3.1),
    (0.8, -1.4),
    (0.0, -3.6),
    (-0.8, -1.4),
    (-2.4, -3.1),
    (-4.5, -3.8),
    (-5.5, -1.2),
    (-3.4, 0.7),
    (-5.2, 2.0),
    (-6.2, 4.0),
    (-4.5, 5.2),
    (-2.4, 4.6),
    (-0.8, 3.4),
    (0.0, 4.2),
]


def subdivide_outline():
    controls = [(x * PATTERN_SCALE, y * PATTERN_SCALE)
                for x, y in BUTTERFLY_CONTROL_POINTS]
    points = [controls[0]]
    for start, end in zip(controls, controls[1:]):
        dx = end[0] - start[0]
        dy = end[1] - start[1]
        divisions = max(1, math.ceil(math.hypot(dx, dy) / MAX_PLANAR_STEP))
        for index in range(1, divisions + 1):
            t = index / divisions
            points.append((start[0] + dx * t, start[1] + dy * t))
    return points


def project_to_sphere(x, y):
    radial_squared = x * x + y * y
    if radial_squared >= SPHERE_RADIUS * SPHERE_RADIUS:
        raise ValueError(f"Butterfly point lies outside R{SPHERE_RADIUS}: ({x}, {y})")
    z_relative = math.sqrt(SPHERE_RADIUS * SPHERE_RADIUS - radial_squared)
    return x, y, SPHERE_CENTER_Z + z_relative


def unwrap_degrees(angle, previous):
    while angle - previous > 180.0:
        angle -= 360.0
    while angle - previous < -180.0:
        angle += 360.0
    return angle


def pointwise_xyzac(points):
    result = []
    previous_c = 0.0
    for index, (x, y) in enumerate(points):
        x, y, z = project_to_sphere(x, y)
        z_relative = z - SPHERE_CENTER_Z
        a = math.degrees(math.atan2(math.hypot(x, y), z_relative))
        raw_c = math.degrees(math.atan2(x, y))
        c = raw_c if index == 0 else unwrap_degrees(raw_c, previous_c)
        result.append((x, y, z, a, c))
        previous_c = c
    return result


def offset_along_normal(point, radius):
    x, y, z, a, c = point
    scale = radius / SPHERE_RADIUS
    return x * scale, y * scale, SPHERE_CENTER_Z + (z - SPHERE_CENTER_Z) * scale, a, c


def format_move(code, point, feed=None):
    x, y, z, a, c = point
    line = f"{code} X{x:.6f} Y{y:.6f} Z{z:.6f} A{a:.6f} C{c:.6f}"
    if feed is not None:
        line += f" F{feed:.3f}"
    return line


def main():
    cutting_points = pointwise_xyzac(subdivide_outline())
    approach = offset_along_normal(cutting_points[0], SAFE_RADIUS)
    retract = offset_along_normal(cutting_points[-1], SAFE_RADIUS)
    output = Path(__file__).resolve().parents[1] / "config" / "5axis" / "program" / "maple.nc"

    lines = [
        "( Simultaneous XYZAC RTCP butterfly engraving on R40 sphere )",
        "( Work sphere center X0 Y0 Z70 mm; A/C align surface normal to +Z )",
        f"( {len(cutting_points)} contour points; max planar step {MAX_PLANAR_STEP:.3f} mm )",
        "( Offline chord tolerance below 0.010 mm; feed convention mm/s )",
        "G21",
        "G90",
        format_move("G0", (0.0, 0.0, HOME_Z, 0.0, 0.0)),
        format_move("G0", approach),
        format_move("G1", cutting_points[0], FEED),
    ]
    lines.extend(format_move("G1", point, FEED) for point in cutting_points[1:])
    lines.extend([
        format_move("G0", retract),
        format_move("G0", (0.0, 0.0, HOME_Z, 0.0, cutting_points[-1][4])),
        "M30",
    ])

    with output.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write("\n".join(lines) + "\n")

    max_xy_radius = max(math.hypot(point[0], point[1]) for point in cutting_points)
    a_values = [point[3] for point in cutting_points]
    c_values = [point[4] for point in cutting_points]
    print(f"Wrote {len(cutting_points)} contour points to {output}")
    print(f"XY radius max: {max_xy_radius:.3f} mm")
    print(f"A range: {min(a_values):.3f} .. {max(a_values):.3f} deg")
    print(f"C range: {min(c_values):.3f} .. {max(c_values):.3f} deg")


if __name__ == "__main__":
    main()
