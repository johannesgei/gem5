from m5.objects.DRAMInterface import DRAMInterface
from m5.params import *

class PIMDRAMInterface(DRAMInterface):
    type = 'PIMDRAMInterface'
    cxx_header = "mem/pim/pim_dram_interface.hh"
    cxx_class = 'gem5::memory::PIMDRAMInterface'
    