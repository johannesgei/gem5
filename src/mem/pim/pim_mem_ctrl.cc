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
    Addr addr = pkt->getAddr();
    // Wir reagieren nur auf Schreibzugriffe (STOREs) der CPU
    if (pkt->isWrite()) {

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

            auto pimDram = dynamic_cast<PIMDRAMInterface*>(&dram[0]);

            if (pimDram) {
                // Diagnose-Ausgabe
                pimDram->printPIMParameters(storedVectorSize, storedElemBytes, cmd);
                // Tick pim_latency = pimDram->calculatePIMLatency(storedVectorSize, storedElemBytes);
                // warn("--> [PIMMemCtrl] Berechnete PIM-Latenz: %lu Ticks", pim_latency);
                pimReadyTick = pimDram->triggerPIMExecution(storedVectorSize, storedElemBytes, cmd);
                warn("--> [PIMMemCtrl] Hardware wird bei Tick %lu bereit sein.", pimReadyTick);
            } else {
                fatal("Fehler: Das zugewiesene DRAM-Interface ist kein PIMDRAMInterface!");
            }
        }
    }

    if (pkt->isRead() && addr == (pimRegCmd)) {
        warn("--> [PIMMemCtrl] REG_COMMAND Lese-Zugriff erkannt bei Tick %lu", curTick());
        uint64_t finished_status = 0;
        pkt->setData((uint8_t*)&finished_status);
        pkt->makeResponse();
        Tick delay = (curTick() < pimReadyTick) ? (pimReadyTick - curTick()) : 0;

        if (delay > 0) {
            warn("--> [PIMMemCtrl] Hardware beschäftigt! CPU schläft für %lu Ticks.", delay);

            // 3. DER TRICK: Ein super simples Einweg-Event (Lambda), das sich selbst zerstört
            auto wakeup_event = new gem5::EventFunctionWrapper(
                [this, pkt]() { 
                    warn("--> [PIMMemCtrl] Zeit um! Wecke CPU auf.");
                    this->port.sendTimingResp(pkt); 
                }, 
                "PIM_Wakeup_Event"
            );

            // Exakt für den Ziel-Tick einplanen
            schedule(wakeup_event, curTick() + delay);

        } else {
            // Falls PIM schon fertig ist, sofort zurückschicken
            warn("--> [PIMMemCtrl] Hardware bereits fertig.");
            port.sendTimingResp(pkt);
        }

        // // Wir holen uns den aktuellen Status (1 oder 0) direkt aus dem Interface
        // auto pimDram = dynamic_cast<PIMDRAMInterface*>(&dram[0]);
        // uint64_t current_status = pimDram->readPimCommand();
        
        // Daten in das gem5-Paket schreiben, damit die CPU sie empfängt

        return true;
    }

    // Alle anderen regulären CPU-Anfragen laufen normal weiter
    return MemCtrl::recvTimingReq(pkt);
}

} // namespace memory
} // namespace gem5
