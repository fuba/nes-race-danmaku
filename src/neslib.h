// Minimal NES library for cc65
// Based on neslib by Shiru

#ifndef _NESLIB_H
#define _NESLIB_H

// Nametable addresses
#define NAMETABLE_A     0x2000
#define NAMETABLE_B     0x2400
#define NAMETABLE_C     0x2800
#define NAMETABLE_D     0x2C00

// Palette functions
void __fastcall__ pal_all(const unsigned char *data);
void __fastcall__ pal_bg(const unsigned char *data);
void __fastcall__ pal_spr(const unsigned char *data);
void __fastcall__ pal_col(unsigned char index, unsigned char color);
void __fastcall__ pal_clear(void);
void __fastcall__ pal_bright(unsigned char bright);

// PPU functions
void __fastcall__ ppu_wait_nmi(void);
void __fastcall__ ppu_off(void);
void __fastcall__ ppu_on_all(void);
void __fastcall__ ppu_on_bg(void);
void __fastcall__ ppu_on_spr(void);

// VRAM functions
void __fastcall__ vram_adr(unsigned int adr);
void __fastcall__ vram_put(unsigned char data);
void __fastcall__ vram_fill(unsigned char data, unsigned int len);
void __fastcall__ vram_write(const unsigned char *data, unsigned int len);

// OAM functions
void __fastcall__ oam_clear(void);
unsigned char __fastcall__ oam_spr(unsigned char x, unsigned char y,
                                   unsigned char chrnum, unsigned char attr,
                                   unsigned char sprid);
unsigned char __fastcall__ oam_meta_spr(unsigned char x, unsigned char y,
                                        unsigned char sprid,
                                        const unsigned char *data);
void __fastcall__ oam_hide_rest(unsigned char sprid);

// Controller functions
unsigned char __fastcall__ pad_poll(unsigned char pad);
unsigned char __fastcall__ pad_trigger(unsigned char pad);
unsigned char __fastcall__ pad_state(unsigned char pad);

// Scroll function
void __fastcall__ scroll(unsigned int x, unsigned int y);

// Bank switching (not used for NROM)
void __fastcall__ set_prg_bank(unsigned char bank);
void __fastcall__ set_chr_bank(unsigned char bank);

#endif // _NESLIB_H
