
	INCLUDE	"hardware.inc"

	SECTION	"PATTERNS",HOME

PATTERN_SIZE EQU (4*4*4*4)

PATTERN_1:
REPT 4*4*4
	DB	$AA
	DB	$A5
	DB	$5A
	DB	$55
ENDR
	
PATTERN_4:
REPT 4*4
	REPT 4
	DB	$AA
	ENDR
	REPT 4
	DB	$A5
	ENDR
	REPT 4
	DB	$5A
	ENDR
	REPT 4
	DB	$55
	ENDR
ENDR

PATTERN_16:
REPT 4
	REPT 16
	DB	$AA
	ENDR
	REPT 16
	DB	$A5
	ENDR
	REPT 16
	DB	$5A
	ENDR
	REPT 16
	DB	$55
	ENDR
ENDR

PATTERN_64:
	REPT 64
	DB	$AA
	ENDR
	REPT 64
	DB	$A5
	ENDR
	REPT 64
	DB	$5A
	ENDR
	REPT 64
	DB	$55
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
;- memset_128()    a = value    hl = start address                        -
;--------------------------------------------------------------------------

memset_128::
	REPT	128
	ld	[hl+],a
	ENDR
	ret

;--------------------------------------------------------------------------
;- memcopy_128()    hl = source address    de = dest address              -
;--------------------------------------------------------------------------

memcopy_128::
	REPT	128
	ld	a,[hl+]
	ld	[de],a
	inc	de
	ENDR
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
	
	ld	a,$0A
	ld	[$0000],a
	
	ld	a,$00
	ld	[$4000],a ; bank 0
	
	;--------------------

; pattern, sram destination address
PERFORM_TEST_READ : MACRO
	
	xor	a,a
	ld	[rIF],a
	ld	[rIE],a
	
	ld	b,144
	call	wait_ly
	
	ld	a,0
	ld	hl,_OAMRAM+128
	call	memset_128
	
	ld	b,144
	call	wait_ly
	
	ld	hl,\1
	ld	de,_OAMRAM+128
	call	memcopy_128
	
	ld	a,STATF_LYC
	ld	[rSTAT],a
	ld	a,5
	ld	[rLYC],a
	xor	a,a
	ld	[rIF],a
	ld	a,IEF_LCDC
	ld	[rIE],a
	
	halt
	
	ld	hl,_OAMRAM+128
	ld	de,\2
	call	memcopy_128

ENDM

; pattern, sram destination address
PERFORM_TEST_WRITE : MACRO
	
	xor	a,a
	ld	[rIF],a
	ld	[rIE],a
	
	ld	b,144
	call	wait_ly
	
	ld	a,0
	ld	hl,_OAMRAM+128
	call	memset_128
	
	ld	a,STATF_LYC
	ld	[rSTAT],a
	ld	a,5
	ld	[rLYC],a
	xor	a,a
	ld	[rIF],a
	ld	a,IEF_LCDC
	ld	[rIE],a
	
	halt
	
	ld	hl,\1
	ld	de,_OAMRAM+128
	call	memcopy_128
	
	ld	b,144
	call	wait_ly
	
	ld	hl,_OAMRAM+128
	ld	de,\2
	call	memcopy_128

ENDM
	
	;--------------------
	
	PERFORM_TEST_READ	PATTERN_1,  $A000
	PERFORM_TEST_READ	PATTERN_4,  $A080
	PERFORM_TEST_READ	PATTERN_16, $A100
	PERFORM_TEST_READ	PATTERN_64, $A180
	
	PERFORM_TEST_WRITE	PATTERN_1,  $A200
	PERFORM_TEST_WRITE	PATTERN_4,  $A280
	PERFORM_TEST_WRITE	PATTERN_16, $A300
	PERFORM_TEST_WRITE	PATTERN_64, $A380
	
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
	
	call	screen_off

.end:
	halt
	jr .end

;--------------------------------------------------------------------------
