# NES Racing Game

A NASCAR-style racing game for the Nintendo Entertainment System (NES/Famicom).

## Game Features

- **1v1 Racing**: Overtake enemy cars to take the lead
- **Obstacles**: Avoid debris on the track
- **Pit Stops**: Pull to the left edge to repair damage
- **Lap System**: Complete 3 laps in 1st place to win
- **Score System**: Points for overtaking and lap completion

## Controls

| Button | Action |
|--------|--------|
| D-Pad  | Move car |
| Start  | Start game / Return to title |

## Technical Specifications

- **Mapper**: NROM-128 (Mapper 0)
- **PRG-ROM**: 16KB
- **CHR-ROM**: 8KB
- **Mirroring**: Horizontal (for vertical scrolling)
- **RAM**: 2KB internal

## Building

### Requirements

- Docker and Docker Compose

### Build Command

```bash
docker compose run --rm nes-dev make
```

Output: `build/race.nes`

### Clean Build

```bash
docker compose run --rm nes-dev make clean
docker compose run --rm nes-dev make
```

## Testing

Test with any NES emulator:
- **FCEUX** (recommended for debugging)
- **Mesen** (high accuracy)
- **Nestopia**
- **Bizhawk**

For hardware: Use flash cartridges like Everdrive N8 or PowerPak.

## Project Structure

```
race/
├── Dockerfile          # Development environment
├── docker-compose.yml
├── Makefile
├── src/
│   ├── main.c         # Game logic
│   ├── crt0.s         # Startup code
│   └── nrom.cfg       # Linker configuration
├── tools/
│   └── generate_chr.py # Graphics generator
├── build/             # Output directory
│   └── race.nes       # Final ROM
└── doc/
    └── README.md
```

## License

CC0 (Creative Commons Zero) - Public Domain
