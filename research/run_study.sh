#!/bin/bash

# Paths relative to this script
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
RESULTS_DIR="$SCRIPT_DIR/results"
CONFIG_SCRIPT="$SCRIPT_DIR/configs/edge_device_qr.py"

# Array with matrix sizes
# SIZES=(100 200 400 600 1000)
SIZES=(100)

echo "Cleaning old results..."
rm -rf "$RESULTS_DIR"/m5out_*

echo "Starting parallel simulations in research folder..."

for N in "${SIZES[@]}"; do
  OUT_DIR="$RESULTS_DIR/m5out_$N"
  mkdir -p $OUT_DIR
  BINARY="research/benchmarks/qr/qr_rv_$N"

  # IMPORTANT: ./build/RISCV/gem5.opt must be accessible from main directory
  ./build/RISCV/gem5.opt -d "$OUT_DIR" "$CONFIG_SCRIPT" "$BINARY" > "$OUT_DIR/console.out" 2>&1 &

  PID=$!
  PIDS[$PID]=$N
  echo "[START] Simulation for N=$N (PID: $PID) running..."

done

# Loop, until all PIDs are removed from array (i.e., all simulations finished)
while [ ${#PIDS[@]} -gt 0 ]; do
  for PID in "${!PIDS[@]}"; do
    # Check if process is still existent
    if ! kill -0 $PID 2>/dev/null; then
      echo "[FINISHED] Simulation for N=${PIDS[$PID]} (PID: $PID) finished!"
      unset PIDS[$PID]  # Remove from array
    fi
  done
  sleep 2 # Check every 2 seconds
done

echo "----------------------------------------------------------"
echo "All simulations finished. Extracting data..."

# Result CSV directly in the research-folders
RESULT_CSV="$RESEARCH_DIR/sensing_performance_study.csv"
echo "Matrix_Size,IPC,Sim_Seconds,Memory_Latency_avg" > $RESULT_CSV

for N in "${SIZES[@]}"; do
  STATS="$RESULTS_DIR/m5out_$N/stats.txt"

  if [ -f "$STATS" ]; then
    IPC=$(grep "board.processor.cores.core.ipc" $STATS | awk '{print $2}')
    TIME=$(grep "simSeconds" $STATS | awk '{print $2}')
    LATENCY=$(grep "board.memory.mem_ctrl.dram.avgMemAccLat" $STATS | awk '{print $2}')

    echo "$N,$IPC,$TIME,$LATENCY" >> $RESULT_CSV
    echo "N=$N: IPC $IPC extracted."
  fi
done

echo "FINISHED! Results under: $RESULT_CSV"
