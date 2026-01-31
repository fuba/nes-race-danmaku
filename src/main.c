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
#define APU_STATUS  (*(volatile unsigned char*)0x4015)

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
                    game_state = STATE_GAMEOVER;
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
                game_state = STATE_GAMEOVER;
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
                    game_state = STATE_GAMEOVER;
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

        if (lap_count >= 3 && position == 1) {
            game_state = STATE_WIN;
            init_win_animation();
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

        // State machine
        switch (game_state) {
            case STATE_TITLE:
                draw_title();
                if (pad_new & BTN_START) {
                    init_game();
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
                    game_state = STATE_TITLE;
                }
                break;

            case STATE_WIN:
                update_win_animation();
                draw_win();
                // Only accept START after animation plays (about 1.5 seconds)
                if (win_timer > 90 && (pad_new & BTN_START)) {
                    game_state = STATE_TITLE;
                }
                break;
        }
    }
}
