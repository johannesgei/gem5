#include "mem/pim/pim_mem_ctrl.hh"

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/DRAM.hh"
#include "debug/PIM.hh"
#include "mem/port_proxy.hh"
#include "sim/system.hh"

namespace gem5 {
namespace memory {

const Addr SECRET_PHYS_ADDR = 0x3FFFFFF0;

PIMMemCtrl::PIMMemCtrl(const PIMMemCtrlParams &p) :
    MemCtrl(p),
    triggerAddr(p.trigger_addr) {
}

bool
PIMMemCtrl::recvTimingReq(PacketPtr pkt) {
    // Wenn die physische Adresse übereinstimmt:
    if (pkt->getAddr() == triggerAddr) {
        warn("!!! PIM TRIGGER GEFUNDEN !!! Adresse: %#x", pkt->getAddr());
    }

    return MemCtrl::recvTimingReq(pkt);
}

} // namespace memory
} // namespace gem5
