#include "mem/pim/pim_mem_ctrl.hh"
#include "mem/pim/pim_dram_interface.hh"

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/DRAM.hh"
#include "debug/PIM.hh"
#include "mem/port_proxy.hh"
#include "sim/system.hh"

namespace gem5 {
namespace memory {

PIMMemCtrl::PIMMemCtrl(const PIMMemCtrlParams &p) :
    MemCtrl(p),
    // storedVectorSize(0),
    // storedElemBytes(0),
    pimBaseAddr(p.pim_base_addr) {
}

bool
PIMMemCtrl::recvTimingReq(PacketPtr pkt) {
    // Wir reagieren nur auf Schreibzugriffe (STOREs) der CPU
    if (pkt->isWrite()) {
        Addr addr = pkt->getAddr();

        // warn("[DIAGNOSE] Schreibzugriff auf physische Adresse: 0x%lx (Größe: %d)", 
        //      addr, pkt->getSize());



        // uint64_t data_val = 0;
        
        // // Sicherstellen, dass das Paket Daten enthält und die Größe passt
        // if (pkt->getSize() == 8 && pkt->hasData()) {
        //     data_val = *(pkt->getConstPtr<uint64_t>());
            
        //     // Wenn der geschriebene Wert exakt 1 ist (dein Signal!)
        //     if (data_val == 1) {
        //         warn("[SPEICHER_DETEKTOR] Signal '1' abgefangen! Physische Zieladresse im Bus ist: 0x%lx", pkt->getAddr());
        //     }
        // }



        // if(addr == pimBaseAddr) {
        //     warn("[PIMMemCtrl] MMIO-Zugriff auf Basisadresse 0x%lx erkannt!", addr);
        // }
        

        if (addr == pimRegSize) {
            storedVectorSize = *(pkt->getConstPtr<uint64_t>());
            warn("--> [PIMMemCtrl] REG_SIZE empfangen! Wert: %lu", storedVectorSize);
        } 
        else if (addr == pimRegBytes) {
            storedElemBytes = *(pkt->getConstPtr<uint64_t>());
            warn("--> [PIMMemCtrl] REG_ELEM_BYTES empfangen! Wert: %lu", storedElemBytes);
        } 
        else if (addr == pimRegCmd) {
            uint64_t cmd = *(pkt->getConstPtr<uint64_t>());
            warn("--> [PIMMemCtrl] REG_COMMAND empfangen! Befehl: %lu", cmd);
            warn("    [STATUS] Aktuelles Setup: N = %lu, Bytes/Element = %lu", 
                 storedVectorSize, storedElemBytes);
        }
    }

    // Alle anderen regulären CPU-Anfragen laufen normal weiter
    return MemCtrl::recvTimingReq(pkt);
}

} // namespace memory
} // namespace gem5
