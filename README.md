# Параллельное программирование

Цикл лабораторных работ по умножению квадратных матриц: последовательная реализация, OpenMP, MPI (суперкомпьютер «Сергей Королёв»), CUDA. Исследуемые алгоритмы написаны на C++, генерация данных, проверка результатов и построение графиков — на Python.

| Лабораторная | Тема | Исходники | Отчёт | Результаты |
|---|---|---|---|---|
| lab_01 | Последовательное умножение (тройной цикл) | [src/lab_01](src/lab_01) | [reports/lab_01](reports/lab_01/README.md) | [results/lab_01](results/lab_01) |
| lab_02 | — | [src/lab_02](src/lab_02) | [reports/lab_02](reports/lab_02) | [results/lab_02](results/lab_02) |
| lab_03 | — | [src/lab_03](src/lab_03) | [reports/lab_03](reports/lab_03) | [results/lab_03](results/lab_03) |
| lab_04 | — | [src/lab_04](src/lab_04) | [reports/lab_04](reports/lab_04) | [results/lab_04](results/lab_04) |
| lab_05 | — | [src/lab_05](src/lab_05) | [reports/lab_05](reports/lab_05) | [results/lab_05](results/lab_05) |

## Структура репозитория

```
data/                 входные матрицы input_<N>.json (генерируются, в git не хранятся)
reports/lab_XX/       отчёт README.md и графики figures/
results/lab_XX/       результаты замеров: general.jsonl, general.csv, output_<N>.json (в git хранятся .jsonl, .csv)
scripts/              генерация данных, запуск замеров, агрегация, проверка, графики
src/utils/            общие заголовочные заголовочные утилит: Matrix, JSON, Timer, стратегии умножения
src/lab_XX/           исходники лабораторной и её CMakeLists.txt
.env.example          пример локальных настроек для Makefile
CMakeLists.txt        корневой проект, подключает src/lab_XX
CMakePresets.json     пресеты сборки release / debug (Ninja)
Makefile              единая точка входа
requirements.txt      зависимости Python
```

## Требования

CMake ≥ 3.20, Ninja, компилятор с поддержкой C++20 (GCC, Clang или MSVC), Python ≥ 3.10.

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
```

## Запуск

```bash
make all
make all SIZES="200 400 800"
make build PRESET=debug
```

Отдельные шаги: `configure`, `build`, `generate_matrices`, `run_experiments`, `aggregate_jsonl_to_csv`, `validate`, `visualize`. Без Make сборка выполняется так:

```bash
cmake --preset release
cmake --build --preset release
./build/release/src/lab_01/lab_01 data/input_1000.json results/lab_01/output_1000.json
```

## Методика исследования

Входные данные — пары квадратных матриц `double` размера N×N с элементами, равномерно распределёнными на [-10, 10]; генератор NumPy с фиксированным seed = 42 (для второй матрицы seed + 1), что делает данные воспроизводимыми.

Программа читает `matrix_a` и `matrix_b` из JSON, умножает их и записывает результат вместе с метриками в `output_<N>.json`. Для каждого размера фиксируются:

- `elapsed_seconds` — время только самого умножения, измеренное `std::chrono::steady_clock` внутри программы (без чтения и записи JSON);
- `wall_time_seconds` — полное время запуска процесса, измеренное из Python;
- `flops` — число операций с плавающей точкой, 2·N·M·P (умножения и сложения);
- `memory_bytes` — объём трёх матриц (A, B, результат) в байтах.

Результат каждого запуска сверяется с `numpy` (`a @ b`) через `np.allclose` с `rtol = 1e-9`, `atol = 1e-6`; выводятся максимальные абсолютная и относительная ошибки. Производительность на графиках считается как `flops / elapsed_seconds`.
