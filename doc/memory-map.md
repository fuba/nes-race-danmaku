# EDGERACE Memory Map for TAS/Automation

This document provides memory addresses for tool-assisted speedruns and automation.

## RAM Layout Overview

| Region | Start | End | Size | Description |
|--------|-------|-----|------|-------------|
| Zero Page | $0002 | $001B | 26 bytes | cc65 runtime variables |
| OAM Buffer | $0200 | $02FF | 256 bytes | Sprite data (64 sprites × 4 bytes) |
| DATA | $0300 | $0324 | 37 bytes | Initialized data |
| BSS | $0325 | $04B3 | 399 bytes | Game variables |
| SRAM | $6000 | $6016 | 23 bytes | Battery-backed save data |

## Game State Variables ($0325-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $0325 | 1 | game_state | 0=Title, 2=Game, 4=GameOver, 6=Win, 8=Finish |
| $0326 | 1 | frame_count | Frame counter (0-255, wraps) |
| $0327 | 1 | scroll_y | Background scroll position |

## Input ($0328-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $0328 | 1 | pad_now | Current button state |
| $0329 | 1 | pad_old | Previous frame button state |
| $032A | 1 | pad_new | Newly pressed buttons this frame |

Button bits: A=$80, B=$40, Select=$20, Start=$10, Up=$08, Down=$04, Left=$02, Right=$01

## Player Variables ($032B-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $032B | 1 | player_x | Player X position (56-184 on road) |
| $032C | 1 | player_y | Player Y position (typically 176) |
| $032D | 1 | player_hp | Player HP (0-3, game over at 0) |
| $032E | 1 | player_inv | Invincibility frames remaining |

## Enemy Variables ($032F-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $032F | 3 | enemy_x[3] | Enemy X positions |
| $0332 | 3 | enemy_y[3] | Enemy Y positions |
| $0335 | 3 | enemy_on[3] | Enemy active flags (0/1) |
| $0338 | 3 | enemy_passed[3] | Enemy overtaken flags |
| $033B | 3 | enemy_rank[3] | Enemy rank (1-11, 1-3=boss) |
| $033E | 3 | enemy_hp[3] | Enemy HP (2=full, 0=destroyed) |
| $0341 | 3 | enemy_destroyed[3] | Enemy destroyed flags |
| $0344 | 1 | enemy_next_x | Next enemy spawn X |
| $0345 | 1 | enemy_warn_timer | Warning countdown (0=spawn) |
| $0346 | 1 | enemy_slot | Next enemy slot to use |
| $0347 | 1 | enemy_next_rank | Next rank to assign |

## Race Progress ($034B-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $034B | 1 | position | Current race position (1-12) |
| $034C | 1 | lap_count | Current lap (0-2, win at 3) |
| $034D | 1 | loop_count | Current loop (0=Loop1, 1=Loop2...) |
| $034E | 2 | score | Score low 16 bits (little-endian) |
| $0350 | 2 | score_high | Score high 16 bits |
| $0352 | 2 | distance | Distance traveled in lap |
| $0354 | 2 | score_multiplier | Current multiplier (1-65535) |
| $0356 | 1 | graze_count | Bullet grazes (HP restore at 20) |
| $0357 | 1 | car_graze_cooldown | Car graze cooldown timer |
| $0358 | 1 | boost_remaining | Boosts remaining (max 2) |
| $0359 | 1 | boost_active | Currently boosting flag |
| $035A | 1 | boss_music_active | Boss BGM playing flag |

## Bullet System ($035B-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $035B | 48 | bullet_x[48] | Bullet X positions |
| $038B | 48 | bullet_y[48] | Bullet Y positions |
| $03BB | 48 | bullet_dx[48] | Bullet X velocities (signed) |
| $03EB | 48 | bullet_dy[48] | Bullet Y velocities (signed) |
| $041B | 48 | bullet_on[48] | Bullet active flags |
| $044B | 48 | bullet_grazed[48] | Bullet grazed flags |
| $047B | 1 | bullet_timer | Bullet spawn timer |
| $047C | 1 | bullet_next | Next bullet slot (circular) |
| $047D | 1 | burst_phase | Burst pattern phase (0-79) |

## Other Variables

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $047E | 1 | rnd_seed | Random number seed |
| $047F | 1 | win_timer | Win animation timer |
| $0480 | 1 | loop_clear_timer | Loop clear celebration timer |
| $0481 | 8 | confetti_x[8] | Confetti X positions |
| $0489 | 8 | confetti_y[8] | Confetti Y positions |
| $0491 | 8 | confetti_color[8] | Confetti colors |

## Name Entry ($0499-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $0499 | 1 | name_entry_pos | Current letter position (0-2) |
| $049A | 1 | name_entry_char | Current character (0-25 = A-Z) |
| $049B | 3 | entry_name[3] | Name being entered |
| $049E | 1 | new_score_rank | Achieved rank (0-2) |
| $049F | 1 | title_select_loop | Selected starting loop |

## Music/SFX ($04A0-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $04A0 | 1 | music_enabled | Music enabled flag |
| $04A1 | 1 | music_frame | Music frame counter |
| $04A2 | 1 | music_pos | Music sequence position |
| $04A3 | 1 | music_tempo | Music tempo |
| $04A4 | 1 | current_track | Current track number |
| $04A8 | 1 | sfx_graze_timer | Graze SFX timer |
| $04A9 | 1 | sfx_damage_timer | Damage SFX timer |
| $04AC | 1 | sfx_lowhp_timer | Low HP warning timer |
| $04AD | 1 | sfx_bump_timer | Bump SFX timer |

## Battery-Backed SRAM ($6000-)

| Address | Size | Variable | Description |
|---------|------|----------|-------------|
| $6000 | 1 | save_magic | Validation byte (0x52 = valid) |
| $6001 | 6 | high_scores[3] | High scores low word (2 bytes each) |
| $6007 | 6 | high_scores_high[3] | High scores high word |
| $600D | 9 | high_names[3][3] | 3-letter names (3 bytes each) |
| $6016 | 1 | max_loop | Maximum loop completed |

## OAM Sprite Buffer ($0200-)

Each sprite uses 4 bytes:
- +0: Y position (0-239, 0xEF hides sprite)
- +1: Tile index
- +2: Attributes (bits: VHP---CC, V=vflip, H=hflip, P=priority, C=palette)
- +3: X position

| Sprite Range | Purpose |
|--------------|---------|
| $0200-$020F | Player car (4 sprites) |
| $0210-$021F | Enemy car 0 (4 sprites) |
| $0220-$022F | Enemy car 1 (4 sprites) |
| $0230-$023F | Enemy car 2 (4 sprites) |
| $0240-$02BF | Bullets (up to 32 visible) |
| $02C0-$02FF | HUD elements |

## Key Values for TAS

### Win Conditions
- Position = 1 AND lap_count = 3 → Advance to next loop
- HP = 0 → Game Over

### Scoring Formula
- Bullet graze: `score += multiplier × (1 << loop_count)`
- Overtake: `score += 20 × multiplier`
- Enemy car destroy: `multiplier *= 2`
- Loop clear bonus: `1000 × loop_count × (1 << loop_count)`

### Movement Bounds
- Road left: X = 56
- Road right: X = 184
- Player Y (fixed): 176

### Useful Memory Watches
```
$032D - Player HP (game over when 0)
$034B - Position (1 = first place)
$034C - Lap count (3 = win)
$034D - Loop count
$034E/$0350 - Score (32-bit)
$0354 - Multiplier
```
