# ps2uke — porting Duke Nukem 3D to the PlayStation 2

This document tracks the architecture of the port and the staged plan to bring it
up on real PS2 hardware via the [ps2dev](https://github.com/ps2dev/ps2dev)
toolchain. It is the map; the code is the territory.

## What this repo is (and what it was missing)

`source/` is the **icculus.org port of 3D Realms' Duke Nukem 3D** — but only the
*game logic* (actors, player, weapons, AI, menus, CON scripting, sound glue).
Duke is built *on top of* Ken Silverman's **BUILD engine** (the renderer + map/
sector/sprite world), which the 2003 source release shipped separately. The game
code proves this dependency directly — `source/duke3d.h` does:

```c
#include "buildengine/pragmas.h"
#include "buildengine/build.h"
#include "buildengine/engine.h"
#include "buildengine/engine_protos.h"
```

…none of which existed in the upstream `icculus/duke3d` checkout. Without the
engine, nothing renders and nothing links.

## What was vendored, and from where

`source/buildengine/` is the **portable** BUILD engine, vendored from
[`icculus/buildengine`](https://github.com/icculus/buildengine) (icculus.org's
cross-platform port of Ken Silverman's engine). This is the right base because:

- It is already portable C with a **32-bit `long`/pointer** model — which matches
  the PS2 EE (`mips64r5900el`, n32 ABI), exactly the assumption BUILD was written
  under.
- Its `#include` layout (`pragmas.h` / `build.h` / `engine.h` / `engine_protos.h`)
  is precisely what `source/duke3d.h` expects — so Duke's includes now resolve.
- It already factors the OS-specific layer behind a clean **driver seam**, so
  adding a console is "write one more driver," not "rewrite the engine."

Two upstreams were evaluated and rejected as the base:
- **`kenbuild.zip`** (Ken's original): pure DOS/x86 — `A.ASM`, Watcom inline-asm
  `PRAGMAS.H`, VGA mode-13h video. Would need a from-scratch portability pass.
- **`buildc.zip`** (Ken's C port of `A.ASM`): portablizes the inner draw loops
  (`a.c`) but `engine.c` still uses `<dos.h>`/`<conio.h>` + VESA video.

The icculus engine already did all of that portability work; both of the above
remain useful as reference for the rendering math.

## The platform seam (what PS2 has to implement)

The engine renders into an **8-bit palettized framebuffer** and presents it via a
small driver. The reference driver is `source/buildengine/sdl_driver.c`; the DOS
one is `dos_drvr.c`. `platform.h` picks the per-OS compat header off a
`PLATFORM_*` macro. The seam, concretely:

| Driver responsibility            | SDL reference            | PS2 implementation (`ps2_driver.c`)                         |
|----------------------------------|--------------------------|------------------------------------------------------------|
| The framebuffer (`screen`)       | SDL surface pixels       | EE-RAM 8-bit buffer; globals `xres/yres/bytesperline`      |
| Set video mode                   | `setvmode()`             | gsKit init (640×480 / 320×200-ish), set `frameplace`       |
| Set palette                      | `VBE_setPalette()`       | upload 256-entry CLUT to the GS                            |
| Present a frame                  | `_nextpage()`            | palette-expand `screen` → GS texture, draw, vsync flip     |
| Timing                           | `getticks()`             | EE timer / `PLATFORM_TIMER_HZ`                             |
| Input → BUILD key/mouse events   | SDL event pump           | read PS2 pad, synthesize the keyboard/mouse codes BUILD wants |

This is the same shape as the doomgeneric → ps2oom port, where the entire console
layer collapsed to `DG_DrawFrame()` + pad + timer.

## Staged roadmap

- [x] **0. Foundation** — vendor the portable BUILD engine so the source tree is
      complete and Duke's `#include "buildengine/..."` lines resolve.
- [ ] **1. Compat header** — add `PLATFORM_PS2` to `platform.h` and a
      `ps2_compat.h` (endianness = little, `__inline`, 32-bit types, no mmap).
- [ ] **2. `ps2/` build** — a `ps2/Makefile` for the ps2dev toolchain that
      compiles `source/` + `source/buildengine/` + `source/audiolib/` with the
      generic portable `pragmas.c` (not the x86 `pragmas_gnu.c`/`a_gnu.c`) and
      links `ps2_driver.c`. Iterate until it *compiles*, then until it *links*.
- [ ] **3. Video** — `ps2_driver.c`: gsKit framebuffer + CLUT, `_nextpage()`
      blit/flip. First light = the BUILD/Duke palette test, then the title.
- [ ] **4. Input** — PS2 pad → BUILD key/mouse events; get through the menu.
- [ ] **5. Filesystem** — `cache1d.c` GRP access off the PS2 (CD/USB/host); Duke
      needs `DUKE3D.GRP` (user-supplied; never committed — see `.gitignore`).
- [ ] **6. Audio** — port `source/audiolib/` (FX + music) onto the PS2 SPU2 /
      audsrv, mirroring ps2oom's audio path.
- [ ] **7. Polish** — performance (EE cache, GS upload), save games, controls.

## Build (today)

The ps2dev toolchain environment is already wired:

```sh
./build.sh shell     # interactive shell in the ps2dev toolchain (cwd = ps2/)
./build.sh           # runs `make` in ps2/ (Makefile lands in stage 2)
```

Game data (`DUKE3D.GRP`, `*.RTS`, `*.MAP`, demos) is **never** committed — it
stays copyrighted to 3D Realms / id and must be user-supplied at runtime.
