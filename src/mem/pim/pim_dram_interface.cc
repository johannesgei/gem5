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

void
PIMDRAMInterface::printPIMParameters(uint64_t size, uint64_t bytes, uint64_t cmd)
{
    warn("========================================================");
    warn("[PIMDRAMInterface] DATEN ERFOLGREICH ANGEKOMMEN!");
    warn("--> Übergebene Vektorgröße (N): %lu", size);
    warn("--> Übergebene Element-Bytes:   %lu", bytes);
    warn("--> Übergebener Befehl (Cmd):   %lu", cmd);
    warn("========================================================");


    warn("[LPDDR5 HARDWARE-PARAMETER AUSGELESEN]:");

    // HIER PASSIERT DIE MAGIE: 
    // Wir casten den universellen _params-Pointer auf deine spezifischen PIM-DRAM-Parameter.
    auto my_params = static_cast<const PIMDRAMInterfaceParams*>(&_params);

    // Jetzt nutzen wir die exakten Variablennamen aus der Python-Generierung (Snake_Case)
    unsigned b_per_rank  = my_params->banks_per_rank;
    unsigned bg_per_rank = my_params->bank_groups_per_rank;

    warn("--> Bank-Groups pro Rank:  %u", bg_per_rank);
    warn("--> Bänke pro Rank:        %u", b_per_rank);

    warn("--------------------------------------------------------");
    warn("[LPDDR5 TIMING-PARAMETER]:");
    warn("--> tRAS (Row Active Time):    %lu Ticks", my_params->tRAS);
    warn("--> tRP  (Row Precharge Time): %lu Ticks", my_params->tRP);
    warn("--> tRCD (RAS-to-CAS Delay):   %lu Ticks", my_params->tRCD); 
    warn("========================================================");
}

} // namespace memory
} // namespace gem5
