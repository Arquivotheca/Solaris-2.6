/*
 *	gen_dhkeys.c
 *
 *	Copyright (c) 1988-1994 Sun Microsystems Inc
 *	All Rights Reserved.
 */

#pragma ident	"@(#)gen_dhkeys.c	1.12	96/04/05 SMI"

#include <mp.h>
#include <rpc/key_prot.h>

extern long	random();
static void	getseed();
static void	adjust();
void		__gen_dhkeys();


/*
 * Generate a seed
 */
static void
getseed(seed, seedsize, pass)
char *seed;
int seedsize;
unsigned char *pass;
{
	int i;
	int rseed;
	struct timeval tv;

	(void) gettimeofday(&tv, (struct timezone *)NULL);
	rseed = tv.tv_sec + tv.tv_usec;
	for (i = 0; i < 8; i++) {
		rseed ^= (rseed << 8) | pass[i];
	}
	(void) srandom(rseed);

	for (i = 0; i < seedsize; i++) {
		seed[i] = (random() & 0xff) ^ pass[i % 8];
	}
}

/*
 * Adjust the input key so that it is 0-filled on the left
 */
static void
adjust(keyout, keyin)
char keyout[HEXKEYBYTES + 1];
char *keyin;
{
	char *p;
	char *s;

	for (p = keyin; *p; p++)
		;
	for (s = keyout + HEXKEYBYTES; p >= keyin; p--, s--) {
		*s = *p;
	}
	while (s >= keyout) {
		*s-- = '0';
	}
}

/*
 * generate a Diffie-Hellman key-pair based on the given password.
 * public and secret are buffers of size HEXKEYBYTES + 1.
 */
void
__gen_dhkeys(public, secret, pass)
char *public;
char *secret;
char *pass;
{
	int i;

#define	BASEBITS	(8 * sizeof (short) - 1)
#define	BASE		(1 << BASEBITS)

	MINT *pk = mp_itom(0);
	MINT *sk = mp_itom(0);
	MINT *tmp;
	MINT *base = mp_itom(BASE);
	MINT *root = mp_itom(PROOT);
	MINT *modulus = mp_xtom(HEXMODULUS);
	unsigned short r;
	unsigned short seed[KEYSIZE/BASEBITS + 1];
	char *xkey;

	(void) getseed((char *)seed, sizeof (seed), (u_char *)pass);
	for (i = 0; i < KEYSIZE/BASEBITS + 1; i++) {
		r = seed[i] % ((unsigned short)BASE);
		tmp = mp_itom(r);
		mp_mult(sk, base, sk);
		mp_madd(sk, tmp, sk);
		mp_mfree(tmp);
	}
	tmp = mp_itom(0);
	mp_mdiv(sk, modulus, tmp, sk);
	mp_mfree(tmp);
	mp_pow(root, sk, modulus, pk);
	xkey = mp_mtox(sk);
	(void) adjust(secret, xkey);
	xkey = mp_mtox(pk);
	(void) adjust(public, xkey);
	mp_mfree(sk);
	mp_mfree(base);
	mp_mfree(pk);
	mp_mfree(root);
	mp_mfree(modulus);
} 
