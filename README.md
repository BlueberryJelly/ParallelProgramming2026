# ParallelProgramming2026
Лабораторные работы по параллельному программированию

## Отчёты:
- [**Лабораторная работа 1**](lab_1/report/README.md).

## Задача
Реализовать различные подходы к вычислению произведения квадратных двумерных массивов.

## Методология
Многократные опыты на разных наборах данных. Входные данные - файлы `input_N.json`, в каждом из которых записано две матрицы, заполненные случайными числами, обе имеют размерность `N*N`. Выходные данные - файлы `output_N.json`, в каждом из которых записана одна матрица размерности `N*N` - произведение входных матриц. Секундомер активируется непосредственно перед началом вычисления выходной матрицы и деактивируется, когда выходная матрица вычислена.

## Верификация
Проверку результатов выполняет скрипт `validate.py`, результатом его работы является запись вида

```bash
Размер:                 2000x2000
Стратегия:              sequential
Время (C++) sec:          74.9974
Совпадает с NumPy:      True
Макс. абс. ошибка:      3.274e-11
Макс. отн. ошибка:      1.257e-09
```

## Агрегация
Выходные данные объединяются в `general.jsonl` и приводятся к табличному формату `general.csv`.

## Визуализация
Данные из `general.csv` используются для графического отображения необходимых зависимостей.

## Результат
Результатом является отчёт в формате README.md, где используются построенные графики и полученные замеры.

## Структура проекта
```
ParallelProgramming2026/
├── data/
│   └── *.json
├── lab_X/
│   ├── src/
│   │   ├── CMakeLists.txt
│   │   └── main.cpp
│   ├── report/
│   │   ├── figures/
│   │   │   └── *.png
│   │   ├── general.csv
│   │   └── README.md
│   ├── results/
│   │   ├── *.json
│   │   └── general.jsonl
│   └── CMakeLists.txt
├── scripts/
│   ├── aggregate_jsonl_to_csv.py
│   ├── generate_matrices.py
│   ├── run_experiments.py
│   ├── validate.py
│   └── visualize.py
├── utils/
│   ├── json_utils.hpp
│   ├── matrix.hpp
│   ├── multiplier.hpp
│   └── timer.hpp
├── CMakeLists.txt
├── CMakePresets.json
├── Makefile
├── requirements.txt
├── .gitignore
└── README.md
```

## Требования

- CMake ≥ 3.20, Ninja, компилятор GCC или Clang;
- Python 3.10+, `make`;
- Python-пакеты `matplotlib`, `numpy`, `pandas`.

## Подготовка и быстрый старт

```bash
python -m venv .venv
source .venv/bin/activate        # Linux / macOS
# .venv\Scripts\Activate.bat     # Windows (PowerShell)

pip install --upgrade pip
pip install -r requirements.txt
```

## Команды Makefile

```bash
make help
make all
make configure
make build
make generate_matrices
make run_experiments
make aggregate_jsonl_to_csv
make validate
make visualize
```