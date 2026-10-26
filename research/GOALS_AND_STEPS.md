# Goals and steps: CPU-centric vs memory-centric energy

This file is the working plan for the gem5 demonstrator. The end product is
plots that compare **energy** and **energy per operation** of a normal
processor-centric edge design against a memory-centric (PIM) design on the
same 6G kernels. Relative ranking is the target, not a tape-out-accurate chip.

Do not edit upstream gem5. Add behaviour only under `research/` or new files
such as `src/mem/pim/`.

---

## 1. Goal

For each kernel, run two comparable gem5 simulations on the same scaled 6G
edge setup (RISC-V, 3 GHz, LPDDR5-6400, 32 KiB L1, 256 KiB L2):

1. **CPU-centric:** the kernel runs on the host CPU; all data moves through
   caches and the off-chip DRAM interface.
2. **Memory-centric:** the host CPU sets up buffers and issues a PIM command;
   compute happens inside the memory device.

From those runs, plot energy and energy per operation, plus a split of DRAM
vs compute energy. Keep an in-cache contrast kernel as a check that PIM is
not supposed to win everywhere.

---

## 2. Kernels

| Kernel | Role | Status |
|---|---|---|
| MIMO GEMM (`mimo_kernel`) | 256×256 beamforming analog | CPU path already run |
| MRC GEMV (`mrc_gemv`) | combining over antennas / users | source only |
| LS channel estimation (`ls_est`) | long Y × pilot reduction | source only |
| STREAM SAXPY (`stream_saxpy`) | bandwidth ceiling | source only |
| Contrast GEMM (`contrast_gemm`) | tiny in-L1 GEMM; PIM should lose | source only |
| DNA search (`dna_search`) | extra streaming kernel | source only |

What we look for on the CPU path before PIM:

- working set vs L2 (fit vs miss)
- CPI / IPC (core waiting on memory)
- L1D / L2 miss rates
- bytes moved from DRAM and DRAM energy

High miss rate and high DRAM traffic → PIM candidate. Contrast GEMM should
stay cheap in cache; SAXPY and LS should be the opposite.

---

## 3. How the memory-centric path works

This is **not** a second CPU sitting next to DRAM.

The host CPU keeps setup, allocation, and control. It then writes a small
uncacheable command (opcode, length, physical base addresses) to a PIM
mailbox. `PIMMemCtrl` / `PIMDRAMInterface` (already stubbed in `src/mem/pim/`)
stall the CPU until an **analytical LPDDR5 bank-group latency** expires.
Operands are assumed to already sit in DRAM. Arithmetic runs **in the memory
device**; results are not streamed over the DQ bus for the compute itself.

Existing stub shape:

- MMIO registers at `pim_base_addr` (size, element bytes, command)
- `calculatePIMLatency()` from LPDDR5 timings (tRCD, tCCD, tRP, row buffer, …)
- timing delay only; no functional write-back and no PIM energy yet

---

## 4. How precise the simulation should be

**Do simulate**

- CPU path: TimingSimpleCPU + caches + real DRAM command timing (already).
- PIM path: command-level latency from the DRAM row/burst schedule, plus a
  small in-memory ALU delay.
- Energy: DRAMPower-style DRAM energy on the CPU path; the same class of
  costs on the PIM path, with internal accesses and extra ACT/PRE, plus
  literature pJ/MAC for the PIM ALUs.

**Do not simulate** (out of scope for the demonstrator)

- transistor-level / SPICE PIM ALUs
- cycle-accurate execution of every MAC inside DRAM
- full-system Linux
- cache-coherent PIM cores or RTL

Functional correctness stays on the CPU golden. The PIM model must be
**timing- and energy-plausible**. It does not need to recompute the result
inside gem5 unless we later add a write-back into simulated DRAM.

---

## 5. Where timings and energy come from

| Quantity | Source |
|---|---|
| DRAM timings (tCK, tRCD, tRP, tRAS, tCCD, …) | JEDEC / gem5 `LPDDR5_6400_*` |
| DRAM energy (activate, read, write, refresh, standby) | Datasheet IDD currents → gem5 DRAMPower (`*Energy` in `stats.txt`) |
| CPU core energy | Weak in gem5. For this demonstrator: published RISC-V edge pJ/instruction, **or** plot DRAM energy only for memory-bound kernels and say so |
| PIM ALU energy | Literature pJ/MAC (HBM-PIM / LPDDR-PIM papers) × number of ops |
| PIM DRAM energy | Same DRAMPower-style costs, but **internal** (no off-chip DQ for compute), plus ACT/PRE implied by the latency formula |

State the CPU-core-energy assumption in every plot caption.

---

## 6. Addresses, translation, and placement

gem5 syscall emulation already walks a simple page table (VA → PA) for the
host CPU. Leave that as-is.

PIM commands must carry **physical** addresses. Bank/row mapping then
decides whether `A[i]` and `B[i]` can be computed in the same bank.

Pick **one** layout rule and document it in the results:

1. **Oracle layout** — arrays aligned to banks (upper bound).
2. **Explicit layout** — padding / interleaved allocation in the benchmark.
3. **Real malloc** — whatever SE gives (often pessimistic for PIM).

Recommended for the first plots: **oracle layout**, labelled as an upper
bound.

The mailbox (`0x79000` in the stub) must be **uncacheable** and must not be
treated as normal DRAM data. Host buffers that PIM will read must be
**flushed from cache** (or allocated uncached). Otherwise PIM operates on
stale DRAM.

---

## 7. What already exists vs what is still missing

**Exists**

- CPU-centric config: `research/configs/run_6g_sim_riscv.py`
- Six kernel sources under `research/benchmarks/`
- One MIMO CPU run under `research/results/mimo_kernel/`
- `PIMMemCtrl` / `PIMDRAMInterface` latency stub
- `research/components/pim_board.py` (not used by the current config)

**Still to build**

- Wire `PIMMemCtrl` into the 6G config (new research files / subclasses only)
- Uncacheable command path from each kernel (or a tiny driver)
- PIM energy accounting: ACT/PRE/burst counts × IDD, plus pJ/MAC
- CPU vs PIM result directories
- Plot script: energy, energy/op, DRAM vs compute split
- Optional later: write PIM results back into simulated DRAM

---

## 8. Steps to the plots

1. **Freeze the CPU baseline.** One config, all six kernels, `stats.txt` plus
   DRAM energy. Rank kernels by miss rate and bytes moved.
2. **Specify the PIM contract.** Commands, physical arguments, layout rule
   (oracle unless we change it).
3. **Finish latency and energy** on `PIMDRAMInterface` (no upstream edits).
4. **Add a PIM mode** of each kernel, or one small driver that issues the
   mailbox command after setup.
5. **Run A/B.** Same kernel, same sizes, CPU dir vs PIM dir, same metrics.
6. **Plot.** CPU vs PIM energy and energy/op. Contrast GEMM must remain the
   “PIM should not win” check.

---

## 9. One-sentence summary

Analytical in-DRAM commands on real LPDDR5 timings and DRAMPower currents —
not a second CPU, and not transistor-level PIM.
