# Schritt 1: Plattform und Metriken einfrieren

Dieses Dokument ist der Vertrag für alle Energie-Plots, bis wir ihn
bewusst ersetzen. `run_6g_sim_riscv.py` wird **nicht** verwendet.

Keine Config-Änderungen an den unten genannten Dateien, solange die
DRAM-Energie-Kette (Schritt 2+) läuft.

---

## 1. Welches Hardware-Paar gilt

| Rolle | Config | Caches | CPU | Takt | DRAM |
|---|---|---|---|---|---|
| CPU-Hauptpfad | `research/configs/edge_device_cpu_withcaches.py` | L1I/L1D 32 KiB, L2 256 KiB | RISC-V Minor, 1 Kern | 2 GHz | LPDDR5-6400, 1×16, BG, BL32, 1024 MiB |
| PIM-Pfad | `research/configs/edge_device_pim.py` | NoCache | gleich | 2 GHz | gleiche LPDDR5-Klasse, plus `PIMMemCtrl` |
| Optional extra | `research/configs/edge_device_cpu_nocache.py` | NoCache | gleich | 2 GHz | gleich, ohne PIM |

Haupt-A/B ist **withcaches vs PIM**. Nocache ist nur eine Zusatzkurve
(„was kostet der Speicher ohne LLC“), kein drittes Chip-Modell.

PIM-Mailbox: `pim_base_addr=0x79000`. Die RISC-V-Binaries
`research/benchmarks/pim_functional/main` und `main_benchmark` müssen
`.pim_regs` an derselben Adresse haben
(`-Wl,--section-start=.pim_regs=0x79000`).

---

## 2. Welches Kernel-Paar für den ersten Energie-Beweis (Schritt 2)

Noch AXPY aus der Thesis, nicht die 6G-`.cpp`-Kernel.

| Mode | Binary | `ELEMENT_SIZE` | FLOPs |
|---|---|---|---|
| CPU | `research/benchmarks/cpu/main_double_uncached` | 8 | `2 * N` |
| CPU-Overhead (Thesis-ROI) | `.../main_double_uncached_benchmark` | 8 | 0 (nur Setup) |
| PIM | `research/benchmarks/pim_functional/main` | 4 (Skript-Default) | `2 * N` |
| PIM-Overhead | `.../main_benchmark` | 4 | 0 |

Sweeps: vorhandene `research/scripts/run_study_cpu_withcaches.sh` und
`run_study_pim.sh`. Skripte mit `bash research/scripts/...` starten
(Workspace-Mount ist oft `noexec`, daher kein `./skript.sh`).
Arbeitsverzeichnis: Repo-Root, damit `./build/RISCV/gem5.opt` gefunden wird.

Für Schritt 1 **keinen** neuen Sweep fahren. Schritt 2 nutzt eine
bereits vorhandene `stats.txt` oder einen einzelnen Smoke-Lauf.

---

## 3. CSV-Vertrag (alle späteren Extract-Skripte)

Eine Zeile = ein Lauf. Spalten:

| Spalte | Bedeutung |
|---|---|
| `kernel` | z.B. `axpy_double`, später `mrc_gemv` |
| `mode` | `cpu` oder `pim` (optional `cpu_nocache`) |
| `N` | Vektorlänge bzw. Problemgröße |
| `elem_bytes` | Byte pro Element |
| `flops` | aus der Formel unten, nicht aus der IPC |
| `simSeconds` | `simSeconds` in `stats.txt` |
| `dram_totalEnergy_pJ` | CPU: Summe `dram.rank*.totalEnergy` (pJ); PIM: später `pimTotalEnergy` |
| `energy_per_flop` | `dram_totalEnergy_pJ / flops` (pJ/FLOP, flops > 0) |

FLOP-Formeln (fest, nicht in jedem Sweep neu erfinden):

- SAXPY / AXPY: `flops = 2 * N` (mul + add pro Element)
- MRC GEMV: `flops = 2 * Nsc * Nt * K`
- LS: `flops = 2 * Nsc * Nt`
- Contrast-GEMM: `flops = 2 * Reps * N^3`
- MIMO-GEMM: `flops = 2 * N^3`

Bytes (nur zur Plausibilität, nicht als Energie-Ersatz):

- AXPY working set: `2 * N * elem_bytes`
- CPU DRAM-Bytes: `bytesRead` + `bytesWritten` aus denselben DRAM-Stats

---

## 4. Namen in `stats.txt` (SimpleBoard, nicht `run_6g`)

Diese Boards schreiben **`board.*`**, nicht `system.cpu`.

Zeit / CPU:

- `simSeconds`
- `hostSeconds`
- `board.processor.cores.core.ipc`

DRAM (CPU-Pfad, DRAMPower):

- `board.memory.mem_ctrl.dram.rank0.totalEnergy` (pJ; Summe über Ranks)
- `board.memory.mem_ctrl.dram.rank0.actEnergy`
- `board.memory.mem_ctrl.dram.rank0.readEnergy`
- `board.memory.mem_ctrl.dram.rank0.writeEnergy`
- `board.memory.mem_ctrl.dram.rank0.refreshEnergy`
- `board.memory.mem_ctrl.dram.bytesRead::total`
- `board.memory.mem_ctrl.dram.bytesWritten::total`
- `board.memory.mem_ctrl.dram.avgMemAccLat` (bereits in den Thesis-Skripten)

IDD-Ströme kommen aus der Research-Subclass
`research/components/lpddr5_idd.py` (`LPDDR5_6400_1x16_BG_BL32_IDD`),
nicht aus gem5-Stock-LPDDR5 (dort sind IDD=0). Extract:
`python3 research/eval/extract_stats.py`.

PIM-Energie-Stats gibt es noch nicht; die kommen in Schritt 4 in
`src/mem/pim/`, nicht in Upstream-`dram_interface.cc`.

---

## 5. Was Schritt 1 bewusst nicht ist

- Kein neues gem5-Build
- Keine Kernel-Änderungen
- Keine ROI-`m5ops` (Schritt 3)
- Keine PIM-Energieformel (Schritt 4)
- Keine GEMV-Anpassung (Schritt 5)
- Keine Plots (Schritt 6)
- CPU-Kern-pJ/Instruction erst nach den DRAM-Plots

---

## 6. Check: Schritt 1 ist fertig, wenn

- Dieses Dokument gilt und niemand `run_6g_sim_riscv.py` in die A/B-Plots mischt
- CPU-Config = `edge_device_cpu_withcaches.py`, PIM-Config = `edge_device_pim.py`
- CSV-Spalten und FLOP-Formeln oben verwendet werden
- Stats mit `board.memory.mem_ctrl.dram.*` gesucht werden
