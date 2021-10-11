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

#ident	"@(#)madd.c	1.3	96/04/05 SMI"	/* SVr4.0 1.1	*/

/* LINTLIBRARY */

#include <mp.h>

extern short *_mp_xalloc();

static void
m_add(a, b, c)
	register struct mint *a, *b, *c;
{
	register int carry, i;
	register int x;
	register short *cval;

	cval = _mp_xalloc(a->len + 1, "m_add");
	carry = 0;
	for (i = 0; i < b->len; i++) {
		x = carry + a->val[i] + b->val[i];
		if (x & 0100000) {
			carry = 1;
			cval[i] = x & 077777;
		} else {
			carry = 0;
			cval[i] = x;
		}
	}
	for (; i < a->len; i++) {
		x = carry + a->val[i];
		if (x & 0100000) {
			cval[i] = x & 077777;
		} else {
			carry = 0;
			cval[i] = x;
		}
	}
	if (carry == 1) {
		cval[i] = 1;
		c->len = i + 1;
	} else {
		c->len = a->len;
	}
	c->val = cval;
	if (c->len == 0) {
		free((char *) cval);
	}
}

void
mp_madd(a, b, c)
	register struct mint *a, *b, *c;
{
	struct mint x, y;
	int sign;

	x.len = y.len = 0;
	_mp_move(a, &x);
	_mp_move(b, &y);
	_mp_xfree(c);
	sign = 1;
	if (x.len >= 0) {
		if (y.len >= 0) {
			if (x.len >= y.len) {
				m_add(&x, &y, c);
			} else {
				m_add(&y, &x, c);
			}
		} else {
			y.len = -y.len;
			mp_msub(&x, &y, c);
		}
	} else {
		if (y.len <= 0) {
			x.len = -x.len;
			y.len = -y.len;
			sign = -1;
			mp_madd(&x, &y, c);
		} else {
			x.len = -x.len;
			mp_msub(&y, &x, c);
		}
	}
	c->len = sign * c->len;
	_mp_xfree(&x);
	_mp_xfree(&y);
}

static void
m_sub(a, b, c)
	register struct mint *a, *b, *c;
{
	register int x, i;
	register int borrow;
	short one;
	struct mint mone;

	one = 1;
	mone.len = 1;
	mone.val = &one;
	c->val = _mp_xalloc(a->len, "m_sub");
	borrow = 0;
	for (i = 0; i < b->len; i++) {
		x = borrow + a->val[i] - b->val[i];
		if (x & 0100000) {
			borrow = -1;
			c->val[i] = x & 077777;
		} else {
			borrow = 0;
			c->val[i] = x;
		}
	}
	for (; i < a->len; i++) {
		x = borrow + a->val[i];
		if (x & 0100000) {
			c->val[i] = x & 077777;
		} else {
			borrow = 0;
			c->val[i] = x;
		}
	}
	if (borrow < 0) {
		for (i = 0; i < a->len; i++) {
			c->val[i] ^= 077777;
		}
		c->len = a->len;
		mp_madd(c, &mone, c);
	}
	for (i = a->len-1; i >= 0; --i) {
		if (c->val[i] > 0) {
			if (borrow == 0) {
				c->len = i + 1;
			} else {
				c->len = -i - 1;
			}
			return;
		}
	}
	free((char *) c->val);
}

void
mp_msub(a, b, c)
	register struct mint *a, *b, *c;
{
	struct mint x, y;
	int sign;

	x.len = y.len = 0;
	_mp_move(a, &x);
	_mp_move(b, &y);
	_mp_xfree(c);
	sign = 1;
	if (x.len >= 0) {
		if (y.len >= 0) {
			if (x.len >= y.len) {
				m_sub(&x, &y, c);
			} else {
				sign = -1;
				mp_msub(&y, &x, c);
			}
		} else {
			y.len = -y.len;
			mp_madd(&x, &y, c);
		}
	} else {
		if (y.len <= 0) {
			x.len = -x.len;
			y.len = -y.len;
			mp_msub(&y, &x, c);
		} else {
			x.len = -x.len;
			mp_madd(&x, &y, c);
			sign = -1;
		}
	}
	c->len = sign * c->len;
	_mp_xfree(&x);
	_mp_xfree(&y);
}
