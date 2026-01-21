import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


def read_report(file_path: Path):
    indices = []
    xs = []
    ys = []
    zs = []
    distances = []
    velocity_limits = []
    raw_xs = []
    raw_ys = []
    raw_zs = []

    with file_path.open('r', newline='', encoding='utf-8') as f:
        reader = csv.reader(f)
        section = 'planned'
        for row in reader:
            if not row:
                continue
            if row[0] == 'RAW_POINTS':
                section = 'raw'
                continue
            if row[0] == 'Index':
                continue

            if section == 'planned':
                if len(row) < 6:
                    continue
                indices.append(int(row[0]))
                xs.append(float(row[1]))
                ys.append(float(row[2]))
                zs.append(float(row[3]))
                distances.append(float(row[4]))
                velocity_limits.append(float(row[5]))
            else:
                if len(row) < 4:
                    continue
                raw_xs.append(float(row[1]))
                raw_ys.append(float(row[2]))
                raw_zs.append(float(row[3]))

    return indices, xs, ys, zs, distances, velocity_limits, raw_xs, raw_ys, raw_zs


def plot_3d(xs, ys, zs, raw_xs, raw_ys, raw_zs, output_path: Path, show: bool):
    fig = plt.figure(figsize=(7, 6), dpi=120)
    ax = fig.add_subplot(111, projection='3d')
    ax.plot(xs, ys, zs, linewidth=2.2, color='tab:blue', label='Planned')
    ax.scatter(xs, ys, zs, s=8, color='tab:blue', alpha=0.35)
    if raw_xs:
        ax.scatter(raw_xs, raw_ys, raw_zs, s=26, color='tab:orange', marker='x', label='Raw')
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.legend()
    fig.tight_layout()
    fig.savefig(output_path)
    if show:
        plt.show()
    plt.close(fig)


def plot_metrics(indices, distances, velocity_limits, output_path: Path, show: bool):
    fig, ax = plt.subplots(figsize=(7, 4), dpi=120)
    ax.plot(indices, distances, label='Distance')
    ax.plot(indices, velocity_limits, label='VelocityLimit')
    ax.set_xlabel('Index')
    ax.set_ylabel('Value')
    ax.grid(True)
    ax.legend()
    fig.tight_layout()
    fig.savefig(output_path)
    if show:
        plt.show()
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--input',
        default=r'e:\zrcs-dev\build\bin\3D_planning_report.txt',
    )
    parser.add_argument('--output-dir', default='')
    parser.add_argument('--show', action='store_true')
    args = parser.parse_args()

    input_path = Path(args.input)
    output_dir = Path(args.output_dir) if args.output_dir else input_path.parent
    output_dir.mkdir(parents=True, exist_ok=True)

    indices, xs, ys, zs, distances, velocity_limits, raw_xs, raw_ys, raw_zs = read_report(input_path)

    plot_3d_path = output_dir / (input_path.stem + '_3d.png')
    plot_metrics_path = output_dir / (input_path.stem + '_metrics.png')

    plot_3d(xs, ys, zs, raw_xs, raw_ys, raw_zs, plot_3d_path, args.show)
    plot_metrics(indices, distances, velocity_limits, plot_metrics_path, args.show)


if __name__ == '__main__':
    main()
