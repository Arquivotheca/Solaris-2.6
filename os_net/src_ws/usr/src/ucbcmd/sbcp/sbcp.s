	.ident	"@(#)sbcp.s 1.12 95/11/12 SMI"

/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#include <sys/asm_linkage.h>
#include <sys/syscall.h>
#include "PIC.h"

#define	FUNC(x) \
	.section	".text"; \
	.align	4; \
	.type	x, #function; \
x:

#define	ENOSYS	90		/* 4.x ENOSYS */

/* derived from <sys/exechdr.h>, which we can't include */
#define	A_MAGIC	0x02	/* offset of a_magic field */
#define	A_ENTRY	0x14	/* offset of a_entry field */
#define	ZMAGIC	0413	/* magic number for demand paged executable */

	.global	atexit, errno

!
!	_start - execution starts here (after the runtime linker runs)
!
!	The SPARC ABI defines our "environment" at this point, see page 3-34.
!	Register the exit handler, register the trap0 handler, find the
!	entry point, and jump to it.  We depend on the stack (argv, envp)
!	being compatible between 4.x and 5.x.  We also depend on the
!	runtime linker to set up ``environ''.
!

ENTRY_NP(_start)
	tst	%g1			! is there a handler to register?
	bz	1f			! no
	nop
	mov	%g1, %o0
	call	atexit			! yes, register it with atexit()
	nop
1:

	! give the kernel the address of our trap0 handler

	PIC_SETUP(g2)
	ld	[%g2+trap0], %g1
	ta	9

	! jump to the main program's entry point

	sethi   %hi(0x2000), %o0
	lduh    [%o0 + A_MAGIC], %g1
	cmp     %g1, ZMAGIC		! is it a ZMAGIC executable?
	be,a    1f			! yes,
	ld      [%o0 + A_ENTRY], %o0	!   get entry point
1:					! else, assume entry point is 0x2000
	jmp	%o0
	nop
	SET_SIZE(_start)

!
!	trap0 - glue between 4.x syscall trap and 5.x BCP routine
!
!	enter with:
!		%g1	syscall number
!		%g7	return address (after trap instruction)
!
!	Note that if we have call into a multithreaded Solaris 2.x lib
!	we will probably have trouble with passing the return pc back
!	in g7. That is because libthread thinks it owns g7. This may have
!	to be redone at some point in the future.
!
!	We use an extra window to save the %o registers we're entered
!	with (which the 4.x system call stubs depend on) and to allow
!	recursive traps (e.g., from a signal handler).
!

FUNC(trap0)
	save	%sp, -SA(MINFRAME), %sp
	tst	%g1
	be	1f
	nop
	mov	%i0, %o0
	mov	%i1, %o1
	mov	%i2, %o2
	mov	%i3, %o3
	mov	%i4, %o4
	mov	%i5, %o5
	ba,a	2f
1:
	! indir syscall
	mov	%i0, %g1
	mov	%i1, %o0
	mov	%i2, %o1
	mov	%i3, %o2
	mov	%i4, %o3
	mov	%i5, %o4
	ld	[%fp + MINFRAME], %o5
2:
	sll	%g1, 4, %l1
	PIC_SETUP(l0)
	ld	[%l0+sysent], %l0
	add	%l1, %l0, %l1
	jmp	%l1			! jump into branch table
	nop
	SET_SIZE(trap0)

FUNC(trap0rtn)
	cmp	%o0, -1
	bne	1f
	addcc	%g0, %g0, %g0		! psr &= ~C
	PIC_SETUP(o1)
	ld	[%o1+errno], %o1
	ld	[%o1], %o0
	subcc	%g0, 1, %g0		! psr |= C
1:
	mov	%o0, %i0
	restore
	jmp	%g7
	nop
	SET_SIZE(trap0rtn)

!
!	nullsys
!
FUNC(nullsys)
	clr	%o0
	b,a	trap0rtn
	SET_SIZE(nullsys)

!
!	nosys
!
FUNC(nosys)
	set	ENOSYS, %o1
	PIC_SETUP(g2)
	ld	[%g2+errno], %g2
	st	%o1, [%g2]
	set	-1, %o0
	b,a	trap0rtn
	SET_SIZE(nosys)

!
!	Have to #include the sysent table and stubs so that all
!	symbols referenced between here and there are "static"
!	to this module so the assembler can resolve them without
!	the linker needing to deal with them at run time.
!
#include "sysent.s"
