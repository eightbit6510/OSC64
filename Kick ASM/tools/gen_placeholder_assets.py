#!/usr/bin/env python3
"""
Emit placeholder PETSCII assets (1000 bytes each):
  assets/petscii_screen.bin — screen codes ($20 = space, $a0 = shifted space, etc.)
  assets/petscii_color.bin  — color RAM nybbles (low 4 bits = C64 color 0–15)
Replace with your editor export (e.g. Marq’s PETSCII, Petmate, etc.).
"""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCR = ROOT / "assets" / "petscii_screen.bin"
COL = ROOT / "assets" / "petscii_color.bin"


def main() -> None:
    screen = bytearray(1000)
    color = bytearray(1000)
    for row in range(25):
        for col in range(40):
            i = row * 40 + col
            # Simple checker: PETSCII block / inverse space
            c = 0xA0 if (row // 2 + col // 2) & 1 else 0x20
            screen[i] = c
            color[i] = (1 + (row + col) % 7) & 0x0F  # avoid black on black

    SCR.write_bytes(screen)
    COL.write_bytes(color)
    print(f"Wrote {SCR} and {COL} (1000 bytes each)")


if __name__ == "__main__":
    main()
