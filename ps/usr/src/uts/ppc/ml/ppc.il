/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)ppc.il	1.7	96/07/17 SMI"

/*
 * In-line functions for PowerPC kernels.
 */

	.inline	htonl,4
	mr	%r4, %r3
	rlwimi	%r3, %r4, 24, 0, 31	! %r3 = B0 B3 B2 B1
	rlwimi	%r3, %r4, 8, 8, 15	! %r3 = B0 B1 B2 B1
	rlwimi	%r3, %r4, 8, 24, 31	! %r3 = B0 B1 B2 B3
	.end

	.inline	ntohl,4
	mr	%r4, %r3
	rlwimi	%r3, %r4, 24, 0, 31	! %r3 = B0 B3 B2 B1
	rlwimi	%r3, %r4, 8, 8, 15	! %r3 = B0 B1 B2 B1
	rlwimi	%r3, %r4, 8, 24, 31	! %r3 = B0 B1 B2 B3
	.end

	.inline	htons,4
	rlwimi	%r3, %r3, 8, 0, 31	! %r3 = X B1 B0 X
	rlwimi	%r3, %r3, 16, 24, 31	! %r3 = X B1 B0 B1
	andi.	%r3, %r3, 0xffff	! %r3 = 0 0 B0 B1
	.end

	.inline	ntohs,4
	rlwimi	%r3, %r3, 8, 0, 31	! %r3 = X B1 B0 X
	rlwimi	%r3, %r3, 16, 24, 31	! %r3 = X B1 B0 B1
	andi.	%r3, %r3, 0xffff	! %r3 = 0 0 B0 B1
	.end
