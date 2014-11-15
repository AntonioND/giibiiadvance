
	INCLUDE "hardware.inc"
	INCLUDE "header.inc"
	
;--------------------------------------------------------------------------
;-                          GENERAL FUNCTIONS                             -
;--------------------------------------------------------------------------	

	SECTION	"Utilities",HOME
	
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
;- memset_rand()    hl = start address    bc = size                       -
;--------------------------------------------------------------------------

memset_rand::
	
	push	hl
	call	GetRandom
	pop	hl
	ld	[hl+],a
	dec	bc
	ld	a,b
	or	a,c
	jr	nz,memset_rand
	
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
;- memcopy_inc()    b = size    c = increase    hl = source    de = dest  -
;--------------------------------------------------------------------------

memcopy_inc::
	
	ld	a,[hl]
	ld	[de],a
	
	inc	de ; increase dest
	
	ld	a,b ; save b
	ld	b,$00
	add	hl,bc ; increase source
	ld	b,a ; restore b
	
	dec	b
	jr	nz,memcopy_inc
	
	ret

;--------------------------------------------------------------------------
;-                                 MATH                                  -
;--------------------------------------------------------------------------	
	
;--------------------------------------------------------------------------
;-                               Functions                                -
;--------------------------------------------------------------------------	

	SECTION	"MathFunctions",HOME

;--------------------------------------------------------------------------
;- mul_u8u8u16()    hl = returned value    a,c = initial values           -
;--------------------------------------------------------------------------

mul_u8u8u16::
	
	ld	hl,$0000
	ld	b,h
.nextbit:	
	bit	0,a
	jr	z,.no_add
	add	hl,bc
.no_add:
	sla	c 
	rl	b ; bc <<= 1
	srl	a ; a >>= 1
	jr	nz,.nextbit
	
	ret

;--------------------------------------------------------------------------
;- div_u8u8u8()     a / b -> c     a % b -> a                             -
;--------------------------------------------------------------------------

div_u8u8u8::
	
	inc	b
	dec	b
	jr	z,.div_by_zero ; check if divisor is 0
	
	ld	c,$FF ; -1
.continue:
	inc	c
	sub	a,b ; if a > b then a -= b , c ++
	jr	nc,.continue ; if a > b continue

	add	a,b ; fix remainder
	and	a,a ; clear carry
	ret

.div_by_zero
	ld	a,$FF
	ld	c,$00
	scf ; set carry
	ret

;--------------------------------------------------------------------------
;- div_s8s8s8()     a / b -> c     a % b -> a                             -
;--------------------------------------------------------------------------

div_s8s8s8::
	
	ld	e,$00 ; bit 0 of e = result sign (0/1 = +/-)
	
	bit	7,a
	jr	z,.dividend_is_positive
	inc	e
	cpl ; change sign
	inc	a
.dividend_is_positive
	
	bit	7,b
	jr	z,.divisor_is_positive
	ld	c,a
	ld	a,b
	cpl
	inc	a
	ld	b,a ; change sign
	inc	e
.divisor_is_positive
	
	call	div_u8u8u8
	ret	c ; if division by 0, exit now
	
	bit	0,e
	ret	z ; exit if both signs are the same
	
	ld	b,a ; save modulo
	ld	a,c ; change sign
	cpl
	inc	a
	ld	c,a
	ld	a,b ; get modulo
	
	ret

;--------------------------------------------------------------------------
;-                            JOYPAD HANDLER                              -
;--------------------------------------------------------------------------	

;--------------------------------------------------------------------------
;-                               Variables                                -
;--------------------------------------------------------------------------	

	SECTION	"JoypadHandlerVariables",HRAM

_joy_old:		DS	1			
joy_held::		DS	1
joy_pressed::	DS	1

;--------------------------------------------------------------------------
;-                               Functions                                -
;--------------------------------------------------------------------------	

	SECTION	"JoypadHandler",HOME
	
;--------------------------------------------------------------------------
;- scan_keys()                                                            -
;--------------------------------------------------------------------------

scan_keys::
	
	ld	a,[joy_held]
	ld	[_joy_old],a   ; current state = old state
	ld	c,a            ; c = old state
	
	ld	a,$10
	ld	[rP1],a  ; select P14
	ld	a,[rP1]
	ld	a,[rP1]  ; wait a few cycles
	cpl	               ; complement A
	and	a,$0F          ; get only first 4 bits
	swap	a          ; swap it
	ld	b,a            ; store A in B
	ld	a,$20
	ld	[rP1],a        ; select P15
	ld	a,[rP1]
	ld	a,[rP1]
	ld	a,[rP1]
	ld	a,[rP1]
	ld	a,[rP1]
	ld	a,[rP1]  ; Wait a few MORE cycles
	cpl
	and	a,$0F
	or	a,b            ; put A and B together
	
	ld	[joy_held],a
	
	xor	a,c            ; c = old state
	and	a,c
	
	ld	[joy_pressed],a
	
	ld	a,$00          ; deselect P14 and P15
	ld	[rP1],a  ; RESET Joypad

	ret


;--------------------------------------------------------------------------
;-                              ROM HANDLER                               -
;--------------------------------------------------------------------------	


;--------------------------------------------------------------------------
;-                               Variables                                -
;--------------------------------------------------------------------------	

	SECTION	"RomHandlerVariables",BSS

rom_stack:		DS	$20
rom_position:	DS	1

;--------------------------------------------------------------------------
;-                               Functions                                -
;--------------------------------------------------------------------------	

	SECTION	"RomHandler",HOME

;--------------------------------------------------------------------------
;- rom_handler_init()                                                     -
;--------------------------------------------------------------------------
	
rom_handler_init::
	
	xor	a,a
	ld	[rom_position],a	
	
	ld	b,1
	call	rom_bank_set  ; select rom bank 1
	
	ret	
	
;--------------------------------------------------------------------------
;- rom_bank_pop()                                                         -
;--------------------------------------------------------------------------
	
rom_bank_pop::
	ld	hl,rom_position
	dec	[hl]
	
	ld	hl,rom_stack
	
	ld	d,$00
	ld	a,[rom_position]
	ld	e,a
	
	add	hl,de             ; hl now holds the pointer to the bank we want to change to
	ld	a,[hl]            ; and a the bank we want to change to
	
	ld	[$2000],a         ; select rom bank
	
	ret

;--------------------------------------------------------------------------
;- rom_bank_push()                                                        -
;--------------------------------------------------------------------------
	
rom_bank_push::
	ld	hl,rom_position
	inc	[hl]

	ret
	
;--------------------------------------------------------------------------
;- rom_bank_set()    b = bank to change to                                -
;--------------------------------------------------------------------------
	
rom_bank_set::
	ld	hl,rom_stack
	
	ld	d,$00
	ld	a,[rom_position]
	ld	e,a
	add	hl,de
	
	ld	a,b               ; hl = pointer to stack, a = bank to change to
	
	ld	[hl],a
	ld	[$2000],a         ; select rom bank
	
	ret
	
;--------------------------------------------------------------------------
;- rom_bank_push_set()    b = bank to change to                           -
;--------------------------------------------------------------------------
	
rom_bank_push_set::
	ld	hl,rom_position
	inc	[hl]
	
	ld	hl,rom_stack
	
	ld	d,$00
	ld	a,[rom_position]
	ld	e,a
	add	hl,de
	
	ld	a,b               ; hl = pointer to stack, a = bank to change to
	
	ld	[hl],a
	ld	[$2000],a         ; select rom bank

	ret

;--------------------------------------------------------------------------
;- ___long_call()    hl = function    b = bank where it is located        -
;--------------------------------------------------------------------------

___long_call::
	push	hl
	call	rom_bank_push_set
	pop	hl
	call	._jump_to_function
	call	rom_bank_pop
	ret
._jump_to_function:
	jp	[hl]


	

