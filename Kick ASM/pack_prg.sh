#!/bin/bash
# Assemble chat64 and pack with exomizer for cartridge / ESP32 upload.
# IMPORTANT: use "sfx sys", NOT "sfx basic" — basic mode decrunches to $07FF
# and corrupts the program by one byte.

set -euo pipefail
cd "$(dirname "$0")"

SRC="osc64.prg"
OUT="osc64_packed.prg"
ASM="osc64.asm"
KICK="tools/KickAss.jar"
ESP_DIR="../ESP32 Sketch"

if [[ ! -f "$ASM" ]]; then
  echo "Missing $ASM"
  exit 1
fi

if [[ ! -f "$KICK" ]]; then
  echo "Missing $KICK"
  exit 1
fi

echo "Assembling $ASM -> $SRC"
java -jar "$KICK" "$ASM"

echo "Packing $SRC -> $OUT"
exomizer sfx sys -t64 -n -o "$OUT" "$SRC"

echo "Verifying decompression..."
exomizer desfx -o /tmp/chat64_verify.prg "$OUT" -q
python3 - "$SRC" /tmp/chat64_verify.prg <<'PY'
import struct, sys
orig = open(sys.argv[1], "rb").read()[2:]
dec = open(sys.argv[2], "rb").read()[2:]
if orig != dec:
    mism = sum(1 for a, b in zip(orig, dec) if a != b)
    print(f"ERROR: decompression mismatch ({mism} bytes differ)", file=sys.stderr)
    sys.exit(1)
print(f"OK: packed file decompresses to identical {len(orig)}-byte image")
PY

echo "Generating ESP32 prgfile.h"
python3 convertToArray.py "$OUT" "$ESP_DIR" "$ASM"

ls -la "$SRC" "$OUT"
