#!/usr/bin/env python3

import argparse
import json
import os
import pathlib
import shutil
import subprocess
import time

import numpy as np


def cpu_list(cores: int) -> str:
    available = sorted(os.sched_getaffinity(0))
    return ",".join(str(cpu) for cpu in available[:cores])


def run_once(binary: pathlib.Path, input_path: pathlib.Path, output_path: pathlib.Path,
             threads: int | None, cores: int | None, repeats: int) -> dict:
    cmd = [str(binary), str(input_path), str(output_path)]
    if threads is not None:
        cmd += [str(threads), str(repeats)]
    if cores is not None:
        cmd = ["taskset", "-c", cpu_list(cores)] + cmd

    start = time.time()
    subprocess.run(cmd, check=True)
    wall_time = time.time() - start

    with output_path.open("r", encoding="utf-8") as f:
        result = json.load(f)

    record = {
        "size": result["rows_a"],
        "elapsed_seconds": result["elapsed_seconds"],
        "wall_time_seconds": wall_time,
        "flops": result["flops"],
        "memory_bytes": result["memory_bytes"],
    }
    if threads is not None:
        record.update({
            "threads": result["threads"],
            "cores": cores if cores is not None else result["available_procs"],
            "available_procs": result["available_procs"],
            "repeats": result["repeats"],
            "elapsed_min_seconds": result["elapsed_min_seconds"],
        })
    return record, result


def check_result(input_path: pathlib.Path, result: dict, cache: dict) -> tuple[bool, float]:
    if input_path not in cache:
        cache.clear()
        with input_path.open("r", encoding="utf-8") as f:
            data = json.load(f)
        a = np.array(data["matrix_a"], dtype=np.float64)
        b = np.array(data["matrix_b"], dtype=np.float64)
        cache[input_path] = a @ b
    expected = cache[input_path]
    actual = np.array(result["result"], dtype=np.float64)
    if expected.shape != actual.shape:
        return False, float("inf")
    ok = bool(np.allclose(expected, actual, rtol=1e-9, atol=1e-6))
    return ok, float(np.max(np.abs(expected - actual)))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=pathlib.Path, 
                        required=True, help="Путь к исполняемому объектному коду")
    parser.add_argument("--data-dir", type=pathlib.Path, 
                        required=True, help="Путь к дериктории с входными данными")
    parser.add_argument("--sizes", type=int, nargs="+",
                        required=True, help="Размеры входных матриц")
    parser.add_argument("--json-out", type=pathlib.Path, 
                        required=True, help="Директория для выходных данных")
    parser.add_argument("--jsonl-out", type=pathlib.Path,
                        required=True, help="Директория для агрегированных выходных данных")
    parser.add_argument("--threads", type=int, nargs="+", default=None,
                        help="Числа потоков")
    parser.add_argument("--cores", type=int, nargs="+", default=None,
                        help="Числа ядер")
    parser.add_argument("--repeats", type=int, default=1,
                        help="Число повторов исследуемого алгоритма")
    parser.add_argument("--validate", action="store_true",
                        help="Сверять каждый результат с NumPy прямо во время замеров")
    parser.add_argument("--keep-outputs", action="store_true",
                        help="Не удалять output_*.json после проверки (с --validate)")
    args = parser.parse_args()

    if not args.binary.exists():
        raise SystemExit(f"Объектный код не найден: {args.binary}. Сначала соберите проект.")

    cores_list: list[int | None] = [None]
    if args.cores:
        if shutil.which("taskset") is None:
            raise SystemExit("Для --cores нужен taskset (util-linux, Linux).")
        available = len(os.sched_getaffinity(0))
        cores_list = [c for c in args.cores if c <= available]
        for c in sorted(set(args.cores) - set(cores_list)):
            print(f"Пропуск: число cores={c} больше доступных ядер ({available})")

    threads_list: list[int | None] = args.threads or [None]

    args.json_out.mkdir(parents=True, exist_ok=True)
    args.jsonl_out.parent.mkdir(parents=True, exist_ok=True)
    expected_cache: dict = {}

    with args.jsonl_out.open("a", encoding="utf-8") as jsonl_file:
        for n in args.sizes:
            input_path = args.data_dir / f"input_{n}.json"
            if not input_path.exists():
                print(f"Пропуск: нет файла {input_path}")
                continue

            for cores in cores_list:
                for threads in threads_list:
                    if threads is None:
                        output_path = args.json_out / f"output_{n}.json"
                    else:
                        suffix = f"_c{cores}" if cores is not None else ""
                        output_path = args.json_out / f"output_{n}_t{threads}{suffix}.json"

                    record, result = run_once(args.binary, input_path, output_path,
                                              threads, cores, args.repeats)
                    if args.validate:
                        record["valid"], record["max_abs_err"] = check_result(input_path, result, expected_cache)
                        if not args.keep_outputs:
                            output_path.unlink()

                    jsonl_file.write(json.dumps(record) + "\n")
                    jsonl_file.flush()
                    print(record)


if __name__ == "__main__":
    main()
