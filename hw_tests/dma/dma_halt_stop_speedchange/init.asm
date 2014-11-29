
	INCLUDE	"hardware.inc"

	SECTION	"TIMER",HOME[$0050]
	ret
	SECTION	"JOY",HOME[$0060]
	ret

	SECTION	"Home",HOME

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
;-                             CARTRIDGE HEADER                           -
;--------------------------------------------------------------------------
	
	SECTION	"Cartridge Header",HOME[$0100]
	
	nop
	nop
	jr	Main

	NINTENDO_LOGO

	;    0123456789ABC
	DB	"DMA HALT TEST"
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
	
	
	ld	a,$A0
	ld	hl,$8000
.set_vram:
	ld	[hl],l
	inc	hl
	dec	a
	jr	nz,.set_vram

	ld	a,$0A
	ld	[$0000],a
	;--------------------------------------------------------------------------
	
	;----------------
	ld	bc,DMA_HALT_END-DMA_HALT_START
	ld	hl,DMA_HALT_START
	ld	de,$FF80
	call	memcopy
	;--------------------
	
	ld	hl,$FE00
	ld	bc,$A0
	ld	d,0
	call	memset ; clear OAM
	
	ld	a,IEF_TIMER
	ld	[rIE],a
	
	xor	a,a
	ld	[rIF],a
	ld	[rDIV],a
	ld	[rTIMA],a
	ld	[rTMA],a
	ld	a,TACF_STOP|TACF_65KHZ
	ld	[rTAC],a
	ld	a,TACF_START|TACF_65KHZ
	ld	[rTAC],a
	ld	a,$FE
	ld	[rDIV],a
	ld	[rTIMA],a
	
	ei
	
	;--------------------
	ld	a,$80
	call	$FF80 ; a = src address
	;--------------------
	
	ld	bc,$A0
	ld	hl,$FE00
	ld	de,$A000
	call	memcopy

	;--------------------------------------------------------------------------

	;--------------------
	ld	bc,DMA_STOP_END-DMA_STOP_START
	ld	hl,DMA_STOP_START
	ld	de,$FF80
	call	memcopy
	;--------------------
	
	ld	hl,$FE00
	ld	bc,$A0
	ld	d,0
	call	memset ; clear OAM
	
	di
	
	ld	a,$00
	ld	[rP1],a
	
	;--------------------
	ld	a,$80
	call	$FF80 ; a = src address
	;--------------------
	
	ld	bc,$A0
	ld	hl,$FE00
	ld	de,$A000 + $A0
	call	memcopy
	
	;--------------------------------------------------------------------------
	
	ld	hl,$A000 + $A0 + $A0
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------------------------------------------------------------
	
	ld	hl,$FE00
	ld	bc,$A0
	ld	d,0
	call	memset ; clear OAM
	
	di
	
	ld	a,$00
	ld	[rIE],a
	
	ld	a,$FF
	ld	[rP1],a
	
	ld	a,1
	ld	[rKEY1],a
	
	;--------------------
	ld	a,$80
	call	$FF80 ; a = src address
	;--------------------
	
	ld	bc,$A0
	ld	hl,$FE00
	ld	de,$A000 + $A0 + $A0
	call	memcopy
	
	;--------------------------------------------------------------------------
	
	ld	hl,$A000 + $A0 + $A0 + $A0
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	ld	a,$00
	ld	[$0000],a

.end:
	halt
	jr .end

;--------------------------------------------------------------------------

DMA_HALT_START: ; bc = src address

	ld	[rDMA],a
	
	nop
	nop
	
	halt

	ld  a,40
.delay:
	dec a
	jr  nz,.delay
	
	ret
	
DMA_HALT_END:

; ------------------------------

DMA_STOP_START: ; bc = src address

	ld	[rDMA],a
	
	nop
	nop
	
	stop

	ld  a,40
.delay:
	dec a
	jr  nz,.delay
	
	ret
	
DMA_STOP_END:

;--------------------------------------------------------------------------

