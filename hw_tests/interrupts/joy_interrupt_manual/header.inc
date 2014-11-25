

	IF	!DEF(HEADER_INC)
HEADER_INC	SET	1

;+---------------------------------------------------------------------------------+
;| +-----------------------------------------------------------------------------+ |
;| |                                  MACROS                                     | |
;| +-----------------------------------------------------------------------------+ |
;+---------------------------------------------------------------------------------+

DMA_COPY: MACRO   ;   src, dst, size, is_hdma
	ld	a, ( \1 >> 8 )& $FF
	ld	[rHDMA1],a
	ld	a, \1 & $F0 ; Lower 4 bits ignored
	ld	[rHDMA2],a
	
	ld	a, ( \2 >> 8 )& $1F ; Upper 3 bits ignored
	ld	[rHDMA3],a
	ld	a, \2 & $F0 ; Lower 4 bits ignored
	ld	[rHDMA4],a
	
	ld	a, ( ( ( \3 >> 4 ) - 1 ) | ( \4 << 7 ) ) ; ( Size / $10 ) - 1
	ld	[rHDMA5],a
ENDM

LONG_CALL: MACRO  ;  function
	ld	b,BANK(\1)
	ld	hl,\1
	call	___long_call
ENDM

;+---------------------------------------------------------------------------------+
;| +-----------------------------------------------------------------------------+ |
;| |                                   INIT.ASM                                  | |
;| +-----------------------------------------------------------------------------+ |
;+---------------------------------------------------------------------------------+

;--------------------------------------------------------------------------
;- irq_set_VBL()    bc = function pointer                                 -
;- irq_set_LCD()    bc = function pointer                                 -
;- irq_set_TIM()    bc = function pointer                                 -
;- irq_set_SIO()    bc = function pointer                                 -
;- irq_set_JOY()    bc = function pointer                                 -
;--------------------------------------------------------------------------

	GLOBAL irq_set_VBL
	GLOBAL irq_set_LCD
	GLOBAL irq_set_TIM
	GLOBAL irq_set_SIO
	GLOBAL irq_set_JOY

;--------------------------------------------------------------------------
;- CPU_fast()                                                             -
;- CPU_slow()                                                             -
;--------------------------------------------------------------------------

	GLOBAL CPU_fast
	GLOBAL CPU_slow

;--------------------------------------------------------------------------
;- wait_vbl()                                                             -
;--------------------------------------------------------------------------

	GLOBAL wait_vbl
	
;--------------------------------------------------------------------------
;- Reset()                                                                -
;--------------------------------------------------------------------------
	
	GLOBAL	Reset
	
;--------------------------------------------------------------------------
;-                                Variables                               -
;--------------------------------------------------------------------------

	GLOBAL Init_Reg_A
	GLOBAL Init_Reg_B

	GLOBAL VBL_handler
	GLOBAL LCD_handler
	GLOBAL TIM_handler
	GLOBAL SIO_handler
	GLOBAL JOY_handler

;+---------------------------------------------------------------------------------+
;| +-----------------------------------------------------------------------------+ |
;| |                                   MAIN.ASM                                  | |
;| +-----------------------------------------------------------------------------+ |
;+---------------------------------------------------------------------------------+

;--------------------------------------------------------------------------
;- Main()                                                                 -
;--------------------------------------------------------------------------
	
	GLOBAL	Main	

;+---------------------------------------------------------------------------------+
;| +-----------------------------------------------------------------------------+ |
;| |                              UTILITIES.ASM                                  | |
;| +-----------------------------------------------------------------------------+ |
;+---------------------------------------------------------------------------------+

;--------------------------------------------------------------------------
;- mul_u8u8u16()    hl = returned value    a,c = initial values           -
;- div_u8u8u8()     c = a / b     a = a % b                               -
;- div_s8s8s8()     a / b -> c     a % b -> a     destroys e              -
;--------------------------------------------------------------------------

	GLOBAL	mul_u8u8u16
	GLOBAL	div_u8u8u8
	GLOBAL	div_s8s8s8

;--------------------------------------------------------------------------
;- memset()    d = value    hl = start address    bc = size               -
;- memset_rand()    hl = start address    bc = size                       -
;- memcopy()    bc = size    hl = source address    de = dest address     -
;- memcopy_inc()    b = size    c = increase    hl = source    de = dest  -
;--------------------------------------------------------------------------

	GLOBAL memset
	GLOBAL memset_rand
	GLOBAL memcopy
	GLOBAL memcopy_inc


;--------------------------------------------------------------------------
;- scan_keys()                                                            -
;--------------------------------------------------------------------------

	GLOBAL scan_keys
    
    GLOBAL	joy_held
	GLOBAL	joy_pressed

PAD_START   EQU $80
PAD_SELECT  EQU $40
PAD_B       EQU $20
PAD_A       EQU $10
PAD_DOWN    EQU $08
PAD_UP      EQU $04
PAD_LEFT    EQU $02
PAD_RIGHT   EQU $01

;--------------------------------------------------------------------------
;- rom_handler_init()                                                     -
;- rom_bank_pop()                                                         -
;- rom_bank_push()                                                        -
;- rom_bank_set()    b = bank to change to                                -
;- rom_bank_push_set()    b = bank to change to                           -
;-                                                                        -
;- ___long_call()    hl = function    b = bank where it is located        -
;--------------------------------------------------------------------------
	
	GLOBAL	rom_handler_init
	GLOBAL	rom_bank_pop	
	GLOBAL	rom_bank_push
	GLOBAL	rom_bank_set
	GLOBAL	rom_bank_push_set
	GLOBAL	___long_call

;--------------------------------------------------------------------------

;+---------------------------------------------------------------------------------+
;| +-----------------------------------------------------------------------------+ |
;| |                                VIDEO.ASM                                    | |
;| +-----------------------------------------------------------------------------+ |
;+---------------------------------------------------------------------------------+

;--------------------------------------------------------------------------
;- wait_ly()    b = ly to wait for                                        -
;- wait_frames()    e = frames to wait                                    -
;- wait_screen_blank()                                                    -
;- screen_off()                                                           -
;--------------------------------------------------------------------------
	
	GLOBAL wait_ly
	GLOBAL wait_frames
	GLOBAL wait_screen_blank
	GLOBAL screen_off
	
;--------------------------------------------------------------------------
;- vram_copy()    bc = size    hl = source address    de = dest address   -
;- vram_memset()    bc = size    d = value    hl = dest address           -
;- vram_copy_tiles()    bc = tiles    de = start index    hl = source     -
;--------------------------------------------------------------------------	
	
	GLOBAL vram_copy
	GLOBAL vram_memset
	GLOBAL vram_copy_tiles
	
;--------------------------------------------------------------------------
;- init_OAM()                                                             -
;- refresh_OAM()                                                          -
;- refresh_custom_OAM()    a = (oam copy >> 8), aligned to 100h           -
;-                                                                        -
;- sprite_get_base_pointer()    l = sprite    return = hl    destroys de  -
;-                                                                        -
;- sprite_set_xy()    b = x    c = y    l = sprite number                 -
;- sprite_set_tile()    a = tile    l = sprite number                     -
;- sprite_set_params()    a = params    l = sprite number                 -
;-                                                                        -
;- spr_set_palette()    a = palette number    hl = pointer to data        -
;--------------------------------------------------------------------------
	
	GLOBAL OAM_Copy
	
	GLOBAL init_OAM
	GLOBAL refresh_OAM
	GLOBAL refresh_custom_OAM
	GLOBAL sprite_get_base_pointer
	GLOBAL sprite_set_xy
	GLOBAL sprite_set_tile
	GLOBAL sprite_set_params
	GLOBAL spr_set_palette
	
;--------------------------------------------------------------------------
;- bg_set_tile()    b = x    c = y    a = tile index                      -
;- bg_set_tile_wrap()    b = x    c = y    a = tile index                 -
;-                                                                        -
;- bg_set_palette()    a = palette number    hl = pointer to data         -
;--------------------------------------------------------------------------
	
	GLOBAL bg_set_tile
	GLOBAL bg_set_tile_wrap
	GLOBAL bg_set_palette

	ENDC	;HEADER_INC
	
