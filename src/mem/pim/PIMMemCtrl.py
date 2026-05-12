from m5.objects import MemCtrl
from m5.params import *
from m5.proxy import *


class PIMMemCtrl(MemCtrl):
    type = "PIMMemCtrl"
    cxx_header = "mem/pim/pim_mem_ctrl.hh"
    cxx_class = "gem5::memory::PIMMemCtrl"

    # You can add custom parameters here later
    pim_latency = Param.Latency("1ns", "Internal PIM processing time")
    trigger_addr = Param.Addr(0x79000, "Address that triggers PIM operation")
