/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All Rights Reserved.
 */

/ @(#)p9000.il 1.2    94/07/07 SMI
/
/ In-line functions for the p9000 driver.
/

/
/ enable interrupts
/
	.inline	sti,0
	sti
	.end

/
/ disable interrupts and return value describing if interrupts were enabled
/
	.inline	clear_int_flag,0
	pushfl
	cli
	popl	%eax
	.end

/
/ restore interrupt enable flag to value returned from 'clear_int_flag' above
/
	.inline restore_int_flag,4
	pushl	(%esp)
	popfl
	.end

/
/ in and out
/
	.inline	inb,4
	movl	(%esp), %edx
	xorl    %eax, %eax
	inb	(%dx)
	.end

	.inline	inw,4
	movl	(%esp), %edx
	xorl    %eax, %eax
	inw	(%dx)
	.end

	.inline	inl,4
	movl	(%esp), %edx
	xorl    %eax, %eax
	inl	(%dx)
	.end

	.inline	outb,8
	movl	(%esp), %edx
	movl    4(%esp), %eax
	outb	(%dx)
	.end

	.inline	outw,8
	movl	(%esp), %edx
	movl    4(%esp), %eax
	outw	(%dx)
	.end

	.inline	outl,8
	movl	(%esp), %edx
	movl    4(%esp), %eax
	outl	(%dx)
	.end

	.inline P9000_NOOP, 0
	.end

	.inline P9000_WAIT_BUSY, 4
	movl    (%esp), %eax
	testl   $0x40000000, 0x180000(%eax)
	jnz     .-10
	.end


	.inline P9000_XY, 8
	movl    (%esp), %eax
	shll    $16, %eax
	movw    4(%esp), %ax
	.end


	.inline P9000_DRAW_QUAD, 4
	movl    (%esp), %eax
	testl   $0x80000000, 0x180008(%eax)
	jnz     .-10
	.end


	.inline P9000_DRAW_BLIT, 4
	movl    (%esp), %eax
	testl   $0x80000000, 0x180004(%eax)
	jnz     .-10
	.end

	.inline P9000_WAIT_VSYNC, 4
	movl    (%esp), %eax
	movl    0x100124(%eax), %edx
	andl    $0x00000fff, %edx
	movl    0x10011c(%eax), %ecx
	andl    $0x00000fff, %ecx
	cmpl    %ecx, %edx
	jne     .-14
	.end

	.inline P9000_PAT_COPY, 8
	movl    (%esp), %edx
	movl    4(%esp), %ecx
	movl    (%ecx), %eax
	movl    %eax, (%edx)
	movl    4(%ecx), %eax
	movl    %eax, 4(%edx)
	movl    8(%ecx), %eax
	movl    %eax, 8(%edx)
	movl    12(%ecx), %eax
	movl    %eax, 12(%edx)
	movl    16(%ecx), %eax
	movl    %eax, 16(%edx)
	movl    20(%ecx), %eax
	movl    %eax, 20(%edx)
	movl    24(%ecx), %eax
	movl    %eax, 24(%edx)
	movl    28(%ecx), %eax
	movl    %eax, 28(%edx)
	.end


	.inline P9000_SET_ENABLE_VIDEO, 4
	movl    (%esp), %eax
	orl     $0x20, 0x100138(%eax)
	.end


	.inline P9000_CLEAR_ENABLE_VIDEO, 4
	movl    (%esp), %eax
	andl    $-1!0x20, 0x100138(%eax)
	.end


	.inline P9000_GET_ENABLE_VIDEO, 4
	movl    (%esp), %eax
	movl    0x100138(%eax), %eax
	shrl    $5, %eax
	andl    $1, %eax
	.end


	.inline P9000_SET_VBLANK_INTR, 4
	movl    (%esp), %eax
	movl    $0xf0, 0x10000c(%eax)
	.end


	.inline P9000_STOP_VBLANK_INTR, 4
	movl    (%esp), %eax
	movl    $0x10, 0x10000c(%eax)
	.end


	.inline P9000_CLEAR_VBLANK_INTR, 4
	movl    (%esp), %eax
	movl    $0x20, 0x100008(%eax)
	.end


	.inline P9000_SET_BUFFER_0, 4
	movl    (%esp), %eax
	andl    $-1!0x600, 0x100004(%eax)
	.end


	.inline P9000_SET_BUFFER_1, 4
	movl    (%esp), %eax
	orl     $0x600, 0x100004(%eax)
	.end


	.inline P9000_WAIT_SUBR, 4
	movl    (%esp), %ecx
	movl    0x180000(%ecx), %eax
	testl   $0x40000000, %eax
	jnz     .-11
	.end

	.inline P9000_QUAD_SUBR, 4
	movl    (%esp), %ecx
	movl    0x180008(%ecx), %eax
	testl   $0x80000000, %eax
	jnz     .-11
	.end

	.inline P9000_BLIT_SUBR, 4
	movl    (%esp), %ecx
	movl    0x180004(%ecx), %eax
	testl   $0x40000000, %eax
	jnz     .-11
	.end



