# EDGERACE
<img width="512" height="480" alt="edgerace-2026-02-03T14-12-38" src="https://github.com/user-attachments/assets/d6f6651d-8a91-4ff1-8a0f-a32434c0542a" />

## 究極の弾幕レーシング、ここに始動！

**EDGERACEは「グレイズ」で勝つ！** 敵弾をギリギリでかわすほどスコア倍率が上昇、敵車とすれすれのグレイズで倍率はさらに2倍！

**追い越しが得点だ！** ミスなく走り抜ければ、天文学的スコアが君を待つ！

1位でゴールして次のループへ突入！昼から夕、そして夜へ…難度は上昇し続ける。どこまで走り続けられるか？

**踏み込め、限界のエッジへ！**

---

## THE ULTIMATE BULLET-HELL RACE IGNITES!

**EDGERACE is won by GRAZE!** Slip through enemy fire at the edge and your score multiplier climbs—graze enemy cars for DOUBLE the boost!

**OVERTAKE TO SCORE!** Keep the line clean and the numbers go ASTRONOMICAL!

Finish in 1st to advance to the next loop—DAY TO DUSK TO NIGHT, SPEED INCREASES, DIFFICULTY RISES ENDLESSLY!

**PUSH TO THE EDGE. RULE THE RACE!**

---

## Play Online

**[Play Now on GitHub Pages](https://fuba.github.io/nes-race-danmaku/)**

Works in browser with virtual controller support for mobile devices.

## Controls / 操作方法

| Button | Action |
|--------|--------|
| D-Pad / 十字キー | Move / 移動 |
| B button / Bボタン | Accelerate / 加速 |
| START | Start / Pause / ゲーム開始・ポーズ |
| A button / Aボタン | Confirm (name entry) / 決定（名前入力） |

### Keyboard (Browser)
- Arrow keys / WASD: D-Pad
- Z / J: A button
- X / K: B button (hold for acceleration)
- Enter: Start
- Shift: Select

### Title Screen
- UP/DOWN: Select starting loop (unlocked after completing loops)
- START: Begin race

## Game Rules

### Objective
- Start in **12th place** (last of 12 cars)
- **Overtake enemies** by moving above them on screen
- Complete **3 laps in 1st place** to advance to the next loop
- Endless loop progression: Loop 1 → Loop 2 → Loop 3 → ...

### Graze System
- **Graze enemy bullets**: Score multiplier +1!
- **Graze enemy cars**: Multiplier x2!
- **HP Recovery**: Every 20 bullet grazes restores +1 HP
- Chain grazes for ASTRONOMICAL scores!
- Score per graze = multiplier × 2^(loop number)

### Position System
- Each enemy you overtake improves your position by 1
- Overtake 11 cars to reach 1st place
- If you finish 3 laps but not in 1st place: Game Over (position displayed)

### Health System
- Start with **5 HP** (max 100 HP)
- Lose HP when hit by enemy bullets or cars
- **Recover HP** by grazing bullets (20 grazes = +1 HP)
- HP reaches 0: Game Over (explosion animation)

### Enemy Types

**Normal Enemies (Rank 4-11)**
- Standard danmaku patterns
- Aimed shots toward player
- Firing rate increases in later loops

**Boss Enemies (Rank 1-3)**
- Distinctive red car design
- 4 danmaku attack patterns:
  1. **Aimed Spiral** - Rotating shots around player direction
  2. **Aimed Spread** - 3-way spread toward player
  3. **Aimed Burst** - Multiple simultaneous shots
  4. **Aimed Wave** - Oscillating bullet stream
- Triggers intense Boss BGM when approaching
- Only one boss spawns at a time

### 1st Place Challenge
- When in 1st place, danmaku comes from **behind** (bottom of screen)
- Sweeping wave patterns to test your skills
- Survive 3 laps to complete the loop!

## Stage Progression

| Loop | Time of Day | Grass Color | Difficulty |
|------|-------------|-------------|------------|
| Loop 1 | Day | Green | Normal |
| Loop 2 | Evening | Orange | Hard |
| Loop 3+ | Night | Blue | Expert |

- Palette changes every loop (Day → Evening → Night → Evening → Night...)
- Bullet speed increases every 3 loops
- Enemy firing rate increases with loop count
- Music intensity increases within each loop (LAP 1 → LAP 2 → LAP 3)

## Features

- **12-car race** with position tracking
- **Boss danmaku patterns** with 4 attack types
- **Graze scoring system** for massive multipliers (up to 65535x!)
- **Day/Evening/Night** visual progression
- **B button acceleration** for speed control
- **Loop selection** from title screen (unlocked after completion)
- **High score save** with 3-letter name entry (battery backup)
- **Top 3 leaderboard** preserved in SRAM

### Music
- **Title BGM**: Heroic fanfare
- **Racing BGM**: 3 variations (Day/Evening/Night themes)
- **Boss BGM**: 3 intense battle themes
- **Victory fanfare**: Loop completion celebration
- **Game Over music**: Dramatic conclusion
- Dynamic intensity system (calm → moderate → intense)

## Technical Specifications

- **Platform**: Nintendo Entertainment System (NES/Famicom)
- **Mapper**: NROM-128 (Mapper 0)
- **PRG-ROM**: 16KB
- **CHR-ROM**: 8KB
- **Mirroring**: Horizontal (for vertical scrolling)
- **SRAM**: Battery-backed save for high scores
- **Max Bullets**: 48 simultaneous bullets on screen
- **Max Enemies**: 3 visible at once

## Building

### Requirements
- cc65 (6502 C compiler)
- Python 3 (for CHR generation)
- Make

### Using Docker
```bash
docker run --rm -v "$PWD:/app" -w /app ubuntu:24.04 bash -c \
  "apt-get update && apt-get install -y cc65 python3 make && make"
```

Output: `build/edgerace.nes`

## Testing

Test with any NES emulator:
- **FCEUX** (recommended for debugging, SRAM support)
- **Mesen** (high accuracy, SRAM support)
- **Nestopia**

Note: Web emulators may not persist SRAM high scores between sessions.

Or play in browser via GitHub Pages link above.

## License

CC0 (Creative Commons Zero) - Public Domain
