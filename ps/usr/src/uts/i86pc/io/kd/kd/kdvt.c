/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved 					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1996, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)kdvt.c	1.18	96/07/30 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/inline.h"
#include "sys/cmn_err.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/stream.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/kd.h"
#include "sys/ws/ws.h"
#include "sys/ws/tcl.h"
#include "sys/cred.h"
#include "sys/vid.h"
#include "sys/kb.h"
#include "sys/kmem.h"
#include "sys/vdc.h"
#include "sys/ddi.h"
#include "sys/sunddi.h"

/* function prototypes for kdkb_ functions used here */
extern void	kdkb_cmd(unchar, unchar);
extern void	kdkb_tone(void);
extern void	kdkb_sound(int);


/* VDC800 hook */
extern struct ext_graph kd_vdc800;

extern struct vdc_info	Vdc;
extern wstation_t	Kdws;

extern channel_t	*ws_activechan(wstation_t *);
extern channel_t	*ws_getchan(wstation_t *, int);

static int get_next_vt(channel_t *chp, int direction);
extern channel_t Kd0chan;

#ifdef EVGA
extern int	evga_inited;
extern int	cur_mode_is_evga;
extern unchar	saved_misc_out;
extern struct at_disp_info disp_info[];
#endif	/* EVGA */

/* prototypes for functions within this source file */
void	kdvt_switch(ushort);


/*
 *
 */

/*ARGSUSED*/
int
kdvt_open(chp, ppid)
channel_t	*chp;
pid_t	ppid;
{
	register int	indx;
	channel_t	*achp;	/* pointer to currently active channel */
	channel_t	*tmp, *save;
	ushort		*scrp;

	if (!(achp = ws_activechan(&Kdws))) {
		cmn_err(CE_WARN, "kdvt_open: no active channel");
		return (EINVAL);
	}
	indx = chp->ch_id;

	if (!Kdws.w_scrbufpp[indx]) {	/* first open of this VT */
		/* allocate the screen save buffer */
		scrp = (ushort *)kmem_zalloc(sizeof (ushort) * KD_MAXSCRSIZE,
				KM_NOSLEEP);
		if (!scrp) {
			cmn_err(CE_WARN,
			    "kdvt_open: out of memory for new screen for virtual terminal 0x%x",
			    indx);
			kdvt_switch(K_VTF);	/* switch to channel 0 */
			return (ENOMEM);
		}
		Kdws.w_scrbufpp[indx] = scrp;
		if (chp != achp)
			chp->ch_vstate.v_scrp = scrp;
		kdclrscr(chp, chp->ch_tstate.t_origin, chp->ch_tstate.t_scrsz);

		/*
		 * Set up the links between this channel and the rest -
		 * link this one after those with lower ids, and before
		 * those with higher ids.  This allows the "activate previous"
		 * and "next" functions to behave predictably.
		 */

		/* loop through the list, finding the highest-numbered
		 * channel that's still lower than our index.
		 */
		tmp = save = achp;
		do {
			/* this channel's index is less than ours, but it's
			 * greater than the last saved one */
			if (tmp->ch_id < indx && tmp->ch_id > save->ch_id)
				save = tmp;	/* save it */
			tmp = tmp->ch_nextp;
		} while (tmp != achp);

		/*
		 * Okay, we've found the one that goes before us, so hook
		 * up the pointers - the new channel goes immediately after
		 * the channel we just found.
		 */
		chp->ch_nextp = save->ch_nextp;		/* save the "next" */
		save->ch_nextp->ch_prevp = chp;		/* old next's prev */
		chp->ch_prevp = save;			/* set up "prev" */
		save->ch_nextp = chp;			/* append new one */
	}

	return (0);
}

/*
 *
 */

/*ARGSUSED*/
void
kdvt_close(chp)
channel_t	*chp;
{
#ifdef XXX_BSH
	int		id;

	id = chp->ch_id;
	if (id != 0 && Kdws.w_scrbufpp[id]) {
		chp->ch_vstate.v_scrp = (ushort *) NULL;
		kmem_free(Kdws.w_scrbufpp[id], sizeof (ushort) * KD_MAXSCRSIZE);
		Kdws.w_scrbufpp[id] = (ushort *) NULL;
	}
#endif XXX_BSH
}


/*
 *	Determine whether the passed channel is in "normal" mode (i.e.
 *	not in graphics mode, not mapped by a process, and not in "process"
 *	mode.)
 */
kdvt_isnormal(chp)
channel_t *chp;
{
	ws_mapavail(chp, &Kdws.w_map);
	if (chp->ch_dmode == KD_GRAPHICS ||
	    CHNFLAG(chp, CHN_MAPPED) || CHNFLAG(chp, CHN_PROC))
		return (0);
	else
		return (1);
}

/* VDC800 hooks */

void (*kd_vdc800_vgamode)();

void
kd_vdc800_vgapass(vgapass_p)
void (*vgapass_p)();
{
	kd_vdc800_vgamode = vgapass_p;
}

/*
 *
 */

#define	NEXT	0
#define	PREV	1

void
kdvt_switch(ushort ch)
{
	extern void kd_vdc800_release();
	channel_t *newchp, *chp, *vtmchp;
	int	chan = -1;

	if ((chp = ws_activechan(&Kdws)) == (channel_t *) NULL) {
		cmn_err(CE_WARN,
		    "kdvt_switch: Could not find active channel!");
		return;
	}

	/* check if VDC800 is open and active vt is not in proc mode */
	if (kd_vdc800.procp && (chp->ch_flags & CHN_PROC) == 0) {
		mutex_enter (&pidlock);
		if (prfind(kd_vdc800.pid) != NULL) {
			mutex_exit (&pidlock);
			kdvt_rel_refuse();
			return;
		} else {		/* process died */
			mutex_exit (&pidlock);
			if (kd_vdc800_vgamode)
				(*kd_vdc800_vgamode)();
			kd_vdc800_release();
		}
	}

	if (ch >= K_VTF && ch <= K_VTL)
		chan = ch - K_VTF;

	switch (ch) {
	case K_NEXT:
		chan = get_next_vt (chp, NEXT);
		break;

	case K_PREV:
		chan = get_next_vt (chp, PREV);
		break;

	case K_FRCNEXT:
		if (kdvt_isnormal(chp)) {
			chan = get_next_vt (chp, NEXT);
			break;
		}
		ws_force(&Kdws, chp);
		return;

	case K_FRCPREV:
		if (kdvt_isnormal(chp)) {
			chan = get_next_vt (chp, PREV);
			break;
		}
		ws_force(&Kdws, chp);
		return;

	default:
		break;
	}

	if (chan != -1 && (newchp = ws_getchan(&Kdws, chan))) {
		if (newchp->ch_opencnt || (chan == 0)) {
			if (ws_activate(&Kdws, newchp, VT_NOFORCE))
				return;
		} else if ((vtmchp = ws_getchan(&Kdws, WS_MAXCHAN))) {
			ws_notifyvtmon(vtmchp, ch);
			return;
		}
	}
	kdvt_rel_refuse();
}

static int
get_next_vt(channel_t *chp, int direction)
{
	channel_t	*newchp;

	newchp = chp;
	do {
		switch (direction) {
		case NEXT:
			newchp = newchp->ch_nextp;
			break;
		case PREV:
			newchp = newchp->ch_prevp;
			break;
		}

		if (newchp->ch_opencnt != 0)	/* is channel open? */
			break;			/* yes - we're done */
	} while (newchp != chp);
	return (newchp->ch_id);
}


/*
 *
 */

extern void	ws_xferkbstat(kbstate_t *,  kbstate_t *);


int
kdvt_activate(channel_t *chp, int force)
{
	channel_t	*achp;
	vidstate_t	*vp;
	termstate_t	*tsp;
	kbstate_t	*kbp;
	unchar		tmp;

	if (Kdws.w_flags & WS_NOCHANSW)		/* does ws allow switching? */
		return (0);			/*  no.  */

	if ((achp = ws_activechan(&Kdws)) == (channel_t *)NULL) {
		cmn_err(CE_WARN, "kdvt_activate: no active channel");
		return (0);
	}

	ws_mapavail(achp, &Kdws.w_map);

	if ((achp->ch_dmode == KD_GRAPHICS || CHNFLAG(achp, CHN_MAPPED)) &&
	    !CHNFLAG(achp, CHN_PROC))
		return (0);

	/* save state of current channel */
	vp = &achp->ch_vstate;
	tsp = &achp->ch_tstate;

	kdkb_sound(0);			/* make sure sound is turned off */

	if (achp->ch_dmode != KD_GRAPHICS) {	/* save text mode state */
		/* unless NOSAVE was set, save the current screen buffer */
		if (!(force & VT_NOSAVE))
			kdv_scrxfer(achp, KD_SCRTOBUF);
		tsp->t_cursor -= tsp->t_origin;
		tsp->t_origin = 0;
	} else {
		vp->v_font = 0;
	}
	vp->v_scrp = *(Kdws.w_scrbufpp + achp->ch_id);
	if (force & VT_NOSAVE) {
		vp->v_cvmode = vp->v_dvmode;
		achp->ch_dmode = KD_TEXT0;
		tsp->t_cols = WSCMODE(vp)->m_cols;
		tsp->t_rows = WSCMODE(vp)->m_rows;
		tsp->t_scrsz = tsp->t_rows * tsp->t_cols;
	} else
		vp->v_dvmode = vp->v_cvmode; /* XXX -- 3.2 behavior was the reverse! */

	if (Kdws.w_timeid) {
		(void) untimeout(Kdws.w_timeid);
		Kdws.w_timeid = 0;
	}
	kbp = &achp->ch_kbstate;
	ws_xferkbstat(kbp, &chp->ch_kbstate);
	ws_rstmkbrk(Kdws.w_qp, &Kdws.w_mp, kbp->kb_state, NONTOGGLES);

	/* check to see whether any scancode data was remaining for the
	 * previous channel, passing it along the stream before we actually
	 * switch the channels, if needed.
	 */
	ws_kbtime(&Kdws);

	Kdws.w_active = chp->ch_id;
	Kdws.w_qp = chp->ch_qp;
	vp = &chp->ch_vstate;
	tsp = &chp->ch_tstate;
	kbp = &chp->ch_kbstate;

	/* Cause calls to kdv_setmode to block */
	Kdws.w_flags |= WS_NOMODESW;

#ifdef EVGA
	evga_ext_rest(cur_mode_is_evga);
#endif	/* EVGA */

	/* XXX used to be kdv_setdisp */
	if (chp->ch_dmode == KD_GRAPHICS)
		kdv_rst(tsp, vp);
	else
		kdv_setdisp(chp, vp, tsp, vp->v_cvmode);

#ifdef EVGA
	evga_ext_init(vp->v_cvmode);
#endif	/* EVGA */

	if (chp->ch_dmode != KD_GRAPHICS) {
		if (VTYPE(V400) || DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {
			if (vp->v_undattr == UNDERLINE) {
				kdv_setuline(vp, 1);
				kdv_mvuline(vp, 1);
			} else {
				kdv_setuline(vp, 0);
				kdv_mvuline(vp, 0);
			}
		}
		kdsetbase(chp, tsp);
		kdv_scrxfer(chp, KD_BUFTOSCR);
		kdsetcursor(chp, tsp);
		if (DTYPE(Kdws, KD_VGA)) {
			(void) inb(vp->v_regaddr + IN_STAT_1);
			outb(0x3c0, 0x10);	/* attribute mode control reg */
			tmp = inb(0x3c1);
			if (tsp->t_flags & T_BACKBRITE)
				outb(0x3c0, (tmp & ~0x08));
			else
				outb(0x3c0, (tmp | 0x08));
			outb(0x3c0, 0x20);	/* turn palette on */
		}
	}
	if ((chp->ch_dmode != KD_GRAPHICS) || !(WSCMODE(vp)->m_font))
		kdv_enable(vp);
	if (Kdws.w_timeid) {
		(void) untimeout(Kdws.w_timeid);
		Kdws.w_timeid = 0;
	}
	ws_rstmkbrk(Kdws.w_qp, &Kdws.w_mp, kbp->kb_state,
	    (kbp->kb_state & NONTOGGLES));

	switch_kb_mode(chp->ch_charmap_p->cr_kbmode);

	ws_kbtime(&Kdws);

	/* update the current LED state */
	kdkb_cmd(LED_WARN, FROM_DRIVER);
	Kdws.w_flags &= ~WS_NOMODESW;
	wakeup((caddr_t)&Kdws.w_flags);
	return (1);
}


/*
 *
 *	Called from kdvm_ioctl in ../kdvm/kdvm.c to handle VT-specific ioctls.
 *	Note that VT_OPENQRY is handled directly by kdmioctlmsg() in
 *	../kd/kdstr.c, so we will never see it here.
 */

/*ARGSUSED*/
int
kdvt_ioctl(queue_t *qp, mblk_t *mp, struct iocblk *iocp, channel_t *chp, int cmd, int arg, int *reply)
{
	channel_t	*newchp;
	int		rv = 0;
	struct vt_mode	vtmode;
	struct vt_stat	vtstat;
	mblk_t		*tmp;

	switch (cmd) {

	case VT_GETMODE:	/* fill in user-supplied "struct vt_mode" */
		vtmode.mode = ws_procmode(&Kdws, chp) ? VT_PROCESS : VT_AUTO;
		vtmode.waitv = CHNFLAG(chp, CHN_WAIT) ? 1 : 0;
		vtmode.relsig = chp->ch_relsig;
		vtmode.acqsig = chp->ch_acqsig;
		vtmode.frsig = chp->ch_frsig;

		if (!(tmp = allocb(sizeof (struct vt_mode), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdvt_ioctl: can't get msg for reply to VT_GETMODE");
			rv = ENOMEM;
			break;
		}
		/* fill in the data block with a copy of the "vtmode" struct */
		*(struct vt_mode *)tmp->b_rptr = vtmode;

		tmp->b_wptr += sizeof (struct vt_mode);

		/* arrange to copy it out to user */
		ws_copyout(qp, mp, tmp, sizeof (struct vt_mode));

		*reply = 0;	/* upper code shouldn't reply */

		/* this ioctl will be completed when the M_IOCDATA response
		 * to our copyout request comes, and kdmiocdatamsg() will send
		 * the M_IOCACK to finish the ioctl.
		 */
		break;

	case VT_SETMODE:
		/* arrange to copy in the user's struct vt_mode info */
		ws_copyin(qp, mp, (caddr_t)arg, sizeof (struct vt_mode),
		    (mblk_t *)NULL);

		*reply = 0;	/* upper code shouldn't reply */

		/* Processing this ioctl will be continued when the M_IOCDATA
		 * response to our copyin request comes, and kdmiocdatamsg()
		 * will call kdvt_setmode_bottom to handle the next step.
		 */
		break;

	case VT_RELDISP:	/* process releases or ack's acquire */
		if ((int)arg == VT_ACKACQ) {	/* ack of acquire */
			Kdws.w_noacquire = 0;
			if (CHNFLAG(chp, CHN_ACTV)) {
				(void) untimeout(chp->ch_timeid);
				chp->ch_timeid = 0;
			}
			break;
		}
		/* process is releasing the display */
		if (!Kdws.w_noacquire) {
			if (chp != ws_activechan(&Kdws)) {
				rv = EACCES;
				break;
			}
			if (!Kdws.w_switchto) {
				rv = EINVAL;
				break;
			}
		}
		/* VDC800 hook */
		if (kd_vdc800.procp != (struct proc *) 0) {
			kdvt_rel_refuse();
			rv = EACCES;
			break;
		}
		if (arg && ws_switch(&Kdws, Kdws.w_switchto, VT_NOFORCE))
			break;		/* successfully switched to other VT */

		kdvt_rel_refuse();
		(void) untimeout(chp->ch_timeid);
		chp->ch_timeid = 0;
		if (arg)
			rv = EBUSY;
		break;

	case VT_ACTIVATE:	/* activate the specified virtual terminal */
		if (!(newchp = ws_getchan(&Kdws, arg))) {
			rv = ENXIO;
			break;
		}
		if (!newchp->ch_opencnt) {	/* VT isn't open */
			rv = ENXIO;
			break;
		}
		ws_activate(&Kdws, newchp, VT_NOFORCE);
		break;

	case VT_WAITACTIVE:	/* wait for VT to become active (no arg) */
		if (ws_procmode(&Kdws, chp) || chp == ws_activechan(&Kdws))
			break;
		chp->ch_waitactive = mp;
		*reply = 0;		/* just return w/o acking */
		qenable(qp);
		break;

	case VT_GETSTATE:	/* get information about currently-used VTs */
		if ((newchp = ws_activechan(&Kdws)))
			vtstat.v_active = newchp->ch_id;
		vtstat.v_state = 0;
		vtstat.v_signal = 0;	/* unused for VT_GETSTATE */
		newchp = chp;

		/* find each open VT, set the corresponding bit in v_state */
		do {
			vtstat.v_state |= (1 << newchp->ch_id);
			newchp = newchp->ch_nextp;
		} while (newchp != chp);

		if (!(tmp = allocb(sizeof (struct vt_stat), BPRI_MED))) {
			cmn_err(CE_NOTE,
			    "!kdvt_ioctl: can't get msg for reply to VT_GETSTATE");
			rv = ENOMEM;
			break;
		}

		/* fill in the data block with a copy of the "vtstat" struct */
		*(struct vt_stat *)tmp->b_rptr = vtstat;

		tmp->b_wptr += sizeof (struct vt_stat);

		/* arrange to copy it out to user */
		ws_copyout(qp, mp, tmp, sizeof (struct vt_stat));

		*reply = 0;	/* upper code shouldn't reply */

		/* this ioctl will be completed when the M_IOCDATA response
		 * to our copyout request comes, and kdmiocdatamsg() will send
		 * the M_IOCACK to finish the ioctl.
		 */
		break;

	case VT_SENDSIG:	/* send signal to each specified VT */
		/* ask to copyin the user's vt_stat structure */
		ws_copyin(qp, mp, (caddr_t)arg, sizeof (struct vt_stat),
		    (mblk_t *)NULL);

		*reply = 0;	/* upper code shouldn't reply */

		/* Processing this ioctl will be continued when the M_IOCDATA
		 * response to our copyin request comes, and kdmiocdatamsg()
		 * will call kdvt_sendsig_bottom() to handle the next step.
		 */

		break;

	default:
		rv = ENXIO;
		break;
	}
	return (rv);
}

/*
 *
 */

int
kdvt_rel_refuse()
{
	if (Kdws.w_switchto)
		Kdws.w_switchto = (channel_t *)NULL;
	kdkb_tone();
	return (0);
}


/*
 * the timeout set for a process mode VT to obtain the VT has expired. We
 * leave the user in the "limbo" state of having the process mode VT be
 * the active VT.  This way, if it does respond with a VT_RELDISP/ACQACK
 * ioctl, the process will indeed own the VT. If the process is still alive,
 * this should happen.  If the process is dead, the next attempt to switch
 * will detect that it is dead and the user will be allowed to switch out.
 * Otherwise, there is VT-FORCE.
 */

int
kdvt_acq_refuse(chp)
channel_t	*chp;
{
	if (!Kdws.w_noacquire)
		return (0);
	Kdws.w_switchto = (channel_t *)NULL;
	chp->ch_timeid = 0;
	Kdws.w_noacquire = 0;
	kdvt_rel_refuse();
	return (0);
}

/*
 * bottom half of VT_SETMODE ioctl
 * The data should be present now after the copyin.
 */

void
kdvt_setmode_bottom (queue_t *qp, mblk_t *mp)
{
	struct vt_mode	vtmode;
	struct iocblk	*iocp;
	channel_t	*chp;
	mblk_t		*tmp;

	chp = (channel_t *)qp->q_ptr;
	iocp = (struct iocblk *)mp->b_rptr;
	tmp = mp->b_cont;

	vtmode = *(struct vt_mode *)tmp->b_rptr;	/* --structure copy-- */
	freemsg (tmp);
	mp->b_cont = (mblk_t *)NULL;

	if (vtmode.mode == VT_PROCESS) {	/* specifies "process" mode */
		unsigned long pid;
		if (!ws_procmode(&Kdws, chp)) {	/* not already process mode */
			/* need to get at current process */
			/*XXX fix this properly XXX*/
			drv_getparm(UPROCP,
				(unsigned long *)&chp->ch_procp);
			drv_getparm(PPID, &pid);
			chp->ch_pid = (pid_t)pid;
			chp->ch_flags |= CHN_PROC;
		}
	} else if (vtmode.mode == VT_AUTO) {
		if (CHNFLAG(chp, CHN_PROC)) {
			ws_automode(&Kdws, chp);
			chp->ch_procp = (struct proc *)0;
			chp->ch_pid = 0;
			chp->ch_flags &= ~CHN_PROC;
		}
	} else {
		ws_iocnack(qp, mp, iocp, EINVAL);
		return;
	}
	if (vtmode.waitv)
		chp->ch_flags |= CHN_WAIT;
	else
		chp->ch_flags &= ~CHN_WAIT;

	/* check that each of the signals specified is valid */
	if (vtmode.relsig) {
		if (vtmode.relsig < 0 || vtmode.relsig >= NSIG) {
			ws_iocnack(qp, mp, iocp, EINVAL);
			return;
		} else
			chp->ch_relsig = vtmode.relsig;
	}
	if (vtmode.acqsig) {
		if (vtmode.acqsig < 0 || vtmode.acqsig >= NSIG) {
			ws_iocnack(qp, mp, iocp, EINVAL);
			return;
		} else
			chp->ch_acqsig = vtmode.acqsig;
	}
	if (vtmode.frsig) {
		if (vtmode.frsig < 0 || vtmode.frsig >= NSIG) {
			ws_iocnack(qp, mp, iocp, EINVAL);
			return;
		} else
			chp->ch_frsig = vtmode.frsig;
	}
	ws_iocack (qp, mp, iocp);
}



/*
 * bottom half of VT_SENDSIG ioctl
 * The data should be present now after the copyin.
 */

void
kdvt_sendsig_bottom (queue_t *qp, mblk_t *mp)
{
	struct vt_stat	vtinfo;
	struct iocblk	*iocp;
	channel_t	*newchp;
	mblk_t		*tmp;
	int		cnt;

	iocp = (struct iocblk *)mp->b_rptr;
	tmp = mp->b_cont;

	vtinfo = *(struct vt_stat *)tmp->b_rptr;
	freemsg (tmp);
	mp->b_cont = (mblk_t *)NULL;

	for (cnt = 0; cnt < WS_MAXCHAN; cnt++) {
		if (!(vtinfo.v_state & (1 << cnt)))
			continue;
		if (!(newchp = ws_getchan(&Kdws, cnt)))
			continue;
		if (!newchp->ch_opencnt)
			continue;
		/* signal process group of channel */
		if (!newchp->ch_qp) {
			cmn_err(CE_NOTE,
			    "kdvt: warning: no queue pointer for VT %d", cnt);
			continue;
		}
		putnextctl1(newchp->ch_qp, M_SIG, vtinfo.v_signal);
	}
	ws_iocack (qp, mp, iocp);
}
