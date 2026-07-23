#!/usr/bin/env python3
"""Generate a continuous hexagonal spiral G-code for the 5-axis R40 sphere."""

import math
import os

# ============================================================
# Sphere (matching maple.nc work coordinates)
# ============================================================
R = 40.0
CX, CY, CZ = 0.0, 0.0, 70.0

# ============================================================
# Pattern parameters
# ============================================================
STEP = 0.5       # step along hex edges (mm)
LAYERS = 45      # hexagonal rings

# ============================================================
# Hex directions — 6 directions at 60° intervals (pointy-top)
# ============================================================
SQRT3 = math.sqrt(3)
DIRS = [
    (1.0, 0.0),           # 0: E
    (0.5, SQRT3 / 2.0),   # 1: NE
    (-0.5, SQRT3 / 2.0),  # 2: NW
    (-1.0, 0.0),          # 3: W
    (-0.5, -SQRT3 / 2.0), # 4: SW
    (0.5, -SQRT3 / 2.0),  # 5: SE
]


def project(x, y):
    """Project (x,y) → sphere upper surface, return (X,Y,Z) in work coords."""
    r2 = x * x + y * y
    limit = R * 0.995
    max_r2 = limit * limit
    if r2 > max_r2:
        r = math.sqrt(r2)
        scale = limit / r
        x *= scale
        y *= scale
    z = CZ + math.sqrt(max(0.0, R * R - x * x - y * y))
    return (x + CX, y + CY, z)


# ============================================================
# Hexagonal spiral path (flat in XY, then projected)
# ============================================================
x, y = 0.0, 0.0
path = [(x, y)]

for layer in range(1, LAYERS + 1):
    for d in range(6):
        n = layer
        if d == 5:
            n -= 1  # last side shortened to connect to next ring
        for _ in range(n):
            x += DIRS[d][0] * STEP
            y += DIRS[d][1] * STEP
            path.append((x, y))

# ============================================================
# Project & write G-code
# ============================================================
pts = [project(px, py) for px, py in path]

OUTPUT = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "config", "5axis", "program", "hexagon.nc",
)

with open(OUTPUT, "w", encoding="utf-8") as f:
    f.write(f"( Hexagonal spiral on R40 sphere )\n")
    f.write(f"( Work coords: sphere center X0 Y0 Z70 )\n")
    f.write(f"( {len(pts)} waypoints, step={STEP}mm, layers={LAYERS} )\n")
    f.write("G21\nG90\n")
    for px, py, pz in pts:
        f.write(f"G1 X{px:.6f} Y{py:.6f} Z{pz:.6f} F20.000\n")

# Stats
max_r = max(math.sqrt((p[0] - CX) ** 2 + (p[1] - CY) ** 2) for p in pts)
print(f"Written {len(pts)} G-code lines → {OUTPUT}")
print(f"Max XY radius: {max_r:.1f} mm  (sphere R={R})")
print(f"Z range: {min(p[2] for p in pts):.3f} .. {max(p[2] for p in pts):.3f}")
