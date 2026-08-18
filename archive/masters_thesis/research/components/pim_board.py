from typing import List

from m5.objects import (
    IOXBar,
    PciBus,
    Port,
    SystemXBar,
)
from m5.params import AddrRange

from gem5.components.boards.abstract_system_board import AbstractSystemBoard
from gem5.components.boards.se_binary_workload import SEBinaryWorkload
from gem5.components.cachehierarchies.abstract_cache_hierarchy import (
    AbstractCacheHierarchy,
)
from gem5.components.memory.abstract_memory_system import AbstractMemorySystem
from gem5.components.processors.abstract_processor import AbstractProcessor
from gem5.utils.override import overrides


class PIMBoard(AbstractSystemBoard, SEBinaryWorkload):
    def __init__(
        self,
        clk_freq: str,
        processor: AbstractProcessor,
        pim_processor: AbstractProcessor,
        memory: AbstractMemorySystem,
        cache_hierarchy: AbstractCacheHierarchy,
    ) -> None:
        super().__init__(
            clk_freq=clk_freq,
            processor=processor,
            memory=memory,
            cache_hierarchy=cache_hierarchy,
        )
        self.pim_processor = pim_processor
        self.pim_bus = SystemXBar()

    @overrides(AbstractSystemBoard)
    def get_mem_ports(self):
        """
        Wir überlisten die incorporate_cache Methode.
        Statt den echten RAM geben wir den PIM-Bus zurück.
        """
        full_range = self.get_memory().get_uninterleaved_range()

        # Wir geben die CPU-Seite unseres PIM-Busses als Ziel zurück
        if len(full_range) > 1:
            # Falls es mehrere Ranges gibt, kombinieren wir sie logisch
            # (für den membus reicht meist die Info über den ersten Port)
            return [(full_range[0], self.pim_bus.cpu_side_ports)]

        return [(r, self.pim_bus.cpu_side_ports) for r in full_range]

    @overrides(
        AbstractSystemBoard
    )  # Wir überschreiben die Methode der Elternklasse
    def _connect_things(self) -> None:
        if self._connect_things_called:
            return

        # --- SCHRITT 1: Speicher exklusiv an PIM-Bus ---
        self.get_memory().incorporate_memory(self)
        for ctrl in self.get_memory().get_memory_controllers():
            # Hier legen wir fest: Speicher hört NUR auf den PIM-Bus
            ctrl.port = self.pim_bus.mem_side_ports

        # --- SCHRITT 2: Host-System an PIM-Bus ---
        if self.get_cache_hierarchy():
            self.get_cache_hierarchy().incorporate_cache(self)
            # Der Host-Traffic "fließt" nun in den PIM-Bus ein
            # self.get_cache_hierarchy().membus.mem_side_ports = self.pim_bus.cpu_side_ports

        # --- SCHRITT 3: Prozessoren registrieren & PIM-Bypass legen ---
        self.get_processor().incorporate_processor(self)
        self.pim_processor.incorporate_processor(self)

        for core in self.pim_processor.get_cores():
            # PIM-Kerne gehen direkt an den PIM-Bus (isoliert vom Host-Bus!)
            core.connect_icache(self.pim_bus.cpu_side_ports)
            core.connect_dcache(self.pim_bus.cpu_side_ports)
            core.connect_walker_ports(
                self.pim_bus.cpu_side_ports, self.pim_bus.cpu_side_ports
            )
            core.connect_interrupt()

        # WICHTIG: Flag setzen, sonst bricht gem5 später ab
        self._connect_things_called = True

    @overrides(AbstractSystemBoard)
    def _setup_board(self) -> None:
        pass

    @overrides(AbstractSystemBoard)
    def has_io_bus(self) -> bool:
        return False

    @overrides(AbstractSystemBoard)
    def get_io_bus(self) -> IOXBar:
        raise NotImplementedError(
            "SimpleBoard does not have an IO Bus. "
            "Use `has_io_bus()` to check this."
        )

    @overrides(AbstractSystemBoard)
    def has_pci_bus(self) -> bool:
        return False

    @overrides(AbstractSystemBoard)
    def get_pci_bus(self) -> PciBus:
        raise NotImplementedError(
            "SimpleBoard does not have an PCI Bus. "
            "Use `has_pci_bus()` to check this."
        )

    @overrides(AbstractSystemBoard)
    def has_dma_ports(self) -> bool:
        return False

    @overrides(AbstractSystemBoard)
    def get_dma_ports(self) -> List[Port]:
        raise NotImplementedError(
            "SimpleBoard does not have DMA Ports. "
            "Use `has_dma_ports()` to check this."
        )

    @overrides(AbstractSystemBoard)
    def has_coherent_io(self) -> bool:
        return False

    @overrides(AbstractSystemBoard)
    def get_mem_side_coherent_io_port(self) -> Port:
        raise NotImplementedError(
            "SimpleBoard does not have any I/O ports. Use `has_coherent_io` to "
            "check this."
        )

    @overrides(AbstractSystemBoard)
    def _setup_memory_ranges(self) -> None:
        memory = self.get_memory()

        # The simple board just has one memory range that is the size of the
        # memory.
        self.mem_ranges = [AddrRange(memory.get_size())]
        memory.set_memory_range(self.mem_ranges)
