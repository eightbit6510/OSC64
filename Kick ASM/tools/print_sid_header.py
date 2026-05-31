#!/usr/bin/env python3
"""Print PSID/RSID header fields (load/init/play, data size)."""
from __future__ import annotations

import struct
import sys
from pathlib import Path


def main() -> None:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent / "assets" / "relic.sid"
    data = path.read_bytes()
    if len(data) < 124 or data[0:4] not in (b"PSID", b"RSID"):
        print("Not a PSID/RSID file:", path)
        sys.exit(1)
    ver, off, load, init_, play = struct.unpack_from(">HHHHH", data, 4)
    print(path.name, magic := data[0:4].decode("ascii"), "v", ver)
    print("  dataOffset:   ", off)
    print("  loadAddress:  ", f"${load:04x}" + (" (use first 2 data bytes if zero)" if load == 0 else ""))
    print("  initAddress:  ", f"${init_:04x}")
    print("  playAddress:  ", f"${play:04x}")
    if load == 0 and len(data) >= off + 2:
        emb, = struct.unpack_from("<H", data, off)
        print("  embedded load:", f"${emb:04x}")
        print("  music bytes:  ", len(data) - off - 2)
        print("  end approx:    ", f"${emb + len(data) - off - 2 - 1:04x}")
    print()
    print("Kick Assembler: * = $<load> / .import c64 \"...sid\"")
    print("Call init with A=song (often 0), X=Y=0; call play each frame.")


if __name__ == "__main__":
    main()
