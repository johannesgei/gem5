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
    pimBaseAddr(p.pim_base_addr) {
}

bool
PIMMemCtrl::recvTimingReq(PacketPtr pkt) {
    Addr addr = pkt->getAddr();
    if (pkt->isWrite()) {

        if (addr == pimRegSize) {
            storedVectorSize = *(pkt->getConstPtr<uint64_t>());
            DPRINTF(PIM, "--> [PIMMemCtrl] REG_SIZE empfangen! Wert: %lu\n", storedVectorSize);
        }
        else if (addr == pimRegBytes) {
            storedElemBytes = *(pkt->getConstPtr<uint64_t>());
            DPRINTF(PIM, "--> [PIMMemCtrl] REG_ELEM_BYTES empfangen! Wert: %lu\n", storedElemBytes);
        }
        else if (addr == pimRegCmd) {
            uint64_t cmd = *(pkt->getConstPtr<uint64_t>());
            DPRINTF(PIM, "--> [PIMMemCtrl] REG_COMMAND empfangen! Befehl: %lu. Leite an PIMDRAMInterface weiter.\n", cmd);

            auto pimDram = dynamic_cast<PIMDRAMInterface*>(&dram[0]);

            if (pimDram) {
                // Diagnose-Ausgabe
                // pimDram->printPIMParameters(storedVectorSize, storedElemBytes, cmd);
                pimReadyTick = pimDram->triggerPIMExecution(storedVectorSize, storedElemBytes, cmd);
                DPRINTF(PIM, "--> [PIMMemCtrl] Hardware wird bei Tick %lu bereit sein.\n", pimReadyTick);
            } else {
                fatal("Fehler: Das zugewiesene DRAM-Interface ist kein PIMDRAMInterface!");
            }
        }
    }

    if (pkt->isRead() && addr == (pimRegCmd)) {
        DPRINTF(PIM, "--> [PIMMemCtrl] REG_COMMAND Lese-Zugriff erkannt bei Tick %lu\n", curTick());
        uint64_t finished_status = 0;
        pkt->setData((uint8_t*)&finished_status);
        pkt->makeResponse();
        Tick delay = (curTick() < pimReadyTick) ? (pimReadyTick - curTick()) : 0;

        if (delay > 0) {
            DPRINTF(PIM, "--> [PIMMemCtrl] Hardware beschäftigt! CPU schläft für %lu Ticks.\n", delay);

            auto wakeup_event = new gem5::EventFunctionWrapper(
                [this, pkt]() { 
                    DPRINTF(PIM, "--> [PIMMemCtrl] Zeit um! Wecke CPU auf.\n");
                    this->port.sendTimingResp(pkt); 
                }, 
                "PIM_Wakeup_Event"
            );
            schedule(wakeup_event, curTick() + delay);

        } else {
            // Falls PIM schon fertig ist, sofort zurückschicken
            DPRINTF(PIM, "--> [PIMMemCtrl] Hardware bereits fertig.\n");

            auto wakeup_event = new gem5::EventFunctionWrapper(
                [this, pkt]() { 
                    this->port.sendTimingResp(pkt); 
                }, 
                "PIM_Wakeup_Event"
            );
            schedule(wakeup_event, curTick());
        }
        return true;
    }

    // Alle anderen regulären CPU-Anfragen laufen normal weiter
    return MemCtrl::recvTimingReq(pkt);
}

} // namespace memory
} // namespace gem5
