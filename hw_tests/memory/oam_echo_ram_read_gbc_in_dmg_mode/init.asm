
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
	DB  $00 ;GBC flag
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
	
	ld	a,$00
	ld	[$4000],a ; bank 0
	
	;--------------------

; pattern, sram destination address
PERFORM_TEST : MACRO
	
	ld	d,0
	ld	hl,_OAMRAM
	ld	bc,$0100
	call	memset
	
	ld	bc,PATTERN_SIZE
	ld	hl,\1
	ld	de,_OAMRAM
	call	memcopy

	ld	bc,PATTERN_SIZE
	ld	hl,_OAMRAM
	ld	de,\2
	call	memcopy

ENDM
	
	;--------------------
	
	PERFORM_TEST  PATTERN_1,  $A000
	PERFORM_TEST  PATTERN_4,  $A100
	PERFORM_TEST  PATTERN_16, $A200
	PERFORM_TEST  PATTERN_64, $A300
	
	;--------------------
	
	ld	hl,$A400
	
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
