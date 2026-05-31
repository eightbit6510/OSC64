#!/usr/bin/env python3
"""Convert C64 monitor dump (>C:addr  hh hh ...  ascii) to Kick Assembler .byte lines."""
from __future__ import annotations

import re
import sys
from pathlib import Path


def strip_ide_prefix(line: str) -> str:
    line = line.rstrip("\n\r")
    m = re.match(r"^\s*\d+\|", line)
    if m:
        line = line[m.end() :]
    return line


def line_to_bytes(line: str) -> list[str] | None:
    line = strip_ide_prefix(line)
    m = re.match(r"^>C:[0-9a-fA-F]+\s+", line)
    if not m:
        return None
    rest = line[m.end() :]
    # Monitor dumps: hex bytes then 3+ spaces then ASCII — ignore ASCII so we do not pick false hex tokens.
    hex_region = re.split(r"\s{3,}", rest, maxsplit=1)[0]
    tokens = [t for t in hex_region.split() if re.fullmatch(r"[0-9a-fA-F]{2}", t, re.I)]
    if not tokens:
        return None
    return tokens[:16] if len(tokens) >= 16 else tokens


def convert(text: str) -> str:
    out = []
    for line in text.splitlines():
        toks = line_to_bytes(line)
        if toks:
            out.append(".byte " + ", ".join("$" + t.upper() for t in toks))
    return "\n".join(out) + ("\n" if out else "")


def main() -> None:
    src = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent / "assets" / "spritebin"
    dst = Path(sys.argv[2]) if len(sys.argv) > 2 else src
    text = src.read_text(encoding="utf-8", errors="replace")
    dst.write_text(convert(text), encoding="utf-8")
    print(f"Wrote {dst}")


if __name__ == "__main__":
    main()
