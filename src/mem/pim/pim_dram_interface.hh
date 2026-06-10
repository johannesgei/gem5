#ifndef __SRC_MEM_PIM_PIM_DRAM_INTERFACE_HH__
#define __SRC_MEM_PIM_PIM_DRAM_INTERFACE_HH__

#include "mem/dram_interface.hh"
#include "params/PIMDRAMInterface.hh"

namespace gem5 {
namespace memory {

class PIMDRAMInterface : public DRAMInterface
{
  public:
    PIMDRAMInterface(const PIMDRAMInterfaceParams &p);

    void printPIMParameters(uint64_t size, uint64_t bytes, uint64_t cmd);

    Tick calculatePIMLatency(uint64_t vectorSize, uint64_t elemBytes);
};

} // namespace memory
} // namespace gem5

#endif // __SRC_MEM_PIM_PIM_DRAM_INTERFACE_HH__
