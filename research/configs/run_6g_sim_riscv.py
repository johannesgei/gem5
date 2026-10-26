"""Scaled-down 6G edge-device CPU in gem5 SE.

Clock and DRAM match a current edge/CPE-class SoC (big RISC-V core,
LPDDR5-6400). Caches are intentionally smaller than a real 1-2 MiB L2
so working-set vs LLC experiments finish in reasonable gem5 time.

  L1I/L1D  32 KiB   (real cores are often 32-64 KiB)
  L2       256 KiB  (scaled down from ~1-2 MiB private L2)
  CPU      3.0 GHz  TimingSimpleCPU
  DRAM     LPDDR5-6400
"""

import os
import sys

import m5
from m5.objects import (
    AddrRange,
    Cache,
    L2XBar,
    LPDDR5_6400_1x16_BG_BL32,
    MemCtrl,
    Process,
    RiscvTimingSimpleCPU,
    Root,
    SEWorkload,
    SrcClockDomain,
    System,
    SystemXBar,
    VoltageDomain,
)


class EdgeL1ICache(Cache):
    size = "32KiB"
    assoc = 8
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 8
    tgts_per_mshr = 8
    is_read_only = True
    writeback_clean = True


class EdgeL1DCache(Cache):
    size = "32KiB"
    assoc = 8
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 8
    tgts_per_mshr = 8


class EdgeL2Cache(Cache):
    size = "256KiB"
    assoc = 8
    tag_latency = 12
    data_latency = 12
    response_latency = 12
    mshrs = 16
    tgts_per_mshr = 12
    write_buffers = 8


repo = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
default_binary = os.path.join(repo, "research/benchmarks/mimo_kernel")
binary = sys.argv[1] if len(sys.argv) > 1 else default_binary

system = System()
system.clk_domain = SrcClockDomain(
    clock="3GHz", voltage_domain=VoltageDomain()
)
system.mem_mode = "timing"
system.mem_ranges = [AddrRange("2GiB")]

system.cpu = RiscvTimingSimpleCPU()
system.membus = SystemXBar()
system.l2bus = L2XBar()

system.cpu.icache = EdgeL1ICache()
system.cpu.dcache = EdgeL1DCache()
system.cpu.icache.cpu_side = system.cpu.icache_port
system.cpu.dcache.cpu_side = system.cpu.dcache_port
system.cpu.icache.mem_side = system.l2bus.cpu_side_ports
system.cpu.dcache.mem_side = system.l2bus.cpu_side_ports

system.l2cache = EdgeL2Cache()
system.l2cache.cpu_side = system.l2bus.mem_side_ports
system.l2cache.mem_side = system.membus.cpu_side_ports

system.cpu.mmu.connectWalkerPorts(
    system.l2bus.cpu_side_ports, system.l2bus.cpu_side_ports
)
system.cpu.createInterruptController()
system.system_port = system.membus.cpu_side_ports

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = LPDDR5_6400_1x16_BG_BL32()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

process = Process()
process.executable = binary
process.cmd = [binary]
system.workload = SEWorkload.init_compatible(binary)
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system=False, system=system)
m5.instantiate()

print("--- 6G edge (scaled L2): 3 GHz, LPDDR5-6400, 32KiB L1, 256KiB L2 ---")
print(f"binary: {binary}")
exit_event = m5.simulate()
print(f"Exited at tick {m5.curTick()} because {exit_event.getCause()}")
