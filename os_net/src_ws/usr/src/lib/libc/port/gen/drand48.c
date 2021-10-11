/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)drand48.c	1.12	96/05/05 SMI"	/* SVr4.0 2.4.1.7	*/

/*LINTLIBRARY*/
/*
 *	drand48, etc. pseudo-random number generator
 *	This implementation assumes unsigned short integers of at least
 *	16 bits, long integers of at least 32 bits, and ignores
 *	overflows on adding or multiplying two unsigned integers.
 *	Two's-complement representation is assumed in a few places.
 *	Some extra masking is done if unsigneds are exactly 16 bits
 *	or longs are exactly 32 bits, but so what?
 *	An assembly-language implementation would run significantly faster.
 */
#ifdef __STDC__
	#pragma weak drand48 = _drand48
	#pragma weak erand48 = _erand48
	#pragma weak lrand48 = _lrand48
	#pragma weak mrand48 = _mrand48
	#pragma weak srand48 = _srand48
	#pragma weak seed48 = _seed48
	#pragma weak lcong48 = _lcong48
	#pragma weak nrand48 = _nrand48
	#pragma weak jrand48 = _jrand48
#endif
#include "synonyms.h"
#include <synch.h>
#include <thread.h>
#include <mtlib.h>

static mutex_t seed_lock = DEFAULTMUTEX;
#ifdef _REENTRANT
#define STATIC static
#define EXPORT0(TYPE, fn, fnu)	TYPE fn() { \
				register TYPE res; \
			        _mutex_lock(&seed_lock); \
			        res = fnu(); \
			        _mutex_unlock(&seed_lock); \
			        return res; }
#define EXPORT1(TYPE, fn, fnu)	TYPE fn(unsigned short xsubi[3]) { \
				register TYPE res; \
			        _mutex_lock(&seed_lock); \
			        res = fnu(xsubi); \
			        _mutex_unlock(&seed_lock); \
			        return res; }
#else
#define _drand48_u _drand48
#define _erand48_u _erand48
#define _lrand48_u _lrand48
#define _mrand48_u _mrand48
#define _nrand48_u _nrand48
#define _jrand48_u _jrand48
#define STATIC
#endif _REENTRANT

#define N	16
#define MASK	((unsigned)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)	((unsigned)(x) & MASK)
#define HIGH(x)	LOW((x) >> N)
#define MUL(x, y, z)	{ long l = (long)(x) * (long)(y); \
		(z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)	((long)(x) + (long)(y) > MASK)
#define ADDEQU(x, y, z)	(z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0	0x330E
#define X1	0xABCD
#define X2	0x1234
#define A0	0xE66D
#define A1	0xDEEC
#define A2	0x5
#define C	0xB
#define SET3(x, x0, x1, x2)	((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
#define SETLOW(x, y, n) SET3(x, LOW((y)[n]), LOW((y)[(n)+1]), LOW((y)[(n)+2]))
#define SEED(x0, x1, x2) (SET3(x, x0, x1, x2), SET3(a, A0, A1, A2), c = C)
#define REST(v)	for (i = 0; i < 3; i++) { xsubi[i] = x[i]; x[i] = temp[i]; } \
		return (v)
#define NEST(TYPE, f, F)  STATIC TYPE f(xsubi) register unsigned short *xsubi; { \
	register int i; register TYPE v; unsigned temp[3]; \
	for (i = 0; i < 3; i++) { temp[i] = x[i]; x[i] = LOW(xsubi[i]); }  \
	v = F(); REST(v); }

/* Way ugly solution to problem names, but it works */
#define x	_drand48_x
#define a	_drand48_a
#define c	_drand48_c
/* End way ugly */
static unsigned x[3] = { X0, X1, X2 }, a[3] = { A0, A1, A2 }, c = C;
static unsigned short lastx[3];
static void next();

STATIC double
_drand48_u()
{
	static double two16m = 1.0 / (1L << N);

	next();
	return (two16m * (two16m * (two16m * x[0] + x[1]) + x[2]));
}

NEST(double, _erand48_u, _drand48_u)

STATIC long
_lrand48_u()
{
	next();
	return (((long)x[2] << (N - 1)) + (x[1] >> 1));
}

STATIC long
_mrand48_u()
{
	next();
	return (((long)x[2] << N) + x[1]);
}

static void
next()
{
	unsigned p[2], q[2], r[2], carry0, carry1;

	MUL(a[0], x[0], p);
	ADDEQU(p[0], c, carry0);
	ADDEQU(p[1], carry0, carry1);
	MUL(a[0], x[1], q);
	ADDEQU(p[1], q[0], carry0);
	MUL(a[1], x[0], r);
	x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] +
		a[0] * x[2] + a[1] * x[1] + a[2] * x[0]);
	x[1] = LOW(p[1] + r[0]);
	x[0] = LOW(p[0]);
}

void
_srand48(seedval)
long seedval;
{
	_mutex_lock(&seed_lock);
	SEED(X0, LOW(seedval), HIGH(seedval));
	_mutex_unlock(&seed_lock);
}

unsigned short *
seed48(seed16v)
unsigned short seed16v[3];
{
	_mutex_lock(&seed_lock);
	SETLOW(lastx, x, 0);
	SEED(LOW(seed16v[0]), LOW(seed16v[1]), LOW(seed16v[2]));
	_mutex_unlock(&seed_lock);
	return (lastx);
}

void
lcong48(param)
unsigned short param[7];
{
	_mutex_lock(&seed_lock);
	SETLOW(x, param, 0);
	SETLOW(a, param, 3);
	c = LOW(param[6]);
	_mutex_unlock(&seed_lock);
}

NEST(long, _nrand48_u, _lrand48_u)

NEST(long, _jrand48_u, _mrand48_u)

#ifdef _REENTRANT

EXPORT0(double,_drand48,_drand48_u)
EXPORT1(double,_erand48,_erand48_u)

EXPORT0(long,_lrand48,_lrand48_u)
EXPORT1(long,_nrand48,_nrand48_u)

EXPORT0(long,_mrand48,_mrand48_u)
EXPORT1(long,_jrand48,_jrand48_u)

#endif /* _REENTRANT */

#ifdef DRIVER
/*
	This should print the sequences of integers in Tables 2
		and 1 of the TM:
	1623, 3442, 1447, 1829, 1305, ...
	657EB7255101, D72A0C966378, 5A743C062A23, ...
 */
#include <stdio.h>

main()
{
	int i;

	for (i = 0; i < 80; i++) {
		printf("%4d ", (int)(4096 * drand48()));
		printf("%.4X%.4X%.4X\n", x[2], x[1], x[0]);
	}
}
#endif
