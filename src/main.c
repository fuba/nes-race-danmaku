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
#define STATE_EXPLODE    7  // Player explosion before game over

// Screen constants
#define ROAD_LEFT       64
#define ROAD_RIGHT      192
#define SCREEN_HEIGHT   240

// Player constants
#define PLAYER_START_X  120
#define PLAYER_START_Y  200
#define PLAYER_SPEED    2
#define PLAYER_MAX_HP   5

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
#define SPR_SLASH       0x05
#define SPR_BAR_FILL    0x06
#define SPR_BAR_EMPTY   0x07
#define SPR_CAR_ICON    0x08
#define SPR_OBSTACLE    0x09
#define SPR_EXPLOSION   0x0E
#define SPR_BULLET      0x0B  // Diamond bullet sprite
#define SPR_DIGIT       0x10
#define SPR_LETTER      0x30

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
static unsigned char enemy_next_x;     // Next enemy spawn X position
static unsigned char enemy_warn_timer; // Warning countdown before spawn
static unsigned char enemy_slot;       // Next enemy slot to use

// Explosion effect
static unsigned char explode_x;
static unsigned char explode_y;
static unsigned char explode_timer;    // Explosion animation timer

// Goal line effect
static unsigned char goal_line_y;      // Y position of goal line (scrolls down)
static unsigned char goal_line_timer;  // Timer for goal line animation

static unsigned char position;
static unsigned char lap_count;
static unsigned char loop_count;  // Current loop (2周目, 3周目...)
static unsigned int score;
static unsigned int distance;
static unsigned char score_multiplier;  // Score multiplier (1-9)
static unsigned int no_damage_timer;    // Frames since last damage
static unsigned char graze_timer;       // Timer for showing "BUZ" display

static unsigned char obs_x[4];
static unsigned char obs_y[4];
static unsigned char obs_on[4];

// Bullet system (danmaku)
#define MAX_BULLETS 16
static unsigned char bullet_x[MAX_BULLETS];
static unsigned char bullet_y[MAX_BULLETS];
static signed char bullet_dx[MAX_BULLETS];  // X velocity
static signed char bullet_dy[MAX_BULLETS];  // Y velocity
static unsigned char bullet_on[MAX_BULLETS];
static unsigned char bullet_timer;  // Timer for shooting patterns
static unsigned char bullet_next;   // Next bullet slot (circular buffer)

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
static volatile unsigned int  high_scores[NUM_HIGH_SCORES];  // Top 3 scores
static volatile unsigned char high_names[NUM_HIGH_SCORES][3]; // 3-letter names
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

// ============================================
// RACING BGM - Energetic with heavy triangle bass!
// ============================================
#define RACING_LEN 32

// Triangle bass line - driving bass!
static const unsigned char racing_tri[RACING_LEN] = {
    E2, E2, E3, E2,  G2, G2, G3, G2,
    A2, A2, A3, A2,  G2, G2, G3, G2,
    E2, E2, E3, E2,  G2, G2, G3, G2,
    A2, A2, B2, B2,  C3, C3, B2, A2,
};

// Pulse 1 - melody
static const unsigned char racing_pl1[RACING_LEN] = {
    E4, NOTE_REST, G4, E4,  NOTE_REST, B4, A4, G4,
    A4, NOTE_REST, C5, A4,  NOTE_REST, G4, E4, D4,
    E4, NOTE_REST, G4, E4,  NOTE_REST, B4, A4, G4,
    A4, A4, B4, B4,  C5, C5, B4, A4,
};

// Pulse 2 - harmony/arpeggios
static const unsigned char racing_pl2[RACING_LEN] = {
    B3, B3, E4, B3,  D4, D4, G4, D4,
    E4, E4, A4, E4,  D4, D4, G4, D4,
    B3, B3, E4, B3,  D4, D4, G4, D4,
    E4, E4, FS4, FS4, G4, G4, FS4, E4,
};

// Noise pattern (0=off, 1=kick, 2=snare, 3=hihat)
static const unsigned char racing_noise[RACING_LEN] = {
    1, 3, 0, 3,  2, 3, 0, 3,
    1, 3, 0, 3,  2, 3, 0, 3,
    1, 3, 0, 3,  2, 3, 0, 3,
    1, 3, 2, 3,  1, 2, 1, 2,
};

// ============================================
// RACING BGM LAP 2 - More intense! (A minor, faster)
// ============================================
// Triangle bass - driving harder
static const unsigned char racing2_tri[RACING_LEN] = {
    A2, A2, A3, A2,  C3, C3, C3, C3,
    D3, D3, D3, D3,  E3, E3, E3, E2,
    A2, A2, A3, A2,  C3, C3, C3, C3,
    D3, D3, E3, E3,  A2, A2, A3, A2,
};

// Pulse 1 - more aggressive melody
static const unsigned char racing2_pl1[RACING_LEN] = {
    A4, C5, E5, G5,  G5, E5, C5, E5,
    D5, F5, G5, F5,  E5, G5, B4, E5,
    A4, C5, E5, G5,  G5, E5, C5, E5,
    D5, E5, F5, E5,  A4, A4, G5, A4,
};

// Pulse 2 - intense arpeggios
static const unsigned char racing2_pl2[RACING_LEN] = {
    E4, A4, C5, E4,  C4, E4, G4, C4,
    F4, A4, D5, F4,  E4, G4, B4, E4,
    E4, A4, C5, E4,  C4, E4, G4, C4,
    F4, G4, A4, G4,  E4, E4, A4, E4,
};

// More aggressive drums
static const unsigned char racing2_noise[RACING_LEN] = {
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 3, 2, 3,  1, 2, 1, 2,
    1, 3, 2, 3,  1, 3, 2, 3,
    1, 2, 1, 2,  1, 2, 1, 2,
};

// ============================================
// RACING BGM LAP 3 - Maximum chaos! (E minor, fastest)
// ============================================
// Triangle bass - relentless
static const unsigned char racing3_tri[RACING_LEN] = {
    E3, E3, E3, E2,  E3, E3, E3, E2,
    G3, G3, G3, G2,  A3, A3, B3, B3,
    E3, E3, E3, E2,  G3, G3, G3, G2,
    A3, B3, C4, B3,  E3, E3, E2, E3,
};

// Pulse 1 - frantic melody
static const unsigned char racing3_pl1[RACING_LEN] = {
    E5, G5, E5, E5,  G5, E5, E5, G5,
    G5, E5, D5, G5,  G5, E5, C5, E5,
    E5, G5, E5, E5,  G5, E5, E5, G5,
    G5, E5, C5, E5,  E5, E5, E5, E5,
};

// Pulse 2 - chaotic harmonies
static const unsigned char racing3_pl2[RACING_LEN] = {
    B4, E5, G4, B4,  D5, G4, B4, D5,
    D5, G5, B4, D5,  E5, FS4, G4, FS4,
    B4, E5, G4, B4,  D5, G4, B4, D5,
    E5, FS4, G4, FS4, B4, B4, B4, B4,
};

// Frantic drums
static const unsigned char racing3_noise[RACING_LEN] = {
    1, 2, 1, 2,  1, 2, 1, 2,
    1, 2, 1, 2,  1, 2, 1, 2,
    1, 2, 1, 2,  1, 2, 1, 2,
    1, 2, 1, 2,  1, 2, 1, 2,
};

// ============================================
// TITLE BGM - Heroic and majestic!
// ============================================
#define TITLE_LEN 32

// Powerful triangle bass - driving rhythm
static const unsigned char title_tri[TITLE_LEN] = {
    C3, C3, C2, C3,  G2, G2, G2, G3,
    A2, A2, A2, A3,  G2, G2, G2, G3,
    F2, F2, F2, F3,  G2, G2, G2, G3,
    A2, A2, G2, G2,  C3, C3, C3, C3,
};

// Heroic fanfare melody
static const unsigned char title_pl1[TITLE_LEN] = {
    C5, C5, G4, C5,  E5, E5, D5, C5,
    A4, A4, C5, A4,  G4, G4, E4, G4,
    F4, F4, A4, C5,  G4, G4, B4, D5,
    E5, E5, D5, C5,  C5, NOTE_REST, C5, NOTE_REST,
};

// Power chord harmony
static const unsigned char title_pl2[TITLE_LEN] = {
    E4, E4, C4, E4,  G4, G4, F4, E4,
    C4, C4, E4, C4,  D4, D4, C4, D4,
    A3, A3, C4, E4,  D4, D4, F4, G4,
    G4, G4, F4, E4,  E4, NOTE_REST, E4, NOTE_REST,
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
    D2, NOTE_REST, A2, NOTE_REST,
    E2, NOTE_REST, E2, NOTE_REST,
};

// Descending sad melody
static const unsigned char gameover_pl1[GAMEOVER_LEN] = {
    E4, D4, C4, B3,
    A3, NOTE_REST, GS3, NOTE_REST,
    A3, B3, C4, NOTE_REST,
    B3, A3, NOTE_REST, NOTE_REST,
};

// Minor harmony
static const unsigned char gameover_pl2[GAMEOVER_LEN] = {
    C4, B3, A3, GS3,
    F3, NOTE_REST, E3, NOTE_REST,
    F3, GS3, A3, NOTE_REST,
    GS3, E3, NOTE_REST, NOTE_REST,
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
    0x0F, 0x11, 0x21, 0x31,  // Player (blue)
    0x0F, 0x06, 0x16, 0x26,  // Enemy (red)
    0x0F, 0x07, 0x17, 0x27,  // Obstacle (orange)
    0x0F, 0x30, 0x30, 0x30   // HUD (white)
};

// ============================================
// FORWARD DECLARATIONS
// ============================================
static void do_game_over(void);
static void finish_game_over(void);
static unsigned char check_high_score(unsigned int new_score);
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

// Start a music track
static void music_play(unsigned char track) {
    current_track = track;
    music_pos = 0;
    music_frame = 0;

    switch (track) {
        case 0:  // Title - Heroic march
            music_tempo = 8;
            music_intensity = 0;
            break;
        case 1:  // Racing - tempo varies with intensity
            switch (music_intensity) {
                case 0:  music_tempo = 6; break;   // Lap 1: Normal
                case 1:  music_tempo = 5; break;   // Lap 2: Faster
                default: music_tempo = 4; break;   // Lap 3: Fastest!
            }
            break;
        case 2:  // Win
            music_tempo = 10;
            music_intensity = 0;
            break;
        case 3:  // Game Over
            music_tempo = 16;
            music_intensity = 0;
            break;
        case 4:  // Epilogue - calm, slow
            music_tempo = 12;
            music_intensity = 0;
            break;
    }
}

// Set music intensity (for lap-based changes)
static void music_set_intensity(unsigned char intensity) {
    music_intensity = intensity;
    // Update tempo if currently playing racing music
    if (current_track == 1) {
        switch (intensity) {
            case 0:  music_tempo = 6; break;   // Lap 1
            case 1:  music_tempo = 5; break;   // Lap 2
            default: music_tempo = 4; break;   // Lap 3
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

// Update music (call every frame)
static void music_update(void) {
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
        case 0:  // Title
            len = TITLE_LEN;
            tri_data = title_tri;
            pl1_data = title_pl1;
            pl2_data = title_pl2;
            noise_data = 0;
            break;
        case 1:  // Racing - different music per lap
            len = RACING_LEN;
            switch (music_intensity) {
                case 0:  // Lap 1 - Normal
                    tri_data = racing_tri;
                    pl1_data = racing_pl1;
                    pl2_data = racing_pl2;
                    noise_data = racing_noise;
                    break;
                case 1:  // Lap 2 - Intense
                    tri_data = racing2_tri;
                    pl1_data = racing2_pl1;
                    pl2_data = racing2_pl2;
                    noise_data = racing2_noise;
                    break;
                default: // Lap 3 - Chaos
                    tri_data = racing3_tri;
                    pl1_data = racing3_pl1;
                    pl2_data = racing3_pl2;
                    noise_data = racing3_noise;
                    break;
            }
            break;
        case 2:  // Win
            len = WIN_LEN;
            tri_data = win_tri;
            pl1_data = win_pl1;
            pl2_data = win_pl2;
            noise_data = 0;
            break;
        case 3:  // Game Over
            len = GAMEOVER_LEN;
            tri_data = gameover_tri;
            pl1_data = gameover_pl1;
            pl2_data = gameover_pl2;
            noise_data = 0;
            break;
        case 4:  // Epilogue
            len = EPILOGUE_LEN;
            tri_data = epilogue_tri;
            pl1_data = epilogue_pl1;
            pl2_data = epilogue_pl2;
            noise_data = 0;
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

// Wait for vblank
static void wait_vblank(void) {
    while (!(PPU_STATUS & 0x80));
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
static void ppu_addr(unsigned int addr) {
    PPU_ADDR = (unsigned char)(addr >> 8);
    PPU_ADDR = (unsigned char)(addr);
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

// Set a single sprite
static unsigned char set_sprite(unsigned char id, unsigned char x, unsigned char y,
                                unsigned char tile, unsigned char attr) {
    unsigned char idx = id * 4;
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
// Loop 1: Day, Loop 2: Evening, Loop 3: Night, Loop 4+: repeat
static void update_loop_palette(void) {
    unsigned char grass_hue, road_hue;

    // Select hue based on loop_count
    // Loop 0: Day (green 0x09, gray 0x00)
    // Loop 1: Evening (orange 0x07, warm 0x06)
    // Loop 2+: Night (blue 0x02, cold 0x01), then alternate
    if (loop_count == 0) {
        grass_hue = 0x09;  // Green
        road_hue = 0x00;   // Gray
    } else if ((loop_count & 1) == 1) {
        grass_hue = 0x07;  // Orange/evening
        road_hue = 0x06;   // Warm
    } else {
        grass_hue = 0x02;  // Blue/night
        road_hue = 0x01;   // Cold
    }

    // Wait for VBlank to safely update palette
    wait_vblank();

    // Disable rendering during palette update
    PPU_MASK = 0x00;

    // Update road palette (BG palette 0 at $3F00-$3F03)
    ppu_addr(0x3F00);
    PPU_DATA = 0x0F;
    PPU_DATA = road_hue;
    PPU_DATA = road_hue + 0x10;
    PPU_DATA = road_hue + 0x20;

    // Update grass palette (BG palette 1 at $3F04-$3F07)
    ppu_addr(0x3F04);
    PPU_DATA = 0x0F;
    PPU_DATA = grass_hue;
    PPU_DATA = grass_hue + 0x10;
    PPU_DATA = grass_hue + 0x20;

    // IMPORTANT: Reset PPU address latch before writing scroll
    (void)PPU_STATUS;

    // Restore scroll to current position (not fixed zero)
    PPU_SCROLL = 0;
    PPU_SCROLL = scroll_y;

    // Re-enable rendering
    PPU_MASK = 0x1E;
}

// Draw the road background
static void draw_road(void) {
    unsigned char row, col;
    unsigned char attr_row;

    ppu_off();

    // Draw nametable (30 rows x 32 columns)
    for (row = 0; row < 30; ++row) {
        ppu_addr(0x2000 + (unsigned int)row * 32);
        for (col = 0; col < 32; ++col) {
            if (col < 8 || col >= 24) {
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
    // Road is columns 8-23, grass is 0-7 and 24-31
    ppu_addr(0x23C0);
    for (attr_row = 0; attr_row < 8; ++attr_row) {
        // 0x55 = 01010101 = all 4 quadrants use palette 1 (grass)
        // 0x00 = 00000000 = all 4 quadrants use palette 0 (road)
        PPU_DATA = 0x55;  // Columns 0-3: grass
        PPU_DATA = 0x55;  // Columns 4-7: grass
        PPU_DATA = 0x00;  // Columns 8-11: road
        PPU_DATA = 0x00;  // Columns 12-15: road
        PPU_DATA = 0x00;  // Columns 16-19: road
        PPU_DATA = 0x00;  // Columns 20-23: road
        PPU_DATA = 0x55;  // Columns 24-27: grass
        PPU_DATA = 0x55;  // Columns 28-31: grass
    }

    ppu_on();
}

// Prepare next enemy spawn (show warning marker)
static void prepare_enemy(void) {
    enemy_next_x = ROAD_LEFT + 16 + (rnd() & 0x3F);
    if (enemy_next_x > ROAD_RIGHT - 24) {
        enemy_next_x = ROAD_RIGHT - 24;
    }
    enemy_warn_timer = 120;  // 2 second warning
}

// Actually spawn the enemy (appears from top, player must overtake)
static void spawn_enemy(void) {
    unsigned char slot = enemy_slot;
    enemy_x[slot] = enemy_next_x;
    enemy_y[slot] = 8;  // Start just below top of screen
    enemy_on[slot] = 1;
    enemy_passed[slot] = 0;
    enemy_warn_timer = 0;
    // Cycle through slots (can't use bitwise AND since MAX_ENEMIES is not power of 2)
    ++enemy_slot;
    if (enemy_slot >= MAX_ENEMIES) enemy_slot = 0;
}

// Spawn obstacle
static void spawn_obstacle(void) {
    unsigned char i;
    for (i = 0; i < 4; ++i) {
        if (!obs_on[i]) {
            obs_x[i] = ROAD_LEFT + 8 + (rnd() & 0x7F);
            if (obs_x[i] > ROAD_RIGHT - 16) {
                obs_x[i] = ROAD_RIGHT - 16;
            }
            obs_y[i] = 0;
            obs_on[i] = 1;
            break;
        }
    }
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
    ++bullet_next;
    if (bullet_next >= MAX_BULLETS) {
        bullet_next = 0;
    }
}

// Current pattern phase for complex patterns
static unsigned char pattern_phase;
static unsigned char pattern_type;

// Calculate aimed bullet velocity - simple version
static signed char calc_aim_dx(unsigned char bx) {
    unsigned char px = player_x + 8;
    if (px > bx + 24) return 2;
    if (px > bx + 8) return 1;
    if (px + 24 < bx) return -2;
    if (px + 8 < bx) return -1;
    return 0;
}

// Spawn danmaku pattern from enemies - ALL AIMED AT PLAYER
static void spawn_danmaku(void) {
    unsigned char i, cx, cy;
    signed char dx, dy;
    unsigned char mask;

    ++bullet_timer;

    // Change pattern every 256 frames
    if (bullet_timer == 0) {
        pattern_type = (pattern_type + 1) & 0x07;
    }

    // When in 1st place: beautiful danmaku from behind
    if (position == 1) {
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

// Update all bullets
static void update_bullets(void) {
    unsigned char i;
    unsigned char nx, ny;

    for (i = 0; i < MAX_BULLETS; ++i) {
        if (bullet_on[i]) {
            // Move bullet
            nx = bullet_x[i] + bullet_dx[i];
            ny = bullet_y[i] + bullet_dy[i];

            // Check bounds
            if (nx < 8 || nx > 248 || ny > 240) {
                bullet_on[i] = 0;
            } else {
                bullet_x[i] = nx;
                bullet_y[i] = ny;
            }
        }
    }
}

// Check bullet collisions with player
static void check_bullet_collisions(void) {
    unsigned char i, dx, dy;

    if (player_inv > 0) return;

    for (i = 0; i < MAX_BULLETS; ++i) {
        if (bullet_on[i]) {
            dx = abs_diff(player_x + 8, bullet_x[i]);
            dy = abs_diff(player_y + 8, bullet_y[i]);

            // Damage zone: dx < 4 && dy < 4 (very small hitbox - cockpit only)
            if (dx < 4 && dy < 4) {
                --player_hp;
                player_inv = 60;
                bullet_on[i] = 0;
                // Penalty: -1 point (but don't go negative)
                if (score > 0) --score;
                // Reset multiplier and no-damage timer
                score_multiplier = 1;
                no_damage_timer = 0;
                if (player_hp == 0) {
                    do_game_over();
                    return;
                }
            }
            // Graze zone: dx < 10 && dy < 10 but outside damage zone
            else if (dx < 10 && dy < 10) {
                // Graze! Points = multiplier * 2^loop_count
                // Loop 0: x1, Loop 1: x2, Loop 2: x4, etc.
                score += score_multiplier * (1 << loop_count);
                graze_timer = 15;  // Show "BUZ" for 15 frames
                // Don't remove bullet - player can still get hit!
            }
        }
    }
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

// Check if score qualifies for high score, return rank (0-2) or 255 if not
static unsigned char check_high_score(unsigned int new_score) {
    unsigned char i;
    for (i = 0; i < NUM_HIGH_SCORES; ++i) {
        if (new_score > high_scores[i]) {
            return i;
        }
    }
    return 255;  // Not a high score
}

// Insert a new high score at given rank
static void insert_high_score(unsigned char rank, unsigned int new_score) {
    unsigned char i;
    // Shift lower scores down
    for (i = NUM_HIGH_SCORES - 1; i > rank; --i) {
        high_scores[i] = high_scores[i - 1];
        high_names[i][0] = high_names[i - 1][0];
        high_names[i][1] = high_names[i - 1][1];
        high_names[i][2] = high_names[i - 1][2];
    }
    // Insert new score
    high_scores[rank] = new_score;
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

// Handle game over - start explosion first, then check high score
static void do_game_over(void) {
    // Start explosion at player position
    explode_x = player_x;
    explode_y = player_y;
    explode_timer = 0;
    game_state = STATE_EXPLODE;
    music_play(3);  // Game over music during explosion only
}

// Finish game over after explosion - check for high score
static void finish_game_over(void) {
    new_score_rank = check_high_score(score);
    if (new_score_rank < NUM_HIGH_SCORES) {
        // Got a high score! Go to name entry
        game_state = STATE_HIGHSCORE;
        init_name_entry(new_score_rank);
        music_play(4);  // Epilogue music for score entry
    } else {
        // No high score, just game over
        game_state = STATE_GAMEOVER;
        music_stop();  // Silence after explosion
    }
}

// Initialize game
static void init_game(void) {
    unsigned char i;

    player_x = PLAYER_START_X;
    player_y = PLAYER_START_Y;
    player_hp = PLAYER_MAX_HP;
    player_inv = 0;

    for (i = 0; i < MAX_ENEMIES; ++i) enemy_on[i] = 0;
    enemy_slot = 0;
    enemy_warn_timer = 0;
    position = 12;  // Start in 12th place (last of 12 cars)
    lap_count = 0;
    loop_count = title_select_loop;  // Start from selected loop
    score = 0;
    distance = 0;
    scroll_y = 0;
    score_multiplier = 1;  // Start with 1x multiplier
    no_damage_timer = 0;
    graze_timer = 0;
    explode_timer = 0;
    goal_line_timer = 0;

    // Set music intensity based on starting loop
    // Loop 1 LAP1: calm(0), Loop 2+ LAP1: moderate(1)
    if (loop_count > 0) {
        music_intensity = 1;  // Loop 2+: moderate start
    } else {
        music_intensity = 0;  // Loop 1: calm start
    }

    for (i = 0; i < 4; ++i) {
        obs_on[i] = 0;
    }

    // Clear all bullets
    for (i = 0; i < MAX_BULLETS; ++i) {
        bullet_on[i] = 0;
    }
    bullet_timer = 0;
    bullet_next = 0;
    pattern_phase = 0;
    pattern_type = 0;

    draw_road();

    // Apply palette for current loop (day/evening/night)
    update_loop_palette();

    // Spawn first enemy immediately (no warning delay)
    enemy_next_x = ROAD_LEFT + 16 + (rnd() & 0x3F);
    if (enemy_next_x > ROAD_RIGHT - 24) {
        enemy_next_x = ROAD_RIGHT - 24;
    }
    spawn_enemy();
}

// Update player
static void update_player(void) {
    if (pad_now & BTN_LEFT) {
        if (player_x > ROAD_LEFT) player_x -= PLAYER_SPEED;
    }
    if (pad_now & BTN_RIGHT) {
        if (player_x < ROAD_RIGHT - 16) player_x += PLAYER_SPEED;
    }
    if (pad_now & BTN_UP) {
        // Keep player below HUD area (Y=60 is about 1/4 of screen height)
        // This prevents sprite overflow conflicts with HUD sprites on same scanlines
        if (player_y > 60) player_y -= PLAYER_SPEED;
    }
    if (pad_now & BTN_DOWN) {
        if (player_y < SCREEN_HEIGHT - 32) player_y += PLAYER_SPEED;
    }

    if (player_inv > 0) --player_inv;
}

// Count enemies ahead of player (not yet passed)
static unsigned char count_enemies_ahead(void) {
    unsigned char i, count = 0;
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i] && !enemy_passed[i]) ++count;
    }
    return count;
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
    if (enemies_ahead < position - 1 && enemies_ahead < MAX_ENEMIES && enemy_warn_timer == 0) {
        prepare_enemy();
    }
    if (enemy_warn_timer > 0) {
        --enemy_warn_timer;
        if (enemy_warn_timer == 0) {
            // Re-check condition: only spawn if still needed
            if (count_enemies_ahead() < position - 1) {
                spawn_enemy();
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
        } else {
            // Ahead of player: half speed (1 pixel every 2 frames)
            if (frame_count & 1) {
                enemy_y[i] += 1;
            }
        }

        // AI: try to block player (only if ahead)
        if (!enemy_passed[i] && (frame_count & 0x07) == 0) {
            if (enemy_x[i] + 8 < player_x && enemy_x[i] < ROAD_RIGHT - 24) {
                enemy_x[i] += 1;
            } else if (enemy_x[i] > player_x + 8 && enemy_x[i] > ROAD_LEFT + 8) {
                enemy_x[i] -= 1;
            }
        }

        // Overtaken - award points once
        if (player_y + 16 < enemy_y[i] && !enemy_passed[i]) {
            enemy_passed[i] = 1;
            score += 20 * score_multiplier;
            if (position > 1) --position;
        }

        // Remove when off screen
        if (enemy_y[i] > 240) {
            enemy_on[i] = 0;
        }
    }
}

// Update obstacles
static void update_obstacles(void) {
    unsigned char i;
    for (i = 0; i < 4; ++i) {
        if (obs_on[i]) {
            obs_y[i] += SCROLL_SPEED;
            if (obs_y[i] > SCREEN_HEIGHT) {
                obs_on[i] = 0;
            }
        }
    }
}

// Check collisions
static void check_collisions(void) {
    unsigned char i, dx, dy;

    if (player_inv > 0) return;

    // Enemy collisions
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i]) {
            dx = abs_diff(player_x, enemy_x[i]);
            dy = abs_diff(player_y, enemy_y[i]);

            if (dx < 14 && dy < 14) {
                --player_hp;
                player_inv = 60;
                if (player_hp == 0) {
                    do_game_over();
                    return;
                }
                break;
            }
        }
    }

    // Obstacle collisions
    for (i = 0; i < 4; ++i) {
        if (obs_on[i]) {
            dx = abs_diff(player_x, obs_x[i]);
            dy = abs_diff(player_y, obs_y[i]);

            if (dx < 12 && dy < 12) {
                --player_hp;
                player_inv = 60;
                obs_on[i] = 0;
                if (player_hp == 0) {
                    do_game_over();
                    return;
                }
            }
        }
    }
}

// Main game update
static void update_game(void) {
    update_player();
    update_enemy();
    update_obstacles();
    check_collisions();

    // Danmaku system
    spawn_danmaku();
    update_bullets();
    check_bullet_collisions();

    // No-damage timer: +1x multiplier every 10 seconds (600 frames)
    ++no_damage_timer;
    if (no_damage_timer >= 600) {
        no_damage_timer = 0;
        if (score_multiplier < 9) {
            ++score_multiplier;
        }
    }

    // Decrease graze display timer
    if (graze_timer > 0) {
        --graze_timer;
    }

    // Update explosion animation
    if (explode_timer > 0) {
        --explode_timer;
        explode_y += SCROLL_SPEED;  // Explosion scrolls down with road
    }

    // Goal line animation update
    if (goal_line_timer > 0) {
        --goal_line_timer;
        goal_line_y += SCROLL_SPEED;  // Goal line scrolls down
    }

    ++distance;
    if (distance >= LAP_DISTANCE) {  // Lap complete
        distance = 0;
        ++lap_count;

        // Trigger goal line effect
        goal_line_y = 0;
        goal_line_timer = 60;  // Show for 60 frames

        // Recover 3 HP on lap completion
        player_hp += 3;
        if (player_hp > PLAYER_MAX_HP) {
            player_hp = PLAYER_MAX_HP;
        }

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
                score += 1000 * (loop_count) * (1 << loop_count);

                // Clear bullets for fresh start
                {
                    unsigned char b;
                    for (b = 0; b < MAX_BULLETS; ++b) {
                        bullet_on[b] = 0;
                    }
                }

                // Go to loop clear celebration screen
                game_state = STATE_LOOP_CLEAR;
                loop_clear_timer = 0;
                music_play(2);  // Victory music
                init_win_animation();  // Reuse confetti
            } else {
                // Finished 3 laps but not in 1st place - game over
                do_game_over();
            }
        }
    }

    if ((frame_count & 0x7F) == 0) {  // Less frequent obstacles
        spawn_obstacle();
    }

    scroll_y += SCROLL_SPEED;
}

// Draw game sprites
static void draw_game(void) {
    unsigned char id = 0;
    unsigned char i;

    // Player car (4 sprites)
    if (player_inv == 0 || (frame_count & 4)) {
        id = set_car(id, player_x, player_y, SPR_CAR, 0);
    }

    // Enemy cars (4 sprites each)
    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (enemy_on[i]) {
            id = set_car(id, enemy_x[i], enemy_y[i], SPR_CAR + 4, 1);
        }
    }

    // Explosion effect (1 sprite, blinking)
    if (explode_timer > 0 && (frame_count & 2)) {
        id = set_sprite(id, explode_x + 4, explode_y + 4, SPR_EXPLOSION, 1);
    }

    // === Critical HUD first (always visible) ===

    // HUD - Position (3-4 sprites)
    if (position >= 10) {
        id = set_sprite(id, 8, 8, SPR_DIGIT + 1, 3);              // "1"
        id = set_sprite(id, 16, 8, SPR_DIGIT + (position - 10), 3); // 0,1,2
        id = set_sprite(id, 24, 8, SPR_LETTER + 19, 3);  // T
        id = set_sprite(id, 32, 8, SPR_LETTER + 7, 3);   // H
    } else {
        id = set_sprite(id, 8, 8, SPR_DIGIT + position, 3);
        if (position == 1) {
            id = set_sprite(id, 16, 8, SPR_LETTER + 18, 3);  // S
            id = set_sprite(id, 24, 8, SPR_LETTER + 19, 3);  // T
        } else if (position == 2) {
            id = set_sprite(id, 16, 8, SPR_LETTER + 13, 3);  // N
            id = set_sprite(id, 24, 8, SPR_LETTER + 3, 3);   // D
        } else if (position == 3) {
            id = set_sprite(id, 16, 8, SPR_LETTER + 17, 3);  // R
            id = set_sprite(id, 24, 8, SPR_LETTER + 3, 3);   // D
        } else {
            id = set_sprite(id, 16, 8, SPR_LETTER + 19, 3);  // T
            id = set_sprite(id, 24, 8, SPR_LETTER + 7, 3);   // H
        }
    }

    // HUD - HP (2 sprites) - bottom left to avoid overlap
    id = set_sprite(id, 8, 224, SPR_LETTER + 7, 3);  // H
    id = set_sprite(id, 16, 224, SPR_DIGIT + player_hp, 3);

    // HUD - Multiplier "xN" (2 sprites)
    id = set_sprite(id, 216, 8, SPR_LETTER + 23, 3);  // X
    id = set_sprite(id, 224, 8, SPR_DIGIT + score_multiplier, 3);

    // HUD - Score (5 sprites)
    {
        unsigned int s = score;
        if (s > 99999) s = 99999;
        id = set_sprite(id, 200, 20, SPR_DIGIT + (unsigned char)(s / 10000), 3);
        id = set_sprite(id, 208, 20, SPR_DIGIT + (unsigned char)((s / 1000) % 10), 3);
        id = set_sprite(id, 216, 20, SPR_DIGIT + (unsigned char)((s / 100) % 10), 3);
        id = set_sprite(id, 224, 20, SPR_DIGIT + (unsigned char)((s / 10) % 10), 3);
        id = set_sprite(id, 232, 20, SPR_DIGIT + (unsigned char)(s % 10), 3);
    }

    // HUD - Lap counter "LX" at center-top (2 sprites, different Y to avoid scanline limit)
    id = set_sprite(id, 120, 16, SPR_LETTER + 11, 3);  // L
    id = set_sprite(id, 128, 16, SPR_DIGIT + lap_count + 1, 3);  // Current lap (1-3)

    // Loop counter (shown when in 2nd loop or higher)
    if (loop_count > 0) {
        // Show loop number at center (周=loop)
        id = set_sprite(id, 104, 16, SPR_DIGIT + loop_count + 1, 1);  // Loop number in red
    }

    // "BUZ" display when grazing (3 sprites)
    if (graze_timer > 0) {
        id = set_sprite(id, player_x - 8, player_y - 16, SPR_LETTER + 1, 1);  // B
        id = set_sprite(id, player_x,     player_y - 16, SPR_LETTER + 20, 1); // U
        id = set_sprite(id, player_x + 8, player_y - 16, SPR_LETTER + 25, 1); // Z
    }

    // === Game objects (may be culled if too many) ===

    // Goal line (checkered pattern scrolling down)
    if (goal_line_timer > 0 && goal_line_y < 240) {
        for (i = 0; i < 6 && id < 56; ++i) {
            // Alternating pattern
            id = set_sprite(id, ROAD_LEFT + i * 16, goal_line_y,
                           (i & 1) ? SPR_BAR_FILL : SPR_BAR_EMPTY, 3);
        }
    }

    // Warning marker for next enemy (2 sprites)
    if (enemy_warn_timer > 0 && (frame_count & 8)) {
        id = set_sprite(id, enemy_next_x + 4, 8, 0x0A, 1 | 0x80);
        id = set_sprite(id, enemy_next_x + 4, 16, 0x0A, 1 | 0x80);
    }

    // Obstacles (up to 4 sprites)
    for (i = 0; i < 4; ++i) {
        if (obs_on[i] && id < 60) {
            id = set_sprite(id, obs_x[i], obs_y[i], SPR_OBSTACLE, 2);
        }
    }

    // Bullets - danmaku (use remaining sprite slots)
    // Flicker rendering: only draw half the bullets per frame to reduce sprite overflow
    // Even frames draw even-indexed bullets, odd frames draw odd-indexed bullets
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (bullet_on[i] && id < 62 && ((i & 1) == (frame_count & 1))) {
            id = set_sprite(id, bullet_x[i], bullet_y[i], SPR_BULLET, 1);
        }
    }

    // HUD - Simple progress indicator (car icon moving toward GOAL)
    // Simplified: distance 0-700 maps to position 80-168 (88 pixels)
    // car_pos = 80 + (distance * 88 / 700) = 80 + distance / 8 (approx)
    {
        unsigned char car_pos;
        car_pos = 80 + (unsigned char)(distance >> 3);  // distance / 8
        if (car_pos > 168) car_pos = 168;
        id = set_sprite(id, car_pos, 224, SPR_CAR_ICON, 0);
        id = set_sprite(id, 168, 224, SPR_LETTER + 6, 3);  // G (goal)
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
    unsigned char x = 100, y = 60;
    unsigned char i;
    unsigned int s;

    // "RACE"
    id = set_sprite(id, x,      y, SPR_LETTER + 17, 0);  // R
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 0,  0);  // A
    id = set_sprite(id, x + 16, y, SPR_LETTER + 2,  0);  // C
    id = set_sprite(id, x + 24, y, SPR_LETTER + 4,  0);  // E

    // Blinking car
    if (frame_count & 0x20) {
        id = set_car(id, x + 4, y + 24, SPR_CAR, 0);
    }

    // High scores display (3 entries, each on different Y line)
    // Entry 1 at y=120, Entry 2 at y=136, Entry 3 at y=152
    for (i = 0; i < NUM_HIGH_SCORES; ++i) {
        y = 120 + i * 16;

        // Rank number (1-3)
        id = set_sprite(id, 64, y, SPR_DIGIT + i + 1, 3);

        // Name (3 letters)
        id = set_sprite(id, 80, y, SPR_LETTER + high_names[i][0], 3);
        id = set_sprite(id, 88, y, SPR_LETTER + high_names[i][1], 3);
        id = set_sprite(id, 96, y, SPR_LETTER + high_names[i][2], 3);

        // Score (5 digits)
        s = high_scores[i];
        if (s > 99999) s = 99999;
        x = 112;
        id = set_sprite(id, x,      y, SPR_DIGIT + (unsigned char)(s / 10000), 3);
        id = set_sprite(id, x + 8,  y, SPR_DIGIT + (unsigned char)((s / 1000) % 10), 3);
        id = set_sprite(id, x + 16, y, SPR_DIGIT + (unsigned char)((s / 100) % 10), 3);
        id = set_sprite(id, x + 24, y, SPR_DIGIT + (unsigned char)((s / 10) % 10), 3);
        id = set_sprite(id, x + 32, y, SPR_DIGIT + (unsigned char)(s % 10), 3);
    }

    // Loop selection (only show if player has completed at least 1 loop)
    if (max_loop > 0) {
        y = 176;
        x = 80;
        // "LOOP" label
        id = set_sprite(id, x,      y, SPR_LETTER + 11, 3);  // L
        id = set_sprite(id, x + 8,  y, SPR_LETTER + 14, 3);  // O
        id = set_sprite(id, x + 16, y, SPR_LETTER + 14, 3);  // O
        id = set_sprite(id, x + 24, y, SPR_LETTER + 15, 3);  // P

        // Loop number (blinking if selectable)
        x = 120;
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

    // "START" prompt at bottom (blinking)
    if (frame_count & 0x20) {
        y = 208;
        x = 88;
        id = set_sprite(id, x,      y, SPR_LETTER + 18, 2);  // S
        id = set_sprite(id, x + 8,  y, SPR_LETTER + 19, 2);  // T
        id = set_sprite(id, x + 16, y, SPR_LETTER + 0,  2);  // A
        id = set_sprite(id, x + 24, y, SPR_LETTER + 17, 2);  // R
        id = set_sprite(id, x + 32, y, SPR_LETTER + 19, 2);  // T
    }

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

    // "GAME"
    id = set_sprite(id, x,      y, SPR_LETTER + 6,  1);  // G
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 0,  1);  // A
    id = set_sprite(id, x + 16, y, SPR_LETTER + 12, 1);  // M
    id = set_sprite(id, x + 24, y, SPR_LETTER + 4,  1);  // E

    // "OVER"
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

        // Draw cursor under current position (blinking)
        if (i == name_entry_pos && (frame_count & 0x10)) {
            id = set_sprite(id, x + i * 16, y + 10, SPR_BAR_FILL, 1);
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
    id = set_sprite(id, 112, 60, SPR_DIGIT + loop_count, 1);

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

    // Draw initial road
    draw_road();

    // Enable NMI and rendering
    PPU_CTRL = 0x88;  // NMI on, sprites at $1000
    ppu_on();

    // Main loop
    while (1) {
        // Wait for vblank
        wait_vblank();

        // OAM DMA
        OAM_ADDR = 0;
        OAM_DMA = 0x02;

        // Set scroll
        PPU_SCROLL = 0;
        PPU_SCROLL = scroll_y;

        // Read controller
        pad_old = pad_now;
        pad_now = read_pad();
        pad_new = pad_now & ~pad_old;

        ++frame_count;
        rnd_seed ^= frame_count;

        // Clear sprites first
        clear_sprites();

        // Update music every frame
        music_update();

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
                    music_play(1);  // Racing BGM - energetic!
                    game_state = STATE_RACING;
                }
                break;

            case STATE_RACING:
                if (pad_new & BTN_START) {
                    game_state = STATE_PAUSED;
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
                }
                break;

            case STATE_EXPLODE:
                // Show explosion animation
                ++explode_timer;
                // Draw road background
                draw_game();
                // Draw explosion effect - expanding
                {
                    unsigned char id = 0;
                    unsigned char ex, ey;
                    unsigned char phase = explode_timer >> 3;  // Every 8 frames

                    // Center explosion
                    id = set_sprite(id, explode_x, explode_y, SPR_EXPLOSION, 2);

                    // Expanding explosions around center
                    if (phase >= 1) {
                        ex = explode_x - 8;
                        ey = explode_y - 8;
                        id = set_sprite(id, ex, ey, SPR_EXPLOSION, 2);
                        id = set_sprite(id, ex + 16, ey, SPR_EXPLOSION, 2);
                    }
                    if (phase >= 2) {
                        ex = explode_x - 16;
                        ey = explode_y;
                        id = set_sprite(id, ex, ey, SPR_EXPLOSION, 2);
                        id = set_sprite(id, ex + 32, ey, SPR_EXPLOSION, 2);
                    }
                    if (phase >= 3) {
                        ey = explode_y + 8;
                        id = set_sprite(id, explode_x - 8, ey, SPR_EXPLOSION, 2);
                        id = set_sprite(id, explode_x + 8, ey, SPR_EXPLOSION, 2);
                    }
                }
                // After ~1 second, move to next state
                if (explode_timer > 60) {
                    finish_game_over();
                }
                break;

            case STATE_GAMEOVER:
                draw_gameover();
                if (pad_new & BTN_START) {
                    music_play(0);  // Back to title BGM
                    game_state = STATE_TITLE;
                }
                break;

            case STATE_WIN:
                update_win_animation();
                draw_win();
                // Only accept START after animation plays (about 1.5 seconds)
                if (win_timer > 90 && (pad_new & BTN_START)) {
                    music_play(0);  // Back to title BGM
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

                    // Redraw road first, then apply new palette
                    draw_road();
                    update_loop_palette();

                    // Resume racing BGM with moderate intensity for LAP 1
                    music_play(1);
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
                        // Done entering name - save score
                        // Direct write to SRAM at $6000
                        {
                            volatile unsigned char *sram = (volatile unsigned char *)0x6000;
                            unsigned int s = score;
                            // Write magic byte
                            sram[0] = SAVE_MAGIC;
                            // Write score at offset 1 (high_scores[0])
                            sram[1] = (unsigned char)(s & 0xFF);
                            sram[2] = (unsigned char)(s >> 8);
                            // Write name at offset 7 (high_names[0])
                            sram[7] = entry_name[0];
                            sram[8] = entry_name[1];
                            sram[9] = entry_name[2];
                        }
                        game_state = STATE_GAMEOVER;
                    } else {
                        // Move to next letter
                        name_entry_char = entry_name[name_entry_pos];
                    }
                }
                // START button to finish name entry immediately
                if (pad_new & BTN_START) {
                    high_scores[new_score_rank] = score;
                    high_names[new_score_rank][0] = entry_name[0];
                    high_names[new_score_rank][1] = entry_name[1];
                    high_names[new_score_rank][2] = entry_name[2];
                    game_state = STATE_GAMEOVER;
                }
                break;
        }
    }
}
