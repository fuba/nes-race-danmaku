# EDGERACE
<img width="512" height="480" alt="edgerace-2026-02-03T14-12-38" src="https://github.com/user-attachments/assets/d6f6651d-8a91-4ff1-8a0f-a32434c0542a" />

## 究極の弾幕レーシング、ここに始動！

**EDGERACEは「グレイズ」で勝つ！** 敵弾をギリギリでかわすほどスコア倍率が上昇、敵車とすれすれのグレイズで倍率はさらに2倍！

**追い越しが得点だ！** ミスなく走り抜ければ、天文学的スコアが君を待つ！

1位でゴールして次のステージへ突入！夕方から夜へ…光が落ちるほど難度は上昇。

**踏み込め、限界のエッジへ！**

---

## THE ULTIMATE BULLET-HELL RACE IGNITES!

**EDGERACE is won by GRAZE!** Slip through enemy fire at the edge and your score multiplier climbs—graze enemy cars for DOUBLE the boost!

**OVERTAKE TO SCORE!** Keep the line clean and the numbers go ASTRONOMICAL!

Finish in 1st to advance—DUSK TO NIGHT, SPEED TO FEAR, DIFFICULTY RISING!

**PUSH TO THE EDGE. RULE THE RACE!**

---

## Play Online

**[Play Now on GitHub Pages](https://fuba.github.io/nes-race-danmaku/)**

Works in browser with virtual controller support for mobile devices.

## Controls / 操作方法

| Button | Action |
|--------|--------|
| D-Pad / 十字キー | Move / 移動 |
| START | Start / Pause / ゲーム開始・ポーズ |

### Keyboard (Browser)
- Arrow keys / WASD: D-Pad
- Z / J: A button
- X / K: B button
- Enter: Start
- Shift: Select

## Game Rules

### Objective
- Start in **12th place** (last of 12 cars)
- **Overtake enemies** by moving above them on screen
- Complete **3 laps in 1st place** to advance to next stage

### Graze System
- **Graze enemy bullets**: Score multiplier UP!
- **Graze enemy cars**: Multiplier x2!
- Chain grazes for ASTRONOMICAL scores!

### Position System
- Each enemy you overtake improves your position by 1
- Overtake 11 cars to reach 1st place
- If you finish 3 laps but not in 1st place: Game Over

### Health System
- Start with **5 HP**
- Lose HP when hit by enemy bullets or cars
- **Recover HP** by grazing
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

## Features

- **12-car race** with position tracking
- **8 danmaku bullet patterns**
- **Graze scoring system** for massive multipliers
- **Day/Evening/Night** stage progression
- **Music**: Title BGM, Racing BGM (3 variations), Boss BGM, Victory fanfare
- **High score save** with battery backup

## Technical Specifications

- **Platform**: Nintendo Entertainment System (NES/Famicom)
- **Mapper**: NROM-128 (Mapper 0)
- **PRG-ROM**: 16KB
- **CHR-ROM**: 8KB
- **Mirroring**: Horizontal (for vertical scrolling)

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
- **FCEUX** (recommended for debugging)
- **Mesen** (high accuracy)
- **Nestopia**

Or play in browser via GitHub Pages link above.

## License

CC0 (Creative Commons Zero) - Public Domain

---
