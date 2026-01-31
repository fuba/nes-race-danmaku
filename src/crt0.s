; Minimal NES CRT0 for cc65
; Uses nes.lib for runtime functions

.export __STARTUP__ : absolute = 1
.export _exit

.import _main
.import initlib, donelib
.import zerobss, copydata
.importzp sp

; Stack is at top of SRAM ($0300-$07FF)
; We'll put C stack at $0700-$0800

.segment "HEADER"
; iNES header (16 bytes)
    .byte $4E, $45, $53, $1A    ; "NES" + $1A
    .byte $01                   ; 16KB PRG-ROM (1 x 16KB)
    .byte $01                   ; 8KB CHR-ROM (1 x 8KB)
    .byte $01                   ; Flags 6: Horizontal mirroring, no SRAM
    .byte $00                   ; Flags 7: Mapper 0 (NROM)
    .byte $00, $00, $00, $00    ; Padding
    .byte $00, $00, $00, $00

.segment "STARTUP"

reset:
    sei                     ; Disable interrupts
    cld                     ; Clear decimal mode
    ldx #$40
    stx $4017               ; Disable APU frame IRQ
    ldx #$FF
    txs                     ; Set up hardware stack
    inx                     ; X = 0
    stx $2000               ; Disable NMI
    stx $2001               ; Disable rendering
    stx $4010               ; Disable DMC IRQs

; Wait for first vblank
@vbl1:
    bit $2002
    bpl @vbl1

; Clear RAM $0000-$07FF
    lda #$00
    ldx #$00
@clear:
    sta $0000, x
    sta $0100, x
    sta $0200, x
    sta $0300, x
    sta $0400, x
    sta $0500, x
    sta $0600, x
    sta $0700, x
    inx
    bne @clear

; Set OAM to offscreen
    lda #$FF
    ldx #$00
@oam:
    sta $0200, x
    inx
    bne @oam

; Wait for second vblank
@vbl2:
    bit $2002
    bpl @vbl2

; Initialize cc65 runtime
    jsr zerobss
    jsr copydata

; Set up C stack pointer at $0800 (top of usable RAM)
    lda #$00
    sta sp
    lda #$08
    sta sp+1

; Initialize C library
    jsr initlib

; Jump to main
    jmp _main

; Exit function
_exit:
    jsr donelib
@hang:
    jmp @hang

; NMI handler (minimal)
nmi:
    rti

; IRQ handler (not used)
irq:
    rti

.segment "VECTORS"
    .word nmi               ; NMI vector
    .word reset             ; RESET vector
    .word irq               ; IRQ vector
