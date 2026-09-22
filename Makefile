-include .env

PRESET ?= release
BACKEND ?= seq
ifeq ($(BACKEND),seq)
  LAB ?= lab_01
endif

BUILD_DIR := $(CURDIR)/build/$(PRESET)
SCRIPTS_DIR := $(CURDIR)/scripts
DATA_DIR := $(CURDIR)/data
RESULTS_DIR := $(CURDIR)/results/$(LAB)
REPORT_DIR := $(CURDIR)/reports/$(LAB)
FIGURES_DIR := $(REPORT_DIR)/figures
BUILD_BIN := $(BUILD_DIR)/src/$(LAB)/$(LAB)
GENERAL_JSON := $(RESULTS_DIR)/general.jsonl
GENERAL_CSV := $(RESULTS_DIR)/general.csv


SIZES ?= 250 500 750 1000 1250 1500 1750 2000

.PHONY: help all configure build generate_matrices run_experiments aggregate_jsonl_to_csv validate visualize

.DEFAULT_GOAL := help

help:
	@echo "Доступные команды:"
	@sed -n 's/^## //p' $(MAKEFILE_LIST) | column -t -s ':'
	@echo "configure, build, generate_matrices, run_experiments, aggregate_jsonl_to_csv, validate, visualize"
	@echo "Пример: make generate_matrices SIZES=\"200 400 800\""

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
		--jsonl-out $(GENERAL_JSON)

aggregate_jsonl_to_csv:
	@rm -f $(GENERAL_CSV)
	@python3 "$(SCRIPTS_DIR)/aggregate_jsonl_to_csv.py" \
		$(GENERAL_JSON) \
		--csv-out $(GENERAL_CSV)

validate:
	@for s in $(SIZES); do \
		python3 "$(SCRIPTS_DIR)/validate.py" \
			"$(DATA_DIR)/input_$$s.json" \
			"$(RESULTS_DIR)/output_$$s.json"; \
	done

visualize:
	@rm -rf $(FIGURES_DIR)
	@mkdir -p $(FIGURES_DIR)
	@python3 "$(SCRIPTS_DIR)/visualize.py" \
	$(GENERAL_CSV) \
	--out-dir $(FIGURES_DIR)
