-include .env

PRESET ?= release
BACKEND ?= seq
ifeq ($(BACKEND),seq)
  LAB ?= lab_01
else ifeq ($(BACKEND),omp)
  LAB ?= lab_02
  THREADS ?= 1 2 4 8 12
  CORES ?= 1 2 4 6
  REPEATS ?= 3
  RUN_ARGS := --threads $(THREADS) --cores $(CORES) --repeats $(REPEATS) --validate
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


SIZES := 50 100 150 200 250 300 350 400 450 500 550 600 \
650 700 750 800 850 900 950 1000 1050 1100 1150 1200 \
1250 1300 1350 1400 1450 1500 1550 1600 1650 1700 1750 1800 1850 1900 1950 2000


.PHONY: help all configure build generate_matrices run_experiments aggregate_jsonl_to_csv validate visualize

.DEFAULT_GOAL := help

help:
	@echo "Доступные команды:"
	@sed -n 's/^## //p' $(MAKEFILE_LIST) | column -t -s ':'
	@echo "configure, build, generate_matrices, run_experiments, aggregate_jsonl_to_csv, validate, visualize"
	@echo "Пример: make generate_matrices SIZES=\"200 400 800\""
	@echo "Л/р 1:  make all BACKEND=seq"
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
		$(RUN_ARGS)

aggregate_jsonl_to_csv:
	@rm -f $(GENERAL_CSV)
	@python3 "$(SCRIPTS_DIR)/aggregate_jsonl_to_csv.py" \
		--jsonl-path $(GENERAL_JSONL) \
		--csv-out $(GENERAL_CSV)

validate:
ifneq ($(RUN_ARGS),)
	@python3 "$(SCRIPTS_DIR)/validate.py" --csv $(GENERAL_CSV)
else
	@for s in $(SIZES); do \
		python3 "$(SCRIPTS_DIR)/validate.py" \
			"$(DATA_DIR)/input_$$s.json" \
			"$(RESULTS_DIR)/output_$$s.json"; \
	done
endif

visualize:
	@rm -rf $(FIGURES_DIR)
	@mkdir -p $(FIGURES_DIR)
	@python3 "$(SCRIPTS_DIR)/visualize.py" \
	$(GENERAL_CSV) \
	--out-dir $(FIGURES_DIR) \
	$(VIZ_ARGS)
