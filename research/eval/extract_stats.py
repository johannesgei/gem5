#!/usr/bin/env python3
"""Pull DRAMPower energy and timing out of a gem5 SimpleBoard stats.txt."""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path

CSV_FIELDS = [
    "kernel",
    "mode",
    "N",
    "elem_bytes",
    "flops",
    "simSeconds",
    "dram_totalEnergy_pJ",
    "dram_actEnergy_pJ",
    "dram_readEnergy_pJ",
    "dram_writeEnergy_pJ",
    "dram_refreshEnergy_pJ",
    "bytes_read",
    "bytes_written",
    "energy_per_flop",
]


def flops_for(kernel: str, n: int) -> int:
    if kernel in ("axpy_double", "axpy_float", "stream_saxpy"):
        return 2 * n
    raise SystemExit(f"unknown kernel for FLOP formula: {kernel}")


def _stat(text: str, name: str) -> float:
    # Last matching value; ignore comments after '#'.
    matches = re.findall(
        rf"^\S*{re.escape(name)}\s+([0-9.eE+-]+)",
        text,
        flags=re.MULTILINE,
    )
    if not matches:
        raise SystemExit(f"stat not found: {name}")
    return float(matches[-1])


def _energy(text: str, field: str) -> float:
    """SimpleBoard DRAMPower dumps per-rank pJ (rank0.totalEnergy, ...)."""
    ranks = re.findall(
        rf"^\S*dram\.rank\d+\.{re.escape(field)}\s+([0-9.eE+-]+)",
        text,
        flags=re.MULTILINE,
    )
    if ranks:
        return sum(float(x) for x in ranks)
    return _stat(text, f"dram.{field}")


def extract(
    stats_path: Path, kernel: str, mode: str, n: int, elem_bytes: int
) -> dict:
    text = stats_path.read_text()
    flops = flops_for(kernel, n)
    total_e = _energy(text, "totalEnergy")
    row = {
        "kernel": kernel,
        "mode": mode,
        "N": n,
        "elem_bytes": elem_bytes,
        "flops": flops,
        "simSeconds": _stat(text, "simSeconds"),
        "dram_totalEnergy_pJ": total_e,
        "dram_actEnergy_pJ": _energy(text, "actEnergy"),
        "dram_readEnergy_pJ": _energy(text, "readEnergy"),
        "dram_writeEnergy_pJ": _energy(text, "writeEnergy"),
        "dram_refreshEnergy_pJ": _energy(text, "refreshEnergy"),
        "bytes_read": _stat(text, "dram.bytesRead::total"),
        "bytes_written": _stat(text, "dram.bytesWritten::total"),
        "energy_per_flop": (total_e / flops) if flops else "",
    }
    return row


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--stats", type=Path, required=True)
    p.add_argument("--kernel", required=True)
    p.add_argument(
        "--mode", required=True, choices=("cpu", "pim", "cpu_nocache")
    )
    p.add_argument("--N", type=int, required=True)
    p.add_argument("--elem-bytes", type=int, required=True)
    p.add_argument(
        "--append", type=Path, default=None, help="CSV to create or append"
    )
    args = p.parse_args()

    row = extract(args.stats, args.kernel, args.mode, args.N, args.elem_bytes)
    writer = csv.DictWriter(sys.stdout, fieldnames=CSV_FIELDS)
    writer.writeheader()
    writer.writerow(row)

    if args.append:
        args.append.parent.mkdir(parents=True, exist_ok=True)
        new_file = not args.append.exists()
        with args.append.open("a", newline="") as f:
            w = csv.DictWriter(f, fieldnames=CSV_FIELDS)
            if new_file:
                w.writeheader()
            w.writerow(row)

    if float(row["dram_totalEnergy_pJ"]) == 0.0:
        print(
            "warning: dram_totalEnergy_pJ is 0 — IDD currents may still be unset",
            file=sys.stderr,
        )
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
