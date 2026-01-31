; NES helper functions for cc65
; Low-level PPU and controller routines

.export _ppu_wait_vblank
.export _ppu_off
.export _ppu_on_all
.export _ppu_set_addr
.export _ppu_write
.export _ppu_fill
.export _set_scroll
.export _read_joypad
.export _clear_sprites

.importzp sp

; PPU registers
PPU_CTRL   = $2000
PPU_MASK   = $2001
PPU_STATUS = $2002
PPU_SCROLL = $2005
PPU_ADDR   = $2006
PPU_DATA   = $2007
OAM_DMA    = $4014
JOYPAD1    = $4016
JOYPAD2    = $4017

.segment "CODE"

; Wait for vblank (NMI)
; void ppu_wait_vblank(void)
_ppu_wait_vblank:
    lda PPU_STATUS          ; Clear vblank flag
@wait:
    lda PPU_STATUS
    bpl @wait               ; Wait until bit 7 set
    rts

; Turn off PPU rendering
; void ppu_off(void)
_ppu_off:
    lda #$00
    sta PPU_MASK
    rts

; Turn on PPU with sprites and background
; void ppu_on_all(void)
_ppu_on_all:
    lda #$1E                ; Show BG and sprites, no clipping
    sta PPU_MASK
    rts

; Set PPU address for VRAM access
; void ppu_set_addr(uint16_t addr)
; A = low byte, X = high byte (fastcall)
_ppu_set_addr:
    stx PPU_ADDR            ; High byte first
    sta PPU_ADDR            ; Then low byte
    rts

; Write single byte to PPU
; void ppu_write(uint8_t data)
_ppu_write:
    sta PPU_DATA
    rts

; Fill VRAM with value
; void ppu_fill(uint16_t addr, uint8_t value, uint16_t count)
; Stack: [addr_lo, addr_hi, value, count_lo, count_hi]
_ppu_fill:
    ; Get parameters from stack
    ldy #0
    lda (sp), y             ; count_lo
    sta tmp_count
    iny
    lda (sp), y             ; count_hi
    sta tmp_count+1
    iny
    lda (sp), y             ; value
    sta tmp_value
    iny
    lda (sp), y             ; addr_lo
    sta tmp_addr
    iny
    lda (sp), y             ; addr_hi
    sta tmp_addr+1

    ; Set PPU address
    lda tmp_addr+1
    sta PPU_ADDR
    lda tmp_addr
    sta PPU_ADDR

    ; Fill loop
    lda tmp_value
    ldx tmp_count+1         ; High byte of count
    beq @do_low
@loop_hi:
    ldy #0
@loop_256:
    sta PPU_DATA
    iny
    bne @loop_256
    dex
    bne @loop_hi
@do_low:
    ldy tmp_count           ; Low byte of count
    beq @done
@loop_lo:
    sta PPU_DATA
    dey
    bne @loop_lo
@done:
    ; Fix stack
    lda sp
    clc
    adc #5
    sta sp
    bcc @no_carry
    inc sp+1
@no_carry:
    rts

; Set scroll position
; void set_scroll(uint8_t x, uint8_t y)
; A = x, stack has y
_set_scroll:
    sta PPU_SCROLL          ; X scroll
    ldy #0
    lda (sp), y             ; Get Y from stack
    sta PPU_SCROLL          ; Y scroll
    ; Fix stack
    inc sp
    bne @done
    inc sp+1
@done:
    rts

; Read joypad state
; uint8_t read_joypad(uint8_t pad)
; A = pad number (0 or 1)
_read_joypad:
    tax                     ; Save pad number
    lda #1
    sta JOYPAD1             ; Strobe on
    lda #0
    sta JOYPAD1             ; Strobe off

    ; Select joypad address
    cpx #0
    beq @pad1
    ; Read pad 2
    ldy #8
    lda #0
@read_pad2:
    pha
    lda JOYPAD2
    and #$03                ; Bits 0-1
    cmp #1                  ; Carry = button state
    pla
    rol a
    dey
    bne @read_pad2
    rts
@pad1:
    ; Read pad 1
    ldy #8
    lda #0
@read_pad1:
    pha
    lda JOYPAD1
    and #$03                ; Bits 0-1
    cmp #1                  ; Carry = button state
    pla
    rol a
    dey
    bne @read_pad1
    rts

; Clear all sprites (move offscreen)
; void clear_sprites(void)
_clear_sprites:
    lda #$FF                ; Y = 255 (offscreen)
    ldx #0
@loop:
    sta $0200, x            ; OAM buffer at $0200
    inx
    inx
    inx
    inx                     ; Next sprite (4 bytes)
    bne @loop
    rts

.segment "ZEROPAGE"
tmp_addr:   .res 2
tmp_value:  .res 1
tmp_count:  .res 2
