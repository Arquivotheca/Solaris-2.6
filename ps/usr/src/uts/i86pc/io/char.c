/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident "@(#)char.c	1.30	96/07/31 SMI"

/*
 * IWE CHAR module; scan-code translation and screen-mapping
 */


#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/termios.h>
#include <sys/strtty.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/ascii.h>
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/ws/tcl.h>
#include <sys/proc.h>
#include <sys/tss.h>
#ifdef DONT_INCLUDE
#include <sys/xque.h>
#endif
#include <sys/ws/ws.h>
#include <sys/ws/chan.h>
#include <sys/mouse.h>
#include <sys/char.h>
#ifdef _VPIX
#include <sys/v86.h>
#endif
#include <sys/ddi.h>
#include <sys/sunddi.h>

extern int ws_newkeymap();
extern int ws_newpfxstr();
extern int ws_newstrbuf();
extern void ws_strreset();
extern int ws_newscrmap();
extern ushort ws_scanchar();
extern int xlate_keymap();

static int chr_open(queue_t *qp, dev_t *devp, int oflag, int sflag,
    cred_t *crp);
static int chr_close(queue_t *qp, int flag, cred_t *crp);
static int chr_read_queue_put(queue_t *qp, mblk_t *mp);
static int chr_read_queue_serv(register queue_t *qp);
static int chr_write_queue_put(queue_t *qp, mblk_t *mp);
static int chr_stat_init(queue_t *qp, register charstat_t *cp);
static void chr_free_stat(register charstat_t *cp);

static void chr_proc_r_data(register queue_t *qp, register mblk_t *mp,
    register charstat_t *cp);
static void chr_proc_r_proto(register queue_t *qp, register mblk_t *mp,
    charstat_t *cp);
static void chr_proc_w_data(queue_t *qp, mblk_t *mp, charstat_t *cp);
static void chr_do_iocdata(queue_t *qp, register mblk_t *mp,
    register charstat_t *cp);
static void chr_do_ioctl(queue_t *qp, register mblk_t *mp,
    register charstat_t *cp);
static void chr_scan(register charstat_t *cp, unsigned char rawscan,
    int israw);
static void chr_do_mouseinfo(queue_t *qp, mblk_t *mp, charstat_t *cp);
static void chr_r_charin(charstat_t *cp, unchar *bufp, int cnt, int flush);
static void chr_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval);
static void chr_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp,
    int error, int rval);
static void chr_copyout(queue_t *qp, register mblk_t *mp, register mblk_t *nmp,
    uint size, unsigned long state);
static void chr_copyin(queue_t *qp, register mblk_t *mp, int size,
    unsigned long state);
static int chr_is_special(register keymap_t *kp, unchar idx, unchar table);
static ushort chr_getkbent(charstat_t *cp, unchar table, unchar idx);
static void chr_setkbent(charstat_t *cp, unchar table, unchar idx, ushort val);
static void chr_proc_w_proto(queue_t *qp, mblk_t *mp, charstat_t *cp);
static int get_keymap_type(register mblk_t *mp);
static mblk_t *put_keymap_type(const int type);

int chr_maxmouse;

int chr_debug = 0;

char	_depends_on[] = "drv/kd";

#define	DEBUG1(a)	if (chr_debug == 1) printf a
#define	DEBUG2(a)	if (chr_debug >= 2) printf a /* allocations */
#define	DEBUG3(a)	if (chr_debug >= 3) printf a /* M_CTL Stuff */
#define	DEBUG4(a)	if (chr_debug >= 4) printf a /* M_READ Stuff */
#define	DEBUG5(a)	if (chr_debug >= 5) printf a
#define	DEBUG6(a)	if (chr_debug >= 6) printf a

static struct module_info chr_iinfo = {
	0,
	"char",
	0,
	MAXCHARPSZ,
	1000,
	100
};

static struct qinit chr_rinit = {
	chr_read_queue_put,
	chr_read_queue_serv,
	chr_open,
	chr_close,
	NULL,
	&chr_iinfo
};

static struct module_info chr_oinfo = {
	0,
	"char",
	0,
	MAXCHARPSZ,
	1000,
	100
};

static struct qinit chr_winit = {
	chr_write_queue_put,
	NULL,
	chr_open,
	chr_close,
	NULL,
	&chr_oinfo
};

struct streamtab char_str_info = {
	&chr_rinit,
	&chr_winit,
	NULL,
	NULL
};

kmutex_t		char_lock;

/*
 * This is the loadable module wrapper.
 */
#include <sys/conf.h>
#include <sys/modctl.h>

/*
 * D_MTQPAIR effectively makes the module single threaded.
 * There can be only one thread active in the module at any time.
 * It may be a read or write thread.
 */
#define	CHAR_CONF_FLAG	(D_NEW | D_MTQPAIR | D_MP)

static struct fmodsw	fsw = {
	"char",
	&char_str_info,
	CHAR_CONF_FLAG
};

static struct modlstrmod modlstrmod = {
	&mod_strmodops,
	"KD scan code converter",
	&fsw
};

/*
 * Module linkage information for the kernel.
 */
static struct modlinkage modlinkage = {
	MODREV_1, &modlstrmod, NULL
};


int
_init(void)
{
	register int	rc;

	mutex_init(&char_lock, "char lock", MUTEX_DEFAULT, NULL);
	if ((rc = mod_install(&modlinkage)) != 0) {
		mutex_destroy(&char_lock);
	}
	return (rc);
}

int
_fini(void)
{
	register int rc;

	if ((rc = mod_remove(&modlinkage)) == 0)
		mutex_destroy(&char_lock);
	return (rc);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 * Initialize per-stream state structure for CHAR. Return 0 for
 * success, error number for failure.
 */

static int
chr_stat_init(queue_t *qp, register charstat_t *cp)
{
	/*LINTED: variable unused (really used for sizeof)*/
	struct mouseinfo *minfop; /* just used for sizing */

	chr_maxmouse = (1 << (8 * sizeof (minfop->xmotion) - 1)) - 1;
	mutex_enter(&char_lock); /* protect against multiple opens */

	/* if q_ptr already set, we've allocated a struct already */
	if (qp->q_ptr != (caddr_t) NULL) {
		mutex_exit(&char_lock);
		kmem_free((caddr_t) cp, sizeof (charstat_t));
		return (0); /* not failure -- just simultaneous open */
	}

	/* set ptrs in queues to point to state structure */
	qp->q_ptr = (caddr_t)cp;
	WR(qp)->q_ptr = (caddr_t)cp;
	mutex_exit(&char_lock);


	cp->c_rqp = qp;
	cp->c_wqp = WR(qp);
	cp->c_wmsg = (mblk_t *) NULL;
	cp->c_rmsg = (mblk_t *) NULL;
	cp->c_state = 0;
	cp->c_map_p = (charmap_t *) NULL;
	cp->c_scrmap_p = NULL;
	cp->c_oldbutton = 0x07;		/* Initially all buttons up */
	return (0);
}


/* release cp and associated allocated data structures */

static void
chr_free_stat(register charstat_t *cp)
{
	if (cp->c_rmsg != (mblk_t *) NULL) {
		freemsg(cp->c_rmsg);
		cp->c_rmsg = (mblk_t *) NULL;
	}

	if (cp->c_wmsg != (mblk_t *) NULL) {
		freemsg(cp->c_wmsg);
		cp->c_wmsg = (mblk_t *) NULL;
	}
	if (cp->c_heldmseread != (mblk_t *) NULL) {
		freemsg(cp->c_heldmseread);
		cp->c_heldmseread = (mblk_t *) NULL;
	}

	kmem_free(cp, sizeof (charstat_t));
}


/*
 * Character module open. Allocate state structure and
 * send pointer to pointer to character map structure
 * to principal stream below in the form of a M_PCPROTO
 * message. Return 0 for sucess, an errno for failure.
 */

/*ARGSUSED1*/
static int
chr_open(queue_t *qp, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	charstat_t	*cp;
	mblk_t		*mp;
	mblk_t		*nmp;
	struct iocblk	*iocp;
	ch_proto_t	*protop;
	int		error = 0;

	if (qp->q_ptr != NULL) {
		return (0);		/* already attached */
	}

	/* allocate and initialize state structure */
	cp = kmem_zalloc(sizeof (charstat_t), KM_SLEEP);

	cp->c_kbstat.kb_proc_state = B_TRUE;
	error = chr_stat_init(qp, cp); /* initialize state structure */
	qprocson(qp);
	if ((nmp = allocb(sizeof (ch_proto_t), BPRI_MED)) == NULL) {
		cmn_err(CE_WARN, "CHAR: open fails, can't get char map \n");
		return (error);
	}

	nmp->b_wptr += sizeof (ch_proto_t);
	nmp->b_datap->db_type = M_PCPROTO;

	protop = (ch_proto_t *)nmp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHAR_MODE;
	protop->chp_stype_cmd = cp->c_state;

	/*
	 * put it on the next write queue to ship to KD
	 */
	putnext(WR(qp), nmp);

	if ((mp = allocb(sizeof (struct iocblk), BPRI_MED)) == NULL) {
		cmn_err(CE_WARN, "CHAR: open fails, can't get char map \n");
		return (error);
	}
	mp->b_datap->db_type = M_IOCTL;
	mp->b_wptr += sizeof (struct iocblk);
	iocp = (struct iocblk *)mp->b_rptr;
	iocp->ioc_cmd = CH_CHRMAP;
	putnext(WR(qp), mp);
	return (error);
}


/* Release state structure associated with stream */

/*ARGSUSED1*/
static int
chr_close(queue_t *qp, int flag, cred_t *crp)
{
	charstat_t *cp = (charstat_t *)qp->q_ptr;

	qprocsoff(qp);
	flushq(qp, FLUSHDATA);
	mutex_enter(&char_lock);

	/* Dump the associated state structure */
	chr_free_stat(cp);
	qp->q_ptr = NULL;

	mutex_exit(&char_lock);
	return (0);
}

/*
 * Put procedure for input from driver end of stream (read queue).
 */

static int
chr_read_queue_put(queue_t *qp, mblk_t *mp)
{
	(void) putq(qp, mp); /* enqueue this pup */
	return (0);
}

/*
 * Read side queued message processing.
 */

static int
chr_read_queue_serv(register queue_t *qp)
{
	register charstat_t *cp;
	mblk_t *mp;

	cp = (charstat_t *)qp->q_ptr;

	/*
	 * keep getting messages until none left or we honor
	 * flow control and see that the stream above us is blocked
	 * or are set to enqueue messages while an ioctl is processed
	 */

	while ((mp = getq(qp)) != NULL) {
		if (mp->b_datap->db_type <= QPCTL && !canputnext(qp)) {
			(void) putbq(qp, mp);
			return (0);	/* read side is blocked */
		}

#ifdef DONT_INCLUDE
		if (cp->c_state & C_XQUEMDE) {
			if (!cp->c_xqinfo ||
			    !validproc(cp->c_xqinfo->xq_proc,
			    cp->c_xqinfo->xq_pid)) {
				cp->c_state &= ~C_XQUEMDE;
				cp->c_xqinfo = NULL;
			}
		}
#endif

#ifdef _VPIX
		if (cp->c_state & C_RAWMODE) {
			if (!validproc(ttoproc(cp->c_rawprocp),
			    cp->c_rawpid)) {
				cp->c_state &= ~C_RAWMODE;
				cp->c_rawprocp = NULL;
				cp->c_rawpid = 0;
			}
		}
#endif /* _VPIX */


		switch (mp->b_datap->db_type) {

		default:
			putnext(qp, mp);	/* pass it on */
			continue;

		case M_FLUSH:
			/*
			 * Flush everything we haven't looked at yet.
			 */
			flushq(qp, FLUSHDATA);
			putnext(qp, mp); /* pass it on */
			continue;

		case M_DATA:
			chr_proc_r_data(qp, mp, cp);
			continue;

		case M_PCPROTO:
		case M_PROTO:
			chr_proc_r_proto(qp, mp, cp);
			continue;

		} /* switch */
	} /* while */
	return (0);
}


/*
 * Currently, CHAR understands messages from KDSTR, MOUSE and
 * MERGE386.
 */

#ifdef DONT_INCLUDE
void chr_do_xmouse();
#endif

void chr_do_mouseinfo();

static void
chr_proc_r_proto(register queue_t *qp, register mblk_t *mp,
    charstat_t *cp)
{
	ch_proto_t *protop;
	int i;

	if ((int)(mp->b_wptr - mp->b_rptr) != sizeof (ch_proto_t)) {
		putnext(qp, mp);
		return;
	}

	protop = (ch_proto_t *)mp->b_rptr;

	switch (protop->chp_type) {

	case CH_CTL: {

		switch (protop->chp_stype) {

		case CH_CHR:
			switch (protop->chp_stype_cmd) {

			case CH_LEDSTATE:
				i = cp->c_kbstat.kb_state;
				cp->c_kbstat.kb_state = (i & NONTOGGLES) |
				    protop->chp_stype_arg;
				break;

			case CH_CHRMAP:
				cp->c_map_p =
				    (charmap_t *)protop->chp_stype_arg;
				break;

			case CH_SCRMAP:
				cp->c_scrmap_p =
				    (scrn_t *)protop->chp_stype_arg;
				break;
#if defined(__DOS_EMUL) && defined(__MERGE)
			case CH_SETMVPI: {
				chr_merge_t *mergep;

				mergep = (mp->b_cont) ?
				    (chr_merge_t *)mp->b_cont->b_rptr
				    : (chr_merge_t *)NULL;
				if (mergep == (chr_merge_t *) NULL ||
				    (int)(mp->b_cont->b_wptr - (unchar *)mergep)
				    != sizeof (chr_merge_t)) {
					cmn_err(CE_WARN,
					    "char: Found CH_SETMVPI with "
					    "invalid arg");
					break;
				}
				cp->c_merge_kbd_ppi = mergep->merge_kbd_ppi;
				cp->c_merge_mse_ppi = mergep->merge_mse_ppi;
				cp->c_merge_mcon = mergep->merge_mcon;
				break;
				}

			case CH_DELMVPI:
				cp->c_merge_kbd_ppi = NULL;
				cp->c_merge_mse_ppi = NULL;
				cp->c_merge_mcon = NULL;
				break;
#endif /* DOS_EMUL && MERGE */
			default:
				putnext(qp, mp);
				return;
			}

			freemsg(mp);
			return; /* case CH_CHR */

#ifdef DONT_INCLUDE
		case CH_XQ:
			if (protop->chp_stype_cmd == CH_XQENAB) {
				cp->c_xqinfo =
				    (xqInfo *)protop->chp_stype_arg;
				cp->c_state |= C_XQUEMDE;
				protop->chp_stype_cmd = CH_XQENAB_ACK;
				mp->b_datap->db_type = M_PCPROTO;
				qreply(qp, mp);
				return;
			}

			if (protop->chp_stype_cmd == CH_XQDISAB) {
				cp->c_xqinfo = (xqInfo *) NULL;
				cp->c_state &= ~C_XQUEMDE;
				protop->chp_stype_cmd = CH_XQDISAB_ACK;
				mp->b_datap->db_type = M_PCPROTO;
				qreply(qp, mp);
				return;
			}
			putnext(qp, mp);
			return;
#endif

		default:
			putnext(qp, mp);
		}
		break;
	}	/* end of case CH_CTL */

	case CH_DATA:
		switch (protop->chp_stype) {

		case CH_MSE:
#ifdef XXX
			if (cp->c_state & C_XQUEMDE)
				chr_do_xmouse(qp, mp, cp);
			else
#endif
#ifdef _VPIX
			if ((cp->c_state & C_RAWMODE) &&
			    cp->c_stashed_v86.v86i_t)
				v86sdeliver(&cp->c_stashed_v86, V86VI_MOUSE,
				    qp);
#endif /* _VPIX */
#if defined(__DOS_EMUL) && defined(__MERGE)
			if (cp->c_merge_mse_ppi)
				(*(cp->c_merge_mse_ppi))(
					(struct mse_event *)mp->b_cont->b_rptr,
					cp->c_merge_mcon);
#endif /* DOS_EMUL && MERGE */
			chr_do_mouseinfo(qp, mp, cp);
			freemsg(mp);
			return;

		case CH_NOSCAN: /* send up b_cont message to LDTERM directly */
			if (mp->b_cont)
				putnext(qp, mp->b_cont);
			freeb(mp);
			return;

		default:
#ifdef DEBUG
			if (chr_debug)
				cmn_err(CE_NOTE,
				"char: Found unknown CH_DATA message in input");
#endif
			putnext(qp, mp);
			break;
		}
		return;

	default:
		putnext(qp, mp);
		return;
	}
}


/* Treat each byte of the message as a scan code to be translated. */

static void
chr_proc_r_data(register queue_t *qp, register mblk_t *mp,
    register charstat_t *cp)
{

	register mblk_t *bp;
	int israw;

	if (cp->c_map_p == (charmap_t *) NULL) {
		freemsg(mp);
		return;
	}

	bp = mp;
	israw = cp->c_state & (C_RAWMODE | C_XQUEMDE);

	/*
	 * For each data block, take the buffered bytes and pass them
	 * to chr_scan; it will translate them and put them in a
	 * message that we send up when when we're through
	 * with this message
	 */

	while (bp) {
		while ((unsigned)bp->b_rptr < (unsigned)bp->b_wptr) {
			chr_scan(cp, *bp->b_rptr++, israw);
		}
		bp = bp->b_cont;
	}

	freemsg(mp); /* free the scanned message */

	/* send up the message we stored at c_rmsg */
	if (cp->c_rmsg != NULL) {
		putnext(qp, cp->c_rmsg);
		cp->c_rmsg = NULL;
	}
}


/*
 * Translate the rawscan code to its mapped counterpart,
 * using the provided WS function ws_scanchar().
 */

static void
chr_scan(register charstat_t *cp, unsigned char rawscan, int israw)
{
	charmap_t	*cmp;		/* char map pointer */
	kbstate_t	*kbstatp;	/* ptr to kboard state */
	ushort		ch;
	int		cnt;
	unchar		*strp, *sp;
	unchar		lbuf[3],	/* Buffer for escape sequences */
			str[4];		/* string buffer */

	pfxstate_t	*pfxstrp;
	strmap_t	*strbufp;
	stridx_t	*strmap_p;

	cmp = cp->c_map_p;
	kbstatp = &cp->c_kbstat;
	pfxstrp = cmp->cr_pfxstrp;
	strbufp = cmp->cr_strbufp;
	strmap_p = cmp->cr_strmap_p;

	/* translate the character */
	ch = ws_scanchar(cmp, kbstatp, rawscan, israw);

	if (ch & NO_CHAR)
		return;

	strp = &lbuf[0];

	if (!israw) {
		switch (ch) {
		case K_SLK:	/* CTRL-F generates K_SLK */
			if ((rawscan == SCROLLLOCK) || (rawscan == 0x45)) {
				ch = (cp->c_state & C_FLOWON) ? CSTART: CSTOP;
			}
			break;

		case K_BRK:
			if (rawscan == SCROLLLOCK ||
			    ws_specialkey(cmp->cr_keymap_p, kbstatp, rawscan)) {
				chr_r_charin(cp, NULL, 0, 1);	/* flush */
				putnextctl(cp->c_rqp, M_BREAK);
				return;
			}
			break;

		default:
			break;
		}
	}

	if (cp->c_state & C_XQUEMDE) {	/* just send the event */
		lbuf[0] = ch;
		chr_r_charin(cp, lbuf, 1, 0);
		return;
	}

	if (ch & GEN_ESCLSB) {		/* Prepend character with "<ESC>["? */
		lbuf[0] = 033;		/* Prepend <ESC> */
		lbuf[1] = '[';		/* Prepend '[' */
		lbuf[2] = ch;		/* Add character */
		cnt = 3;		/* Three characters in buffer */
	} else if (ch & GEN_ESCN) {	/* Prepend character with "<ESC>N"? */
		lbuf[0] = 033;		/* Prepend <ESC> */
		lbuf[1] = 'N';		/* Prepend 'N' */
		lbuf[2] = ch;		/* Add character */
		cnt = 3;		/* Three characters in buffer */
	} else if (ch & GEN_ZERO) {	/* Prepend character with 0? */
		lbuf[0] = 0;		/* Prepend 0 */
		lbuf[1] = ch;		/* Add character */
		cnt = 2;		/* Two characters in buffer */
	} else if (ch & GEN_FUNC) {	/* Function key? */
		if ((int)(ch & 0xff) >= (int)K_PFXF &&
		    (ushort)(ch & 0xff) <= (ushort)K_PFXL) {
			ushort val;
			struct pfxstate *pfxp;

			str[0] = '\033';
			pfxp = (struct pfxstate *)pfxstrp;
			pfxp += (ch & 0xff) - K_PFXF;
			val = pfxp->val;
			switch (pfxp->type) {
			case K_ESN:
				str[1] = 'N';
				break;
			case K_ESO:
				str[1] = 'O';
				break;
			case K_ESL:
				str[1] = '[';
				break;
			}
			str[2] = (unchar)val;
			strp = &str[0];
			cnt = 3;
		} else {
			/* Start of string */
			ushort idx, *ptr;

			ptr = (ushort *)strmap_p;

			idx = * (ptr + (ch&0xff) - K_FUNF);
			strp = ((unchar *)strbufp) + idx;

			/* Count characters in string */
			for (cnt = 0, sp = strp; *sp != '\0'; cnt++, sp++)
				;
		}
	} else {  /* Nothing special about character */
		lbuf[0] = ch;		/* Put character in buffer */
		cnt = 1;		/* Only one character */
	}


	chr_r_charin(cp, strp, cnt, 0);	/* Put characters in data message */
}


/*
 * Stuff the characters pointed to by bufp in the message allocated for
 * shipping upstream if normal operation. VP/ix and MERGE386 hooks will most
 * likely be handled here, too, to some extent, as well as the X-queue.
 */
static void
chr_r_charin(charstat_t *cp, unsigned char *bufp, int cnt, int flush)
{
	mblk_t *mp;
	int size;

	if (flush || (cp->c_rmsg == NULL) ||
	    (cp->c_rmsg->b_wptr >= (cp->c_rmsg->b_datap->db_lim - cnt))) {
		if (cp->c_rmsg)		/* send any pending data on its way */
			putnext(cp->c_rqp, cp->c_rmsg);
		cp->c_rmsg = NULL;

		/*
		 * allocate a new message which is at least large enough
		 * for the data at hand.
		 */
		size = max(CHARPSZ, cnt);
		if ((mp = allocb(size, BPRI_MED)) == NULL) {
			cmn_err(CE_WARN,
			    "char: chr_scan: cannot allocate %d-byte message, "
			    "dropping input data", size);

			return;
		}
		cp->c_rmsg = mp;		/* save the new message ptr */
	}

#ifdef DONT_INCLUDE
	/*
	 * If we're in queue mode, just send the event.
	 */
	if (cp->c_state & C_XQUEMDE) {
		cp->c_xevent.xq_type = XQ_KEY;
		while (cnt-- != 0) {
			cp->c_xevent.xq_code = *bufp++;
			if (!(*cp->c_xqinfo->xq_addevent)
			    (cp->c_xqinfo, &cp->c_xevent)) {
#ifdef MERGE386
				if (cp->c_merge_kbd_ppi)
					(*(cp->c_merge_kbd_ppi))
					    (cp->c_xevent.xq_code,
					    cp->c_merge_mcon);
				else
#endif /* MERGE386 */
					*cp->c_rmsg->b_wptr++ =
					    cp->c_xevent.xq_code;
			}
		}
		return;
	}
	else
#endif /* DONT_INCLUDE */
#ifdef _VPIX
		if (cnt && (cp->c_state & C_RAWMODE) &&
				cp->c_stashed_v86.v86i_t)
			v86sdeliver(&cp->c_stashed_v86, V86VI_KBD, cp->c_rqp);
#endif /* _VPIX */
#if defined(__DOS_EMUL) && defined(__MERGE)
	if (cp->c_merge_kbd_ppi) {
		while (cnt-- != 0)
			(*(cp->c_merge_kbd_ppi))(*bufp++, cp->c_merge_mcon);
		return;
	}
#endif /* DOS_EMUL && MERGE */

	/*
	 * Add the characters to the message block.
	 */
	while (cnt-- != 0)
		*cp->c_rmsg->b_wptr++ = *bufp++;
}


/*
 * Char module output queue put procedure.
 */
static int
chr_write_queue_put(queue_t *qp, mblk_t *mp)
{
	register charstat_t *cp;

	cp = (charstat_t *)qp->q_ptr;

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		/*
		 * This is coming from above, so we only handle the write
		 * queue here.  If FLUSHR is set, it will get turned around
		 * at the driver, and the read procedure will see it
		 * eventually.
		 */
		if (*mp->b_rptr & FLUSHW)
			flushq(qp, FLUSHDATA);
		putnext(qp, mp);
		break;

	case M_IOCTL:
		chr_do_ioctl(qp, mp, cp);
		break;

	case M_IOCDATA:
		chr_do_iocdata(qp, mp, cp);
		break;

	case M_DATA:
		chr_proc_w_data(qp, mp, cp);
		break;

	case M_PCPROTO:
	case M_PROTO:
		chr_proc_w_proto(qp, mp, cp);
		break;

	default:
		putnext(qp, mp);	/* pass it through unmolested */
		break;
	}
	return (0);
}


static void
chr_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mblk_t	*tmp;

	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	if ((tmp = unlinkb(mp)) != (mblk_t *)NULL)
		freemsg(tmp);
	qreply(qp, mp);
}

static void
chr_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error, int rval)
{
	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp, mp);
}

static void
chr_copyout(queue_t *qp, register mblk_t *mp, register mblk_t *nmp,
    uint size, unsigned long state)
{
	register struct copyreq *cqp;
	copy_state_t *copyp;
	charstat_t *cp;

	cp = (charstat_t *)qp->q_ptr;
	copyp = &(cp->c_copystate);

	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	cqp->cq_addr = (caddr_t)*(long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	copyp->cpy_arg = (unsigned long) cqp->cq_addr;
	copyp->cpy_state = state;

	mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;

	if (mp->b_cont)
		freemsg(mp->b_cont);
	mp->b_cont = nmp;

	qreply(qp, mp);
}


/*
 *	Send an M_COPYIN request to fetch the user data for an ioctl command.
 */
static void
chr_copyin(queue_t *qp, register mblk_t *mp, int size, unsigned long state)
{
	register struct copyreq *cqp;
	copy_state_t *copyp;
	charstat_t *cp;

	cp = (charstat_t *)qp->q_ptr;
	copyp = &cp->c_copystate;

	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	cqp->cq_addr = (caddr_t)*(long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	copyp->cpy_arg = (unsigned long) cqp->cq_addr;
	copyp->cpy_state = state;

	if (mp->b_cont) {		/* this should always be true! */
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
	}

	mp->b_datap->db_type = M_COPYIN;
	mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);

	qreply(qp, mp);
}


/*
 * Called when an M_IOCTL message is seen on the write queue; does whatever
 * we're supposed to do with it, and either replies immediately or passes it
 * to the next module down.
 */

static void
chr_do_ioctl(queue_t *qp, register mblk_t *mp, register charstat_t *cp)
{
	register struct iocblk *iocp;
	int transparent;
	mblk_t *nmp;
	ch_proto_t *protop;


	iocp = (struct iocblk *)mp->b_rptr;

	transparent = (iocp->ioc_count == TRANSPARENT);

	switch (iocp->ioc_cmd) {

	case MOUSEIOCDELAY: {
		cp->c_state |= C_MSEBLK;
		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case MOUSEIOCNDELAY: {
		cp->c_state &= ~C_MSEBLK;
		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case MOUSEIOCREAD: {
		mblk_t *bp;
		struct mouseinfo *minfop;

		if ((!(cp->c_state & C_MSEINPUT)) &&
		    (cp->c_state & C_MSEBLK)) {
			cp->c_heldmseread = mp;
			return;
		}
		if ((bp = allocb(sizeof (struct mouseinfo), BPRI_MED))
		    == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}
		minfop = &cp->c_mouseinfo;
		bcopy((caddr_t)minfop, (caddr_t)bp->b_rptr,
		    sizeof (struct mouseinfo));
		minfop->xmotion = minfop->ymotion = 0;
		minfop->status &= BUTSTATMASK;
		cp->c_state &= ~C_MSEINPUT;
		bp->b_wptr += sizeof (struct mouseinfo);
		if (transparent)
			chr_copyout(qp, mp, bp, sizeof (struct mouseinfo), 0);
		else {
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = sizeof (struct mouseinfo);
			if (mp->b_cont) freemsg(mp->b_cont);
			mp->b_cont = bp;
			qreply(qp, mp);
		}
		break;
	}

	case TIOCSTI: { /* Simulate typing of a character at the terminal. */
		register mblk_t *bp;

		/*
		 * The permission checking has already been done at the stream
		 * head, since it has to be done in the context of the process
		 * doing the call. Special processing was done at STREAM head.
		 */

		if ((bp = allocb(1, BPRI_MED)) != NULL) {
			if ((nmp = allocb(sizeof (ch_proto_t), BPRI_MED))
			    == NULL) {
				freemsg(bp);
				bp = NULL;
			} else {
				*bp->b_wptr++ = *mp->b_cont->b_rptr++;
				nmp->b_datap->db_type = M_PROTO;
				protop = (ch_proto_t *)nmp->b_rptr;
				nmp->b_wptr += sizeof (ch_proto_t);
				protop->chp_type = CH_DATA;
				protop->chp_stype = CH_NOSCAN;
				nmp->b_cont = bp;
				(void) putq(cp->c_rqp, nmp);
			}
		}
		if (bp)
			chr_iocack(qp, mp, iocp, 0);
		else
			chr_iocnack(qp, mp, iocp, ENOMEM, 0);

		break;
	}

	case KBENABLED: {
		kbstate_t *kbp;

		if (transparent) {
			kbp = &cp->c_kbstat;
			chr_iocack(qp, mp, iocp, kbp->kb_extkey);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;
	}

	case TIOCKBOF: {
		kbstate_t *kbp;

		if (transparent) {
			kbp = &cp->c_kbstat;
			kbp->kb_extkey = 0;
			chr_iocack(qp, mp, iocp, 0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;
	}

	case TIOCKBON: {
		kbstate_t *kbp;

		if (transparent) {
			kbp = &cp->c_kbstat;
			kbp->kb_extkey = 1;
			chr_iocack(qp, mp, iocp, 0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;
	}

	case KDGKBENT:
		if (transparent)
			chr_copyin(qp, mp, sizeof (struct kbentry), CHR_IN_0);
		else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;

	case KDSKBENT:
		if (transparent) {
			chr_copyin(qp, mp, sizeof (struct kbentry), CHR_IN_0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;

	case KDGKBMODE: {
		unsigned char val;
		mblk_t *nmp;

		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}


		if ((nmp = allocb(sizeof (val), BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		val = (cp->c_state & C_RAWMODE) ? K_RAW : K_XLATE;
		bcopy((caddr_t)&val, (caddr_t)nmp->b_rptr, sizeof (val));
		nmp->b_wptr += sizeof (val);

		chr_copyout(qp, mp, nmp, sizeof (val), CHR_OUT_0);
		break;
		}

	case KDSKBMODE: {
		int arg;
#ifdef _VPIX
		struct v86blk *v86p;
#endif
		kbstate_t *kbp;

		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

#ifdef _VPIX
		/*
		 * The VP/ix version of streamio adds a "v86blk" structure,
		 * allowing the streams driver to access the user process
		 * context.
		 */
		arg = *(int *)mp->b_cont->b_cont->b_rptr;
		v86p = (struct v86blk *)mp->b_cont->b_rptr;
#else
		arg = *(int *)mp->b_cont->b_rptr;
#endif

		if ((arg != K_RAW) && (arg != K_XLATE)) {
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			break;
		}

		kbp = &cp->c_kbstat;
		if (arg == K_RAW) {	/* switch to raw (scancode) mode */
			cp->c_state |= C_RAWMODE;
			kbp->kb_proc_state = B_FALSE;
			kbp->kb_rstate = kbp->kb_state;
#ifdef _VPIX
			v86stash(&cp->c_stashed_v86, v86p);
			cp->c_rawprocp = v86p->v86b_t;
			cp->c_rawpid = v86p->v86b_p_pid;
#endif
		} else {
			cp->c_state &= ~C_RAWMODE;
			kbp->kb_proc_state = B_TRUE;
			kbp->kb_state = kbp->kb_rstate;
#ifdef _VPIX
			v86unstash(&cp->c_stashed_v86);
			cp->c_rawprocp = 0;
			cp->c_rawpid = 0;
#endif
		}

		if ((nmp = allocb(sizeof (ch_proto_t), BPRI_HI)) != NULL) {
			ch_proto_t	*protop;

			nmp->b_wptr += sizeof (ch_proto_t);
			nmp->b_datap->db_type = M_PCPROTO;

			protop = (ch_proto_t *)nmp->b_rptr;
			protop->chp_type = CH_CTL;
			protop->chp_stype = CH_CHAR_MODE;
			protop->chp_stype_cmd = cp->c_state;

			/*
			 * put it on the next write queue to ship to KD
			 */
			putnext(qp, nmp);
		}
		chr_iocack(qp, mp, iocp, 0);
		break;
	}


	case SETFKEY:	/* Assign a given string to a function key */
		if (transparent) {
			chr_copyin(qp, mp, sizeof (struct fkeyarg), CHR_IN_0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;

	case GETFKEY:	/* Get the string assigned to a function key */
		if (transparent)
			chr_copyin(qp, mp, sizeof (struct fkeyarg), CHR_IN_0);
		else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;

	case KDDFLTKEYMAP: {
		int	type = get_keymap_type(mp);
		charmap_t *cmp;

#ifdef DEBUG
		if (chr_debug)
			cmn_err(CE_NOTE, "ioctl KDDFLTKEYMAP type %d", type);
#endif


		if (!transparent || type < 0)
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		else {
			cmp = cp->c_map_p;
			cmp->cr_flags = type;
			chr_copyin(qp, mp, sizeof (struct key_dflt), CHR_IN_0);
		}
	}
		break;

	case KDDFLTSTRMAP:
		if (transparent) {
			chr_copyin(qp, mp, sizeof (struct str_dflt), CHR_IN_0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;

	case KDDFLTSCRNMAP:
		if (transparent) {
			chr_copyin(qp, mp, sizeof (struct scrn_dflt), CHR_IN_0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;


	case GIO_KEYMAP: {
		int size;
		int type;
		charmap_t *cmp = cp->c_map_p;
		keymap_t *kmp;
		mblk_t *nmp, *fmp;

		type = cmp->cr_flags;
#ifdef DEBUG
		if (chr_debug)
			cmn_err(CE_NOTE, "GIO_KEYMAP type %d", type);
#endif

		if (!transparent || type < 0) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		size = cmp->cr_keymap_p->n_keys;
		size *= sizeof (cmp->cr_keymap_p->key[0]);
		size += sizeof (cmp->cr_keymap_p->n_keys);

		if ((nmp = allocb(size, BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		kmp = (keymap_t *)nmp->b_rptr;
		if (type == SCO_FORMAT)
			xlate_keymap(cmp->cr_keymap_p, kmp, type);
		else
			bcopy((caddr_t)cmp->cr_keymap_p, (caddr_t)kmp, size);
		nmp->b_wptr += size;

		if ((fmp = put_keymap_type(type)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}
		fmp->b_cont = nmp;

		chr_copyout(qp, mp, fmp, size, CHR_OUT_0);
		break;
	}



	case PIO_KEYMAP: {
		int size, error;
		/*LINTED: variable unused (really used for sizeof)*/
		charmap_t *cmp;

		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		error = drv_priv(iocp->ioc_cr);
		if (error) {
			chr_iocnack(qp, mp, iocp, error, 0);
			break;
		}

		size = sizeof (struct keymap_flags) +
		    sizeof (cmp->cr_keymap_p->n_keys);
		chr_copyin(qp, mp, size, CHR_IN_0);
		break;
		}

	case PIO_SCRNMAP:

		if (transparent) {
			chr_copyin(qp, mp, sizeof (scrnmap_t), CHR_IN_0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;

	case GIO_SCRNMAP: {
		scrnmap_t *scrp;
		unsigned int i = 0;

		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}


		if ((nmp = allocb(sizeof (scrnmap_t), BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		scrp = NULL;
		if (cp->c_scrmap_p) {
			if ((scrp = cp->c_scrmap_p->scr_map_p)
			    == (scrnmap_t *)NULL)
				scrp = cp->c_scrmap_p->scr_defltp->scr_map_p;
		}
		if (scrp) {
			bcopy((caddr_t)scrp, (caddr_t)nmp->b_rptr,
			    sizeof (scrnmap_t));
			nmp->b_wptr += sizeof (scrnmap_t);
		} else {
			for (i = 0; i < sizeof (scrnmap_t); i++)
				*nmp->b_wptr++ = (unsigned char) i;
		}
		chr_copyout(qp, mp, nmp, sizeof (scrnmap_t), CHR_OUT_0);
		break;
	}

	case PIO_STRMAP:

		if (transparent) {
			chr_copyin(qp, mp, STRTABLN, CHR_IN_0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;

	case PIO_STRMAP_21:

		if (transparent) {
			chr_copyin(qp, mp, STRTABLN_21, CHR_IN_0);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);

		break;

	case GIO_STRMAP:
		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}


		if ((nmp = allocb(STRTABLN, BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		bcopy((caddr_t)cp->c_map_p->cr_strbufp, (caddr_t)nmp->b_rptr,
		    STRTABLN);
		nmp->b_wptr += STRTABLN;

		chr_copyout(qp, mp, nmp, STRTABLN, CHR_OUT_0);
		break;

	case GIO_STRMAP_21:
		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		if ((nmp = allocb(STRTABLN_21, BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		bcopy((caddr_t)cp->c_map_p->cr_strbufp, (caddr_t)nmp->b_rptr,
		    STRTABLN_21);
		nmp->b_wptr += STRTABLN_21;

		chr_copyout(qp, mp, nmp, STRTABLN_21, CHR_OUT_0);
		break;

	case SETLOCKLOCK:
		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		if (mp->b_cont && (*(int *)mp->b_cont->b_rptr == 0)) {
			chr_iocack(qp, mp, iocp, 0);
			break;
		}

		chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case KDGKBSTATE: {
		kbstate_t *kbp;
		unchar state = 0;
		mblk_t *nmp;
		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		if ((nmp = allocb(sizeof (state), BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		kbp = &cp->c_kbstat;
		if (kbp->kb_state & SHIFTSET)
			state |= SHIFTED;
		if (kbp->kb_state & CTRLSET)
			state |= CTRLED;
		if (kbp->kb_state & ALTSET)
			state |= ALTED;

		bcopy((caddr_t)&state, (caddr_t)nmp->b_rptr, sizeof (state));
		nmp->b_wptr += sizeof (state);

		chr_copyout(qp, mp, nmp, sizeof (state), CHR_OUT_0);
		break;
		}

	default:
		putnext(qp, mp);
		break;
	}
}

static int
chr_is_special(register keymap_t *kp, unchar idx, unchar table)
{
	return (kp->key[idx].spcl & (0x80 >> table));
}


/* KDGKBENT ioctl translation function for character mapping tables */

static ushort
chr_getkbent(charstat_t *cp, unchar table, unchar idx)
{
	register keymap_t *kp;
	ushort	val, pfx;
	struct pfxstate *pfxp;

	kp = cp->c_map_p->cr_keymap_p;
	pfxp = (struct pfxstate *)cp->c_map_p->cr_pfxstrp;

	val = kp->key[idx].map[table];
	if (kp->key[idx].flgs & KMF_NLOCK)
		val |= NUMLCK;
	if (kp->key[idx].flgs & KMF_CLOCK)
		val |= CAPLCK;
	if (chr_is_special(kp, idx, table)) {
		if (IS_FUNKEY(val & 0xff) &&
		    pfxp[(val & 0xff) - K_PFXF].type != (unchar) 0) {
			pfx = pfxp[(val & 0xff) - K_PFXF].type;
			val = pfxp[(val & 0xff) - K_PFXF].val;
			switch (pfx) {
			case K_ESN:
				pfx = SS2PFX;
				break;
			case K_ESO:
				pfx = SS3PFX;
				break;
			case K_ESL:
				pfx = CSIPFX;
				break;
			}
			return (pfx| (unchar)val);
		}
		switch (val & 0xff) {
		case K_NOP:
			return (NOKEY);
		case K_BRK:
			return (BREAKKEY);
		case K_LSH:
		case K_RSH:
		case K_CLK:
		case K_NLK:
		case K_ALT:
		case K_CTL:
		case K_LAL:
		case K_RAL:
		case K_LCT:
		case K_RCT:
			return (val | SHIFTKEY);
		case K_ESN:
			return (kp->key[idx].map[table ^ ALTED] |
					SS2PFX | (val & 0xff00));
		case K_ESO:
			return (kp->key[idx].map[table ^ ALTED] |
					SS3PFX | (val & 0xff00));
		case K_ESL:
			return (kp->key[idx].map[table ^ ALTED] |
					CSIPFX | (val & 0xff00));
		case K_BTAB:
			return ('Z' | CSIPFX);
		default: {
			return (val | SPECIALKEY);
		}
		}
	}
	if (kp->key[idx].map[table | CTRLED] & 0x1f)
		val |= CTLKEY;
	return (val);
}


/* KDSKBENT ioctl translation function for character mapping tables */
static void
chr_setkbent(charstat_t *cp, unchar table, unchar idx, ushort val)
{
	int special = 0, smask, pfx;
	register struct pfxstate *pfxp;
	register keymap_t *kp;

	kp = cp->c_map_p->cr_keymap_p;
	pfxp = (struct pfxstate *)cp->c_map_p->cr_pfxstrp;

#ifdef DEBUG1
	if (chr_debug && idx >= NUM_KEYS)
		cmn_err(CE_PANIC, "char_setkbent -- idx was bad");
#endif
	if ((val & TYPEMASK) == SHIFTKEY)
		return;
	if ((val & TYPEMASK) != NORMKEY)
		val &= ~CTLKEY;
	if (chr_is_special(kp, idx, table)) {
		int old_val = kp->key[idx].map[table];

		if (IS_FUNKEY(old_val) && (old_val > K_PFXF) &&
		    pfxp[old_val - K_PFXF].type != 0) {
			pfxp[old_val - K_PFXF].val = 0;
			pfxp[old_val - K_PFXF].type = 0;
		}
	}
	kp->key[idx].flgs = 0;
	if (val & NUMLCK)
		kp->key[idx].flgs |= KMF_NLOCK;
	if (val & CAPLCK)
		kp->key[idx].flgs |= KMF_CLOCK;
	smask = (0x80 >> table) + (0x80 >> (table | CTRLED));
	switch (val & TYPEMASK) {
	case BREAKKEY:
		special = smask;
		val = K_BRK;
		break;
	case NORMKEY:
		break;
	case SPECIALKEY:
		special = smask;
		break;
	default:
		special = smask;
		val = K_NOP;
		break;
	case SS2PFX:
		pfx = K_ESN;
		goto prefix;
	case SS3PFX:
		pfx = K_ESO;
		goto prefix;
	case CSIPFX:
		if ((val & 0xff) == 'Z') {
			special = smask;
			val = K_BTAB;
			break;
		}
		pfx = K_ESL;
prefix:
		special = smask;
		if ((val & 0xff) == kp->key[idx].map[table ^ ALTED])
			val = (ushort)pfx;
		else {
			int	keynum;

			for (keynum = 0; keynum < (K_PFXL - K_PFXF); keynum++) {
				if (pfxp[keynum].type == 0)
					break;
			}
			if (keynum < K_PFXL - K_PFXF) {
				pfxp[keynum].val = val & 0xff;
				pfxp[keynum].type = (unchar)pfx;
				val = K_PFXF + keynum;
			} else
				val = K_NOP;
		}
		break;
	}
	kp->key[idx].map[table] = (unchar)val;
	kp->key[idx].map[table | CTRLED] = (val & CTLKEY) ? (val & 0x1f) : val;
	kp->key[idx].spcl = (kp->key[idx].spcl & ~smask) | special;
}


extern int	ws_addstring(charmap_t *, ushort, unchar *, ushort);

static int
get_keymap_type(register mblk_t *mp)
{
	register mblk_t *cont = mp->b_cont;
	struct keymap_flags *kp = (struct keymap_flags *)cont->b_rptr;

	/* XXX should remove this, as it breaks compatibility! XXX */
	if (kp->km_magic != KEYMAP_MAGIC) {
		cmn_err(CE_NOTE, "ERROR in KEYMAP message");
		return (-1);
	}
	if (kp->km_type != SCO_FORMAT && kp->km_type != USL_FORMAT)
		return (-1);

	return (kp->km_type);
}

static mblk_t *
put_keymap_type(const int type)
{
	register mblk_t *fmp;
	register struct keymap_flags *kp;

	if ((fmp = allocb(sizeof (struct keymap_flags), BPRI_MED)) == NULL)
		return (NULL);

	kp = (struct keymap_flags *)fmp->b_rptr;
	kp->km_type = type;
	kp->km_magic = KEYMAP_MAGIC;
	fmp->b_wptr += sizeof (struct keymap_flags);

	return (fmp);
}

/*
 *	Called when the M_IOCDATA message has arrived, containing the user
 *	data requested for performing an ioctl command.
 */
static void
chr_do_iocdata(queue_t *qp, register mblk_t *mp,
    register charstat_t *cp)
{
	register struct iocblk *iocp;
	struct copyresp *csp;
	struct copyreq *cqp;
	copy_state_t *copyp;
	int error;

	iocp = (struct iocblk *)mp->b_rptr;

	csp = (struct copyresp *)mp->b_rptr;
	copyp = (copy_state_t *)csp->cp_private;


	switch (iocp->ioc_cmd) {

	default:
		putnext(qp, mp); /* not for us */
		break;

	case GIO_SCRNMAP: /* and other M_COPYOUT ioctl types */
	case GIO_KEYMAP:
	case GIO_STRMAP:
	case GIO_STRMAP_21:
	case KDGKBMODE:
	case KDGKBSTATE:
	case MOUSEIOCREAD:
		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		chr_iocack(qp, mp, iocp, 0);
		break;

	case GETFKEY: {			/* requests string for function key */

		struct fkeyarg *fp;
		unchar *charp;
		ushort *idxp;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			/*
			 * This message is the response to our previous
			 * M_COPYOUT (as indicated by ->cpy_state), so we
			 * simply need to send a normal M_IOCACK response
			 * to the original ioctl, so it will complete.
			*/
			chr_iocack(qp, mp, iocp, 0);
			return;
		}

		/* must be response to copyin */
		if (pullupmsg(mp->b_cont, sizeof (struct fkeyarg)) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of GETFKEY ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		fp = (struct fkeyarg *)mp->b_cont->b_rptr;
		if ((int)fp->keynum < 1 || (int)fp->keynum > NSTRKEYS) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}

		idxp = (ushort *)cp->c_map_p->cr_strmap_p;
		idxp += fp->keynum - 1;
		fp->flen = 0;
		charp = (unchar *)cp->c_map_p->cr_strbufp + *idxp;

		/* copy key string definition */
		while (fp->flen < MAXFK && *charp != '\0')
			fp->keydef[fp->flen++] = *charp++;

		/* now copyout fkeyarg back up to user */
		cqp = (struct copyreq *)mp->b_rptr;
		cqp->cq_size = sizeof (struct fkeyarg);
		cqp->cq_addr = (caddr_t)copyp->cpy_arg;
		cqp->cq_flag = 0;
		cqp->cq_private = (mblk_t *)copyp;
		copyp->cpy_state = CHR_OUT_0;
		mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
		mp->b_datap->db_type = M_COPYOUT;
		qreply(qp, mp);
		return;
	}

	case SETFKEY: {
		struct fkeyarg *fp;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (pullupmsg(mp->b_cont, sizeof (struct fkeyarg)) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of SETFKEY ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		fp = (struct fkeyarg *)mp->b_cont->b_rptr;
		if ((int)fp->keynum < 1 || (int)fp->keynum > NSTRKEYS) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}

		/* ws_addstring assumes 0..NSTRKEYS-1 range */
		fp->keynum -= 1;

		if (!ws_addstring(cp->c_map_p, fp->keynum, fp->keydef,
		    fp->flen))
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		else
			chr_iocack(qp, mp, iocp, 0);

		return;
	}

	case PIO_STRMAP_21:
	case PIO_STRMAP: {
		unsigned long size;

		size = (iocp->ioc_cmd == PIO_STRMAP) ? STRTABLN : STRTABLN_21;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (pullupmsg(mp->b_cont, size) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of PIO_STRMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		if (!ws_newstrbuf(cp->c_map_p, KM_NOSLEEP)) {
			chr_iocnack(qp, mp, iocp, ENOMEM, 0);
			return;
		}

		bcopy((caddr_t)mp->b_cont->b_rptr,
			(caddr_t)cp->c_map_p->cr_strbufp,
			size);
		(void) ws_strreset(cp->c_map_p);
		chr_iocack(qp, mp, iocp, 0);
		break;
	}


	case PIO_SCRNMAP: {
		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (pullupmsg(mp->b_cont, sizeof (scrnmap_t)) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of PIO_SCRNMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		if (!ws_newscrmap(cp->c_scrmap_p, KM_NOSLEEP)) {
			chr_iocnack(qp, mp, iocp, ENOMEM, 0);
			return;
		}

		bcopy((caddr_t)mp->b_cont->b_rptr,
		    (caddr_t)cp->c_scrmap_p->scr_map_p, sizeof (scrnmap_t));
		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case PIO_KEYMAP: {
		short numkeys;
		uint size;
		charmap_t *cmp;

		cmp = cp->c_map_p;

		if (csp->cp_rval) {
			freemsg(mp);
			cmp->cr_flags = USL_FORMAT;
			return;
		}

		if (copyp->cpy_state == CHR_IN_0) {
			int type;

			if ((type = get_keymap_type(mp)) < 0) {
				chr_iocnack(qp, mp, iocp, EINVAL, 0);
				return;
			}
#ifdef DEBUG
			if (chr_debug)
				cmn_err(CE_NOTE, "PIO_KEYMAP type %d", type);
#endif
			cmp->cr_flags = type;

			numkeys = *(short *)(mp->b_cont->b_rptr +
			    sizeof (struct keymap_flags));
			numkeys = min(numkeys, NUM_KEYS);
			freemsg(mp->b_cont);
			mp->b_cont = NULL;

			size = numkeys * sizeof (cmp->cr_keymap_p->key[0]);
			size += sizeof (cmp->cr_keymap_p->n_keys);

			cqp = (struct copyreq *)mp->b_rptr;
			cqp->cq_size = size;
			cqp->cq_addr = (caddr_t)copyp->cpy_arg +
			    sizeof (struct keymap_flags);
			cqp->cq_flag = 0;
			cqp->cq_private = (mblk_t *)copyp;
			copyp->cpy_arg = numkeys;
			copyp->cpy_state = CHR_IN_1;
			mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
			mp->b_datap->db_type = M_COPYIN;
			qreply(qp, mp);
			return;
		}

		numkeys = copyp->cpy_arg;
		size = numkeys*sizeof (cmp->cr_keymap_p->key[0]) +
		    sizeof (numkeys);
		if (pullupmsg(mp->b_cont, size) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of PIO_KEYMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			cmp->cr_flags = USL_FORMAT;
			return;
		}
#ifdef DEBUG
		if (chr_debug)
			cmn_err(CE_NOTE, "INSTALLING new map of type %s",
			    cmp->cr_flags == SCO_FORMAT ? "SCO FORMAT" :
					"USL FORMAT");
#endif
		cmp = cp->c_map_p;

		if (!ws_newkeymap(cmp, numkeys, (keymap_t *)mp->b_cont->b_rptr,
		    KM_NOSLEEP)) {
			cmn_err(CE_WARN,
			    "char: PIO_KEYMAP: could not allocate new keymap");
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			cmp->cr_flags = USL_FORMAT;
			return;
		}

		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case KDDFLTSTRMAP: {
		charmap_t *cmp;
		struct str_dflt *kp;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp, 0);
			return;
		}

		cmp = cp->c_map_p;

		if (pullupmsg(mp->b_cont, sizeof (struct str_dflt)) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of KDDFLTSTRMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		kp = (struct str_dflt *)mp->b_cont->b_rptr;
		if (kp->str_direction == KD_DFLTGET) {
			cqp = (struct copyreq *)mp->b_rptr;
			cqp->cq_size = sizeof (struct str_dflt);
			cqp->cq_addr = (caddr_t)cp->c_copystate.cpy_arg;
			cqp->cq_flag = 0;
			cqp->cq_private = (mblk_t *)copyp;
			copyp->cpy_state = CHR_OUT_0;
			mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
			mp->b_datap->db_type = M_COPYOUT;
			bcopy((caddr_t)cmp->cr_defltp->cr_strbufp,
			    (caddr_t)&kp->str_map, sizeof (strmap_t));
			qreply(qp, mp);
			return;
		}
		if (kp->str_direction != KD_DFLTSET) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}
		error = drv_priv(iocp->ioc_cr);
		if (error) {
			chr_iocnack(qp, mp, iocp, error, 0);
			break;
		}

		bcopy((caddr_t)&kp->str_map,
		    (caddr_t)cmp->cr_defltp->cr_strbufp, sizeof (strmap_t));
		ws_strreset(cmp->cr_defltp);
		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case KDDFLTKEYMAP: {
		charmap_t *cmp;
		struct key_dflt *kp;
		int	type;

		cmp = cp->c_map_p;
		type = cmp->cr_flags;
		cmp->cr_flags = USL_FORMAT;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp, 0);
			return;
		}

		if (pullupmsg(mp->b_cont, sizeof (struct key_dflt)) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of KDDFLTKEYMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		kp = (struct key_dflt *)mp->b_cont->b_rptr;
		if (kp->key_direction == KD_DFLTGET) {
			cqp = (struct copyreq *)mp->b_rptr;
			cqp->cq_size = sizeof (struct key_dflt);
			cqp->cq_addr = (caddr_t)cp->c_copystate.cpy_arg;
			cqp->cq_flag = 0;
			cqp->cq_private = (mblk_t *)copyp;
			copyp->cpy_state = CHR_OUT_0;
			mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
			mp->b_datap->db_type = M_COPYOUT;
#ifdef DEBUG
			if (chr_debug)
				cmn_err(CE_NOTE,
					"DFFLTKEYMAP get map of type %s FORMAT",
					type == SCO_FORMAT ? "SCO" : "USL");
#endif
			if (type == SCO_FORMAT)
				xlate_keymap(cmp->cr_defltp->cr_keymap_p,
							&kp->key_map, type);
			else
				bcopy((caddr_t)cmp->cr_defltp->cr_keymap_p,
				    (caddr_t)&kp->key_map, sizeof (keymap_t));
			qreply(qp, mp);
			return;
		}
		if (kp->key_direction != KD_DFLTSET) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}
		error = drv_priv(iocp->ioc_cr);
		if (error) {
			chr_iocnack(qp, mp, iocp, error, 0);
			break;
		}

#ifdef DEBUG
		if (chr_debug)
			cmn_err(CE_NOTE,
				"DFFLTKEYMAP set map of type %s FORMAT",
				type == SCO_FORMAT ? "SCO" : "USL");
#endif
		if (type == SCO_FORMAT)
			xlate_keymap(&kp->key_map,
				cmp->cr_defltp->cr_keymap_p, type);
		else
			bcopy((caddr_t)&kp->key_map,
			    (caddr_t)cmp->cr_defltp->cr_keymap_p,
			    sizeof (keymap_t));
		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case KDDFLTSCRNMAP: {
		scrn_t *scrp;
		struct scrn_dflt *kp;
		int i;
		char *c;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp, 0);
			return;
		}

		if ((scrp = cp->c_scrmap_p) == NULL) {
			chr_iocnack(qp, mp, iocp, EACCES, 0);
			return;
		}

		if (pullupmsg(mp->b_cont, sizeof (struct scrn_dflt)) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of KDDFLTSCRNMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		kp = (struct scrn_dflt *)mp->b_cont->b_rptr;
		if (kp->scrn_direction == KD_DFLTGET) {
			cqp = (struct copyreq *)mp->b_rptr;
			cqp->cq_size = sizeof (struct scrn_dflt);
			cqp->cq_addr = (caddr_t)cp->c_copystate.cpy_arg;
			cqp->cq_flag = 0;
			cqp->cq_private = (mblk_t *)copyp;
			copyp->cpy_state = CHR_OUT_0;
			mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
			mp->b_datap->db_type = M_COPYOUT;
			if (scrp->scr_defltp->scr_map_p) {
				bcopy((caddr_t)scrp->scr_defltp->scr_map_p,
				    (caddr_t)&kp->scrn_map, sizeof (scrnmap_t));
			} else {
				c = (char *)&kp->scrn_map;
				for (i = 0; i < sizeof (scrnmap_t); *c++ = i++)
					/* null */;
			}
			qreply(qp, mp);
			return;
		}
		if (kp->scrn_direction != KD_DFLTSET) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}
		error = drv_priv(iocp->ioc_cr);
		if (error) {
			chr_iocnack(qp, mp, iocp, error, 0);
			break;
		}

		if (!scrp->scr_defltp->scr_map_p) {
			if (!ws_newscrmap(scrp->scr_defltp, KM_NOSLEEP)) {
				chr_iocnack(qp, mp, iocp, ENOMEM, 0);
				return;
			}
		}
		bcopy((caddr_t)&kp->scrn_map,
		    (caddr_t)scrp->scr_defltp->scr_map_p, sizeof (scrnmap_t));
		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case KDGKBENT: {
		struct kbentry *kbep;
		charmap_t *cmp;

		cmp = cp->c_map_p;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}


		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp, 0);
			return;
		}

		if (pullupmsg(mp->b_cont, sizeof (struct kbentry)) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of KDGKBENT ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		kbep = (struct kbentry *)mp->b_cont->b_rptr;

		if (kbep->kb_index >= 128) {
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			return;
		}

		switch (kbep->kb_table) {
		case K_NORMTAB:
			kbep->kb_value = chr_getkbent(cp, NORMAL,
			    kbep->kb_index);
			break;
		case K_SHIFTTAB:
			kbep->kb_value = chr_getkbent(cp, SHIFTED,
			    kbep->kb_index);
			break;
		case K_ALTTAB:
			kbep->kb_value = chr_getkbent(cp, ALT,
			    kbep->kb_index);
			break;
		case K_ALTSHIFTTAB:
			kbep->kb_value = chr_getkbent(cp, ALTSHF,
			    kbep->kb_index);
			break;
		case K_SRQTAB:
			kbep->kb_value = *((unchar *)cmp->cr_srqtabp +
			    kbep->kb_index);
			break;
		default:
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			return;
		}

		cqp = (struct copyreq *)mp->b_rptr;
		cqp->cq_size = sizeof (*kbep);
		cqp->cq_addr = (caddr_t)cp->c_copystate.cpy_arg;
		cqp->cq_flag = 0;
		cqp->cq_private = (mblk_t *)copyp;
		copyp->cpy_state = CHR_OUT_0;
		mp->b_wptr = mp->b_rptr + sizeof (struct copyreq);
		mp->b_datap->db_type = M_COPYOUT;
		qreply(qp, mp);
		return;
	}

	case KDSKBENT: {
		struct kbentry *kbep;
		charmap_t *cmp;
		ushort numkeys;

		cmp = cp->c_map_p;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}


		if (pullupmsg(mp->b_cont, sizeof (struct kbentry)) == 0) {
			cmn_err(CE_WARN,
			    "char: pull up of KDSKBENT ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}

		kbep = (struct kbentry *)mp->b_cont->b_rptr;

		if (kbep->kb_index >= 128) {
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			return;
		}

		numkeys = cmp->cr_keymap_p->n_keys;

		if (!ws_newkeymap(cmp, numkeys, cmp->cr_keymap_p,
		    KM_NOSLEEP)) {
			cmn_err(CE_WARN,
			    "char: could not allocate new keymap");
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			return;
		}

		if (!ws_newpfxstr(cmp, KM_NOSLEEP)) {
			cmn_err(CE_WARN,
			    "char: could not allocate new pfxstr");
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			return;
		}

		switch (kbep->kb_table) {
		case K_NORMTAB:
			chr_setkbent(cp, NORMAL, kbep->kb_index,
			    kbep->kb_value);
			break;
		case K_SHIFTTAB:
			chr_setkbent(cp, SHIFTED, kbep->kb_index,
			    kbep->kb_value);
			break;
		case K_ALTTAB:
			chr_setkbent(cp, ALT, kbep->kb_index,
			    kbep->kb_value);
			break;
		case K_ALTSHIFTTAB:
			chr_setkbent(cp, ALTSHF, kbep->kb_index,
			    kbep->kb_value);
			break;
		case K_SRQTAB:		/* SysReq table */
			kbep->kb_value =
				*((unchar *) cmp->cr_srqtabp + kbep->kb_index);
			break;
		default:
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			return;
		}

		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	}
}

static void
chr_proc_w_data(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	register unchar *chp;
	register scrnmap_t *scrnp;
	scrn_t *map_p;
	mblk_t *bp;

	if ((map_p = cp->c_scrmap_p) == NULL) {
		putnext(qp, mp);
		return;
	}

	if ((scrnp = map_p->scr_map_p) == NULL) {
		if ((scrnp = map_p->scr_defltp->scr_map_p) == NULL) {
			putnext(qp, mp);
			return;
		}
	}

	for (bp = mp; bp != (mblk_t *)NULL; bp = bp->b_cont) {
		chp = (unchar *)bp->b_rptr;
		while (chp != (unchar *)bp->b_wptr) {
			*chp = *((unchar *)scrnp + *chp);
			chp++;
		}
	}

	putnext(qp, mp);
}


#ifdef DONT_INCLUDE
void
chr_do_xmouse(qp, mp, cp)
queue_t *qp;
mblk_t *mp;
charstat_t *cp;
{
	register struct mse_event *msep;
	register xqEvent *evp;

	msep = (struct mse_event *)mp->b_cont->b_rptr;
	evp = &cp->c_xevent;

	evp->xq_type = msep->type;
	evp->xq_x = msep->x;
	evp->xq_y = msep->y;
	evp->xq_code = msep->code;

	(*cp->c_xqinfo->xq_addevent)(cp->c_xqinfo, evp);
}
#endif

static void
chr_do_mouseinfo(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	register struct mse_event *msep;
	register struct mouseinfo *minfop;

	msep = (struct mse_event *)mp->b_cont->b_rptr;
	minfop = &cp->c_mouseinfo;
	minfop->status = (~msep->code & 7) |
	    ((msep->code ^ cp->c_oldbutton) << 3) |
	    (minfop->status & BUTCHNGMASK) | (minfop->status & MOVEMENT);

	if (msep->type == MSE_MOTION) {
		register int sum;

		minfop->status |= MOVEMENT;
		sum = minfop->xmotion + msep->x;
		if (sum >= chr_maxmouse)
			minfop->xmotion = chr_maxmouse;
		else if (sum <= -chr_maxmouse)
			minfop->xmotion = -chr_maxmouse;
		else
			minfop->xmotion = (char)sum;
		sum = minfop->ymotion + msep->y;
		if (sum >= chr_maxmouse)
			minfop->ymotion = chr_maxmouse;
		else if (sum <= -chr_maxmouse)
			minfop->ymotion = -chr_maxmouse;
		else
			minfop->ymotion = (char)sum;
	}
	/* Note the button state */
	cp->c_oldbutton = msep->code;
	cp->c_state |= C_MSEINPUT;
	if (cp->c_heldmseread) {
		mblk_t *tmp;
		tmp = cp->c_heldmseread;
		cp->c_heldmseread = (mblk_t *)NULL;
		chr_write_queue_put(WR(qp), tmp);
	}
}

static void
chr_proc_w_proto(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	register ch_proto_t *chp;

	if ((int)(mp->b_wptr - mp->b_rptr) != sizeof (ch_proto_t)) {
		putnext(qp, mp);
		return;
	}


	chp = (ch_proto_t *)mp->b_rptr;

	if (chp->chp_type != CH_CTL) {
		putnext(qp, mp);
		return;
	}

	switch (chp->chp_stype) {

	default:		/* all but CH_TCL are invalid here */
		putnext(qp, mp);
		return;

	case CH_TCL:
		break;

	} /* switch */

	/* CH_TCL message handling */
	switch (chp->chp_stype_cmd) {

	default:
		putnext(qp, mp);
		return;

	case TCL_ADD_STR: {
		tcl_data_t *tp;
		ushort keynum, len;

		if (mp->b_cont == NULL) {
			putnext(qp, mp);
			return;
		}

		/* assume one data block */
		tp = (tcl_data_t *)mp->b_cont->b_rptr;

		keynum = tp->add_str.keynum;
		len = tp->add_str.len;

		/*
		 * put tp past the tcl_data structure in the
		 * data block. Now it points to the string itself
		 */

		tp++;

		/* if ws_addstring fails, beep the driver */
		if (!ws_addstring(cp->c_map_p, keynum, (unchar *)tp, len)) {
			chp->chp_stype_cmd = TCL_BELL;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			putnext(qp, mp);	/* ship it down */
		} else
			freemsg(mp);

		break;
	}

	case TCL_FLOWCTL:
		if (chp->chp_stype_arg == TCL_FLOWON)
			cp->c_state |= C_FLOWON;
		else if (chp->chp_stype_arg == TCL_FLOWOFF)
			cp->c_state &= ~C_FLOWON;
		putnext(qp, mp);
		break;
	} /* switch */
}
