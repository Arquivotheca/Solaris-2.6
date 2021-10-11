/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/* 	Portions Copyright(c) 1988, Sun Microsystems Inc.	*/
/*	All Rights Reserved					*/

#ident	"@(#)gcd.c	1.3	96/04/05 SMI"	/* SVr4.0 1.1	*/

/* LINTLIBRARY */

#include <mp.h>

void
mp_gcd(a, b, c)
	MINT *a, *b, *c;
{
	MINT x, y, z, w;

	x.len = y.len = z.len = w.len = 0;
	_mp_move(a, &x);
	_mp_move(b, &y);
	while (y.len != 0) {
		mp_mdiv(&x, &y, &w, &z);
		_mp_move(&y, &x);
		_mp_move(&z, &y);
	}
	_mp_move(&x, c);
	_mp_xfree(&x);
	_mp_xfree(&y);
	_mp_xfree(&z);
	_mp_xfree(&w);
}

void
mp_invert(x1, x0, c)
	MINT *x1;
	MINT *x0;
	MINT *c;
{
	MINT u2, u3;
	MINT v2, v3;
	MINT zero;
	MINT q, r;
	MINT t;
	MINT x0_prime;
	static MINT *one = (MINT *)0;

	/*
	 * Minimize calls to allocators.  Don't use pointers for local
	 * variables, for the one "initialized" multiple precision
	 * variable, do it just once.
	 */
	if (one == (MINT *)0)
		one = mp_itom((short)1);

	zero.len = q.len = r.len = t.len = 0;

	x0_prime.len = u2.len = u3.len = 0;
	_mp_move(x0, &u3);
	_mp_move(x0, &x0_prime);

	v2.len = v3.len = 0;
	_mp_move(one, &v2);
	_mp_move(x1, &v3);

	while (mp_mcmp(&v3, &zero) != 0) {
		/* invariant: x0*u1 + x1*u2 = u3 */
		/* invariant: x0*v1 + x2*v2 = v3 */
		/* invariant: x(n+1) = x(n-1) % x(n) */
		mp_mdiv(&u3, &v3, &q, &r);
		_mp_move(&v3, &u3);
		_mp_move(&r, &v3);

		mp_mult(&q, &v2, &t);
		mp_msub(&u2, &t, &t);
		_mp_move(&v2, &u2);
		_mp_move(&t, &v2);
	}
	/* now x0*u1 + x1*u2 == 1, therefore,  (u2*x1) % x0  == 1 */
	_mp_move(&u2, c);
	if (mp_mcmp(c, &zero) < 0) {
		mp_madd(&x0_prime, c, c);
	}
	_mp_xfree(&zero);
	_mp_xfree(&v2);
	_mp_xfree(&v3);
	_mp_xfree(&u2);
	_mp_xfree(&u3);
	_mp_xfree(&q);
	_mp_xfree(&r);
	_mp_xfree(&t);
}
