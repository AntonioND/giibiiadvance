
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
	DB	"LCD TIMINGS.."
	DW	$0000
	DB  $00 ;GBC flag
	DB	0,0,0	;SuperGameboy
	DB	$1B	;CARTTYPE (MBC5+RAM+BATTERY)
	DB	0	;ROMSIZE
	DB	4	;RAMSIZE (16*8KB)
	DB	$01,$00 ;Destination (0 = Japan, 1 = Non Japan) | Manufacturer
	DB	0,0,0,0 ;Version | Complement check | Checksum

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------

Main:

	di

	ld	a,$0A
	ld	[$0000],a ; enable SRAM
	
	; Clear SRAM
	ld	a,$00
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$01
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset

	ld	a,$02
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$03
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$04
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$05
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset

	ld	a,$06
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset
	
	ld	a,$07
	ld	[$4000],a
	ld	d,0
	ld	hl,$A000
	ld	bc,$2000
	call	memset

	;--------------------
	
	ld	a,IEF_VBLANK
	ld	[rIE],a
	
	;--------------------
	
	ld	a,2
	ld	[rLYC],a
	
	;--------------------
	
	ld	a,$00
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rSTAT & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_dmg_0
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------
	
	ld	a,$01
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rSTAT & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_dmg_1
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------
	
	ld	a,$02
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rSTAT & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_dmg_2
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------
	
	ld	a,$03
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rSTAT & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_dmg_3
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78

	;--------------------
	
	ld	a,144
	ld	[rLYC],a
	
	;--------------------
	
	ld	a,$04
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rSTAT & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_dmg_0
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------
	
	ld	a,$05
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rSTAT & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_dmg_1
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------
	
	ld	a,$06
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rSTAT & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_dmg_2
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------
	
	ld	a,$07
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rSTAT & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_dmg_3
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78

	;--------------------
	
	ld	a,$00
	ld	[$0000],a ; disable SRAM
	
	;--------------------
	
	ld	a,$80
	ld	[rNR52],a
	ld	a,$FF
	ld	[rNR51],a
	ld	a,$77
	ld	[rNR50],a
	
	ld	a,$C0
	ld	[rNR11],a
	ld	a,$E0
	ld	[rNR12],a
	ld	a,$00
	ld	[rNR13],a
	ld	a,$87
	ld	[rNR14],a

.end:
	halt
	jr .end

;--------------------------------------------------------------------------

stat_read_test_delay_dmg_3:
	nop
stat_read_test_delay_dmg_2:
	nop
stat_read_test_delay_dmg_1:
	nop
stat_read_test_delay_dmg_0:
	REPT ( ((70224+456*12)/4) / 4 )
	ld	a,[$FF00+c]
	ld	[hl+],a
	ENDR
	ret	

;--------------------------------------------------------------------------

