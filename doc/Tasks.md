# Development Tasks

## Completed

### Task 1: Docker Development Environment
- [x] Created Dockerfile with cc65 toolchain
- [x] Set up docker-compose.yml
- [x] Verified compilation tools (cc65, ca65, ld65)

### Task 2: NES Game Structure
- [x] Created linker configuration (nrom.cfg)
- [x] Implemented crt0.s startup code
- [x] Set up Makefile for build process
- [x] Configured iNES header for NROM-128

### Task 3: Graphics Assets (CHR-ROM)
- [x] Created generate_chr.py tool
- [x] Implemented tile definitions:
  - Road, grass, center line tiles
  - Player car sprite (16x16)
  - Enemy car sprite (16x16)
  - Obstacle sprite (8x8)
  - Digits 0-9
  - Letters A-Z

### Task 4: Game Logic
- [x] Title screen with "RACE" text
- [x] Player movement (D-pad controls)
- [x] Enemy car AI (simple tracking)
- [x] Obstacle spawning and collision
- [x] Pit stop system (repair HP)
- [x] Position tracking (1st/2nd place)
- [x] Lap counting and win condition
- [x] Score display
- [x] Game over screen
- [x] Win screen with score

### Task 5: Testing and Debugging
- [x] Successful ROM build (24592 bytes)
- [x] iNES header verified
- [ ] Test on emulator
- [ ] Test on real hardware (if available)

## Future Improvements

- Add sound effects
- Add music
- Multiple difficulty levels
- More obstacle types
- Better AI behavior
- Smooth scrolling background
- Title screen graphics
