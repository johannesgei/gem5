"""LPDDR5-6400 timings plus DRAMPower IDD currents.

gem5's stock LPDDR5_6400_1x16_BG_BL32 leaves all IDD/VDD at 0, so
totalEnergy in stats.txt is always zero. This subclass keeps the stock
timings and fills currents from a Micron LPDDR5 x16 6400 Mb/s WT table
(Y31M datasheet, Rev. A 05/2021). gem5 has two voltage domains: VDD1
maps to VDD, VDD2H to VDD2. VDDQ I/O current is not modeled separately.

https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8885/441b-315b-y31m-lpddr5-multi-pkg.pdf
"""

from gem5.components.memory.dram_interfaces.lpddr5 import (
    LPDDR5_6400_1x16_BG_BL32,
)


class LPDDR5_6400_1x16_BG_BL32_IDD(LPDDR5_6400_1x16_BG_BL32):
    """Stock LPDDR5-6400 BG/BL32 timings with datasheet IDD for DRAMPower."""

    VDD = "1.8V"
    VDD2 = "1.05V"

    # Operating 1-bank act-precharge
    IDD0 = "2.80mA"
    IDD02 = "32mA"

    # Precharge power-down (fast and slow share the datasheet IDD2P/IDD2PS)
    IDD2P0 = "1.20mA"
    IDD2P02 = "1.80mA"
    IDD2P1 = "1.20mA"
    IDD2P12 = "1.80mA"

    # Precharge standby
    IDD2N = "1.20mA"
    IDD2N2 = "16mA"

    # Active power-down
    IDD3P0 = "1.30mA"
    IDD3P02 = "4.80mA"
    IDD3P1 = "1.30mA"
    IDD3P12 = "4.80mA"

    # Active standby
    IDD3N = "1.60mA"
    IDD3N2 = "23mA"

    # Burst read / write (x16, 6400 Mb/s)
    IDD4R = "7.10mA"
    IDD4R2 = "360mA"
    IDD4W = "6.10mA"
    IDD4W2 = "280mA"

    # Refresh
    IDD5 = "20mA"
    IDD52 = "160mA"

    # Self-refresh, 25 C typical
    IDD6 = "0.25mA"
    IDD62 = "0.60mA"
