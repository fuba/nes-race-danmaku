; Minimal NES library for cc65
; Based on neslib by Shiru

.export _pal_all, _pal_bg, _pal_spr, _pal_col, _pal_clear, _pal_bright
.export _ppu_wait_nmi, _ppu_off, _ppu_on_all, _ppu_on_bg, _ppu_on_spr
.export _vram_adr, _vram_put, _vram_fill, _vram_write
.export _oam_clear, _oam_spr, _oam_meta_spr, _oam_hide_rest
.export _pad_poll, _pad_trigger, _pad_state
.export _scroll
.export _set_prg_bank, _set_chr_bank

; cc65 runtime imports
.import popa, popax
.importzp sp, ptr1, tmp1, tmp2, tmp3, tmp4

; Hardware registers
PPU_CTRL   = $2000
PPU_MASK   = $2001
PPU_STATUS = $2002
PPU_SCROLL = $2005
PPU_ADDR   = $2006
PPU_DATA   = $2007
OAM_ADDR   = $2003
OAM_DMA    = $4014
JOYPAD1    = $4016
JOYPAD2    = $4017

; OAM buffer at $0200
OAM_BUF    = $0200

.segment "ZEROPAGE"

nmi_ready:      .res 1      ; NMI ready flag
nmi_count:      .res 1      ; NMI counter
ppu_ctrl_val:   .res 1      ; Shadow of PPU_CTRL
ppu_mask_val:   .res 1      ; Shadow of PPU_MASK
scroll_x:       .res 1      ; Scroll X position
scroll_y:       .res 1      ; Scroll Y position
pad1_state:     .res 1      ; Pad 1 current state
pad1_prev:      .res 1      ; Pad 1 previous state
pad2_state:     .res 1      ; Pad 2 current state
pad2_prev:      .res 1      ; Pad 2 previous state

.segment "BSS"

pal_buf:        .res 32     ; Palette buffer
pal_update:     .res 1      ; Palette update flag

.segment "CODE"

;---------------------------------------
; Wait for NMI (vertical blank)
;---------------------------------------
_ppu_wait_nmi:
    lda #1
    sta nmi_ready
@wait:
    lda nmi_ready
    bne @wait
    rts

;---------------------------------------
; Turn off PPU
;---------------------------------------
_ppu_off:
    lda #$00
    sta ppu_mask_val
    sta PPU_MASK
    rts

;---------------------------------------
; Turn on PPU (all)
;---------------------------------------
_ppu_on_all:
    lda #$1E            ; Show BG and sprites
    sta ppu_mask_val
    sta PPU_MASK
    rts

;---------------------------------------
; Turn on PPU (BG only)
;---------------------------------------
_ppu_on_bg:
    lda #$0A            ; Show BG only
    sta ppu_mask_val
    sta PPU_MASK
    rts

;---------------------------------------
; Turn on PPU (sprites only)
;---------------------------------------
_ppu_on_spr:
    lda #$14            ; Show sprites only
    sta ppu_mask_val
    sta PPU_MASK
    rts

;---------------------------------------
; Set all palettes (32 bytes)
;---------------------------------------
_pal_all:
    sta ptr1
    stx ptr1+1
    ldy #0
@loop:
    lda (ptr1), y
    sta pal_buf, y
    iny
    cpy #32
    bne @loop
    lda #1
    sta pal_update
    rts

;---------------------------------------
; Set BG palettes (16 bytes)
;---------------------------------------
_pal_bg:
    sta ptr1
    stx ptr1+1
    ldy #0
@loop:
    lda (ptr1), y
    sta pal_buf, y
    iny
    cpy #16
    bne @loop
    lda #1
    sta pal_update
    rts

;---------------------------------------
; Set sprite palettes (16 bytes)
;---------------------------------------
_pal_spr:
    sta ptr1
    stx ptr1+1
    ldy #0
@loop:
    lda (ptr1), y
    sta pal_buf+16, y
    iny
    cpy #16
    bne @loop
    lda #1
    sta pal_update
    rts

;---------------------------------------
; Set single palette color
; A = color, stack has index
;---------------------------------------
_pal_col:
    sta tmp1            ; Save color
    jsr popa            ; Get index
    tax
    lda tmp1
    sta pal_buf, x
    lda #1
    sta pal_update
    rts

;---------------------------------------
; Clear palette
;---------------------------------------
_pal_clear:
    lda #$0F
    ldx #0
@loop:
    sta pal_buf, x
    inx
    cpx #32
    bne @loop
    lda #1
    sta pal_update
    rts

;---------------------------------------
; Set brightness (not fully implemented)
;---------------------------------------
_pal_bright:
    ; Just store, actual brightness would need implementation
    rts

;---------------------------------------
; Set VRAM address
;---------------------------------------
_vram_adr:
    stx PPU_ADDR        ; High byte
    sta PPU_ADDR        ; Low byte
    rts

;---------------------------------------
; Write byte to VRAM
;---------------------------------------
_vram_put:
    sta PPU_DATA
    rts

;---------------------------------------
; Fill VRAM
; A = data, stack has length
;---------------------------------------
_vram_fill:
    sta tmp1            ; Save data
    jsr popax           ; Get length
    sta tmp2            ; Length low
    stx tmp3            ; Length high
    lda tmp1
    ldx tmp3
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
    ldy tmp2
    beq @done
@loop_lo:
    sta PPU_DATA
    dey
    bne @loop_lo
@done:
    rts

;---------------------------------------
; Write data to VRAM
;---------------------------------------
_vram_write:
    ; Get length from stack
    jsr popax
    sta tmp2
    stx tmp3
    ; Get data pointer
    jsr popax
    sta ptr1
    stx ptr1+1
    ; Write loop
    ldy #0
@loop:
    lda tmp2
    ora tmp3
    beq @done
    lda (ptr1), y
    sta PPU_DATA
    iny
    bne @no_inc
    inc ptr1+1
@no_inc:
    lda tmp2
    bne @dec_lo
    dec tmp3
@dec_lo:
    dec tmp2
    jmp @loop
@done:
    rts

;---------------------------------------
; Clear OAM buffer
;---------------------------------------
_oam_clear:
    lda #$FF
    ldx #0
@loop:
    sta OAM_BUF, x      ; Y = 255 (offscreen)
    inx
    inx
    inx
    inx
    bne @loop
    rts

;---------------------------------------
; Set single sprite
; Returns sprid + 4
; A = sprid, stack has: x, y, chrnum, attr
;---------------------------------------
_oam_spr:
    tax                 ; X = sprid
    ; Get attr
    jsr popa
    sta OAM_BUF+2, x    ; Attribute
    ; Get chrnum
    jsr popa
    sta OAM_BUF+1, x    ; Tile
    ; Get y
    jsr popa
    sta OAM_BUF, x      ; Y position
    ; Get x
    jsr popa
    sta OAM_BUF+3, x    ; X position
    ; Return sprid + 4
    txa
    clc
    adc #4
    ldx #0
    rts

;---------------------------------------
; Set metasprite
; Returns new sprid
;---------------------------------------
_oam_meta_spr:
    ; Get data pointer
    jsr popax
    sta ptr1
    stx ptr1+1
    ; Get sprid
    jsr popa
    sta tmp4            ; sprid
    ; Get y
    jsr popa
    sta tmp2            ; base Y
    ; Get x
    jsr popa
    sta tmp1            ; base X

    ldy #0
@loop:
    lda (ptr1), y       ; X offset
    cmp #128
    beq @done
    clc
    adc tmp1            ; Add base X
    sta tmp3            ; Save X
    iny
    lda (ptr1), y       ; Y offset
    clc
    adc tmp2            ; Add base Y
    ldx tmp4
    sta OAM_BUF, x      ; Store Y
    iny
    lda (ptr1), y       ; Tile
    sta OAM_BUF+1, x
    iny
    lda (ptr1), y       ; Attribute
    sta OAM_BUF+2, x
    lda tmp3
    sta OAM_BUF+3, x    ; Store X
    iny
    ; Next sprite
    lda tmp4
    clc
    adc #4
    sta tmp4
    jmp @loop
@done:
    lda tmp4
    ldx #0
    rts

;---------------------------------------
; Hide remaining sprites
;---------------------------------------
_oam_hide_rest:
    tax
@loop:
    cpx #0              ; 256 sprites = overflow to 0
    beq @done
    lda #$FF
    sta OAM_BUF, x
    inx
    inx
    inx
    inx
    bne @loop
@done:
    rts

;---------------------------------------
; Poll controller
;---------------------------------------
_pad_poll:
    tax                 ; Pad number
    ; Strobe
    lda #1
    sta JOYPAD1
    lda #0
    sta JOYPAD1

    cpx #0
    bne @pad2

    ; Read pad 1
    lda pad1_state
    sta pad1_prev
    ldy #8
    lda #0
@read1:
    pha
    lda JOYPAD1
    and #$03
    cmp #1
    pla
    rol a
    dey
    bne @read1
    sta pad1_state
    ldx #0
    rts

@pad2:
    ; Read pad 2
    lda pad2_state
    sta pad2_prev
    ldy #8
    lda #0
@read2:
    pha
    lda JOYPAD2
    and #$03
    cmp #1
    pla
    rol a
    dey
    bne @read2
    sta pad2_state
    ldx #0
    rts

;---------------------------------------
; Trigger mode (press detection)
;---------------------------------------
_pad_trigger:
    jsr _pad_poll
    tax
    cpx #0
    bne @pad2_trig
    lda pad1_state
    eor pad1_prev
    and pad1_state
    ldx #0
    rts
@pad2_trig:
    lda pad2_state
    eor pad2_prev
    and pad2_state
    ldx #0
    rts

;---------------------------------------
; Get pad state without polling
;---------------------------------------
_pad_state:
    tax
    cpx #0
    bne @pad2_state
    lda pad1_state
    ldx #0
    rts
@pad2_state:
    lda pad2_state
    ldx #0
    rts

;---------------------------------------
; Set scroll position
; A = X low, X = X high (ignored for simple scroll)
; Stack has Y
;---------------------------------------
_scroll:
    sta scroll_x        ; X scroll
    jsr popax
    sta scroll_y        ; Y scroll low
    rts

;---------------------------------------
; Bank switching (NOP for NROM)
;---------------------------------------
_set_prg_bank:
    rts

_set_chr_bank:
    rts

;---------------------------------------
; NMI handler (called from crt0)
;---------------------------------------
.export nmi_handler
nmi_handler:
    pha
    txa
    pha
    tya
    pha

    ; OAM DMA
    lda #0
    sta OAM_ADDR
    lda #>OAM_BUF
    sta OAM_DMA

    ; Update palette if needed
    lda pal_update
    beq @no_pal
    lda #$3F
    sta PPU_ADDR
    lda #$00
    sta PPU_ADDR
    ldx #0
@pal_loop:
    lda pal_buf, x
    sta PPU_DATA
    inx
    cpx #32
    bne @pal_loop
    lda #0
    sta pal_update
@no_pal:

    ; Reset scroll
    lda #0
    sta PPU_ADDR
    sta PPU_ADDR
    lda scroll_x
    sta PPU_SCROLL
    lda scroll_y
    sta PPU_SCROLL

    ; Clear NMI flag
    lda #0
    sta nmi_ready

    ; Increment counter
    inc nmi_count

    pla
    tay
    pla
    tax
    pla
    rti
