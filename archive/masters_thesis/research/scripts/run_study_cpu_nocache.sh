#!/bin/bash

# Pfade relativ zum Speicherort dieses Skripts
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
RESEARCH_DIR=$(cd -- "$SCRIPT_DIR/.." &> /dev/null && pwd)
RESULTS_DIR="$RESEARCH_DIR/results"
CONFIG_SCRIPT="$RESEARCH_DIR/configs/edge_device_cpu_nocache.py"
# BINARY="$RESEARCH_DIR/benchmarks/cpu/main_double"
# BENCHMARK_BINARY="$RESEARCH_DIR/benchmarks/cpu/main_double_benchmark"
BINARY="$RESEARCH_DIR/benchmarks/cpu/main_float"
BENCHMARK_BINARY="$RESEARCH_DIR/benchmarks/cpu/main_float_benchmark"
# BINARY="$RESEARCH_DIR/benchmarks/cpu/main_FP16"
# BENCHMARK_BINARY="$RESEARCH_DIR/benchmarks/cpu/main_FP16_benchmark"

# Konfiguration der Parameter-Sweeps
SIZES=(1 2 4 8 16 32 64 128 256 512 1024 2048 4096) # Gewünschte Vektorlängen
# SIZES=($((2**14)) $((2**16)) $((2**18)) $((2**20)) $((2**22)))
# ELEMENT_SIZE=8              # 8 Byte entspricht sizeof(double)
ELEMENT_SIZE=4              # 8 Byte entspricht sizeof(float)
# ELEMENT_SIZE=2              # 8 Byte entspricht sizeof(FP16)

echo "Starte parallele gem5 CPU-Simulationen (No Cache)..."

# Assoziatives Array für die Prozess-IDs (PIDs) deklarieren
declare -A PIDS

for N in "${SIZES[@]}"; do
  # Erstelle separates Output-Verzeichnis für jede Simulation
  OUT_DIR="$RESULTS_DIR/cpu_nocache_$N"
  OUT_DIR_BENCHMARK="$RESULTS_DIR/cpu_nocache_${N}_benchmark"

  if [ -d "$OUT_DIR" ]; then
    echo "[CLEAN] Lösche alte Ergebnisse für N=$N..."
    rm -rf "$OUT_DIR"
  fi
  mkdir -p $OUT_DIR

  if [ -d "$OUT_DIR_BENCHMARK" ]; then
    echo "[CLEAN] Lösche alten Benchmark für N=$N..."
    rm -rf "$OUT_DIR_BENCHMARK"
  fi
  mkdir -p "$OUT_DIR_BENCHMARK"

  # IMPORTANT: ./build/RISCV/gem5.opt must be accessible from main directory
  ./build/RISCV/gem5.opt \
    -d "$OUT_DIR_BENCHMARK" \
    "$CONFIG_SCRIPT" "$BENCHMARK_BINARY" "$N" "$ELEMENT_SIZE" > "$OUT_DIR_BENCHMARK/console.out" 2>&1 &

  BENCH_PID=$!
  PIDS[$BENCH_PID]="${N}_benchmark"
  echo "[START] Referenz-Benchmark (PID: $BENCH_PID) gestartet."

  ./build/RISCV/gem5.opt \
    -d "$OUT_DIR" \
    "$CONFIG_SCRIPT" "$BINARY" "$N" "$ELEMENT_SIZE" > "$OUT_DIR/console.out" 2>&1 &

  PID=$!
  PIDS[$PID]=$N
  echo "[START] Simulation für N=$N (PID: $PID) gestartet."
done

# Warteschleife, bis alle Hintergrundprozesse (PIDs) beendet sind
while [ ${#PIDS[@]} -gt 0 ]; do
  for PID in "${!PIDS[@]}"; do
    # Prüfen, ob der Prozess mit dieser PID noch existiert
    if ! kill -0 $PID 2>/dev/null; then
      echo "[FINISHED] Simulation für N=${PIDS[$PID]} (PID: $PID) erfolgreich beendet!" 
      unset PIDS[$PID]  # Aus dem Array entfernen
    fi
  done
  sleep 2 # Alle 2 Sekunden prüfen
done

echo "----------------------------------------------------------"
echo "Alle Simulationen abgeschlossen. Starte Datenextraktion..."

# Ziel-CSV im Forschungsordner definieren
RESULT_CSV="$RESULTS_DIR/cpu_nocache_performance_study.csv"
TEMP_CSV="$RESULTS_DIR/cpu_nocache_performance_study_temp.csv"
echo "Vector_Size,Element_Bytes,IPC,Host_Seconds,Overhead_Host_Seconds,Pure_Host_Seconds,Sim_Seconds,Overhead_Seconds,Pure_AXPY_Seconds,Memory_Latency_avg" > $TEMP_CSV

for N in "${SIZES[@]}"; do
  STATS="$RESULTS_DIR/cpu_nocache_$N/stats.txt"
  BENCH_STATS="$RESULTS_DIR/cpu_nocache_${N}_benchmark/stats.txt"

  if [ -f "$STATS" ] && [ -f "$BENCH_STATS" ]; then
    OVERHEAD_TIME=$(grep "simSeconds" $BENCH_STATS | awk '{print $2}')
    OVERHEAD_HOST_TIME=$(grep "hostSeconds" $BENCH_STATS | awk '{print $2}')

    IPC=$(grep "board.processor.cores.core.ipc" $STATS | awk '{print $2}')
    HOST_TIME=$(grep "hostSeconds" $STATS | awk '{print $2}')
    TIME=$(grep "simSeconds" $STATS | awk '{print $2}')
    LATENCY=$(grep "board.memory.mem_ctrl.dram.avgMemAccLat" $STATS | awk '{print $2}')

    PURE_AXPY_TIME=$(python3 -c "print(round($TIME - $OVERHEAD_TIME, 6))" 2>/dev/null)
    if [ -z "$PURE_AXPY_TIME" ]; then
      PURE_AXPY_TIME="N/A"
    fi
    PURE_HOST_TIME=$(python3 -c "print(round($HOST_TIME - $OVERHEAD_HOST_TIME, 6))" 2>/dev/null)
    if [ -z "$PURE_HOST_TIME" ]; then
      PURE_HOST_TIME="N/A"
    fi

    # Werte in die CSV schreiben
    echo "$N,$ELEMENT_SIZE,$IPC,$HOST_TIME,$OVERHEAD_HOST_TIME,$PURE_HOST_TIME,$TIME,$OVERHEAD_TIME,$PURE_AXPY_TIME,$LATENCY" >> "$TEMP_CSV"
    echo "N=$N: Element_Bytes $ELEMENT_SIZE, IPC $IPC, PURE_AXPY_TIME $PURE_AXPY_TIME s (Total: $TIME s), Latency $LATENCY extrahiert."
  fi
done

echo "Zusammenführen und Sortieren der CSV-Daten..."

python3 -c "
import os
import csv

final_path = '$RESULT_CSV'
temp_path = '$TEMP_CSV'

data_dict = {}

# 1. Zuerst alte Daten laden (falls vorhanden)
if os.path.exists(final_path) and os.path.getsize(final_path) > 0:
    with open(final_path, mode='r', newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        header = reader.fieldnames
        for row in reader:
            # Nutze die Vector_Size als Schlüssel
            data_dict[int(row['Vector_Size'])] = row
else:
    # Standard-Header falls die Datei komplett neu angelegt wird
    header = ['Vector_Size', 'Element_Bytes', 'IPC', 'Host_Seconds', 'Overhead_Host_Seconds', 'Pure_Host_Seconds', 'Sim_Seconds', 'Overhead_Seconds', 'Pure_AXPY_Seconds', 'Memory_Latency_avg']

# 2. Neue Daten laden und bestehende Keys überschreiben
if os.path.exists(temp_path):
    with open(temp_path, mode='r', newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            data_dict[int(row['Vector_Size'])] = row

# 3. Keys aufsteigend sortieren
sorted_sizes = sorted(data_dict.keys())

# 4. Zurück in die finale Datei schreiben
with open(final_path, mode='w', newline='', encoding='utf-8') as f:
    writer = csv.DictWriter(f, fieldnames=header)
    writer.writeheader()
    for size in sorted_sizes:
        writer.writerow(data_dict[size])
"

# Temporäre Datei aufräumen
rm -f "$TEMP_CSV"

echo "----------------------------------------------------------"
echo "FERTIG! Gesamte Datenstudie hinterlegt unter: $RESULT_CSV"
