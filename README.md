# ps2uke

**Duke Nukem 3D on the PlayStation 2.** A port of
[JFDuke3D](https://github.com/jonof/jfduke3d) (engine + game + input + audio) to
the PS2, built with the [ps2dev](https://github.com/ps2dev/ps2dev) toolchain and
booted from an ISO in PCSX2 or on real hardware.

> **Status (2026-06-13): playable.** Boots through cdfs → `DUKE3D.GRP` → palette →
> ART to the menu and **into levels at a vsync-capped 60 fps**. The engine's 8-bit
> frame is handed to the GS as a **`PSMT8` texture + `CT32` CLUT**, so the Graphics
> Synthesizer does the indexed→RGB palette-expand *and* the 320×200→640×448 upscale
> **in hardware** — the EE just feeds it. DualShock input works (libpad), and
> **sound effects play** through the PS2 `audsrv` module. The last piece is
> **music** (Duke's MIDI tracks need an OPL synth).

```mermaid
graph LR
    GAME["Duke game<br/>src/"] --> ENGINE["BUILD engine<br/>jfbuild/ (8-bit software)"]
    ENGINE --> SEAM["sdlayer2.c<br/>baselayer seam"]
    SEAM --> GSPRES["ps2_gs.c<br/>8-bit frame → GS T8 + CLUT"]
    GSPRES --> GS["Graphics Synthesizer<br/>hardware palette-expand + upscale"]
    GAME -->|"Bopen"| FS["ps2_fileio.c<br/>cdfs / GRP"]
    SEAM -->|"handleevents"| PAD["ps2_pad.c<br/>DualShock (libpad)"]
    GAME -.->|"next"| AUD["jfaudiolib → audsrv"]
```

## Build

The toolchain runs in Docker. With the image built (`ps2uke-dock:local`):

```sh
./build.sh                 # make in ps2/  →  ps2/ps2uke.elf
./make_iso.sh [grp-dir]    # stage SYSTEM.CNF + ELF + DUKE3D.GRP + duke3d.cfg
                           #   →  dist/ps2uke.iso
```

Then boot `dist/ps2uke.iso` in PCSX2 or on hardware.

> **Game data is never committed.** `DUKE3D.GRP` (and demos/maps) stay
> copyrighted to 3D Realms; supply your own at ISO-build time. The Atomic 1.5 GRP
> is what's tested.

## Docs

- **[PORTING.md](PORTING.md)** — the architecture map: base choice, the PS2 seam, roadmap.
- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** — full subsystem deep-dive with Mermaid diagrams (boot, video, input, filesystem, audio, build, EE↔IOP).
- **[progress.md](progress.md)** — dated progress log (incl. the icculus → Chocolate → JFDuke3D history).

## Why JFDuke3D

Earlier attempts on the icculus source (no engine / no `main()`) and Chocolate
Duke3D (dead-ended on a VESA-BIOS palette path that doesn't exist on a PS2)
failed. JFDuke3D's **baselayer** lets the engine own the palette and asks the
platform only to present a finished 8-bit frame — a handoff a console can actually
implement. See [PORTING.md](PORTING.md#why-jfduke3d-and-not-chocolate--icculus).

## License

Duke Nukem 3D source is GPLv2 (3D Realms, 2003); JFDuke3D and its components carry
their respective upstream licenses. The PS2 glue in `ps2/` follows the same terms
as the code it links against.
