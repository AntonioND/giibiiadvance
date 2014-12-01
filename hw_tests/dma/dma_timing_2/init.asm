
	INCLUDE	"hardware.inc"

	SECTION	"Helper Functions",HOME

;--------------------------------------------------------------------------
;- CPU_fast()                                                             -
;- CPU_slow()                                                             -
;--------------------------------------------------------------------------
	
CPU_fast::

	ld	a,[rKEY1]
	bit	7,a
	jr	z,__CPU_switch
	ret
	
CPU_slow::

	ld	a,[rKEY1]
	bit	7,a
	jr	nz,__CPU_switch
	ret

__CPU_switch:
	
	ld	a,[rIE]
	ld	b,a ; save IE
	xor	a,a
	ld	[rIE],a
	ld	a,$30
	ld	[rP1],a
	ld	a,$01
	ld	[rKEY1],a
	
	stop
	
	ld	a,b
	ld	[rIE],a ; restore IE
	
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

MAGIC_VALUE EQU $FE9F ; last byte in OAM
	
;--------------------------------------------------------------------------
;-                             CARTRIDGE HEADER                           -
;--------------------------------------------------------------------------
	
	SECTION	"Cartridge Header",HOME[$0100]
	
	nop
	nop
	jr	Main

	DB	$CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D
	DB	$00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99
	DB	$BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E

	;    0123456789ABC
	DB	"DMA OAM TIME."
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
	
	ld	a,$DB ; debug :)
	ld	[MAGIC_VALUE],a
	
	call	screen_off
	
	ld	a,$0A
	ld	[$0000],a
	
	ld	d,$FF
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,0
	ld	hl,$C000
.fillram:
	ld	[hl+],a
	inc	a
	cp	a,$A0
	jr	nz,.fillram

	;--------------------
	
	ld	bc,DMA0_END-DMA0_START
	ld	hl,DMA0_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA1_END-DMA1_START
	ld	hl,DMA1_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA2_END-DMA2_START
	ld	hl,DMA2_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA3_END-DMA3_START
	ld	hl,DMA3_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA4_END-DMA4_START
	ld	hl,DMA4_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA5_END-DMA5_START
	ld	hl,DMA5_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA6_END-DMA6_START
	ld	hl,DMA6_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA7_END-DMA7_START
	ld	hl,DMA7_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA8_END-DMA8_START
	ld	hl,DMA8_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA9_END-DMA9_START
	ld	hl,DMA9_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA10_END-DMA10_START
	ld	hl,DMA10_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA11_END-DMA11_START
	ld	hl,DMA11_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA12_END-DMA12_START
	ld	hl,DMA12_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA13_END-DMA13_START
	ld	hl,DMA13_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA14_END-DMA14_START
	ld	hl,DMA14_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA15_END-DMA15_START
	ld	hl,DMA15_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA16_END-DMA16_START
	ld	hl,DMA16_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA17_END-DMA17_START
	ld	hl,DMA17_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA18_END-DMA18_START
	ld	hl,DMA18_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	ld	bc,DMA19_END-DMA19_START
	ld	hl,DMA19_START
	ld	de,$FF80
	call	memcopy
	call	$FF80
	
	;--------------------
	
	ld	a,$00
	ld	[$0000],a

.end:
	halt
	jr .end

;--------------------------------------------------------------------------
	
	SECTION	"Functions",HOME

;--------------------------------------------------------------------------

DMA_FN: MACRO
	ld	a,$C0
	ld	[rDMA],a
	ld  a,37
delay\@:
	dec a
	jr  nz,delay\@
	REPT \1
	nop
	ENDR
	ld	a,[MAGIC_VALUE] 
	ld	[$A000+\1],a
	ld  a,9
delay2\@:
	dec a
	jr  nz,delay2\@
	ret
ENDM

;--------------------------------------------------------------------------

DMA0_START:
	DMA_FN 0
DMA0_END:
DMA1_START:
	DMA_FN 1
DMA1_END:
DMA2_START:
	DMA_FN 2
DMA2_END:
DMA3_START:
	DMA_FN 3
DMA3_END:
DMA4_START:
	DMA_FN 4
DMA4_END:
DMA5_START:
	DMA_FN 5
DMA5_END:
DMA6_START:
	DMA_FN 6
DMA6_END:
DMA7_START:
	DMA_FN 7
DMA7_END:
DMA8_START:
	DMA_FN 8
DMA8_END:
DMA9_START:
	DMA_FN 9
DMA9_END:
DMA10_START:
	DMA_FN 10
DMA10_END:
DMA11_START:
	DMA_FN 11
DMA11_END:
DMA12_START:
	DMA_FN 12
DMA12_END:
DMA13_START:
	DMA_FN 13
DMA13_END:
DMA14_START:
	DMA_FN 14
DMA14_END:
DMA15_START:
	DMA_FN 15
DMA15_END:
DMA16_START:
	DMA_FN 16
DMA16_END:
DMA17_START:
	DMA_FN 17
DMA17_END:
DMA18_START:
	DMA_FN 18
DMA18_END:
DMA19_START:
	DMA_FN 19
DMA19_END:

;--------------------------------------------------------------------------

