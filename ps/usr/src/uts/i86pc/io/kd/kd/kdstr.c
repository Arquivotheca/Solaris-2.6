/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1996, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)kdstr.c	1.52	96/09/16 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/inline.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/sysinfo.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/ioctl.h"
#include "sys/termios.h"
#include "sys/stream.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/kd.h"
#include "sys/ws/chan.h"
#include "sys/ws/ws.h"
#include "sys/ws/tcl.h"
#include "sys/cred.h"
#include "sys/jioctl.h"
#include "sys/kmem.h"
#include "sys/kb.h"
#include "sys/vid.h"
#include "sys/user.h"
#include "sys/consdev.h"
#include "sys/mem.h"
#include "sys/archsystm.h"
#include <sys/modctl.h>

#include "sys/ddi.h"
#include "sys/stat.h"
#include "sys/sunddi.h"

/* function prototypes for tcl_ functions used here */
extern void	tcl_cursor(wstation_t *, channel_t *);
extern int	tcl_curattr(channel_t *);
extern void	tcl_norm(wstation_t *, channel_t *, termstate_t *, ushort);
extern void	tcl_scrolllock(wstation_t *, channel_t *, int);
extern void	tcl_handler(wstation_t *, mblk_t *, termstate_t *, channel_t *);

/* function prototypes for kdkb_ functions used here */
extern void	kdkb_init(wstation_t *);
extern void	kdkb_cmd(unchar, unchar);
extern void	kdkb_tone(void);
extern void	kdkb_setled(channel_t *, kbstate_t *, unchar);
extern int	kdkb_scrl_lock(channel_t *);
extern int	kdkb_locked(ushort, unchar);
extern void	kdkb_keyclick(ushort);
extern void	kdkb_force_enable(void);


/* function prototypes for ws_ functions used here */
extern channel_t *ws_activechan(wstation_t *);
extern channel_t *ws_getchan(wstation_t *, int);
extern charmap_t *ws_cmap_alloc(wstation_t *, int);
extern int	ws_enque(queue_t *, mblk_t **, unchar);
extern int	ws_freechan(wstation_t *);
extern int	ws_getchanno(minor_t);
extern int	ws_procscan(charmap_t *, kbstate_t *, unchar);
extern int	ws_specialkey(keymap_t *, kbstate_t *, unchar);
extern int	ws_speckey(ushort);
extern int	ws_toglchange(ushort, ushort);
extern ushort	ws_scanchar(charmap_t *, kbstate_t *, unsigned char, unsigned int);
extern ushort	ws_shiftkey(ushort, unchar, keymap_t *, kbstate_t *, unchar);
extern void	ws_chinit(wstation_t *, channel_t *, int);
extern void	ws_chrmap(queue_t *, channel_t *);
extern void	ws_closechan(queue_t *, wstation_t *, channel_t *, mblk_t *);
extern void	ws_cmap_free(wstation_t *, charmap_t *);
extern void	ws_cmap_init(wstation_t *, int);
extern void	ws_cmap_reset(wstation_t *,  charmap_t *);
extern void	ws_copyout(queue_t *, mblk_t *, mblk_t *, uint);
extern void	ws_iocack(queue_t *, mblk_t *, struct iocblk *);
extern void	ws_iocnack(queue_t *, mblk_t *, struct iocblk *, int);
extern void	ws_kbtime(wstation_t *);
extern void	ws_mctlmsg(queue_t *, mblk_t *);
extern void	ws_openresp(queue_t *, mblk_t *, ch_proto_t *, channel_t *, unsigned long);
extern void	ws_preclose(wstation_t *, channel_t *);
extern void	ws_rstmkbrk(queue_t *, mblk_t **, ushort, ushort);
extern void	ws_scrn_free(wstation_t *, channel_t *);
extern void	ws_scrn_init(wstation_t *, int);
extern void	ws_scrn_reset(wstation_t *,  channel_t *);
extern void	ws_set_char_mode (kbstate_t *, long);
extern void	ws_winsz(queue_t *, mblk_t *, channel_t *, int);




static int kdclose(queue_t *qp, int flag, cred_t *credp);
static int kdopen(queue_t *qp, dev_t *devp, int flag, int sflag,
    cred_t *credp);
static int kdwsrv(queue_t *qp);
static int kdwput(queue_t *qp, mblk_t *mp);

static void	kdmiocdatamsg(queue_t *, mblk_t *);

struct module_info
	kds_info = { 42, "kd", 0, 32, 256, 128 };

static struct qinit
	kd_rinit = { NULL, NULL, kdopen, kdclose, NULL, &kds_info, NULL };

static struct qinit
	kd_winit = { kdwput, kdwsrv, kdopen, kdclose, NULL, &kds_info, NULL };

struct streamtab
	kd_str_info = { &kd_rinit, &kd_winit, NULL, NULL };

wstation_t	Kdws = {0};

struct kdptrs {
	channel_t	*k_chanpp[WS_MAXCHAN+1];
	charmap_t	*k_charpp[WS_MAXCHAN+1];
	ushort		*k_scrnpp[WS_MAXCHAN+1];
} Kdptrs = { 0 };

channel_t	Kd0chan = {0};
caddr_t		kd_va;		/* virtual address of mapped vga area */
extern caddr_t	p0_va;		/* virtual address of mapped page 0 for VGA */
				/* mapping is done in machdep.c */

static int kd_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
		void **result);
static int kd_attach(dev_info_t *, ddi_attach_cmd_t);
static int kd_reset(dev_info_t *, ddi_reset_cmd_t);
int kd_mmap(dev_t, off_t, int);

static dev_info_t *kd_dip;
major_t	kd_major = (major_t)-1;

static void kdresetcpu(void);
static void kdsetval(ushort addr, unchar reglow, ushort val);
static void kdnotsysrq(kbstate_t *kbp);
static void kdmiocdatamsg(queue_t *qp, mblk_t *mp);
static int kdcksysrq(charmap_t *cmp, kbstate_t *kbp, ushort ch, unchar scan);
static void kdproto(queue_t *qp, mblk_t *mp);
static void kdmioctlmsg(queue_t *qp, mblk_t *mp);
static void kdstart(void);
static void kdinit(void);
static int kdgetchar();
static int kdputchar();
static int kdtone();
static int kdshiftset();
static int kdischar();

#define	KD_CONF_FLAG	0


static 	struct cb_ops cb_kd_ops = {
	nulldev,		/* cb_open */
	nulldev,		/* cb_close */
	nodev,			/* cb_strategy */
	nodev,			/* cb_print */
	nodev,			/* cb_dump */
	nodev,			/* cb_read */
	nodev,			/* cb_write */
	nodev,			/* cb_ioctl */
	nodev,			/* cb_devmap */
	kd_mmap,		/* cb_mmap */
	nodev,			/* cb_segmap */
	nochpoll,		/* cb_chpoll */
	ddi_prop_op,		/* cb_prop_op */
	(&kd_str_info),		/* cb_stream */
	(int)(KD_CONF_FLAG)	/* cb_flag */
};

struct dev_ops kd_ops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	kd_info,		/* devo_getinfo */
	nulldev,		/* devo_identify */
	nulldev,		/* devo_probe */
	kd_attach,		/* devo_attach */
	nodev,			/* devo_detach */
	kd_reset,		/* devo_reset */
	&(cb_kd_ops),		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};
/*
 * Module linkage information for the kernel.
 */

static struct modldrv modldrv = {
	&mod_driverops, /* Type of module.  This one is a pseudo driver */
	"KD driver",
	&kd_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,
	(void *)&modldrv,
	NULL
};


int
_init(void)
{     
	int	rv;

	rv = mod_install(&modlinkage);
	return (rv);
}


int
_fini(void)
{
	return (mod_remove(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*ARGSUSED*/
static int
kd_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	extern int	console;
	extern int	(*getcharptr)();
	extern int	(*putcharptr)();
	extern int	(*ischarptr)();

	u_int	kdintr ();
	ddi_iblock_cookie_t 	tmp;

	if (ddi_create_minor_node(devi, "kd", S_IFCHR,
	    0, NULL, NULL) == DDI_FAILURE) {
		ddi_remove_minor_node(devi, NULL);
		return (-1);
	}
	kd_dip = devi;

	(void) ddi_map_regs(kd_dip, 0, &kd_va, 0, 0);

	/*
	 * This is so that phystokv can see the right address
	 */
	kd_va -= 0xA0000;

	kdinit();
	if (console == CONSOLE_IS_FB) {
		getcharptr = kdgetchar;
		putcharptr = kdputchar;
		ischarptr = kdischar;
	}

	/* Establish initial softc values */
	if (ddi_add_intr(devi, (u_int) 0, &tmp,
	    (ddi_idevice_cookie_t *) 0, kdintr, (caddr_t)0)) {
		cmn_err (CE_PANIC, "kd_attach: cannot add intr");
		/* NOTREACHED */
	}
	return (DDI_SUCCESS);
}


/*
 *	Called when the system is reset (shut down).
 *	Eventually, this should flip to the primary (console) VT so the user
 *	can see what's happening.
 */
/* ARGSUSED */
static int
kd_reset(dev_info_t *dip, ddi_reset_cmd_t rst)
{
	switch (rst) {
	case DDI_RESET_FORCE:
		return (0);
	default:
		return (0);
	}
}


/* ARGSUSED */
static int
kd_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
	void **result)
{
	register int error;

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (kd_dip == NULL) {
			error = DDI_FAILURE;
		} else {
			*result = (void *) kd_dip;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)0;
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}


unchar	Kd0tabs[ANSI_MAXTAB];

extern struct font_info fontinfo[];
extern ushort	kd_iotab[][MKDBASEIO];
extern struct b_param kd_inittab[];
extern struct reginfo kd_regtab[];
extern unchar	kd_ramdactab[];
extern struct cgareginfo kd_cgaregtab[];
extern struct m6845init kd_cgainittab[];
extern struct m6845init kd_monoinittab[];
extern long	kdmonitor[];
extern char	kd_swmode[];
extern struct attrmask	kb_attrmask[];
extern int	nattrmsks;

extern int	kdv_sborder();

extern int	kdvt_rel_refuse(),
		kdvt_acq_refuse(),
		kdvt_activate(),
		kdv_cursortype(),
		kdvm_mapdisp(),
		kdvm_unmapdisp();

extern long	inrtnfirm;	/* rtnfirm should set this to non-zero */

#ifdef EVGA
extern  int evga_inited;
extern  int evga_mode;
extern  unchar saved_misc_out;
extern  struct at_disp_info  disp_info[];
#endif	/* EVGA */

struct ext_graph kd_vdc800 = { 0 };	/* vdc800 hook structure */


/*
 *
 */

static void
kdinit(void)
{
	unchar	kd_get_attributes ();
	ushort	kdgetval ();
	ushort	cursor_pos;

	if (Kdws.w_init)
		return;
	cursor_pos = kdgetval (COLOR_REGBASE, R_CURADRL);
	Kdws.w_init++;
	Kdws.w_stchar = kdv_stchar;
	Kdws.w_clrscr = kdclrscr;
	Kdws.w_setbase = kdsetbase;
	Kdws.w_activate = kdvt_activate;
	Kdws.w_setcursor = kdsetcursor;
	Kdws.w_active = 0;
	Kdws.w_bell = kdtone;
	Kdws.w_setborder = kdv_sborder;
	Kdws.w_shiftset = kdshiftset;
	Kdws.w_undattr = kdv_undattrset;
	Kdws.w_rel_refuse = kdvt_rel_refuse;
	Kdws.w_acq_refuse = kdvt_acq_refuse;
#ifdef DONT_INCLUDE
	Kdws.w_unmapdisp = kdvm_unmapdisp;
#endif
	Kdws.w_ticks = 0;
	Kdws.w_mvword = kdv_mvword;
	Kdws.w_switchto = (channel_t *)NULL;
	Kdws.w_scrllck = kdkb_scrl_lock;
	Kdws.w_cursortype = kdv_cursortype;
	Kdws.w_wsid = 0;	/* not used by KD, but initialize it anyway */
	Kdws.w_private = (caddr_t) NULL;	/* not used by KD */
	Kdws.w_qp = (queue_t *)NULL;
	kdv_init(&Kd0chan);
	Kdws.w_chanpp = Kdptrs.k_chanpp;
	Kdws.w_scrbufpp = Kdptrs.k_scrnpp;
	Kdws.w_nchan ++;
	Kd0chan.ch_tstate.t_tabsp = Kd0tabs;
	Kd0chan.ch_nextp = Kd0chan.ch_prevp = &Kd0chan;
	Kdws.w_chanpp[0] = &Kd0chan;
	kdstart ();
	ws_chinit(&Kdws, &Kd0chan, 0);
	ws_cmap_init(&Kdws, KM_NOSLEEP);
	ws_scrn_init(&Kdws, KM_NOSLEEP);
	Kd0chan.ch_charmap_p = ws_cmap_alloc(&Kdws, KM_NOSLEEP);
	ws_scrn_alloc(&Kdws, &Kd0chan);
	Kdptrs.k_charpp[0] = Kd0chan.ch_charmap_p;

	Kdws.w_scrbufpp[0] =
	    (ushort *)kmem_alloc(sizeof (ushort) * KD_MAXSCRSIZE, KM_NOSLEEP);
	if (Kdws.w_scrbufpp[0] == (ushort *) NULL)
		cmn_err(CE_WARN, "kdinit: out of memory for screen");

	/*
	 * This hack is to keep the position and attributes left
	 * by the boot.
	 * Instead of clearing the screen, read the cursor position from the
	 * VGA adapter and read the attributes from the
	 * character buffer.
	 */
	/*
	kdclrscr(&Kd0chan, Kd0chan.ch_tstate.t_origin, Kd0chan.ch_tstate.t_scrsz);
	*/
	{
		termstate_t	*tsp;
		unchar		nattributes;	/* normal attributes */
		unchar		rattributes;	/* reverse attributes */
		unchar		fg;
		unchar		bg;

		tsp = &Kd0chan.ch_tstate;

		nattributes = kd_get_attributes (&Kdws.w_vstate);
		fg = nattributes & 0xf;		/* split out foreground color */
		bg = nattributes >> 4;		/* same for background color */
		rattributes = (fg << 4) | bg;	/* create reverse attributes */
		rattributes &= ~BLINK;
		rattributes |= BRIGHT;
		tsp->t_nfcolor = fg;	/* normal foreground color */
		tsp->t_nbcolor = bg;	/* normal background color */
		tsp->t_rfcolor = bg;	/* reverse fg video color */
		tsp->t_rbcolor = fg;	/* reverse bg video color */
		tsp->t_attrmskp[0].attr = nattributes;
		tsp->t_attrmskp[7].attr = rattributes;

		tsp->t_normattr = nattributes;
		tsp->t_curattr = tsp->t_normattr;

		if (tsp->t_cols != 0)
			tsp->t_row = ((cursor_pos - tsp->t_origin)
			    / tsp->t_cols);
		else
			tsp->t_row = 0;
		tsp->t_col = 0;
		tcl_cursor (&Kdws, &Kd0chan);
	}
	Kdws.w_init++;

	/* read keyboard scan data to clear possible spurious data */
	(void) inb(KB_IDAT);

	drv_setparm(SYSRINT, 1);	/* reset keyboard interrupts */
}

/*
 *
 */

static void
kdstart(void)
{
	kdkb_init(&Kdws);
}

/*
 *
 */

/*ARGSUSED2*/
static int
kdopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	int	indx;
	channel_t	*chp;
	unchar		*tabsp;

	if (qp->q_ptr != (caddr_t) NULL)
		return (EBUSY);

	if (kd_major == (major_t)-1)		/* stash kd's major number */
		kd_major = getmajor (*devp);

	indx = getminor(*devp);
	if (indx > WS_MAXCHAN)		/* device number out of range */
		return (ENODEV);

	if (Kdws.w_mp == (mblk_t *)NULL) {
		if ((Kdws.w_mp = allocb(4, BPRI_MED)) == (mblk_t *)NULL)
			cmn_err(CE_PANIC, "kdopen: no msg blocks");
	}

	if ((chp = ws_getchan(&Kdws, indx)) == (channel_t *)NULL) {
		chp = kmem_zalloc(sizeof (channel_t), KM_SLEEP);
		tabsp = kmem_alloc(ANSI_MAXTAB, KM_SLEEP);
		Kdws.w_nchan++;	/* increment count of configured channels */
		Kdws.w_chanpp[indx] = chp;
		chp->ch_tstate.t_tabsp = tabsp;
		ws_chinit(&Kdws, chp, indx);
		chp->ch_charmap_p = ws_cmap_alloc(&Kdws, KM_SLEEP);
		ws_scrn_alloc(&Kdws, chp);
		Kdptrs.k_charpp[indx] = chp->ch_charmap_p;
	}
	qp->q_ptr = (caddr_t)chp;
	WR(qp)->q_ptr = qp->q_ptr;
	chp->ch_qp = qp;
	if (!Kdws.w_qp)
		Kdws.w_qp = chp->ch_qp;

	return (0);
}


/*
 * Close of /dev/kd/.... (when the chanmux is being disassembled),
 * and /dev/vtmon
 */

/*ARGSUSED*/
static int
kdclose(queue_t *qp, int flag, cred_t *credp)
{
#ifdef XXX_BSH
	channel_t	*chp = (channel_t *)qp->q_ptr;
	int	indx, oldpri;

	if (!chp->ch_id)	/* channel 0 never "really" closes */
		return;
	if (Kdws.w_qp == chp->ch_qp) {
		if (Kdws.w_timeid) {
			(void) untimeout(Kdws.w_timeid);
			Kdws.w_timeid = 0;
		}
		Kdws.w_qp = (queue_t *)NULL;
	}
	flushq(WR(qp), FLUSHALL);
	chp->ch_qp = (queue_t *)NULL;
	qp->q_ptr = WR(qp)->q_ptr = (caddr_t)NULL;
	indx = chp->ch_id;
	if (Kdws.w_scrbufpp[indx]) {
		kmem_free(Kdws.w_scrbufpp[indx], sizeof (ushort) * KD_MAXSCRSIZE);
	}
	Kdws.w_scrbufpp[indx] = (ushort *)NULL;
	kmem_free(chp->ch_tstate.t_tabsp, ANSI_MAXTAB);
	ws_cmap_free(&Kdws, chp->ch_charmap_p);
	ws_scrn_free(&Kdws, chp);
	kmem_free(Kdws.w_chanpp[indx], sizeof (channel_t));
	Kdws.w_chanpp[indx] = (channel_t *)NULL;
	Kdws.w_nchan--;	/* decrement count of configured channels */
#endif XXX_BSH
	return (0);
}

/*
 *	Streams write put - simply place the message onto the queue,
 *	to be handled later by the write service procedure.
 */

static int
kdwput(queue_t *qp, mblk_t *mp)
{
	(void) putq(qp, mp);
	return (0);
}


/*
 *
 */

static int
kdwsrv(queue_t *qp)
{
	channel_t	*chp = (channel_t *)qp->q_ptr;
	mblk_t		*mp;
	termstate_t	*tsp;

	tsp = &chp->ch_tstate;
	while ((mp = getq(qp))) {
		switch (mp->b_datap->db_type) {
		case M_PROTO:
		case M_PCPROTO:
			if ((mp->b_wptr - mp->b_rptr) != sizeof (ch_proto_t)) {
				cmn_err(CE_NOTE,
				    "kdwput: bad M_PROTO or M_PCPROTO msg");
				freemsg(mp);
				break;
			}
			kdproto(qp, mp);
			continue;
		case M_DATA:
			/* We should check to see if we're panicing the system.
			 * If so, then forcibly switch to the console VT (if
			 * not already there), then display the output.
			 */
			if (ddi_in_panic()) {
				/* if this is for the console, and the console
				 * is not the active channel, flip to it.
				 */
				channel_t *cons = ws_getchan(&Kdws, 0);
				if (cons != ws_activechan(&Kdws))
					kdvt_activate(cons, VT_FORCE|VT_NOSAVE);
			}

			/* writes as if to /dev/null when in KD_GRAPHICS mode */
			if (chp->ch_dmode != KD_GRAPHICS) {
				while (mp->b_rptr != mp->b_wptr)
					tcl_norm(&Kdws, chp, tsp, *mp->b_rptr++);
			}
			freemsg(mp);
			break;
		case M_CTL:
			ws_mctlmsg(qp, mp);
			continue;
		case M_IOCTL:
			kdmioctlmsg(qp, mp);
			continue;
		case M_IOCDATA:		/* response to M_COPYIN/M_COPYOUT */
			kdmiocdatamsg(qp, mp);
			continue;
		case M_DELAY:
		case M_STARTI:
		case M_STOPI:
		case M_READ:	/* ignore, no buffered data */
			freemsg(mp);
			continue;
		case M_FLUSH:
			*mp->b_rptr &= ~FLUSHW;
			if (*mp->b_rptr & FLUSHR)
				qreply(qp, mp);
			else
				freemsg(mp);
			continue;
		default:
			cmn_err(CE_NOTE, "kdwput: bad msg type %x",
			    mp->b_datap->db_type);
			freemsg(mp);	/* toss the message */
		}
		qenable(qp);
		return (0);
	}
	return (0);
}

/*
 *	Called from kdwsrv() to handle M_IOCTL messages.
 */

static void
kdmioctlmsg(queue_t *qp, mblk_t *mp)
{
	struct iocblk	*iocp;
	channel_t	*chp = (channel_t *)qp->q_ptr;
	struct strtty	*sttyp;
	mblk_t		*tmp;
	ch_proto_t	*protop;
	int		rval = 0;
	int		copyflag = 0;
	int		hit = 1;
	int		reply = 1;
	int		error;
/* XXX
	if (mp->b_wptr - mp->b_rptr != sizeof (struct iocblk)) {
		cmn_err(CE_NOTE, "!kdmioctlmsg: bad M_IOCTL msg");
		return;
	}
*/
	sttyp = (struct strtty *)&chp->ch_strtty;
	iocp = (struct iocblk *)mp->b_rptr;

	switch (iocp->ioc_cmd) {

	case KIOCINFO:
		iocp->ioc_rval = ('k' << 8) | 'd';
		ws_iocack(qp, mp, iocp);
		break;

	case KDGKBTYPE:		/* get the keyboard type.  Fills in user ptr. */
		if (!(tmp = allocb(sizeof (unchar), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get msg for reply to KDGKBTYPE");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}
		*(unchar *)tmp->b_rptr = Kdws.w_kbtype;
		tmp->b_wptr += sizeof (unchar);
		ws_copyout(qp, mp, tmp, sizeof (unchar));
		break;

	case KDGETLED:		/* get the LED state.  Fills in user ptr. */
		if (!(tmp = allocb(sizeof (unchar), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get msg for reply to KDGETLED");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}
		*(unchar *)tmp->b_rptr = ws_getled(&chp->ch_kbstate);
		tmp->b_wptr += sizeof (unchar);
		ws_copyout(qp, mp, tmp, sizeof (unchar));
		break;

	case KDSETLED:
		if (Kdws.w_timeid) {
			(void) untimeout(Kdws.w_timeid);
			Kdws.w_timeid = 0;
		}
		if (!(tmp = allocb(sizeof (ch_proto_t), BPRI_HI))) {
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}
		kdkb_setled(chp, &chp->ch_kbstate,
		    (unchar)(*(int *)mp->b_cont->b_rptr));
		tmp->b_datap->db_type = M_PROTO;
		tmp->b_wptr += sizeof (ch_proto_t);
		protop = (ch_proto_t *)tmp->b_rptr;
		protop->chp_type = CH_CTL;
		protop->chp_stype = CH_CHR;
		protop->chp_stype_cmd = CH_LEDSTATE;
		protop->chp_stype_arg = (chp->ch_kbstate.kb_state & ~NONTOGGLES);
		ws_iocack(qp, mp, iocp);
		ws_kbtime(&Kdws);
		qreply(qp, tmp);
		break;

	case TCSETSW:
	case TCSETSF:
	case TCSETS: {
		struct termios	*tsp;

		if (!mp->b_cont) {
			ws_iocnack(qp, mp, iocp, EINVAL);
			break;
		}
		tsp = (struct termios *)mp->b_cont->b_rptr;
		sttyp->t_cflag = tsp->c_cflag;
		sttyp->t_iflag = tsp->c_iflag;
		ws_iocack(qp, mp, iocp);
		break;
	}
	case TCSETAW:
	case TCSETAF:
	case TCSETA: {
		struct termio	*tp;

		if (!mp->b_cont) {
			ws_iocnack(qp, mp, iocp, EINVAL);
			break;
		}
		tp = (struct termio *)mp->b_cont->b_rptr;
		sttyp->t_cflag = (sttyp->t_cflag & 0xffff0000 | tp->c_cflag);
		sttyp->t_iflag = (sttyp->t_iflag & 0xffff0000 | tp->c_iflag);
		ws_iocack(qp, mp, iocp);
		break;
	}
	case TCGETA: {
		struct termio	*tp;

		if (mp->b_cont)		/* bad user-supplied parameter */
			freemsg(mp->b_cont);
		mp->b_cont = allocb(sizeof (struct termio), BPRI_MED);
		if (mp->b_cont == (mblk_t *)NULL) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get msg for reply to TCGETA");
			freemsg(mp);
			break;
		}
		tp = (struct termio *)mp->b_cont->b_rptr;
		tp->c_iflag = (ushort)sttyp->t_iflag;
		tp->c_cflag = (ushort)sttyp->t_cflag;
		mp->b_cont->b_wptr += sizeof (struct termio);
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = sizeof (struct termio);
		qreply(qp, mp);
		break;
	}
	case TCGETS: {
		struct termios	*tsp;

		if (mp->b_cont)		/* bad user-supplied parameter */
			freemsg(mp->b_cont);
		mp->b_cont = allocb(sizeof (struct termios), BPRI_MED);
		if (mp->b_cont == (mblk_t *)NULL) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get msg for reply to TCGETS");
			freemsg(mp);
			break;
		}
		tsp = (struct termios *)mp->b_cont->b_rptr;
		tsp->c_iflag = sttyp->t_iflag;
		tsp->c_cflag = sttyp->t_cflag;
		mp->b_cont->b_wptr += sizeof (struct termios);
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = sizeof (struct termios);
		qreply(qp, mp);
		break;
	}

	case KBIO_SETMODE:
		kbio_setmode(qp, mp, iocp);
		break;

	case KBIO_GETMODE:
		iocp->ioc_rval = chp->ch_charmap_p->cr_kbmode;
		ws_iocack(qp, mp, iocp);
		break;

/* NOT IMPLEMENTED
	case TIOCSWINSZ:
		ws_iocack(qp, mp, iocp);
		break;
	case TIOCGWINSZ:
	case JWINSIZE:
		ws_winsz(qp, mp, chp, iocp->ioc_cmd);
		break;
NOT IMPLEMENTED */

	case TCSBRK:
		ws_iocack(qp, mp, iocp);
		break;

	case GIO_ATTR:		/* return current attribute */
		iocp->ioc_rval = tcl_curattr(chp);
		ws_iocack(qp, mp, iocp);
		break;

	case VT_OPENQRY:	/* fill in number of first free VT */
		if (!(tmp = allocb(sizeof (int), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdmioctlmsg: can't get msg for reply to VT_OPENQRY");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}
		*(int *)tmp->b_rptr = ws_freechan(&Kdws);
		tmp->b_wptr += sizeof (int);
		ws_copyout(qp, mp, tmp, sizeof (int));
		break;

	default:
#if defined(__DOS_EMUL) && defined(__MERGE)
		if (merge_kdppi_ioctl(qp, mp, iocp, chp)){
			cmn_err(CE_NOTE, "!kdmioctlmsg %x", iocp->ioc_cmd);
			break;
		}
#endif

		if (iocp->ioc_count != TRANSPARENT) {
			/*
			 * Don't handle it if it's not transparent
			 */
			ws_iocnack(qp, mp, iocp, EINVAL);
			return;
		}

		/*
		 *	Give it to kdvm_ioctl and see if it can handle it.
		 *	If it recognizes the command, it will set 'hit'.
		 *	If it returns non-zero, that is the 'errno' value
		 *	to return to the user.
		 *
		 *	If "reply" is set, then:
		 *	If 'copyflag' is set, then the value 'rval' needs to
		 *	be copied out to the user's address space, otherwise
		 *	'rval' simply becomes the ioctl's return value.
		 *	(If reply is zero, then assume that kdvm_ioctl has
		 *	communicated upstream directly and 'copyflag' is
		 *	irrelevant.)
		 */
		error = kdvm_ioctl(qp, mp, &hit, &rval, &copyflag, &reply);

		if (! hit) {		/* kdvm_ioctl didn't recognize it */
			ws_iocnack(qp, mp, iocp, EINVAL);
			break;
		}

		/* at this point, kdvm_ioctl recognized the command */

		if (error) {		/* ioctl failed - set errno value */
			ws_iocnack(qp, mp, iocp, error);
			break;
		}

		if (! reply) {
			/*
			 * kdvm_ioctl has communicated upstream directly
			 * (most likely M_COPYIN/M_COPYOUT), and we do not
			 * need to handle it here.
			 */
			break;
		}

		/* see if the value needs to be copied out */
		if (copyflag) {
			/* allocate a message block to hold the value */
			tmp = allocb(sizeof (int), BPRI_MED);

			/* check for allocation failure */
			if (tmp == (mblk_t *) NULL) {
				cmn_err(CE_NOTE,
				"!kdmioctlmsg: can't get msg for return value");
				ws_iocnack(qp, mp, iocp, ENOMEM);
				break;
			}

			/* fill in the mblock with the value to copy out */
			*(int *)tmp->b_rptr = rval;
			tmp->b_wptr += sizeof (int);

			/* send it up the stream */
			ws_copyout(qp, mp, tmp, sizeof (int));

			/*
			 * The driver will be called again with an M_IOCDATA
			 * message, after the copyout is done.  At that time,
			 * we still need to send an IOCACK message with an
			 * ioc_rval of 0.
			 */
			/* Fortunately (?), kdmiocdatamsg() does that for us. */
		} else {	/* rval simply becomes the return value */
			iocp->ioc_rval = rval;		/* set return value */
			ws_iocack(qp, mp, iocp);
		}
	}
}

/*
 *	Called from kdwsrv() to handle M_PROTO and M_PCPROTO messages.
 */

static void
kdproto(queue_t *qp, mblk_t *mp)
{
	register ch_proto_t *chprp;
	channel_t *chp;
	int error;

	chp = (channel_t *)qp->q_ptr;
	chprp = (ch_proto_t *)mp->b_rptr;
	switch (chprp->chp_stype) {
	case CH_TCL:
		if (chprp->chp_stype_cmd == TCL_FLOWCTL) {
			tcl_scrolllock(&Kdws, chp, chprp->chp_stype_arg);
			break;
		}
		/* writes as if to /dev/null when in KD_GRAPHICS mode */
		if (chp->ch_dmode != KD_GRAPHICS)
			tcl_handler(&Kdws, mp, &chp->ch_tstate, chp);
		break;

	case CH_CHAN:
		switch (chprp->chp_stype_cmd) {
		case CH_CHANOPEN:	/* open the specified channel */
			error = kdvt_open(chp, chprp->chp_stype_arg);
			ws_openresp(qp, mp, chprp, chp, error);
			return;	/* do not free mp because ws_openresp will */

		case CH_CHANCLOSE:
#ifdef XXX_BSH
			if (chp->ch_id == 0 && chp == ws_activechan(&Kdws)) {
#else
			if (chp == ws_activechan(&Kdws)) {
#endif
			   vidstate_t *vp;
			   vp = &chp->ch_vstate;
#ifdef EVGA
			   evga_ext_rest(vp->v_cvmode);
#endif	/* EVGA */

			   if (vp->v_cvmode != vp->v_dvmode) {
				kdv_setdisp(chp, vp, &chp->ch_tstate, Kdws.w_vstate.v_dvmode);
				kdclrscr(chp, chp->ch_tstate.t_origin, chp->ch_tstate.t_scrsz);
			   }
#ifdef EVGA
			   /* kdv_setdisp() sets vp->v_cvmode to new mode */
			   evga_ext_init(vp->v_cvmode);
#endif EVGA
			}
			ws_preclose(&Kdws, chp);
			kdvt_close(chp);
			ws_closechan(qp, &Kdws, chp, mp);	/* XXX_BSH */
#ifdef XXX_BSH
			chp->ch_kbstate.kb_state = 0;
			ws_closechan(qp, &Kdws, chp, mp);
			ws_cmap_reset(&Kdws, chp->ch_charmap_p);
			ws_scrn_reset(&Kdws, chp);
			if (chp == ws_activechan(&Kdws))
				kdkb_setled(chp, &chp->ch_kbstate, 0); /* turn off LEDs */
			else
				chp->ch_vstate.v_scrp = Kdws.w_scrbufpp[chp->ch_id];
			if (chp->ch_id != 0) {
				ws_chinit(&Kdws, chp, chp->ch_id);
				if (Kdws.w_scrbufpp[chp->ch_id])
					kdclrscr(chp, chp->ch_tstate.t_origin,
					    chp->ch_tstate.t_scrsz);
			}
#endif XXX_BSH
			if (chp != ws_activechan(&Kdws))
				chp->ch_vstate.v_scrp = Kdws.w_scrbufpp[chp->ch_id];
			return;	/* don't free mp */

		case CH_CHRMAP:
			freemsg(mp);
			ws_chrmap(qp, chp);
			return;

		default:
			cmn_err(CE_WARN,
			    "kd_proto, received unknown CH_CHAN %d",
			    chprp->chp_stype_cmd);
		}
		break;

	case CH_CHAR_MODE:
		ws_set_char_mode (&chp->ch_kbstate,
				chprp->chp_stype_cmd);
		break;

	default:
		cmn_err(CE_WARN,
		    "kd_proto, received unknown CH_CTL %d", chprp->chp_stype);
		break;
	}
	freemsg(mp);
}

/*
 * Called from interrupt handler when keyboard interrupt occurs.
 */

u_int
kdintr()
{
	register unchar	rawscan, /* XT raw keyboard scan code */
			kbscan,	/* AT/XT raw scan code */
			scan;	/* "cooked" scan code */
	channel_t	*achp;	/* active channel pointer */
	charmap_t	*cmp;	/* character map pointer */
	keymap_t	*kmp;
	kbstate_t	*kbp;	/* pointer to keyboard state */
	unchar	kbrk,
		oldprev;
	ushort	ch,
		okbstate;
	extern 	int	kb_raw_mode;

	ASSERT(UNSAFE_DRIVER_LOCK_HELD());
	if (!(inb(KB_STAT) & KB_OUTBF)) {	/* no data from keyboard? */
		rawscan = inb(KB_IDAT);		/* clear possible spurious data */
		drv_setparm(SYSRINT, 1);	/* don't care if it succeeds */
		return (DDI_INTR_UNCLAIMED);	/* return immediately */
	}
	kbscan = inb(KB_IDAT);		/* read scan data */
	if (kb_raw_mode == KBM_AT)
		rawscan = kd_xlate_at2xt(kbscan);
	else
		rawscan = kbscan;

	drv_setparm(SYSRINT, 1);	/* don't care if it succeeds */
	if (rawscan == KB_ACK) {	/* ack from keyboard? */
		return (DDI_INTR_UNCLAIMED);	/* Spurious ACK -- cmds to keyboard now polled */
	}
	if (!Kdws.w_init)	/* can't do anything anyway */
		return (DDI_INTR_CLAIMED);
	kbrk = rawscan & KBD_BREAK;
	if ((achp = ws_activechan(&Kdws)) == (channel_t *)NULL) {
		cmn_err(CE_NOTE,
		    "kdintr: received interrupt before active channel");
		return (DDI_INTR_CLAIMED);
	}
	kbp = &achp->ch_kbstate;
	if (Kdws.w_qp != achp->ch_qp) {
		cmn_err(CE_NOTE, "kdintr: no active channel queue");
		return (DDI_INTR_CLAIMED);
	}
	if ((cmp = achp->ch_charmap_p) == (charmap_t *)NULL) {
		cmn_err(CE_NOTE, "kdintr: no valid ch_charmap_p");
		return (DDI_INTR_CLAIMED);
	}
	Kdws.w_intr++;
	okbstate = kbp->kb_state;
	kmp = cmp->cr_keymap_p;
	oldprev = kbp->kb_prevscan;
	ch = ws_scanchar(cmp, kbp, rawscan, 0);

	/* check for handling extended scan codes correctly */
	/* this is because ws_scanchar calls ws_procscan on its own */
	if (oldprev == 0xe0 || oldprev == 0xe1)
		kbp->kb_prevscan = oldprev;

	scan = ws_procscan(cmp, kbp, rawscan);
	if (!kbrk)
		kdkb_keyclick(ch);
	if (kdkb_locked(ch, kbrk)) {
		Kdws.w_intr = 0;
		return (DDI_INTR_CLAIMED);
	}
	if (!kbrk) {
		if ((ws_specialkey(kmp, kbp, scan) || kbp->kb_sysrq) &&
		    kdcksysrq(cmp, kbp, ch, scan)) {
			Kdws.w_intr = 0;
			return (DDI_INTR_CLAIMED);
		}
	} else if (kbp->kb_sysrq)
		if (kbp->kb_srqscan == scan) {
				Kdws.w_intr = 0;
				return (DDI_INTR_CLAIMED);
		}
	if (ws_toglchange(okbstate, kbp->kb_state) &&
			kbp->kb_proc_state == B_TRUE)
		kdkb_cmd(LED_WARN, FROM_DRIVER);
	if (Kdws.w_timeid) {
		(void) untimeout(Kdws.w_timeid);
		Kdws.w_timeid = 0;
	}
	if (ws_enque(Kdws.w_qp, &Kdws.w_mp, kbscan))
		Kdws.w_timeid = timeout((void(*)())ws_kbtime, (caddr_t)&Kdws,
					HZ / 29);
	Kdws.w_intr = 0;
	return (DDI_INTR_CLAIMED);
}


/*
 *	Called from kdwsrv() to handle M_IOCDATA messages, which are in
 *	response to copyin/copyout requests.
 */

static void
kdmiocdatamsg(queue_t *qp, mblk_t *mp)
{
	struct copyresp	*csp;

	csp = (struct copyresp *)mp->b_rptr;
	if (csp->cp_rval) {
		freemsg(mp);
		return;
	}

	switch (csp->cp_cmd) {
	case VT_SETMODE:
		kdvt_setmode_bottom (qp, mp);
		break;

	case VT_SENDSIG:
		kdvt_sendsig_bottom (qp, mp);
		break;

	case WS_PIO_ROMFONT:
		if (csp->cp_private == (mblk_t *)1)
			ws_pio_romfont_bottom (qp, mp);
		else
			ws_pio_romfont_middle (qp, mp);
		break;

	case KDMAPDISP:
		kdvm_mapdisp (qp, mp);
		break;

#ifdef EVGA
	case KDEVGA:
		evga_init_bottom (qp, mp);
		break;
#endif

	case MCAIO:
	case CGAIO:
	case EGAIO:
	case VGAIO:
	case CONSIO:
		/*
		 * kdvm_xenixdoio_bottom() must check csp->cp_private to
		 * determine if this is simply the response to its previous
		 * COPYOUT request, otherwise this would cause a loop.
		 */
		kdvm_xenixdoio_bottom (qp, mp);
		break;

	case PIO_FONT8x16:
		kdv_setxenixfont_bottom(qp, mp, csp->cp_private);
		break;

	default:
		ws_iocack(qp, mp, (struct iocblk *)mp->b_rptr);
		break;
	}
}


/*
 *
 */

static int
kdcksysrq(charmap_t *cmp, kbstate_t *kbp, ushort ch, unchar scan)
{
	extern int	kadb_is_running;
	keymap_t	*kmp = cmp->cr_keymap_p;

	if (kbp->kb_sysrq) {
		kbp->kb_sysrq = 0;
		ch = *(*cmp->cr_srqtabp + scan);
		if (!ch) {
			kdnotsysrq(kbp);
			return (0);
		}
	}

	if (ws_speckey(ch) == HOTKEY) {		/* is this a VT switch key? */
		kdvt_switch(ch);
		return (1);
	}

	switch (ch) {
	case K_DBG:			/* enter the debugger */
		/*
		 * If no debugger return 0
		 * so the application sees the ctrl-alt-d
		 */
		if (!kadb_is_running)
			return (0);
		/* save the state on entry........... */
		kbp->kb_sstate = kbp->kb_state;
		debug_enter ((char *)NULL);
		kdnotsysrq(kbp);
		/* ......and restore it on exit */
		kbp->kb_state &= ~(CAPS_LOCK | NUM_LOCK | SCROLL_LOCK);
		kbp->kb_state |= (kbp->kb_sstate &
				(CAPS_LOCK | NUM_LOCK | SCROLL_LOCK));
		kdkb_cmd (LED_WARN, FROM_DRIVER);
		return (1);
	case K_RBT:			/* reboot the system */
		kdresetcpu();
		return (0);
	case K_SRQ:			/* System Request key */
		if (ws_specialkey(kmp, kbp, scan)) {
			kbp->kb_sstate = kbp->kb_state;
			kbp->kb_srqscan = scan;
			kbp->kb_sysrq++;
			return (1);
		}
		break;
	default:
		break;
	}
	return (0);
}

/*
 *
 */

static void
kdnotsysrq(kbstate_t *kbp)
{
	ushort	msk;

	if ((msk = kbp->kb_sstate ^ kbp->kb_state) != 0) {
		if (Kdws.w_timeid) {
			(void) untimeout(Kdws.w_timeid);
			Kdws.w_timeid = 0;
		}
		ws_rstmkbrk(Kdws.w_qp, &Kdws.w_mp, kbp->kb_sstate, msk);
		(void) ws_enque(Kdws.w_qp, &Kdws.w_mp, kbp->kb_srqscan);
		(void) ws_enque(Kdws.w_qp, &Kdws.w_mp, 0x80 | kbp->kb_srqscan);
		ws_rstmkbrk(Kdws.w_qp, &Kdws.w_mp, kbp->kb_state, msk);
	}
}

/*
 *	Clear a portion of the screen by filling with spaces using the
 *	current video attribute.
 */

int
kdclrscr(chp, last, cnt)
channel_t	*chp;
ushort	last;
int	cnt;
{
	if (cnt)
		kdv_stchar(chp, last,
			(ushort)(chp->ch_tstate.t_curattr << 8 | ' '), cnt);
	return (0);
}

/*
 * Implement TCL_BELL functionality.  Only do it if active channel.
 */

static int
kdtone(wsp, chp)
wstation_t	*wsp;
channel_t	*chp;
{
	if (chp == ws_activechan(wsp))	/* active channel */
		kdkb_tone();
	return (0);
}	


/*
 * perform a font shift in/shift out if requested by the active
 * channel
 */

static int
kdshiftset(wsp, chp, dir)
wstation_t	*wsp;
channel_t	*chp;
int	dir;
{
	if (chp == ws_activechan(wsp))	/* active channel */
		kdv_shiftset(&chp->ch_vstate, dir);
	return (0);
}

/*
 *
 */

static void
kdsetval(ushort addr, unchar reglow, ushort val)
{
	register int	s;

	s = clear_int_flag();
	outb(addr, reglow);
	outb(addr + DATA_REG, val & 0xFF);
	outb(addr, reglow - 1);
	outb(addr + DATA_REG, (val >> 8) & 0xFF);
	restore_int_flag(s);
}

unchar
kd_get_attributes (vp)
vidstate_t	*vp;
{
	unchar	attr;
	ushort	val;

	val = *vp->v_scrp;
	attr = val >> 8;
	return (attr);
}
/*
 *
 */

ushort
kdgetval(addr, reglow)
ushort	addr;
unchar	reglow;
{
	register int	s;
	unsigned char	vl, vh;
	ushort	val;

	s = clear_int_flag();
	outb(addr, reglow);
	vl = inb(addr + DATA_REG);
	outb(addr, reglow - 1);
	vh = inb(addr + DATA_REG);
	restore_int_flag(s);
	val = vh << 8 | vl;
	return (val);
}

/*
 *
 */

int
kdsetcursor(chp, tsp)
channel_t	*chp;
termstate_t	*tsp;
{
	vidstate_t	*vp = &chp->ch_vstate;

	if (chp == ws_activechan(&Kdws)) {
		kdsetval(vp->v_regaddr, R_CURADRL, tsp->t_cursor);
	}
	return (0);
}

/*
 *
 */

int
kdsetbase(chp, tsp)
channel_t	*chp;
termstate_t	*tsp;
{
	vidstate_t	*vp = &Kdws.w_vstate;

	if (chp == ws_activechan(&Kdws))
		kdsetval(vp->v_regaddr, R_STARTADRL, tsp->t_origin);
	return (0);
}

/*
 *
 */

static void
kdresetcpu(void)
{
	if (inrtnfirm) {
		softreset();
		SEND2KBD(KB_ICMD, KB_RESETCPU);
	}
}

/*
 *
 */

static int
kdputchar(ch)
unchar	ch;
{
	register int	cnt = 0;
	unchar		out[2];
	channel_t	*chp;

	/* convert LF to CRLF */
	if (ch == '\n')
		out[cnt++] = '\r';
	out[cnt++] = ch;
	chp = ws_getchan(&Kdws, 0);
	wsansi_parse(&Kdws, chp, out, cnt);
	return (0);
}

/*
 *
 */


static unchar	lastscan;
static int	got_scan = 0;

static int
kdischar()
{
	register ushort ch;	/* processed scan code */
	register unchar rawscan;	/* raw keyboard scan code */
	channel_t	*chp;
	charmap_t	*cmp;
	kbstate_t	*kbp;
	ushort		okbstate;

	/* If there's already a saved character return true */
	if (got_scan)
		return (1);

	/* if no character in keyboard output buffer, return 0 */
	if (!(inb(KB_STAT) & KB_OUTBF)) {
		return (0);
	}

	/* get the scan code */
	rawscan = inb(KB_IDAT);		/* Read scan data */
	kdkb_force_enable();
	chp = ws_getchan(&Kdws, 0);
	cmp = chp->ch_charmap_p;
	kbp = &chp->ch_kbstate;

	/*
	 * Call ws_scanchar to convert scan code to a character.
	 * ws_scanchar returns a short, with flags in the top byte and the
	 * character in the low byte.
	 * A legal ascii character will have the top 9 bits off.
	 */
	okbstate = kbp->kb_state;
	ch = ws_scanchar(cmp, kbp, rawscan, 0);

	if (ws_toglchange(okbstate, kbp->kb_state)) {
		kdkb_cmd(LED_WARN, FROM_DEBUGGER);
		return (0);
	}
	if (ch & 0xFF80)
		return (0);
	else {
		lastscan = rawscan;
		got_scan = 1;
		return (1);
	}
}

/*
 *
 */

static int
kdgetchar()
{
	register ushort ch;		/* processed scan code */
	register unchar rawscan, kbrk;	/* raw keyboard scan code */
	channel_t	*chp;
	charmap_t	*cmp;
	keymap_t	*kmp;
	kbstate_t	*kbp;
	ushort		okbstate;

	chp = ws_getchan(&Kdws, 0);
	cmp = chp->ch_charmap_p;
	kmp = cmp->cr_keymap_p;
	kbp = &chp->ch_kbstate;
do_it_again:
	if (got_scan) {
		rawscan = lastscan;
		kbrk = rawscan & KBD_BREAK;
		got_scan = 0;
	} else {
		/* wait for character in keyboard output buffer */
		while (!(inb(KB_STAT) & KB_OUTBF))
			;
		/* get the scan code */
		rawscan = inb(KB_IDAT);		/* Read scan data */
		kdkb_force_enable();
		kbrk = rawscan & KBD_BREAK;
	}

	/*
	 * Call ws_scanchar to convert scan code to a character.
	 * ws_scanchar returns a short, with flags in the top byte and the
	 * character in the low byte.
	 * A legal ascii character will have the top 9 bits off.
	 */
	okbstate = kbp->kb_state;
	ch = ws_scanchar(cmp, kbp, rawscan, 0);
	if (ws_toglchange(okbstate, kbp->kb_state))
		kdkb_cmd(LED_WARN, FROM_DEBUGGER);

	(void) ws_shiftkey(ch, (rawscan & ~KBD_BREAK), kmp, kbp, kbrk);
	if (ch & 0xFF80) {
		switch (ch) {
		case K_DBG:
			debug_enter ((char *)NULL);
			break;
		case K_RBT:
			kdresetcpu();
			return (ch);
		default:
			break;
		}
		goto do_it_again;
	} else
		return (ch);
}


/* VDC800 hook */
#ifdef	OLD
unchar *
kd_vdc800_ramd_p()
{
	channel_t	*achp;
	vidstate_t	*vp;

	achp = ws_activechan(&Kdws);
	vp = &achp->ch_vstate;
	return (&kd_ramdactab[0] + (WSCMODE(vp)->m_ramdac * 0x300));
}

kd_vdc800_access()
{
	dev_t devp;

	if (ws_getctty(&devp) || (getminor(devp) != Kdws.w_active))
		return (1);
	kd_vdc800.procp = curproc;
	kd_vdc800.pid = curproc->p_pid;
	return (0);
}

#endif	/* OLD */

void
kd_vdc800_release()
{
	kd_vdc800.procp = (struct proc *) 0;
	kd_vdc800.pid = 0;
}




/* mmap routine */

/*ARGSUSED*/
int
kd_mmap(dev, off, prot)
dev_t dev;
off_t off;
{
	int	pf;

	pf = btop(off);
	if (pf < 0xA0 || pf >= 0x100)
		return (-1);

	return (impl_obmem_pfnum(pf));
	/*XXX we're supposed to return hat_getkpfnum(caddr_t vaddr) XXX*/
}
