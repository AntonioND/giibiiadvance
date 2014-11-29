
	INCLUDE	"hardware.inc"
	
	
	SECTION	"2",ROMX[$4000],BANK[1]

	REPT $A0
	db	$40
	ENDR
	
	SECTION	"Helper Functions",HOME[$0000]

	REPT $A0
	db	$00
	ENDR
	
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

	SECTION	"VAR",BSS[$C100]

MAGIC_VALUE:
	
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
	DB	"DMA VALID SRC"
	DW	$0000
	DB  $00 ;GBC flag
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
	
	ld	a,$0A
	ld	[$0000],a
	
	;----------------
	
	ld	hl,$8000
	ld	bc,$6000
.setloop;
	ld	[hl],h
	inc	hl
	dec	bc
	ld	a,b
	or	a,c
	jr	nz,.setloop
	
	;--------------------
	
DMA_TEST_ADDRESS : MACRO
	
	ld	bc,DMA_END-DMA_START
	ld	hl,DMA_START
	ld	de,$FF80
	call	memcopy
	ld	bc,\1
	ld	de,\2
	ld	hl,\3
	call	$FF80 ; bc = src address, de = test address, hl = write sram result address

ENDM
	
DMA_TEST_BANK : MACRO
	DMA_TEST_ADDRESS \1,$0000,0+\2
	DMA_TEST_ADDRESS \1,$4000,1+\2
	DMA_TEST_ADDRESS \1,$8000,2+\2
	DMA_TEST_ADDRESS \1,$9000,3+\2
	DMA_TEST_ADDRESS \1,$A000,4+\2
	DMA_TEST_ADDRESS \1,$B000,5+\2
	DMA_TEST_ADDRESS \1,$C000,6+\2
	DMA_TEST_ADDRESS \1,$D000,7+\2
	DMA_TEST_ADDRESS \1,$E000,8+\2
	DMA_TEST_ADDRESS \1,$E800,9+\2
	DMA_TEST_ADDRESS \1,$F000,10+\2
	DMA_TEST_ADDRESS \1,$F800,11+\2
	DMA_TEST_ADDRESS \1,$FC00,12+\2
	DMA_TEST_ADDRESS \1,$FD00,13+\2
	DMA_TEST_ADDRESS \1,$FE00,14+\2
	DMA_TEST_ADDRESS \1,$FF00,15+\2
ENDM
	
	DMA_TEST_BANK $0000,$9000
	DMA_TEST_BANK $4000,$9010
	DMA_TEST_BANK $8000,$9020
	DMA_TEST_BANK $9000,$9030
	DMA_TEST_BANK $A000,$9040
	DMA_TEST_BANK $B000,$9050
	DMA_TEST_BANK $C000,$9060
	DMA_TEST_BANK $D000,$9070
	DMA_TEST_BANK $E000,$9080
	DMA_TEST_BANK $E800,$9090
	DMA_TEST_BANK $F000,$90A0
	DMA_TEST_BANK $F800,$90B0
	DMA_TEST_BANK $FC00,$90C0
	DMA_TEST_BANK $FD00,$90D0
	DMA_TEST_BANK $FE00,$90E0
	DMA_TEST_BANK $FF00,$90F0

	ld	bc,$100
	ld	hl,$9000
	ld	de,$A000
	call	memcopy ; bc = size    hl = source address    de = dest address
	
	ld	hl,$A100
	ld	[hl],$12
	ld	hl,$A101
	ld	[hl],$34
	ld	hl,$A102
	ld	[hl],$56
	ld	hl,$A103
	ld	[hl],$78
	
	;--------------------
	
	ld	a,$00
	ld	[$0000],a

.end:
	halt
	jr .end

;--------------------------------------------------------------------------
	
	SECTION	"Functions",HOME
	
;--------------------------------------------------------------------------

DMA_FN: MACRO ; bc = src address, de = test address, hl = write sram result address
	ld	a,b
	ld	[rDMA],a
	ld  a,37
delay\@:
	dec a
	jr  nz,delay\@
	ld	a,[de]
	ld  b,9
delay2\@:
	dec b
	jr  nz,delay2\@
	ld	[hl],a
	ret
ENDM

;--------------------------------------------------------------------------

DMA_START:
	DMA_FN 0
DMA_END:

;--------------------------------------------------------------------------

