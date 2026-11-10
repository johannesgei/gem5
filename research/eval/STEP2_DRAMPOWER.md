# Schritt 2: DRAMPower auf dem CPU-Pfad

Stock-LPDDR5 in gem5 hat IDD=0. Die Edge-Configs nutzen deshalb
`LPDDR5_6400_1x16_BG_BL32_IDD` in `research/components/lpddr5_idd.py`
(Micron LPDDR5 x16 6400 Mb/s WT, VDD1=1.8 V, VDD2H=1.05 V).

Energie kommt aus gem5 DRAMPower in `stats.txt` unter
`board.memory.mem_ctrl.dram.rank0.*Energy` (Einheit pJ).
Kein Upstream-Patch.

Check: ein AXPY-Lauf, `rank0.totalEnergy` ungleich 0 und Extract-CSV mit
`energy_per_flop`.
