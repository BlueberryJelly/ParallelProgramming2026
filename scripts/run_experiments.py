#!/usr/bin/env python3

import argparse
import json
import pathlib
import subprocess
import time

DEFAULT_SIZES = [200, 400, 800, 1200, 1600, 2000]


def run_once(binary: pathlib.Path, input_path: pathlib.Path, output_path: pathlib.Path) -> dict:
    start = time.time()
    subprocess.run(
        [str(binary), str(input_path), str(output_path)],
        check=True,
    )
    wall_time = time.time() - start

    with output_path.open("r", encoding="utf-8") as f:
        result = json.load(f)

    return {
        "size": result["rows_a"],
        "elapsed_seconds": result["elapsed_seconds"],
        "wall_time_seconds": wall_time,
        "flops": result["flops"],
        "memory_bytes": result["memory_bytes"],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=pathlib.Path, default=pathlib.Path("build/lab_1/src/lab_1"))
    parser.add_argument("--data-dir", type=pathlib.Path, default=pathlib.Path("data"))
    parser.add_argument("--sizes", type=int, nargs="+", default=DEFAULT_SIZES)
    parser.add_argument("--json-out", type=pathlib.Path, default=pathlib.Path("lab_1/results"))
    parser.add_argument("--jsonl-out", type=pathlib.Path, default=pathlib.Path("lab_1/results/general.jsonl"))
    args = parser.parse_args()

    if not args.binary.exists():
        raise SystemExit(f"Бинарник не найден: {args.binary}. Сначала соберите проект через CMake.")

    args.jsonl_out.parent.mkdir(parents=True, exist_ok=True)

    with args.jsonl_out.open("a", encoding="utf-8") as jsonl_file:
        for n in args.sizes:
            input_path = args.data_dir / f"input_{n}.json"
            if not input_path.exists():
                print(f"Пропуск: нет файла {input_path} (запустите generate_matrices.py)")
                continue

            output_path = args.json_out / f"output_{n}.json"
            record = run_once(args.binary, input_path, output_path)
            jsonl_file.write(json.dumps(record) + "\n")
            jsonl_file.flush()
            print(record)


if __name__ == "__main__":
    main()