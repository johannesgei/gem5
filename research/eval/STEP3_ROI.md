# Schritt 3: ROI um die AXPY-Schleife

`roi_begin()` / `roi_end()` in `research/benchmarks/kernel_common.h` rufen
`m5_reset_stats` und `m5_dump_stats` auf. Damit zählt DRAMPower-Energie
nicht mehr malloc und Cache-Flush.

Binary (RISC-V, magic m5op instruction):

```
riscv64-linux-gnu-gcc -static -O0 -DGEM5_ROI \
  -Iinclude -Iresearch/benchmarks \
  research/benchmarks/cpu/main_double_uncached.c \
  util/m5/src/abi/riscv/m5op.S \
  -o research/benchmarks/cpu/main_double_uncached
```

Extract nutzt den **ersten** Stats-Dump (ROI), nicht den Dump beim Sim-Ende.

Check (CPU AXPY, with caches):

- Vor ROI: N=64 und N=128 beide ~153 µJ (Setup).
- Mit ROI, kleine N (passen in L1): Energie ~0.20–0.24 µJ, Refresh 0/1 stört das Verhältnis.
- Mit ROI, N=8192 vs 16384 (über L1): Energie 9.43 µJ vs 18.58 µJ (**1.97×**),
  DRAM-Bytes **2.00×**, simSeconds **2.00×**.

Writebacks können nach `roi_end` kommen (`writeEnergy` oft 0 in der ROI).
Das ist für Schritt 3 akzeptabel; später ggf. Drain vor dem Dump.
