# ps2uke — porting Duke Nukem 3D to the PlayStation 2

This document is the architecture map of the port: what the base is, how the PS2
seam is wired, and the staged plan. The code is the territory; this is the map.

> **Full subsystem deep-dive (with diagrams):**
> [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — boot sequence, video/input/
> filesystem/audio pipelines, EE↔IOP model, and the gotchas.

> **History:** the project first tried the **icculus** Duke3D source (no engine,
> no `main()`), then **Chocolate Duke3D** (dead-ended on its VESA-BIOS palette
> path — there is no VESA on a PS2, so the screen stayed black after the intro).
> The working base is **JFDuke3D**. The older `progress.md` entries trace that
> journey; everything below describes the current JFDuke3D port.

## What this repo is

ps2uke is a PlayStation 2 port of **[JFDuke3D](https://github.com/jonof/jfduke3d)**
(Jonathon Fowler's port of 3D Realms' Duke Nukem 3D). JFDuke3D is the right base
for a console because it already factors the platform behind a clean engine
**baselayer**, and it ships the whole stack we need, vendored here at the repo
root:

| Dir            | What it is                                                            |
|----------------|-----------------------------------------------------------------------|
| `jfbuild/`     | Ken Silverman's BUILD engine + JonoF's portable **baselayer** seam. The classic 8-bit software renderer (`a-c.c`/`engine.c`) and the SDL2 platform layer `sdlayer2.c`. |
| `src/`         | The Duke game (actors, player, weapons, AI, menus, CON). `app_main()` lives in `game.c`. |
| `jfmact/`      | Input/control library (the keyboard/mouse/joystick → game-function layer Duke reads). |
| `jfaudiolib/`  | Apogee-style audio (MultiVoc mixer + music + per-OS output drivers).   |
| `ps2/`         | **Our** PS2 code: the `Makefile`, the cdfs/GRP/input/stubs glue in `ps2/platform/`, the ISO builder, and `duke3d.cfg`. |

It is built as the **classic 8-bit software renderer** — `USE_POLYMOST=0`,
`USE_OPENGL=0`, `RENDERTYPE=SDL` — which is what suits the Emotion Engine.

### Why JFDuke3D and not Chocolate / icculus

The decisive difference is the **palette**. Chocolate pokes the palette straight
at the VESA BIOS (`VBE_setPalette`), which does not exist on a PS2 — hence the
post-intro black screen. In jfbuild the **engine owns the palette**
(`curpalettefaded`) and the platform only has to *present* an already-finished
8-bit frame. That is a clean handoff a console can implement. The baselayer seam
is small and explicit:

```
setvideomode() / resetvideomode()   begindrawing() / enddrawing()
showframe()                         setpalette()
handleevents()                      getticks() / gettimerfreq()
```

`sdlayer2.c` implements those over SDL2, and the PS2 SDL2 port renders the
software frame through **gsKit**. So "port to PS2" became "drive the existing
SDL2 baselayer," not "rewrite the renderer."

## The PS2 seam (what we had to implement)

Everything PS2-specific lives in `ps2/platform/` and a few guarded
(`#if defined(_EE)`) branches in the vendored sources.

| Concern              | How it works on PS2                                                                  |
|----------------------|-------------------------------------------------------------------------------------|
| **Entry / IOP boot** | `libSDL2main`'s PS2 `main()` resets the IOP + applies SBV patches, then calls `SDL_main`. We make `sdlayer2.c`'s `main` become `SDL_main` (`-Dmain=SDL_main`) so that bring-up runs first. |
| **Video**            | SDL2-over-gsKit presents the engine's 8-bit frame; `setvideomode`/`showframe` drive it. Engine owns the palette. |
| **Filesystem**       | GRP + all data sit on the boot disc, reached through `cdfs.irx` — a *legacy ioman* device `open()` can't see. jfbuild's `Bopen/Bread/Blseek/Bclose` are routed (in `compat.h`) to `ps2_fileio.c`, which uses the **fio** API (`fioOpen`, `FIO_O_RDONLY`) with tagged fds and loops short sector reads. cdfs is brought up lazily (`sbv_patch_enable_lmb` + `init_cdfs_driver`). |
| **Input**            | SDL2's PS2 port exposes **no** joystick backend, so `ps2_pad.c` reads the DualShock with **libpad** directly and `sdlayer2.c`'s `handleevents()` injects BUILD key events through the same `keystatus[]`/`keyfifo[]` path as a keyboard. One button→key map serves menus and gameplay (modelled on ps2quake's `IN_PadButtons`). |
| **Audio**            | A native **audsrv** driver for jfaudiolib (MultiVoc fills a buffer; a feed thread streams it to `audsrv_play_audio`, like ps2quake's `snd_ps2.c`). *In progress — currently `driver_nosound` (silent).* |
| **Stubs**            | `ps2_stubs.c`: app-icon/`build_*` symbols + an `init_joystick_driver` **no-op** that dodges libps2_drivers' `mtapInit` deadlock (it spins on the unloaded `mtapman` IOP module). |

cdfs/sound/IOP bring-up mirrors the proven **ps2quake** and **ps2oom** ports.

## Build & run

The ps2dev toolchain runs in Docker (`Dockerfile`, image `ps2uke-dock:local`):

```sh
./build.sh                 # make in ps2/  ->  ps2/ps2uke.elf
./make_iso.sh [grp-dir]    # stage SYSTEM.CNF + ELF + DUKE3D.GRP + DUKE3D.CFG -> dist/ps2uke.iso
```

Then boot `dist/ps2uke.iso` in PCSX2 or on hardware. (PCSX2 logs to
`emulog.txt`; the emulator is never auto-launched from here.)

**Game data (`DUKE3D.GRP`, demos, maps) is never committed** — it stays
copyrighted to 3D Realms and must be user-supplied at runtime.

## Status & roadmap

- [x] **Base** — JFDuke3D (engine + game + jfmact + jfaudiolib) compiles & links for PS2.
- [x] **Filesystem** — cdfs/GRP off the boot disc (fio shim); `DUKE3D.GRP` (Atomic 1.5) loads.
- [x] **Video** — **GS hardware present** (`ps2_gs.c`): the 8-bit frame goes up as a `PSMT8` texture + `CT32` CLUT and the GS does the palette-expand + 320×200→640×448 upscale. Locked ~60 fps.
- [x] **Input** — DualShock via libpad → BUILD keys; menus *and* gameplay.
- [x] **Playable** — boots end to end into levels and runs the 3D world at speed.
- [ ] **Audio** — native audsrv driver for jfaudiolib (SFX), then MIDI music (replace `driver_nosound`).
- [ ] **Analog sticks** — left stick move/strafe, right stick look (jfmact CONTROL axes).
- [ ] **Polish** — `a-ee.s` rasterizer / SPRAM, save games, faster cdfs (skip the loose-file probe).
