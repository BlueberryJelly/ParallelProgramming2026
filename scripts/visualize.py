#!/usr/bin/env python3

import argparse
import pathlib

import matplotlib.pyplot as plt
import pandas as pd


def _save(fig, out_path: pathlib.Path) -> None:
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_time_vs_size(df: pd.DataFrame, out_path: pathlib.Path) -> None:
    fig, ax = plt.subplots(figsize=(9, 6))

    sorted_df = df.sort_values("size")
    ax.plot(sorted_df["size"], sorted_df["elapsed_seconds"]*1000, marker="o")

    ax.set_xlabel("Размер матрицы N (N x N)")
    ax.set_ylabel("Время выполнения, ms")
    ax.set_title("Время умножения матриц от размера задачи")
    ax.grid(True, which="both", linestyle="--", alpha=0.4)
    _save(fig, out_path)


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
    _save(fig, out_path)


def with_speedup(df: pd.DataFrame) -> pd.DataFrame:
    """Ускорение S = T(1 поток) / T(p) и эффективность E = S / p при том же числе ядер."""
    work = df.copy()
    work["gflops"] = work["flops"] / work["elapsed_seconds"] / 1e9
    base = (work[work["threads"] == 1]
            .set_index(["size", "cores"])["elapsed_seconds"]
            .rename("t1_seconds"))
    work = work.join(base, on=["size", "cores"])
    work["speedup"] = work["t1_seconds"] / work["elapsed_seconds"]
    work["efficiency"] = work["speedup"] / work["threads"]
    return work


def plot_parallel(df: pd.DataFrame, out_dir: pathlib.Path) -> list[pathlib.Path]:
    work = with_speedup(df)
    cores_max = int(work["cores"].max())
    at_max = work[work["cores"] == cores_max]
    threads = sorted(at_max["threads"].unique())
    sizes = sorted(at_max["size"].unique())
    paths = []

    fig, ax = plt.subplots(figsize=(9, 6))
    for t in threads:
        part = at_max[at_max["threads"] == t].sort_values("size")
        ax.plot(part["size"], part["elapsed_seconds"] * 1000, marker="o", label=f"{t} пот.")
    ax.set_xlabel("Размер матрицы N (N x N)")
    ax.set_ylabel("Время выполнения, ms")
    ax.set_title(f"Время умножения от размера задачи ({cores_max} ядер)")
    ax.set_yscale("log")
    ax.grid(True, which="both", linestyle="--", alpha=0.4)
    ax.legend(title="Потоки")
    paths.append(out_dir / "time_vs_size.png"); _save(fig, paths[-1])

    fig, ax = plt.subplots(figsize=(9, 6))
    for t in threads:
        part = at_max[at_max["threads"] == t].sort_values("size")
        ax.plot(part["size"], part["gflops"], marker="o", label=f"{t} пот.")
    ax.set_xlabel("Размер матрицы N (N x N)")
    ax.set_ylabel("Производительность, GFLOP/s")
    ax.set_title(f"Производительность от размера задачи ({cores_max} ядер)")
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.legend(title="Потоки")
    paths.append(out_dir / "gflops_vs_size.png"); _save(fig, paths[-1])

    fig, ax = plt.subplots(figsize=(9, 6))
    for n in sizes:
        part = at_max[at_max["size"] == n].sort_values("threads")
        ax.plot(part["threads"], part["speedup"], marker="o", label=f"N={n}")
    ax.plot(threads, [min(t, cores_max) for t in threads], "k--", alpha=0.6, label="идеальное")
    ax.set_xscale("log", base=2)
    ax.set_xticks(threads, [str(t) for t in threads])
    ax.set_xlabel("Число потоков")
    ax.set_ylabel("Ускорение S = T1 / Tp")
    ax.set_title(f"Ускорение от числа потоков ({cores_max} ядер)")
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.legend()
    paths.append(out_dir / "speedup_vs_threads.png"); _save(fig, paths[-1])

    fig, ax = plt.subplots(figsize=(9, 6))
    for n in sizes:
        part = at_max[at_max["size"] == n].sort_values("threads")
        ax.plot(part["threads"], part["efficiency"], marker="o", label=f"N={n}")
    ax.axhline(1.0, color="k", linestyle="--", alpha=0.6)
    ax.set_xscale("log", base=2)
    ax.set_xticks(threads, [str(t) for t in threads])
    ax.set_xlabel("Число потоков")
    ax.set_ylabel("Эффективность E = S / p")
    ax.set_title(f"Эффективность распараллеливания ({cores_max} ядер)")
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.legend()
    paths.append(out_dir / "efficiency_vs_threads.png"); _save(fig, paths[-1])

    if work["cores"].nunique() > 1:
        n_max = int(work["size"].max())
        big = work[work["size"] == n_max]
        cores_min = int(big["cores"].min())
        t_ref = big[(big["threads"] == 1) & (big["cores"] == cores_min)]["elapsed_seconds"].iloc[0]
        fig, ax = plt.subplots(figsize=(9, 6))
        for t in sorted(big["threads"].unique()):
            part = big[big["threads"] == t].sort_values("cores")
            ax.plot(part["cores"], t_ref / part["elapsed_seconds"], marker="o", label=f"{t} пот.")
        cores = sorted(big["cores"].unique())
        ax.plot(cores, cores, "k--", alpha=0.6, label="идеальное")
        ax.set_xticks(cores, [str(c) for c in cores])
        ax.set_xlabel("Число выделенных ядер (логических процессоров)")
        ax.set_ylabel(f"Ускорение относительно 1 потока на {cores_min} ядре")
        ax.set_title(f"Ускорение от числа ядер, N={n_max}")
        ax.grid(True, linestyle="--", alpha=0.4)
        ax.legend(title="Потоки")
        paths.append(out_dir / "speedup_vs_cores.png"); _save(fig, paths[-1])

    return paths


def write_tables(df: pd.DataFrame, out_path: pathlib.Path) -> None:
    """Сводные таблицы в Markdown для вставки в отчёт."""
    work = with_speedup(df)
    lines = []
    for cores in sorted(work["cores"].unique()):
        part = work[work["cores"] == cores]
        for column, title, fmt in (("elapsed_seconds", "Время, с", "{:.4g}"),
                                   ("speedup", "Ускорение S", "{:.2f}"),
                                   ("efficiency", "Эффективность E", "{:.2f}")):
            table = part.pivot_table(index="size", columns="threads", values=column)
            lines.append(f"### {title}, ядер: {cores}\n")
            lines.append("| N / потоки | " + " | ".join(str(t) for t in table.columns) + " |")
            lines.append("|---" * (len(table.columns) + 1) + "|")
            for n, row in table.iterrows():
                lines.append(f"| {n} | " + " | ".join(fmt.format(v) for v in row) + " |")
            lines.append("")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=pathlib.Path,
                        help="Путь ко входным данным для визуализации")
    parser.add_argument("--out-dir", type=pathlib.Path,
                        required=True, help="Директория для сохранения графиков")
    parser.add_argument("--tables-out", type=pathlib.Path, 
                        default=None, help="Путь для записи Markdown-таблицы (для многопоточных замеров)")
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    df = pd.read_csv(args.csv_path)

    if "threads" in df.columns:
        for out_path in plot_parallel(df, args.out_dir):
            print(f"График сохранён: {out_path}")
        if args.tables_out is not None:
            write_tables(df, args.tables_out)
            print(f"Таблицы сохранены: {args.tables_out}")
        return

    plots = {"time_vs_size.png": plot_time_vs_size,
            "gflops_vs_size.png": plot_gflops_vs_size}

    for filename, plot_fn in plots.items():
        out_path = args.out_dir / filename
        plot_fn(df, out_path)
        print(f"График сохранён: {out_path}")


if __name__ == "__main__":
    main()
