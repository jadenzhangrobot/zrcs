import csv
from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np

# ── 中文字体配置（Windows）────────────────────────────────────────────
# 按优先级依次尝试系统中可用的中文字体
for _font in ["Microsoft YaHei", "SimHei", "SimSun", "DengXian", "Arial Unicode MS"]:
    try:
        matplotlib.font_manager.findfont(_font, fallback_to_default=False)
        matplotlib.rcParams["font.family"] = "sans-serif"
        matplotlib.rcParams["font.sans-serif"] = [_font] + matplotlib.rcParams["font.sans-serif"]
        matplotlib.rcParams["axes.unicode_minus"] = False  # 负号正常显示
        break
    except Exception:
        continue


def read_axis_log(csv_path: Path):
    cols = {
        "time_ms": [],
        "plat_x": [], "plat_y": [],
        "galvo_x": [], "galvo_y": [],
        "plat_x_cmd": [], "plat_y_cmd": [],
        "galvo_x_cmd": [], "galvo_y_cmd": [],
        "plat_x_vel": [], "plat_y_vel": [],
        "galvo_x_vel": [], "galvo_y_vel": [],
    }
    with csv_path.open("r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                parsed = {key: float(row[key]) for key in cols}
            except (KeyError, TypeError, ValueError):
                continue  # 跳过不完整或截断的行（原子性：要么全部追加，要么全部跳过）
            for key in cols:
                cols[key].append(parsed[key])
    return {k: np.array(v) for k, v in cols.items()}


def main():
    csv_path = Path("axis_log.csv")
    if not csv_path.exists():
        raise FileNotFoundError(f"Log file not found: {csv_path.resolve()}")

    d = read_axis_log(csv_path)
    t = d["time_ms"]

    # ── 裁剪到活跃运动窗口 ────────────────────────────────────────────
    # 用平台指令位置的变化量检测运动起止，避免 idle 时间段干扰轨迹图
    combined_x_all = d["plat_x_cmd"] + d["galvo_x_cmd"]
    combined_y_all = d["plat_y_cmd"] + d["galvo_y_cmd"]

    cmd_speed = np.hypot(np.diff(combined_x_all, prepend=combined_x_all[0]),
                         np.diff(combined_y_all, prepend=combined_y_all[0]))
    # 找第一个 cmdPosition 发生显著变化的帧（运动开始）
    MOVE_THRESH = 0.01  # mm/sample
    active = cmd_speed > MOVE_THRESH
    if not active.any():
        print("警告: 未检测到有效运动，绘制全部数据")
        mask = np.ones(len(t), dtype=bool)
    else:
        first_active = int(np.argmax(active))
        last_active  = int(len(active) - 1 - np.argmax(active[::-1]))
        # 前留 2 帧（避免把 0,0 idle 数据带入），后留 20 帧
        start_idx = max(0, first_active - 2)
        end_idx   = min(len(t) - 1, last_active + 20)
        mask = np.zeros(len(t), dtype=bool)
        mask[start_idx:end_idx + 1] = True
        print(f"活跃运动窗口: {t[first_active]:.0f}ms ~ {t[last_active]:.0f}ms  "
              f"({(t[last_active]-t[first_active])/1000:.1f} s)")

    # 裁剪后的数据
    dm = {k: v[mask] for k, v in d.items()}
    tm = dm["time_ms"]
    combined_x = dm["plat_x_cmd"] + dm["galvo_x_cmd"]
    combined_y = dm["plat_y_cmd"] + dm["galvo_y_cmd"]

    from matplotlib.collections import LineCollection
    from matplotlib.cm import get_cmap

    fig = plt.figure(figsize=(14, 18))
    fig.suptitle("振镜-平台联动轨迹分析", fontsize=14, fontweight="bold")

    # 布局：① ② 各占一整行（全宽），③ ④ 并排在第三行
    gs = gridspec.GridSpec(3, 2, figure=fig,
                           height_ratios=[1, 1, 0.7],
                           hspace=0.45, wspace=0.35)

    # 计算两图共享的坐标范围（加 5% 边距）
    all_x = np.concatenate([combined_x, dm["plat_x_cmd"]])
    all_y = np.concatenate([combined_y, dm["plat_y_cmd"]])
    xspan = all_x.max() - all_x.min()
    yspan = all_y.max() - all_y.min()
    xlim = (all_x.min() - xspan*0.05 - 2, all_x.max() + xspan*0.05 + 2)
    ylim = (all_y.min() - yspan*0.05 - 2, all_y.max() + yspan*0.05 + 2)

    # ── 子图 ①: 综合加工轨迹（全宽）─────────────────────────────────
    ax_comb = fig.add_subplot(gs[0, :])
    points   = np.array([combined_x, combined_y]).T.reshape(-1, 1, 2)
    segments = np.concatenate([points[:-1], points[1:]], axis=1)
    lc = LineCollection(segments, cmap=get_cmap("plasma"),
                        norm=plt.Normalize(tm[0], tm[-1]), linewidth=1.8, zorder=2)
    lc.set_array(tm[:-1])
    ax_comb.add_collection(lc)
    cbar = fig.colorbar(lc, ax=ax_comb, shrink=0.9, pad=0.02)
    cbar.set_label("时间 (ms)", fontsize=7)
    ax_comb.scatter(combined_x[0],  combined_y[0],  s=60, color="green",
                    zorder=5, marker="o", label="起点")
    ax_comb.scatter(combined_x[-1], combined_y[-1], s=60, color="red",
                    zorder=5, marker="x", label="终点")
    ax_comb.set_xlim(xlim); ax_comb.set_ylim(ylim)
    ax_comb.set_xlabel("X (mm)"); ax_comb.set_ylabel("Y (mm)")
    ax_comb.set_title("① 综合加工轨迹  =  平台指令 + 振镜偏移", fontweight="bold", fontsize=12)
    ax_comb.legend(fontsize=9); ax_comb.set_aspect("equal", adjustable="box")
    ax_comb.grid(True, alpha=0.3)

    # ── 子图 ②: 平台轴笛卡尔轨迹（全宽）─────────────────────────────
    ax_plat = fig.add_subplot(gs[1, :])
    ax_plat.plot(dm["plat_x_cmd"], dm["plat_y_cmd"],
                 color="steelblue", linewidth=1.8, label="平台指令", zorder=2)
    ax_plat.plot(dm["plat_x"], dm["plat_y"],
                 color="orange", linewidth=1.2, linestyle="--",
                 alpha=0.8, label="平台实际", zorder=1)
    ax_plat.scatter(dm["plat_x_cmd"][0],  dm["plat_y_cmd"][0],  s=60,
                    color="green", zorder=5, marker="o", label="起点")
    ax_plat.scatter(dm["plat_x_cmd"][-1], dm["plat_y_cmd"][-1], s=60,
                    color="red",   zorder=5, marker="x", label="终点")
    ax_plat.set_xlim(xlim); ax_plat.set_ylim(ylim)
    ax_plat.set_xlabel("X (mm)"); ax_plat.set_ylabel("Y (mm)")
    ax_plat.set_title("② 平台轴笛卡尔轨迹  （低通滤波后的低频大行程）", fontweight="bold", fontsize=12)
    ax_plat.legend(fontsize=9); ax_plat.set_aspect("equal", adjustable="box")
    ax_plat.grid(True, alpha=0.3)

    # ── 子图 ③: 振镜偏移轨迹 ─────────────────────────────────────────
    ax_galvo = fig.add_subplot(gs[2, 0])
    ax_galvo.plot(dm["galvo_x_cmd"], dm["galvo_y_cmd"], color="crimson",
                  linewidth=1.5, label="指令偏移", zorder=2)
    ax_galvo.plot(dm["galvo_x"], dm["galvo_y"], color="gold", linewidth=1,
                  linestyle="--", alpha=0.8, label="实际偏移", zorder=1)
    ax_galvo.scatter(dm["galvo_x_cmd"][0],  dm["galvo_y_cmd"][0],  s=50,
                     color="green", zorder=5, label="起点", marker="o")
    ax_galvo.scatter(dm["galvo_x_cmd"][-1], dm["galvo_y_cmd"][-1], s=50,
                     color="blue",  zorder=5, label="终点", marker="x")
    ax_galvo.set_xlabel("X_offset (mm)")
    ax_galvo.set_ylabel("Y_offset (mm)")
    ax_galvo.set_title("③ 振镜偏移轨迹（高频残差）\n"
                        r"offset = 综合指令 − 平台指令")
    ax_galvo.legend(fontsize=8)
    ax_galvo.set_aspect("equal", adjustable="datalim")
    ax_galvo.grid(True, alpha=0.3)

    # ── 子图 ④: 速度曲线 ──────────────────────────────────────────────
    ax_vel = fig.add_subplot(gs[2, 1])
    plat_speed  = np.hypot(dm["plat_x_vel"],  dm["plat_y_vel"])
    galvo_speed = np.hypot(dm["galvo_x_vel"], dm["galvo_y_vel"])
    total_speed = np.hypot(dm["plat_x_vel"] + dm["galvo_x_vel"],
                           dm["plat_y_vel"] + dm["galvo_y_vel"])
    ax_vel.plot(tm, plat_speed,  color="steelblue", linewidth=1.2, label="平台合速度")
    ax_vel.plot(tm, galvo_speed, color="crimson",   linewidth=1.2, label="振镜合速度")
    ax_vel.plot(tm, total_speed, color="gray", linewidth=1.0, linestyle="--",
                alpha=0.7, label="综合合速度")
    ax_vel.set_xlabel("时间 (ms)")
    ax_vel.set_ylabel("速度 (mm/s)")
    ax_vel.set_title("④ 速度分解: 平台 vs 振镜")
    ax_vel.legend(fontsize=8)
    ax_vel.grid(True, alpha=0.3)

    plt.savefig("galvo_platform_analysis.png", dpi=150, bbox_inches="tight")
    plt.show()
    print("图表已保存: galvo_platform_analysis.png")


if __name__ == "__main__":
    main()
