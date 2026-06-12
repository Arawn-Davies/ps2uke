# Progress log

## 2026-06-12 (cont.) — Stage 6: real Duke ART on screen — TITLE logo (`005d8bd`)

The Duke3D TITLE tile (2486) renders full-screen in PCSX2 in the real palette --
the "DUKE NUKEM 3D" logo. The ART/tile pipeline works end-to-end.

`ps2_main.c` follows Ken's TEST.C order: `initengine` -> `loadpics("tiles000.art")`
-> `setgamemode(0,320,200)` -> loop `clearview`/`rotatesprite(TITLE)`/`nextpage`.
Console confirms `loadpics(...)=0` (catalog read from the GRP) and palette LOADED.

`_setgamemode` now also sets the engine render globals `rotatesprite` needs
(`xdim/ydim`, `ylookup[]`, `horizycent`, aspect invalidation, `setvlinebpl`,
`setview`, `clearallviews`) -- mirroring sdl_driver's `go_to_new_vid_mode`.

Stages 0-6 done + hardware-verified: cross-compile -> cdfs/GRP -> palette ->
gsKit display -> real Duke artwork.

Note: the per-ART-file "CAN NOT FOUND" log spam (~2 s startup) is the engine
probing for loose files before the GRP fallback; harmless, optimisable later by
not prepending `cdfs:/` for in-GRP lookups.

### Next: the game, and input
- PS2 pad -> BUILD input (the keyboard/mouse events the menu/game read).
- Start wiring the Duke game objects (`source/*.c`): the real menu + game loop
  (`drawrooms` for the 3D world), replacing this standalone test harness.

## 2026-06-12 (cont.) — WORKING on PCSX2: real Duke palette on screen (`54b7c74`)

End-to-end verified in PCSX2: colour bars render in Duke's **real palette**,
loaded from `PALETTE.DAT` inside `DUKE3D.GRP` off the disc via cdfs. The whole
stack is live: `cdfs.irx` -> fio shim -> `cache1d`/GRP -> `initengine`/
`loadpalette` -> gsKit T8 + CT32 CLUT -> display.

The first gsKit attempt was black on hardware; the working present path mirrors
ps2oom's proven PSMT8 backend:
- explicit video mode (`Mode=NTSC`, `Interlace`/`Field`, `Width/Height`, `PSMZ`)
  instead of auto-detect;
- `dmaKit_init(..., 1 << DMA_CHANNEL_GIF)` (the GIF channel mask, not `0`);
- `gsKit_TexManager` (init/invalidate/bind/nextFrame), not manual
  `vram_alloc`/`texture_upload`;
- a **128-byte-aligned** `memalign` upload buffer that `screen` is `memcpy`'d
  into each frame (GS DMA needs aligned source; `malloc`'d `screen` wasn't);
- per-frame CT32 CLUT rebuild from the live palette with the CSM1 swizzle.

Also fixed the cdfs bring-up: `init_cdfs_driver()` needs an IOP reset + SBV
patches first (`SifInitRpc`/`SifIopReset`/`sbv_patch_enable_lmb`), the work
SDL2main does for ps2oom -- added as `ps2_iop_init()` before any driver init.

### Next: real Duke pixels, then the game loop
The palette proves the data path. Next, draw an actual ART tile (`loadpics` +
`rotatesprite`, e.g. the title/loading screen) for real Duke graphics, then
start wiring the Duke game objects (`source/*.c`) + PS2 pad input.

## 2026-06-12 (cont.) — Stage 5: cdfs/GRP filesystem (`47a6d48`)

Real Duke data now loads off the boot disc. `cache1d.c`'s POSIX `open/read/lseek/
close` are redirected (4-line splice, after its system includes) to a fio-backed
shim `ps2_fileio.c`: cdfs.irx is a legacy ioman device newlib's `open()` can't
reach and that rejects `O_RDONLY(0)`, so read-only disc opens go through
`fioOpen`/`fioRead`/`fioLseek` (`FIO_O_RDONLY`) with a tagged fd; bare filenames
resolve to `cdfs:/`. (Safe because cache1d keeps group-vs-external in `filegrp[]`,
not in the fd bits.)

`ps2_main.c` is the stage-5 test: `init_cdfs_driver()` →
`initgroupfile("cdfs:/DUKE3D.GRP")` → `initengine()` (loads `PALETTE.DAT` from
the GRP) → render the colour bars through the engine's **real palette**.

**ISO build:** `./make_iso.sh [grp-dir]` stages `SYSTEM.CNF` + `PS2UKE.ELF` +
`DUKE3D.GRP` into a bootable ISO9660 (`xorriso`, added to the Dockerfile).
Defaults to the GRP at `/mnt/c/Users/azama/Downloads/duke3d`. Output:
`dist/ps2uke.iso` (~47 MB; never committed — contains the copyrighted GRP).

**Verified:** compiles + links clean into a PS2 ELF, ISO builds (root entries
`DUKE3D.GRP`/`PS2UKE.ELF`/`SYSTEM.CNF`). **Runtime/visual needs PCSX2 or hardware
(not auto-launched).** To test: `./build.sh && ./make_iso.sh`, then boot
`dist/ps2uke.iso` in PCSX2. Expected console log: `initgroupfile(...) = <fd>`
(>= 0 means the GRP opened), then colour bars in Duke's real palette. If
`initgroupfile` returns -1, the GRP isn't being found on the disc.

### Next: real Duke pixels, then the game loop
With the GRP readable, draw an actual ART tile (`loadpics` + `rotatesprite`,
e.g. the loading/title screen), then start wiring the Duke game objects
(`source/*.c`) and the PS2 pad input.

## 2026-06-12 (cont.) — Stage 4: gsKit framebuffer present path (`d5dce24`)

`ps2_driver.c` now brings up gsKit and presents the engine's 8-bit framebuffer:
`ps2_video_init()` (dmaKit + `gsKit_init_global`/`init_screen`, NTSC, double-
buffered, Z off) sets up a `GS_PSM_T8` texture backed directly by the engine's
`screen` buffer with a `GS_PSM_CT32` CLUT; `_nextpage()` uploads screen+CLUT to
VRAM, blits fullscreen-scaled (GS does the indexed→RGB expansion), vsync-flips;
`VBE_setPalette()` scales BUILD's 0..63 channels to 0..255 and applies the GS
CSM1 CLUT index swizzle (bits 3/4 swapped).

`ps2_main.c` is a **data-free** framebuffer test — fills `screen` with colour
bars + a brightness gradient and presents every frame. **Builds clean, links
into a valid ELF32 MIPS executable.**

**Verification status:** compiles + links only. Visual confirmation needs running
`ps2/ps2uke.elf` on **PCSX2** or hardware (not auto-launched here). Expected: 16
vertical colour bars over a 16-step vertical brightness gradient. If the bars are
miscoloured, the CLUT swizzle/`VBE_setPalette` scaling is the first suspect; if
nothing shows, check the gsKit mode (NTSC vs PAL) and the T8 texture upload.

To run: `./build.sh` (from repo root) → load `ps2/ps2uke.elf` in PCSX2.

### Next: real palette/art (needs Duke data)
Supply `DUKE3D.GRP` (shareware ~11 MB is fine) or the loose `PALETTE.DAT` +
`TILES000.ART`. Then swap the synthetic palette/pattern for `initengine()` +
`loadpalette()` + a real ART tile, and we get actual Duke pixels on screen.

## 2026-06-12 (cont.) — BUILD engine compiles + links for PS2 (stages 1–3)

Verified against the real ps2dev toolchain (gcc 15.2.0) in Docker.

- **Stage 1 — compat layer** (`678b11a`): `PLATFORM_PS2` branch in `platform.h`
  + `ps2_compat.h` (DOSism shims, little-endian, `Uint8/16/32`, timer 120Hz, no
  SDL).
- **Stage 2/3 — build + driver** (`f51c3ca`): `source/buildengine/ps2_driver.c`
  (PS2 platform layer: owns the 8-bit `screen` framebuffer + globals, implements
  `setvmode`/`_setgamemode`/`VBE_setPalette`/`_nextpage`/`drawpixel`/`drawline16`/
  mouse/timer/`filelength`/`stricmp`), `ps2/Makefile` (ps2dev `Makefile.eeglobal`),
  and `ps2/ps2_main.c` smoke-test.

**Milestone reached:** `engine.c + a.c + pragmas.c + cache1d.c + ps2_driver.c`
compile clean and **link into a valid ELF32 MIPS executable** (`ps2uke.elf`,
~2.2 MB). `engine.c` compiled `-DPLATFORM_PS2` with zero changes — the portability
work was all in the compat header + driver. Presentation/palette/pad in the
driver are correct-but-stubbed (no gsKit yet), so the ELF links and boots through
init but doesn't display — that's stage 4.

How to reproduce: `./build.sh` from repo root (or `make` in `ps2/` inside the
image). CI (`ps2.yml`) now builds `ps2/` for real.

### Next: stage 4 — first light
- Flesh out `ps2_driver.c` with gsKit: allocate a GS framebuffer + CLUT, and make
  `_nextpage()` palette-expand `screen` → GS texture and vsync-flip; upload the
  palette in `VBE_setPalette`. Needs `-lgraph -ldraw -ldma -lpacket` etc. in
  `EE_LIBS`. First visible target = palette/title screen with real ART data.
- Then PS2 pad → BUILD input (`readmousexy`/key events), GRP filesystem in
  `cache1d`, and begin wiring the Duke game objects (`source/*.c`, stage 5+).

## 2026-06-12 — Foundation laid

### Where the repo stands
- GitHub: <https://github.com/Arawn-Davies/ps2uke> (branch `main`).
- Working tree is **consistent and non-broken**: no half-written code. CI stays
  green because `ps2.yml` only builds `ps2/` when a `ps2/Makefile` exists and
  otherwise just verifies the ps2dev cross-toolchain (none exists yet — stage 2).

### What's in place
- **PS2 toolchain scaffolding** — `Dockerfile`, `docker-compose.yml`, `build.sh`
  (ps2dev image + make/bash), `.github/workflows/ps2.yml`, `.gitignore`
  (build output + never-commit game data).
- **Source tree is now complete** — `source/` is the icculus Duke3D *game logic*;
  it was missing the renderer. Vendored the portable BUILD engine into
  `source/buildengine/` from [`icculus/buildengine`](https://github.com/icculus/buildengine),
  so `duke3d.h`'s `#include "buildengine/{pragmas,build,engine,engine_protos}.h"`
  now resolve. 39 engine files added.
- **Docs** — `PORTING.md` (architecture, provenance, platform seam, staged
  roadmap) and this log.

### Key findings (so the next session doesn't re-derive them)
- `icculus/duke3d` ships game logic only; the BUILD engine was a separate release.
- `kenbuild.zip` = pure DOS/x86 (asm pragmas, VGA). `buildc.zip` = Ken's C port of
  `A.ASM` but `engine.c` still DOS. Both rejected as the base in favor of
  `icculus/buildengine`, which is already portable C and **32-bit `long`/pointer**
  clean — matching the PS2 EE n32 ABI that BUILD assumes.
- Platform seam = a driver (`sdl_driver.c` / `dos_drvr.c`, chosen in `platform.h`)
  that owns the 8-bit `screen` framebuffer and exposes `setvmode()`,
  `VBE_setPalette()`, `_nextpage()` (present), `getticks()`. PS2 = one more driver.

### Commits today
- `29d9aad` Initial commit: Duke3D source + PS2 toolchain scaffolding
- `761b1fc` Vendor portable BUILD engine + PS2 porting roadmap

### Next session starts here (stage 1–3 in PORTING.md)
1. Add `PLATFORM_PS2` to `source/buildengine/platform.h` + a `ps2_compat.h`
   (little-endian, `__inline`, 32-bit types).
2. Write `ps2/Makefile` for the ps2dev toolchain: compile `source/` +
   `source/buildengine/` + `source/audiolib/` using the **generic** portable
   `pragmas.c` (NOT the x86 `pragmas_gnu.c` / `a_gnu.c`). Iterate to *compile*,
   then to *link* against a stub `ps2_driver.c`.
3. Flesh out `ps2_driver.c` (gsKit framebuffer + CLUT, `_nextpage()` flip).
   First light = palette/title screen.

### Not done (intentionally — large stages, not one-session work)
- No `ps2/Makefile`, no `ps2_driver.c`/`ps2_compat.h` yet → nothing compiles for
  PS2 yet. Audiolib port, GRP filesystem, and input are stages 4–6.
- `DUKE3D.GRP` and other game data remain user-supplied; never committed.
