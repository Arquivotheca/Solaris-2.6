/ @(#)p9100.il 1.1    95/03/10 SMI

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.  All Rights Reserved.
 */

/
/ In-line functions for the p9100 driver.
/

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

	.inline P9100_NOOP, 0
	.end

	.inline P9100_WAIT_BUSY, 4
	movl    (%esp), %eax
	testl   $0x40000000, 0x2000(%eax)
	jnz     .-10
	.end


	.inline P9100_XY, 8
	movl    (%esp), %eax
	shll    $16, %eax
	movw    4(%esp), %ax
	.end


	.inline P9100_DRAW_QUAD, 4
	movl    (%esp), %eax
	testl   $0x80000000, 0x2008(%eax)
	jnz     .-10
	.end


	.inline P9100_DRAW_BLIT, 4
	movl    (%esp), %eax
	testl   $0x80000000, 0x2004(%eax)
	jnz     .-10
	.end

	.inline P9100_WAIT_VSYNC, 4
	movl    (%esp), %eax
	movl    0x124(%eax), %edx
	andl    $0x00000fff, %edx
	movl    0x11c(%eax), %ecx
	andl    $0x00000fff, %ecx
	cmpl    %ecx, %edx
	jne     .-14
	.end

	.inline P9100_PAT_COPY, 8
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
	.end


	.inline P9100_SET_ENABLE_VIDEO, 4
	movl    (%esp), %eax
	orl     $0x20, 0x138(%eax)
	.end


	.inline P9100_CLEAR_ENABLE_VIDEO, 4
	movl    (%esp), %eax
	andl    $-1!0x20, 0x138(%eax)
	.end


	.inline P9100_GET_ENABLE_VIDEO, 4
	movl    (%esp), %eax
	movl    0x138(%eax), %eax
	shrl    $5, %eax
	andl    $1, %eax
	.end


	.inline P9100_SET_VBLANK_INTR, 4
	movl    (%esp), %eax
	movl    $0xf0, 0xc(%eax)
	.end


	.inline P9100_STOP_VBLANK_INTR, 4
	movl    (%esp), %eax
	movl    $0x10, 0xc(%eax)
	.end


	.inline P9100_CLEAR_VBLANK_INTR, 4
	movl    (%esp), %eax
	movl    $0x20, 0x8(%eax)
	.end


	.inline P9100_SET_BUFFER_0, 4
	movl    (%esp), %eax
	andl    $-1!0x600, 0x4(%eax)
	.end


	.inline P9100_SET_BUFFER_1, 4
	movl    (%esp), %eax
	orl     $0x600, 0x4(%eax)
	.end


	.inline P9100_WAIT_SUBR, 4
	movl    (%esp), %ecx
	movl    0x2000(%ecx), %eax
	testl   $0x40000000, %eax
	jnz     .-11
	.end

	.inline P9100_QUAD_SUBR, 4
	movl    (%esp), %ecx
	movl    0x2008(%ecx), %eax
	testl   $0x80000000, %eax
	jnz     .-11
	.end

	.inline P9100_BLIT_SUBR, 4
	movl    (%esp), %ecx
	movl    0x2004(%ecx), %eax
	testl   $0x40000000, %eax
	jnz     .-11
	.end

	.inline P9100_READ_FRAMEBUFFER_FOR_RAMDAC, 4
	movl    (%esp), %eax
	movl    0x800200(%eax), %eax
	.end

	.inline P9100_READ_FRAMEBUFFER_FOR_SYSCTRL, 4
	movl    (%esp), %eax
	movl    0x800000(%eax), %eax
	.end

