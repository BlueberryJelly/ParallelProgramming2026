#!/usr/bin/env python3

import argparse
import json
import os
import pathlib
import shutil
import subprocess

from validate import compare, reference


def cpu_list(cores: int) -> str:
    available = sorted(os.sched_getaffinity(0))
    return ",".join(str(cpu) for cpu in available[:cores])


def run_once(binary: pathlib.Path, input_path: pathlib.Path, output_path: pathlib.Path,
             repeats: int, threads: int | None, cores: int | None) -> tuple[dict, dict]:
    cmd = [str(binary), str(input_path), str(output_path), str(repeats)]
    if threads is not None:
        cmd.append(str(threads))
    if cores is not None:
        cmd = ["taskset", "-c", cpu_list(cores)] + cmd

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
    args = parser.parse_args()

    if not args.binary.exists():
        raise SystemExit(f"Исполняемый файл не найден: {args.binary}. Сначала соберите проект.")

    cores_list: list[int | None] = [None]
    if args.cores:
        if shutil.which("taskset") is None:
            raise SystemExit("Для --cores нужен taskset (util-linux, Linux).")
        available = len(os.sched_getaffinity(0))
        cores_list = [c for c in args.cores if c <= available]
        for c in sorted(set(args.cores) - set(cores_list)):
            print(f"Пропуск: cores={c} больше доступных ядер ({available})")

    threads_list: list[int | None] = args.threads or [None]

    args.json_out.mkdir(parents=True, exist_ok=True)
    args.jsonl_out.parent.mkdir(parents=True, exist_ok=True)

    with args.jsonl_out.open("a", encoding="utf-8") as jsonl_file:
        for n in args.sizes:
            input_path = args.data_dir / f"input_{n}.json"
            if not input_path.exists():
                print(f"Пропуск: нет файла {input_path}")
                continue
            expected = reference(input_path)

            for cores in cores_list:
                for threads in threads_list:
                    suffix = "" if threads is None else f"_t{threads}" + ("" if cores is None else f"_c{cores}")
                    output_path = args.json_out / f"output_{n}{suffix}.json"

                    record, result = run_once(args.binary, input_path, output_path,
                                              args.repeats, threads, cores)
                    record["valid"], record["max_abs_err"] = compare(expected, result["result"])
                    output_path.unlink()

                    jsonl_file.write(json.dumps(record) + "\n")
                    jsonl_file.flush()
                    print(record)


if __name__ == "__main__":
    main()
