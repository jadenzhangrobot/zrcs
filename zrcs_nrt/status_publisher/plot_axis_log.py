import csv
from pathlib import Path

import matplotlib.pyplot as plt


def read_axis_log(csv_path: Path):
    time_ms = []
    x_pos, y_pos, z_pos = [], [], []
    x_cmd, y_cmd, z_cmd = [], [], []
    x_vel, y_vel, z_vel = [], [], []

    with csv_path.open("r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                vals = [
                    float(row["time_ms"]),
                    float(row["x_pos"]), float(row["y_pos"]), float(row["z_pos"]),
                    float(row["x_cmd"]), float(row["y_cmd"]), float(row["z_cmd"]),
                    float(row["x_vel"]), float(row["y_vel"]), float(row["z_vel"]),
                ]
            except (TypeError, ValueError):
                continue  # 跳过不完整或截断的行
            time_ms.append(vals[0])
            x_pos.append(vals[1]); y_pos.append(vals[2]); z_pos.append(vals[3])
            x_cmd.append(vals[4]); y_cmd.append(vals[5]); z_cmd.append(vals[6])
            x_vel.append(vals[7]); y_vel.append(vals[8]); z_vel.append(vals[9])

    return {
        "time_ms": time_ms,
        "x_pos": x_pos,
        "y_pos": y_pos,
        "z_pos": z_pos,
        "x_cmd": x_cmd,
        "y_cmd": y_cmd,
        "z_cmd": z_cmd,
        "x_vel": x_vel,
        "y_vel": y_vel,
        "z_vel": z_vel,
    }


def main():
    csv_path = Path("axis_log.csv")
    if not csv_path.exists():
        raise FileNotFoundError(f"Log file not found: {csv_path.resolve()}")

    data = read_axis_log(csv_path)
    t = data["time_ms"]

    fig = plt.figure(figsize=(12, 10))

    ax3d = fig.add_subplot(2, 1, 1, projection="3d")
    ax3d.plot(data["x_pos"], data["y_pos"], data["z_pos"], label="actual xyz", linewidth=2)
    ax3d.plot(data["x_cmd"], data["y_cmd"], data["z_cmd"], "--", label="command xyz", linewidth=2)
    ax3d.scatter(data["x_pos"][0], data["y_pos"][0], data["z_pos"][0], label="actual start", marker="o")
    ax3d.scatter(data["x_pos"][-1], data["y_pos"][-1], data["z_pos"][-1], label="actual end", marker="x")
    ax3d.set_xlabel("X")
    ax3d.set_ylabel("Y")
    ax3d.set_zlabel("Z")
    ax3d.set_title("XYZ 3D Trajectory")
    ax3d.legend()

    ax_vel = fig.add_subplot(2, 1, 2)
    ax_vel.plot(t, data["x_vel"], label="x_vel")
    ax_vel.plot(t, data["y_vel"], label="y_vel")
    ax_vel.plot(t, data["z_vel"], label="z_vel")
    ax_vel.set_xlabel("Time (ms)")
    ax_vel.set_ylabel("Velocity")
    ax_vel.set_title("XYZ Axis Velocity")
    ax_vel.grid(True)
    ax_vel.legend()

    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
