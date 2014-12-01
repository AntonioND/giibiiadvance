
	INCLUDE	"hardware.inc"
	
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
	DB	"CPU REG START"
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
	
	ld	[$C000],a
	
	; C001 is F
	
	ld	a,b
	ld	[$C002],a
	
	ld	a,c
	ld	[$C003],a
	
	push	af
	pop		bc
	
	ld	a,c
	ld	[$C001],a; F
	
	ld	a,d
	ld	[$C004],a
	
	ld	a,e
	ld	[$C005],a
	
	ld	a,h
	ld	[$C006],a
	
	ld	a,l
	ld	[$C007],a
	
	ld	hl,sp+0
	
	ld	a,h
	ld	[$C008],a
	
	ld	a,l
	ld	[$C009],a
	
	call	screen_off
	
	;--------------------
	
	ld	a,$0A
	ld	[$0000],a
	
	ld	a,$00
	ld	[$4000],a ; bank 0
	
	ld	bc,10
	ld	hl,$C000
	ld	de,$A000
	call	memcopy
	
	ld	h,d
	ld	l,e
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	ld	a,$00
	ld	[$0000],a
	
	;--------------------

.end:
	halt
	jr .end

;--------------------------------------------------------------------------
