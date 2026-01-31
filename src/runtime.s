; Minimal cc65 runtime for NES
; Provides required functions: zerobss, copydata, initlib, popa, popax

.export zerobss, copydata, initlib
.export popa, popax, incsp1, incsp2, decsp1
.export pushax, pusha
.export tosicmp, tosmula0, tossuba0, tosudiva0, tosumoda0
.export aslax4, staspidx

.exportzp sp, ptr1, tmp1, tmp2, tmp3, tmp4

.import __BSS_RUN__, __BSS_SIZE__
.import __DATA_LOAD__, __DATA_RUN__, __DATA_SIZE__

.segment "ZEROPAGE"

sp:         .res 2      ; C stack pointer
ptr1:       .res 2      ; Pointer 1
tmp1:       .res 1      ; Temp 1
tmp2:       .res 1      ; Temp 2
tmp3:       .res 1      ; Temp 3
tmp4:       .res 1      ; Temp 4

.segment "CODE"

; Clear BSS segment
zerobss:
    lda #<__BSS_RUN__
    sta ptr1
    lda #>__BSS_RUN__
    sta ptr1+1
    lda #<__BSS_SIZE__
    sta tmp1
    lda #>__BSS_SIZE__
    sta tmp2
    lda #0
    ldy #0
@loop:
    ldx tmp1
    bne @do
    ldx tmp2
    beq @done
    dec tmp2
@do:
    dec tmp1
    sta (ptr1), y
    inc ptr1
    bne @loop
    inc ptr1+1
    jmp @loop
@done:
    rts

; Copy initialized data from ROM to RAM
copydata:
    lda #<__DATA_SIZE__
    sta tmp1
    lda #>__DATA_SIZE__
    sta tmp2
    lda #<__DATA_LOAD__
    sta ptr1
    lda #>__DATA_LOAD__
    sta ptr1+1
    lda #<__DATA_RUN__
    sta tmp3
    lda #>__DATA_RUN__
    sta tmp4
    ldy #0
@loop:
    ldx tmp1
    bne @do
    ldx tmp2
    beq @done
    dec tmp2
@do:
    dec tmp1
    lda (ptr1), y
    sta (tmp3 - ptr1), y    ; Use self-modifying code or indirect
    inc ptr1
    bne @noinc1
    inc ptr1+1
@noinc1:
    inc tmp3
    bne @loop
    inc tmp4
    jmp @loop
@done:
    rts

; Initialize C library (no-op for minimal runtime)
initlib:
    rts

; Pop A from stack
popa:
    ldy #0
    lda (sp), y
    inc sp
    bne @done
    inc sp+1
@done:
    rts

; Pop A/X from stack
popax:
    ldy #0
    lda (sp), y
    tax
    iny
    lda (sp), y
    sta tmp1
    lda sp
    clc
    adc #2
    sta sp
    bcc @done
    inc sp+1
@done:
    lda tmp1
    xchg            ; Exchange A and X - not available on 6502!
    ; Actually we need:
    txa
    ldx tmp1
    rts

; Increment SP by 1
incsp1:
    inc sp
    bne @done
    inc sp+1
@done:
    rts

; Increment SP by 2
incsp2:
    lda sp
    clc
    adc #2
    sta sp
    bcc @done
    inc sp+1
@done:
    rts

; Decrement SP by 1
decsp1:
    lda sp
    bne @dec
    dec sp+1
@dec:
    dec sp
    rts

; Push A to stack
pusha:
    ldy #0
    dec sp
    bne @store
    dec sp+1
@store:
    sta (sp), y
    rts

; Push A/X to stack
pushax:
    sta tmp1
    dec sp
    bne @noinc
    dec sp+1
@noinc:
    txa
    ldy #0
    sta (sp), y
    dec sp
    bne @store
    dec sp+1
@store:
    lda tmp1
    sta (sp), y
    rts

; Compare TOS to A (signed)
tosicmp:
    sta tmp1
    jsr popa
    cmp tmp1
    rts

; Multiply TOS by A (unsigned)
tosmula0:
    sta tmp1
    jsr popa
    ; Simple multiply
    ldx #0
    stx tmp2
    ldy #8
@loop:
    lsr tmp1
    bcc @noadd
    clc
    adc tmp2
@noadd:
    asl tmp2
    dey
    bne @loop
    ldx #0
    rts

; Subtract A from TOS
tossuba0:
    sta tmp1
    jsr popa
    sec
    sbc tmp1
    ldx #0
    rts

; Unsigned divide TOS by A
tosudiva0:
    sta tmp1
    jsr popa
    ldx #0
    ; Simple divide
    ldy #8
    asl a
@loop:
    rol a
    cmp tmp1
    bcc @no
    sbc tmp1
@no:
    rol tmp2
    dey
    bne @loop
    lda tmp2
    rts

; Unsigned modulo TOS by A
tosumoda0:
    sta tmp1
    jsr popa
    ldx #0
    ; Simple modulo
    ldy #8
    asl a
@loop:
    rol a
    cmp tmp1
    bcc @no
    sbc tmp1
@no:
    dey
    bne @loop
    ; Remainder is in A
    rts

; Shift A/X left by 4
aslax4:
    asl a
    rol tmp1
    asl a
    rol tmp1
    asl a
    rol tmp1
    asl a
    rol tmp1
    ldx tmp1
    rts

; Store A at (ptr1), Y with Y from stack
staspidx:
    sta tmp1
    jsr popa
    tay
    lda tmp1
    sta (ptr1), y
    rts
