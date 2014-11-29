
	INCLUDE	"hardware.inc"
	
	SECTION	"Timer INT Vector",HOME[$0050]
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a
	
	ret

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
	DB	"DMA IRQ TEST."
	DW	$0000
	DB  $C0 ;GBC flag
	DB	0,0,0	;SuperGameboy
	DB	$1B	;CARTTYPE (MBC5+RAM+BATTERY)
	DB	0	;ROMSIZE
	DB	3	;RAMSIZE (4*8KB)

	DB	$01 ;Destination (0 = Japan, 1 = Non Japan)
	DB	$00	;Manufacturer

	DB	0	;Version
	DB	0	;Complement check
	DW	0	;Checksum

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:

	di
	
	call	screen_off
	
	;----------------
	
	ld	bc,DMA_END-DMA_START
	ld	hl,DMA_START
	ld	de,$FF80
	call	memcopy

	;--------------------

	ld	a,$80
	ld	[rNR52],a
	ld	a,$FF
	ld	[rNR51],a
	ld	a,$77
	ld	[rNR50],a
	
	;--------------------
	
	ld	bc,$C000
	call	$FF80 ; bc = src address

	;--------------------

.end:
	halt
	jr .end

;--------------------------------------------------------------------------

DMA_START: ; bc = src address

	xor	a,a
	ld	[rIF],a
	
	ld	a,b
	ld	[rDMA],a

	ei
	ld	a,IEF_TIMER
	ld	[rIE],a
	ld	[rIF],a

	ld  a,40
.delay:
	dec a
	jr  nz,.delay
	
	ret
	
DMA_END:

;--------------------------------------------------------------------------

