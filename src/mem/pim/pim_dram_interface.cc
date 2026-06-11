#include "mem/pim/pim_dram_interface.hh"

#include "base/logging.hh"
#include "base/types.hh"
#include "sim/core.hh"

namespace gem5 {
namespace memory {

PIMDRAMInterface::PIMDRAMInterface(const PIMDRAMInterfaceParams &p) :
    DRAMInterface(p),
    pimEvent(*this),
    pimVectorSize(0),
    pimElementBytes(0),
    pimProcessing(false)
{
    // Leer
}

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

    auto my_params = static_cast<const PIMDRAMInterfaceParams*>(&_params);

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

Tick
PIMDRAMInterface::calculatePIMLatency(uint64_t vectorSize, uint64_t elemBytes) const
{
    auto my_params = static_cast<const PIMDRAMInterfaceParams*>(&_params);

    // 1. Logik-Zyklen in gem5 Ticks umrechnen
    Tick tCK = my_params->tCK;
    Tick tMUL = 5 * tCK;
    Tick tADD = 2 * tCK;
    Tick tLogic = tMUL + tADD; // Reine Rechenzeit pro Element

    // 2. Hardware-Architektur bestimmen
    unsigned bg_per_rank = my_params->bank_groups_per_rank;
    uint64_t row_buffer_size = my_params->device_rowbuffer_size;

    // 3. Verteilung auf die Bank-Groups (Parallelisierung des Vektors)
    uint64_t elems_per_bg = (vectorSize + bg_per_rank - 1) / bg_per_rank;
    uint64_t bytes_per_bg = elems_per_bg * elemBytes;

    // 4. Zeilenwechsel-Overhead berechnen
    uint64_t rows_needed = (bytes_per_bg + row_buffer_size - 1) / row_buffer_size;
    Tick row_change_overhead = rows_needed * (my_params->tRCD + my_params->tRP);

    // 5. Gesamtlatenz nach der BGA-NMP Formel zusammenführen
    Tick total_logic_time = elems_per_bg * tLogic;
    Tick total_latency = row_change_overhead + total_logic_time;

    double global_freq = static_cast<double>(gem5::getClockFrequency());

    // Diagnose-Ausgabe auf dem Terminal
    warn("=============== [PIM LATENCY SIMULATION] ===============");
    warn("--> tClock der Speicher-Logik:    %lu Ticks", tCK);
    warn("--> Berechnetes tLogic (MUL+ADD): %lu Ticks pro Element", tLogic);
    warn("--> Elemente pro Bank-Group:      %lu", elems_per_bg);
    warn("--> Benötigte Zeilenwechsel:      %lu", rows_needed);
    warn("--> Strafzeit für Zeilenwechsel:  %lu Ticks", row_change_overhead);
    warn("                                = %.6f ms", row_change_overhead / global_freq * 1e3);
    warn("--> Reine Berechnungszeit:        %lu Ticks", total_logic_time);
    warn("                                = %.6f ms", total_logic_time / global_freq * 1e3);
    warn("==> PIM-GESAMTLATENZ:             %lu Ticks", total_latency);
    warn("                                = %.6f ms", total_latency / global_freq * 1e3);
    warn("========================================================");

    return total_latency;
}

Tick
PIMDRAMInterface::triggerPIMExecution(uint64_t size, uint64_t bytes, uint64_t cmd)
{
    Tick total_latency = 0;

    // Nur starten, wenn das Kommando stimmt und wir nicht schon mitten im Lauf sind
    if (cmd == 1 && !pimProcessing) {
        pimProcessing = true;

        // 1. Hole die reine Berechnungszeit aus der mathematischen Funktion
        total_latency = calculatePIMLatency(size, bytes);

        // 2. Event-Eintrag absichern und planen
        if (!pimEvent.scheduled()) {
            schedule(pimEvent, curTick() + total_latency);

            warn("============= [PIM EVENT SCHEDULED] =============");
            warn("--> Aktueller Sim-Tick: %lu Ticks", curTick());
            warn("--> Geplantes Ende bei: %lu Ticks (+%lu)", curTick() + total_latency, total_latency);
            warn("==================================================");
        } else {
            warn("[PIM] Warnung: Event war bereits geplant! Überspringe doppelten Schedule.");
        }
    }
    return curTick() + total_latency;
}

uint64_t
PIMDRAMInterface::readPimCommand()
{
    // Wenn die PIM-Logik im Hintergrund noch rechnet, gib 1 zurück, ansonsten 0
    warn("============= [PIM STATUS CHECK] =============");
    return pimProcessing ? 1 : 0;
}

void
PIMDRAMInterface::pimExecutionFinished()
{
    pimProcessing = false;
    
    warn("============= [PIM EVENT FINISHED] =============");
    warn("--> PIM-Berechnung im Speicher abgeschlossen bei Tick: %lu", curTick());
    warn("=================================================");
}

} // namespace memory
} // namespace gem5
