	
@---------------------------------------------------------------------------------
	.section	.text,"ax",%progbits  
	.align 4
	.code 32
	.arm
	
	.global main        @  -------  DUMMY main()
main:	b main
@---------------------------------------------------------------------------------




@---------------------------------------------------------------------------------
@-                                                                               -
@-                               BIOS                                            -
@-                                                                               -
@---------------------------------------------------------------------------------
	
	.section	.text,"ax",%progbits 
	
	.align 4
	.string "@@@@@@@@@@@@@@@@@@@@@@@@@ START OF BIOS @@@@@@@@@@@@@@@@@@@@@@@@@"
	.align 4
	.string	"NEXT 100h-ALIGNED ADDRESS"
	
	.word	0x01234567
	.word	0x89ABCDEF
	
	.align 8
	.code 32
	.arm
@---------------------------------------------------------------------------------
	.global	BIOS_START
BIOS_START:
@---------------------------------------------------------------------------------
	b	.reset_v @ Reset
	b	.reset_v @ Undefined Instruction 
	b	.swi___v @ Software Interrupt (SWI)
	b	.reset_v @ Prefetch Abort
	b	.reset_v @ Data Abort
	b	.reset_v @ (Reserved)  
	b	.irq___v @ Normal Interrupt (IRQ)
	b	.reset_v @ Fast Interrupt (FIQ)

.reset_v: @ ---------------------------------- RESET ------------------------
	mov	r0,#(0x12|0x80|0x40)		@ |- set irq mode, IRQ and FIQ disabled
	msr	cpsr_fc,r0					@ |
	
	mov	r13,#0x03000000
	orr	r13,#0x00007F00
	orr	r13,#0x000000A0				@ SP_irq=03007FA0h			
	
	@ -----------
	
	mov	r0,#(0x13|0x80|0x40)		@ |- set svc mode, IRQ and FIQ disabled
	msr	cpsr_fc,r0					@ |
	
	mov	r13,#0x03000000
	orr	r13,#0x00007F00
	orr	r13,#0x000000E0				@ SP_svc=03007FE0h
	
	@ -----------
	
	mov	r0,#0x1F					@ |- set system mode
	msr	cpsr_fc,r0					@ |
	
	mov	r13,#0x03000000
	orr	r13,#0x00007F00				@ SP_usr=03007F00h
	
	mov r14,#0x08000000				@ LR_usr=08000000h
    mov r15,r14						@ PC_usr=08000000h -- jump to start address
	
.irq___v: @ ---------------------------------- IRQ --------------------------
	stmfd	r13!,{r0-r3,r12,r14}	@ save registers to SP_irq
	mov		r0,#0x04000000			@ ptr+4 to 03FFFFFC (mirror of 03007FFC)
	add		r14,r15,#0				@ retadr for USER handler $+8=138h
	ldr		r15,[r0,#-4]			@ jump to [03FFFFFC] USER handler
	ldmfd	r13!,{r0-r3,r12,r14}	@ restore registers from SP_irq
	subs	r15,r14,#0x4			@ return from IRQ (PC=LR-4, CPSR=SPSR)
	
.swi___v: @ ---------------------------------- swi handler (0X05 only) -------
	stmdb	r13!,{r11,r12,r14}
	mrs		r11,spsr
	stmdb	r13!,{r11}
	and		r11,r11,#0x80			@ |-save IRQ flag
	orr		r11,r11,#0x1F			@ | set system mode
	msr		cpsr_fc,r11				@ |
	stmdb	r13!,{r2,r14}
	bl		.swi_irqwait			@ only irqwait, the rest should be emulated high-level
	ldmia	r13!,{r2,r14}
	mov		r12,#0xD3				@ IRQ and FIQ disabled, supervisor mode
	msr		cpsr_fc,r12
	ldmia	r13!,{r11}
	msr		spsr_fc,r11
	ldmia	r13!,{r11,r12,r14}
	movs	r15,r14				 	@ return from SWI

.swi_irqwait:
	stmdb	r13!,{r4,r14}
	mov		r3,#0
	mov		r4,#1
	cmp		r0,#0					@ |-see if we can avoid halting the cpu 
	blne	.handle_flags			@ | if r0 != 0 when SWI was called
.wait_IRQ:
	@mov	r12,#0x04000000			@ base reg ---- BUG, REAL BIOS SHOULD HAVE THIS !!!
	strb	r3,[r12,#+0x301]		@ halt
	bl		.handle_flags
	beq		.wait_IRQ				@ if Z = 0 (no IRQ checked) wait
	ldmia	r13!,{r4,r14}
	bx		r14						@ return to SWI handler

.handle_flags:						@ -- checks if ( BIOSFLAGS&selected_flags(in r1) ) != 0
	mov		r12,#0x04000000			@ REG_BASE
	strb	r3,[r12,#+0x208]		@ IME = 0
	ldrh	r2,[r12,#-0x8]			@ get BIOSFLAGS
	ands	r0,r1,r2				@ check if any IRQ has been checked. set cpsr flags
	eorne	r2,r2,r0				@ if so, clear them
	strneh	r2,[r12,#-0x8]			@ save BIOSFLAGS without r1 flags
	strb	r4,[r12,#+0x208]		@ IME = 1
	bx		r14						@ return. Z is now set if any IRQ checked

@---------------------------------------------------------------------------------

	.string "@@@@@@@@@@@@@@@@@@@@@@@@@ END OF BIOS @@@@@@@@@@@@@@@@@@@@@@@@@" 

@---------------------------------------------------------------------------------

