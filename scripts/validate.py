#!/usr/bin/env python3

import argparse
import json
import pathlib
import sys

import numpy as np
import pandas as pd

RTOL = 1e-9
ATOL = 1e-6


def reference(input_path: pathlib.Path) -> np.ndarray:
    with input_path.open("r", encoding="utf-8") as f:
        data = json.load(f)
    a = np.array(data["matrix_a"], dtype=np.float64)
    b = np.array(data["matrix_b"], dtype=np.float64)
    return a @ b


def compare(expected: np.ndarray, result: list) -> tuple[bool, float]:
    actual = np.array(result, dtype=np.float64)
    if expected.shape != actual.shape:
        return False, float("inf")
    ok = bool(np.allclose(expected, actual, rtol=RTOL, atol=ATOL))
    return ok, float(np.max(np.abs(expected - actual)))


def check_csv(csv_path: pathlib.Path) -> bool:
    df = pd.read_csv(csv_path)
    bad = df[~df["valid"].astype(bool)]
    print(f"Проверено запусков:     {len(df)}")
    print(f"Совпадает с NumPy:      {len(df) - len(bad)} из {len(df)}")
    print(f"Макс. абс. ошибка:      {df['max_abs_err'].max():.3e}")
    if not bad.empty:
        print("Несовпадения:")
        print(bad.to_string(index=False))
    return bad.empty


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", type=pathlib.Path, required=True,
                        help="Сводный general.csv с колонками valid и max_abs_err")
    args = parser.parse_args()

    sys.exit(0 if check_csv(args.csv) else 1)


if __name__ == "__main__":
    main()
