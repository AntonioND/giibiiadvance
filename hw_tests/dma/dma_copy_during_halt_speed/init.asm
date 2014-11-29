
	INCLUDE	"hardware.inc"
	
	SECTION	"Functions",HOME

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
	
	;--------------------
	ld	bc,DMA_HALT_END-DMA_HALT_START
	ld	hl,DMA_HALT_START
	ld	de,$FF80
	call	memcopy
	;--------------------
	ld	hl,$C000
	ld	a,$A0
.loop:
	ld	[hl],l
	inc	hl
	dec	a
	jr	nz,.loop
	;--------------------

	ld	a,$0A
	ld	[$0000],a
	
	xor	a,a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,$FC
	ld	[rDIV],a
	ld	[rTIMA],a
	
	ld	bc,$C000
	ld	hl,$A000
	call	$FF80 ; bc = src address, hl = write results here
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	ld	a,$00
	ld	[$0000],a
	
	;--------------------------------------------------------------------------
	
	;--------------------
	ld	bc,DMA_STOP_END-DMA_STOP_START
	ld	hl,DMA_STOP_START
	ld	de,$FF80
	call	memcopy
	;--------------------
	
	ld	a,0
	ld	[rIE],a
	ld	a,$FF
	ld	[rP1],a
	
	ld	a,1
	ld	[rKEY1],a
	
	ld	a,$0A
	ld	[$0000],a
	
	ld	bc,$C000
	ld	hl,$A003
	call	$FF80 ; bc = src address, hl = write results here
	
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

DMA_HALT_START: ; bc = src address

	xor	a,a
	ld	[rIF],a
	
	ld	a,b
	ld	[rDMA],a

	ld	a,IEF_TIMER
	ld	[rIE],a ; do not activate IME. IF only needs to end halt mode
	
	ld	a,[bc]
	ld	d,a
	
	halt
	
	ld	a,[bc]
	ld	e,a

	ld  a,20
.delay:
	dec a
	jr  nz,.delay
	
	ld	a,[bc]
	ld	b,a
	
	ld  a,20
.delay2:
	dec a
	jr  nz,.delay2
	
	ld	[hl],d
	inc	hl
	ld	[hl],e
	inc	hl
	ld	[hl],b
	inc	hl
	
	ret
	
DMA_HALT_END:

;--------------------------------------------------------------------------

DMA_STOP_START: ; bc = src address

	ld	a,b
	ld	[rDMA],a

	ld	a,[bc]
	ld	d,a
	
	stop
	
	ld	a,[bc]
	ld	e,a

	ld  a,20
.delay:
	dec a
	jr  nz,.delay
	
	ld	a,[bc]
	ld	b,a
	
	ld  a,20
.delay2:
	dec a
	jr  nz,.delay2
	
	ld	[hl],d
	inc	hl
	ld	[hl],e
	inc	hl
	ld	[hl],b
	inc	hl
	
	ret
	
DMA_STOP_END:

;--------------------------------------------------------------------------

