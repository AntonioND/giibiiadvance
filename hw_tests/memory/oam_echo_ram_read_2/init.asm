
	INCLUDE	"hardware.inc"
	
	SECTION	"PATTERNS",HOME

PATTERN_SIZE EQU (256)

PATTERN:
VALUE	SET	0
REPT	256
	DB	VALUE
VALUE	SET	VALUE+1
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
	
	ld	a,$00
	ld	[$4000],a ; bank 0
	
	;--------------------

; copy size, sram destination address, sram bank
PERFORM_TEST : MACRO
	
	ld	d,0
	ld	hl,_OAMRAM
	ld	bc,$0100
	call	memset
	
	ld	bc,\1
	ld	hl,PATTERN
	ld	de,_OAMRAM
	call	memcopy
	
	ld	a,\3
	ld	[$4000],a
	
	ld	bc,PATTERN_SIZE
	ld	hl,_OAMRAM
	ld	de,\2
	call	memcopy

ENDM
	
	;--------------------
	
OFFSET SET 0
	REPT	$20
	PERFORM_TEST  ($90+OFFSET), ($A000 + OFFSET*256), 0
OFFSET SET OFFSET+1
	ENDR

OFFSET SET 0
	REPT	$20
	PERFORM_TEST  ($B0+OFFSET), ($A000 + OFFSET*256), 1
OFFSET SET OFFSET+1
	ENDR

OFFSET SET 0
	REPT	$20
	PERFORM_TEST  ($D0+OFFSET), ($A000 + OFFSET*256), 2
OFFSET SET OFFSET+1
	ENDR

OFFSET SET 0
	REPT	$10 + 1
	PERFORM_TEST  ($F0+OFFSET), ($A000 + OFFSET*256), 3
OFFSET SET OFFSET+1
	ENDR

	;--------------------
	
	ld	a,3
	ld	[$4000],a
	
	ld	hl,$B100
	
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
