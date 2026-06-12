# Progress log

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
