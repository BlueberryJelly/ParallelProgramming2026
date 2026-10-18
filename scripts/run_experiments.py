#!/usr/bin/env python3

import argparse
import json
import os
import pathlib
import shlex
import shutil
import subprocess
from typing import List, Optional, Tuple

from validate import compare, reference


def cpu_list(cores: int) -> str:
    available = sorted(os.sched_getaffinity(0))
    return ",".join(str(cpu) for cpu in available[:cores])


def run_once(binary: pathlib.Path, input_path: pathlib.Path, output_path: pathlib.Path,
             repeats: int, threads: Optional[int], cores: Optional[int],
             block: Optional[Tuple[int, int]] = None,
             procs: Optional[int] = None, mpirun: str = "mpirun -np {procs}") -> Tuple[dict, dict]:
    cmd = [str(binary), str(input_path), str(output_path), str(repeats)]
    if threads is not None:
        cmd.append(str(threads))
    if block is not None:
        cmd.extend([str(block[0]), str(block[1])])
    if cores is not None:
        cmd = ["taskset", "-c", cpu_list(cores)] + cmd
    if procs is not None:
        cmd = shlex.split(mpirun.format(procs=procs)) + cmd

    subprocess.run(cmd, check=True)
    with output_path.open("r", encoding="utf-8") as f:
        result = json.load(f)

    record = {
        "size": result["rows_a"],
        "strategy": result["strategy"],
        "repeats": result["repeats"],
        "elapsed_seconds": result["elapsed_seconds"],
        "flops": result["flops"],
        "memory_bytes": result["memory_bytes"],
    }
    if threads is not None:
        record["threads"] = result["threads"]
        record["cores"] = cores if cores is not None else result["available_procs"]
    if block is not None:
        record["block_x"] = result["block_x"]
        record["block_y"] = result["block_y"]
        record["threads_per_block"] = result["threads_per_block"]
        record["grid_x"] = result["grid_x"]
        record["grid_y"] = result["grid_y"]
        record["gpu_name"] = result["gpu_name"]
    if procs is not None:
        record["processes"] = result["processes"]
        record["nodes"] = result["nodes"]
        record["node_names"] = " ".join(result["node_names"])
        record["elapsed_min_seconds"] = result["elapsed_min_seconds"]
        record["compute_seconds"] = result["compute_seconds"]
        record["comm_seconds"] = max(result["elapsed_seconds"] - result["compute_seconds"], 0.0)
    return record, result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=pathlib.Path,
                        required=True, help="Путь к исполняемому файлу")
    parser.add_argument("--data-dir", type=pathlib.Path,
                        required=True, help="Директория с входными данными")
    parser.add_argument("--sizes", type=int, nargs="+",
                        required=True, help="Размеры входных матриц")
    parser.add_argument("--json-out", type=pathlib.Path,
                        required=True, help="Директория для выходных данных")
    parser.add_argument("--jsonl-out", type=pathlib.Path,
                        required=True, help="Файл для агрегированных выходных данных")
    parser.add_argument("--repeats", type=int, default=1,
                        help="Число повторов умножения внутри одного запуска (берётся медиана)")
    parser.add_argument("--threads", type=int, nargs="+", default=None,
                        help="Числа потоков (OpenMP)")
    parser.add_argument("--cores", type=int, nargs="+", default=None,
                        help="Числа ядер, выделяемых через taskset (OpenMP)")
    parser.add_argument("--block-sizes", type=int, nargs="+", default=None,
                        help="Размеры квадратного блока CUDA (block_x = block_y)")
    parser.add_argument("--procs", type=int, nargs="+", default=None,
                        help="Числа MPI-процессов")
    parser.add_argument("--mpirun", type=str, default="mpirun -np {procs}",
                        help="Шаблон команды запуска MPI, {procs} заменяется числом процессов")
    args = parser.parse_args()

    if sum(bool(x) for x in (args.block_sizes, args.threads or args.cores, args.procs)) > 1:
        raise SystemExit("--block-sizes, --threads/--cores и --procs относятся к разным лабораторным")

    if not args.binary.exists():
        raise SystemExit("Исполняемый файл не найден: {}. Сначала соберите проект.".format(args.binary))

    cores_list = [None]
    if args.cores:
        if shutil.which("taskset") is None:
            raise SystemExit("Для --cores нужен taskset (util-linux, Linux).")
        available = len(os.sched_getaffinity(0))
        cores_list = [c for c in args.cores if c <= available]
        for c in sorted(set(args.cores) - set(cores_list)):
            print("Пропуск: cores={} больше доступных ядер ({})".format(c, available))

    threads_list = args.threads or [None]
    blocks_list = (
        [(b, b) for b in args.block_sizes] if args.block_sizes else [None]
    )
    procs_list = args.procs or [None]

    args.json_out.mkdir(parents=True, exist_ok=True)
    args.jsonl_out.parent.mkdir(parents=True, exist_ok=True)

    with args.jsonl_out.open("a", encoding="utf-8") as jsonl_file:
        for n in args.sizes:
            input_path = args.data_dir / "input_{}.json".format(n)
            if not input_path.exists():
                print("Пропуск: нет файла {}".format(input_path))
                continue
            expected = reference(input_path)

            for block in blocks_list:
                for cores in cores_list:
                    for threads in threads_list:
                        for procs in procs_list:
                            suffix = "" if threads is None else "_t{}".format(threads) + (
                                "" if cores is None else "_c{}".format(cores))
                            if block is not None:
                                suffix += "_b{}x{}".format(block[0], block[1])
                            if procs is not None:
                                suffix += "_p{}".format(procs)
                            output_path = args.json_out / "output_{}{}.json".format(n, suffix)

                            record, result = run_once(args.binary, input_path, output_path,
                                                      args.repeats, threads, cores, block,
                                                      procs, args.mpirun)
                            record["valid"], record["max_abs_err"] = compare(expected, result["result"])
                            del result
                            output_path.unlink()

                            jsonl_file.write(json.dumps(record) + "\n")
                            jsonl_file.flush()
                            print(record, flush=True)


if __name__ == "__main__":
    main()
