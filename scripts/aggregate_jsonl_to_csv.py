#!/usr/bin/env python3

import argparse
import csv
import json
import pathlib


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("jsonl_files", type=pathlib.Path, nargs="+")
    parser.add_argument("--csv-out", type=pathlib.Path, default=pathlib.Path("lab_1/report/general.csv"))
    args = parser.parse_args()

    rows = []
    for jsonl_path in args.jsonl_files:
        with jsonl_path.open("r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line:
                    rows.append(json.loads(line))

    if not rows:
        print("Нет данных для агрегации")
        return

    fieldnames = sorted({key for row in rows for key in row.keys()})
    args.csv_out.parent.mkdir(parents=True, exist_ok=True)

    with args.csv_out.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Записано {len(rows)} строк в {args.csv_out}")


if __name__ == "__main__":
    main()