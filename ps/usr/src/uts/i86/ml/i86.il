/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#ident "@(#)i86.il	1.12	96/06/04 SMI"
/
/ In-line functions for i86 kernels.
/

/
/ return current cpu pointer
/
	.inline	curcpup,0
	movl	%fs:0, %eax
	.end

/
/ invalidate and flush cache.
/
	.inline	cache_bug,0
	wbinvd
	.end

/
/ convert ipl to spl.  This is the identity function for i86
/
	.inline	ipltospl,0
	movl	(%esp), %eax
	.end

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

/
/ Networking byte order functions (too bad, Intel has the wrong byte order)
/

	.inline	htonl,4
	movl	(%esp), %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	.end

	.inline	ntohl,4
	movl	(%esp), %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	.end

	.inline	htons,4
	movzwl	(%esp), %eax
	xchgb	%ah, %al
	.end

	.inline	ntohs,4
	movzwl	(%esp), %eax
	xchgb	%ah, %al
	.end

/*
 * multiply two long numbers and yield a u_lonlong_t result
 * Provided to manipulate hrtime_t values.
 */
	.inline mul32, 8
	movl	4(%esp), %eax
	movl	(%esp), %ecx
	mull	%ecx
	.end
