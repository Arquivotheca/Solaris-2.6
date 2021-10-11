/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)cc.h	1.1	93/10/29 SMI"

#define H_TRUE -1
#define H_FALSE	0
#define SIGNATURE_OFFSET 0x14		/* PCMCIA Manufacturers ID */


/* delay */
/* */
/*	create delay for strobe pulses */
/* */
/* Changed to 1 jmp $+2 on 6-2-92 */

#ifdef NEEDSWORK
delay			macro
			jmp	$ + 2
			endm

delay386		macro
			push	ax
			in	al, 61h
			pop	ax
			endm

bigdelay		macro
			push	cx
			mov	cx, 100h
@@:			in	al, 61h
			loop	@B
			pop	cx
			endm
#endif
