/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)ws_cmap.c	1.16	96/06/02 SMI"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/termios.h>
#include <sys/strtty.h>
#include <sys/kd.h>
#include <sys/promif.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

extern keymap_t	kdkeymap;
extern esctbl_t	kdesctbl;
extern ushort	kb_shifttab[];

/*
 * Some debugging macros
 */

#if defined(DEBUG)
#define KD_DEBUG
#endif

#ifdef	KD_DEBUG
extern int kd_getchar_debug;
#define	DEBUG_KDGETCHAR(f)	if (kd_getchar_debug) { prom_printf f; }
#else
#define	DEBUG_KDGETCHAR(f)	/* nothing */
#endif

/*
 * Translate raw scan code into stripped, usable form.
 */

int
ws_procscan(kbstate_t *kbp, unchar scan)
{
	register int	indx;
	unchar	stscan,			/* stripped scan code */
		oldprev;		/* old previous scan code */
	
        stscan = scan & ~KBD_BREAK;
	oldprev = kbp->kb_prevscan;
	kbp->kb_prevscan = scan;
	if (oldprev == 0xe1) {
		if (stscan == 0x1d)
			kbp->kb_prevscan = oldprev;
		else if (stscan == 0x45)
			return (0x77);
	} else if (oldprev == 0xe0) {
		for (indx = 0; indx < ESCTBLSIZ; indx++) {
			if (kdesctbl[indx][0] == stscan)
				return (kdesctbl[indx][1]);
		}
	} else if (scan != 0xe0 && scan != 0xe1)
		return (stscan);
	return (0);
}

/*
 *
 */

ws_getstate(kmp, kbp, scan)
keymap_t	*kmp;
kbstate_t	*kbp;
unchar	scan;
{
	unsigned	state = 0;

	if (kbp->kb_state & SHIFTSET)
		state |= SHIFTED;
	if (kbp->kb_state & CTRLSET)
		state |= CTRLED;
	if (kbp->kb_state & ALTSET)
		state |= ALTED;
	if ((kmp->key[scan].flgs & KMF_CLOCK && kbp->kb_state & CAPS_LOCK) ||
		(kmp->key[scan].flgs & KMF_NLOCK && kbp->kb_state & NUM_LOCK))
		state ^= SHIFTED;	/* Locked - invert shift state */
	return (state);
}

/*
 *
 */

ushort
ws_transchar(kmp, kbp, scan)
keymap_t	*kmp;
kbstate_t	*kbp;
unchar	scan;
{
	DEBUG_KDGETCHAR(("ws_transchar:  %x->%x->%x\n",
		scan, ws_getstate(kmp,kbp,scan),
		((ushort)kmp->key[scan].map[ws_getstate(kmp, kbp, scan)])));

	return ((ushort)kmp->key[scan].map[ws_getstate(kmp, kbp, scan)]);
}

/*
 *
 */

ws_statekey(ch, kbp, kbrk)
ushort	ch;
kbstate_t	*kbp;
unchar	kbrk;
{
	ushort	shift;
	ushort	togls = 0;

	switch (ch) {
	case K_SLK: 
		togls = kbp->kb_togls;
		/*FALLTHROUGH*/
	case K_ALT:
	case K_LAL:
	case K_RAL:
	case K_LSH:
	case K_RSH:
	case K_CTL:
	case K_CLK:
	case K_NLK:
	case K_LCT:
	case K_RCT:
		shift = kb_shifttab[ch];
		break;
	case K_AGR:
		shift = kb_shifttab[K_ALT] | kb_shifttab[K_CTL];
		break;
	default:
		return (0);
	}

	DEBUG_KDGETCHAR(("ws_statekey(%x):  ", ch));
	if (kbrk) {
		DEBUG_KDGETCHAR(("break  "));
		if (shift & NONTOGGLES) {
			DEBUG_KDGETCHAR(("non-toggle old=%x  ", kbp->kb_state));
			kbp->kb_state &= ~shift;	/* state off */
			DEBUG_KDGETCHAR(("new=%x  ", kbp->kb_state));
		} else {
			DEBUG_KDGETCHAR(("toggle old=%x  ", kbp->kb_togls));
			kbp->kb_togls &= ~shift;	/* state off */
			DEBUG_KDGETCHAR(("new=%x  ", kbp->kb_togls));
		}
	} else {
		DEBUG_KDGETCHAR(("make  "));
		if (shift & NONTOGGLES) {
			DEBUG_KDGETCHAR(("non-toggle old=%x  ", kbp->kb_state));
			kbp->kb_state |= shift;	/* state on */
			DEBUG_KDGETCHAR(("new=%x  ", kbp->kb_state));
		} else if (!(kbp->kb_togls & shift)) {
			DEBUG_KDGETCHAR(("toggle old=%x,%x  ",
				kbp->kb_state,kbp->kb_togls));
			kbp->kb_state ^= shift;	/* invert state */
			kbp->kb_togls |= shift;
			DEBUG_KDGETCHAR(("new=%x,%x  ",
				kbp->kb_state,kbp->kb_togls));
		}
	}
	
	DEBUG_KDGETCHAR(("\n"));

	if ((ch == K_SLK) && !kbrk && (togls != kbp->kb_togls))
		return (0);
	return (1);
}

/*
 *
 */

ws_specialkey(kmp, kbp, scan)
keymap_t	*kmp;
kbstate_t	*kbp;
unchar	scan;
{
	return (IS_SPECKEY(kmp, scan, ws_getstate(kmp, kbp, scan)));
}

/*
 *
 */

ushort
ws_esckey(ch, scan, kbp, kbrk)
ushort	ch;
kbstate_t	*kbp;
unchar	kbrk;
{
	ushort	newch = ch;
	keymap_t	*kmp = &kdkeymap;
	unsigned	state;

	if (IS_FUNKEY(ch))
		return (!kbrk ? (GEN_FUNC | ch) : NO_CHAR);
	if (!kbrk) {
		switch (ch) {
		case K_BTAB:
			newch = GEN_ESCLSB | 'Z';
			break;
		case K_ESN:
			state = ws_getstate(kmp, kbp, scan);
			newch = GEN_ESCN | kmp->key[scan].map[state ^ ALTED];
			break;
		case K_ESO:
			state = ws_getstate(kmp, kbp, scan);
			newch = GEN_ESCO | kmp->key[scan].map[state ^ ALTED];
			break;
		case K_ESL:
			state = ws_getstate(kmp, kbp, scan);
			newch = GEN_ESCLSB | kmp->key[scan].map[state ^ ALTED];
			break;
		default:
			break;
		}
	}
	return (newch);
}

/*
 *
 */

ushort
ws_scanchar(kbstate_t *kbp, unsigned char rawscan)
{
	register unsigned char	scan;	/* "cooked" scan code */
	keymap_t *kmp;
	unchar	kbrk;
	ushort ch;

	kbrk = rawscan & KBD_BREAK;	/* extract make/break from scan */

	kmp = &kdkeymap;
	scan = ws_procscan(kbp, rawscan);
	if (!scan || (int) scan > kmp->n_keys) {
		return (NO_CHAR);
	}

	ch = ws_transchar(kmp,kbp,scan);
	DEBUG_KDGETCHAR(("ws_scanchar:  %x->%x->%x\n", rawscan, scan, ch));
	if (ws_specialkey(kmp, kbp, scan)) {
		ch = ws_esckey(ch, scan, kbp, kbrk);
		if (ws_statekey(ch, kbp, kbrk))
		       return (NO_CHAR);
	}
	return (kbrk ? NO_CHAR : ch);
}

int
ws_toglchange(ostate,nstate)
register ushort	ostate,nstate;
{
	if ((ostate & (CAPS_LOCK | NUM_LOCK)) ==
				(nstate & (CAPS_LOCK | NUM_LOCK)))
		return 0;
	else
		return 1;
}
