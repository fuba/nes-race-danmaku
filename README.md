# NES Racing Danmaku

A NASCAR-style racing game with bullet hell (danmaku) elements for the Nintendo Entertainment System (NES/Famicom).

## Play Online

**[Play Now on GitHub Pages](https://fuba.github.io/nes-race-danmaku/)**

Works in browser with virtual controller support for mobile devices.

## Game Rules

### Objective
- Start in **12th place** (last of 12 cars)
- **Overtake enemies** by moving above them on screen
- Complete **3 laps in 1st place** to win

### Position System
- Each enemy you overtake improves your position by 1
- Overtake 11 cars to reach 1st place
- If you finish 3 laps but not in 1st place: Game Over

### Health System
- Start with **5 HP**
- Lose HP when hit by:
  - Enemy bullets (danmaku patterns)
  - Track obstacles
  - Enemy car collision
- **Recover 3 HP** on each lap completion
- HP reaches 0: Game Over

### Danmaku Patterns
Enemy cars fire 8 different bullet patterns:
1. Spread shot
2. Aimed shot
3. Spiral
4. Ring burst
5. Random scatter
6. Stream
7. Cross pattern
8. Wave

## Controls

| Button | Action |
|--------|--------|
| D-Pad Up/Down | Move car vertically (overtake!) |
| D-Pad Left/Right | Move car horizontally |
| Start | Start game / Pause |

### Keyboard (Browser)
- Arrow keys: D-Pad
- Z: A button
- X: B button
- Enter: Start
- Shift: Select

## Features

- **12-car race** with position tracking
- **8 danmaku bullet patterns**
- **Music**: Title BGM, Racing BGM, Victory fanfare, Game Over theme
- **Triangle wave bass** and pulse wave melodies
- **Progress bar** showing lap progress
- **Victory celebration** with confetti animation

## Technical Specifications

- **Mapper**: NROM-128 (Mapper 0)
- **PRG-ROM**: 16KB
- **CHR-ROM**: 8KB
- **Mirroring**: Horizontal (for vertical scrolling)
- **RAM**: 2KB internal

## Building

### Requirements

- cc65 (6502 C compiler)
- Python 3 (for CHR generation)
- Make

### Using Docker

```bash
docker run --rm -v "$PWD:/app" -w /app ubuntu:22.04 bash -c \
  "apt-get update && apt-get install -y cc65 python3 make && make"
```

Output: `build/race.nes`

### Local Build (if cc65 installed)

```bash
python3 tools/generate_chr.py build/tiles.chr
make
```

## Testing

Test with any NES emulator:
- **FCEUX** (recommended for debugging)
- **Mesen** (high accuracy)
- **Nestopia**
- **Bizhawk**

Or play in browser via GitHub Pages link above.

For hardware: Use flash cartridges like Everdrive N8 or PowerPak.

## Project Structure

```
race/
├── .github/workflows/  # CI/CD (auto-build & deploy)
├── src/
│   ├── main.c          # Game logic + music engine
│   ├── crt0.s          # Startup code
│   └── nrom.cfg        # Linker configuration
├── tools/
│   └── generate_chr.py # Graphics generator
├── web/
│   ├── index.html      # Browser player (JSNES)
│   └── race.nes        # ROM for web
├── build/
│   └── race.nes        # Built ROM
└── doc/
    └── README.md
```

## License

CC0 (Creative Commons Zero) - Public Domain
