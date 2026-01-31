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
#define STATE_TITLE     0
#define STATE_RACING    1
#define STATE_PITSTOP   2
#define STATE_GAMEOVER  3
#define STATE_WIN       4
#define STATE_PAUSED    5

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
#define SPR_OBSTACLE    0x08
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

static unsigned char enemy_x;
static unsigned char enemy_y;
static unsigned char enemy_speed;
static unsigned char enemy_on;
static unsigned char enemy_next_x;     // Next enemy spawn X position
static unsigned char enemy_warn_timer; // Warning countdown before spawn

static unsigned char position;
static unsigned char lap_count;
static unsigned int score;
static unsigned int distance;

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

static unsigned char pit_timer;
static unsigned char rnd_seed;
static unsigned char win_timer;  // Animation timer for win screen

// Confetti particles for win animation
#define MAX_CONFETTI 12
static unsigned char confetti_x[MAX_CONFETTI];
static unsigned char confetti_y[MAX_CONFETTI];
static unsigned char confetti_color[MAX_CONFETTI];

// ============================================
// MUSIC ENGINE
// ============================================

// Music state
static unsigned char music_enabled;
static unsigned char music_frame;      // Frame counter for timing
static unsigned char music_pos;        // Current position in sequence
static unsigned char music_tempo;      // Frames per beat (lower = faster)
static unsigned char current_track;    // 0=title, 1=racing, 2=win

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
// MUSIC FUNCTIONS
// ============================================

// Initialize APU
static void init_apu(void) {
    // Enable all channels
    APU_STATUS = 0x0F;  // Enable pulse1, pulse2, triangle, noise

    // Set frame counter mode (4-step, no IRQ)
    APU_FRAME = 0x40;

    // Initialize channels to silence
    APU_PL1_VOL = 0x30;  // Silence, constant volume
    APU_PL2_VOL = 0x30;
    APU_TRI_LIN = 0x80;  // Silence triangle
    APU_NOI_VOL = 0x30;

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
        APU_TRI_LIN = 0x00;  // Silence
        return;
    }
    period = note_table[note];
    APU_TRI_LIN = 0xFF;  // Max linear counter (sustain)
    APU_TRI_LO = (unsigned char)(period & 0xFF);
    APU_TRI_HI = (unsigned char)((period >> 8) & 0x07) | 0xF8;
}

// Play a note on pulse 1 channel
static void play_pulse1(unsigned char note) {
    unsigned int period;
    if (note == NOTE_REST || note >= 48) {
        APU_PL1_VOL = 0x30;  // Silence
        return;
    }
    period = note_table[note];
    APU_PL1_VOL = 0xBF;  // 50% duty, constant volume, max volume
    APU_PL1_SWP = 0x00;  // No sweep
    APU_PL1_LO = (unsigned char)(period & 0xFF);
    APU_PL1_HI = (unsigned char)((period >> 8) & 0x07) | 0xF8;
}

// Play a note on pulse 2 channel
static void play_pulse2(unsigned char note) {
    unsigned int period;
    if (note == NOTE_REST || note >= 48) {
        APU_PL2_VOL = 0x30;  // Silence
        return;
    }
    period = note_table[note];
    APU_PL2_VOL = 0x7A;  // 25% duty, constant volume, medium volume
    APU_PL2_SWP = 0x00;  // No sweep
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
            music_tempo = 8;   // Strong, steady tempo
            break;
        case 1:  // Racing
            music_tempo = 6;   // Fast!
            break;
        case 2:  // Win
            music_tempo = 10;  // Medium
            break;
        case 3:  // Game Over
            music_tempo = 16;  // Very slow, sad
            break;
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
        case 1:  // Racing
            len = RACING_LEN;
            tri_data = racing_tri;
            pl1_data = racing_pl1;
            pl2_data = racing_pl2;
            noise_data = racing_noise;
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
    enemy_warn_timer = 60;  // 1 second warning
}

// Actually spawn the enemy (appears from top, player must overtake)
static void spawn_enemy(void) {
    enemy_x = enemy_next_x;
    enemy_y = 8;  // Start just below top of screen (ahead of player)
    enemy_speed = 1;  // Moves down slowly (player catches up via scroll)
    enemy_on = 1;
    enemy_warn_timer = 0;
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

// Spawn a single bullet
static void spawn_bullet(unsigned char x, unsigned char y, signed char dx, signed char dy) {
    unsigned char i;
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (!bullet_on[i]) {
            bullet_x[i] = x;
            bullet_y[i] = y;
            bullet_dx[i] = dx;
            bullet_dy[i] = dy;
            bullet_on[i] = 1;
            break;
        }
    }
}

// Sine approximation table (8 values for 0-315 degrees, scaled -2 to 2)
static const signed char sin_table[8] = { 0, 1, 2, 1, 0, -1, -2, -1 };
static const signed char cos_table[8] = { 2, 1, 0, -1, -2, -1, 0, 1 };

// Current pattern phase for complex patterns
static unsigned char pattern_phase;
static unsigned char pattern_type;

// Spawn danmaku pattern from enemy - Geometric/Scientific patterns
static void spawn_danmaku(void) {
    unsigned char cx, cy;
    unsigned char i;
    signed char dx, dy;
    unsigned char angle_idx;

    if (!enemy_on) return;
    if (enemy_y < 24) return;  // Don't shoot while entering screen

    cx = enemy_x + 8;  // Center X
    cy = enemy_y + 16; // Bullet spawn Y

    ++bullet_timer;

    // Change pattern every 256 frames
    if (bullet_timer == 0) {
        pattern_type = (pattern_type + 1) & 0x07;
        pattern_phase = 0;
    }

    switch (pattern_type) {
        case 0:
            // === SPIRAL GALAXY ===
            // Rotating spiral arms, like a galaxy
            if ((bullet_timer & 0x07) == 0) {
                angle_idx = (pattern_phase) & 0x07;
                dx = cos_table[angle_idx];
                dy = 1 + ((sin_table[angle_idx] + 2) >> 1);  // 1-2 speed
                spawn_bullet(cx, cy, dx, dy);

                // Second arm (opposite)
                angle_idx = (pattern_phase + 4) & 0x07;
                dx = cos_table[angle_idx];
                dy = 1 + ((sin_table[angle_idx] + 2) >> 1);
                spawn_bullet(cx, cy, dx, dy);

                ++pattern_phase;
            }
            break;

        case 1:
            // === FLOWER BLOOM ===
            // Petals expanding outward
            if ((bullet_timer & 0x1F) == 0) {
                for (i = 0; i < 6; ++i) {
                    angle_idx = (i + pattern_phase) & 0x07;
                    dx = cos_table[angle_idx];
                    dy = 1;
                    spawn_bullet(cx, cy, dx, dy);
                }
                ++pattern_phase;
            }
            break;

        case 2:
            // === DNA HELIX ===
            // Double helix pattern
            if ((bullet_timer & 0x0F) == 0) {
                angle_idx = pattern_phase & 0x07;
                // Strand 1
                dx = cos_table[angle_idx];
                spawn_bullet(cx - 8, cy, dx, 1);
                // Strand 2 (opposite phase)
                dx = cos_table[(angle_idx + 4) & 0x07];
                spawn_bullet(cx + 8, cy, dx, 1);
                ++pattern_phase;
            }
            break;

        case 3:
            // === SACRED GEOMETRY - Hexagon ===
            // Expanding hexagonal pattern
            if ((bullet_timer & 0x3F) == 0) {
                // 6 points of hexagon
                spawn_bullet(cx, cy, 0, 2);       // Down
                spawn_bullet(cx, cy, 2, 1);       // Down-right
                spawn_bullet(cx, cy, 2, -1);      // Up-right (but still moves down overall)
                spawn_bullet(cx, cy, -2, 1);      // Down-left
                spawn_bullet(cx, cy, -2, -1);     // Up-left
                spawn_bullet(cx, cy, 0, 1);       // Slow down
            }
            break;

        case 4:
            // === WAVE INTERFERENCE ===
            // Sine wave bullets from two sources
            if ((bullet_timer & 0x0F) == 0) {
                angle_idx = pattern_phase & 0x07;
                // Left wave source
                dx = sin_table[angle_idx];
                spawn_bullet(cx - 16, cy, dx, 1);
                // Right wave source (opposite phase)
                dx = sin_table[(angle_idx + 4) & 0x07];
                spawn_bullet(cx + 16, cy, dx, 1);
                ++pattern_phase;
            }
            break;

        case 5:
            // === MANDALA BURST ===
            // Circular burst that rotates
            if ((bullet_timer & 0x3F) == 0) {
                for (i = 0; i < 8; ++i) {
                    angle_idx = (i + pattern_phase) & 0x07;
                    dx = cos_table[angle_idx];
                    dy = (sin_table[angle_idx] >> 1) + 1;  // Mostly downward
                    spawn_bullet(cx, cy, dx, dy);
                }
                pattern_phase += 1;
            }
            break;

        case 6:
            // === FIBONACCI SPIRAL ===
            // Approximation of golden ratio spiral
            if ((bullet_timer & 0x0B) == 0) {  // Prime-ish interval
                angle_idx = (pattern_phase * 5) & 0x07;  // Golden angle approx
                dx = cos_table[angle_idx];
                dy = 1;
                spawn_bullet(cx, cy, dx, dy);
                ++pattern_phase;
            }
            break;

        case 7:
            // === CROSS ROTATION ===
            // Rotating cross/plus pattern
            if ((bullet_timer & 0x1F) == 0) {
                // Vertical line
                spawn_bullet(cx, cy, 0, 2);
                spawn_bullet(cx, cy, 0, 1);
                // Horizontal line (rotated by phase)
                if (pattern_phase & 1) {
                    spawn_bullet(cx, cy, 2, 1);
                    spawn_bullet(cx, cy, -2, 1);
                } else {
                    spawn_bullet(cx, cy, 1, 1);
                    spawn_bullet(cx, cy, -1, 1);
                }
                ++pattern_phase;
            }
            break;
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

            if (dx < 10 && dy < 10) {
                --player_hp;
                player_inv = 60;
                bullet_on[i] = 0;
                if (player_hp == 0) {
                    game_state = STATE_GAMEOVER; music_play(3);
                }
            }
        }
    }
}

// Initialize game
static void init_game(void) {
    unsigned char i;

    player_x = PLAYER_START_X;
    player_y = PLAYER_START_Y;
    player_hp = PLAYER_MAX_HP;
    player_inv = 0;

    enemy_on = 0;
    position = 2;
    lap_count = 0;
    score = 0;
    distance = 0;
    scroll_y = 0;

    for (i = 0; i < 4; ++i) {
        obs_on[i] = 0;
    }

    // Clear all bullets
    for (i = 0; i < MAX_BULLETS; ++i) {
        bullet_on[i] = 0;
    }
    bullet_timer = 0;
    pattern_phase = 0;
    pattern_type = 0;

    draw_road();
    prepare_enemy();  // Start with warning marker
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
        if (player_y > 32) player_y -= PLAYER_SPEED;
    }
    if (pad_now & BTN_DOWN) {
        if (player_y < SCREEN_HEIGHT - 32) player_y += PLAYER_SPEED;
    }

    // Pit stop check (left edge of road)
    if (player_x <= ROAD_LEFT + 8 && player_hp < PLAYER_MAX_HP) {
        if ((frame_count & 0x3F) == 0) {
            game_state = STATE_PITSTOP;
            pit_timer = 0;
        }
    }

    if (player_inv > 0) --player_inv;
}

// Update enemy
static void update_enemy(void) {
    // Warning phase - countdown before spawn
    if (!enemy_on && enemy_warn_timer > 0) {
        --enemy_warn_timer;
        if (enemy_warn_timer == 0) {
            spawn_enemy();
        }
        return;
    }

    // No enemy and no warning - prepare next one
    if (!enemy_on) {
        prepare_enemy();
        return;
    }

    enemy_y += enemy_speed;

    // Racing AI - enemy tries to block player's overtaking path
    // Enemy moves toward player's X position to obstruct
    if ((frame_count & 0x07) == 0) {
        if (enemy_x + 8 < player_x && enemy_x < ROAD_RIGHT - 24) {
            enemy_x += 1;  // Move right to block
        } else if (enemy_x > player_x + 8 && enemy_x > ROAD_LEFT + 8) {
            enemy_x -= 1;  // Move left to block
        }
    }

    // Overtaken
    if (enemy_y > SCREEN_HEIGHT) {
        enemy_on = 0;
        score += 50;
        position = 1;
        if (lap_count < 3) {
            prepare_enemy();  // Show warning for next enemy
            position = 2;
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

    // Enemy collision
    if (enemy_on) {
        dx = abs_diff(player_x, enemy_x);
        dy = abs_diff(player_y, enemy_y);

        if (dx < 14 && dy < 14) {
            --player_hp;
            player_inv = 60;
            if (player_hp == 0) {
                game_state = STATE_GAMEOVER; music_play(3);
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
                    game_state = STATE_GAMEOVER; music_play(3);
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

    ++distance;
    if (distance >= 1000) {
        distance = 0;
        ++lap_count;
        score += 100;
        if (position == 1) score += 100;

        // Recover 3 HP on lap completion
        player_hp += 3;
        if (player_hp > PLAYER_MAX_HP) {
            player_hp = PLAYER_MAX_HP;
        }

        if (lap_count >= 3 && position == 1) {
            game_state = STATE_WIN;
            init_win_animation();
            music_play(2);  // Victory fanfare!
        }
    }

    if ((frame_count & 0x3F) == 0) {
        spawn_obstacle();
    }

    scroll_y += SCROLL_SPEED;
}

// Draw game sprites
static void draw_game(void) {
    unsigned char id = 0;
    unsigned char i;

    // Player car
    if (player_inv == 0 || (frame_count & 4)) {
        id = set_car(id, player_x, player_y, SPR_CAR, 0);
    }

    // Warning marker for next enemy (blinking)
    if (!enemy_on && enemy_warn_timer > 0 && (frame_count & 8)) {
        // Draw arrow pointing down at spawn position
        id = set_sprite(id, enemy_next_x + 4, 8, 0x0A, 1 | 0x80);  // Arrow, flipped vertically
        id = set_sprite(id, enemy_next_x + 4, 16, 0x0A, 1 | 0x80);
    }

    // Enemy car
    if (enemy_on) {
        id = set_car(id, enemy_x, enemy_y, SPR_CAR + 4, 1);
    }

    // Obstacles
    for (i = 0; i < 4; ++i) {
        if (obs_on[i]) {
            id = set_sprite(id, obs_x[i], obs_y[i], SPR_OBSTACLE, 2);
        }
    }

    // Bullets (danmaku)
    for (i = 0; i < MAX_BULLETS; ++i) {
        if (bullet_on[i] && id < 60) {  // Leave room for HUD
            id = set_sprite(id, bullet_x[i], bullet_y[i], SPR_BULLET, 1);
        }
    }

    // HUD - Position (P1 or P2)
    id = set_sprite(id, 8, 8, SPR_LETTER + 15, 3);  // P
    id = set_sprite(id, 16, 8, SPR_DIGIT + position, 3);

    // HUD - HP (hearts or number)
    id = set_sprite(id, 8, 20, SPR_LETTER + 7, 3);  // H
    id = set_sprite(id, 16, 20, SPR_DIGIT + player_hp, 3);

    // HUD - Lap counter "LAP X/3" at top center
    id = set_sprite(id, 100, 8, SPR_LETTER + 11, 3);  // L
    id = set_sprite(id, 108, 8, SPR_LETTER + 0, 3);   // A
    id = set_sprite(id, 116, 8, SPR_LETTER + 15, 3);  // P
    id = set_sprite(id, 128, 8, SPR_DIGIT + lap_count + 1, 3);  // Current lap
    id = set_sprite(id, 136, 8, SPR_SLASH, 3);        // "/"
    id = set_sprite(id, 144, 8, SPR_DIGIT + 3, 3);    // 3 (total laps)

    // HUD - Progress bar at bottom of screen
    {
        unsigned char progress;
        unsigned char bar_x;
        unsigned char bar_filled;
        unsigned char car_pos;

        // Calculate progress (0-100)
        progress = (unsigned char)(distance / 10);
        if (progress > 100) progress = 100;

        // Draw progress bar background (empty boxes)
        for (bar_x = 0; bar_x < 8 && id < 58; ++bar_x) {
            id = set_sprite(id, 64 + bar_x * 10, 224, SPR_BAR_EMPTY, 3);
        }

        // Draw filled portion
        bar_filled = progress / 13;  // 0-7 segments
        for (bar_x = 0; bar_x < bar_filled && bar_x < 8 && id < 60; ++bar_x) {
            id = set_sprite(id, 64 + bar_x * 10, 224, SPR_BAR_FILL, 3);
        }

        // Draw car icon at current position
        car_pos = 64 + ((progress * 70) / 100);
        id = set_sprite(id, car_pos, 216, SPR_CAR_ICON, 0);

        // Draw "GOAL" at end
        id = set_sprite(id, 152, 224, SPR_LETTER + 6, 3);  // G
        id = set_sprite(id, 160, 224, SPR_LETTER + 14, 3); // O
        id = set_sprite(id, 168, 224, SPR_LETTER + 0, 3);  // A
        id = set_sprite(id, 176, 224, SPR_LETTER + 11, 3); // L
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
    unsigned char x = 100, y = 100;

    // "RACE"
    id = set_sprite(id, x,      y, SPR_LETTER + 17, 0);  // R
    id = set_sprite(id, x + 8,  y, SPR_LETTER + 0,  0);  // A
    id = set_sprite(id, x + 16, y, SPR_LETTER + 2,  0);  // C
    id = set_sprite(id, x + 24, y, SPR_LETTER + 4,  0);  // E

    // Blinking car
    if (frame_count & 0x20) {
        id = set_car(id, x + 4, y + 24, SPR_CAR, 0);
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

    // Score value
    y = 165;
    x = 96;
    s = score;
    if (s > 999) s = 999;  // Cap display
    id = set_sprite(id, x,      y, SPR_DIGIT + (unsigned char)(s / 100), 3);
    id = set_sprite(id, x + 8,  y, SPR_DIGIT + (unsigned char)((s / 10) % 10), 3);
    id = set_sprite(id, x + 16, y, SPR_DIGIT + (unsigned char)(s % 10), 3);
    id = set_sprite(id, x + 24, y, SPR_LETTER + 15, 3);  // P
    id = set_sprite(id, x + 32, y, SPR_LETTER + 19, 3);  // T (PTS)
    id = set_sprite(id, x + 40, y, SPR_LETTER + 18, 3);  // S

    // === "PRESS START" blinking prompt ===
    if (win_timer > 90 && (frame_count & 0x20)) {
        y = 200;
        x = 72;
        id = set_sprite(id, x,      y, SPR_LETTER + 15, 3);  // P
        id = set_sprite(id, x + 8,  y, SPR_LETTER + 17, 3);  // R
        id = set_sprite(id, x + 16, y, SPR_LETTER + 4,  3);  // E
        id = set_sprite(id, x + 24, y, SPR_LETTER + 18, 3);  // S
        id = set_sprite(id, x + 32, y, SPR_LETTER + 18, 3);  // S

        x = 128;
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
                    draw_game();
                }
                break;

            case STATE_PAUSED:
                draw_pause();
                if (pad_new & BTN_START) {
                    game_state = STATE_RACING;
                }
                break;

            case STATE_PITSTOP:
                ++pit_timer;
                if (pit_timer >= 60) {
                    player_hp = PLAYER_MAX_HP;
                    game_state = STATE_RACING;
                }
                draw_game();
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
        }
    }
}
