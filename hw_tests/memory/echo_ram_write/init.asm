
	INCLUDE	"hardware.inc"
	
	SECTION	"PATTERNS",HOME

PATTERN_SIZE EQU (4*4*4*4)

PATTERN_1:
REPT 4*4*4
	DB	$FF
	DB	$F0
	DB	$0F
	DB	$00
ENDR
	
PATTERN_4:
REPT 4*4
	REPT 4
	DB	$FF
	ENDR
	REPT 4
	DB	$F0
	ENDR
	REPT 4
	DB	$0F
	ENDR
	REPT 4
	DB	$00
	ENDR
ENDR

PATTERN_16:
REPT 4
	REPT 16
	DB	$FF
	ENDR
	REPT 16
	DB	$F0
	ENDR
	REPT 16
	DB	$0F
	ENDR
	REPT 16
	DB	$00
	ENDR
ENDR

PATTERN_64:
	REPT 64
	DB	$FF
	ENDR
	REPT 64
	DB	$F0
	ENDR
	REPT 64
	DB	$0F
	ENDR
	REPT 64
	DB	$00
	ENDR
	
	SECTION	"Helper Functions",HOME

;--------------------------------------------------------------------------
;- wait_ly()    b = ly to wait for                                        -
;--------------------------------------------------------------------------
	
wait_ly::

	ld	c,rLY & $FF

.no_same_ly:
	ld	a,[$FF00+c]
	cp	a,b
	jr	nz,.no_same_ly
	
	ret

;--------------------------------------------------------------------------
;- screen_off()                                                           -
;--------------------------------------------------------------------------
	
screen_off::
	
	ld	a,[rLCDC]
	and	a,LCDCF_ON
	ret	z
	
	ld	b,$91
	call	wait_ly

	xor	a,a
	ld	[rLCDC],a ;Shutdown LCD

	ret

;--------------------------------------------------------------------------
;- memset()    d = value    hl = start address    bc = size               -
;--------------------------------------------------------------------------

memset::
	
	ld	a,d
	ld	[hl+],a
	dec	bc
	ld	a,b
	or	a,c
	jr	nz,memset
	
	ret

;--------------------------------------------------------------------------
;- memcopy()    bc = size    hl = source address    de = dest address     -
;--------------------------------------------------------------------------

memcopy::
	
	ld	a,[hl+]
	ld	[de],a
	inc	de
	dec	bc
	ld	a,b
	or	a,c
	jr	nz,memcopy
	
	ret

;--------------------------------------------------------------------------
;-                             CARTRIDGE HEADER                           -
;--------------------------------------------------------------------------
	
	SECTION	"Cartridge Header",HOME[$0100]
	
	nop
	nop
	jr	Main
	NINTENDO_LOGO
	;    0123456789ABC
	DB	"ECHO RAM TEST"
	DW	$0000
	DB  $C0 ;GBC flag
	DB	0,0,0	;SuperGameboy
	DB	$1B	;CARTTYPE (MBC5+RAM+BATTERY)
	DB	0	;ROMSIZE
	DB	3	;RAMSIZE (4*8KB)
	DB	$01,$00 ;Destination (0 = Japan, 1 = Non Japan) | Manufacturer
	DB	0,0,0,0 ;Version | Complement check | Checksum

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:

	di
	call	screen_off
	ld	a,$0A
	ld	[$0000],a
	
	ld	a,$00 ; clear sram
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$01
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset

	ld	a,$02
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$03
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$03
	ld	[$4000],a ; use bank 3 for testing, the other banks for storing data
	
	;--------------------

; vram pattern, sram pattern, wram pattern, echo_pattern, offset, vram temp address, sram destination address, sram destination bank
PERFORM_TEST : MACRO
	ld	a,$03
	ld	[$4000],a
	
	ld	bc,PATTERN_SIZE ; set patterns to vram, sram, wram and then write to echo
	ld	hl,\1
	ld	de,_VRAM + \5
	call	memcopy
	
	ld	bc,PATTERN_SIZE
	ld	hl,\2
	ld	de,$A000 + \5
	call	memcopy
	
	ld	bc,PATTERN_SIZE
	ld	hl,\3
	ld	de,_RAM + \5
	call	memcopy
	
	ld	bc,PATTERN_SIZE
	ld	hl,\4
	ld	de,$E000 + \5
	call	memcopy
	
	;--------------------

	ld	bc,PATTERN_SIZE ; save result from vram
	ld	hl,_VRAM + \5
	ld	de,\6
	call	memcopy
	
	ld	a,\8
	ld	[$4000],a
	
	ld	bc,PATTERN_SIZE
	ld	hl,\6
	ld	de,\7
	call	memcopy
	
	ld	a,$03
	ld	[$4000],a
	
	;-------------------
	
	ld	bc,PATTERN_SIZE ; save result from sram
	ld	hl,$A000 + \5
	ld	de,\6
	call	memcopy
	
	ld	a,\8
	ld	[$4000],a
	
	ld	bc,PATTERN_SIZE
	ld	hl,\6
	ld	de,\7 + PATTERN_SIZE
	call	memcopy
	
	ld	a,$03
	ld	[$4000],a
	
	;-------------------
	
	ld	bc,PATTERN_SIZE ; save result from wram
	ld	hl,_RAM + \5
	ld	de,\6
	call	memcopy
	
	ld	a,\8
	ld	[$4000],a
	
	ld	bc,PATTERN_SIZE
	ld	hl,\6
	ld	de,\7 + PATTERN_SIZE + PATTERN_SIZE
	call	memcopy
	
	ld	a,$03
	ld	[$4000],a
	
ENDM

; vram pattern, sram pattern, wram pattern, echo_pattern, offset, vram temp address, sram destination address, sram destination bank

	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_1, PATTERN_1,$0000,  $9000,$A000,0
	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_1, PATTERN_4,$0000,  $9000,$A300,0
	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_1,PATTERN_16,$0000,  $9000,$A600,0
	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_1,PATTERN_64,$0000,  $8000,$A900,0
	
	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_4, PATTERN_1,$0000,  $8000,$AC00,0
	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_4, PATTERN_4,$0000,  $8000,$AF00,0
	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_4,PATTERN_16,$0000,  $8000,$B200,0
	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_4,PATTERN_64,$0000,  $8000,$B500,0
	
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_16, PATTERN_1,$0000,  $8000,$B800,0
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_16, PATTERN_4,$0000,  $8000,$BB00,0
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_16,PATTERN_16,$0000,  $9000,$A000,1
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_16,PATTERN_64,$0000,  $9000,$A300,1
	
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_64, PATTERN_1,$0000,  $9000,$A600,1
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_64, PATTERN_4,$0000,  $8000,$A900,1
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_64,PATTERN_16,$0000,  $8000,$AC00,1
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_64,PATTERN_64,$0000,  $8000,$AF00,1
	
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_1, PATTERN_1,$0000,  $8000,$B200,1
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_1, PATTERN_4,$0000,  $8000,$B500,1
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_1,PATTERN_16,$0000,  $8000,$B800,1
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_1,PATTERN_64,$0000,  $8000,$BB00,1
	
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_4, PATTERN_1,$0000,  $9000,$A000,2
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_4, PATTERN_4,$0000,  $9000,$A300,2
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_4,PATTERN_16,$0000,  $9000,$A600,2
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_4,PATTERN_64,$0000,  $8000,$A900,2
	
	PERFORM_TEST  PATTERN_1, PATTERN_4,PATTERN_16, PATTERN_1,$0000,  $8000,$AC00,2
	PERFORM_TEST  PATTERN_1, PATTERN_4,PATTERN_16, PATTERN_4,$0000,  $8000,$AF00,2
	PERFORM_TEST  PATTERN_1, PATTERN_4,PATTERN_16,PATTERN_16,$0000,  $8000,$B200,2
	PERFORM_TEST  PATTERN_1, PATTERN_4,PATTERN_16,PATTERN_64,$0000,  $8000,$B500,2
	
	PERFORM_TEST PATTERN_16, PATTERN_4,PATTERN_64, PATTERN_1,$0000,  $8000,$B800,2
	PERFORM_TEST PATTERN_16, PATTERN_4,PATTERN_64, PATTERN_4,$0000,  $8000,$BB00,2

	;--------------------
	
	ld	a,$02
	ld	[$4000],a
	
	ld	hl,$BE00
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------
	
	ld	a,$00
	ld	[$0000],a
	
	;--------------------
	
	ld	a,$80
	ld	[rNR52],a
	ld	a,$FF
	ld	[rNR51],a
	ld	a,$77
	ld	[rNR50],a
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a

.end:
	halt
	jr .end

;--------------------------------------------------------------------------
