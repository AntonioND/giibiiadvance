
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
	DB  $C0 ;GBC flag
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
	
	call	CPU_fast
	
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
	
	ld	a,STATF_LYC
	ld	[rSTAT],a
	
	;--------------------
	
	ld	a,$00
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rIF & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_gbc_0
	
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
	
	ld	c,rIF & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_gbc_1
	
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
	
	ld	c,rIF & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_gbc_2
	
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
	
	ld	c,rIF & $FF
	ld	hl,$A000
	
	halt
	
	call	stat_read_test_delay_gbc_3
	
	ld	[hl],$12
	inc	hl
	ld	[hl],$34
	inc	hl
	ld	[hl],$56
	inc	hl
	ld	[hl],$78
	
	;--------------------
	
	call	CPU_slow
	
	;--------------------
	
	ld	a,$04
	ld	[$4000],a
	
	ld	b,140
	call	wait_ly
	xor	a,a
	ld	[rIF],a
	
	ld	c,rIF & $FF
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
	
	ld	c,rIF & $FF
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
	
	ld	c,rIF & $FF
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
	
	ld	c,rIF & $FF
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

	SECTION	"Functions",ROMX

stat_read_test_delay_gbc_3:
	nop
stat_read_test_delay_gbc_2:
	nop
stat_read_test_delay_gbc_1:
	nop
stat_read_test_delay_gbc_0:
	REPT ( ((456*15)*2/4) / 4 )
	ld	a,[$FF00+c]
	ld	[hl+],a
	ENDR
	
	ld	a,$A5
	ld	[hl+],a
	ld	[hl+],a
	ld	[hl+],a
	ld	[hl+],a
	ld	[hl+],a
	ld	[hl+],a
	ld	[hl+],a
	ld	[hl+],a
	REPT	(((456*120)*2/4)/8) ; delay 120 lines
	push	de
	pop		de
	nop
	ENDR
	
	REPT ( ((456*30)*2/4) / 4 )
	ld	a,[$FF00+c]
	ld	[hl+],a
	ENDR
	ret	

;--------------------------------------------------------------------------

