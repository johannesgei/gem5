#!/bin/bash

# Pfade relativ zum Speicherort dieses Skripts
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
RESULTS_DIR="$SCRIPT_DIR/results"
CONFIG_SCRIPT="$SCRIPT_DIR/configs/edge_device_pim.py"
BINARY="$SCRIPT_DIR/benchmarks/pim_functional/main"

# Konfiguration der Parameter-Sweeps
SIZES=(1024 2048 4096 8192) # Hier deine gewünschten Vektorlängen eintragen
ELEMENT_SIZE=8              # 8 Byte entspricht sizeof(double)

echo "Reinige alte Simulationsergebnisse..."
rm -rf "$RESULTS_DIR"/m5out_*

echo "Starte parallele gem5 6G-Edge PIM-Simulationen..."

# Assoziatives Array für die Prozess-IDs (PIDs) deklarieren
declare -A PIDS

for N in "${SIZES[@]}"; do
  # Erstelle separates Output-Verzeichnis für jede Simulation
  OUT_DIR="$RESULTS_DIR/m5out_$N"
  mkdir -p $OUT_DIR

  # IMPORTANT: ./build/RISCV/gem5.opt must be accessible from main directory
  ./build/RISCV/gem5.opt -d "$OUT_DIR" "$CONFIG_SCRIPT" "$BINARY" "$N" "$ELEMENT_SIZE" > "$OUT_DIR/console.out" 2>&1 &

  PID=$!
  PIDS[$PID]=$N
  echo "[START] Simulation für N=$N (PID: $PID) im Hintergrund gestartet..."
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
RESULT_CSV="$RESEARCH_DIR/pim_performance_study.csv"
echo "Vector_Size,Element_Bytes,IPC,Sim_Seconds,Memory_Latency_avg" > $RESULT_CSV

for N in "${SIZES[@]}"; do
  STATS="$RESULTS_DIR/m5out_$N/stats.txt"

  if [ -f "$STATS" ]; then
    # Datenextraktion mittels grep und awk aus der stats.txt
    IPC=$(grep "board.processor.cores.core.ipc" $STATS | awk '{print $2}')
    TIME=$(grep "simSeconds" $STATS | awk '{print $2}')
    LATENCY=$(grep "board.memory.mem_ctrl.dram.avgMemAccLat" $STATS | awk '{print $2}')

    # Werte in die CSV schreiben
    echo "$N,$ELEMENT_SIZE,$IPC,$TIME,$LATENCY" >> "$RESULT_CSV"
    echo "N=$N: Element_Bytes $ELEMENT_SIZE, IPC $IPC, Sim_Seconds $TIME, Memory_Latency_avg $LATENCY extrahiert."
  else
    echo "[WARNUNG] Keine stats.txt für N=$N unter $OUT_DIR gefunden!"
  fi
done

echo "----------------------------------------------------------"
echo "FERTIG! Gesamte Datenstudie hinterlegt unter: $RESULT_CSV"
