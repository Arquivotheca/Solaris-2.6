/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1996, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)ws_cmap.c	1.10	96/07/30 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/errno.h"
#include "sys/inline.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/kmem.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "sys/ascii.h"
#include "sys/proc.h"
#include "sys/termio.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/ws/ws.h"
#include "sys/ws/chan.h"
#include "sys/mouse.h"
#include "sys/char.h"

extern stridx_t	kdstrmap;
pfxstate_t kdpfxstr;
extern strmap_t	kdstrbuf;
extern keymap_t	kdkeymap;
extern extkeys_t	ext_keys;
extern esctbl_t	kdesctbl;
extern srqtab_t	srqtab;
extern struct kb_shiftmkbrk	kb_mkbrk[];
extern ushort	kb_shifttab[];
extern struct attrmask	kb_attrmask[];
extern int	nattrmsks;

static unchar ws_ext(unchar *x, ushort y, ushort z);
static int ws_statekey(ushort ch, charmap_t *cmp, kbstate_t *kbp, unchar kbrk);

/* the string index table is not pre-initialized like kdkeymap.
 * This routine initializes it
 */

stridx_t *
ws_dflt_strmap_p(void)
{
	int strix = 0;
	int key;
	static int init_flg = 0;


	if (init_flg) {
		return ( (stridx_t *) kdstrmap);
	}

	init_flg++;

	/* Make sure buffer ends with null */
	kdstrbuf[STRTABLN - 1] = '\0';

	for (key = 0; key < NSTRKEYS; key++) {
		kdstrmap[key] = (ushort)strix;
		/* Search for start of next string */
		while (strix < STRTABLN - 1 && kdstrbuf[strix++])
			;
	}

	return ( (stridx_t *) kdstrmap);
}

int
ws_newscrmap(scrn_t *scrp, int km_flag)
{
	scrnmap_t *nscrp;

	if (scrp->scr_map_p && (scrp->scr_map_p != scrp->scr_defltp->scr_map_p))
		return (1);
	if (nscrp = kmem_alloc(sizeof(scrnmap_t),km_flag)) {
		scrp->scr_map_p = nscrp;
		return (1);
	}
	return (0);
}


/*
 *
 */

int
ws_newkeymap(register charmap_t *cmp, ushort nkeys, keymap_t *nkmp, int km_flag)
{
	int size;
	keymap_t *okmp;
	charmap_t *dcmp;

	okmp = cmp->cr_keymap_p;
	dcmp = cmp->cr_defltp;

	if (okmp == dcmp->cr_keymap_p) {
	   okmp = (keymap_t *)kmem_alloc(sizeof(keymap_t), km_flag);
	   if (okmp == (keymap_t *) NULL)
		return (0);
	}

	size = nkeys*sizeof(okmp->key[0]) + sizeof(okmp->n_keys);
	if(cmp->cr_flags == SCO_FORMAT) {
		xlate_keymap(nkmp, okmp, USL_FORMAT);
		cmn_err(CE_NOTE, "SCO keymap installed");
	} else
		bcopy((caddr_t)nkmp, (caddr_t)okmp, size);
	cmp->cr_flags = USL_FORMAT;
	cmp->cr_keymap_p = okmp;

	return (1);
}


int
ws_newsrqtab(register charmap_t *cmp, int km_flag)
{
	srqtab_t *nsrqp, *osrqp;
	charmap_t *dcmp;

	dcmp = cmp->cr_defltp;
	nsrqp = osrqp = cmp->cr_srqtabp;

	if (osrqp == dcmp->cr_srqtabp) {
	   nsrqp = (srqtab_t *)kmem_alloc(sizeof(srqtab_t), km_flag);
	   if (nsrqp == (srqtab_t *) NULL)
		return (0);
	   bcopy((caddr_t)osrqp,(caddr_t)nsrqp,sizeof(srqtab_t));
	   cmp->cr_srqtabp = nsrqp;
	}

	return (1);
}


int
ws_newstrbuf(register charmap_t *cmp, int km_flag)
{
	strmap_t *osbp, *nsbp;
	stridx_t *osip, *nsip;
	charmap_t *dcmp;

	dcmp = cmp->cr_defltp;

	nsbp = osbp = cmp->cr_strbufp;
	nsip = osip = cmp->cr_strmap_p;

	if (osbp == dcmp->cr_strbufp) {
	   nsbp = (strmap_t *)kmem_alloc(sizeof(strmap_t), km_flag);
	   if (nsbp == (strmap_t *) NULL)
		return (0);

	   nsip = (stridx_t *)kmem_alloc(sizeof(stridx_t), km_flag);
	   if (nsip == (stridx_t *) NULL) {
		kmem_free(nsbp, sizeof(strmap_t));
		return (0);
	   }
	   bcopy((caddr_t)osbp,(caddr_t)nsbp,sizeof(strmap_t));
	   bcopy((caddr_t)osip,(caddr_t)nsip,sizeof(stridx_t));

	   cmp->cr_strbufp = nsbp;
	   cmp->cr_strmap_p = nsip;
	}

	return (1);
}


void
ws_strreset(charmap_t *cmp)
{
	register int strix;		/* Index into string buffer */
	register ushort	key;		/* Function key number */
	register ushort *idxp;
	register unchar *bufp;

	bufp = (unchar *) cmp->cr_strbufp;
	idxp = (ushort *) cmp->cr_strmap_p;

	bufp[STRTABLN - 1] = '\0';	/* Make sure buffer ends with null */
	strix = 0;			/* Start from beginning of buffer */

	for (key = 0; (int) key < NSTRKEYS; key++) {
		idxp[key] = (ushort)strix; /* Point to start of string */
		while (strix < STRTABLN - 1 && bufp[strix++])
			;		/* Find start of next string */
	}
}


int
ws_addstring(charmap_t *cmp, ushort keynum, unchar *str, ushort len)
{
	register int	amount;		/* Amount to move */
	int		cnt,
			oldlen;		/* Length of old string */
	register unchar	*oldstr,	/* Location of old string in table */
			*to,		/* Destination of move */
			*from;		/* Source of move */
	unchar		*bufend,	/* End of string buffer */
			*bufbase,	/* Beginning of buffer */
			*tmp;		/* Temporary pointer into old string */
	ushort		*idxp;


	if ((int)keynum >= NSTRKEYS)	/* Invalid key number? */
		return 0;		/* Ignore string setting */
	len++;				/* Adjust length to count end null */

	if (!ws_newstrbuf(cmp, KM_SLEEP))
		return 0;

	idxp = (ushort *) cmp->cr_strmap_p;
	idxp += keynum;

	oldstr = (unchar *) cmp->cr_strbufp;
	oldstr += *idxp;

	/* Now oldstr points at beginning of old string for key */

	bufbase = (unchar *) cmp->cr_strbufp;
	bufend = bufbase + STRTABLN - 1;

	tmp = oldstr;
	while (*tmp++ != '\0')		/* Find end of old string */
		;

	oldlen =  tmp - oldstr;		/* Compute length of string + null */

	/*
	 * If lengths are different, expand or contract table to fit
	 */
	if (oldlen > (int) len) {	/* Move up for shorter string? */
		from = oldstr + oldlen;	/* Calculate source */
		to = oldstr + len;	/* Calculate destination */
		amount = STRTABLN - (oldstr - bufbase) - oldlen;
		for (cnt = amount; cnt > 0; cnt--)
			*to++ = *from++;
	} else if (oldlen < (int) len) { /* Move down for longer string? */
		from = bufend - (len - oldlen);	/* Calculate source */
		to = bufend;		/* Calculate destination */
		if (from < (oldstr + len))	/* String won't fit? */
			return 0;	/* Return without doing anything */
		amount = STRTABLN - (oldstr - bufbase) - len;	/* Move length */
		while (--amount >= 0)		/* Copy whole length */
			*to-- = *from--;	/* Copy character at a time */
	}

	len--;				/* Remove previous addition for null */
	bcopy((caddr_t)str, (caddr_t)oldstr, len);	/* Install new string over old */
	*(oldstr + len)  = '\0';	/* Terminate string will null */
	ws_strreset(cmp);		/* Readjust string index table */
	return 1;
}


int
ws_newpfxstr(register charmap_t *cmp, int km_flag)
{
	pfxstate_t *npfxp, *opfxp;
	charmap_t *dcmp;

	dcmp = cmp->cr_defltp;
	npfxp = opfxp = cmp->cr_pfxstrp;

	if (opfxp == dcmp->cr_pfxstrp)
	{
	   npfxp = (pfxstate_t *)kmem_alloc(sizeof(pfxstate_t), km_flag);
	   if (npfxp == (pfxstate_t *) NULL)
		return (0);

	   bcopy((caddr_t)opfxp,(caddr_t)npfxp,sizeof(pfxstate_t));

	   cmp->cr_pfxstrp = npfxp;
	}

	return (1);
}


/*ARGSUSED*/
void
ws_scrn_init(wstation_t *wsp, int km_flag)
{
	scrn_t *scrp;

	scrp = &wsp->w_scrn;
	scrp->scr_defltp = scrp;
	scrp->scr_map_p = (scrnmap_t *) NULL;
}

void
ws_cmap_init(wstation_t *wsp, int km_flag)
{
	charmap_t *cmp;

	cmp = &wsp->w_charmap;
	wsp->w_charmap.cr_defltp = cmp;

	cmp->cr_keymap_p = (keymap_t *) &kdkeymap;
	cmp->cr_extkeyp =  (extkeys_t *) ext_keys;
	cmp->cr_esctblp =  (esctbl_t *) kdesctbl;
	cmp->cr_strbufp =  (strmap_t *) kdstrbuf;
	cmp->cr_srqtabp =  (srqtab_t *) srqtab;
	cmp->cr_strmap_p = ws_dflt_strmap_p();
	cmp->cr_pfxstrp = (pfxstate_t *) kdpfxstr;
	cmp->cr_flags   = USL_FORMAT;
	cmp->cr_kbmode  = KBM_XT;

	if (!ws_newkeymap(cmp, kdkeymap.n_keys, &kdkeymap,km_flag))
		cmn_err(CE_PANIC,
		    "ws_cmap_init: Unable to allocate space for keyboard map");

	if (!ws_newsrqtab(cmp,km_flag))
		cmn_err(CE_PANIC,
		    "ws_cmap_init: Unable to allocate space for system request table");

	if (!ws_newstrbuf(cmp,km_flag))
		cmn_err(CE_PANIC,
		    "ws_cmap_init: Unable to allocate space for function keys");

	if (!ws_newpfxstr(cmp,km_flag))
		cmn_err(CE_PANIC,
		    "ws_cmap_init: Unable to allocate space for prefix strings");
}




/*
 * initialize per-channel state structure for screen map
 */

void
ws_scrn_alloc(wstation_t *wsp, channel_t *chp)
{
	bcopy((caddr_t)&wsp->w_scrn,(caddr_t)&chp->ch_scrn,sizeof(scrn_t));
}


/* return pointer to initialized charmap_t or NULL if kmem_alloc
 * cannot allocate structure
 *
 * "kmem_flag" specifies whether we should call to kmem_alloc sleep or not.
 */

charmap_t *
ws_cmap_alloc(wstation_t *wsp, int kmem_flag)
{
	charmap_t *cmp;

	cmp = (charmap_t *) kmem_alloc(sizeof(charmap_t), kmem_flag);
	if (cmp == (charmap_t *) NULL)
		return ( (charmap_t *) NULL);

	bcopy((caddr_t)&wsp->w_charmap,(caddr_t)cmp,sizeof(charmap_t));
	return (cmp);
}

static void
ws_scrn_release(register scrn_t *scrp)
{
	register scrn_t *dcmp;
	register int same;

	if (scrp == (scrn_t *) NULL)
		return;
	dcmp = scrp->scr_defltp;
	same = (dcmp == scrp);

	if (same || scrp->scr_map_p != dcmp->scr_map_p) {
		if (scrp->scr_map_p)
			kmem_free(scrp->scr_map_p,sizeof(scrnmap_t));
		scrp->scr_map_p = (scrnmap_t *) NULL;
	}
}

static void
ws_cmap_release(register charmap_t *cmp)
{
	register charmap_t *dcmp;
	register int same;

	if (cmp == (charmap_t *) NULL)
		return;
	dcmp = cmp->cr_defltp;
	same = (dcmp == cmp);
	if (same || cmp->cr_keymap_p != dcmp->cr_keymap_p)
		kmem_free(cmp->cr_keymap_p, sizeof(keymap_t));
	if (same || cmp->cr_extkeyp != dcmp->cr_extkeyp)
		kmem_free(cmp->cr_keymap_p, sizeof(extkeys_t));
	if (same || cmp->cr_esctblp !=  dcmp->cr_esctblp)
		kmem_free(cmp->cr_esctblp, sizeof(esctbl_t));
	if (same || cmp->cr_strbufp != dcmp->cr_strbufp)
		kmem_free(cmp->cr_strbufp, sizeof(strmap_t));
	if (same || cmp->cr_srqtabp != dcmp->cr_srqtabp)
		kmem_free(cmp->cr_srqtabp, sizeof(srqtab_t));
	if (same || cmp->cr_strmap_p != dcmp->cr_strmap_p)
		kmem_free(cmp->cr_strmap_p, sizeof(stridx_t));
}

/*ARGSUSED*/
void
ws_scrn_free(wstation_t *wsp, channel_t *chp)
{
	ws_scrn_release(&chp->ch_scrn);
}

/* release the allocated cmap structure. If any of the members
 * are not the default, assume that they were allocated via
 * kmem_alloc and should be released via kmem_free
 */

void
ws_cmap_free(wstation_t *wsp, charmap_t *cmp)
{
	if (cmp == (charmap_t *) NULL)
		return;
	ws_cmap_release(cmp);
	if (cmp != &wsp->w_charmap)
		kmem_free(cmp, sizeof(charmap_t));
	return;
}

void
ws_scrn_reset(wstation_t *wsp, register channel_t *chp)
{
	ws_scrn_release(&chp->ch_scrn);
	bcopy((caddr_t)&wsp->w_scrn,(caddr_t)&chp->ch_scrn,sizeof(scrn_t));
}


/*ARGSUSED*/
void
ws_cmap_reset(wstation_t *wsp, register charmap_t *cmp)
{
	ws_cmap_release(cmp);
	bcopy((caddr_t)cmp->cr_defltp,(caddr_t)cmp,sizeof(charmap_t));
}

/*
 *
 */

void
ws_kbtime(wstation_t *wsp)
{
	register mblk_t *mp;

	ASSERT(UNSAFE_DRIVER_LOCK_HELD());
	wsp->w_timeid = 0;
	if ((mp=wsp->w_mp) != NULL) { /* something to send up? */
		if (mp->b_wptr != mp->b_rptr) { /* make sure there's data */
			if (wsp->w_qp == (queue_t *)NULL)
				freemsg(mp);	/* nowhere to send it */
			else
				putnext(wsp->w_qp, mp);
			/*
			 * Now that we've sent (or freed) the mblk, we need
			 * to allocate a new one.
			 */
			wsp->w_mp = allocb(4, BPRI_MED);
		}
	}
}

/*
 * Place the scancode in the message block, and return 1 if there
 * is still room left in the message block. Return 0 if the data
 * was sent up or dropped (used to indicate whether a timer should
 * be set to make sure the data gets sent up).
 */

int
ws_enque(queue_t *qp, mblk_t **mpp, unchar scan)
{
	register mblk_t *mp;

	if (!mpp || !qp || !(mp = *mpp))
		return (0);	/* bail out if invalid qp or mpp */

	*mp->b_wptr++ = scan;
	if (mp->b_wptr == mp->b_datap->db_lim) {	/* mblk is full */
		putnext(qp, mp);		/* send it on its way */
		*mpp = allocb(4, BPRI_MED);	/* and allocate another */
		return(0);
	}
	return(1);
}


/*
 * Translate raw scan code into stripped, usable form.
 */

int
ws_procscan(charmap_t *chmp, kbstate_t *kbp, unchar scan)
{
	register int	indx;
	unchar	stscan,			/* stripped scan code */
		oldprev;		/* old previous scan code */

        stscan = scan & ~KBD_BREAK;
	oldprev = kbp->kb_prevscan;
	kbp->kb_prevscan = scan;
	if (oldprev == 0xe1) {		/* special handling for Pause/Break */
		if (stscan == 0x1d)
			kbp->kb_prevscan = oldprev;
		else if (stscan == 0x45)
			return(0x77);
	} else if (oldprev == 0xe0) {
		for (indx = 0; indx < ESCTBLSIZ; indx++) {
			if ((*chmp->cr_esctblp)[indx][0] == stscan)
				return((*chmp->cr_esctblp)[indx][1]);
		}
	} else if (scan != 0xe0 && scan != 0xe1)
		return(stscan);
	return(0);
}


/*
 * Reset the make/break state.
 */

void
ws_rstmkbrk(queue_t *qp, mblk_t **mpp, ushort state, ushort mask)
{
	register int	cnt, make;
	unchar	prfx = 0xe0;

	for (cnt = 0; cnt < 6; cnt++) {	/* non-toggles */
		if (!(mask & kb_mkbrk[cnt].mb_mask))
			continue;
		if (kb_mkbrk[cnt].mb_prfx)
			(void)ws_enque(qp, mpp, prfx);
		make = (state & kb_mkbrk[cnt].mb_mask) ? 1 : 0;
		if (make)
			(void)ws_enque(qp, mpp, kb_mkbrk[cnt].mb_make);
		else
			(void)ws_enque(qp, mpp, kb_mkbrk[cnt].mb_break);
	}
}

/*
 *
 */
int
ws_getstate(keymap_t *kmp, kbstate_t *kbp, unchar scan)
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
	return(state);
}

void
ws_xferkbstat(register kbstate_t *okbp, register kbstate_t *nkbp)
{
	unsigned state;

	state = okbp->kb_state & NONTOGGLES;
	state |= nkbp->kb_state & (CAPS_LOCK | NUM_LOCK | SCROLL_LOCK);
	nkbp->kb_state = state;
	/* turn off NONTOGGLES in okbp kbstate */
	okbp->kb_state &= (CAPS_LOCK | NUM_LOCK | SCROLL_LOCK);
}

/*
 *
 */

ushort
ws_transchar(keymap_t *kmp, kbstate_t *kbp, unchar scan)
{
	return((ushort)kmp->key[scan].map[ws_getstate(kmp, kbp, scan)]);
}

/*
 *
 */
void
ws_set_char_mode (kbstate_t *kbp, long mode)
{
	if (mode & C_RAWMODE) {
		kbp->kb_proc_state = B_FALSE;
		kbp->kb_rstate = kbp->kb_state;
	} else {
		kbp->kb_proc_state = B_TRUE;
		kbp->kb_state = kbp->kb_rstate;
		kdkb_cmd (LED_WARN, FROM_DRIVER);
	}
}

/*
 *
 */

/*ARGSUSED*/
int
ws_statekey(ushort ch, charmap_t *cmp, kbstate_t *kbp, unchar kbrk)
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
		return(0);
	}

	if (kbrk) {
		if (shift & NONTOGGLES)
			kbp->kb_state &= ~shift;	/* state off */
		else
			kbp->kb_togls &= ~shift;	/* state off */
	} else {
		if (shift & NONTOGGLES)
			kbp->kb_state |= shift;	/* state on */
		else if (!(kbp->kb_togls & shift)) {
			kbp->kb_state ^= shift;	/* invert state */
			kbp->kb_togls |= shift;
		}
	}
	if ((ch == K_SLK) && !kbrk && (togls != kbp->kb_togls))
		return(0);
	return(1);
}

/*
 *
 */

int
ws_specialkey(keymap_t *kmp, kbstate_t *kbp, unchar scan)
{
	return(IS_SPECKEY(kmp, scan, ws_getstate(kmp, kbp, scan)));
}

/*
 *
 */
ushort
ws_shiftkey(ushort ch, unchar scan, keymap_t *kmp, kbstate_t *kbp, unchar kbrk)
{
	ushort	shift;

	if (ws_specialkey(kmp,kbp,scan)) {
		switch (ch) {
		case K_LAL:
		case K_RAL:
		case K_LCT:
		case K_RCT:
		case K_SLK:
		case K_ALT:
		case K_LSH:
		case K_RSH:
		case K_CTL:
		case K_CLK:
		case K_NLK:
			shift = kb_shifttab[ch];
			if (kbrk) {
				if (shift & NONTOGGLES)
					kbp->kb_state &= ~shift;
				else
					kbp->kb_togls &= ~shift;
			} else {
				if (shift & NONTOGGLES)
					kbp->kb_state |= shift;
				else if (!(kbp->kb_togls & shift)) {
					kbp->kb_state ^= shift;
					kbp->kb_togls |= shift;
				}
			}
			return(shift);
		default:
			break;
		}
	}
	return(0);
}

/*
 *
 */

int
ws_speckey(ushort ch)
{
	if (ch >= K_VTF && ch <= K_VTL)
		return(HOTKEY);
	switch (ch) {
	case K_NEXT:
	case K_PREV:
	case K_FRCNEXT:
	case K_FRCPREV:
		return(HOTKEY);
	default:
		return(NO_CHAR);
	}
}

/*
 *
 */
static unchar
ws_ext(unchar *x, ushort y, ushort z)
{
	return *(x + y*NUMEXTSTATES + z);
}


ushort
ws_extkey(unchar ch, charmap_t *cmp, kbstate_t *kbp, unchar scan)
{
	unsigned state = 0;
	unsigned estate = 0;

	extkeys_t *ext_keys;
	strmap_t *strbufp;
	stridx_t *strmap_p;
	keymap_t *kmp;
	ushort idx;


	ext_keys = cmp->cr_extkeyp;
	strbufp = cmp->cr_strbufp;
	strmap_p = cmp->cr_strmap_p;
	kmp = cmp->cr_keymap_p;

	state = ws_getstate(kmp,kbp,scan);

	/* Get index */
	estate = (state & ALTED) ? 3 : ((state & CTRLED) ? 2 : state);

	/*
	 * If the entry in ext_keys[][] is K_NOP and we don't have a
	 * ctl-ScrollLock, continue processing.	 The special test for
	 * ctl-ScrollLock is necessary because its value in
	 * ext_keys[][] is 0.  0 also happens to be what K_NOP is
	 * defined to be.  Unfortunately, ctl-ScrollLock in extended
	 * mode must return 0 as the extra byte.
	 */

	if (ws_ext((unchar *)ext_keys, scan, estate) == K_NOP &&
	    !((state & CTRLED) && (scan == SCROLLLOCK)))
	{
		/* Not extended? */
		return (ch);
	}

	/*
	 * If this key is a string key, but not a number pad key, and
	 * the string key is not null, let kdspecialkey() output the
	 * string.
	 */

	if (IS_FUNKEY(ch) && ws_specialkey(kmp,kbp,scan) && !IS_NPKEY(scan)) {
		idx = (*strmap_p)[ch - K_FUNF];
		if ((*strbufp)[idx] != '\0') {
			return (ch);	/* Not extended character */
		}
	}

	/*
	 * If this is a number pad key and the alt key is depressed,
	 * then the user is building a code (modulo 256).
	 */

	if (IS_NPKEY(scan) && (state & ALTED))	/* Alt number pad? */
	{
	   if (kbp->kb_altseq == -1)	/* No partial character yet? */
		kbp->kb_altseq =
		    ws_ext((unchar *)ext_keys,scan,estate) & 0xf; /* Start */

	   else /* Partial character present */
		kbp->kb_altseq = ((kbp->kb_altseq * 10) +
		    ( ws_ext((unchar *)ext_keys,scan,estate) & 0xf)) & 0xff;	

	   return(NO_CHAR);/* Return no character yet */
	}

	if (state & ALTED)	/* Alt key present without number? */
	   kbp->kb_altseq = -1;	/* Make sure no partial characters */

	return (GEN_ZERO |
	    ws_ext((unchar *)ext_keys,scan,estate));	/* Add zero */
}

/*
 *
 */
ushort
ws_esckey(ushort ch, unchar scan, charmap_t *cmp, kbstate_t *kbp, unchar kbrk)
{
	ushort	newch = ch;
	keymap_t	*kmp = cmp->cr_keymap_p;
	unsigned	state;

	if (IS_FUNKEY(ch))
		return(!kbrk ? (GEN_FUNC | ch) : NO_CHAR);
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
	} else
		switch (ch) {
		case K_LAL:
		case K_RAL:
		case K_ALT:
			if (kbp->kb_altseq != -1) {
				newch = kbp->kb_altseq;
				kbp->kb_altseq = -1;
			}
			break;
		default:
			break;
		}

	return(newch);
}

/*
 *
 */

ushort
ws_scanchar(
	charmap_t *cmp,			/* character map pointer */
	kbstate_t *kbp,			/* pointer to keyboard state */
	unsigned char rawscan,
	unsigned int israw)
{
	register unsigned char	scan;	/* "cooked" scan code */
	keymap_t *kmp;
	unchar	kbrk;
	ushort ch;

	kbrk = rawscan & KBD_BREAK;	/* extract make/break from scan */
	kmp = cmp->cr_keymap_p;

	if (cmp->cr_kbmode == KBM_AT)
		scan = char_xlate_at2xt(rawscan);
	else
		scan = rawscan;

	scan = ws_procscan(cmp, kbp, scan);
	if (!scan || (int) scan > kmp->n_keys) {
		if (!israw)
			return (NO_CHAR);
		else
			return (rawscan);
	}

	ch = ws_transchar(kmp,kbp,scan);
	if (!israw && kbp->kb_extkey && kbrk == 0) { /* Possible ext key?*/
		char nch;
		nch = ws_extkey(ch, cmp, kbp, scan);
		if (nch != ch)
			return (nch); /* kbrk is 0 and ! raw */
	} else
		kbp->kb_altseq = -1;

	if (ws_specialkey(kmp, kbp, scan)) {
		ch = ws_esckey(ch, scan, cmp, kbp, kbrk);
		if (ws_statekey(ch, cmp, kbp, kbrk)) {
			if (!israw)
			   return(NO_CHAR);
		}
	}

	if (israw)
		return (ushort) (rawscan);

	return (kbrk ? NO_CHAR : ch);
}

int
ws_toglchange(register ushort ostate, register ushort nstate)
{
	if ((ostate & (CAPS_LOCK|NUM_LOCK)) == (nstate & (CAPS_LOCK|NUM_LOCK)))
		return 0;
	else
		return 1;
}

/*
 *
 */

unchar
ws_getled(kbstate_t *kbp)
{
	unchar	tmp_led = 0;

	if (kbp->kb_state & CAPS_LOCK)
		tmp_led |= LED_CAP;
	if (kbp->kb_state & NUM_LOCK)
		tmp_led |= LED_NUM;
	if (kbp->kb_state & SCROLL_LOCK)
		tmp_led |= LED_SCR;
	return(tmp_led);
}
