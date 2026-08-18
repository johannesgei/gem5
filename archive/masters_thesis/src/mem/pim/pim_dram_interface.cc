#include "mem/pim/pim_dram_interface.hh"

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/PIM.hh"
#include "base/types.hh"
#include "sim/core.hh"
#include <algorithm>

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

    // 2. Hardware-Architektur bestimmen
    unsigned bg_per_rank = my_params->bank_groups_per_rank;
    uint64_t row_buffer_size = my_params->device_rowbuffer_size;

    // 3. Verteilung auf die Bank-Groups (Parallelisierung des Vektors)
    uint64_t elems_per_bg = (size + bg_per_rank - 1) / bg_per_rank;
    uint64_t bytes_per_bg = elems_per_bg * bytes;

    // 4. Zeilenwechsel-Overhead berechnen
    uint64_t rows_needed = (bytes_per_bg + row_buffer_size - 1) / row_buffer_size;

    warn("--> Bank-Groups pro Rank:  %u", my_params->bank_groups_per_rank);
    warn("--> Bänke pro Rank:        %u", my_params->banks_per_rank);
    warn("--> Row Buffer Size:       %u", my_params->device_rowbuffer_size);
    warn("--> Elements per BG:       %u", (size + my_params->bank_groups_per_rank - 1) / my_params->bank_groups_per_rank);
    warn("--> Rows needed:           %u", rows_needed);
    warn("--> max_accesses_per_row:      %lu", my_params->max_accesses_per_row);
    warn("--------------------------------------------------------");
    warn("[LPDDR5 TIMING-PARAMETER]:");
    warn("Lese- und Schreiblatenzen:");
    warn("--> tCK:                       %lu Ticks", my_params->tCK);
    warn("--> tCK:                       %lu tCK", my_params->tCK / my_params->tCK);
    warn("--> tCL:                       %lu Ticks", my_params->tCL);
    warn("--> tCL:                       %lu tCK", my_params->tCL / my_params->tCK);
    warn("--> tCWL:                      %lu Ticks", my_params->tCWL);
    warn("--> tCWL:                      %lu tCK", my_params->tCWL / my_params->tCK);
    warn("Bus-Übertragungszeiten:");
    warn("--> tBURST:                    %lu Ticks", my_params->tBURST);
    warn("--> tBURST:                    %lu tCK", my_params->tBURST / my_params->tCK);
    warn("--> tBURST_MIN:                %lu Ticks", my_params->tBURST_MIN);
    warn("--> tBURST_MIN:                %lu tCK", my_params->tBURST_MIN / my_params->tCK);
    warn("--> tBURST_MAX:                %lu Ticks", my_params->tBURST_MAX);
    warn("--> tBURST_MAX:                %lu TtCK", my_params->tBURST_MAX / my_params->tCK);
    warn("Spalten-zu-Spalten-Befehle:");    
    warn("--> tCCD_L:                    %lu Ticks", my_params->tCCD_L);
    warn("--> tCCD_L:                    %lu tCK", my_params->tCCD_L / my_params->tCK);
    warn("--> tCCD_L_WR:                 %lu Ticks", my_params->tCCD_L_WR);
    warn("--> tCCD_L_WR:                 %lu tCK", my_params->tCCD_L_WR / my_params->tCK);
    warn("Zeilen-Zugriffszeiten:");
    warn("--> tRCD (Read):               %lu Ticks", my_params->tRCD); 
    warn("--> tRCD (Read):               %lu tCK", my_params->tRCD / my_params->tCK); 
    warn("--> tRCD_WR:                   %lu Ticks", my_params->tRCD_WR); 
    warn("--> tRCD_WR:                   %lu tCK", my_params->tRCD_WR / my_params->tCK); 
    warn("--> tRAS (Row Active Time):    %lu Ticks", my_params->tRAS);
    warn("--> tRAS (Row Active Time):    %lu tCK", my_params->tRAS / my_params->tCK);
    warn("--> tRP  (Row Precharge Time): %lu Ticks", my_params->tRP);
    warn("--> tRP  (Row Precharge Time): %lu tCK", my_params->tRP / my_params->tCK);
    warn("Wechsel zwischen Schreiben und Lesen:");
    warn("--> tRTW:                      %lu Ticks", my_params->tRTW);
    warn("--> tRTW:                      %lu tCK", my_params->tRTW / my_params->tCK);
    warn("--> tWTR:                      %lu Ticks", my_params->tWTR);
    warn("--> tWTR:                      %lu tCK", my_params->tWTR / my_params->tCK);
    warn("--> tWTR_L:                    %lu Ticks", my_params->tWTR_L);
    warn("--> tWTR_L:                    %lu tCK", my_params->tWTR_L / my_params->tCK);
    warn("--> tWR:                       %lu Ticks", my_params->tWR);
    warn("--> tWR:                       %lu tCK", my_params->tWR / my_params->tCK);
    warn("--> tRTP:                      %lu Ticks", my_params->tRTP);
    warn("--> tRTP:                      %lu tCK", my_params->tRTP / my_params->tCK);
    warn("Bank-zu-Bank-Einschränkungen:");    
    warn("--> tRRD_L:                    %lu Ticks", my_params->tRRD_L);
    warn("--> tRRD_L:                    %lu tCK", my_params->tRRD_L / my_params->tCK);
    warn("--> tPPD:                      %lu Ticks", my_params->tPPD);
    warn("--> tPPD:                      %lu tCK", my_params->tPPD / my_params->tCK);
    warn("========================================================");
}

Tick
PIMDRAMInterface::calculatePIMLatency(uint64_t vectorSize, uint64_t elemBytes) const
{
    auto my_params = static_cast<const PIMDRAMInterfaceParams*>(&_params);

    // Logik-Zyklen in gem5 Ticks umrechnen
    // Tick tCK = my_params->tCK;
    // Tick tMUL = 5 * tCK;
    // Tick tADD = 2 * tCK;
    // Tick tLogic = tMUL + tADD; // Reine Rechenzeit pro Element

    // Hardware-Architektur bestimmen
    uint64_t bg_per_rank = my_params->bank_groups_per_rank;
    uint64_t row_buffer_size = my_params->device_rowbuffer_size;
    Tick tCK = my_params->tCK;

    // Verteilung auf die Bank-Groups (Parallelisierung des Vektors)
    uint64_t elems_per_bg = (vectorSize + bg_per_rank - 1) / bg_per_rank;
    uint64_t bytes_per_bg = elems_per_bg * elemBytes;
    // uint64_t elems_per_row = row_buffer_size / elemBytes;
    uint64_t rows_needed = (bytes_per_bg + row_buffer_size - 1) / row_buffer_size;

    // Umrechnen in bursts
    uint64_t burst_size_bytes = my_params->burst_length * my_params->device_bus_width / 8;
    // uint64_t elems_per_burst = burst_size_bytes / elemBytes;
    // uint64_t total_bursts = (vectorSize * elemBytes + burst_size_bytes - 1) / burst_size_bytes;
    uint64_t bursts_per_bg = (bytes_per_bg + burst_size_bytes - 1) / burst_size_bytes;
    // uint64_t bursts_per_row = (row_buffer_size + burst_size_bytes - 1) / burst_size_bytes;

    // Latenzen berechnen
    Tick tReadReadWrite = my_params->tCCD_L + my_params->tBURST + my_params->tRTW + my_params->tBURST;
    Tick tFirstOverallBurstBestCase = my_params->tRCD - my_params->tCCD_L + std::max({my_params->tCCD_L, my_params->tRRD_L}) + tReadReadWrite;
    Tick tFirstOverallBurstWorstCase = tFirstOverallBurstBestCase - std::max({my_params->tCCD_L, my_params->tRRD_L}) + std::max({my_params->tCCD_L, my_params->tRRD_L, my_params->tPPD}) + my_params->tRP + my_params->tWR;
    Tick tNextBurstInLine = my_params->tWTR_L + tReadReadWrite;
    Tick tFirstBurstInNewLine = tReadReadWrite - my_params->tCCD_L + my_params->tRCD + my_params->tRP + my_params->tWR;
    Tick tLastOverallBurst = tNextBurstInLine;

    // Gesamtlatenz nach der BGA-NMP Formel zusammenführen
    Tick tTotalBestCase = tFirstOverallBurstBestCase + (rows_needed - 1) * tFirstBurstInNewLine + (bursts_per_bg - rows_needed - 1) * tNextBurstInLine + tLastOverallBurst;
    Tick tTotalWorstCase = tFirstOverallBurstWorstCase + (rows_needed - 1) * tFirstBurstInNewLine + (bursts_per_bg - rows_needed - 1) * tNextBurstInLine + tLastOverallBurst;

    double global_freq = static_cast<double>(gem5::getClockFrequency());

    double totalBytes = 2* static_cast<double>(vectorSize) * static_cast<double>(elemBytes);
    double totalBytesMiB = totalBytes / (1024.0 * 1024.0);

    // Diagnose-Ausgabe auf dem Terminal
    DPRINTF(PIM, "--> Vector Size (# elements):      %lu\n", vectorSize);
    DPRINTF(PIM, "--> Bytes per element:             %lu\n", elemBytes);
    DPRINTF(PIM, "--> Total size of both vectors:    %.6f MiB\n", totalBytesMiB);
    DPRINTF(PIM, "--> tClock:                        %lu Ticks\n", tCK);

    DPRINTF(PIM, "--> Bus Width / Burst Length:      %u Bit / BL%u\n", my_params->device_bus_width, my_params->burst_length);
    DPRINTF(PIM, "--> Calculated Burst Size:         %lu Bytes\n", burst_size_bytes);
    DPRINTF(PIM, "--> Active Bank-Groups:            %lu\n", bg_per_rank);
    DPRINTF(PIM, "--> Elements per Bank-Group:       %lu\n", elems_per_bg);
    DPRINTF(PIM, "--> Bursts per BG:                 %lu\n", bursts_per_bg);
    DPRINTF(PIM, "--> # first overall bursts:        1\n");
    DPRINTF(PIM, "--> # first bursts in new line:    %lu\n", rows_needed - 1);
    DPRINTF(PIM, "--> # next bursts in line:         %lu\n", bursts_per_bg - rows_needed - 1);
    DPRINTF(PIM, "--> # last bursts:                 1\n");
    DPRINTF(PIM, "--> Required Row Changes:          %lu\n", rows_needed - 1);
    DPRINTF(PIM, "----------------- Protocol Timings (Ticks) -----------------\n");
    DPRINTF(PIM, "--> tCK: %lu | tCCD_L: %lu | tBURST: %lu | tRTW: %lu | tWTR_L: %lu\n", my_params->tCK, my_params->tCCD_L, my_params->tBURST, my_params->tRTW, my_params->tWTR_L);
    DPRINTF(PIM, "--> tRCD: %lu | tRP: %lu | tWR: %lu | tPPD: %lu | tRRD_L: %lu\n", my_params->tRCD, my_params->tRP, my_params->tWR, my_params->tPPD, my_params->tRRD_L);
    DPRINTF(PIM, "----------------- Phase Latencies -----------------\n");
    DPRINTF(PIM, "--> Time first overall burst:      %lu / %lu Ticks (best/worst case)\n", tFirstOverallBurstBestCase, tFirstOverallBurstWorstCase);
    DPRINTF(PIM, "--> Time burst in open line:       %lu Ticks\n", tNextBurstInLine);
    DPRINTF(PIM, "--> Time burst row change:         %lu Ticks\n", tFirstBurstInNewLine);
    DPRINTF(PIM, "--> Time last overall burst:       %lu Ticks\n", tLastOverallBurst);
    DPRINTF(PIM, "------------------- Final Results -------------------\n");
    DPRINTF(PIM, "--> PIM-LATENCY (BEST CASE):       %lu Ticks = %.6f ms\n", tTotalBestCase, tTotalBestCase / global_freq * 1e3);
    DPRINTF(PIM, "--> PIM-LATENCY (WORST CASE):      %lu Ticks = %.6f ms\n", tTotalWorstCase, tTotalWorstCase / global_freq * 1e3);
    DPRINTF(PIM, "==========================================================\n");

    return tTotalBestCase;
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
            DPRINTF(PIM, "--> PIM Event Scheduled bei Sim-Tick: %lu Ticks\n", curTick());
            DPRINTF(PIM, "--> Geplantes Ende bei: %lu Ticks (+%lu)\n", curTick() + total_latency, total_latency);
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
    DPRINTF(PIM, "--> PIM Status: %s\n", pimProcessing ? "Processing" : "Idle");
    return pimProcessing ? 1 : 0;
}

void
PIMDRAMInterface::pimExecutionFinished()
{
    pimProcessing = false;
    DPRINTF(PIM, "--> PIM-Berechnung im Speicher abgeschlossen bei Tick: %lu\n", curTick());
}

} // namespace memory
} // namespace gem5
