# Параллельное программирование

Цикл лабораторных работ по умножению квадратных матриц: последовательная реализация, OpenMP, MPI (суперкомпьютер «Сергей Королёв») и CUDA. Исследуемые алгоритмы написаны на C++, а генерация данных, проверка результатов и построение графиков — на Python.

| Лабораторная | Тема | Исходники | Отчёт | Результаты |
|---|---|---|---|---|
| lab_01 | Последовательное умножение (тройной цикл) | [src/lab_01](src/lab_01) | [reports/lab_01](reports/lab_01/README.md) | [results/lab_01](results/lab_01) |
| lab_02 | — | [src/lab_02](src/lab_02) | [reports/lab_02](reports/lab_02) | [results/lab_02](results/lab_02) |
| lab_03 | — | [src/lab_03](src/lab_03) | [reports/lab_03](reports/lab_03) | [results/lab_03](results/lab_03) |
| lab_04 | — | [src/lab_04](src/lab_04) | [reports/lab_04](reports/lab_04) | [results/lab_04](results/lab_04) |
| lab_05 | — | [src/lab_05](src/lab_05) | [reports/lab_05](reports/lab_05) | [results/lab_05](results/lab_05) |

## Структура репозитория

```text
data/                 входные матрицы input_<N>.json (генерируются, в Git не хранятся)
reports/lab_XX/       отчёт README.md и графики figures/
results/lab_XX/       результаты замеров: general.jsonl, general.csv, output_<N>.json (в Git хранится .csv)
scripts/              генерация данных, запуск замеров, агрегация, проверка и построение графиков
src/utils/            общие заголовочные файлы утилит
src/lab_XX/           исходники лабораторной и её CMakeLists.txt
CMakeLists.txt        корневой проект, подключает src/lab_XX
CMakePresets.json     пресеты сборки release / debug (Ninja)
Makefile              единая точка входа
requirements.txt      зависимости Python
```

## Требования

CMake ≥ 3.20, Ninja, компилятор с поддержкой C++20 (GCC, Clang или MSVC), Python ≥ 3.10.

Для `lab_04` (CUDA) дополнительно требуются CUDA Toolkit (`nvcc`) и GPU NVIDIA. При их отсутствии `lab_04` автоматически исключается из сборки при конфигурации, а остальные лабораторные собираются как обычно.

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env
```

## Запуск

Полный конвейер для лабораторной (сборка, генерация данных, замеры, формирование CSV, проверка и построение графиков):

```bash
make all                       # lab_01, пресет release
make all SIZES="200 400 800"   # свой набор размеров
make build PRESET=debug        # отладочная сборка в build/debug
make all BACKEND=omp           # lab_02: серия замеров по потокам и ядрам
make all BACKEND=omp THREADS="1 2 4 8" CORES="1 2 4" REPEATS=5
make all BACKEND=cuda          # lab_04: серия замеров по конфигурациям блока CUDA
make all BACKEND=cuda BLOCKS="4 8 16 32" SIZES="512 1024 2048 4096" REPEATS=5
```

Отдельные шаги: `configure`, `build`, `generate_matrices`, `run_experiments`, `aggregate_jsonl_to_csv`, `validate`, `visualize`.

Без Make сборка выполняется следующим образом:

```bash
cmake --preset release
cmake --build --preset release
./build/release/src/lab_01/lab_01 data/input_1000.json results/lab_01/output_1000.json
./build/release/src/lab_04/lab_04 data/input_1000.json results/lab_04/output_1000.json 3 16 16
```

## Методика исследования

Входные данные — пары квадратных матриц `double` размера N × N с элементами, равномерно распределёнными на отрезке [-10, 10]. Генератор использует фиксированный `seed = 42` для первой матрицы и `seed = 43` для второй, что обеспечивает воспроизводимость данных.

Размеры матриц:

```text
400 600 800 1000 1200 1400 1600 1800 2000 2200 2400 2600 2800 3000
```

Программа читает `matrix_a` и `matrix_b` из JSON, умножает их и записывает результат вместе с метриками в `output_<N>.json`.

Для каждого размера фиксируются:

* `elapsed_seconds` — время только самого умножения, измеренное с помощью `std::chrono::steady_clock` внутри программы (без чтения и записи JSON);
* `flops` — число операций с плавающей точкой, `2·N·M·P` (умножения и сложения);
* `memory_bytes` — объём памяти, занимаемый тремя матрицами (A, B и результатом), в байтах.

Для многопоточных сценариев (OpenMP) дополнительно фиксируются:

* `threads` — число потоков;
* `cores` — число логических процессоров, выделенных процессу с помощью `taskset -c` (только Linux).

Для сценариев на CUDA (`lab_04`) дополнительно фиксируются:

* `block_x`, `block_y` — размерность блока потоков (`blockDim`), задаваемая аргументами командной строки;
* `grid_x`, `grid_y` — размерность сетки блоков (`gridDim`), вычисляемая как `ceil(N / blockDim)`;
* `gpu_name`, `gpu_compute_capability`, `gpu_multiprocessors` — сведения об использованном GPU, полученные с помощью `cudaGetDeviceProperties`.

`elapsed_seconds` для CUDA включает копирование матриц на устройство, выполнение ядра и копирование результата обратно (полный цикл `multiply()`), то есть отражает время полного выполнения операции, которое фактически затрачивает программа.

Умножение повторяется `repeats` раз в рамках одного запуска. В `elapsed_seconds` записывается медианное время, а в `elapsed_min_seconds` — минимальное.

Ускорение рассчитывается как `S = T₁ / Tₚ` при том же числе ядер, эффективность — как `E = S / p`.

Результат каждого запуска сверяется с результатом NumPy (`a @ b`) с помощью `np.allclose` с параметрами `rtol = 1e-9` и `atol = 1e-6`. Выводятся максимальные абсолютная и относительная ошибки.

Проверка выполняется сразу после каждого запуска. Итоговые значения попадают в колонки `valid` и `max_abs_err`, после чего полные файлы `output_*.json` удаляются.