#include "mem/pim/pim_dram_interface.hh"

#include "base/logging.hh"
#include "base/types.hh"

namespace gem5 {
namespace memory {

PIMDRAMInterface::PIMDRAMInterface(const PIMDRAMInterfaceParams &p) :
    DRAMInterface(p)
{
    // Leer
}

// Tick
// PIMDRAMInterface::calculatePIMLatency(uint64_t vectorSize, uint64_t elemBytes)
// {
//     return 123456789;
// }

} // namespace memory
} // namespace gem5