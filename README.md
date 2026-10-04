# Tempest 2000 (T2K mode) for the Sega Master System

A fan port of the Tempest 2000 game mode to the Sega Master System / Mark III,
built from the platform-independent C core of
[haroldo-ok/tempest-2k-32x](https://github.com/haroldo-ok/tempest-2k-32x).

The SMS can't draw vector graphics, scale sprites or rotate anything in real time,
so this port pre-renders all of that offline and streams it from a 4 MB ROM:

* **Pre-rendered FMV** for the title-screen rotation, the warp-in that starts every
  level, and the zoom-out dive that ends it.
* **Pre-rendered, pre-scaled, pre-rotated sprites** for every enemy, powerup and the claw.
* **Pre-computed projection tables** in place of runtime 3D maths.

| | |
|---|---|
| ROM | `rom/t2k_sms.sms` (built), 4 MiB, standard Sega mapper |
| Banks | 256 × 16 KiB: 2 code banks + 181 data banks + padding |
| Code | ~20.7 KB of the 32 KB fixed area |
| Video | 34 web shapes × (warp-in + zoom-out) + looping title clip |
| Sprites | 845 pre-rendered images (5 sizes × 16 orientations / frames) |
| Audio | Original PSG music (6 tracks) and 14 sound effects |

---

## Credits and legal

* **Tempest** (1981) was created by Dave Theurer for Atari. **Tempest 2000** (1994)
  is by Jeff Minter / Llamasoft for the Atari Jaguar. This is an unofficial,
  non-commercial fan port; no ownership of the game or its trademarks is claimed.
* Game rules are ported from haroldo-ok's 32X reimplementation (`src/core/game.c`).
* Web geometry comes from that project's `webdata.h` (included here as
  `third_party/webdata.h`), which in turn was extracted from mwenge's
  reconstructed Tempest 2000 source.
* **Original to this port:** the sprite designs, the HUD font, the title text,
  all music and all sound effects. The Jaguar soundtrack, speech samples, logo
  and bitmap art are *not* used.
* Built with SDCC and devkitSMS (SMSlib / PSGlib) by sverx.

---

## Playing

The ROM needs an emulator or flash cart that supports 4 MiB Sega-mapper ROMs
(for example Emulicious, or an Everdrive-class cartridge). Emulators limited to
1 MiB ROMs will not load it.

### Controls

| Input | Title / menus | In game |
|---|---|---|
| D-pad Left / Right | Choose start level | Move the claw around the web |
| D-pad Up | – | Jump (after collecting the Jump powerup) |
| Button 1 | Start / confirm | Fire (hold for autofire) |
| Button 2 | Back to title | Superzapper |
| Pause | – | Pause / resume |

### Flow

Title → level select (odd levels up to the best you have reached this session)
→ warp-in → play → clear the web → zoom-out dive (you can still steer and shoot
spikes; hitting one costs a life) → next level's warp-in.

### What's implemented

* All 48 web entries (34 distinct shapes), cycling for levels 49–99.
* Enemies: flippers, mutant flippers, tankers (flipper / pulsar / fuseball
  payloads), spikers and their spikes, fuseballs, pulsars, demonheads, mirrors.
* Superzapper (full clear first, single kill second), jumping, AI droid.
* Powerups: particle laser, jump, AI droid, Zappo 2000, Outta Here, warp tokens.
* Highlighted player lane, extra lives every 100,000 points, pause, game over.

---

## Building

### Requirements

* SDCC 4.2+ and devkitSMS (SMSlib, crt0, ihx2sms). The devkitSMS skill's
  `scripts/setup_toolchain.sh` installs everything into `~/.devkitsms`.
* Python 3 with NumPy and Pillow (for the offline data pipeline).
* `smstest` headless emulator (installed by the same setup script) for tests.

### Steps

```sh
export PATH=$HOME/.devkitsms/bin:$PATH

# 1. Offline data: webs, FMV clips, sprites, tables -> gen/  (about a minute)
cd tools && python3 build.py && cd ..

# 2. Music and effects -> src/music_data.c, src/sfx_data.c
#    (generated files are already included; only needed after editing the songs)
cd music && python3 compose.py && python3 sfx.py && cd ..

# 3. Code + data -> rom/t2k_sms.sms
make
```

`make` compiles the code into the 32 KB fixed area, converts it with `ihx2sms`,
then `tools/mkrom.py` appends `gen/blob.bin` as banks 2 onward, pads to 4 MiB and
fixes the SEGA header checksum.

`tools/build.py --quick` builds only the first three webs (useful while iterating);
`--out DIR` writes somewhere other than `gen/`.

---

## Project layout

```
src/            Z80 C code
  main.c        state machine, HUD, sprite streaming, lit lane, palettes
  game.c/.h     game rules (ported from the 32X core, 8.8 fixed point)
  fmv.c/.h      FMV player (assembly upload loop)
  vdp.c/.h      paced VRAM writers (assembly)
  sound.c/.h    VBlank-interrupt music sequencer + effect player
  music_data.*  generated song tables
  sfx_data.*    generated effect tables
tools/          offline data pipeline (Python)
  webs.py       parse third_party/webdata.h
  geom.py       per-web centring, focal length, projection
  raster.py     line rasteriser
  render_web.py wireframe web renderer (with lit-lane variant)
  tiles.py      8×8 cutting, flip-aware de-duplication
  fmvenc.py     VRAM-modelling FMV encoder
  sprites.py    sprite designs and pre-rendering
  webtables.py  per-web projection / flip-path tables
  font.py       HUD font
  blob.py       bank packer
  build.py      builds everything into gen/
  build_frames.py  frame generators shared with verify.py
  verify.py     decodes every FMV clip from the ROM and checks it pixel-exact
  mkrom.py      splices code + data, pads, fixes checksum
music/
  songs.py      the songs, in a small text tracker notation
  compose.py    songs.py -> src/music_data.c/.h
  sfx.py        effect designs -> src/sfx_data.c/.h
  psg2wav.py    render a smstest PSG log to WAV
tests/          smstest scripts
third_party/
  webdata.h     web geometry from tempest-2k-32x
```

---

## How it works

### Memory map

**ROM**

| Banks | Contents |
|---|---|
| 0–1 | Code and fixed data (music, effect tables, index tables) |
| 2 | Font, static sprite tiles |
| 3–9 | 845 sprite images (128 per bank) |
| 10–26 | Per-web tables, two webs per bank |
| 27–121 | Lit-lane overlays, canonical layouts, warp-in and zoom-out streams |
| 122–125 | Title clip (intro + loop) |
| 126–182 | Shared 2bpp tile dictionary (1024 tiles per bank) |
| 183–255 | Padding |

All data is read through mapper slot 2 (`0x8000–0xBFFF`). Every routine that maps
another bank restores the previous one, tracked in `cur_bank`.

**VRAM**

| Tiles | Use |
|---|---|
| 0 | Blank |
| 1–40 | HUD font |
| 41–191 | Gameplay web (canonical layout, ≤151 tiles) |
| 192–255 | Lit-lane overlay pools (2 × 32) |
| 256–431 | Sprites: shots/explosions + 36 enemy slots × 4 tiles |
| 432–447 | FMV-safe sprites: claw, player shots, spike tip/dot, droid |
| 41–431 | During FMV playback, the whole range is used by the video |
| `0x3800` | Name table; `0x3F00` sprite attribute table |

Row 0 of the screen is the HUD; the video and playfield use rows 1–23.

### Pre-rendered FMV

1. **Rendering.** Each frame of each clip is drawn offline as a 2-bit wireframe
   (dim far ring, mid edges, bright rim), 256×192.
2. **Tiling.** Frames are cut into 8×8 tiles; each tile is canonicalised under
   horizontal/vertical flip and looked up in one global dictionary shared by every
   clip (57,424 tiles, stored 2bpp at 16 bytes each).
3. **Encoding.** `fmvenc.py` tracks exactly what is in every VRAM tile slot and
   name-table cell. For each frame it emits a *step*: tiles to upload (only into
   slots not currently on screen) plus the name-table cells that change.
4. **Convergence.** A warp-in ends with invisible "converge" steps that move the
   final picture into the web's canonical slot layout without changing what is on
   screen. Gameplay, the lit lane and the zoom-out all rely on that layout.
   The title loop uses the same trick to return to its own starting state.
5. **Playback.** `upload_loop` (Z80 assembly) expands 2bpp dictionary tiles to
   4bpp while uploading, with ≥28 T-states between VRAM writes so it is safe with
   the display on. Uploads run at the end of each frame up to a beam-line limit;
   the name-table writes for the next frame happen right after VBlank. Playback
   runs at up to 30 fps (15–20 fps for the busiest frames).

**Step format** (`flags, hold, aux, u16 nup`, then `nup × {slot lo, slot hi, tile id lo, tile id hi}`,
then `u16 nruns`, then `nruns × {u16 cell, u8 len, len × u16 name-table entry}`;
a `0xFF` byte means "continue at the start of the next bank"). `flags` bit 0 marks
a visible frame and bit 1 the last step; `aux` carries the camera depth for the
zoom-out so the interactive part stays in sync with the picture.

### Sprites

* Each image is 16×16 (two 8×16 hardware sprites), or 8×16 for the smallest size.
* Sizes are chosen per lane and depth from the lane's on-screen width; orientations
  come from the lane's direction into the web, quantised to 16 steps.
* Each enemy owns a 4-tile VRAM slot. When the image it needs changes (closer,
  different lane, animation frame), it's queued and up to two images are uploaded
  per frame; until then the enemy is drawn with its previous image of the same kind.
* Flipper flips use pre-computed pivot paths (8 steps per lane and direction) that
  sweep outward over the rim.
* Draw order rotates every frame so the 8-sprites-per-line limit flickers instead
  of hiding the same object.

### Per-web tables

Each web gets a 5.6 KB table: lane count and start lane, lane orientation, claw
positions along each lane's rim (8 steps), flipper flip paths, and the screen
position and size of every lane centre at 81 depths (z = −4 … 16 in quarter units).
The negative depths cover the claw while jumping and objects during the dive.

### Lit lane

For every web and lane, the builder diffs the lit image against the plain one and
stores the changed cells (≤25) as 3bpp tiles plus each cell's normal name-table
entry. When the player changes lane, the new overlay is uploaded into the inactive
32-tile pool over a few frames, then the old cells are restored and the new ones
pointed at the pool in a single VBlank.

### Game rules

`game.c` follows the 32X core's rules, converted to 8.8 fixed point (web depth =
4096) and byte-sized counters. Enemy speeds, spawn pools, flipper behaviour,
spike growth, fuseball lane hopping, pulsar pulses, superzapper and powerup logic
all match the source. The score is also kept as six decimal digits so the HUD
never needs 32-bit division.

### Music and sound

All music and effects were composed and designed for this port.

* **Driver.** `sound.c` runs from the VBlank interrupt (about 3,200 cycles per
  frame), so tempo doesn't depend on the game's frame rate. Game code only posts
  requests; all sound-chip writes happen in the interrupt, so there are no latch
  races. Writes are skipped when a value hasn't changed.
* **Channels.** Music uses channel 0 for bass, channel 1 for the lead (with delayed
  vibrato), channel 2 for a chord arpeggio, and the noise channel for drums.
  Effects have a tone slot on channel 2 and a noise slot, each with priorities;
  they borrow the arpeggio or drums only while they play.
* **Songs** (`music/songs.py`):

  | Song | Use | Key / speed |
  |---|---|---|
  | Vector Dawn | Title and level select | A minor, ~112 BPM |
  | Warp fanfare | Each warp-in | A minor, fast |
  | Lane Runner | Levels 1–16, 49–64, … | D minor |
  | Grid Pulse | Levels 17–32, 65–80, … | E minor |
  | Spike Field | Levels 33–48, 81–96, … | G minor |
  | Game over sting | Game over | A minor, slow |

  Notation: 16 rows per bar; tokens are notes (`A4`, `C#5`), `.` hold, `-` off,
  chord arpeggios (`A4m`, `F4M`) on the pad channel, and `K S h o`
  (kick, snare, hat, open hat) on the drum channel.
* **Effects** (`music/sfx.py`): shot, particle laser, explosion, tanker split,
  flipper flip, player death, superzapper, powerup, extra life, level clear,
  spike hit, jump, menu and warp.

---

## Testing

```sh
python3 tools/verify.py                       # FMV: decode every clip from the ROM, compare to ideal frames
smstest rom/t2k_sms.sms tests/play2.txt       # any of the scripts below
```

| Script | Checks |
|---|---|
| `smoke.txt` | Boots, display on, title renders |
| `play1.txt` | Title → select → warp-in → play, firing |
| `play2.txt` | Superzapper, level clear → zoom-out → level 2 warp-in |
| `play3.txt` | Three deaths → game over → back to title |
| `alllevels.txt` | Warp-in and zoom-out for every level 2–49 (screenshot of each) |
| `enemies.txt` | Level 33 enemy mix |
| `hw.txt` | Pause, spike death during the dive, VRAM pacing, stack low-water mark |
| `audio.txt`, `audio2.txt` | PSG captures of title, level tunes, game over |
| `fps.txt`, `rate.txt`, `prof.txt` | Gameplay frame rate, FMV frame rate, profiler |

Some scripts poke raw RAM addresses (the enemy pools and spike array). If the
code changes, re-read them from `build/code.noi`.

**Results at the time of packaging**

* All 69 FMV clips (34 warp-ins, 34 zoom-outs, the title loop played twice)
  decode pixel-exact from the ROM, and every warp-in ends on the canonical layout.
* All playtest scripts pass.
* 0 unpaced VRAM writes during active display out of ~312,000 checked.
* Gameplay runs at about 54–60 fps with a full screen of enemies; FMV at up to 30 fps.

---

## Known limitations

* Not yet tested on real hardware or in Emulicious (only the headless `smstest`).
* Not implemented: the UFO, the bonus round (three warp tokens award 10,000 points
  instead), camera panning, particle explosions (sprite explosions instead) and
  high-score / progress saving.
* The web is wireframe (no filled lanes).
* With many enemies on one scanline, sprites flicker (8-per-line hardware limit).
* The highlighted lane uses yellow, so it is hard to see on the yellow-palette webs.
