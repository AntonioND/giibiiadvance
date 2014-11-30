
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
	ld	a,$03
	ld	[$4000],a ; use bank 3 for testing, the other banks for storing data
	
	;--------------------

; vram pattern, sram pattern, wram pattern, offset, vram temp address, sram destination address, sram destination bank
PERFORM_TEST : MACRO
	ld	a,$03
	ld	[$4000],a
	
	ld	bc,PATTERN_SIZE
	ld	hl,\1
	ld	de,_VRAM + \4
	call	memcopy
	
	ld	bc,PATTERN_SIZE
	ld	hl,\2
	ld	de,$A000 + \4
	call	memcopy
	
	ld	bc,PATTERN_SIZE
	ld	hl,\3
	ld	de,_RAM + \4
	call	memcopy
	
	;--------------------

	ld	bc,PATTERN_SIZE
	ld	hl,$E000 + \4
	ld	de,\5
	call	memcopy
	
	ld	a,\7
	ld	[$4000],a
	
	ld	bc,PATTERN_SIZE
	ld	hl,\5
	ld	de,\6
	call	memcopy
	
	ld	a,$03
	ld	[$4000],a
ENDM

	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_1,$0000,  $9000,$A000,0
	PERFORM_TEST  PATTERN_1, PATTERN_1, PATTERN_4,$0100,  $9000,$A100,0
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_16,$0200,  $9000,$A200,0
	PERFORM_TEST  PATTERN_1, PATTERN_1,PATTERN_64,$0300,  $9000,$A300,0
	
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_1,$0400,  $9000,$A400,0
	PERFORM_TEST  PATTERN_1, PATTERN_4, PATTERN_4,$0500,  $9000,$A500,0
	PERFORM_TEST  PATTERN_1, PATTERN_4,PATTERN_16,$0600,  $9000,$A600,0
	PERFORM_TEST  PATTERN_1, PATTERN_4,PATTERN_64,$0700,  $9000,$A700,0
	
	PERFORM_TEST  PATTERN_1,PATTERN_16, PATTERN_1,$0800,  $9000,$A800,0
	PERFORM_TEST  PATTERN_1,PATTERN_16, PATTERN_4,$0900,  $9000,$A900,0
	PERFORM_TEST  PATTERN_1,PATTERN_16,PATTERN_16,$0A00,  $9000,$AA00,0
	PERFORM_TEST  PATTERN_1,PATTERN_16,PATTERN_64,$0B00,  $9000,$AB00,0

	PERFORM_TEST  PATTERN_1,PATTERN_64, PATTERN_1,$0C00,  $9000,$AC00,0
	PERFORM_TEST  PATTERN_1,PATTERN_64, PATTERN_4,$0D00,  $9000,$AD00,0
	PERFORM_TEST  PATTERN_1,PATTERN_64,PATTERN_16,$0E00,  $9000,$AE00,0
	PERFORM_TEST  PATTERN_1,PATTERN_64,PATTERN_64,$0F00,  $9000,$AF00,0
	
	; --
	
	PERFORM_TEST  PATTERN_4, PATTERN_1, PATTERN_1,$1000,  $8000,$B000,0
	PERFORM_TEST  PATTERN_4, PATTERN_1, PATTERN_4,$1100,  $8000,$B100,0
	PERFORM_TEST  PATTERN_4, PATTERN_1,PATTERN_16,$1200,  $8000,$B200,0
	PERFORM_TEST  PATTERN_4, PATTERN_1,PATTERN_64,$1300,  $8000,$B300,0
	
	PERFORM_TEST  PATTERN_4, PATTERN_4, PATTERN_1,$1400,  $8000,$B400,0
	PERFORM_TEST  PATTERN_4, PATTERN_4, PATTERN_4,$1500,  $8000,$B500,0
	PERFORM_TEST  PATTERN_4, PATTERN_4,PATTERN_16,$1600,  $8000,$B600,0
	PERFORM_TEST  PATTERN_4, PATTERN_4,PATTERN_64,$1700,  $8000,$B700,0
	
	PERFORM_TEST  PATTERN_4,PATTERN_16, PATTERN_1,$1800,  $8000,$B800,0
	PERFORM_TEST  PATTERN_4,PATTERN_16, PATTERN_4,$1900,  $8000,$B900,0
	PERFORM_TEST  PATTERN_4,PATTERN_16,PATTERN_16,$1A00,  $8000,$BA00,0
	PERFORM_TEST  PATTERN_4,PATTERN_16,PATTERN_64,$1B00,  $8000,$BB00,0

	PERFORM_TEST  PATTERN_4,PATTERN_64, PATTERN_1,$1C00,  $8000,$BC00,0
	PERFORM_TEST  PATTERN_4,PATTERN_64, PATTERN_4,$1D00,  $8000,$BD00,0
	PERFORM_TEST  PATTERN_4,PATTERN_64,PATTERN_16,$0E00,  $9000,$BE00,0 ; 1E is OAM
	PERFORM_TEST  PATTERN_4,PATTERN_64,PATTERN_64,$0F00,  $9000,$BF00,0 ; 1F is REGs
	
	; --
	
	PERFORM_TEST PATTERN_16, PATTERN_1, PATTERN_1,$0000,  $9000,$A000,1
	PERFORM_TEST PATTERN_16, PATTERN_1, PATTERN_4,$0100,  $9000,$A100,1
	PERFORM_TEST PATTERN_16, PATTERN_1,PATTERN_16,$0200,  $9000,$A200,1
	PERFORM_TEST PATTERN_16, PATTERN_1,PATTERN_64,$0300,  $9000,$A300,1
	
	PERFORM_TEST PATTERN_16, PATTERN_4, PATTERN_1,$0400,  $9000,$A400,1
	PERFORM_TEST PATTERN_16, PATTERN_4, PATTERN_4,$0500,  $9000,$A500,1
	PERFORM_TEST PATTERN_16, PATTERN_4,PATTERN_16,$0600,  $9000,$A600,1
	PERFORM_TEST PATTERN_16, PATTERN_4,PATTERN_64,$0700,  $9000,$A700,1
	
	PERFORM_TEST PATTERN_16,PATTERN_16, PATTERN_1,$0800,  $9000,$A800,1
	PERFORM_TEST PATTERN_16,PATTERN_16, PATTERN_4,$0900,  $9000,$A900,1
	PERFORM_TEST PATTERN_16,PATTERN_16,PATTERN_16,$0A00,  $9000,$AA00,1
	PERFORM_TEST PATTERN_16,PATTERN_16,PATTERN_64,$0B00,  $9000,$AB00,1

	PERFORM_TEST PATTERN_16,PATTERN_64, PATTERN_1,$0C00,  $9000,$AC00,1
	PERFORM_TEST PATTERN_16,PATTERN_64, PATTERN_4,$0D00,  $9000,$AD00,1
	PERFORM_TEST PATTERN_16,PATTERN_64,PATTERN_16,$0E00,  $9000,$AE00,1
	PERFORM_TEST PATTERN_16,PATTERN_64,PATTERN_64,$0F00,  $9000,$AF00,1
	
	; --
	
	PERFORM_TEST PATTERN_64, PATTERN_1, PATTERN_1,$1000,  $8000,$B000,1
	PERFORM_TEST PATTERN_64, PATTERN_1, PATTERN_4,$1100,  $8000,$B100,1
	PERFORM_TEST PATTERN_64, PATTERN_1,PATTERN_16,$1200,  $8000,$B200,1
	PERFORM_TEST PATTERN_64, PATTERN_1,PATTERN_64,$1300,  $8000,$B300,1
	
	PERFORM_TEST PATTERN_64, PATTERN_4, PATTERN_1,$1400,  $8000,$B400,1
	PERFORM_TEST PATTERN_64, PATTERN_4, PATTERN_4,$1500,  $8000,$B500,1
	PERFORM_TEST PATTERN_64, PATTERN_4,PATTERN_16,$1600,  $8000,$B600,1
	PERFORM_TEST PATTERN_64, PATTERN_4,PATTERN_64,$1700,  $8000,$B700,1
	
	PERFORM_TEST PATTERN_64,PATTERN_16, PATTERN_1,$1800,  $8000,$B800,1
	PERFORM_TEST PATTERN_64,PATTERN_16, PATTERN_4,$1900,  $8000,$B900,1
	PERFORM_TEST PATTERN_64,PATTERN_16,PATTERN_16,$1A00,  $8000,$BA00,1
	PERFORM_TEST PATTERN_64,PATTERN_16,PATTERN_64,$1B00,  $8000,$BB00,1

	PERFORM_TEST PATTERN_64,PATTERN_64, PATTERN_1,$1C00,  $8000,$BC00,1
	PERFORM_TEST PATTERN_64,PATTERN_64, PATTERN_4,$1D00,  $8000,$BD00,1
	PERFORM_TEST PATTERN_64,PATTERN_64,PATTERN_16,$0E00,  $9000,$BE00,1 ; 1E is OAM
	PERFORM_TEST PATTERN_64,PATTERN_64,PATTERN_64,$0F00,  $9000,$BF00,1 ; 1F is REGs
	
	;--------------------
	
	ld	a,$02
	ld	[$4000],a
	
	ld	hl,$A000
	
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

.end:
	halt
	jr .end

;--------------------------------------------------------------------------
