
	INCLUDE "hardware.inc"
	INCLUDE "header.inc"
	
	SECTION	"Video_General",HOME
	
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
;- wait_frames()    e = frames to wait                                    -
;--------------------------------------------------------------------------

wait_frames::

	call	wait_vbl
	dec	e
	jr	nz,wait_frames
	
	ret

;--------------------------------------------------------------------------
;- screen_off()                                                           -
;--------------------------------------------------------------------------
	
screen_off::
	
	ld	a,[rLCDC]
	and	a,LCDCF_ON
	jr	z,.lcd_off_
	
	ld	b,$91
	call	wait_ly

	xor	a,a
	ld	[rLCDC],a ;Shutdown LCD

.lcd_off_:
	
	ret
	
;--------------------------------------------------------------------------
;- wait_screen_blank()                                                    -
;--------------------------------------------------------------------------

wait_screen_blank::

	ld	a,[rSTAT]
	bit	1,a
	jr	nz,wait_screen_blank ; Not mode 0 or 1
	
	ret
	
;--------------------------------------------------------------------------
;- vram_copy()    bc = size    hl = source address    de = dest address   -
;--------------------------------------------------------------------------
	
vram_copy::
	
	ld	a,[rSTAT]
	bit	1,a
	jr	nz,vram_copy ; Not mode 0 or 1
	
	ld	a,[hl+]
	ld	[de],a
	inc	de
	dec	bc
	ld	a,b
	or	a,c
	jr	nz,vram_copy
	
	ret

;--------------------------------------------------------------------------
;- vram_memset()    bc = size    d = value    hl = dest address           -
;--------------------------------------------------------------------------
	
vram_memset::
	
	ld	a,[rSTAT]
	bit	1,a
	jr	nz,vram_memset ; Not mode 0 or 1
	
	ld	[hl],d
	inc	hl
	dec	bc
	ld	a,b
	or	a,c
	jr	nz,vram_memset
	
	ret

;--------------------------------------------------------------------------
;- vram_copy_tiles()    bc = tiles    de = start index    hl = source     -
;--------------------------------------------------------------------------

vram_copy_tiles::
	
	push	hl
	
	ld	h,d
	ld	l,e
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl ; index * 16
	ld	de,$8000
	add	hl,de ; dest + base
	ld	d,h
	ld	e,l
	
	pop	hl
	
	; de = dest

._copy_tile:
	
	push	bc
	ld	bc,$0010
	call vram_copy
	pop	bc
	
	dec	bc
	ld	a,b
	or	c
	jr	nz,._copy_tile
	
	ret


;--------------------------------------------------------------------------------------------------
;--------------------------------------------------------------------------------------------------
;--                                        SPRITES                                               --
;--------------------------------------------------------------------------------------------------
;--------------------------------------------------------------------------------------------------

;--------------------------------------------------------------------------
;-                            RAM VARIABLES                               -
;--------------------------------------------------------------------------

	SECTION	"OAMCopy",BSS[$C000]

OAM_Copy: DS $A0 ; We will use DMA to copy this to OAM

;--------------------------------------------------------------------------
;-                              FUNCTIONS                                 -
;--------------------------------------------------------------------------

	SECTION	"Video_Sprites",HOME
	
;--------------------------------------------------------------------------
;- sprite_get_base_pointer()    l = sprite    return = hl    destroys de  -
;--------------------------------------------------------------------------

sprite_get_base_pointer::
	ld	h,$00
	add	hl,hl
	add	hl,hl ; spr number *= 4
	ld	de,OAM_Copy
	add	hl,de
	
	ret

;--------------------------------------------------------------------------
;- sprite_set_xy()    b = x    c = y    l = sprite number                 -
;--------------------------------------------------------------------------

sprite_set_xy::

	call sprite_get_base_pointer
	
	ld	[hl],c
	inc	hl
	ld	[hl],b
	
	ret
	
;--------------------------------------------------------------------------
;- sprite_set_tile()    a = tile    l = sprite number                     -
;--------------------------------------------------------------------------
	
sprite_set_tile::

	call sprite_get_base_pointer
	inc	hl
	inc	hl
	ld	[hl],a
	
	ret
	
;--------------------------------------------------------------------------
;- sprite_set_params()    a = params    l = sprite number                 -
;--------------------------------------------------------------------------
	
sprite_set_params::

	call sprite_get_base_pointer
	inc	hl
	inc	hl
	inc	hl
	ld	[hl],a
	
	ret
	
;--------------------------------------------------------------------------
;- spr_set_palette()    a = palette number    hl = pointer to data        -
;--------------------------------------------------------------------------
	
spr_set_palette::
	
	swap	a
	rra ; multiply palette by 8
	set	7,a ; auto increment
	ld	[rOCPS],a
	
	ld	a,[hl+]
	ld	[rOCPD],a
	ld	a,[hl+]
	ld	[rOCPD],a
	
	ld	a,[hl+]
	ld	[rOCPD],a
	ld	a,[hl+]
	ld	[rOCPD],a
	
	ld	a,[hl+]
	ld	[rOCPD],a
	ld	a,[hl+]
	ld	[rOCPD],a
	
	ld	a,[hl+]
	ld	[rOCPD],a
	ld	a,[hl+]
	ld	[rOCPD],a
	
	ret

;--------------------------------------------------------------------------
;- init_OAM()                                                             -
;--------------------------------------------------------------------------
	
init_OAM::

	ld	bc,__refresh_OAM_end - __refresh_OAM
	ld	hl,__refresh_OAM
	ld	de,refresh_OAM_HRAM
	call memcopy
	
	ret
	
__refresh_OAM:

	ld  [rDMA],a 
	ld  a,$28      ;delay 200ms
.delay
	dec a
	jr  nz,.delay
	
	ret

__refresh_OAM_end:

;--------------------------------------------------------------------------
;- refresh_OAM()                                                          -
;--------------------------------------------------------------------------
	
refresh_OAM::
	
	ld	a,OAM_Copy >> 8
	jp	refresh_OAM_HRAM

;--------------------------------------------------------------------------
;- refresh_custom_OAM()                                                   -
;--------------------------------------------------------------------------

refresh_custom_OAM::
	jp	refresh_OAM_HRAM

;--------------------------------------------------------------------------
;-                           HRAM VARIABLES                               -
;--------------------------------------------------------------------------

	SECTION	"OAMRefreshFn",HRAM[$FF80]
	
refresh_OAM_HRAM:	DS (__refresh_OAM_end - __refresh_OAM)
	
;--------------------------------------------------------------------------------------------------
;--------------------------------------------------------------------------------------------------
;--                                       BACKGROUND                                             --
;--------------------------------------------------------------------------------------------------
;--------------------------------------------------------------------------------------------------

	SECTION	"Video_Background",HOME
	
;--------------------------------------------------------------------------
;- bg_set_palette()    a = palette number    hl = pointer to data         -
;--------------------------------------------------------------------------
	
bg_set_palette::
	
	swap	a ; \  multiply
	rrca      ; /  palette by 8
	
	set	7,a ; auto increment
	ld	[rBCPS],a
	
	ld	a,[hl+]
	ld	[rBCPD],a
	ld	a,[hl+]
	ld	[rBCPD],a
	
	ld	a,[hl+]
	ld	[rBCPD],a
	ld	a,[hl+]
	ld	[rBCPD],a
	
	ld	a,[hl+]
	ld	[rBCPD],a
	ld	a,[hl+]
	ld	[rBCPD],a
	
	ld	a,[hl+]
	ld	[rBCPD],a
	ld	a,[hl+]
	ld	[rBCPD],a
	
	ret


;--------------------------------------------------------------------------
;- bg_set_tile_wrap()    b = x    c = y    a = tile index                 -
;--------------------------------------------------------------------------
	
bg_set_tile_wrap::

	ld	l,a
	
	ld	h,31
	
	ld	a,b
	and	a,h
	ld	b,a
	
	ld	a,c
	and	a,h
	ld	c,a
	
	ld	a,l
	
	; Fall through

;--------------------------------------------------------------------------
;- bg_set_tile()    b = x    c = y    a = tile index                      -
;--------------------------------------------------------------------------
	
bg_set_tile::

;	ld	de,$9800	
	ld	d,$98
	ld	e,b ; de = base + x
	
	ld	l,c
	ld	h,$00 ; hl = y
	
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl ; y *  32
	
	add	hl,de ; hl = base + x + (y * 32)
	
	ld	[hl],a
	
	ret
	
