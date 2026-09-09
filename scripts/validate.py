#!/usr/bin/env python3

import argparse
import json
import pathlib
import sys

import numpy as np


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_json", type=pathlib.Path, help="Файл с matrix_a/matrix_b")
    parser.add_argument("output_json", type=pathlib.Path, help="Файл с result от C++ программы")
    parser.add_argument("--rtol", type=float, default=1e-9)
    parser.add_argument("--atol", type=float, default=1e-6)
    args = parser.parse_args()

    with args.input_json.open("r", encoding="utf-8") as f:
        input_data = json.load(f)
    with args.output_json.open("r", encoding="utf-8") as f:
        output_data = json.load(f)

    a = np.array(input_data["matrix_a"], dtype=np.float64)
    b = np.array(input_data["matrix_b"], dtype=np.float64)
    result_cpp = np.array(output_data["result"], dtype=np.float64)

    expected = a @ b

    if expected.shape != result_cpp.shape:
        print(f"Несовпадение формы: ожидалось {expected.shape}, получено {result_cpp.shape}")
        sys.exit(1)

    ok = bool(np.allclose(expected, result_cpp, rtol=args.rtol, atol=args.atol))
    max_abs_err = float(np.max(np.abs(expected - result_cpp)))
    denom = np.abs(expected) + 1e-12
    max_rel_err = float(np.max(np.abs((expected - result_cpp) / denom)))

    print(f"Размер:                 {a.shape[0]}x{a.shape[0]}")
    print(f"Стратегия:              {output_data.get('strategy')}")
    print(f"Время (C++) sec:          {output_data.get('elapsed_seconds')}")
    print(f"Совпадает с NumPy:      {ok}")
    print(f"Макс. абс. ошибка:      {max_abs_err:.3e}")
    print(f"Макс. отн. ошибка:      {max_rel_err:.3e}")

    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()