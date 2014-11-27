
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"
	
SET_VALUES: MACRO
	db \1,\1+1,\1+2,\1+3,\1+4,\1+5,\1+6,\1+7,\1+8,\1+9,\1+10,\1+11,\1+12,\1+13,\1+14,\1+15
ENDM

	SECTION	"22",ROMX[$4000],BANK[1]
	SET_VALUES 1
	SET_VALUES 2
	SET_VALUES 3
	SECTION	"33",ROMX[$4000],BANK[2]
	SET_VALUES 4
	SET_VALUES 5
	SET_VALUES 6
	
	SECTION	"Main",HOME

;--------------------------------------------------------------------------

wait_hbl_end:
	ld	a,[rHDMA5]
	and	a,$80
	jr	z,wait_hbl_end
	ret
	
;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

DMA_COPY_UNSAFE: MACRO   ;   src, dst, size, is_hdma
	ld	a, ( \1 >> 8 )& $FF
	ld	[rHDMA1],a
	ld	a, \1 & $FF
	ld	[rHDMA2],a
	
	ld	a, ( \2 >> 8 )& $FF
	ld	[rHDMA3],a
	ld	a, \2 & $FF
	ld	[rHDMA4],a
	
	ld	a, ( ( ( \3 >> 4 ) - 1 ) | ( \4 << 7 ) ) ; ( Size / $10 ) - 1
	ld	[rHDMA5],a
ENDM

Main:

	di
	
	ld	a,$0A
	ld	[$0000],a ; enable ram
	
	ld	d,0
	ld	bc,$2000
	ld	hl,$8000
	call	memset
	
	; -------------------------------------------------------
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	; -------------------------------------------------------
	
	ld	b,10 ; change vram bank
	call	wait_ly
	ld	a,0
	ld	[rVBK],a
	DMA_COPY $4000,$9000,$30,1
	
	ld	b,11
	call	wait_ly
	ld	a,1
	ld	[rVBK],a
	
	ld	b,15
	call	wait_ly
	ld	a,0
	ld	[rVBK],a
	
	; -------------------------------------------------------
	
	call	screen_off
	
	ld	bc,$30
	ld	hl,$9000
	ld	de,$A000
	call	memcopy
	
	ld	a,1
	ld	[rVBK],a
	
	ld	bc,$30
	ld	hl,$9000
	ld	de,$A030
	call	memcopy
	
	ld	a,0
	ld	[rVBK],a
	
	ld	a,LCDCF_ON
	ld	[rLCDC],a
	
	; -------------------------------------------------------
	
	ld	b,10 ; change rom bank
	call	wait_ly
	DMA_COPY $4000,$9000,$30,1
	ld	a,1
	ld	[$2000],a
	
	ld	b,11
	call	wait_ly
	ld	a,2
	ld	[$2000],a
	
	ld	b,15
	call	wait_ly
	
	; -------------------------------------------------------
	
	call	screen_off
	
	ld	bc,$30
	ld	hl,$9000
	ld	de,$A060
	call	memcopy

	; -------------------------------------------------------
	
	; invalid destinations
	DMA_COPY_UNSAFE $4000,$0000,$10,0
	DMA_COPY_UNSAFE $4000,$C010,$10,0
	DMA_COPY_UNSAFE $4000,$A020,$10,0
	DMA_COPY_UNSAFE $4000,$E030,$10,0
	
	ld	bc,$40
	ld	hl,$8000
	ld	de,$A090
	call	memcopy
	
	; -------------------------------------------------------
	
	; unaligned copy
	DMA_COPY_UNSAFE $4001,$8002,$10,0
	
	ld	bc,$20
	ld	hl,$8000
	ld	de,$A0D0
	call	memcopy
	
	ld	hl,$A0F0
	
	; -------------------------------------------------------
	
	push	hl ; magic number
	ld	[hl],$12
	inc hl
	ld	[hl],$34
	inc hl
	ld	[hl],$56
	inc hl
	ld	[hl],$78
	pop	hl
	
	; -------------------------------------------------------
	
	ld	a,$00
	ld	[$0000],a ; disable ram
	
.endloop:
	halt
	jr	.endloop
