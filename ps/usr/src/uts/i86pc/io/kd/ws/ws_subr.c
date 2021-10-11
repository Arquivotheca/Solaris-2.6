/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1992-1996, by Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)ws_subr.c	1.19	96/08/01 SMI"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/inline.h"
#include "sys/cmn_err.h"
#include "sys/kmem.h"
#include "sys/vt.h"
#include "sys/at_ansi.h"
#include "sys/ascii.h"
#include "sys/proc.h"
#include "sys/termio.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "sys/kd.h"
#include "sys/ws/chan.h"
#include "sys/ws/ws.h"
#include "sys/ws/tcl.h"
#include "sys/vid.h"
#include "sys/jioctl.h"
#include "sys/eucioctl.h"
#include "sys/session.h"
#include "sys/strsubr.h"
#include "sys/cred.h"
#include "sys/conf.h"
#include "sys/ddi.h"

/* function prototypes for tcl_ functions used here */
extern void	tcl_reset(wstation_t *, channel_t *, termstate_t *);

/* function prototypes for functions defined here */
channel_t	*ws_activechan(wstation_t *);
channel_t	*ws_getchan(wstation_t *, int);
int	ws_getindex(channel_t *);
int	ws_freechan(wstation_t *);
int	ws_getchanno(minor_t);
int	ws_getws(minor_t);
void	ws_chinit(wstation_t *, channel_t *, int);
void	ws_openresp(queue_t *, mblk_t *, ch_proto_t *, channel_t *, unsigned long);
void	ws_chrmap(queue_t *, channel_t *);
void	ws_preclose(wstation_t *, channel_t *);
void	ws_closechan(queue_t *, wstation_t *, channel_t *, mblk_t *);
int	ws_activate(wstation_t *, channel_t *, int);
int	ws_switch(wstation_t *, channel_t *, int);
int	ws_procmode(wstation_t *, channel_t *);
void	ws_automode(wstation_t *, channel_t *);
void	ws_xferwords(ushort *, ushort *, int, char);
void	ws_setlock(wstation_t *, int);
static void	ws_sigkill(wstation_t *);
static void	ws_noclose(wstation_t *);
void	ws_force(wstation_t *, channel_t *);
void	ws_mctlmsg(queue_t *, mblk_t *);
void	ws_notifyvtmon(channel_t *, unchar);
void	ws_iocack(queue_t *, mblk_t *, struct iocblk *);
void	ws_iocnack(queue_t *, mblk_t *, struct iocblk *, int);
void	ws_copyin(queue_t *, mblk_t *, caddr_t, uint, mblk_t *);
void	ws_copyout(queue_t *, mblk_t *, mblk_t *, uint);
void	ws_mapavail(channel_t *, struct map_info *);
int	ws_ck_kd_port(vidstate_t *, ushort);
void	ws_winsz(queue_t *, mblk_t *, channel_t *, int);
int	ws_ioctl(dev_t, int, int, int, cred_t *, int *);
void	ws_scrnres(ulong *, ulong *);
void	ws_setcompatflgs(dev_t);
void	ws_clrcompatflgs(dev_t);
int	ws_compatset(dev_t);
void	ws_initcompatflgs(dev_t);




/*
 * This should be in proc.h but it isn't
 */

#define PTRACED(p)	((p)->p_flag & (STRC | SPROCTR))

extern wstation_t	Kdws;

/*
 * Given a (wstation_t *),
 * return the active channel as a (channel_t *).
 */

channel_t *
ws_activechan(register wstation_t *wsp)
{
	return((channel_t *)*(wsp->w_chanpp + wsp->w_active));
}

/*
 * Given a (wstation_t *) and a channel number,
 * return the channel as a (channel_t *).
 */

channel_t *
ws_getchan(register wstation_t *wsp, register int chan)
{
	if (chan < 0 || chan >= wsp->w_nchan)	/* channel is out of range */
		return (channel_t *) NULL;

	return((channel_t *)*(wsp->w_chanpp + chan));
}

/*
 * Given a (channel_t *) return the channel index.
 */

int
ws_getindex(register channel_t *chp)
{
	int	i;
	for (i = 0; i < Kdws.w_nchan; i++) {
		if (Kdws.w_chanpp[i] == chp)
			return(i);
	}
	return(0);
}

/*
 * Return the index of the next unused channel.  If all are in use,
 * return -1.
 */

int
ws_freechan(register wstation_t *wsp)
{
	register int	cnt;
	channel_t *chp;

	for (cnt = 0; cnt < wsp->w_nchan; cnt++) {
		chp = *(wsp->w_chanpp + cnt);
		if (!chp->ch_opencnt)
			return(cnt);
	}
	return(-1);
}

/*
 *
 */

int
ws_getchanno(minor_t cmux_minor)
{
	return (cmux_minor % WS_MAXCHAN);
}


int
ws_getws(minor_t cmux_minor)
{
	return (cmux_minor / WS_MAXCHAN);
}

extern struct attrmask	kb_attrmask[];
extern int	nattrmsks;

extern charmap_t	*ws_cmap_alloc();

void
ws_chinit(wstation_t *wsp, channel_t *chp, int chan)
{
	vidstate_t	*vp;
	termstate_t	*tsp;
	unchar	cnt;

	chp->ch_wsp = wsp;
	chp->ch_opencnt = 0;
	chp->ch_procp = (struct proc *)NULL;
	chp->ch_pid = 0;
	chp->ch_relsig = SIGUSR1;	/* initialize VT switching signals */
	chp->ch_acqsig = SIGUSR1;
	chp->ch_frsig = SIGUSR2;

	/* first open - initialize tty state */
	if (!(chp->ch_strtty.t_state & (ISOPEN | WOPEN))) {
		chp->ch_strtty.t_line = 0;
		chp->ch_strtty.t_iflag = IXON | ICRNL | ISTRIP;
		chp->ch_strtty.t_oflag = OPOST | ONLCR;
		chp->ch_strtty.t_cflag = B9600 | CS8 | CREAD | HUPCL;
		chp->ch_strtty.t_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK;
		chp->ch_strtty.t_state |= CARR_ON;
		chp->ch_strtty.t_state |= (ISOPEN | WOPEN);
	}
	chp->ch_id = chan;
	chp->ch_dmode = wsp->w_dmode;
	chp->ch_vstate = wsp->w_vstate;		/* struct copy */
	chp->ch_flags = 0;
#if defined(__DOS_EMUL) && defined(__MERGE)
	chp->ch_merge = 0;
#endif /* (__DOS_EMUL) && defined(__MERGE) */
	vp = &chp->ch_vstate;
	tsp = &chp->ch_tstate;
	tsp->t_sending = tsp->t_sentrows = tsp->t_sentcols = 0;
	if (vp->v_cmos == MCAP_COLOR)
		tsp->t_flags = ANSI_MOVEBASE;
	else
		tsp->t_flags = 0;
	vp->v_undattr = wsp->w_vstate.v_undattr;
	tsp->t_flags = 0;

	tsp->t_bell_time = BELLCNT;
	tsp->t_bell_freq = NORMBELL;
	tsp->t_auto_margin = AUTO_MARGIN_OFF;
	tsp->t_rows = WSCMODE(vp)->m_rows;
	tsp->t_cols = WSCMODE(vp)->m_cols;
	tsp->t_scrsz = tsp->t_rows * tsp->t_cols;
	tsp->t_attrmskp = kb_attrmask;
 	if (vp->v_regaddr == MONO_REGBASE) {
 		tsp->t_attrmskp[1].attr = 0;
 		tsp->t_attrmskp[4].attr = 1;
 		tsp->t_attrmskp[34].attr = 7;
 	} else {
 		tsp->t_attrmskp[1].attr = BRIGHT;
 		tsp->t_attrmskp[4].attr = 0;
 		tsp->t_attrmskp[34].attr = 1;
 	}

	tsp->t_nattrmsk = (unchar)nattrmsks;
	tsp->t_normattr = NORM;
	tsp->t_nfcolor = WHITE;	/* normal foreground color */
	tsp->t_nbcolor = BLACK;	/* normal background color */
	tsp->t_rfcolor = BLACK;	/* reverse foreground video color */
	tsp->t_rbcolor = WHITE;	/* reverse background video color */
	tsp->t_gfcolor = WHITE;	/* graphic foreground character color */
	tsp->t_gbcolor = BLACK;	/* graphic background character color */
	tsp->t_origin = 0;
	tsp->t_row = 0;
	tsp->t_col = 0;
	tsp->t_cursor = 0;
	tsp->t_curtyp = 0;
	tsp->t_undstate = 0;
	tsp->t_curattr = tsp->t_normattr;
	tsp->t_font = ANSI_FONT0;
	tsp->t_pstate = 0;
	tsp->t_ppres = 0;
	tsp->t_pcurr = 0;
	tsp->t_pnum = 0;
	tsp->t_ntabs = 9;
	for (cnt = 0; cnt < 9; cnt++)
		tsp->t_tabsp[cnt] = cnt * 8 + 8;
}


/*
 * ws_openresp -- expected call from principal stream upon receipt of
 * CH_CHANOPEN message from CHANMUX
 */
void
ws_openresp(queue_t *qp, mblk_t *mp, ch_proto_t *protop,
    channel_t *chp, unsigned long error)
{
	mp->b_datap->db_type = M_PCPROTO;
	protop->chp_stype = CH_PRINC_STRM;
	protop->chp_stype_cmd = CH_OPEN_RESP;
	protop->chp_stype_arg = error;

	qreply(qp, mp);

	if (error)
		return;

	chp->ch_opencnt++;
}

void
ws_chrmap(queue_t *qp, channel_t *chp)
{
	mblk_t *charmp, *scrmp;
	ch_proto_t *protop;

	if (!(charmp = allocb(sizeof(ch_proto_t), BPRI_HI)))
		return;

	charmp->b_datap->db_type = M_PROTO;
	charmp->b_wptr += sizeof(ch_proto_t);
	protop = (ch_proto_t *)charmp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHR;
	protop->chp_stype_cmd = CH_CHRMAP;
	protop->chp_stype_arg = (unsigned long)chp->ch_charmap_p;

	if ((scrmp = copymsg(charmp)) == (mblk_t *) NULL)
		qreply(qp, charmp);
	else {
		qreply(qp, charmp);
		protop = (ch_proto_t *) scrmp->b_rptr;
		protop->chp_stype_cmd = CH_SCRMAP;
		protop->chp_stype_arg = (unsigned long)&chp->ch_scrn;
		qreply(qp, scrmp);
	}
}

/*
 * ws_preclose -- call before doing actual channel close in principal
 * stream. Should be called upon receipt of a CH_CLOSECHAN message
 * from CHANMUX
 */
void
ws_preclose(wstation_t *wsp, channel_t *chp)
{
	chp->ch_flags &= ~CHN_KILLED;
	wsp->w_noacquire = 0;
	if (wsp->w_forcetimeid && (wsp->w_forcechan == chp->ch_id)) {
		(void) untimeout(wsp->w_forcetimeid);
		wsp->w_forcetimeid = 0;
		wsp->w_forcechan = 0;
	}

	if (!(ws_activechan(wsp))) {
		cmn_err(CE_WARN, "ws_preclose: no active channel");
		return;
	}
	chp->ch_opencnt = 0;
	ws_automode(wsp, chp);
	chp->ch_dmode = wsp->w_dmode;
	chp->ch_vstate = wsp->w_vstate;		/* struct copy */
	chp->ch_flags = 0;
}

/*
 * ws_close_chan() is called after principal stream-specific close() routine
 * is called. This routine sends up the CH_CLOSE_ACK message CHANMUX is
 * sleeping on
 */

/*ARGSUSED*/
void
ws_closechan(queue_t *qp, wstation_t *wsp, channel_t *chp, mblk_t *mp)
{
	ch_proto_t *protop;

	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_stype = CH_PRINC_STRM;
	protop->chp_stype_cmd = CH_CLOSE_ACK;
	qreply(qp, mp);
}


/*
 *
 */

int
ws_activate(wstation_t *wsp, channel_t *chp, int force)
{
	channel_t	*achp;

	/* see if requested channel is already the currently-active channel */
	if (chp == (achp = ws_activechan(wsp)))
		return(1);

	/*
	 * If not in process mode, or if this is a forced switch, do it
	 * the quick way (using ws_switch).
	 */
	if (!ws_procmode(wsp, achp) || force || PTRACED(achp->ch_procp))
		return(ws_switch(wsp, chp, force));

	if (wsp->w_switchto)		/* switch pending */
		return(0);

	if (wsp->w_noacquire)
		return(0);

	psignal(achp->ch_procp, achp->ch_relsig);
	wsp->w_switchto = chp;
	achp->ch_timeid = timeout((void(*)())wsp->w_rel_refuse,
				(caddr_t)wsp, 10 * HZ);
	return(1);
}

/*
 * Return 0 if switch is successful or 1 if not.
 */

int
ws_switch(wstation_t *wsp, channel_t *chp, int force)
{
	channel_t	*achp;
	int		oldpri;
	ch_proto_t	*protop;
	mblk_t		*mp;

	if (wsp->w_forcetimeid || !chp ||
	    (chp->ch_id != 0 && CHNFLAG(chp, CHN_KILLED)))
		return (0);
	if ((mp = allocb(sizeof(ch_proto_t), BPRI_HI)) == (mblk_t *)NULL)
		return(0);
	achp = ws_activechan(wsp);

	oldpri = splhi();		/*XXX*/

	if (achp->ch_timeid) {
		(void) untimeout(achp->ch_timeid);
		achp->ch_timeid = 0;
	}
	if ((*wsp->w_activate)(chp, force)) {
		mp->b_datap->db_type = M_PROTO;
		mp->b_wptr += sizeof(ch_proto_t);
		protop = (ch_proto_t *)mp->b_rptr;
		protop->chp_type = CH_CTL;
		protop->chp_stype = CH_PRINC_STRM;
		protop->chp_stype_cmd = CH_CHANGE_CHAN;
		drv_getparm(LBOLT, (unsigned long *)&protop->chp_tstmp);
		protop->chp_chan = chp->ch_id;
		putnext(chp->ch_qp, mp);
	} else {
		freemsg(mp);
		(void) splx(oldpri);	/*XXX*/
		return(0);
	}
	wsp->w_switchto = (channel_t *)NULL;
	achp->ch_flags &= ~CHN_ACTV;
	if (ws_procmode(wsp, achp) && force && !PTRACED(achp->ch_procp))
		psignal(achp->ch_procp, achp->ch_frsig);	/*XXX*/
	chp->ch_flags |= CHN_ACTV;
	(void) splx(oldpri);	/*XXX*/
	if (ws_procmode(wsp, chp) && !PTRACED(chp->ch_procp)) {
		wsp->w_noacquire++;
		psignal(chp->ch_procp, chp->ch_acqsig);	/*XXX*/
		chp->ch_timeid = timeout((void (*)())wsp->w_acq_refuse,
				(caddr_t)chp, 10*HZ);
	}
	/* if new vt is waiting to become active, then wake it up */
	if (chp->ch_waitactive) {
	    ws_iocack(OTHERQ(chp->ch_qp), chp->ch_waitactive,
		(struct iocblk *)(chp->ch_waitactive->b_rptr));
	    chp->ch_waitactive = 0;
	}
	return(1);
}

/*
 *
 */

int
ws_procmode(wstation_t *wsp, channel_t *chp)
{
	if (chp->ch_procp && !validproc(chp->ch_procp, chp->ch_pid))
		ws_automode(wsp, chp);
	return(CHNFLAG(chp, CHN_PROC));
}

/*
 *
 */

void
ws_automode(wstation_t *wsp, channel_t *chp)
{
	channel_t	*achp;
	struct map_info	*map_p = &wsp->w_map;

	achp = ws_activechan(wsp);
	if (chp == achp && map_p->m_procp && map_p->m_procp == chp->ch_procp) {
		if (!validproc(chp->ch_procp, chp->ch_pid)) {
			map_p->m_procp = (struct proc *)0;
			chp->ch_flags &= ~CHN_MAPPED;
			map_p->m_cnt = 0;
			map_p->m_chan = 0;
		}
	}
	chp->ch_procp = (struct proc *)NULL;
	chp->ch_pid = 0;
	chp->ch_flags &= ~CHN_PROC;
	chp->ch_relsig = SIGUSR1;
	chp->ch_acqsig = SIGUSR1;
	chp->ch_frsig = SIGUSR2;
}

/*
 * XXX This could/should be implemented as a "rep movw" kind of thing.
 */

#ifdef notdef
ws_xferwords(register ushort *srcp, register ushort *dstp, register int cnt, char dir)
{
	switch (dir) {
	case UP:
		while (cnt--)
			*dstp-- = *srcp--;
		break;
	default:
		while (cnt--)
			*dstp++ = *srcp++;
		break;
	}
}

/*
 *
 */
void
ws_setlock(wstation_t *wsp, int lock)
{
	if (lock)
		wsp->w_flags |= KD_LOCKED;
	else
		wsp->w_flags &= ~KD_LOCKED;
}
#endif

static void
ws_sigkill(wstation_t *wsp)
{
	int		chan;
	vidstate_t	vbuf;
	channel_t	*chp;
	struct map_info	*map_p;

	map_p = &wsp->w_map;
	chan = wsp->w_forcechan;
	if (wsp->w_forcetimeid && (wsp->w_active == chan)) {
		chp = (channel_t *) ws_getchan(wsp, chan);
		if (chp == NULL)
			return;
		bcopy((caddr_t)&chp->ch_vstate, (caddr_t)&vbuf, sizeof(vbuf));
		ws_chinit(wsp, chp, chan);
/* XXX_BSH This could cause the ttymon bug ???? */
		chp->ch_opencnt = 1;
		chp->ch_flags |= CHN_KILLED;
		if (map_p->m_procp && map_p->m_chan == chp->ch_id)
			bzero((caddr_t)map_p, sizeof(struct map_info));
		bcopy((caddr_t)&vbuf, (caddr_t)&chp->ch_vstate, sizeof(vbuf));
		chp->ch_vstate.v_cvmode = wsp->w_vstate.v_dvmode;
		wsp->w_forcetimeid = 0;
		wsp->w_forcechan = 0;
		tcl_reset(wsp, chp, &chp->ch_tstate);
		putnextctl1(chp->ch_qp, M_ERROR, ENXIO);
		if (chp->ch_nextp) {
			chp->ch_nextp->ch_prevp = chp->ch_prevp;
			ws_activate(wsp, chp->ch_nextp, VT_NOFORCE);
		}
		if (chp->ch_prevp)
			chp->ch_prevp->ch_nextp = chp->ch_nextp;
	}
}


static void
ws_noclose(wstation_t *wsp)
{
	int chan;

	chan = wsp->w_forcechan;
	if (wsp->w_forcetimeid && (wsp->w_active == chan)) {
		putnextctl1(wsp->w_chanpp[wsp->w_active]->ch_qp, M_PCSIG, SIGKILL);
		wsp->w_forcetimeid = timeout((void (*)())ws_sigkill,
				(caddr_t)wsp, 5*HZ);
	} else {
		wsp->w_forcetimeid = 0;
		wsp->w_forcechan = 0;
	}
}

void
ws_force(wstation_t *wsp, channel_t *chp)
{
	wsp->w_forcetimeid = timeout((void(*)())ws_noclose,
				(caddr_t)wsp, 10*HZ);
	wsp->w_forcechan = chp->ch_id;
	putnextctl1(chp->ch_qp, M_PCSIG, SIGINT);
	putnextctl(chp->ch_qp, M_HANGUP);
}

/*
 *
 */

void
ws_mctlmsg(queue_t *qp, mblk_t *mp)
{
	struct iocblk	*iocp;

	if (mp->b_wptr - mp->b_rptr != sizeof(struct iocblk)) {
		cmn_err(CE_NOTE, "!ws_mctlmsg: bad M_CTL msg");
		freemsg(mp);
		return;
	}
	if ((iocp = (struct iocblk *)mp->b_rptr)->ioc_cmd != MC_CANONQUERY) {
		/*
		 * ldterm insists on sending this to us; don't
		 * complain if it does
		 */
		if (iocp->ioc_cmd != EUC_WSET)
		    cmn_err(CE_NOTE,
			"!ws_mctlmsg: M_CTL msg not MC_CANONQUERY");
		freemsg(mp);
		return;
	}
	iocp->ioc_cmd = MC_DO_CANON;
	qreply(qp, mp);
}

/*
 *
 */

void
ws_notifyvtmon(channel_t *vtmchp, unchar ch)
{
	mblk_t	*mp;

	if (!(mp = allocb(sizeof(unchar) * 1, BPRI_MED))) {
		cmn_err(CE_NOTE, "!kdnotifyvtmon: can't get msg");
		return;
	}
	*mp->b_wptr++ = ch;
	putnext(vtmchp->ch_qp, mp);
}

/*
 *
 */

void
ws_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp)
{
	mblk_t	*tmp;

	mp->b_datap->db_type = M_IOCACK;
	if ((tmp = unlinkb(mp)) != (mblk_t *)NULL)
		freeb(tmp);
	iocp->ioc_count = iocp->ioc_error = 0;
	qreply(qp, mp);
}

/*
 *	Send an ioctl error response up the stream, eventually setting
 *	errno to 'error'.
 */

void
ws_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = -1;
	iocp->ioc_error = error;
	qreply(qp, mp);
}

/*
 *	Send an M_COPYIN message up the stream, requesting that 'size'
 *	bytes of data be copied in from the user address 'addr'.
 */

void
ws_copyin(queue_t *qp, mblk_t *mp, caddr_t addr, uint size,
    mblk_t *private)
{
	struct copyreq	*cqp;

	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	cqp->cq_addr = addr;
	cqp->cq_flag = 0;
	cqp->cq_private = private;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYIN;
	if (mp->b_cont) {
		freemsg(mp->b_cont);
		mp->b_cont = (mblk_t *)NULL;
	}
	qreply(qp, mp);
}


/*
 *	Send an M_COPYOUT message up the stream, requesting that 'size'
 *	bytes of data be copied out to the user address stored in the first
 *	continuation dblock.  The data to be copied is contained in the 'tmp'
 *	message block, which will be attached to the message sent upstream.
 */

void
ws_copyout(queue_t *qp, mblk_t *mp, mblk_t *tmp, uint size)
{
	struct copyreq	*cqp;

	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	cqp->cq_addr = (caddr_t)*(long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)1;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;
	if (mp->b_cont)
		freemsg(mp->b_cont);
	mp->b_cont = tmp;
	qreply(qp, mp);
}

/*
 *
 */

void
ws_mapavail(channel_t *chp, struct map_info *map_p)
{
	if (!map_p->m_procp) {
		chp->ch_flags &= ~CHN_MAPPED;
		return;
	}

	if (!validproc(map_p->m_procp, map_p->m_pid)) {
		map_p->m_procp = (struct proc *)0;
		map_p->m_pid = (pid_t) 0;
		chp->ch_flags &= ~CHN_MAPPED;
		map_p->m_cnt = 0;
		map_p->m_chan = 0;
	}
}





/*
 *  Check that the i/o port passed is in the list of allowable ports
 *  for the user to access.  This is called from kdvm_xenixdoio_bottom()
 *  in ../kdvm/kdvm.c to handle the xenix MCAIO, CGAIO, EGAIO, VGAIO,
 *  and CONSIO ioctls.
 */
int
ws_ck_kd_port(vidstate_t *vp, ushort port)
{
	register int	cnt;

	for (cnt = 0; cnt < MKDIOADDR; cnt++) {
		if (vp->v_ioaddrs[cnt] == port)
			return(1);
		if (!vp->v_ioaddrs[cnt])
			break;
	}
	return(0);
}


/*
 *
 */

#ifdef notdef
ws_winsz(queue_t *qp, mblk_t *mp, channel_t *chp, int cmd)
{
	vidstate_t *vp = &chp->ch_vstate;
	mblk_t *tmp;

	switch (cmd) {
	case TIOCGWINSZ: {
		struct winsize	*winp;

		tmp = allocb(sizeof(struct winsize), BPRI_MED);
		if (tmp == (mblk_t *)NULL) {
			cmn_err(CE_NOTE,
			    "!ws_winsz: can't get msg for reply to TIOCGWINSZ");
			freemsg(mp);
			break;
		}
		winp = (struct winsize *)tmp->b_rptr;
		winp->ws_row = (ushort)(WSCMODE(vp)->m_rows & 0xffff);
		winp->ws_col = (ushort)(WSCMODE(vp)->m_cols & 0xffff);
		winp->ws_xpixel = (ushort)(WSCMODE(vp)->m_xpels & 0xffff);
		winp->ws_ypixel = (ushort)(WSCMODE(vp)->m_ypels & 0xffff);
		tmp->b_wptr += sizeof(struct winsize);
		ws_copyout(qp, mp, tmp, sizeof(struct winsize));
		break;
	}
	case JWINSIZE: {
		struct jwinsize	*jwinp;

		tmp = allocb(sizeof(struct jwinsize), BPRI_MED);
		if (tmp == (mblk_t *)NULL) {
			cmn_err(CE_NOTE,
			    "!ws_winsz: can't get msg for reply to JWINSIZE");
			freemsg(mp);
			break;
		}
		jwinp = (struct jwinsize *)tmp->b_rptr;
		jwinp->bytesx = (char)(WSCMODE(vp)->m_cols & 0xff);
		jwinp->bytesy = (char)(WSCMODE(vp)->m_rows & 0xff);
		jwinp->bitsx = (short)(WSCMODE(vp)->m_xpels & 0xffff);
		jwinp->bitsy = (short)(WSCMODE(vp)->m_ypels & 0xffff);
		tmp->b_wptr += sizeof(struct jwinsize);
		ws_copyout(qp, mp, tmp, sizeof(struct jwinsize));
		break;
	}
	default:
		break;
	}
}
#endif

/*
 * WS routine for performing ioctls. Allows the mouse add-on to
 * be protected from cdevsw[] dependencies
 */

extern vnode_t *specfind();

int
ws_ioctl(dev_t dev, int cmd, int arg, int mode, cred_t *crp, int *rvalp)
{
	vnode_t *vp;
	int error;

	if (STREAMSTAB(getmajor(dev))) {
		vp = specfind(dev, VCHR); /* does a VN_HOLD on the vnode */
		if (vp == (vnode_t *) NULL)
			return (EINVAL);
		error = strioctl(vp, cmd, arg, mode, U_TO_K, crp, rvalp);
		VN_RELE(vp); /* lower reference count */
	} else
		error = (*devopsp[getmajor(dev)]->devo_cb_ops->cb_ioctl)
				  (dev, cmd, arg, mode, crp, rvalp);
	return error;
}

/*
 * Return (via two pointers to longs) the screen resolution for the
 * active channel.  For text modes, return the number of columns and
 * rows, for graphics modes, return the number of x and y pixels.
 */

void
ws_scrnres(ulong *xp, ulong *yp)
{
	vidstate_t *vp = &(ws_activechan(&Kdws)->ch_vstate);

	if (!WSCMODE(vp)->m_font) {	/* graphics mode */
		*xp = WSCMODE(vp)->m_xpels;
		*yp = WSCMODE(vp)->m_ypels;
	} else {			/* text mode */
		*xp = WSCMODE(vp)->m_cols;
		*yp = WSCMODE(vp)->m_rows;
	}
}


/* The following routines support COFF-based SCO applications that
 * use KD driver ioctls that overlap with STREAMS ioctls.
 */

extern unchar ws_compatflgs[];

void
ws_clrcompatflgs(dev_t dev)
{
	ws_compatflgs[getminor(dev)/8] &= ~(1 << (getminor(dev) % 8));
}

#ifdef notdef
void
ws_setcompatflgs(dev_t dev)
{
	ws_compatflgs[getminor(dev)/8] |= 1 << (getminor(dev) % 8);
}

int
ws_compatset(dev_t dev)
{
	return (ws_compatflgs[getminor(dev)/8] & (1 << (getminor(dev) % 8)));
}
#endif

extern int maxminor;

/*ARGSUSED*/
void
ws_initcompatflgs(dev_t dev)
{
	bzero((caddr_t)&ws_compatflgs[0], maxminor/8 + 1);
}
