// NASCAR-style Racing Game for NES
// Direct hardware access version (no external neslib)

// NES Hardware registers
#define PPU_CTRL    (*(volatile unsigned char*)0x2000)
#define PPU_MASK    (*(volatile unsigned char*)0x2001)
#define PPU_STATUS  (*(volatile unsigned char*)0x2002)
#define OAM_ADDR    (*(volatile unsigned char*)0x2003)
#define OAM_DATA    (*(volatile unsigned char*)0x2004)
#define PPU_SCROLL  (*(volatile unsigned char*)0x2005)
#define PPU_ADDR    (*(volatile unsigned char*)0x2006)
#define PPU_DATA    (*(volatile unsigned char*)0x2007)
#define OAM_DMA     (*(volatile unsigned char*)0x4014)
#define JOYPAD1     (*(volatile unsigned char*)0x4016)
#define JOYPAD2     (*(volatile unsigned char*)0x4017)
// APU (Audio Processing Unit) registers
#define APU_STATUS  (*(volatile unsigned char*)0x4015)
#define APU_FRAME   (*(volatile unsigned char*)0x4017)

// Pulse 1 (Square wave channel 1)
#define APU_PL1_VOL (*(volatile unsigned char*)0x4000)  // Duty, loop, env, volume
#define APU_PL1_SWP (*(volatile unsigned char*)0x4001)  // Sweep
#define APU_PL1_LO  (*(volatile unsigned char*)0x4002)  // Timer low
#define APU_PL1_HI  (*(volatile unsigned char*)0x4003)  // Length, timer high

// Pulse 2 (Square wave channel 2)
#define APU_PL2_VOL (*(volatile unsigned char*)0x4004)
#define APU_PL2_SWP (*(volatile unsigned char*)0x4005)
#define APU_PL2_LO  (*(volatile unsigned char*)0x4006)
#define APU_PL2_HI  (*(volatile unsigned char*)0x4007)

// Triangle wave channel
#define APU_TRI_LIN (*(volatile unsigned char*)0x4008)  // Linear counter
#define APU_TRI_LO  (*(volatile unsigned char*)0x400A)  // Timer low
#define APU_TRI_HI  (*(volatile unsigned char*)0x400B)  // Length, timer high

// Noise channel
#define APU_NOI_VOL (*(volatile unsigned char*)0x400C)  // Loop, env, volume
#define APU_NOI_LO  (*(volatile unsigned char*)0x400E)  // Mode, period
#define APU_NOI_HI  (*(volatile unsigned char*)0x400F)  // Length

// OAM buffer location
#define OAM         ((unsigned char*)0x0200)

// Controller buttons
#define BTN_A       0x80
#define BTN_B       0x40
#define BTN_SELECT  0x20
#define BTN_START   0x10
#define BTN_UP      0x08
#define BTN_DOWN    0x04
#define BTN_LEFT    0x02
#define BTN_RIGHT   0x01

// Game states
#define STATE_TITLE      0
#define STATE_RACING     1
#define STATE_GAMEOVER   2
#define STATE_WIN        3
#define STATE_PAUSED     4
#define STATE_HIGHSCORE  5  // Name entry for high score
#define STATE_LOOP_CLEAR 6  // Loop completion celebration
#define STATE_EXPLODE    7  // Player explosion (HP=0)
#define STATE_FINISH     8  // Finished race but not 1st place

// Screen constants
#define ROAD_LEFT       40
#define ROAD_RIGHT      216
#define SCREEN_HEIGHT   240
#define HUD_TOP_Y       8
#define HUD_LINE        8
#define HUD_BAND_BOTTOM 32  // Reserve top band for HUD to avoid sprite overflow

// Player constants
#define PLAYER_START_X  120
#define PLAYER_START_Y  200
#define PLAYER_SPEED    2
#define PLAYER_START_HP 5
#define PLAYER_MAX_HP   100

// Enemy constants
#define ENEMY_START_Y   0    // Start from top of screen (will use 240-16 wrapped)
#define SCROLL_SPEED    2

// Lap distance constant
#define LAP_DISTANCE    700u

// Tile indices
#define TILE_ROAD       0x01
#define TILE_GRASS      0x02
#define TILE_LINE       0x03

// Sprite tiles
#define SPR_CAR         0x00
#define SPR_ENEMY       0x04    // Normal enemy car
#define SPR_SLASH       0x05
#define SPR_BAR_FILL    0x06
#define SPR_BAR_EMPTY   0x07
#define SPR_CAR_ICON    0x08
#define SPR_HLINE       0x09    // Small horizontal line marker
#define SPR_EXPLOSION   0x0E
#define SPR_HITBOX      0x0F    // Hitbox indicator
#define SPR_BULLET      0x0B    // Diamond bullet sprite
#define SPR_DIGIT       0x10
#define SPR_LETTER      0x30
#define SPR_COPYRIGHT   0x4A    // (C) symbol (unused)
#define SPR_HEART       0x4B    // Heart symbol for HP
#define SPR_DOT         0x4C    // Dot/period for version
#define SPR_BOSS        0x60    // Boss/Elite enemy car

// NMI flag from crt0.s (set by NMI handler, cleared by main loop)
extern volatile unsigned char nmi_flag;

// Global variables
static unsigned char game_state;
static unsigned char frame_count;
static unsigned char scroll_y;
static unsigned char pad_now;
static unsigned char pad_old;
static unsigned char pad_new;

static unsigned char player_x;
static unsigned char player_y;
static unsigned char player_hp;
static unsigned char player_inv;

#define MAX_ENEMIES 3
static unsigned char enemy_x[MAX_ENEMIES];
static unsigned char enemy_y[MAX_ENEMIES];
static unsigned char enemy_on[MAX_ENEMIES];
static unsigned char enemy_passed[MAX_ENEMIES];
static unsigned char enemy_rank[MAX_ENEMIES];  // Position/rank of this enemy (1-11)
static unsigned char enemy_next_x;     // Next enemy spawn X position
static unsigned char enemy_warn_timer; // Warning countdown before spawn
static unsigned char enemy_slot;       // Next enemy slot to use
static unsigned char enemy_next_rank;  // Next rank to assign (11, 10, 9, ... 1)

// Explosion effect
static unsigned char explode_x;        // Explosion/retire position X
static unsigned char explode_y;        // Explosion/retire position Y
static unsigned char explode_timer;    // Animation timer (explosion or retire)


static unsigned char position;
static unsigned char lap_count;
static unsigned char loop_count;  // Current loop (2周目, 3周目...)
static unsigned int score;        // Score lower 16 bits (0-65535)
static unsigned int score_high;   // Score upper (increments when score overflows)
static unsigned int distance;
static unsigned int score_multiplier;  // Score multiplier (1-65535)
static unsigned char graze_count;       // Graze counter for HP recovery (every 10)
static unsigned char car_graze_cooldown; // Cooldown for enemy car graze
static unsigned char boost_remaining;   // Boosts remaining this loop (max 2)
static unsigned char boost_active;      // Currently boosting flag
static unsigned char boss_music_active; // Is boss music currently playing?


// Bullet system (danmaku)
#define MAX_BULLETS 48
static unsigned char bullet_x[MAX_BULLETS];
static unsigned char bullet_y[MAX_BULLETS];
static signed char bullet_dx[MAX_BULLETS];  // X velocity
static signed char bullet_dy[MAX_BULLETS];  // Y velocity
static unsigned char bullet_on[MAX_BULLETS];
static unsigned char bullet_grazed[MAX_BULLETS];  // Already grazed flag (1 graze per bullet)
static unsigned char bullet_timer;  // Timer for shooting patterns
static unsigned char bullet_next;   // Next bullet slot (circular buffer)
static unsigned char burst_phase;   // 0-79 counter (avoids % 80 division)

static unsigned char rnd_seed;
static unsigned char win_timer;  // Animation timer for win screen
static unsigned char loop_clear_timer;  // Timer for loop clear celebration

// Confetti particles for win animation
#define MAX_CONFETTI 8
static unsigned char confetti_x[MAX_CONFETTI];
static unsigned char confetti_y[MAX_CONFETTI];
static unsigned char confetti_color[MAX_CONFETTI];

// ============================================
// HIGH SCORE SYSTEM (Battery-backed SRAM)
// ============================================

#define SAVE_MAGIC 0x52  // 'R' for Race - validates save data
#define NUM_HIGH_SCORES 3

// Save data structure in battery-backed SRAM ($6000-$7FFF)
// Use volatile to ensure compiler doesn't optimize away SRAM writes
#pragma bss-name(push, "SAVE")
static volatile unsigned char save_magic;           // Magic byte to validate save
static volatile unsigned int  high_scores[NUM_HIGH_SCORES];      // Top 3 scores (low 16-bit)
static volatile unsigned int  high_scores_high[NUM_HIGH_SCORES]; // Top 3 scores (high 16-bit)
static volatile unsigned char high_names[NUM_HIGH_SCORES][3];    // 3-letter names
static volatile unsigned char max_loop;             // Maximum loop reached (for loop select)
#pragma bss-name(pop)

// Name entry state
static unsigned char name_entry_pos;     // Current letter position (0-2)
static unsigned char name_entry_char;    // Current character index (0-25 = A-Z)
static unsigned char entry_name[3];      // Name being entered
static unsigned char new_score_rank;     // Which rank the new score achieved (0-2)

// Title screen loop selection
static unsigned char title_select_loop;  // Selected starting loop (0-based)

// ============================================
// MUSIC ENGINE
// ============================================

// Music state
static unsigned char music_enabled;
static unsigned char music_frame;      // Frame counter for timing
static unsigned char music_pos;        // Current position in sequence
static unsigned char music_tempo;      // Frames per beat (lower = faster)
static unsigned char current_track;    // 0=title, 1=racing, 2=win, 3=gameover
static unsigned char music_intensity;  // 0=normal, 1=intense, 2=chaos (distortion level)

// Channel states
static unsigned char tri_note;         // Current triangle note
static unsigned char pl1_note;         // Current pulse 1 note
static unsigned char pl2_note;         // Current pulse 2 note
static unsigned char noise_on;         // Noise state

// Note period table (NTSC, octave 2-5)
// Lower value = higher pitch
// Format: timer value low byte, high byte is note index dependent
// Notes: C, C#, D, D#, E, F, F#, G, G#, A, A#, B
static const unsigned int note_table[48] = {
    // Octave 2
    0x6B0, 0x650, 0x5F3, 0x59D, 0x54D, 0x501, 0x4B9, 0x475, 0x435, 0x3F8, 0x3BF, 0x388,
    // Octave 3
    0x358, 0x328, 0x2FA, 0x2CF, 0x2A7, 0x281, 0x25C, 0x23B, 0x21B, 0x1FC, 0x1DF, 0x1C4,
    // Octave 4
    0x1AC, 0x194, 0x17D, 0x168, 0x153, 0x140, 0x12E, 0x11D, 0x10D, 0x0FE, 0x0EF, 0x0E2,
    // Octave 5
    0x0D6, 0x0CA, 0x0BE, 0x0B4, 0x0AA, 0x0A0, 0x097, 0x08F, 0x087, 0x07F, 0x078, 0x071,
};

// Note definitions (index into note_table)
#define NOTE_REST 0xFF
#define C2  0
#define CS2 1
#define D2  2
#define DS2 3
#define E2  4
#define F2  5
#define FS2 6
#define G2  7
#define GS2 8
#define A2  9
#define AS2 10
#define B2  11
#define C3  12
#define CS3 13
#define D3  14
#define DS3 15
#define E3  16
#define F3  17
#define FS3 18
#define G3  19
#define GS3 20
#define A3  21
#define AS3 22
#define B3  23
#define C4  24
#define CS4 25
#define D4  26
#define DS4 27
#define E4  28
#define F4  29
#define FS4 30
#define G4  31
#define GS4 32
#define A4  33
#define AS4 34
#define B4  35
#define C5  36
#define CS5 37
#define D5  38
#define DS5 39
#define E5  40
#define F5  41
#define FS5 42
#define G5  43
#define GS5 44
#define A5  45
#define AS5 46
#define B5  47

// ============================================
// RACING BGM LOOP 1 (DAY) - Bright, upbeat rock
// ============================================
#define RACING_LEN 32

// Triangle bass - bouncy driving bass (C major feel)
static const unsigned char racing_tri[RACING_LEN] = {
    C2, C2, G2, C3,  A2, A2, G2, G2,
    F2, F2, G2, G2,  A2, A2, B2, B2,
    C2, C2, G2, C3,  A2, A2, G2, G2,
    F2, F2, G2, G2,  C3, C3, NOTE_REST, C2,
};

// Pulse 1 - cheerful melody
static const unsigned char racing_pl1[RACING_LEN] = {
    C4, E4, G4, E4,  A4, G4, E4, C4,
    F4, A4, C5, A4,  G4, E4, D4, C4,
    C4, E4, G4, E4,  A4, C5, B4, G4,
    F4, G4, A4, B4,  C5, NOTE_REST, NOTE_REST, NOTE_REST,
};

// Pulse 2 - harmony
static const unsigned char racing_pl2[RACING_LEN] = {
    C4, E4, G4, E4,  A3, E4, A4, E4,
    F3, A3, C4, A3,  G3, B3, D4, B3,
    C4, E4, G4, E4,  A3, E4, A4, E4,
    F3, A3, C4, A3,  G3, B3, D4, B3,
};

// Noise pattern (0=off, 1=kick, 2=snare, 3=hihat)
static const unsigned char racing_noise[RACING_LEN] = {
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 2, 3,  1, 3, 3, 3,
};

// ============================================
// RACING BGM LOOP 2 (EVENING) - Mysterious, tension (D minor)
// ============================================
// Triangle bass - ominous pulsing
static const unsigned char racing2_tri[RACING_LEN] = {
    D2, D2, A2, D3,  D2, D2, A2, D3,
    F2, F2, C3, F3,  F2, F2, C3, F3,
    C2, C2, G2, C3,  A2, A2, E3, A3,
    D2, D2, A2, D3,  D2, D2, A2, D3,
};

// Pulse 1 - haunting melody
static const unsigned char racing2_pl1[RACING_LEN] = {
    D4, F4, A4, F4,  C5, A4, G4, F4,
    E4, G4, A4, G4,  F4, E4, D4, NOTE_REST,
    D4, F4, A4, C5,  A4, G4, F4, E4,
    D4, E4, F4, G4,  A4, NOTE_REST, NOTE_REST, NOTE_REST,
};

// Pulse 2 - eerie arpeggios
static const unsigned char racing2_pl2[RACING_LEN] = {
    D3, F3, A3, F3,  D4, F4, A4, F4,
    F3, A3, C4, A3,  F3, A3, C4, A3,
    C3, E3, G3, E3,  A3, C4, E4, C4,
    D3, F3, A3, F3,  D4, F4, A4, F4,
};

// Subtle drums - tension building
static const unsigned char racing2_noise[RACING_LEN] = {
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 2, 3,  1, 3, 3, 3,
};

// ============================================
// RACING BGM LOOP 3 (NIGHT) - Dark, intense (E minor)
// ============================================
// Triangle bass - menacing
static const unsigned char racing3_tri[RACING_LEN] = {
    E2, E2, B2, E3,  E2, E2, B2, E3,
    G2, G2, D3, G3,  A2, A2, E3, A3,
    B2, B2, FS3, B3,  A2, A2, E3, A3,
    E2, E2, B2, E3,  E2, E2, B2, E3,
};

// Pulse 1 - dark powerful melody
static const unsigned char racing3_pl1[RACING_LEN] = {
    E4, G4, B4, G4,  D5, B4, A4, G4,
    E5, D5, B4, G4,  A4, B4, G4, E4,
    G4, B4, D5, B4,  E5, D5, C5, B4,
    A4, G4, E4, G4,  E4, NOTE_REST, NOTE_REST, NOTE_REST,
};

// Pulse 2 - ominous harmonies
static const unsigned char racing3_pl2[RACING_LEN] = {
    E3, G3, B3, G3,  G3, B3, D4, B3,
    A3, C4, E4, C4,  B3, D4, FS4, D4,
    E3, G3, B3, G3,  G3, B3, D4, B3,
    A3, C4, E4, C4,  B3, D4, FS4, D4,
};

// Heavy drums
static const unsigned char racing3_noise[RACING_LEN] = {
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 3, 3,  2, 3, 3, 3,
    1, 3, 2, 3,  1, 3, 3, 3,
};

// ============================================
// BOSS BGM LOOP 1 - Exciting battle! (A minor)
// ============================================
#define BOSS_LEN 32

static const unsigned char boss1_tri[BOSS_LEN] = {
    A2, A2, E3, A2,  A2, A2, E3, A2,
    G2, G2, D3, G2,  F2, F2, C3, F2,
    A2, A2, E3, A2,  A2, A2, E3, A2,
    G2, G2, F2, F2,  E2, E2, B2, E2,
};

static const unsigned char boss1_pl1[BOSS_LEN] = {
    A4, C5, E5, C5,  A4, E5, C5, A4,
    G4, B4, D5, B4,  F4, A4, C5, A4,
    A4, C5, E5, G5,  E5, C5, A4, C5,
    G4, B4, D5, B4,  E4, E4, E5, E4,
};

static const unsigned char boss1_pl2[BOSS_LEN] = {
    E4, A4, C5, A4,  E4, C5, A4, E4,
    D4, G4, B4, G4,  C4, F4, A4, F4,
    E4, A4, C5, E5,  C5, A4, E4, A4,
    D4, G4, B4, G4,  B3, B3, B4, B3,
};

static const unsigned char boss1_noise[BOSS_LEN] = {
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
};

// ============================================
// BOSS BGM LOOP 2 - Mysterious danger (D minor)
// ============================================
static const unsigned char boss2_tri[BOSS_LEN] = {
    D2, D2, A2, D3,  D2, D2, A2, D3,
    C2, C2, G2, C3,  AS2, AS2, F2, AS2,
    D2, D2, A2, D3,  F2, F2, C3, F3,
    G2, G2, D3, G3,  A2, A2, E3, A3,
};

static const unsigned char boss2_pl1[BOSS_LEN] = {
    D5, A4, D5, F5,  D5, A4, D5, F5,
    C5, G4, C5, E5,  AS4, F4, A4, C5,
    D5, F5, A5, F5,  E5, G5, F5, E5,
    D5, C5, AS4, A4,  D5, NOTE_REST, D5, D5,
};

static const unsigned char boss2_pl2[BOSS_LEN] = {
    F4, D4, F4, A4,  F4, D4, F4, A4,
    E4, C4, E4, G4,  D4, AS3, D4, F4,
    F4, A4, D5, A4,  G4, B4, A4, G4,
    F4, E4, D4, CS4,  D4, NOTE_REST, D4, D4,
};

static const unsigned char boss2_noise[BOSS_LEN] = {
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
};

// ============================================
// BOSS BGM LOOP 3 - FINAL BOSS! Maximum intensity (E minor)
// ============================================
static const unsigned char boss3_tri[BOSS_LEN] = {
    E2, E2, B2, E3,  E2, E2, B2, E3,
    E2, E2, B2, E3,  G2, G2, D3, G3,
    A2, A2, E3, A3,  B2, B2, FS3, B3,
    G2, G2, E2, E2,  E2, E2, E3, E2,
};

static const unsigned char boss3_pl1[BOSS_LEN] = {
    E5, B4, E5, G5,  E5, B4, E5, G5,
    E5, G5, B5, G5,  E5, G5, A5, B5,
    B4, D5, FS5, B5,  A5, FS5, D5, FS5,
    G5, A5, G5, A5,  E5, E5, E5, E5,
};

static const unsigned char boss3_pl2[BOSS_LEN] = {
    G4, E4, G4, B4,  G4, E4, G4, B4,
    G4, B4, E5, B4,  G4, B4, C5, E5,
    D4, FS4, B4, D5,  C5, A4, FS4, A4,
    B4, C5, B4, C5,  G4, G4, G4, G4,
};

static const unsigned char boss3_noise[BOSS_LEN] = {
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 3, 2, 3,
};

// ============================================
// TITLE BGM - Heroic and majestic!
// ============================================
#define TITLE_LEN 32

// Powerful triangle bass - driving rhythm
static const unsigned char title_tri[TITLE_LEN] = {
    C3, C3, G2, C3,  A2, A2, E3, A2,
    F2, F2, C3, F3,  G2, G2, D3, G3,
    C3, C3, G2, C3,  A2, A2, E3, A2,
    F2, F2, G2, G2,  C3, C3, C3, C3,
};

// Heroic fanfare melody
static const unsigned char title_pl1[TITLE_LEN] = {
    C5, E5, G5, E5,  D5, C5, G4, C5,
    A4, C5, E5, C5,  G4, A4, B4, G4,
    F4, A4, C5, A4,  G4, B4, D5, B4,
    E5, D5, C5, B4,  C5, NOTE_REST, C5, NOTE_REST,
};

// Power chord harmony
static const unsigned char title_pl2[TITLE_LEN] = {
    E4, G4, C5, G4,  G4, E4, C4, E4,
    C4, E4, A4, E4,  D4, F4, G4, D4,
    A3, C4, F4, C4,  B3, D4, G4, D4,
    G4, F4, E4, D4,  E4, NOTE_REST, E4, NOTE_REST,
};

// ============================================
// WIN BGM - Triumphant fanfare!
// ============================================
#define WIN_LEN 16

static const unsigned char win_tri[WIN_LEN] = {
    C3, C3, G2, G2,  C3, C3, C3, C3,
    F2, F2, C3, C3,  G2, G2, C3, C3,
};

static const unsigned char win_pl1[WIN_LEN] = {
    C5, E5, G5, G5,  E5, C5, E5, G5,
    F5, F5, E5, E5,  D5, D5, C5, C5,
};

static const unsigned char win_pl2[WIN_LEN] = {
    E4, G4, C5, C5,  G4, E4, G4, C5,
    A4, A4, G4, G4,  F4, F4, E4, E4,
};

// ============================================
// GAME OVER BGM - Sad, descending melody
// ============================================
#define GAMEOVER_LEN 16

// Slow, mournful triangle bass
static const unsigned char gameover_tri[GAMEOVER_LEN] = {
    A2, NOTE_REST, E2, NOTE_REST,
    F2, NOTE_REST, E2, NOTE_REST,
    D2, NOTE_REST, C2, NOTE_REST,
    B2, NOTE_REST, A2, NOTE_REST,
};

// Descending sad melody
static const unsigned char gameover_pl1[GAMEOVER_LEN] = {
    E4, D4, C4, B3,
    A3, NOTE_REST, G3, NOTE_REST,
    A3, B3, C4, NOTE_REST,
    B3, A3, NOTE_REST, NOTE_REST,
};

// Minor harmony
static const unsigned char gameover_pl2[GAMEOVER_LEN] = {
    C4, B3, A3, G3,
    F3, NOTE_REST, E3, NOTE_REST,
    F3, G3, A3, NOTE_REST,
    G3, E3, NOTE_REST, NOTE_REST,
};

// ============================================
// EPILOGUE BGM - Calm, reflective ending
// ============================================
#define EPILOGUE_LEN 16

// Gentle, sustained bass
static const unsigned char epilogue_tri[EPILOGUE_LEN] = {
    C3, C3, C3, C3,  G2, G2, G2, G2,
    A2, A2, A2, A2,  E2, E2, E2, E2,
};

// Soft, peaceful melody
static const unsigned char epilogue_pl1[EPILOGUE_LEN] = {
    E4, G4, C5, NOTE_REST,  D5, C5, B4, NOTE_REST,
    C5, E5, A4, NOTE_REST,  G4, F4, E4, NOTE_REST,
};

// Gentle harmony
static const unsigned char epilogue_pl2[EPILOGUE_LEN] = {
    C4, E4, G4, NOTE_REST,  B3, A3, G3, NOTE_REST,
    A3, C4, E4, NOTE_REST,  E3, D3, C3, NOTE_REST,
};

// Palette data
const unsigned char palette[32] = {
    // BG palettes
    0x0F, 0x00, 0x10, 0x30,  // Road (gray)
    0x0F, 0x09, 0x19, 0x29,  // Grass (green)
    0x0F, 0x01, 0x21, 0x31,  // Blue
    0x0F, 0x16, 0x27, 0x37,  // Red
    // Sprite palettes
    0x0F, 0x11, 0x21, 0x31,  // Palette 0: Player (blue)
    0x0F, 0x06, 0x16, 0x26,  // Palette 1: Enemy (red)
    0x0F, 0x28, 0x38, 0x30,  // Palette 2: Bullet (bright yellow)
    0x0F, 0x30, 0x30, 0x30   // Palette 3: HUD (white)
};

// ============================================
// FORWARD DECLARATIONS
// ============================================
static void do_game_over(void);
static void do_finish_lose(void);
static void finish_game_over(void);
static unsigned char check_high_score(unsigned int new_score_high, unsigned int new_score_low);
static void insert_high_score(unsigned char rank, unsigned int new_score_high, unsigned int new_score_low);
static void init_name_entry(unsigned char rank);
static void music_play(unsigned char track);
static void music_stop(void);
static void update_loop_palette(void);

// ============================================
// MUSIC FUNCTIONS
// ============================================

// Initialize APU
static void init_apu(void) {
    // Disable all channels first
    APU_STATUS = 0x00;

    // Initialize all channel registers to known state
    APU_PL1_VOL = 0x30;  // Silence pulse 1
    APU_PL1_SWP = 0x00;
    APU_PL1_LO = 0x00;
    APU_PL1_HI = 0x00;

    APU_PL2_VOL = 0x30;  // Silence pulse 2
    APU_PL2_SWP = 0x00;
    APU_PL2_LO = 0x00;
    APU_PL2_HI = 0x00;

    APU_TRI_LIN = 0x80;  // Halt triangle linear counter
    APU_TRI_LO = 0x00;
    APU_TRI_HI = 0x00;

    APU_NOI_VOL = 0x30;  // Silence noise
    APU_NOI_LO = 0x00;
    APU_NOI_HI = 0x00;

    // Set frame counter mode (4-step sequence, disable IRQ)
    APU_FRAME = 0x40;

    // Now enable all channels
    APU_STATUS = 0x0F;  // Enable pulse1, pulse2, triangle, noise

    music_enabled = 1;
    music_frame = 0;
    music_pos = 0;
    music_tempo = 8;  // 8 frames per beat = ~7.5 BPS at 60fps
    current_track = 0;
}

// Graze SFX timer (counts down, plays sound while > 0)
static unsigned char sfx_graze_timer;
// Damage SFX timer and pitch
static unsigned char sfx_damage_timer;
static unsigned int sfx_damage_pitch;
// Low HP warning beep timer
static unsigned char sfx_lowhp_timer;

// Play graze sound effect (short metallic scrape)
static void sfx_graze(void) {
    sfx_graze_timer = 8;  // 8 frames duration
}

// Play damage sound effect (deflating/shrinking sound)
static void sfx_damage(void) {
    sfx_damage_timer = 20;  // 20 frames duration
    sfx_damage_pitch = 200; // Start at mid-high pitch
}

// Stop all SFX (call on scene transitions)
static void sfx_stop(void) {
    sfx_graze_timer = 0;
    sfx_damage_timer = 0;
    sfx_lowhp_timer = 0;
    APU_NOI_VOL = 0x30;  // Silence noise channel
}

// Update SFX (call every frame - AFTER music_update to override BGM)
static void update_sfx(void) {
    // Graze SFX - noise channel
    if (sfx_graze_timer > 0) {
        // Re-apply noise settings every frame to override BGM
        APU_NOI_VOL = 0x3F;
        APU_NOI_LO = 0x82;
        APU_NOI_HI = 0x08;
        --sfx_graze_timer;
    }
    // Damage SFX - pulse 2 channel with descending pitch
    if (sfx_damage_timer > 0) {
        // Descending pitch "womp" sound
        APU_PL2_VOL = 0xBF;  // Duty 50%, volume 15
        APU_PL2_LO = (unsigned char)(sfx_damage_pitch & 0xFF);
        APU_PL2_HI = (unsigned char)((sfx_damage_pitch >> 8) & 0x07) | 0x08;
        // Increase period = lower pitch (deflating effect)
        sfx_damage_pitch += 40;
        --sfx_damage_timer;
    }
    // Low HP warning beep - distinct high-pitched pip (HP=1 critical warning)
    if (sfx_lowhp_timer > 0) {
        if (sfx_lowhp_timer > 2) {
            // Beep on (high pitch, medium volume, 50% duty for clarity)
            APU_PL2_VOL = 0xB8;  // Duty 50%, volume 8 (audible over BGM)
            APU_PL2_SWP = 0x00;
            APU_PL2_LO = 0x50;   // Higher pitch (~1500Hz)
            APU_PL2_HI = 0x00;
        }
        --sfx_lowhp_timer;
    }
}

// Play a note on triangle channel
static void play_triangle(unsigned char note) {
    unsigned int period;
    if (note == NOTE_REST || note >= 48) {
        // Silence: set linear counter to 0, halt flag clear
        APU_TRI_LIN = 0x00;
        APU_TRI_HI = 0x00;  // Trigger reload with 0 counter
        return;
    }
    period = note_table[note];
    // Bit 7 = 1: halt length counter (so it plays continuously)
    // Bits 6-0 = 127: max linear counter value
    APU_TRI_LIN = 0xFF;
    APU_TRI_LO = (unsigned char)(period & 0xFF);
    // Bits 7-3: length counter load (0x1F = longest)
    // Bits 2-0: timer high 3 bits
    // Writing to $400B also reloads the linear counter
    APU_TRI_HI = (unsigned char)((period >> 8) & 0x07) | 0xF8;
}

// Play a note on pulse 1 channel (duty changes with intensity)
static void play_pulse1(unsigned char note) {
    unsigned int period;
    unsigned char vol;
    if (note == NOTE_REST || note >= 48) {
        APU_PL1_VOL = 0x30;  // Silence
        return;
    }
    period = note_table[note];
    // Duty cycle: 0=12.5%, 1=25%, 2=50%, 3=25%neg
    // Intensity 0: 50% (clean), 1: 25% (edgy), 2: 12.5% (harsh)
    switch (music_intensity) {
        case 0:  vol = 0xBF; break;  // 50% duty, max vol
        case 1:  vol = 0x7F; break;  // 25% duty, max vol
        default: vol = 0x3F; break;  // 12.5% duty, max vol (distorted!)
    }
    APU_PL1_VOL = vol;
    APU_PL1_SWP = 0x00;
    APU_PL1_LO = (unsigned char)(period & 0xFF);
    APU_PL1_HI = (unsigned char)((period >> 8) & 0x07) | 0xF8;
}

// Play a note on pulse 2 channel (duty changes with intensity)
static void play_pulse2(unsigned char note) {
    unsigned int period;
    unsigned char vol;
    if (note == NOTE_REST || note >= 48) {
        APU_PL2_VOL = 0x30;  // Silence
        return;
    }
    period = note_table[note];
    // More aggressive duty cycle changes for pulse 2
    switch (music_intensity) {
        case 0:  vol = 0x7A; break;  // 25% duty, med vol (clean)
        case 1:  vol = 0x3C; break;  // 12.5% duty, med vol (harsh)
        default: vol = 0x3F; break;  // 12.5% duty, max vol (chaos!)
    }
    APU_PL2_VOL = vol;
    APU_PL2_SWP = 0x00;
    APU_PL2_LO = (unsigned char)(period & 0xFF);
    APU_PL2_HI = (unsigned char)((period >> 8) & 0x07) | 0xF8;
}

// Play noise drum
static void play_noise(unsigned char type) {
    switch (type) {
        case 0:  // Off
            APU_NOI_VOL = 0x30;
            break;
        case 1:  // Kick
            APU_NOI_VOL = 0x3F;  // Constant, max volume
            APU_NOI_LO = 0x0C;   // Low pitch
            APU_NOI_HI = 0x18;
            break;
        case 2:  // Snare
            APU_NOI_VOL = 0x3A;  // Constant, medium volume
            APU_NOI_LO = 0x05;   // Mid pitch, mode bit
            APU_NOI_HI = 0x28;
            break;
        case 3:  // Hi-hat
            APU_NOI_VOL = 0x34;  // Constant, low volume
            APU_NOI_LO = 0x02;   // High pitch
            APU_NOI_HI = 0x08;
            break;
    }
}

// Track numbers
#define TRACK_TITLE     0
#define TRACK_RACING    1
#define TRACK_WIN       2
#define TRACK_GAMEOVER  3
#define TRACK_EPILOGUE  4
#define TRACK_BOSS1     5   // Boss battle loop 1
#define TRACK_BOSS2     6   // Boss battle loop 2
#define TRACK_BOSS3     7   // Boss battle loop 3 (final)

// Start a music track
static void music_play(unsigned char track) {
    current_track = track;
    music_pos = 0;
    music_frame = 0;

    switch (track) {
        case TRACK_TITLE:  // Title - Heroic march
            music_tempo = 12;  // Slower, more majestic
            music_intensity = 0;
            break;
        case TRACK_RACING:  // Racing - tempo varies with intensity
            switch (music_intensity) {
                case 0:  music_tempo = 10; break;  // Loop 1: Relaxed
                case 1:  music_tempo = 9;  break;  // Loop 2: Moderate
                default: music_tempo = 8;  break;  // Loop 3: Intense
            }
            break;
        case TRACK_WIN:  // Win
            music_tempo = 14;
            music_intensity = 0;
            break;
        case TRACK_GAMEOVER:  // Game Over
            music_tempo = 20;  // Slower, more dramatic
            music_intensity = 0;
            break;
        case TRACK_EPILOGUE:  // Epilogue - calm, slow
            music_tempo = 16;
            music_intensity = 0;
            break;
        case TRACK_BOSS1:  // Boss loop 1 - exciting
            music_tempo = 8;
            break;
        case TRACK_BOSS2:  // Boss loop 2 - tense
            music_tempo = 7;
            break;
        case TRACK_BOSS3:  // Boss loop 3 - INTENSE!
            music_tempo = 6;
            break;
    }
}

// Set music intensity (for loop-based changes)
static void music_set_intensity(unsigned char intensity) {
    music_intensity = intensity;
    // Update tempo if currently playing racing or boss music
    if (current_track == TRACK_RACING) {
        switch (intensity) {
            case 0:  music_tempo = 10; break;  // Loop 1
            case 1:  music_tempo = 9;  break;  // Loop 2
            default: music_tempo = 8;  break;  // Loop 3
        }
        music_pos = 0;  // Reset to sync with new track
    }
}

// Stop music
static void music_stop(void) {
    APU_PL1_VOL = 0x30;
    APU_PL2_VOL = 0x30;
    APU_TRI_LIN = 0x00;
    APU_NOI_VOL = 0x30;
}

// Pause music (silence but keep position)
static void music_pause(void) {
    music_enabled = 0;
    APU_PL1_VOL = 0x30;
    APU_PL2_VOL = 0x30;
    APU_TRI_LIN = 0x00;
    APU_NOI_VOL = 0x30;
}

// Resume music from where it paused
static void music_resume(void) {
    music_enabled = 1;
}

// Update music (call every frame)
// Called from NMI handler for stable timing
void music_update(void) {
    unsigned char len;
    const unsigned char *tri_data;
    const unsigned char *pl1_data;
    const unsigned char *pl2_data;
    const unsigned char *noise_data;

    if (!music_enabled) return;

    ++music_frame;
    if (music_frame < music_tempo) return;

    music_frame = 0;

    // Select track data
    switch (current_track) {
        case TRACK_TITLE:  // Title
            len = TITLE_LEN;
            tri_data = title_tri;
            pl1_data = title_pl1;
            pl2_data = title_pl2;
            noise_data = 0;
            break;
        case TRACK_RACING:  // Racing - different music per loop
            len = RACING_LEN;
            switch (music_intensity) {
                case 0:  // Loop 1 - Day
                    tri_data = racing_tri;
                    pl1_data = racing_pl1;
                    pl2_data = racing_pl2;
                    noise_data = racing_noise;
                    break;
                case 1:  // Loop 2 - Evening
                    tri_data = racing2_tri;
                    pl1_data = racing2_pl1;
                    pl2_data = racing2_pl2;
                    noise_data = racing2_noise;
                    break;
                default: // Loop 3 - Night
                    tri_data = racing3_tri;
                    pl1_data = racing3_pl1;
                    pl2_data = racing3_pl2;
                    noise_data = racing3_noise;
                    break;
            }
            break;
        case TRACK_WIN:  // Win
            len = WIN_LEN;
            tri_data = win_tri;
            pl1_data = win_pl1;
            pl2_data = win_pl2;
            noise_data = 0;
            break;
        case TRACK_GAMEOVER:  // Game Over
            len = GAMEOVER_LEN;
            tri_data = gameover_tri;
            pl1_data = gameover_pl1;
            pl2_data = gameover_pl2;
            noise_data = 0;
            break;
        case TRACK_EPILOGUE:  // Epilogue
            len = EPILOGUE_LEN;
            tri_data = epilogue_tri;
            pl1_data = epilogue_pl1;
            pl2_data = epilogue_pl2;
            noise_data = 0;
            break;
        case TRACK_BOSS1:  // Boss battle loop 1
            len = BOSS_LEN;
            tri_data = boss1_tri;
            pl1_data = boss1_pl1;
            pl2_data = boss1_pl2;
            noise_data = boss1_noise;
            break;
        case TRACK_BOSS2:  // Boss battle loop 2
            len = BOSS_LEN;
            tri_data = boss2_tri;
            pl1_data = boss2_pl1;
            pl2_data = boss2_pl2;
            noise_data = boss2_noise;
            break;
        case TRACK_BOSS3:  // Boss battle loop 3 - FINAL
            len = BOSS_LEN;
            tri_data = boss3_tri;
            pl1_data = boss3_pl1;
            pl2_data = boss3_pl2;
            noise_data = boss3_noise;
            break;
        default:
            return;
    }

    // Play current notes
    play_triangle(tri_data[music_pos]);
    play_pulse1(pl1_data[music_pos]);
    play_pulse2(pl2_data[music_pos]);
    if (noise_data) {
        play_noise(noise_data[music_pos]);
    }

    // Advance position
    ++music_pos;
    if (music_pos >= len) {
        music_pos = 0;
    }
}

// NMI enabled flag (set after PPU_CTRL enables NMI)
static unsigned char nmi_enabled;

// Wait for vblank using NMI flag (more reliable than PPU_STATUS)
static void wait_vblank(void) {
    if (nmi_enabled) {
        // Clear flag FIRST to ensure we wait for the *next* VBlank
        nmi_flag = 0;
        while (!nmi_flag);
    } else {
        // Fallback for early init before NMI is enabled
        while (!(PPU_STATUS & 0x80));
    }
}

// Turn off PPU
static void ppu_off(void) {
    PPU_MASK = 0x00;
}

// Turn on PPU (sprites and background)
static void ppu_on(void) {
    PPU_MASK = 0x1E;
}

// Set PPU address for VRAM writes
// Always reset the address latch first for safety
static void ppu_addr(unsigned int addr) {
    (void)PPU_STATUS;  // Reset PPU address latch
    PPU_ADDR = (unsigned char)(addr >> 8);
    PPU_ADDR = (unsigned char)(addr);
}

// Add points to score with overflow handling
static void add_score(unsigned int points) {
    unsigned int old_score = score;
    score += points;
    // Check for overflow (new score is less than old score)
    if (score < old_score) {
        ++score_high;
    }
}

// Read controller
static unsigned char read_pad(void) {
    unsigned char result = 0;
    unsigned char i;

    JOYPAD1 = 1;
    JOYPAD1 = 0;

    for (i = 0; i < 8; ++i) {
        result <<= 1;
        result |= (JOYPAD1 & 1);
    }
    // Return result directly - emulators typically return pressed=1, released=0
    return result;
}

// Simple random number
static unsigned char rnd(void) {
    rnd_seed ^= (rnd_seed << 5);
    rnd_seed ^= (rnd_seed >> 3);
    rnd_seed ^= (rnd_seed << 7);
    if (rnd_seed == 0) rnd_seed = 42;
    return rnd_seed;
}

// Forward declaration
static void init_win_animation(void);

// Clear all sprites (move offscreen)
static void clear_sprites(void) {
    unsigned char i;
    for (i = 0; i < 64; ++i) {
        OAM[i * 4] = 0xFF;
    }
}

// Set a single sprite (with overflow guard)
static unsigned char set_sprite(unsigned char id, unsigned char x, unsigned char y,
                                unsigned char tile, unsigned char attr) {
    unsigned int idx;
    // Guard against OAM overflow (NES has 64 sprites max)
    if (id >= 64) return id;
    idx = id * 4;
    OAM[idx] = y;
    OAM[idx + 1] = tile;
    OAM[idx + 2] = attr;
    OAM[idx + 3] = x;
    return id + 1;
}

// Set a 16x16 metasprite (4 sprites)
static unsigned char set_car(unsigned char id, unsigned char x, unsigned char y,
                             unsigned char tile_base, unsigned char attr) {
    id = set_sprite(id, x,     y,     tile_base,     attr);
    id = set_sprite(id, x + 8, y,     tile_base + 1, attr);
    id = set_sprite(id, x,     y + 8, tile_base + 2, attr);
    id = set_sprite(id, x + 8, y + 8, tile_base + 3, attr);
    return id;
}

// Load palettes
static void load_palettes(void) {
    unsigned char i;
    ppu_addr(0x3F00);
    for (i = 0; i < 32; ++i) {
        PPU_DATA = palette[i];
    }
}

// Update background palette for different loops
// Loop 1: Day, Loop 2: Evening, Loop 3+: Night, then alternate
// This function should be called while PPU is OFF (during draw_road)
static void update_loop_palette(void) {
    unsigned char grass_hue, road_hue;

    // Select hue based on loop_count
    if (loop_count == 0) {
        grass_hue = 0x09;  // Green (day)
        road_hue = 0x00;   // Gray
    } else if ((loop_count & 1) == 1) {
        grass_hue = 0x17;  // Yellow-orange (evening)
        road_hue = 0x07;   // Orange-brown
    } else {
        grass_hue = 0x01;  // Blue (night)
        road_hue = 0x00;   // Gray-blue
    }

    // Write road palette (BG palette 0 at $3F00-$3F03)
    ppu_addr(0x3F00);
    PPU_DATA = 0x0F;              // Background color (black)
    PPU_DATA = road_hue;          // Dark
    PPU_DATA = road_hue + 0x10;   // Medium
    PPU_DATA = road_hue + 0x20;   // Light

    // Write grass palette (BG palette 1 at $3F04-$3F07)
    ppu_addr(0x3F04);
    PPU_DATA = 0x0F;              // Background color (black)
    PPU_DATA = grass_hue;         // Dark
    PPU_DATA = grass_hue + 0x10;  // Medium
    PPU_DATA = grass_hue + 0x20;  // Light

    // Reset PPU address latch and scroll after palette writes
    (void)PPU_STATUS;
    PPU_SCROLL = 0;
    PPU_SCROLL = 0;
}

// Draw the road background
// ROAD_LEFT=40 (tile 5), ROAD_RIGHT=216 (tile 27)
static void draw_road(void) {
    unsigned char row, col;
    unsigned char attr_row;

    // Wait for VBlank before turning off PPU to avoid mid-frame glitch
    wait_vblank();
    ppu_off();

    // Draw nametable (30 rows x 32 columns)
    // Road spans tiles 5-26, grass is 0-4 and 27-31
    for (row = 0; row < 30; ++row) {
        ppu_addr(0x2000 + (unsigned int)row * 32);
        for (col = 0; col < 32; ++col) {
            if (col < 5 || col >= 27) {
                // Grass on sides
                PPU_DATA = TILE_GRASS;
            } else if (col == 15 || col == 16) {
                // Center line (dashed)
                PPU_DATA = ((row & 1) == 0) ? TILE_LINE : TILE_ROAD;
            } else {
                // Road
                PPU_DATA = TILE_ROAD;
            }
        }
    }

    // Attribute table (at $23C0)
    // Each byte controls 4x4 tiles (32x32 pixels)
    // Palette 0 = road (gray), Palette 1 = grass (green)
    // Road is tiles 5-26, grass is 0-4 and 27-31
    ppu_addr(0x23C0);
    for (attr_row = 0; attr_row < 8; ++attr_row) {
        // 0x55 = all grass, 0x00 = all road
        // 0x05 = left half grass, right half road
        // 0x50 = left half road, right half grass
        PPU_DATA = 0x55;  // Columns 0-3: grass
        PPU_DATA = 0x05;  // Columns 4-7: grass/road border
        PPU_DATA = 0x00;  // Columns 8-11: road
        PPU_DATA = 0x00;  // Columns 12-15: road
        PPU_DATA = 0x00;  // Columns 16-19: road
        PPU_DATA = 0x00;  // Columns 20-23: road
        PPU_DATA = 0x50;  // Columns 24-27: road/grass border
        PPU_DATA = 0x55;  // Columns 28-31: grass
    }

    ppu_on();
}

// Clear center line for title screen (replace dashed line with plain road)
static void clear_center_line(void) {
    unsigned char row;

    wait_vblank();
    ppu_off();

    // Overwrite center line tiles (columns 15-16) with plain road
    for (row = 0; row < 30; ++row) {
        ppu_addr(0x2000 + (unsigned int)row * 32 + 15);
        PPU_DATA = TILE_ROAD;
        PPU_DATA = TILE_ROAD;
    }

    ppu_on();
}

// Prepare next enemy spawn (show warning marker)
static void prepare_enemy(void) {
    enemy_next_x = ROAD_LEFT + 8 + (rnd() & 0x7F);
    if (enemy_next_x > ROAD_RIGHT - 24) {
        enemy_next_x = ROAD_RIGHT - 24;
    }
    enemy_warn_timer = 120;  // 2 second warning
}

// Actually spawn the enemy (appears from top, player must overtake)
static void spawn_enemy(void) {
    unsigned char slot = enemy_slot;

    // Don't spawn if no more ranks to assign
    if (enemy_next_rank < 1) return;

    enemy_x[slot] = enemy_next_x;
    enemy_y[slot] = 8;  // Start just below top of screen
    enemy_on[slot] = 1;
    enemy_passed[slot] = 0;
    enemy_rank[slot] = enemy_next_rank;  // Assign unique rank
    enemy_next_rank--;  // Next enemy gets lower rank
    enemy_warn_timer = 0;
    // Cycle through slots (can't use bitwise AND since MAX_ENEMIES is not power of 2)
    ++enemy_slot;
    if (enemy_slot >= MAX_ENEMIES) enemy_slot = 0;
}

// Absolute value helper (needed before bullet collision check)
static unsigned char abs_diff(unsigned char a, unsigned char b) {
    if (a >= b) return a - b;
    return b - a;
}

// Spawn a single bullet (circular buffer - overwrites oldest)
static void spawn_bullet(unsigned char x, unsigned char y, signed char dx, signed char dy) {
    // Use circular buffer - always use next slot, overwriting old bullets
    bullet_x[bullet_next] = x;
    bullet_y[bullet_next] = y;
    bullet_dx[bullet_next] = dx;

    // Increase bullet speed slightly in later loops
    // Add +1 speed for every 3 loops
    if (dy > 0) {
        dy += (loop_count / 3);  // Moving down
        if (dy > 5) dy = 5;  // Cap at 5
    } else if (dy < 0) {
        dy -= (loop_count / 3);  // Moving up (more negative)
        if (dy < -5) dy = -5;
    }

    bullet_dy[bullet_next] = dy;
    bullet_on[bullet_next] = 1;
    bullet_grazed[bullet_next] = 0;  // Reset graze flag for new bullet
    ++bullet_next;
    if (bullet_next >= MAX_BULLETS) {
        bullet_next = 0;
    }
}

// Current pattern phase for complex patterns
static unsigned char pattern_phase;
static unsigned char pattern_type;

// Calculate aimed bullet velocity - X component
static signed char calc_aim_dx(unsigned char bx) {
    unsigned char px = player_x + 8;
    if (px > bx + 24) return 2;
    if (px > bx + 8) return 1;
    if (px + 24 < bx) return -2;
    if (px + 8 < bx) return -1;
    return 0;
}

// Calculate aimed bullet velocity - Y component
static signed char calc_aim_dy(unsigned char by) {
    unsigned char py = player_y + 8;
    if (py > by + 24) return 2;
    if (py > by + 8) return 1;
    if (py + 24 < by) return -2;
    if (py + 8 < by) return -1;
    return 0;
}

// Spawn boss danmaku - aimed patterns towards player
// Rank 1: Intense patterns, Rank 2: Moderate, Rank 3: Mild
static void spawn_boss_danmaku(unsigned char i, unsigned char cx, unsigned char cy) {
    unsigned char pattern, angle, rank;
    signed char dx, dy, aim_dx, aim_dy;

    rank = enemy_rank[i];
    // Boss pattern based on rank and timer
    pattern = (bullet_timer + rank * 64) & 0xFF;

    // Calculate base aim direction
    aim_dx = calc_aim_dx(cx);
    aim_dy = calc_aim_dy(cy);
    // Ensure some movement if directly aligned
    if (aim_dy == 0) aim_dy = (cy < player_y) ? 1 : -1;

    // Rank 1 (Final Boss): Full intensity - all patterns
    // Rank 2: Moderate intensity - fewer patterns, slower
    // Rank 3: Mild intensity - simple patterns only

    if (rank == 1) {
        // Final Boss: Maximum intensity
        // Pattern 1: Aimed spiral (every 12 frames)
        if ((pattern & 0x0B) == 0) {
            angle = pattern >> 3;
            switch (angle & 0x03) {
                case 0: dx = aim_dx;     dy = aim_dy + 1; break;
                case 1: dx = aim_dx + 1; dy = aim_dy;     break;
                case 2: dx = aim_dx;     dy = aim_dy - 1; break;
                default: dx = aim_dx - 1; dy = aim_dy;    break;
            }
            spawn_bullet(cx, cy, dx, dy);
        }
        // Pattern 2: 5-way spread (every 32 frames)
        if ((pattern & 0x1F) == 0) {
            spawn_bullet(cx, cy, aim_dx, aim_dy);
            spawn_bullet(cx, cy, aim_dx - 1, aim_dy);
            spawn_bullet(cx, cy, aim_dx + 1, aim_dy);
            spawn_bullet(cx, cy, aim_dx - 2, aim_dy);
            spawn_bullet(cx, cy, aim_dx + 2, aim_dy);
        }
        // Pattern 3: Aimed burst (every 48 frames)
        if ((pattern & 0x2F) == 0) {
            spawn_bullet(cx, cy, aim_dx, aim_dy);
            spawn_bullet(cx - 8, cy, aim_dx, aim_dy);
            spawn_bullet(cx + 8, cy, aim_dx, aim_dy);
        }
    } else if (rank == 2) {
        // Rank 2: Moderate intensity
        // Pattern 1: Aimed spiral (every 20 frames - slower)
        if ((pattern & 0x13) == 0) {
            angle = pattern >> 3;
            switch (angle & 0x03) {
                case 0: dx = aim_dx;     dy = aim_dy + 1; break;
                case 1: dx = aim_dx + 1; dy = aim_dy;     break;
                case 2: dx = aim_dx;     dy = aim_dy - 1; break;
                default: dx = aim_dx - 1; dy = aim_dy;    break;
            }
            spawn_bullet(cx, cy, dx, dy);
        }
        // Pattern 2: 3-way spread (every 40 frames)
        if ((pattern & 0x27) == 0) {
            spawn_bullet(cx, cy, aim_dx, aim_dy);
            spawn_bullet(cx, cy, aim_dx - 1, aim_dy);
            spawn_bullet(cx, cy, aim_dx + 1, aim_dy);
        }
    } else {
        // Rank 3: Mild intensity
        // Simple aimed shots (every 24 frames)
        if ((pattern & 0x17) == 0) {
            spawn_bullet(cx, cy, aim_dx, aim_dy);
        }
        // Occasional 2-way (every 48 frames)
        if ((pattern & 0x2F) == 0) {
            spawn_bullet(cx, cy, aim_dx - 1, aim_dy);
            spawn_bullet(cx, cy, aim_dx + 1, aim_dy);
        }
    }
}

// Spawn danmaku pattern from enemies
static void spawn_danmaku(void) {
    unsigned char i, cx, cy;
    signed char dx, dy;
    unsigned char mask;

    ++bullet_timer;

    // Change pattern every 256 frames
    if (bullet_timer == 0) {
        pattern_type = (pattern_type + 1) & 0x07;
    }

    // Burst pattern: 3 bursts then 2 pauses (cycle of ~80 frames)
    // Use counter reset instead of modulo (% 80) for performance
    ++burst_phase;
    if (burst_phase >= 80) burst_phase = 0;

    // When in 1st place: beautiful danmaku from behind
    if (position == 1) {
        // Only fire during burst phase
        if (burst_phase >= 56) return;

        // Wave pattern from bottom - sweeping left to right
        if ((bullet_timer & 0x17) == 0) {
            // Sine-like wave using frame count
            cx = 80 + ((bullet_timer >> 2) & 0x3F);
            dx = calc_aim_dx(cx);
            spawn_bullet(cx, 236, dx, -2);
        }
        if ((bullet_timer & 0x17) == 12) {
            cx = 176 - ((bullet_timer >> 2) & 0x3F);
            dx = calc_aim_dx(cx);
            spawn_bullet(cx, 236, dx, -2);
        }
        // Center aimed shot
        if ((bullet_timer & 0x2F) == 0) {
            cx = ROAD_LEFT + 64;
            dx = calc_aim_dx(cx);
            spawn_bullet(cx, 232, dx, -3);
            spawn_bullet(cx + 32, 232, calc_aim_dx(cx + 32), -3);
        }
        return;
    }

    // Each enemy shoots
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemy_on[i]) continue;
        if (enemy_y[i] < 24 && enemy_y[i] < player_y) continue;

        cx = enemy_x[i] + 8;
        cy = enemy_y[i] + 8;

        // Boss enemies (rank 1-3): special danmaku patterns
        if (enemy_rank[i] <= 3 && !enemy_passed[i]) {
            // Bosses fire continuously with beautiful patterns (no pause phase)
            spawn_boss_danmaku(i, cx, cy);
            continue;
        }

        // Normal enemies: pause phase
        if (burst_phase >= 48) continue;

        dx = calc_aim_dx(cx);
        dy = (enemy_y[i] > player_y) ? -3 : 3;

        mask = 0x17;
        if (loop_count >= 2) mask = 0x0F;
        if (loop_count >= 3) mask = 0x0B;
        if (dy < 0) mask |= 0x10;  // Slower when shooting up

        // Offset timing per enemy
        if (((bullet_timer + (i << 3)) & mask) == 0) {
            spawn_bullet(cx, cy, dx, dy);
        }
    }
}

// Update all bullets (with LOD optimization)
static void update_bullets(void) {
    unsigned char i;
    unsigned char nx, ny;
    unsigned char by;

    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!bullet_on[i]) continue;

        by = bullet_y[i];

        // LOD: Update far bullets (top/bottom of screen) every other frame
        if ((frame_count & 1) && (by < 40 || by > 200)) {
            continue;
        }

        // Move bullet
        nx = bullet_x[i] + bullet_dx[i];
        ny = by + bullet_dy[i];

        // Check bounds
        if (nx < 8 || nx > 248 || ny > 240) {
            bullet_on[i] = 0;
        } else {
            bullet_x[i] = nx;
            bullet_y[i] = ny;
        }
    }
}

// Check bullet collisions with player (optimized single-pass)
// Returns 1 if damage occurred, 0 otherwise
static unsigned char check_bullet_collisions(void) {
    unsigned char i, dx, dy;
    unsigned char player_cx, player_cy;
    unsigned char graze_found = 0;

    if (player_inv > 0) return 0;

    // Precompute player center
    player_cx = player_x + 8;
    player_cy = player_y + 8;

    // Single pass: check damage and record graze
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!bullet_on[i]) continue;

        // Inline abs_diff to avoid function call overhead
        dx = (player_cx >= bullet_x[i]) ? (player_cx - bullet_x[i]) : (bullet_x[i] - player_cx);
        dy = (player_cy >= bullet_y[i]) ? (player_cy - bullet_y[i]) : (bullet_y[i] - player_cy);

        // Damage zone: dx < 4 && dy < 4 (very small hitbox - cockpit only)
        if (dx < 4 && dy < 4) {
            player_inv = 60;
            bullet_on[i] = 0;
            if (score > 0) --score;
            score_multiplier = 1;
            graze_count = 0;
            sfx_damage();
            if (player_hp > 0) --player_hp;
            if (player_hp == 0) do_game_over();
            return 1;
        }

        // Record graze candidate (apply later only if no damage)
        // Only count bullets that haven't been grazed yet
        if (dx < 10 && dy < 10 && !bullet_grazed[i]) {
            bullet_grazed[i] = 1;  // Mark as grazed (one graze per bullet)
            graze_found = 1;
        }
    }

    // Apply graze effect if any NEW grazes found (no damage this frame)
    if (graze_found) {
        add_score(score_multiplier * (1 << loop_count));
        if (score_multiplier < 65535u) ++score_multiplier;
        ++graze_count;
        if (graze_count >= 20) {  // 20 grazes for +1 HP (balanced recovery)
            graze_count = 0;
            if (player_hp < PLAYER_MAX_HP) ++player_hp;
        }
        sfx_graze();
    }

    return 0;
}

// ============================================
// HIGH SCORE FUNCTIONS
// ============================================

// Initialize save data if not valid
static void init_save(void) {
    unsigned char i;
    unsigned char sram_ok = 1;

    // SRAM functionality test
    // Use $6100 (not $6000) to avoid corrupting save_magic which is at $6000
    // Note: Web emulators (jsnes) may not persist SRAM - use FCEUX/Mesen for testing
    *((volatile unsigned char*)0x6100) = 0xAA;
    if (*((volatile unsigned char*)0x6100) != 0xAA) {
        sram_ok = 0;  // SRAM not working
    }

    // Initialize if SRAM not working OR if magic byte is wrong
    if (!sram_ok || save_magic != SAVE_MAGIC) {
        // First run, corrupted save, or SRAM not working - initialize
        save_magic = SAVE_MAGIC;
        for (i = 0; i < NUM_HIGH_SCORES; ++i) {
            high_scores[i] = 0;
            high_scores_high[i] = 0;
            high_names[i][0] = 0;  // A
            high_names[i][1] = 0;  // A
            high_names[i][2] = 0;  // A
        }
        max_loop = 0;  // No loops completed yet
    }

    // Range check for title_select_loop (prevent invalid values)
    if (title_select_loop > max_loop) {
        title_select_loop = 0;
    }
    title_select_loop = 0;  // Default to starting from loop 1
}

// Compare two 32-bit scores: returns 1 if (a_high:a_low) > (b_high:b_low)
static unsigned char score_greater(
    unsigned int a_high, unsigned int a_low,
    unsigned int b_high, unsigned int b_low) {
    if (a_high > b_high) return 1;
    if (a_high < b_high) return 0;
    return a_low > b_low;
}

// Check if score qualifies for high score, return rank (0-2) or 255 if not
static unsigned char check_high_score(unsigned int new_score_high, unsigned int new_score_low) {
    unsigned char i;
    for (i = 0; i < NUM_HIGH_SCORES; ++i) {
        if (score_greater(new_score_high, new_score_low,
                          high_scores_high[i], high_scores[i])) {
            return i;
        }
    }
    return 255;  // Not a high score
}

// Insert a new high score at given rank
static void insert_high_score(unsigned char rank,
                               unsigned int new_score_high,
                               unsigned int new_score_low) {
    unsigned char i;
    // Shift lower scores down
    for (i = NUM_HIGH_SCORES - 1; i > rank; --i) {
        high_scores[i] = high_scores[i - 1];
        high_scores_high[i] = high_scores_high[i - 1];
        high_names[i][0] = high_names[i - 1][0];
        high_names[i][1] = high_names[i - 1][1];
        high_names[i][2] = high_names[i - 1][2];
    }
    // Insert new score
    high_scores[rank] = new_score_low;
    high_scores_high[rank] = new_score_high;
    high_names[rank][0] = entry_name[0];
    high_names[rank][1] = entry_name[1];
    high_names[rank][2] = entry_name[2];
}

// Initialize name entry
static void init_name_entry(unsigned char rank) {
    new_score_rank = rank;
    name_entry_pos = 0;
    name_entry_char = 0;
    entry_name[0] = 0;
    entry_name[1] = 0;
    entry_name[2] = 0;
}

// Handle game over (HP=0) - explosion animation, then check high score
static void do_game_over(void) {
    // Stop any playing SFX
    sfx_stop();
    // Save player position for explosion animation
    explode_x = player_x;
    explode_y = player_y;
    explode_timer = 0;
    game_state = STATE_EXPLODE;
    music_play(3);  // Game over music during explosion
}

// Handle finishing race in 2nd place or lower - show position
static void do_finish_lose(void) {
    sfx_stop();
    explode_timer = 0;
    game_state = STATE_FINISH;
    music_play(3);  // Game over music
}

// Finish game over after animation - check for high score
static void finish_game_over(void) {
    new_score_rank = check_high_score(score_high, score);
    if (new_score_rank < NUM_HIGH_SCORES) {
        // Got a high score! Go to name entry
        game_state = STATE_HIGHSCORE;
        init_name_entry(new_score_rank);
        music_play(4);  // Epilogue music for score entry
    } else {
        // No high score, just game over
        game_state = STATE_GAMEOVER;
        music_stop();  // Silence after retire
    }
}

// Initialize game
static void init_game(void) {
    unsigned char i;

    player_x = PLAYER_START_X;
    player_y = PLAYER_START_Y;
    player_hp = PLAYER_START_HP;
    player_inv = 0;

    for (i = 0; i < MAX_ENEMIES; ++i) enemy_on[i] = 0;
    enemy_slot = 0;
    enemy_warn_timer = 0;
    enemy_next_rank = 11;  // First enemy will be 11th place
    position = 12;  // Start in 12th place (last of 12 cars)
    lap_count = 0;
    loop_count = title_select_loop;  // Start from selected loop
    score = 0;
    score_high = 0;
    distance = 0;
    scroll_y = 0;
    score_multiplier = 1;  // Start with 1x multiplier
    graze_count = 0;
    car_graze_cooldown = 0;
    boost_remaining = 2;   // 2 boosts per loop
    boost_active = 0;
    boss_music_active = 0; // No boss music at start
    explode_timer = 0;

    // Set music intensity based on starting loop
    // Loop 1 LAP1: calm(0), Loop 2+ LAP1: moderate(1)
    if (loop_count > 0) {
        music_intensity = 1;  // Loop 2+: moderate start
    } else {
        music_intensity = 0;  // Loop 1: calm start
    }

    // Clear all bullets
    for (i = 0; i < MAX_BULLETS; ++i) {
        bullet_on[i] = 0;
        bullet_grazed[i] = 0;
    }
    bullet_timer = 0;
    bullet_next = 0;
    pattern_phase = 0;
    pattern_type = 0;

    // Setup PPU like main() does - this order works
    ppu_off();
    load_palettes();
    update_loop_palette();  // Override road/grass colors based on loop_count
    draw_road();

    // Spawn first enemy immediately (no warning delay)
    enemy_next_x = ROAD_LEFT + 8 + (rnd() & 0x7F);
    if (enemy_next_x > ROAD_RIGHT - 24) {
        enemy_next_x = ROAD_RIGHT - 24;
    }
    spawn_enemy();
}

// Update player
static void update_player(void) {
    unsigned char speed = (pad_now & BTN_B) ? 4 : PLAYER_SPEED;

    if (pad_now & BTN_LEFT) {
        if (player_x > ROAD_LEFT) player_x -= speed;
    }
    if (pad_now & BTN_RIGHT) {
        if (player_x < ROAD_RIGHT - 16) player_x += speed;
    }
    if (pad_now & BTN_UP) {
        // Normal area: full speed
        // Slowdown zones near top of screen
        if (player_y > 60) {
            player_y -= speed;
        } else if (player_y > 40) {
            // Zone 1: half speed
            if ((frame_count & 1) == 0) player_y -= speed;
        } else if (player_y > 24) {
            // Zone 2: quarter speed
            if ((frame_count & 3) == 0) player_y -= speed;
        } else if (player_y > 16) {
            // Zone 3: 1/8 speed
            if ((frame_count & 7) == 0) player_y -= speed;
        }
    }
    if (pad_now & BTN_DOWN) {
        if (player_y < SCREEN_HEIGHT - 32) player_y += speed;
    }
    // Note: player_inv is now decremented in update_game() after collision checks
}

// Count enemies ahead of player (not yet passed)
static unsigned char count_enemies_ahead(void) {
    unsigned char i, count = 0;
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i] && !enemy_passed[i]) ++count;
    }
    return count;
}

// Check if there's any boss (rank 1-3) on screen - for music
static unsigned char has_any_boss(void) {
    unsigned char i;
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i] && !enemy_passed[i] && enemy_rank[i] <= 3) {
            return 1;
        }
    }
    return 0;
}

// Check if there's the final boss (rank 1) on screen
// Rank 2 and 3 can appear together, but rank 1 appears alone
static unsigned char has_final_boss(void) {
    unsigned char i;
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i] && !enemy_passed[i] && enemy_rank[i] == 1) {
            return 1;
        }
    }
    return 0;
}

// Update all enemies
static void update_enemy(void) {
    unsigned char i, enemies_ahead;

    // When in 1st place, no enemies spawn
    if (position == 1) {
        for (i = 0; i < MAX_ENEMIES; ++i) enemy_on[i] = 0;
        enemy_warn_timer = 0;
        return;
    }

    // Count enemies ahead - only spawn if fewer than (position - 1)
    // e.g., position 3 means 2 cars ahead, so max 2 non-passed enemies
    enemies_ahead = count_enemies_ahead();

    // Boss spawning rule: rank 2 & 3 can appear together, rank 1 (final boss) appears alone
    // Only block spawning if we're about to spawn rank 1 and rank 1 is already active
    if (enemies_ahead < position - 1 && enemies_ahead < MAX_ENEMIES && enemy_warn_timer == 0) {
        // If next enemy would be the final boss (rank 1), check if already present
        if (position == 2 && has_final_boss()) {
            // Don't spawn another final boss
        } else {
            prepare_enemy();
        }
    }
    if (enemy_warn_timer > 0) {
        --enemy_warn_timer;
        if (enemy_warn_timer == 0) {
            // Re-check condition: only spawn if still needed
            if (count_enemies_ahead() < position - 1) {
                if (position == 2 && has_final_boss()) {
                    // Cancel spawn - final boss already active
                } else {
                    spawn_enemy();
                }
            }
        }
    }

    // Update each enemy
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemy_on[i]) continue;

        // Speed depends on whether enemy is ahead or behind player
        if (enemy_passed[i]) {
            // Behind player: double speed (2 pixels/frame)
            enemy_y[i] += 2;
        } else if (enemy_rank[i] < 3) {
            // Boss enemies (rank 1-3): very slow approach (1 pixel every 4 frames)
            if ((frame_count & 3) == 0) {
                enemy_y[i] += 1;
            }
        } else {
            // Normal enemies ahead: half speed (1 pixel every 2 frames)
            if (frame_count & 1) {
                enemy_y[i] += 1;
            }
        }

        // AI: try to block player (only if ahead, not for bosses)
        // Movement frequency increases with loop: loop0=8frames, loop1=4frames, loop2+=2frames
        {
            unsigned char move_mask;
            switch (loop_count) {
                case 0:  move_mask = 0x07; break;  // Every 8 frames
                case 1:  move_mask = 0x03; break;  // Every 4 frames
                default: move_mask = 0x01; break;  // Every 2 frames
            }
            if (!enemy_passed[i] && enemy_rank[i] >= 3 && (frame_count & move_mask) == 0) {
                if (enemy_x[i] + 8 < player_x && enemy_x[i] < ROAD_RIGHT - 24) {
                    enemy_x[i] += 1;
                } else if (enemy_x[i] > player_x + 8 && enemy_x[i] > ROAD_LEFT + 8) {
                    enemy_x[i] -= 1;
                }
            }
        }

        // Overtaken - award points once
        if (player_y + 16 < enemy_y[i] && !enemy_passed[i]) {
            enemy_passed[i] = 1;
            add_score(20 * score_multiplier);
            if (position > 1) --position;
        }

        // Remove when off screen
        if (enemy_y[i] > 240) {
            enemy_on[i] = 0;
        }
    }
}

// Check collisions with enemy cars
// Returns 1 if damage occurred, 0 otherwise
static unsigned char check_collisions(void) {
    unsigned char i, dx, dy;

    // Decrease car graze cooldown
    if (car_graze_cooldown > 0) --car_graze_cooldown;

    if (player_inv > 0) return 0;

    // Enemy collisions - damage check first
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i]) {
            dx = abs_diff(player_x, enemy_x[i]);
            dy = abs_diff(player_y, enemy_y[i]);

            // Damage zone (smaller hitbox - core collision only)
            if (dx < 10 && dy < 10) {
                player_inv = 60;
                score_multiplier = 1;
                graze_count = 0;
                sfx_damage();
                // Decrement HP with underflow protection
                if (player_hp > 0) {
                    --player_hp;
                }
                if (player_hp == 0) {
                    do_game_over();
                }
                // Return immediately - no graze this frame
                return 1;
            }
        }
    }

    // Graze check only if no damage
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i]) {
            dx = abs_diff(player_x, enemy_x[i]);
            dy = abs_diff(player_y, enemy_y[i]);

            // Graze zone: beside (dx < 18) OR front/back (dy < 24)
            // But not in damage zone
            if (!(dx < 10 && dy < 10) &&
                car_graze_cooldown == 0 &&
                ((dx < 20 && dy < 32) || (dx < 28 && dy < 20))) {
                // Double the multiplier (max 65535)
                if (score_multiplier <= 32767u) {
                    score_multiplier *= 2;
                } else {
                    score_multiplier = 65535u;
                }
                sfx_graze();
                car_graze_cooldown = 30;  // Half second cooldown
            }
        }
    }

    return 0;
}

// Main game update
static void update_game(void) {
    unsigned char took_damage;
    unsigned char boss_now;

    update_player();
    update_enemy();
    took_damage = check_collisions();

    // Check boss state and switch music if needed
    boss_now = has_any_boss();
    if (boss_now && !boss_music_active) {
        // Boss appeared - switch to boss music
        boss_music_active = 1;
        switch (loop_count) {
            case 0:  music_play(TRACK_BOSS1); break;
            case 1:  music_play(TRACK_BOSS2); break;
            default: music_play(TRACK_BOSS3); break;
        }
    } else if (!boss_now && boss_music_active) {
        // Boss defeated/passed - return to racing music
        boss_music_active = 0;
        music_set_intensity(loop_count);
        music_play(TRACK_RACING);
    }

    // Danmaku system
    spawn_danmaku();
    update_bullets();
    // Only check bullet collisions if no damage from enemy collision
    if (!took_damage) {
        check_bullet_collisions();
    }

    // Decrement invincibility AFTER all collision checks
    // This prevents "last frame of inv" vulnerability
    if (player_inv > 0) --player_inv;

    // Low HP warning beep (critical: HP == 1)
    if (player_hp == 1 && sfx_lowhp_timer == 0) {
        // Beep every 20 frames (~0.33 sec) for urgency
        if ((frame_count & 0x13) == 0) {
            sfx_lowhp_timer = 5;  // Short pip duration
        }
    }

    // Update explosion animation
    if (explode_timer > 0) {
        --explode_timer;
        explode_y += (pad_now & BTN_B) ? 4 : SCROLL_SPEED;  // Match road speed
    }

    ++distance;
    if (distance >= LAP_DISTANCE) {  // Lap complete
        distance = 0;
        ++lap_count;

        // Change music intensity gradually
        // Loop 1: LAP1-2=calm(0), LAP3=moderate(1)
        // Loop 2+: LAP1=moderate(1), LAP2-3=intense(2)
        if (loop_count == 0) {
            // First loop: stay calm longer, only LAP3 gets slightly intense
            if (lap_count >= 2) {
                music_set_intensity(1);  // LAP3 only
            } else {
                music_set_intensity(0);  // LAP1-2 calm
            }
        } else {
            // Later loops: moderate start, intense finish
            if (lap_count >= 1) {
                music_set_intensity(2);  // LAP2-3 intense
            } else {
                music_set_intensity(1);  // LAP1 moderate
            }
        }

        if (lap_count >= 3) {
            if (position == 1) {
                // Victory! Advance to next loop (2周目, 3周目...)
                ++loop_count;

                // Save new max loop record if achieved
                if (loop_count > max_loop) {
                    max_loop = loop_count;
                }

                // Big score bonus for completing a loop (with loop multiplier)
                add_score(1000 * (loop_count) * (1 << loop_count));

                // Clear bullets for fresh start
                {
                    unsigned char b;
                    for (b = 0; b < MAX_BULLETS; ++b) {
                        bullet_on[b] = 0;
                    }
                }

                // Go to loop clear celebration screen
                sfx_stop();
                game_state = STATE_LOOP_CLEAR;
                loop_clear_timer = 0;
                music_play(2);  // Victory music
                init_win_animation();  // Reuse confetti
            } else {
                // Finished 3 laps but not in 1st place - show position
                do_finish_lose();
            }
        }
    }

    // Speed boost with B button
    if (pad_now & BTN_B) {
        scroll_y -= 4;  // Fast speed
        ++distance;     // Extra distance for boost
    } else {
        scroll_y -= SCROLL_SPEED;  // Normal speed
    }
}

// Draw game sprites
static void draw_game(void) {
    unsigned char id = 0;
    unsigned char i;

    // Player car (4 sprites) - skip during explosion/finish (drawn separately)
    if (game_state != STATE_EXPLODE && game_state != STATE_FINISH && (player_inv == 0 || (frame_count & 4))) {
        id = set_car(id, player_x, player_y, SPR_CAR, 0);
    }

    // Hitbox indicator (8x8 centered on player center)
    // Hitbox is dx < 4, dy < 4 from center (player_x+8, player_y+8)
    // Draw 8x8 sprite at center - 4 = player_x+4, player_y+4
    if (game_state == STATE_RACING) {
        id = set_sprite(id, player_x + 4, player_y + 4, SPR_HITBOX, 0);
    }

    // Enemy cars (4 sprites each) - color/design based on rank
    // Also show rank number above each enemy car
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i] && enemy_y[i] >= HUD_BAND_BOTTOM) {
            unsigned char tile, pal;
            unsigned char rank = enemy_rank[i];
            unsigned char ex = enemy_x[i];
            unsigned char ey = enemy_y[i];

            // Top 3 positions (rank 1-3): Boss design
            if (rank <= 3) {
                tile = SPR_BOSS;
                pal = 1;  // Red (boss color)
            }
            // Rank 4-5: Strong enemies (palette 3 = blue/white)
            else if (rank <= 5) {
                tile = SPR_ENEMY;
                pal = 3;
            }
            // Rank 6-8: Medium enemies (palette 2 = green)
            else if (rank <= 8) {
                tile = SPR_ENEMY;
                pal = 2;
            }
            // Rank 9-11: Weak enemies (palette 1 = red)
            else {
                tile = SPR_ENEMY;
                pal = 1;
            }

            // Draw rank number FIRST (lower OAM index = appears on top)
            if (id < 60) {
                unsigned char rank_y = ey + 4;  // Center vertically on car
                if (rank >= 10) {
                    // Two digits: "1X" centered
                    id = set_sprite(id, ex + 1, rank_y, SPR_DIGIT + 1, 2);
                    id = set_sprite(id, ex + 9, rank_y, SPR_DIGIT + (rank - 10), 2);
                } else {
                    // Single digit centered on car
                    id = set_sprite(id, ex + 4, rank_y, SPR_DIGIT + rank, 2);
                }
            }

            // Draw car AFTER rank number (so number appears on top)
            id = set_car(id, ex, ey, tile, pal);
        }
    }

    // Explosion effect (1 sprite, blinking) - only during racing
    if (game_state == STATE_RACING && explode_timer > 0 && (frame_count & 2)) {
        id = set_sprite(id, explode_x + 4, explode_y + 4, SPR_EXPLOSION, 1);
    }

    // === Critical HUD (always visible) ===
    // Position is now shown above each enemy car, not in HUD
    // HUD Layout:
    // Row 1 (Y=8):   Lap(center), HP(right)
    // Row 2 (Y=216): Progress indicator (left-center), Multiplier (right)
    // Row 3 (Y=224): Score (right)

    // HUD - HP (3 sprites) - top right: ❤XX
    {
        unsigned char hp = player_hp;
        if (hp > 99) hp = 99;  // Cap display at 99
        id = set_sprite(id, 208, HUD_TOP_Y, SPR_HEART, 1);  // Red heart
        id = set_sprite(id, 216, HUD_TOP_Y, SPR_DIGIT + (hp / 10), 3);
        id = set_sprite(id, 224, HUD_TOP_Y, SPR_DIGIT + (hp % 10), 3);
    }

    // HUD - Multiplier "x" + max 4 digits - bottom right row 1
    // < 10000: x#### (5 sprites), >= 10000: xXXE# scientific (5 sprites)
    {
        unsigned int m = score_multiplier;
        unsigned char mult_x = 200;
        id = set_sprite(id, mult_x, 216, SPR_LETTER + 23, 3);  // x prefix
        mult_x += 8;

        if (m < 10000u) {
            // Normal 4-digit display (white)
            id = set_sprite(id, mult_x, 216, SPR_DIGIT + (m / 1000), 3);
            m %= 1000;
            id = set_sprite(id, mult_x + 8, 216, SPR_DIGIT + (m / 100), 3);
            m %= 100;
            id = set_sprite(id, mult_x + 16, 216, SPR_DIGIT + (m / 10), 3);
            id = set_sprite(id, mult_x + 24, 216, SPR_DIGIT + (m % 10), 3);
        } else {
            // Scientific notation: XXE# (red)
            unsigned char exp = 0;
            while (m >= 100u) {
                m /= 10;
                exp++;
            }
            id = set_sprite(id, mult_x, 216, SPR_DIGIT + (m / 10), 2);
            id = set_sprite(id, mult_x + 8, 216, SPR_DIGIT + (m % 10), 2);
            id = set_sprite(id, mult_x + 16, 216, SPR_LETTER + 4, 2);  // E
            id = set_sprite(id, mult_x + 24, 216, SPR_DIGIT + exp, 2);
        }
    }

    // HUD - Score: bottom right row 2 (max 4 sprites)
    // Small scores: show 4 digits normally
    // Large scores: use scientific notation XXE# (e.g., 12E6 = 12,000,000)
    {
        unsigned char score_x = 208;
        unsigned long full_score = ((unsigned long)score_high << 16) + score;

        if (full_score < 10000u) {
            // Normal 4-digit display (white)
            id = set_sprite(id, score_x, 224, SPR_DIGIT + (full_score / 1000), 3);
            full_score %= 1000;
            id = set_sprite(id, score_x + 8, 224, SPR_DIGIT + (full_score / 100), 3);
            full_score %= 100;
            id = set_sprite(id, score_x + 16, 224, SPR_DIGIT + (full_score / 10), 3);
            id = set_sprite(id, score_x + 24, 224, SPR_DIGIT + (full_score % 10), 3);
        } else {
            // Scientific notation: XXE# format (4 sprites)
            // Find 2-digit mantissa and exponent
            unsigned char exp = 0;
            unsigned char mantissa;
            while (full_score >= 100u) {
                full_score /= 10;
                exp++;
            }
            mantissa = (unsigned char)full_score;

            // Display in yellow (palette 2): MM E X
            id = set_sprite(id, score_x, 224, SPR_DIGIT + (mantissa / 10), 2);
            id = set_sprite(id, score_x + 8, 224, SPR_DIGIT + (mantissa % 10), 2);
            id = set_sprite(id, score_x + 16, 224, SPR_LETTER + 4, 2);  // E
            id = set_sprite(id, score_x + 24, 224, SPR_DIGIT + exp, 2);
        }
    }

    // Loop counter at center-top (shown when in 2nd loop or higher)
    if (loop_count > 0) {
        id = set_sprite(id, 120, HUD_TOP_Y, SPR_LETTER + 11, 3);  // L
        id = set_sprite(id, 128, HUD_TOP_Y, SPR_DIGIT + loop_count + 1, 2);  // Loop number in yellow
    }

    // === Game objects (may be culled if too many) ===

    // Warning marker for next enemy (single up arrow)
    // Position at Y=24 (between HUD top and game area)
    if (enemy_warn_timer > 0 && (frame_count & 8)) {
        id = set_sprite(id, enemy_next_x + 4, 24, 0x0A, 1);  // red (danger)
    }

    // Bullets - danmaku (use remaining sprite slots)
    // Flicker rendering: only draw half the bullets per frame to reduce sprite overflow
    // Even frames draw even-indexed bullets, odd frames draw odd-indexed bullets
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (bullet_on[i] && id < 62 && ((i & 1) == (frame_count & 1))) {
            unsigned char by = bullet_y[i];
            if (by >= HUD_BAND_BOTTOM) {
                id = set_sprite(id, bullet_x[i], by, SPR_BULLET, 2);
            }
        }
    }

    // HUD - Vertical progress indicator on left side
    // Shows total progress across all 3 laps (bottom to top)
    // Y range: 200 (bottom) to 32 (top) = 168 pixels
    // Total distance: 3 * 700 = 2100
    {
        unsigned int total_progress;
        unsigned char car_y;
        unsigned char marker_y1, marker_y2;

        // Calculate total progress across all laps
        total_progress = (unsigned int)lap_count * LAP_DISTANCE + distance;
        if (total_progress > 2100u) total_progress = 2100u;

        // Map to Y position: 200 - (progress * 168 / 2100)
        // Simplified: 200 - (progress / 12.5) ≈ 200 - (progress * 2 / 25)
        car_y = 200 - (unsigned char)((total_progress * 2u) / 25u);
        if (car_y < 32) car_y = 32;

        // Draw car icon at current progress position
        id = set_sprite(id, 8, car_y, SPR_CAR_ICON, 0);

        // Draw GOAL marker at top
        id = set_sprite(id, 8, 24, SPR_LETTER + 6, 3);  // G (goal)

        // Draw START marker at bottom
        id = set_sprite(id, 8, 208, SPR_LETTER + 18, 3);  // S (start)

        // Draw lap boundary markers (1/3 and 2/3 of the way)
        // Lap 1 boundary: Y = 200 - 56 = 144
        // Lap 2 boundary: Y = 200 - 112 = 88
        marker_y1 = 144;  // End of lap 1
        marker_y2 = 88;   // End of lap 2

        // Show markers as small horizontal lines
        id = set_sprite(id, 4, marker_y1, SPR_HLINE, 3);
        id = set_sprite(id, 4, marker_y2, SPR_HLINE, 3);
    }

    // Hide remaining sprites
    while (id < 64) {
        OAM[id * 4] = 0xFF;
        ++id;
    }
}

// Draw title screen
static void draw_title(void) {
    unsigned char id = 0;
    unsigned char x = 96, y = 40;
    unsigned char i;
    unsigned int s;

    // Version "V5.0" at top-right (4 sprites)
    id = set_sprite(id, 216, 8, SPR_LETTER + 21, 3);  // V
    id = set_sprite(id, 224, 8, SPR_DIGIT + 5, 3);    // 5
    id = set_sprite(id, 232, 8, SPR_DOT, 3);          // .
    id = set_sprite(id, 240, 8, SPR_DIGIT + 0, 3);    // 0

    // "EDGE" - top line (blue, player color)
    id = set_sprite(id, x,      y, SPR_LETTER + 4,  0);  // E
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 3,  0);  // D
    id = set_sprite(id, x + 16, y, SPR_LETTER + 6,  0);  // G
    id = set_sprite(id, x + 24, y, SPR_LETTER + 4,  0);  // E

    // "RACE" - bottom line, offset right (red)
    id = set_sprite(id, x + 8,  y + 12, SPR_LETTER + 17, 1);  // R
    id = set_sprite(id, x + 16, y + 12, SPR_LETTER + 0,  1);  // A
    id = set_sprite(id, x + 24, y + 12, SPR_LETTER + 2,  1);  // C
    id = set_sprite(id, x + 32, y + 12, SPR_LETTER + 4,  1);  // E

    // High scores display (2 lines per entry to avoid 8-sprite limit)
    // Line 1: Rank + Name (4 sprites, 40px wide, centered)
    // Line 2: Score (6 sprites, 48px wide, centered)
    for (i = 0; i < NUM_HIGH_SCORES; ++i) {
        unsigned char y_base = 110 + i * 20;  // 110, 130, 150

        // Line 1: Rank + Name (4 sprites) - centered at X=108
        y = y_base;
        id = set_sprite(id, 108, y, SPR_DIGIT + i + 1, 3);  // Rank
        id = set_sprite(id, 124, y, SPR_LETTER + high_names[i][0], 3);
        id = set_sprite(id, 132, y, SPR_LETTER + high_names[i][1], 3);
        id = set_sprite(id, 140, y, SPR_LETTER + high_names[i][2], 3);

        // Line 2: Score (5-6 sprites) - centered at X=104
        y = y_base + 10;
        x = 104;
        {
            unsigned long full_score = ((unsigned long)high_scores_high[i] << 16) | high_scores[i];
            if (full_score >= 1000000UL) {
                // Large score: use scientific notation (XXXE# format, 5 sprites)
                unsigned char exp = 0;
                unsigned int mantissa;
                while (full_score >= 1000UL) {
                    full_score /= 10;
                    exp++;
                }
                mantissa = (unsigned int)full_score;
                // Display: XXX E X (5 sprites, centered)
                x = 108;
                id = set_sprite(id, x,      y, SPR_DIGIT + (unsigned char)(mantissa / 100), 3);
                id = set_sprite(id, x + 8,  y, SPR_DIGIT + (unsigned char)((mantissa / 10) % 10), 3);
                id = set_sprite(id, x + 16, y, SPR_DIGIT + (unsigned char)(mantissa % 10), 3);
                id = set_sprite(id, x + 24, y, SPR_LETTER + 4, 2);  // E (yellow)
                id = set_sprite(id, x + 32, y, SPR_DIGIT + exp, 2); // exponent (yellow)
            } else if (full_score >= 100000UL) {
                // 6-digit score: display normally
                id = set_sprite(id, x,      y, SPR_DIGIT + (unsigned char)(full_score / 100000UL), 3);
                id = set_sprite(id, x + 8,  y, SPR_DIGIT + (unsigned char)((full_score / 10000UL) % 10), 3);
                id = set_sprite(id, x + 16, y, SPR_DIGIT + (unsigned char)((full_score / 1000UL) % 10), 3);
                id = set_sprite(id, x + 24, y, SPR_DIGIT + (unsigned char)((full_score / 100UL) % 10), 3);
                id = set_sprite(id, x + 32, y, SPR_DIGIT + (unsigned char)((full_score / 10UL) % 10), 3);
                id = set_sprite(id, x + 40, y, SPR_DIGIT + (unsigned char)(full_score % 10), 3);
            } else {
                // Smaller score: display with leading zeros
                s = (unsigned int)full_score;
                id = set_sprite(id, x,      y, SPR_DIGIT + 0, 3);
                id = set_sprite(id, x + 8,  y, SPR_DIGIT + (unsigned char)(s / 10000), 3);
                id = set_sprite(id, x + 16, y, SPR_DIGIT + (unsigned char)((s / 1000) % 10), 3);
                id = set_sprite(id, x + 24, y, SPR_DIGIT + (unsigned char)((s / 100) % 10), 3);
                id = set_sprite(id, x + 32, y, SPR_DIGIT + (unsigned char)((s / 10) % 10), 3);
                id = set_sprite(id, x + 40, y, SPR_DIGIT + (unsigned char)(s % 10), 3);
            }
        }
    }

    // Loop selection (only show if player has completed at least 1 loop)
    // Positioned above high scores, centered
    if (max_loop > 0) {
        y = 84;
        x = 100;  // Centered: "LOOP X" is ~48px, start at 104
        // "LOOP" label
        id = set_sprite(id, x,      y, SPR_LETTER + 11, 3);  // L
        id = set_sprite(id, x + 8,  y, SPR_LETTER + 14, 3);  // O
        id = set_sprite(id, x + 16, y, SPR_LETTER + 14, 3);  // O
        id = set_sprite(id, x + 24, y, SPR_LETTER + 15, 3);  // P

        // Loop number (blinking if selectable)
        x = 136;
        if (frame_count & 0x10) {
            id = set_sprite(id, x, y, SPR_DIGIT + title_select_loop + 1,
                           title_select_loop > 0 ? 1 : 3);  // Red if loop 2+
        }

        // Up/Down arrows if more than 1 option
        if (max_loop > 0) {
            id = set_sprite(id, x + 12, y - 6, 0x0A, 3);        // Up arrow
            id = set_sprite(id, x + 12, y + 6, 0x0A, 3 | 0x80); // Down arrow (flipped)
        }
    }

    // "START" prompt (blinking) with car icon - centered
    if (frame_count & 0x20) {
        y = 188;
        x = 108;  // Centered: 128 - 20 = 108
        // Car icon to the left of START
        id = set_car(id, x - 24, y - 4, SPR_CAR, 0);
        // START text
        id = set_sprite(id, x,      y, SPR_LETTER + 18, 2);  // S
        id = set_sprite(id, x + 8,  y, SPR_LETTER + 19, 2);  // T
        id = set_sprite(id, x + 16, y, SPR_LETTER + 0,  2);  // A
        id = set_sprite(id, x + 24, y, SPR_LETTER + 17, 2);  // R
        id = set_sprite(id, x + 32, y, SPR_LETTER + 19, 2);  // T
    }

    // "2026 FUBA" at bottom-center (no copyright symbol)
    y = 216;
    x = 88;  // Centered: 128 - 40 = 88
    id = set_sprite(id, x,      y, SPR_DIGIT + 2, 3);    // 2
    id = set_sprite(id, x + 8,  y, SPR_DIGIT + 0, 3);    // 0
    id = set_sprite(id, x + 16, y, SPR_DIGIT + 2, 3);    // 2
    id = set_sprite(id, x + 24, y, SPR_DIGIT + 6, 3);    // 6
    id = set_sprite(id, x + 40, y, SPR_LETTER + 5, 3);   // F
    id = set_sprite(id, x + 48, y, SPR_LETTER + 20, 3);  // U
    id = set_sprite(id, x + 56, y, SPR_LETTER + 1, 3);   // B
    id = set_sprite(id, x + 64, y, SPR_LETTER + 0, 3);   // A

    // Hide rest
    while (id < 64) {
        OAM[id * 4] = 0xFF;
        ++id;
    }
}

// Draw game over screen
static void draw_gameover(void) {
    unsigned char id = 0;
    unsigned char x = 88, y = 100;

    // "GAME" (red)
    id = set_sprite(id, x,      y, SPR_LETTER + 6,  1);  // G
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 0,  1);  // A
    id = set_sprite(id, x + 16, y, SPR_LETTER + 12, 1);  // M
    id = set_sprite(id, x + 24, y, SPR_LETTER + 4,  1);  // E

    // "OVER" (red)
    x = 96; y = 116;
    id = set_sprite(id, x,      y, SPR_LETTER + 14, 1);  // O
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 21, 1);  // V
    id = set_sprite(id, x + 16, y, SPR_LETTER + 4,  1);  // E
    id = set_sprite(id, x + 24, y, SPR_LETTER + 17, 1);  // R

    // Hide rest
    while (id < 64) {
        OAM[id * 4] = 0xFF;
        ++id;
    }
}

// Initialize win animation
static void init_win_animation(void) {
    unsigned char i;
    win_timer = 0;

    // Initialize confetti particles
    for (i = 0; i < MAX_CONFETTI; ++i) {
        confetti_x[i] = 32 + (rnd() & 0x7F) + (rnd() & 0x3F);  // Spread across screen
        confetti_y[i] = rnd() & 0x1F;  // Start near top
        confetti_color[i] = rnd() & 0x03;  // Random palette
    }
}

// Update win animation
static void update_win_animation(void) {
    unsigned char i;

    ++win_timer;

    // Update confetti falling
    for (i = 0; i < MAX_CONFETTI; ++i) {
        confetti_y[i] += 1 + (i & 1);  // Different speeds

        // Wiggle horizontally
        if (frame_count & 2) {
            if (i & 1) {
                confetti_x[i] += 1;
            } else {
                confetti_x[i] -= 1;
            }
        }

        // Reset when off screen
        if (confetti_y[i] > 240) {
            confetti_y[i] = 0;
            confetti_x[i] = 32 + (rnd() & 0x7F) + (rnd() & 0x3F);
        }
    }
}

// Draw high score name entry screen
static void draw_highscore_entry(void) {
    unsigned char id = 0;
    unsigned char x, y, i;
    unsigned int s;

    // "NEW HIGH SCORE!" (split to 2 lines for sprite limit)
    x = 72;
    y = 40;
    id = set_sprite(id, x,      y, SPR_LETTER + 13, 0);  // N
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 4,  0);  // E
    id = set_sprite(id, x + 16, y, SPR_LETTER + 22, 0);  // W

    x = 68;
    y = 56;
    id = set_sprite(id, x,      y, SPR_LETTER + 7,  0);  // H
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 8,  0);  // I
    id = set_sprite(id, x + 16, y, SPR_LETTER + 6,  0);  // G
    id = set_sprite(id, x + 24, y, SPR_LETTER + 7,  0);  // H

    // Score value
    x = 96;
    y = 80;
    s = score;
    if (s > 99999) s = 99999;
    id = set_sprite(id, x,      y, SPR_DIGIT + (unsigned char)(s / 10000), 3);
    id = set_sprite(id, x + 8,  y, SPR_DIGIT + (unsigned char)((s / 1000) % 10), 3);
    id = set_sprite(id, x + 16, y, SPR_DIGIT + (unsigned char)((s / 100) % 10), 3);
    id = set_sprite(id, x + 24, y, SPR_DIGIT + (unsigned char)((s / 10) % 10), 3);
    id = set_sprite(id, x + 32, y, SPR_DIGIT + (unsigned char)(s % 10), 3);

    // "ENTER NAME" (on 2 lines)
    x = 88;
    y = 110;
    id = set_sprite(id, x,      y, SPR_LETTER + 13, 3);  // N
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 0,  3);  // A
    id = set_sprite(id, x + 16, y, SPR_LETTER + 12, 3);  // M
    id = set_sprite(id, x + 24, y, SPR_LETTER + 4,  3);  // E

    // Name entry display (3 letters)
    x = 104;
    y = 140;
    for (i = 0; i < 3; ++i) {
        // Draw letter
        id = set_sprite(id, x + i * 16, y, SPR_LETTER + entry_name[i],
                       (i == name_entry_pos) ? 0 : 3);

        // Draw cursor under current position (blinking, yellow)
        if (i == name_entry_pos && (frame_count & 0x10)) {
            id = set_sprite(id, x + i * 16, y + 10, SPR_BAR_FILL, 2);
        }
    }

    // Up/Down arrows hint
    x = 80;
    y = 140;
    id = set_sprite(id, x, y - 4, 0x0A, 0);  // Up arrow (rotated)
    id = set_sprite(id, x, y + 8, 0x0A, 0 | 0x80);  // Down arrow

    // "A=OK" hint
    y = 190;
    x = 104;
    id = set_sprite(id, x,      y, SPR_LETTER + 0, 3);   // A
    id = set_sprite(id, x + 12, y, SPR_LETTER + 14, 3);  // O
    id = set_sprite(id, x + 20, y, SPR_LETTER + 10, 3);  // K

    // Hide remaining sprites
    while (id < 64) {
        OAM[id * 4] = 0xFF;
        ++id;
    }
}

// Draw win screen with celebration animation
static void draw_win(void) {
    unsigned char id = 0;
    unsigned char x, y;
    unsigned int s;
    unsigned char i;
    unsigned char text_y;
    unsigned char bounce;

    // Calculate bounce effect for text
    bounce = 0;
    if (win_timer < 60) {
        // Initial bounce animation
        bounce = (win_timer >> 2) & 0x03;
        if (bounce > 2) bounce = 4 - bounce;
    }

    // === CONFETTI ===
    for (i = 0; i < MAX_CONFETTI && id < 20; ++i) {
        // Use different bullet sprites for confetti
        id = set_sprite(id, confetti_x[i], confetti_y[i],
                       SPR_BULLET + (i & 1), confetti_color[i]);
    }

    // === "FINISH!" text with bounce ===
    text_y = 60 - bounce;
    x = 84;

    id = set_sprite(id, x,      text_y, SPR_LETTER + 5,  0);  // F
    id = set_sprite(id, x + 8,  text_y, SPR_LETTER + 8,  0);  // I
    id = set_sprite(id, x + 16, text_y, SPR_LETTER + 13, 0);  // N
    id = set_sprite(id, x + 24, text_y, SPR_LETTER + 8,  0);  // I
    id = set_sprite(id, x + 32, text_y, SPR_LETTER + 18, 0);  // S
    id = set_sprite(id, x + 40, text_y, SPR_LETTER + 7,  0);  // H

    // === "1ST PLACE" ===
    y = 90;
    x = 88;
    id = set_sprite(id, x,      y, SPR_DIGIT + 1, 3);         // 1
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 18, 3);       // S
    id = set_sprite(id, x + 16, y, SPR_LETTER + 19, 3);       // T

    x = 116;
    id = set_sprite(id, x,      y, SPR_LETTER + 15, 3);       // P
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 11, 3);       // L
    id = set_sprite(id, x + 16, y, SPR_LETTER + 0,  3);       // A
    id = set_sprite(id, x + 24, y, SPR_LETTER + 2,  3);       // C
    id = set_sprite(id, x + 32, y, SPR_LETTER + 4,  3);       // E

    // === Animated player car (spinning/celebrating) ===
    y = 115;
    x = 112;
    if (frame_count & 0x10) {
        // Car facing forward
        id = set_car(id, x, y, SPR_CAR, 0);
    } else {
        // Car tilted (using different attributes for flip effect)
        id = set_car(id, x, y, SPR_CAR, 0 | 0x40);  // H-flip
    }

    // === SCORE display ===
    y = 150;
    x = 88;
    id = set_sprite(id, x,      y, SPR_LETTER + 18, 3);  // S
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 2,  3);  // C
    id = set_sprite(id, x + 16, y, SPR_LETTER + 14, 3);  // O
    id = set_sprite(id, x + 24, y, SPR_LETTER + 17, 3);  // R
    id = set_sprite(id, x + 32, y, SPR_LETTER + 4,  3);  // E

    // Score value (5 digits)
    y = 165;
    x = 80;
    s = score;
    if (s > 99999) s = 99999;  // Cap at 5 digits
    id = set_sprite(id, x,      y, SPR_DIGIT + (unsigned char)(s / 10000), 3);
    id = set_sprite(id, x + 8,  y, SPR_DIGIT + (unsigned char)((s / 1000) % 10), 3);
    id = set_sprite(id, x + 16, y, SPR_DIGIT + (unsigned char)((s / 100) % 10), 3);
    id = set_sprite(id, x + 24, y, SPR_DIGIT + (unsigned char)((s / 10) % 10), 3);
    id = set_sprite(id, x + 32, y, SPR_DIGIT + (unsigned char)(s % 10), 3);
    id = set_sprite(id, x + 44, y, SPR_LETTER + 15, 3);  // P
    id = set_sprite(id, x + 52, y, SPR_LETTER + 19, 3);  // T (PTS)
    id = set_sprite(id, x + 60, y, SPR_LETTER + 18, 3);  // S

    // === "PRESS" / "START" blinking prompt (2 lines to avoid sprite limit) ===
    if (win_timer > 90 && (frame_count & 0x20)) {
        x = 96;
        y = 192;
        id = set_sprite(id, x,      y, SPR_LETTER + 15, 3);  // P
        id = set_sprite(id, x + 8,  y, SPR_LETTER + 17, 3);  // R
        id = set_sprite(id, x + 16, y, SPR_LETTER + 4,  3);  // E
        id = set_sprite(id, x + 24, y, SPR_LETTER + 18, 3);  // S
        id = set_sprite(id, x + 32, y, SPR_LETTER + 18, 3);  // S

        y = 204;
        id = set_sprite(id, x,      y, SPR_LETTER + 18, 3);  // S
        id = set_sprite(id, x + 8,  y, SPR_LETTER + 19, 3);  // T
        id = set_sprite(id, x + 16, y, SPR_LETTER + 0,  3);  // A
        id = set_sprite(id, x + 24, y, SPR_LETTER + 17, 3);  // R
        id = set_sprite(id, x + 32, y, SPR_LETTER + 19, 3);  // T
    }

    // Hide remaining sprites
    while (id < 64) {
        OAM[id * 4] = 0xFF;
        ++id;
    }
}

// Draw loop clear celebration screen (simplified to save ROM)
static void draw_loop_clear(void) {
    unsigned char id = 0;
    unsigned char i;

    // Confetti
    for (i = 0; i < MAX_CONFETTI && id < 16; ++i) {
        id = set_sprite(id, confetti_x[i], confetti_y[i], SPR_BULLET, confetti_color[i]);
    }

    // "LOOP" "X" (Y=60)
    id = set_sprite(id, 76,  60, SPR_LETTER + 11, 0);  // L
    id = set_sprite(id, 84,  60, SPR_LETTER + 14, 0);  // O
    id = set_sprite(id, 92,  60, SPR_LETTER + 14, 0);  // O
    id = set_sprite(id, 100, 60, SPR_LETTER + 15, 0);  // P
    id = set_sprite(id, 112, 60, SPR_DIGIT + loop_count, 2);  // yellow

    // "OK" (Y=80)
    id = set_sprite(id, 100, 80, SPR_LETTER + 14, 0);  // O
    id = set_sprite(id, 108, 80, SPR_LETTER + 10, 0);  // K

    // Car
    id = set_car(id, 112, 100, SPR_CAR, (frame_count & 0x10) ? 0 : 0x40);

    // "START" blinking (after 60 frames)
    if (loop_clear_timer > 60 && (frame_count & 0x20)) {
        id = set_sprite(id, 88, 180, SPR_LETTER + 18, 3);   // S
        id = set_sprite(id, 96, 180, SPR_LETTER + 19, 3);   // T
        id = set_sprite(id, 104, 180, SPR_LETTER + 0, 3);   // A
        id = set_sprite(id, 112, 180, SPR_LETTER + 17, 3);  // R
        id = set_sprite(id, 120, 180, SPR_LETTER + 19, 3);  // T
    }

    // Hide rest
    while (id < 64) {
        OAM[id * 4] = 0xFF;
        ++id;
    }
}

// Draw pause screen
static void draw_pause(void) {
    unsigned char id = 0;
    unsigned char x = 92, y = 100;

    // Draw game in background first
    draw_game();

    // "PAUSE" text overlay (blinking)
    if (frame_count & 0x10) {
        id = set_sprite(id, x,      y, SPR_LETTER + 15, 3);  // P
        id = set_sprite(id, x + 8,  y, SPR_LETTER + 0,  3);  // A
        id = set_sprite(id, x + 16, y, SPR_LETTER + 20, 3);  // U
        id = set_sprite(id, x + 24, y, SPR_LETTER + 18, 3);  // S
        id = set_sprite(id, x + 32, y, SPR_LETTER + 4,  3);  // E
    }
}

// Main entry point
void main(void) {
    // Initialize
    rnd_seed = 42;
    game_state = STATE_TITLE;

    // Initialize battery-backed save data
    init_save();

    // Wait for PPU to stabilize
    wait_vblank();
    wait_vblank();

    // Disable rendering during setup
    ppu_off();

    // Initialize audio
    init_apu();
    music_play(0);  // Title BGM

    // Load palettes
    load_palettes();

    // Draw initial road (then clear center line for title screen)
    draw_road();
    clear_center_line();

    // Enable NMI and rendering
    PPU_CTRL = 0x88;  // NMI on, sprites at $1000
    nmi_enabled = 1;  // Allow wait_vblank to use NMI flag
    ppu_on();

    // Main loop
    while (1) {
        // Read controller first (before vblank for responsive input)
        pad_old = pad_now;
        pad_now = read_pad();
        pad_new = pad_now & ~pad_old;

        ++frame_count;
        rnd_seed ^= frame_count;

        // Clear sprites and build OAM buffer BEFORE vblank
        clear_sprites();

        // Music is updated in NMI handler for stable timing
        // (no music_update call here)

        // Update sound effects
        update_sfx();

        // State machine
        switch (game_state) {
            case STATE_TITLE:
                draw_title();
                // Loop selection with Up/Down (only if player has unlocked loops)
                if (max_loop > 0) {
                    if (pad_new & BTN_UP) {
                        if (title_select_loop < max_loop) {
                            ++title_select_loop;
                        }
                    }
                    if (pad_new & BTN_DOWN) {
                        if (title_select_loop > 0) {
                            --title_select_loop;
                        }
                    }
                }
                if (pad_new & BTN_START) {
                    init_game();
                    music_play(TRACK_RACING);  // Racing BGM - energetic!
                    game_state = STATE_RACING;
                }
                break;

            case STATE_RACING:
                if (pad_new & BTN_START) {
                    game_state = STATE_PAUSED;
                    music_pause();
                    sfx_stop();
                } else {
                    update_game();
                    // Only draw game if still racing (not transitioned to WIN/GAMEOVER)
                    if (game_state == STATE_RACING) {
                        draw_game();
                    }
                }
                break;

            case STATE_PAUSED:
                draw_pause();
                if (pad_new & BTN_START) {
                    game_state = STATE_RACING;
                    music_resume();
                }
                break;

            case STATE_EXPLODE:
                // Show explosion animation (HP=0 death)
                ++explode_timer;
                {
                    unsigned char id = 0;
                    unsigned char ex, ey;
                    unsigned char phase = explode_timer >> 3;  // Every 8 frames

                    // Center explosion
                    id = set_sprite(id, explode_x + 4, explode_y + 4, SPR_EXPLOSION, 2);

                    // Expanding explosions around center
                    if (phase >= 1) {
                        ex = explode_x - 4;
                        ey = explode_y - 4;
                        id = set_sprite(id, ex, ey, SPR_EXPLOSION, 2);
                        id = set_sprite(id, ex + 16, ey, SPR_EXPLOSION, 2);
                    }
                    if (phase >= 2) {
                        ex = explode_x - 8;
                        ey = explode_y + 4;
                        id = set_sprite(id, ex, ey, SPR_EXPLOSION, 2);
                        id = set_sprite(id, ex + 24, ey, SPR_EXPLOSION, 2);
                    }
                    if (phase >= 3) {
                        ey = explode_y + 12;
                        id = set_sprite(id, explode_x - 4, ey, SPR_EXPLOSION, 2);
                        id = set_sprite(id, explode_x + 12, ey, SPR_EXPLOSION, 2);
                    }

                    // Hide remaining sprites
                    while (id < 64) {
                        OAM[id * 4] = 0xFF;
                        ++id;
                    }
                }
                // After ~1 second, move to next state
                if (explode_timer > 60) {
                    finish_game_over();
                }
                break;

            case STATE_FINISH:
                // Show finishing position (2nd place or lower)
                ++explode_timer;
                {
                    unsigned char id = 0;
                    unsigned char x, y;

                    // Draw player car (stopped)
                    id = set_sprite(id, player_x,     player_y,     SPR_CAR,     0);
                    id = set_sprite(id, player_x + 8, player_y,     SPR_CAR + 1, 0);
                    id = set_sprite(id, player_x,     player_y + 8, SPR_CAR + 2, 0);
                    id = set_sprite(id, player_x + 8, player_y + 8, SPR_CAR + 3, 0);

                    // Show position (e.g., "2ND" or "3RD") in yellow
                    x = 96;
                    y = 100;
                    id = set_sprite(id, x, y, SPR_DIGIT + position, 2);
                    if (position == 2) {
                        id = set_sprite(id, x + 8,  y, SPR_LETTER + 13, 2);  // N
                        id = set_sprite(id, x + 16, y, SPR_LETTER + 3,  2);  // D
                    } else if (position == 3) {
                        id = set_sprite(id, x + 8,  y, SPR_LETTER + 17, 2);  // R
                        id = set_sprite(id, x + 16, y, SPR_LETTER + 3,  2);  // D
                    } else {
                        id = set_sprite(id, x + 8,  y, SPR_LETTER + 19, 2);  // T
                        id = set_sprite(id, x + 16, y, SPR_LETTER + 7,  2);  // H
                    }

                    // "PLACE" below (yellow)
                    x = 88;
                    y = 116;
                    id = set_sprite(id, x,      y, SPR_LETTER + 15, 2);  // P
                    id = set_sprite(id, x + 8,  y, SPR_LETTER + 11, 2);  // L
                    id = set_sprite(id, x + 16, y, SPR_LETTER + 0,  2);  // A
                    id = set_sprite(id, x + 24, y, SPR_LETTER + 2,  2);  // C
                    id = set_sprite(id, x + 32, y, SPR_LETTER + 4,  2);  // E

                    // Hide remaining sprites
                    while (id < 64) {
                        OAM[id * 4] = 0xFF;
                        ++id;
                    }
                }
                // After ~1.5 seconds, move to next state
                if (explode_timer > 90) {
                    finish_game_over();
                }
                break;

            case STATE_GAMEOVER:
                draw_gameover();
                if (pad_new & BTN_START) {
                    music_play(0);  // Back to title BGM
                    clear_center_line();  // Remove dotted line for title
                    game_state = STATE_TITLE;
                }
                break;

            case STATE_WIN:
                update_win_animation();
                draw_win();
                // Only accept START after animation plays (about 1.5 seconds)
                if (win_timer > 90 && (pad_new & BTN_START)) {
                    music_play(0);  // Back to title BGM
                    clear_center_line();  // Remove dotted line for title
                    game_state = STATE_TITLE;
                }
                break;

            case STATE_LOOP_CLEAR:
                ++loop_clear_timer;
                update_win_animation();  // Reuse confetti animation
                draw_loop_clear();
                // Accept START after 60 frames
                if (loop_clear_timer > 60 && (pad_new & BTN_START)) {
                    // Start next loop
                    lap_count = 0;
                    position = 12;  // Start from the back again
                    distance = 0;
                    scroll_y = 0;
                    boost_remaining = 2;  // Reset boosts for new loop
                    boost_active = 0;
                    boss_music_active = 0; // No boss at loop start
                    enemy_next_rank = 11;  // Reset enemy ranks for new loop
                    {
                        unsigned char j;
                        for (j = 0; j < MAX_ENEMIES; ++j) enemy_on[j] = 0;
                    }

                    // Setup PPU like main() does
                    ppu_off();
                    load_palettes();
                    update_loop_palette();  // Override road/grass colors based on loop_count
                    draw_road();

                    // Resume racing BGM with moderate intensity for LAP 1
                    music_play(TRACK_RACING);
                    music_set_intensity(1);  // Loop 2+: moderate start

                    game_state = STATE_RACING;
                }
                break;

            case STATE_HIGHSCORE:
                draw_highscore_entry();
                // Up/Down to change letter
                if (pad_new & BTN_UP) {
                    if (name_entry_char < 25) {
                        ++name_entry_char;
                    } else {
                        name_entry_char = 0;  // Wrap around
                    }
                    entry_name[name_entry_pos] = name_entry_char;
                }
                if (pad_new & BTN_DOWN) {
                    if (name_entry_char > 0) {
                        --name_entry_char;
                    } else {
                        name_entry_char = 25;  // Wrap around
                    }
                    entry_name[name_entry_pos] = name_entry_char;
                }
                // A button to confirm letter
                if (pad_new & BTN_A) {
                    ++name_entry_pos;
                    if (name_entry_pos >= 3) {
                        // Done entering name - save score (32-bit)
                        insert_high_score(new_score_rank, score_high, score);
                        game_state = STATE_GAMEOVER;
                    } else {
                        // Move to next letter
                        name_entry_char = entry_name[name_entry_pos];
                    }
                }
                // START button to finish name entry immediately
                if (pad_new & BTN_START) {
                    insert_high_score(new_score_rank, score_high, score);
                    game_state = STATE_GAMEOVER;
                }
                break;
        }

        // Wait for vblank AFTER building OAM buffer
        wait_vblank();

        // OAM DMA - now transfers current frame's sprites
        OAM_ADDR = 0;
        OAM_DMA = 0x02;

        // Set scroll
        PPU_SCROLL = 0;
        PPU_SCROLL = scroll_y;
    }
}
