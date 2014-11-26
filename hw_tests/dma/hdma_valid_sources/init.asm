
	INCLUDE	"hardware.inc"
	INCLUDE "header.inc"
	

;--------------------------------------------------------------------------
;-                             RESTART VECTORS                            -
;--------------------------------------------------------------------------

	SECTION	"RST_10",HOME[$0010]
	ret

	SECTION	"RST_18",HOME[$0018]
	ret

	SECTION	"RST_20",HOME[$0020]
	ret

	SECTION	"RST_28",HOME[$0028]
	ret

	SECTION	"RST_30",HOME[$0030]
	ret

	SECTION	"RST_38",HOME[$0038]
	jp	Reset
	
;--------------------------------------------------------------------------
;-                            INTERRUPT VECTORS                           -
;--------------------------------------------------------------------------
	
	SECTION	"Interrupt Vectors",HOME[$0040]
	
;	SECTION	"VBL Interrupt Vector",HOME[$0040]
	push	hl
	ld	hl,_is_vbl_flag
	ld	[hl],1
	jr	irq_VBlank
	
;	SECTION	"LCD Interrupt Vector",HOME[$0048]
	push	hl
	ld	hl,LCD_handler
	jr	irq_Common
	nop
	nop
	
;	SECTION	"TIM Interrupt Vector",HOME[$0050]
	push	hl
	ld	hl,TIM_handler
	jr	irq_Common
	nop
	nop
	
;	SECTION	"SIO Interrupt Vector",HOME[$0058]
	push	hl
	ld	hl,SIO_handler
	jr	irq_Common
	nop
	nop
	
;	SECTION	"JOY Interrupt Vector",HOME[$0060]
	push	hl
	ld	hl,JOY_handler
	jr	irq_Common
;	nop
;	nop

;--------------------------------------------------------------------------
;-                               IRQS HANDLER                             -
;--------------------------------------------------------------------------

irq_VBlank:	
	ld	hl,VBL_handler
	
irq_Common:
	push	af
	
	ld	a,[hl+]
	ld	h,[hl]
	ld	l,a
	
;	or	a,h
;	jr	z,.no_irq
	
	push	bc
	push	de
	call .goto_irq_handler	
	pop	de
	pop	bc
	
;.no_irq:
	pop	af
	pop	hl	
	
	reti
	
.goto_irq_handler:	
	jp	hl

;--------------------------------------------------------------------------
;- wait_vbl()                                                             -
;--------------------------------------------------------------------------
	
wait_vbl:
	
	ld	hl,_is_vbl_flag
	ld	[hl],0
	
._not_yet:	
	halt
	bit	0,[hl]
	jr	z,._not_yet
	
	ret

;--------------------------------------------------------------------------
;-                             CARTRIDGE HEADER                           -
;--------------------------------------------------------------------------
	
	SECTION	"Cartridge Header",HOME[$0100]
	
	nop
	jp	StartPoint

	DB	$CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D
	DB	$00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99
	DB	$BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E

	;    0123456789ABC
	DB	"TESTING......"
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
;-                          INITIALIZE THE GAMEBOY                        -
;--------------------------------------------------------------------------

	SECTION	"Program Start",HOME[$0150]
	
StartPoint:

	di
	
	ld	sp,$FFFE ; Use this as stack for a while

	push	af ; Save CPU type
	push	bc
	
	xor	a,a
	ld	[rNR52],a ; Switch off sound
	
	ld	hl,_RAM ; Clear RAM
	ld	bc,$2000
	ld	d,$00
	call	memset
	
	pop	bc ; Get CPU type
	pop	af
	
	ld	[Init_Reg_A],a  ; Save CPU type into RAM
	ld	a,b
	ld	[Init_Reg_B],a
	
	ld	sp,StackTop ; Real stack
	
	call	screen_off

	ld	hl,_VRAM ; Clear VRAM
	ld	bc,$2000
	ld	d,$00
	call	memset
	
	ld	hl,_HRAM ; Clear high RAM (and rIE)
	ld	bc,$0080
	ld	d,$00
	call	memset
	
	call	init_OAM ; Copy OAM refresh function to high ram
	call	refresh_OAM ; We filled RAM with $00, so this will clear OAM
	
	call	rom_handler_init
	
	; Real program starts here
	
	call	Main
	
	;Should never reach this point
	
	jp	Reset
	
;--------------------------------------------------------------------------
;- Reset()                                                                -
;--------------------------------------------------------------------------

Reset::
	ld	a,[Init_Reg_B]
	ld	b,a
	ld	a,[Init_Reg_A]
	jp	$0100

;--------------------------------------------------------------------------
;- irq_set_VBL()    bc = function pointer                                 -
;- irq_set_LCD()    bc = function pointer                                 -
;- irq_set_TIM()    bc = function pointer                                 -
;- irq_set_SIO()    bc = function pointer                                 -
;- irq_set_JOY()    bc = function pointer                                 -
;--------------------------------------------------------------------------
	
irq_set_VBL::

	ld	hl,VBL_handler
	jr	irq_set_handler

irq_set_LCD::

	ld	hl,LCD_handler
	jr	irq_set_handler

irq_set_TIM::

	ld	hl,TIM_handler
	jr	irq_set_handler
	
irq_set_SIO::

	ld	hl,SIO_handler
	jr	irq_set_handler
	
irq_set_JOY::

	ld	hl,JOY_handler
;	jr	irq_set_handler

irq_set_handler:  ; hl = dest handler    bc = function pointer

	ld	[hl],c
	inc	hl
	ld	[hl],b
	
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
;-                               Variables                                -
;--------------------------------------------------------------------------	
	
	SECTION	"StartupVars",BSS
	
Init_Reg_A::	DS 1
Init_Reg_B::	DS 1

_is_vbl_flag:	DS 1
	
VBL_handler:	DS 2
LCD_handler:	DS 2
TIM_handler:	DS 2
SIO_handler:	DS 2
JOY_handler:	DS 2

	SECTION	"Stack",BSS[$CE00]
	
Stack:	DS	$200
StackTop: ; $D000
