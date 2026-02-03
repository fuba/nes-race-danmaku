# Development Tasks

## Version 5.3 (Current)

### Completed Features

#### Core Gameplay
- [x] 12-car race with position tracking
- [x] 3-lap races with loop progression
- [x] Endless difficulty scaling (loops)
- [x] Day/Evening/Night palette progression

#### Graze System
- [x] Bullet graze detection (+1 multiplier)
- [x] HP recovery from bullet grazes (20 grazes = +1 HP)
- [x] Per-bullet graze tracking (one graze per bullet)
- [x] Score calculation: multiplier × 2^(loop number)

#### Enemy Car Destruction (V5.2+)
- [x] Enemy cars have 2 HP
- [x] Side graze deals 1 HP damage
- [x] Destruction gives 2x multiplier bonus
- [x] Destroyed enemies slow down and scroll off
- [x] Destroyed enemies stop shooting
- [x] Bump sound effect for car collision
- [x] Tighter graze hitbox (V5.3)

#### Boss System
- [x] 3 boss enemies (rank 1-3) with unique patterns
- [x] 4 danmaku attack types per boss
- [x] Boss BGM triggers on approach
- [x] Visible retreat when overtaken (V5.3)

#### 1st Place Mode
- [x] Overtaken enemy retreats visibly while shooting
- [x] Background danmaku after enemy scrolls off
- [x] Wave patterns from bottom of screen

#### Audio
- [x] Title BGM
- [x] Racing BGM (3 variations: Day/Evening/Night)
- [x] Boss BGM (3 intensity levels)
- [x] Victory fanfare
- [x] Game Over theme
- [x] SFX: Graze, Damage, Bump, Low HP warning

#### UI/UX
- [x] Title screen with high scores
- [x] Loop selection from title
- [x] 3-letter name entry for high scores
- [x] Battery-backed SRAM save
- [x] High score validation/sorting (V5.1)
- [x] B button acceleration
- [x] Warning arrow for incoming enemies

#### Technical
- [x] 48 simultaneous bullets
- [x] 3 visible enemies maximum
- [x] LOD system for bullet updates
- [x] NROM-128 mapper (16KB PRG, 8KB CHR)
- [x] GitHub Pages web deployment
- [x] Mobile virtual controller support

## Future Ideas

- [ ] Additional boss patterns
- [ ] Power-up items
- [ ] Two-player mode
- [ ] Sound test mode
- [ ] Replay system
