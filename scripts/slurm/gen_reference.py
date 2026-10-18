#!/usr/bin/env python3
"""Проверка корректности результатов л/р 3 без скачивания матриц с кластера.

Программа на кластере создаёт матрицы A и B детерминированно по размеру и seed
(тем же генератором, что реализован здесь и в src/utils/matrix_gen.hpp) и
записывает в summary.csv контрольные суммы результата C = A @ B:
    checksum    = сумма всех элементов C,
    checksum_sq = сумма квадратов всех элементов C.

Этот скрипт пересоздаёт те же матрицы у вас на компьютере, честно считает C = A @ B
через NumPy и сравнивает контрольные суммы. Совпадение подтверждает, что MPI-версия
считает произведение верно. Матрицы с кластера скачивать не нужно.

Запуск (на своём компьютере, нужен только NumPy):
    python3 scripts/slurm/gen_reference.py --summary results/lab_03/summary.csv
    python3 scripts/slurm/gen_reference.py --sizes 400 800 --seed 42   # без сравнения
"""

import argparse
import csv
import pathlib
import sys

try:
    import numpy as np
except ImportError:
    sys.exit("Нужен NumPy: pip install numpy (запускается на вашем компьютере, не на кластере)")

MASK = (1 << 64) - 1
MULT = 6364136223846793005
INCR = 1442695040888963407
INV = 1.0 / 9007199254740992.0


def generate(n: int, seed: int, lo: float = -10.0, hi: float = 10.0) -> np.ndarray:
    """Матрица n x n тем же 64-битным LCG, что и matrix_gen.hpp (бит-в-бит)."""
    count = n * n
    out = np.empty(count, dtype=np.float64)
    state = seed & MASK
    span = hi - lo
    for i in range(count):
        state = (state * MULT + INCR) & MASK
        unit = (state >> 11) * INV
        out[i] = lo + span * unit
    return out.reshape(n, n)


def checksums(n: int, seed: int) -> "tuple":
    a = generate(n, seed)
    b = generate(n, seed + 1)
    c = a @ b
    return float(c.sum()), float((c * c).sum())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--summary", type=pathlib.Path, default=None,
                        help="results/lab_03/summary.csv для сравнения контрольных сумм")
    parser.add_argument("--sizes", type=int, nargs="+", default=None,
                        help="Размеры для расчёта эталонных сумм (если нет --summary)")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--rtol", type=float, default=1e-6,
                        help="Относительный допуск сравнения контрольных сумм")
    args = parser.parse_args()

    if args.summary is not None:
        with args.summary.open(newline="", encoding="utf-8") as f:
            rows = list(csv.DictReader(f))
        seeds = {int(r["size"]): args.seed for r in rows}
        expected = {n: checksums(n, s) for n, s in sorted(seeds.items())}

        bad = 0
        print("{:>6} {:>5} {:>14} {:>14} {:>8}".format("size", "np", "checksum", "эталон", "ok"))
        for r in rows:
            n = int(r["size"])
            exp_sum, exp_sq = expected[n]
            got_sum = float(r["checksum"])
            ok = abs(got_sum - exp_sum) <= args.rtol * max(1.0, abs(exp_sum))
            bad += not ok
            print("{:>6} {:>5} {:>14.6g} {:>14.6g} {:>8}".format(
                n, r["processes"], got_sum, exp_sum, "да" if ok else "НЕТ"))
        if bad:
            sys.exit("Несовпадений: {}".format(bad))
        print("Все контрольные суммы совпали — результаты корректны.")
    else:
        sizes = args.sizes or [400, 800, 1600]
        print("{:>6} {:>18} {:>18}".format("size", "checksum", "checksum_sq"))
        for n in sizes:
            s, sq = checksums(n, args.seed)
            print("{:>6} {:>18.10g} {:>18.10g}".format(n, s, sq))


if __name__ == "__main__":
    main()
