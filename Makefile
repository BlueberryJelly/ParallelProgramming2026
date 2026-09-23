-include .env

PRESET ?= release
BACKEND ?= seq
SIZES ?= 400 600 800 1000 1200 1400 1600 1800 2000 2200 2400 2600 2800 3000
REPEATS ?= 3

ifeq ($(BACKEND),seq)
  LAB ?= lab_01
else ifeq ($(BACKEND),omp)
  LAB ?= lab_02
  THREADS ?= 1 2 4 8 12
  CORES ?= 1 2 4 6
  RUN_ARGS := --threads $(THREADS) --cores $(CORES)
  VIZ_ARGS := --tables-out $(CURDIR)/reports/$(LAB)/tables.md
else
  $(error Неизвестный BACKEND=$(BACKEND), ожидалось seq или omp)
endif

BUILD_DIR := $(CURDIR)/build/$(PRESET)
SCRIPTS_DIR := $(CURDIR)/scripts
DATA_DIR := $(CURDIR)/data
RESULTS_DIR := $(CURDIR)/results/$(LAB)
REPORT_DIR := $(CURDIR)/reports/$(LAB)
FIGURES_DIR := $(REPORT_DIR)/figures
BUILD_BIN := $(BUILD_DIR)/src/$(LAB)/$(LAB)
GENERAL_JSONL := $(RESULTS_DIR)/general.jsonl
GENERAL_CSV := $(RESULTS_DIR)/general.csv

.PHONY: help all configure build generate_matrices run_experiments aggregate_jsonl_to_csv validate visualize

.DEFAULT_GOAL := help

help:
	@echo "Доступные команды:"
	@echo "configure, build, generate_matrices, run_experiments, aggregate_jsonl_to_csv, validate, visualize"
	@echo "Пример: make generate_matrices SIZES=\"200 400 800\""
	@echo "Л/р 1:  make all BACKEND=seq REPEATS=5"
	@echo "Л/р 2:  make all BACKEND=omp THREADS=\"1 2 4 8\" CORES=\"1 2 4\""

all: configure \
	 build \
	 generate_matrices \
	 run_experiments \
	 aggregate_jsonl_to_csv \
	 validate \
	 visualize

configure:
	cmake --preset $(PRESET)

build:
	@test -f "$(BUILD_DIR)/CMakeCache.txt" || $(MAKE) configure
	cmake --build --preset $(PRESET)

generate_matrices:
	@mkdir -p $(DATA_DIR)
	@rm -f $(DATA_DIR)/input_*.json
	@python3 "$(SCRIPTS_DIR)/generate_matrices.py" \
		--out-dir $(DATA_DIR) \
		--sizes $(SIZES) \
		--seed 42

run_experiments:
	@rm -rf $(RESULTS_DIR)
	@mkdir -p $(RESULTS_DIR)
	@python3 "$(SCRIPTS_DIR)/run_experiments.py" \
		--binary $(BUILD_BIN) \
		--data-dir $(DATA_DIR) \
		--sizes $(SIZES) \
		--json-out $(RESULTS_DIR) \
		--jsonl-out $(GENERAL_JSONL) \
		--repeats $(REPEATS) \
		$(RUN_ARGS)

aggregate_jsonl_to_csv:
	@rm -f $(GENERAL_CSV)
	@python3 "$(SCRIPTS_DIR)/aggregate_jsonl_to_csv.py" \
		--jsonl-path $(GENERAL_JSONL) \
		--csv-out $(GENERAL_CSV)

validate:
	@python3 "$(SCRIPTS_DIR)/validate.py" --csv $(GENERAL_CSV)

visualize:
	@rm -rf $(FIGURES_DIR)
	@mkdir -p $(FIGURES_DIR)
	@python3 "$(SCRIPTS_DIR)/visualize.py" \
		$(GENERAL_CSV) \
		--out-dir $(FIGURES_DIR) \
		$(VIZ_ARGS)
