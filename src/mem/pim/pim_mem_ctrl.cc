#include "mem/pim/pim_mem_ctrl.hh"
#include "mem/pim/pim_dram_interface.hh"
#include "mem/dram_interface.hh"

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
            warn("--> [PIMMemCtrl] REG_COMMAND empfangen! Befehl: %lu. Leite an PIMDRAMInterface weiter", cmd);
            // warn("    [STATUS] Aktuelles Setup: N = %lu, Bytes/Element = %lu",
            //      storedVectorSize, storedElemBytes);

            // Da dram ein Vektor aus DRAMInterface* ist, holen wir uns Kanal 0
            // und casten ihn dynamisch auf unser neues PIMDRAMInterface
            auto pimDram = dynamic_cast<PIMDRAMInterface*>(&dram[0]);

            if (pimDram) {
                // Hier passiert die Magie: Der Funktionsaufruf im Interface!
                pimDram->printPIMParameters(storedVectorSize, storedElemBytes, cmd);
            } else {
                fatal("Fehler: Das zugewiesene DRAM-Interface ist kein PIMDRAMInterface!");
            }
        }
    }

    // Alle anderen regulären CPU-Anfragen laufen normal weiter
    return MemCtrl::recvTimingReq(pkt);
}

} // namespace memory
} // namespace gem5
