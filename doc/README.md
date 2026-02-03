# EDGERACE - Technical Documentation

For gameplay instructions and features, see the main [README.md](../README.md).

## Architecture Overview

### Main Components

- `src/main.c` - All game logic, rendering, and music in a single file
- `src/crt0.s` - NES startup code and interrupt handlers
- `src/nrom.cfg` - Linker configuration for NROM mapper
- `tools/generate_chr.py` - Graphics tile generator

### Memory Map

```
$0000-$07FF: Internal RAM (2KB)
$6000-$7FFF: Battery-backed SRAM (8KB, used for high scores)
$8000-$BFFF: PRG-ROM (16KB)
$C000-$FFFF: PRG-ROM mirror
```

### SRAM Layout ($6000-$7FFF)

```
$6000: save_magic (validation marker)
$6002-$6007: high_scores[3] (16-bit each, low word)
$6008-$600D: high_scores_high[3] (16-bit each, high word)
$600E-$6016: high_names[3][3] (3-letter names)
$6017: max_loop (highest loop completed)
$6018: title_select_loop (selected starting loop)
```

### Sprite System

- 64 sprites maximum (NES hardware limit)
- Player car: 4 sprites (16x16)
- Enemy cars: 4 sprites each (16x16, max 3 enemies = 12 sprites)
- Bullets: 1 sprite each (max 48 bullets)
- HUD elements use remaining sprites

### Music Engine

Simple sequencer using NES APU:
- Triangle channel: Bass line
- Pulse 1: Melody
- Pulse 2: Harmony / SFX
- Noise: Percussion / SFX

Tracks:
- Title BGM, Racing BGM (3 variations), Boss BGM (3 variations)
- Victory fanfare, Game Over theme

### Build Process

1. `generate_chr.py` creates tile graphics (8KB CHR-ROM)
2. `cc65` compiles C to 6502 assembly
3. `ca65` assembles startup code and compiled output
4. `ld65` links everything into PRG-ROM binary
5. CHR-ROM appended to create final .nes file

### Version History

- V5.0: Title screen improvements, graze exploit fix
- V5.1: High score ranking order fix
- V5.2: Enemy car graze destruction system
- V5.3: Improved enemy car mechanics (tighter hitbox, visible 1st place retreat)
