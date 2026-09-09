#!/usr/bin/env python3

import argparse
import json
import pathlib

import numpy as np

DEFAULT_SIZES = [200, 400, 800, 1200, 1600, 2000]


def generate_matrix(n: int, seed: int) -> np.ndarray:
    rng = np.random.default_rng(seed)
    return rng.uniform(-10.0, 10.0, size = (n, n))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out-dir", type=pathlib.Path,
                        default=pathlib.Path("data"))
    parser.add_argument("--sizes", type=int, nargs="+",
                        default=DEFAULT_SIZES)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)

    for n in args.sizes:
        a = generate_matrix(n, args.seed)
        b = generate_matrix(n, args.seed + 1)

        payload = {
            "matrix_a": a.tolist(),
            "matrix_b": b.tolist(),
            "sizes": n,
        }

        out_path = args.out_dir / f"input_{n}.json"
        with out_path.open("w", encoding="utf-8") as f:
            json.dump(payload, f)

        print(f"Сгенерировано: {out_path} ({n}*{n})")


if __name__ == "__main__":
    main()