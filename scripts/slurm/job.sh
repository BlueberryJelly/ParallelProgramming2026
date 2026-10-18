#!/bin/bash

set -u

SIZES="${SIZES:-400 600 800 1000 1200 1400 1600 1800 2000 2200 2400 2600 2800 3000}"
NPROCS="${NPROCS:-1 2 4 8 16 24 32}"
REPEATS="${REPEATS:-3}"
SEED="${SEED:-42}"
MPIRUN="${MPIRUN:-mpirun -r ssh}"

ROOT="${SLURM_SUBMIT_DIR:-$(pwd)}"
BIN="${ROOT}/lab_03"
OUT="${ROOT}/results/lab_03"
mkdir -p "$OUT"

module load "${MPI_MODULE:-intel/mpi5}" 2>/dev/null || module load intel/mpi5 2>/dev/null || true

if [ ! -x "$BIN" ]; then
    echo "ОШИБКА: не найден $BIN. Сначала соберите: bash scripts/slurm/build.sh" >&2
    exit 1
fi

TOTAL="${SLURM_NTASKS:-$(echo "$NPROCS" | tr ' ' '\n' | sort -n | tail -1)}"

{
    echo "date=$(date '+%Y-%m-%d %H:%M:%S')"
    echo "job_id=${SLURM_JOB_ID:-}"
    echo "nodelist=${SLURM_JOB_NODELIST:-$(hostname)}"
    echo "nodes=${SLURM_NNODES:-1} ntasks=${TOTAL}"
    echo "sizes=${SIZES}"
    echo "nprocs=${NPROCS}"
    echo "repeats=${REPEATS} seed=${SEED}"
    "$BIN" --help 2>/dev/null || true
    (mpirun --version 2>&1 | head -2) || true
} > "${OUT}/environment.txt"
cat "${OUT}/environment.txt"

echo "size,processes,nodes,time_seconds,compute_seconds,flops,checksum,checksum_sq" > "${OUT}/summary.csv"
: > "${OUT}/results.jsonl"

for s in $SIZES; do
    for np in $NPROCS; do
        if [ "$np" -gt "$TOTAL" ]; then
            echo "ПРОПУСК size=$s np=$np: выделено только $TOTAL слотов"
            continue
        fi
        out="${OUT}/size_${s}_np${np}.json"
        echo ">>> size=$s np=$np"
        $MPIRUN -np "$np" "$BIN" --size "$s" --seed "$SEED" --repeats "$REPEATS" --output "$out"
        if [ -f "$out" ]; then
            tr -d '\n' < "$out" | sed 's/  */ /g' >> "${OUT}/results.jsonl"
            echo >> "${OUT}/results.jsonl"
            get() { grep -o "\"$1\"[^,}]*" "$out" | head -1 | sed 's/.*: *//'; }
            echo "${s},${np},$(get nodes),$(get time_seconds),$(get compute_seconds),$(get flops),$(get checksum),$(get checksum_sq)" >> "${OUT}/summary.csv"
            rm -f "$out"
        fi
    done
done

echo "Готово. Результаты в ${OUT}/ (summary.csv, results.jsonl, environment.txt)"
