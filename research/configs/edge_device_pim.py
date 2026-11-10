import os
import sys

from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.no_cache import NoCache
from gem5.components.cachehierarchies.classic.private_l1_shared_l2_cache_hierarchy import (
    PrivateL1SharedL2CacheHierarchy,
)

# Import the specific LPDDR5 interface
from gem5.components.memory.dram_interfaces.lpddr5 import (
    LPDDR5_6400_1x16_BG_BL32,
)
from gem5.components.memory.memory import ChanneledMemory
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import BinaryResource
from gem5.simulate.simulator import Simulator

sys.path.append(os.getcwd())
from m5.objects import (
    PIMDRAMInterface,
    PIMMemCtrl,
    Process,
)


# Create class that inherits all timings from LPDDR5 and creates PIMDRAMInterface.
class PIM_LPDDR5(LPDDR5_6400_1x16_BG_BL32):
    def __init__(self):
        super().__init__()

    type = "PIMDRAMInterface"


# 1. Setup Cache Hierarchy
# Edge devices favor efficiency. 32kB L1s and a 256kB L2 provide a realistic
# balance between hit-rate and power consumption.
# cache_hierarchy = PrivateL1SharedL2CacheHierarchy(
#     l1d_size="32KiB",
#     l1i_size="32KiB",
#     l2_size="256KiB", # or 512KiB for a more powerful edge device
# )
cache_hierarchy = NoCache()

# 2. Setup Memory Subsystem
# We use LPDDR5_6400 with Bank Groups (BG).
# The Bank Group architecture is CRITICAL for PIM because it allows
# parallel operations across different groups of banks.
# 6400MT/s data transfer rate
# 1 channel with bus width of 16 bit
# has bank groups enabled (critical for parallel PIM performance)
# burst length 32 --> 32 x 16 bit (bus width) = 64 Byte (exactly one cache line)
# 16 banks per rank; 4 bank groups per rank
memory = ChanneledMemory(
    dram_interface_class=LPDDR5_6400_1x16_BG_BL32,
    num_channels=1,
    interleaving_size=128,  # Aligns with cache line size
    size="1024MiB",
)

# 3. Setup Processor
# For Edge/IoT, the 'MINOR' CPU model is best. It is a four-stage
# in-order model that accurately reflects the power profile of an Edge SoC.
processor = SimpleProcessor(
    cpu_type=CPUTypes.MINOR,  # For max power, use CPUTypes.O3
    isa=ISA.RISCV,
    num_cores=1,
)

# 4. Assemble the Board
board = SimpleBoard(
    clk_freq="2GHz",  # 1-2 GHz is typical for edge devices
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

board.memory.mem_ctrl = PIMMemCtrl(pim_base_addr=0x79000)
board.memory.mem_ctrl.dram = PIM_LPDDR5()

# 5. Define Workload
# Erwarteter Aufruf: gem5.opt edge_device_pim.py <Pfad_zum_Binary> <N> <ELEMENT_SIZE>
args = [a for a in sys.argv[1:] if not a.startswith("--")]
if len(args) < 3:
    print("Fehler: Zu wenige Argumente übergeben!")
    print("Nutzung: gem5.opt edge_device_pim.py <binary_path> <N> <elem_size>")
    sys.exit(1)

binary_path = args[0]
vector_size = args[1]
element_size = args[2]

binary = BinaryResource(local_path=binary_path)
board.set_se_binary_workload(binary)

pim_process = Process()
pim_process.executable = binary.get_local_path()
pim_process.cmd = [binary.get_local_path(), vector_size, element_size]

for core in processor.get_cores():
    core.core.workload = [pim_process]

# 6. Run Simulation
simulator = Simulator(board=board)
simulator.run()
