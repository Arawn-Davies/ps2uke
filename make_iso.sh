#!/usr/bin/env bash
#
# Build a bootable PS2 ISO for ps2uke that PCSX2 (or a real PS2) can run.
#
# The ELF reads its data off the boot disc via the "cdfs:" device, so the ISO
# must carry DUKE3D.GRP. The IOP modules (cdfs etc.) are embedded in the ELF by
# ps2_drivers, so no irx/ dir is needed on the disc.
#
# Disc layout produced:
#   /SYSTEM.CNF        -> boots cdrom0:\PS2UKE.ELF
#   /PS2UKE.ELF        (built by ./build.sh)
#   /DUKE3D.GRP        (read at runtime as cdfs:/DUKE3D.GRP)
#
# Usage:
#   ./make_iso.sh [dir-containing-DUKE3D.GRP]
# Default GRP dir:
#   /mnt/c/Users/azama/Downloads/duke3d
#
# Output: dist/ps2uke.iso
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
IMAGE="ps2uke-dock:local"
GRPDIR="${1:-/mnt/c/Users/azama/Downloads/duke3d}"
STAGE="$ROOT/dist/iso"
OUT="$ROOT/dist/ps2uke.iso"

[ -f "$ROOT/ps2/ps2uke.elf" ] || { echo "!! ps2/ps2uke.elf missing -- run ./build.sh first"; exit 1; }

grp="$(find "$GRPDIR" -maxdepth 1 -iname 'duke3d.grp' | head -1 || true)"
[ -n "$grp" ] || { echo "!! DUKE3D.GRP not found in $GRPDIR"; exit 1; }

echo ">> staging ISO tree in $STAGE"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp "$ROOT/ps2/SYSTEM.CNF"   "$STAGE/SYSTEM.CNF"
cp "$ROOT/ps2/ps2uke.elf"   "$STAGE/PS2UKE.ELF"
cp "$grp"                   "$STAGE/DUKE3D.GRP"
cp "$ROOT/ps2/duke3d.cfg"   "$STAGE/DUKE3D.CFG"   # ship a 320x200 config

echo ">> building $OUT  (GRP: $grp)"
mkdir -p "$ROOT/dist"
# Plain ISO9660 level 2 (allows our 8.3 names). cdfs is case-insensitive, so the
# lowercase cdfs:/DUKE3D.GRP path matches the uppercase disc entry.
docker run --rm -v "$ROOT":/work -w /work "$IMAGE" \
    xorriso -as mkisofs -iso-level 2 -l -o dist/ps2uke.iso dist/iso

echo ">> done:"
ls -la "$OUT"
