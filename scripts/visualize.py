#!/usr/bin/env python3

import argparse
import pathlib

import matplotlib.pyplot as plt
import pandas as pd


def plot_time_vs_size(df: pd.DataFrame, out_path: pathlib.Path) -> None:
    fig, ax = plt.subplots(figsize=(9, 6))

    sorted_df = df.sort_values("size")
    ax.plot(sorted_df["size"], sorted_df["elapsed_seconds"], marker="o")

    ax.set_xlabel("Размер матрицы N (N x N)")
    ax.set_ylabel("Время выполнения, ms")
    ax.set_title("Время умножения матриц от размера задачи")
    ax.grid(True, which="both", linestyle="--", alpha=0.4)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_gflops_vs_size(df: pd.DataFrame, out_path: pathlib.Path) -> None:
    fig, ax = plt.subplots(figsize=(9, 6))

    work = df.copy()
    work["gflops"] = work["flops"] / work["elapsed_seconds"] / 1e9

    sorted_work = work.sort_values("size")
    ax.plot(sorted_work["size"], sorted_work["gflops"], marker="o")

    ax.set_xlabel("Размер матрицы N (N x N)")
    ax.set_ylabel("Производительность, GFLOP/s")
    ax.set_title("Производительность умножения матриц от размера задачи")
    ax.grid(True, linestyle="--", alpha=0.4)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=pathlib.Path)
    parser.add_argument("--out-dir", type=pathlib.Path, default=pathlib.Path("lab_1/report/figures"))
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    df = pd.read_csv(args.csv_path)

    plots = {"time_vs_size.png": plot_time_vs_size,
            "gflops_vs_size.png": plot_gflops_vs_size}

    for filename, plot_fn in plots.items():
        out_path = args.out_dir / filename
        plot_fn(df, out_path)
        if out_path.exists():
            print(f"График сохранён: {out_path}")


if __name__ == "__main__":
    main()