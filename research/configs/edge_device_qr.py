import os
import sys

from m5.objects import Process

from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
from gem5.components.memory import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.simulate.simulator import Simulator

sys.path.append(os.getcwd())

from research.components.pim_board import PIMBoard

# ------------------------------------------
USE_PIM = 1  # 1 = PIMBoard, 0 = SimpleBoard
# ------------------------------------------

# 1. Setup: A typical edge-device (ARM, moderate CPU, DDR4)
host_processor = SimpleProcessor(
    cpu_type=CPUTypes.O3, isa=ISA.RISCV, num_cores=4
)
pim_processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING, isa=ISA.RISCV, num_cores=4
)
cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size="1KiB", l1i_size="1KiB", l2_size="4KiB"
)
memory = SingleChannelDDR4_2400(size="2GiB")

# 2. Assemble board
if USE_PIM:
    print("Using PIMBoard architecture.")
    board = PIMBoard(
        clk_freq="4GHz",
        processor=host_processor,
        pim_processor=pim_processor,
        memory=memory,
        cache_hierarchy=cache_hierarchy,
    )
else:
    print("Using standard SimpleBoard architecture.")
    board = SimpleBoard(
        clk_freq="4GHz",
        processor=host_processor,
        memory=memory,
        cache_hierarchy=cache_hierarchy,
    )

# 3. Set QR-Givens binary as workload
if len(sys.argv) > 1:
    binary_path = sys.argv[1]
else:
    binary_path = os.path.join(os.getcwd(), "research/benchmarks/qr/qr_100")
binary = BinaryResource(local_path=binary_path)
board.set_se_binary_workload(binary)

if USE_PIM:
    process_pim = Process()
    process_pim.executable = binary.get_local_path()
    process_pim.cmd = [
        binary.get_local_path()
    ]  # Argumentliste (mindestens der Name)

    process_pim.pid = 101

    for core in pim_processor.get_cores():
        # Jetzt übergeben wir das SimObject 'Process', nicht die Ressource.
        core.core.workload = [process_pim]

# 4. Define and start simulation
# simulator = Simulator(board=board, max_ticks=10_000_000_000)
simulator = Simulator(board=board, max_ticks=10_000_000)
print("Starting simulation.")
simulator.run()
