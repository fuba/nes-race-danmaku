// NES hardware definitions and helper functions
#ifndef NES_H
#define NES_H

#include <stdint.h>

// PPU registers
#define PPU_CTRL    (*(volatile uint8_t*)0x2000)
#define PPU_MASK    (*(volatile uint8_t*)0x2001)
#define PPU_STATUS  (*(volatile uint8_t*)0x2002)
#define OAM_ADDR    (*(volatile uint8_t*)0x2003)
#define OAM_DATA    (*(volatile uint8_t*)0x2004)
#define PPU_SCROLL  (*(volatile uint8_t*)0x2005)
#define PPU_ADDR    (*(volatile uint8_t*)0x2006)
#define PPU_DATA    (*(volatile uint8_t*)0x2007)
#define OAM_DMA     (*(volatile uint8_t*)0x4014)

// APU registers
#define APU_PULSE1_CTRL  (*(volatile uint8_t*)0x4000)
#define APU_PULSE1_SWEEP (*(volatile uint8_t*)0x4001)
#define APU_PULSE1_LO    (*(volatile uint8_t*)0x4002)
#define APU_PULSE1_HI    (*(volatile uint8_t*)0x4003)
#define APU_STATUS       (*(volatile uint8_t*)0x4015)
#define APU_FRAME        (*(volatile uint8_t*)0x4017)

// Controller registers
#define JOYPAD1     (*(volatile uint8_t*)0x4016)
#define JOYPAD2     (*(volatile uint8_t*)0x4017)

// Controller button masks
#define BTN_A       0x80
#define BTN_B       0x40
#define BTN_SELECT  0x20
#define BTN_START   0x10
#define BTN_UP      0x08
#define BTN_DOWN    0x04
#define BTN_LEFT    0x02
#define BTN_RIGHT   0x01

// PPU_CTRL flags
#define PPUCTRL_NMI         0x80  // Enable NMI
#define PPUCTRL_SPRITE_SIZE 0x20  // 8x16 sprites
#define PPUCTRL_BG_ADDR     0x10  // BG pattern table at $1000
#define PPUCTRL_SPR_ADDR    0x08  // Sprite pattern table at $1000
#define PPUCTRL_INC32       0x04  // VRAM address increment 32
#define PPUCTRL_NT_2000     0x00  // Base nametable $2000
#define PPUCTRL_NT_2400     0x01  // Base nametable $2400
#define PPUCTRL_NT_2800     0x02  // Base nametable $2800
#define PPUCTRL_NT_2C00     0x03  // Base nametable $2C00

// PPU_MASK flags
#define PPUMASK_BLUE        0x80  // Emphasize blue
#define PPUMASK_GREEN       0x40  // Emphasize green
#define PPUMASK_RED         0x20  // Emphasize red
#define PPUMASK_SPR         0x10  // Show sprites
#define PPUMASK_BG          0x08  // Show background
#define PPUMASK_SPR_CLIP    0x04  // Show sprites in leftmost 8 pixels
#define PPUMASK_BG_CLIP     0x02  // Show BG in leftmost 8 pixels
#define PPUMASK_GRAYSCALE   0x01  // Grayscale mode

// Nametable addresses
#define NAMETABLE_A 0x2000
#define NAMETABLE_B 0x2400
#define NAMETABLE_C 0x2800
#define NAMETABLE_D 0x2C00

// Palette addresses
#define PALETTE_BG  0x3F00
#define PALETTE_SPR 0x3F10

// OAM buffer (256 bytes at $0200)
extern uint8_t oam_buffer[256];
#define OAM ((uint8_t*)0x0200)

// Sprite OAM structure (4 bytes per sprite)
typedef struct {
    uint8_t y;          // Y position (0-239, 0xFF = offscreen)
    uint8_t tile;       // Tile index
    uint8_t attr;       // Attributes: VHP000CC
                        // V=vertical flip, H=horizontal flip, P=priority
                        // CC=palette (0-3)
    uint8_t x;          // X position (0-255)
} Sprite;

// Sprite attribute flags
#define SPR_FLIPV   0x80
#define SPR_FLIPH   0x40
#define SPR_BEHIND  0x20
#define SPR_PAL0    0x00
#define SPR_PAL1    0x01
#define SPR_PAL2    0x02
#define SPR_PAL3    0x03

// Function declarations (implemented in nes.s)
void __fastcall__ ppu_wait_vblank(void);
void __fastcall__ ppu_off(void);
void __fastcall__ ppu_on_all(void);
void __fastcall__ ppu_set_addr(uint16_t addr);
void __fastcall__ ppu_write(uint8_t data);
void __fastcall__ ppu_fill(uint16_t addr, uint8_t value, uint16_t count);
void __fastcall__ set_scroll(uint8_t x, uint8_t y);
uint8_t __fastcall__ read_joypad(uint8_t pad);
void __fastcall__ clear_sprites(void);

#endif // NES_H
