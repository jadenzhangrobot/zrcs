#!/usr/bin/env python3
"""Generate a continuous XYZAC hex grid over the reachable R40 sphere cap."""

from collections import deque
import math
from pathlib import Path
import random


SPHERE_RADIUS = 40.0
SPHERE_CENTER_Z = 70.0
CAP_ANGLE_DEG = 85.0
HEX_SIDE = 5.0
MAX_PLANAR_STEP = 0.5
SAFE_RADIUS = 60.0
HOME_Z = 170.0
FEED = 5.0
A_MAX_VELOCITY = 1.047
C_MAX_VELOCITY = 1.571
KEY_SCALE = 1_000_000


def point_key(x, y):
    return round(x * KEY_SCALE), round(y * KEY_SCALE)


def canonical_edge(first, second):
    return (first, second) if first < second else (second, first)


def build_hex_grid():
    cap_radius = SPHERE_RADIUS * math.radians(CAP_ANGLE_DEG)
    bound = math.ceil((cap_radius + HEX_SIDE) / (1.5 * HEX_SIDE)) + 2
    positions = {}
    cells = []

    for q in range(-bound, bound + 1):
        for r in range(-bound, bound + 1):
            center_x = 1.5 * HEX_SIDE * q
            center_y = math.sqrt(3.0) * HEX_SIDE * (r + 0.5 * q)
            vertices = []
            raw_vertices = []
            for index in range(6):
                angle = math.radians(60.0 * index)
                x = center_x + HEX_SIDE * math.cos(angle)
                y = center_y + HEX_SIDE * math.sin(angle)
                raw_vertices.append((x, y))
                vertices.append(point_key(x, y))

            if max(math.hypot(x, y) for x, y in raw_vertices) > cap_radius + 1e-9:
                continue

            for key, position in zip(vertices, raw_vertices):
                positions.setdefault(key, position)
            cells.append((q, r, tuple(vertices)))

    if not cells:
        raise RuntimeError("No hex cells fit inside the configured sphere cap")

    edges = set()
    graph = {key: set() for key in positions}
    for _, _, vertices in cells:
        for index in range(6):
            edge = canonical_edge(vertices[index], vertices[(index + 1) % 6])
            edges.add(edge)

    used_vertices = {vertex for edge in edges for vertex in edge}
    graph = {key: set() for key in used_vertices}
    for first, second in edges:
        graph[first].add(second)
        graph[second].add(first)

    return cells, positions, graph, sorted(edges)


def shortest_paths(graph, start):
    parent = {start: None}
    distance = {start: 0}
    queue = deque([start])
    while queue:
        node = queue.popleft()
        for neighbor in sorted(graph[node]):
            if neighbor in parent:
                continue
            parent[neighbor] = node
            distance[neighbor] = distance[node] + 1
            queue.append(neighbor)
    return parent, distance


def reconstruct_path(parent, end):
    path = []
    node = end
    while node is not None:
        path.append(node)
        node = parent[node]
    path.reverse()
    return path


def eulerize_graph(graph, edges):
    remaining = set(node for node, neighbors in graph.items() if len(neighbors) % 2 == 1)
    duplicated_edges = []

    while remaining:
        first = min(remaining)
        remaining.remove(first)
        parent, distance = shortest_paths(graph, first)
        second = min(remaining, key=lambda node: (distance[node], node))
        remaining.remove(second)
        path = reconstruct_path(parent, second)
        duplicated_edges.extend(zip(path, path[1:]))

    multiedges = list(edges) + duplicated_edges
    degree = {node: 0 for node in graph}
    for first, second in multiedges:
        degree[first] += 1
        degree[second] += 1
    if any(value % 2 for value in degree.values()):
        raise RuntimeError("Eulerization left odd-degree grid vertices")
    return multiedges, duplicated_edges


def hierholzer_route(multiedges, start, seed):
    rng = random.Random(seed)
    adjacency = {}
    for edge_id, (first, second) in enumerate(multiedges):
        adjacency.setdefault(first, []).append((edge_id, second))
        adjacency.setdefault(second, []).append((edge_id, first))
    for entries in adjacency.values():
        rng.shuffle(entries)

    used = [False] * len(multiedges)
    stack = [start]
    circuit = []
    while stack:
        node = stack[-1]
        while adjacency[node] and used[adjacency[node][-1][0]]:
            adjacency[node].pop()
        if adjacency[node]:
            edge_id, neighbor = adjacency[node].pop()
            if not used[edge_id]:
                used[edge_id] = True
                stack.append(neighbor)
        else:
            circuit.append(stack.pop())

    circuit.reverse()
    if len(circuit) != len(multiedges) + 1 or not all(used):
        raise RuntimeError("Failed to build a complete Euler circuit")
    return circuit


def rotate_to_nearest_pole(route, positions):
    cycle = route[:-1]
    start_index = min(
        range(len(cycle)), key=lambda index: math.hypot(*positions[cycle[index]]))
    rotated = cycle[start_index:] + cycle[:start_index]
    return rotated + [rotated[0]]


def unwrap_angles(route, positions):
    values = []
    previous = None
    for node in route:
        x, y = positions[node]
        angle = math.degrees(math.atan2(x, y))
        if previous is not None:
            while angle - previous > 180.0:
                angle -= 360.0
            while angle - previous < -180.0:
                angle += 360.0
        values.append(angle)
        previous = angle
    return values


def best_global_c_shift(values):
    best = None
    for turns in range(-12, 13):
        shifted = [value + 360.0 * turns for value in values]
        score = max(abs(min(shifted)), abs(max(shifted)))
        if best is None or score < best[0]:
            best = score, 360.0 * turns
    return best


def select_euler_route(multiedges, positions):
    nodes = sorted(positions)
    best = None
    for seed in range(256):
        start = nodes[seed % len(nodes)]
        route = rotate_to_nearest_pole(hierholzer_route(multiedges, start, seed), positions)
        for candidate in (route, list(reversed(route))):
            c_values = unwrap_angles(candidate, positions)
            max_abs, shift = best_global_c_shift(c_values)
            span = max(c_values) - min(c_values)
            winding = abs(c_values[-1] - c_values[0])
            score = max_abs, span, winding, seed
            if best is None or score < best[0]:
                best = score, candidate, shift

    score, route, shift = best
    if score[0] > 360.0005:
        raise RuntimeError(
            f"Continuous route needs C outside +/-360 deg (best {score[0]:.3f} deg)")
    return route, shift


def map_to_sphere(u, v):
    arc_radius = math.hypot(u, v)
    theta = arc_radius / SPHERE_RADIUS
    if arc_radius == 0.0:
        return 0.0, 0.0, SPHERE_CENTER_Z + SPHERE_RADIUS
    surface_radius = SPHERE_RADIUS * math.sin(theta)
    scale = surface_radius / arc_radius
    return u * scale, v * scale, SPHERE_CENTER_Z + SPHERE_RADIUS * math.cos(theta)


def sample_route(route, positions):
    planar_points = [positions[node] for node in route]
    sampled = [planar_points[0]]
    for start, end in zip(planar_points, planar_points[1:]):
        dx = end[0] - start[0]
        dy = end[1] - start[1]
        divisions = max(1, math.ceil(math.hypot(dx, dy) / MAX_PLANAR_STEP))
        for index in range(1, divisions + 1):
            t = index / divisions
            sampled.append((start[0] + dx * t, start[1] + dy * t))
    return sampled


def pointwise_xyzac(planar_points, c_shift):
    points = []
    previous_c = None
    for u, v in planar_points:
        x, y, z = map_to_sphere(u, v)
        a = math.degrees(math.hypot(u, v) / SPHERE_RADIUS)
        c = math.degrees(math.atan2(x, y)) + c_shift
        if previous_c is not None:
            while c - previous_c > 180.0:
                c -= 360.0
            while c - previous_c < -180.0:
                c += 360.0
        points.append((x, y, z, a, c))
        previous_c = c
    return points


def offset_along_normal(point, radius):
    x, y, z, a, c = point
    scale = radius / SPHERE_RADIUS
    return x * scale, y * scale, SPHERE_CENTER_Z + (z - SPHERE_CENTER_Z) * scale, a, c


def format_move(code, point, feed=None):
    x, y, z, a, c = point
    line = f"{code} X{x:.9f} Y{y:.9f} Z{z:.9f} A{a:.9f} C{c:.9f}"
    if feed is not None:
        line += f" F{feed:.3f}"
    return line


def main():
    cells, positions, graph, edges = build_hex_grid()
    multiedges, duplicated_edges = eulerize_graph(graph, edges)
    route, c_shift = select_euler_route(multiedges, positions)
    cutting_points = pointwise_xyzac(sample_route(route, positions), c_shift)
    approach = offset_along_normal(cutting_points[0], SAFE_RADIUS)
    retract = offset_along_normal(cutting_points[-1], SAFE_RADIUS)

    c_values = [point[4] for point in cutting_points]
    if min(c_values) < -360.0005 or max(c_values) > 360.0005:
        raise RuntimeError("Sampled C values exceed the configured +/-360 deg limit")

    max_a_velocity = 0.0
    max_c_velocity = 0.0
    for start, end in zip(cutting_points, cutting_points[1:]):
        distance = math.dist(start[:3], end[:3])
        duration = distance / FEED
        max_a_velocity = max(
            max_a_velocity, math.radians(abs(end[3] - start[3])) / duration)
        max_c_velocity = max(
            max_c_velocity, math.radians(abs(end[4] - start[4])) / duration)
    if max_a_velocity > A_MAX_VELOCITY or max_c_velocity > C_MAX_VELOCITY:
        raise RuntimeError(
            "Generated feed exceeds a rotary velocity limit: "
            f"A={max_a_velocity:.3f}, C={max_c_velocity:.3f} rad/s")

    output = Path(__file__).resolve().parents[1] / "config" / "5axis" / "program" / "sphere_hex_grid.nc"
    lines = [
        "( Continuous XYZAC spherical hex grid on R40 workpiece )",
        f"( {len(cells)} cells; {len(edges)} unique edges; {len(duplicated_edges)} repeated routing edges )",
        f"( Reachable cap 0..{CAP_ANGLE_DEG:.1f} deg; hex side {HEX_SIDE:.3f} mm; step <= {MAX_PLANAR_STEP:.3f} mm )",
        f"( One uninterrupted G1 Euler route at F{FEED:.3f}; A/C align surface normal to +Z )",
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

    a_values = [point[3] for point in cutting_points]
    print(f"Wrote {len(cutting_points)} cutting points to {output}")
    print(f"Cells: {len(cells)}, unique edges: {len(edges)}, repeated edges: {len(duplicated_edges)}")
    print(f"A range: {min(a_values):.3f} .. {max(a_values):.3f} deg")
    print(f"C range: {min(c_values):.3f} .. {max(c_values):.3f} deg")
    print(f"Peak rotary velocity: A={max_a_velocity:.3f}, C={max_c_velocity:.3f} rad/s")


if __name__ == "__main__":
    main()
