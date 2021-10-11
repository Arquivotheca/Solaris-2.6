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

#pragma ident	"@(#)chanmux.c	1.21	96/06/05 SMI"

/*
 * IWE Channel Multiplexor
 * Multiplexes N secondary input devices (lower streams) across
 * M primary input/output channels (referred to as principal streams)
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/open.h>
#include <sys/termios.h>
#include <sys/file.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strtty.h>
#include <sys/kmem.h>
#include <sys/ws/chan.h>
#include <sys/chanmux.h>
#include <sys/cred.h>
#include <sys/cmn_err.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>


/* useful macro to make property lookup simple */
#define GET_INT_PROP(devi, pname, pval, plen) \
		(ddi_prop_op (DDI_DEV_T_ANY, (devi), PROP_LEN_AND_VAL_BUF, \
			DDI_PROP_DONTPASS, (pname), (caddr_t)(pval), (plen)))

#define	INHI	4096
#define	INLO	512
#define	OUTHI	512
#define	OUTLO	128



/* function prototypes */
int		cmux_allocchan(cmux_ws_t *, ulong_t);
static int	cmux_allocstrms(cmux_ws_t *, ulong_t);
static int	cmux_attach(dev_info_t *, ddi_attach_cmd_t);
static void	cmux_close_chan(cmux_ws_t *, cmux_lstrm_t *);
void		cmux_clr_ioc(cmux_ws_t *);
static void	cmux_do_iocresp(cmux_ws_t *);
cmux_t	       *cmux_findchan(cmux_ws_t *, clock_t);
int		cmux_foundit(clock_t, clock_t, clock_t);
static int	cmux_identify(dev_info_t *);
void		cmux_iocack(queue_t *, mblk_t *, struct iocblk *, int);
void		cmux_iocnak(queue_t *, mblk_t *, struct iocblk *, int, int);
static int	cmux_info(dev_info_t *, ddi_info_cmd_t, void *, void **);
int		cmux_initws(cmux_ws_t *, ulong_t);
int		cmux_mux_rput(queue_t *, mblk_t *);
int		cmux_mux_rsrv(queue_t *);
int		cmux_mux_wput(queue_t *, mblk_t *);
int		cmux_mux_wsrv(queue_t *);
int		cmux_openchan(queue_t *, cmux_ws_t *, ulong_t, dev_t, int);
int		cmux_realloc(ulong_t);
clock_t		cmux_striptime(mblk_t *);
void		cmux_switch_chan(cmux_ws_t *, ch_proto_t *);
static int	cmux_unlink(mblk_t *, cmux_t *);
int		cmux_up_rput(queue_t *, mblk_t *);
int		cmux_up_rsrv(queue_t *);
int		cmux_up_wput(queue_t *, mblk_t *);
int		cmux_up_wsrv(queue_t *);
int		cmuxclose(queue_t *, int, cred_t *);
int		cmuxopen(queue_t *, dev_t *, int, int, cred_t *);


/* external functions */
extern int	ws_getws();
extern int	ws_getchanno();
extern void	ws_clrcompatflgs();
extern void	ws_initcompatflgs();

extern int	kd_mmap();
extern int	nulldev();


char	_depends_on[] = "drv/kd";

struct module_info cmux_iinfo = {
	0, "cmux", 0, INFPSZ, INHI, INLO };


struct module_info cmux_oinfo = {
	0, "cmux", 0, CMUXPSZ, OUTHI, OUTLO };


struct qinit cmux_up_rinit = {
	cmux_up_rput, cmux_up_rsrv, cmuxopen, cmuxclose,
	NULL, &cmux_iinfo, NULL
};


struct qinit cmux_up_winit = {
	cmux_up_wput, cmux_up_wsrv, cmuxopen, cmuxclose,
	NULL, &cmux_oinfo, NULL
};

struct qinit cmux_mux_rinit = {
	cmux_mux_rput, cmux_mux_rsrv, nulldev, nulldev,
	NULL, &cmux_iinfo, NULL
};


struct qinit cmux_mux_winit = {
	cmux_mux_wput, cmux_mux_wsrv, nulldev, nulldev,
	NULL, &cmux_oinfo, NULL
};


struct streamtab cmux_str_info = {
	&cmux_up_rinit, &cmux_up_winit, &cmux_mux_rinit, &cmux_mux_winit
};



static dev_info_t *cmux_dip;	/* saved dev_info pointer */


#define	CMUX_MAX_UNITS	13	/* maximum number of vt's - 0 .. 12 */

/* How many chanmux units were configured?
 * (Done this way to also be tunable via /etc/system)
 */
static int	cmux_units = CMUX_MAX_UNITS;

cmux_ws_t	**wsbase;
unsigned long	numwsbase = 0;	/* number of workstations allocated */

#define	CMUX_CONF_FLAG	0

static 	struct cb_ops cb_cmux_ops = {
	nulldev,		/* cb_open - nulldev for STREAMS driver */
	nulldev,		/* cb_close - nulldev for STREAMS driver */
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
	&cmux_str_info,		/* cb_stream */
	(int)(CMUX_CONF_FLAG)	/* cb_flag */
};

struct dev_ops cmux_ops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	cmux_info,		/* devo_getinfo */
	cmux_identify,		/* devo_identify */
	nulldev,		/* devo_probe */
	cmux_attach,		/* devo_attach */
	nodev,			/* devo_detach */
	nodev,			/* devo_reset */
	&cb_cmux_ops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

/*
 * This is the loadable module wrapper.
 */
#include <sys/modctl.h>

extern struct mod_ops mod_driverops;
extern struct dev_ops cmux_ops;

/*
 * Module linkage information for the kernel.
 */

static struct modldrv modldrv = {
	&mod_driverops, /* Type of module.  This one is a pseudo driver */
	"VT channel multiplexor",
	&cmux_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,
	&modldrv,
	NULL
};


int
_init(void)
{
	return (mod_install(&modlinkage));
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

static int
cmux_identify(dev_info_t *devi)
{
	if (strcmp(ddi_get_name(devi), "chanmux") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

/*ARGSUSED*/
static int
cmux_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	char	name[12];
	int	unit, units, len;

	/* find out how many units we're expected to create */
	len = sizeof (units);
	units = cmux_units;		/* start with default value */

	if (GET_INT_PROP (devi, "units", &units, &len) == DDI_PROP_SUCCESS) {
		if (units < 1 || units > CMUX_MAX_UNITS) /* sanity check */
			units = CMUX_MAX_UNITS;
	}	/* else not specified - retain the default value */

	for (unit = 0; unit < units; unit++) {
		if (unit == 0)		/* unit 0 is "chanmux:chanmux" */
			sprintf(name, "chanmux");
		else			/* other units are "chanmux:#" */
			sprintf(name, "%d", unit);

		if (ddi_create_minor_node(devi, name, S_IFCHR, unit,
		    DDI_PSEUDO, 0) == DDI_FAILURE) {
			ddi_remove_minor_node(devi, NULL);
			return (-1);
		}
	}

	cmux_dip = devi;		/* save the dev_info pointer */
	cmux_units = units;		/* also remember how many units */
	return (DDI_SUCCESS);
}

/* ARGSUSED */
static int
cmux_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	register int error;

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (cmux_dip == NULL) {
			error = DDI_FAILURE;
		} else {
			*result = (void *) cmux_dip;
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



/*
 * cmux_realloc: allocate to the power of two greater than the
 * argument passed in: copy over old ptrs and NULL new ones out.
 * Return ENOMEM if kmem fails, 0 otherwise
 */

int
cmux_realloc(ulong_t wsno)
{
	cmux_ws_t **nwsbase, **owsbase;
	unsigned long pwr;
	int i;

	wsno++;		/* wsno counts from zero....... */
	if (wsno < numwsbase)
		return (0);

	/* make sure wsno is the closest power of 2, rounded up */
	for (pwr = 1; pwr < wsno; pwr <<= 1)
		;
	wsno = pwr;

	owsbase = wsbase;

	/* allocate new wsbase array */
	nwsbase = kmem_alloc(wsno * sizeof (cmux_ws_t *), KM_SLEEP);

	/* copy old wsbase to new */
	bcopy((caddr_t)wsbase, (caddr_t)nwsbase,
	    numwsbase * sizeof (cmux_ws_t *));

	/* zero out newly-added ws info */
	for (i = numwsbase; i < wsno; i++)
		nwsbase[i] = (cmux_ws_t *) NULL;

	wsbase = nwsbase;		/* move the pointers */

	/* free old wsbase data */
	kmem_free(owsbase, numwsbase * sizeof (cmux_ws_t *));

	numwsbase = wsno;		/* update count */

	return (0);
}

static int
cmux_allocstrms(cmux_ws_t *wsp, ulong_t numstreams)
{
	unsigned long size, onumlstrms;
	cmux_link_t *olstrmsp, *nlstrmsp;

	onumlstrms = wsp->w_numlstrms;
	numstreams = max(CMUX_STRMALLOC, (numstreams>>1) << 2);
	if (numstreams <= onumlstrms)
		return (0);	/* enough lower streams allocated */

	olstrmsp = wsp->w_lstrmsp;

	size = numstreams * sizeof (cmux_link_t);
	nlstrmsp = (cmux_link_t *) kmem_alloc(size, KM_NOSLEEP);
	if (nlstrmsp == (cmux_link_t *) NULL)
		return (ENOMEM);

	bzero((caddr_t)nlstrmsp, size);

	if (olstrmsp) {
		bcopy((caddr_t)olstrmsp, (caddr_t)nlstrmsp,
				onumlstrms * sizeof (cmux_link_t));
		wsp->w_lstrmsp = nlstrmsp;
		kmem_free(olstrmsp, onumlstrms * sizeof (cmux_link_t));
	} else
		wsp->w_lstrmsp = nlstrmsp;

	wsp->w_numlstrms = numstreams;

	return (0);
}


/*
 * allocate cmux_t pointers and cmux_link_t structs for numchan channels
 */

int
cmux_allocchan(cmux_ws_t *wsp, ulong_t numchan)
{
	unsigned long cmuxsz, princsz, onumchan;
	cmux_t **ocmuxpp, **ncmuxpp;
	cmux_link_t *oprincp, *nprincp;
	unsigned long pwr;

	onumchan = wsp->w_numchan;

	/* make sure numchan is the closest power of 2, rounded up */
	for (pwr = 1; pwr < numchan; pwr <<= 1)
		;

	numchan = max(CMUX_CHANALLOC, pwr);

	if (numchan <= onumchan)
		return (0);	/* already allocated enough space */

	ocmuxpp = wsp->w_cmuxpp;
	oprincp = wsp->w_princp;

	cmuxsz = numchan * sizeof (cmux_t *);
	ncmuxpp = kmem_alloc(cmuxsz, KM_SLEEP);

	princsz = numchan * sizeof (cmux_link_t);
	nprincp = kmem_alloc(princsz, KM_SLEEP);

	bzero((caddr_t)ncmuxpp, cmuxsz);
	bzero((caddr_t)nprincp, princsz);

#ifdef DEBUG1
	cmn_err(CE_NOTE,
	    "ncmuxpp %x ocmuxpp %x size %x onumchan %x",
	    ncmuxpp, ocmuxpp, cmuxsz, onumchan);
#endif
	if (ocmuxpp) {
		bcopy((caddr_t)ocmuxpp, (caddr_t)ncmuxpp,
					onumchan * sizeof (cmux_t *));
		wsp->w_cmuxpp = ncmuxpp;
		kmem_free(ocmuxpp, onumchan * sizeof (cmux_t *));
	} else
		wsp->w_cmuxpp = ncmuxpp;

	if (oprincp) {
		bcopy((caddr_t)oprincp, (caddr_t)nprincp,
		    onumchan * sizeof (cmux_link_t));
		wsp->w_princp = nprincp;
		kmem_free(oprincp, onumchan * sizeof (cmux_link_t));
	} else
		wsp->w_princp = nprincp;

	wsp->w_numchan = numchan;
	return (0);
}

/*
 * cmux_initws: allocate space for per-channel struct; clear flags;
 * Return ENOMEM if kmem fails.
 */

int
cmux_initws(cmux_ws_t *wsp, ulong_t numchan)
{
	int error = 0;
	struct cmux_swtch *switchp;

	if (error = cmux_allocchan(wsp, numchan))
		return (error);

	if (error = cmux_allocstrms(wsp, CMUX_STRMALLOC))
		return (error);

	wsp->w_numswitch = 1;
	switchp = &wsp->w_swtchtimes[0];
	drv_getparm(LBOLT, (unsigned long *)&switchp->sw_time);
	switchp->sw_chan = numchan - 1;
#ifdef DEBUG1
	cmn_err(CE_NOTE, "switch time is %x, switch chan is %x",
	    wsp->w_swtchtimes[0].sw_time, wsp->w_swtchtimes[0].sw_chan);
#endif
	return (0);
}

/*
 * cmux_openchan: allocate space for the new channel structure.
 * Make sure that pointers to ws struct and queues get set up.
 * Return an error number based on the success of allocation.
 * Send a ch_proto message indicating that a channel is opening
 * up.
 */

static int openflg = 0;

/*ARGSUSED4*/
int
cmux_openchan(queue_t *qp, cmux_ws_t *wsp, ulong_t chan, dev_t dev, int flag)
{
	int error;
	cmux_t *cmuxp;
	mblk_t *mp;
	ch_proto_t *protop;
	struct proc *procp;
	cmux_link_t *linkp;

	if (error = cmux_allocchan(wsp, chan+1))
		return (error);

	if (qp->q_ptr) {
#ifdef DEBUG1
		cmn_err(CE_WARN,
		    "chanmux: Invalid open state! Open fails.");
#endif
		return (ENXIO);
	}


	cmuxp = kmem_alloc(sizeof (cmux_t), KM_SLEEP);

	cmuxp->cmux_dev = dev;
	cmuxp->cmux_num = chan;
	cmuxp->cmux_wsp = wsp;
	cmuxp->cmux_rqp = qp;
	cmuxp->cmux_wqp = WR(qp);
	cmuxp->cmux_flg = CMUX_OPEN;
	wsp->w_cmuxpp[chan] = cmuxp;

	qp->q_ptr = (caddr_t) cmuxp;
	WR(qp)->q_ptr = (caddr_t) cmuxp;

	while ((mp = allocb(sizeof (ch_proto_t), BPRI_HI)) == NULL) {
		(void) bufcall(sizeof (ch_proto_t), BPRI_HI, wakeup,
			(long) &qp->q_ptr);
		(void) sleep((caddr_t)&qp->q_ptr, STIPRI);
	}

	mp->b_wptr += sizeof (ch_proto_t);
	mp->b_datap->db_type = M_PCPROTO;

	/* send a "Channel Open" request to the underlying driver (kd) */
	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHAN;
	protop->chp_stype_cmd = CH_CHANOPEN;

	drv_getparm(UPROCP, (unsigned long *)&procp);

	protop->chp_stype_arg = procp->p_ppid;
	protop->chp_chan = chan;

	/*
	 * Put it on our write queue to ship to the principal stream when
	 * it is opened.
	 */
	(void) putq(cmuxp->cmux_wqp, mp);
	linkp = wsp->w_princp + chan;

	openflg--;
	/* at this point in open, safe to allow another in */
	wakeup((caddr_t)&openflg);

	if (!linkp->cmlb_flg)
		return (0);	/* don't sleep on open until */
				/* mux is initialized */
#if	XXX
	linkp->cmlb_flg |= CMUX_PRINCSLEEP;

	if (sleep((caddr_t)&cmuxp->cmux_flg, STOPRI|PCATCH)) {
		linkp->cmlb_flg &= ~CMUX_PRINCSLEEP;
		wsp->w_cmuxpp[chan] = (cmux_t *) NULL;
		qp->q_ptr = (caddr_t) NULL;
		WR(qp)->q_ptr = (caddr_t) NULL;
		kmem_free(cmuxp, sizeof (cmux_t));
		return (EINTR);
	}
	linkp->cmlb_flg &= ~CMUX_PRINCSLEEP;
#endif

	qenable(RD(linkp->cmlb_lblk.l_qbot));

	if (linkp->cmlb_err) {
		wsp->w_cmuxpp[chan] = (cmux_t *) NULL;
		qp->q_ptr = (caddr_t) NULL;
		WR(qp)->q_ptr = (caddr_t) NULL;
		kmem_free(cmuxp, sizeof (cmux_t));
	}
	return (linkp->cmlb_err);
}

/*ARGSUSED4*/
int
cmuxopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	register unsigned long chan, wsno;
	register int minor_dev;
	cmux_ws_t *wsp;
	mblk_t *mp;
	struct stroptions *sop;
	int error = 0;

	if (sflag)
		return (EINVAL);	/* can't be opened as module */

	minor_dev = getminor(*devp);
	wsno = ws_getws(minor_dev);
	chan = ws_getchanno(minor_dev);

	if (chan >= cmux_units)		/* is unit completely out of range? */
		return ENXIO;

	if (qp->q_ptr) {
		cmux_t *cmuxp = (cmux_t *) qp->q_ptr;

		if (cmuxp->cmux_num != chan) {
#ifdef DEBUG1
			cmn_err(CE_WARN,
			    "chanmux: found q_ptr != chan in open; open fails");
#endif
			return (EINVAL);
		}

		if (cmuxp->cmux_flg & CMUX_CLOSE)
			return (EAGAIN);	/* prevent open during close */
		else
			return ((flag & FEXCL) ? EBUSY : 0);
	}


	/*
	 * If another open is in progress, check to see if FNONBLOCK or
	 * FNDELAY set; return EAGAIN if so.
	 */

	if (openflg && (flag & FNONBLOCK || flag & FNDELAY)) {
		return (EAGAIN);
	}

	/* sleep until noone else is in the midst of opening */
	while (openflg > 0)
		sleep((caddr_t)&openflg, TTIPRI);

	openflg++;

	if (wsno >= numwsbase)		/* need to grow wsbase? */
		error = cmux_realloc(wsno);

	if (error)
		goto openexit;

	wsp = wsbase[wsno];

	if (wsp == (cmux_ws_t *) NULL) {
		wsp = kmem_zalloc(sizeof (cmux_ws_t), KM_SLEEP);
		wsbase[wsno] = wsp;

		if (error = cmux_initws(wsp, chan+1))
			goto openexit;
	} /* wsp == NULL */

	/* open channel */


	/*
	 * openflg decrement and wakeup done in cmux_openchan if
	 * we reach here
	 */
	error = cmux_openchan(qp, wsp, chan, *devp, flag);
	if (error)
		return (error);

	/* allocate stroptions struct and indicate that stream is TTY */
	while ((mp = allocb(sizeof (struct stroptions), BPRI_HI)) == NULL) {
		(void) bufcall(sizeof (ch_proto_t), BPRI_HI, wakeup,
			(long) &qp->q_ptr);
		(void) sleep((caddr_t)&qp->q_ptr, STIPRI);
	}

	mp->b_datap->db_type = M_SETOPTS;
	mp->b_wptr += sizeof (struct stroptions);
	sop = (struct stroptions *) mp->b_rptr;
	sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
	sop->so_hiwat = INHI;
	sop->so_lowat = INLO;
	putnext(qp, mp);

	return (0);

openexit:
	openflg--;
	wakeup((caddr_t)&openflg);
	return (error);
}

/*ARGSUSED1*/
int
cmuxclose(queue_t *qp, int flag, cred_t *credp)
{
	queue_t	*q;
	cmux_t *cmuxp;
	cmux_ws_t *wsp;
	mblk_t *mp;
	ch_proto_t *protop;
	cmux_link_t *linkp;

	cmuxp = (cmux_t *) qp->q_ptr;

	if (cmuxp == (cmux_t *) NULL) {
#ifdef DEBUG1
		cmn_err(CE_WARN,
		    "chanmux: finding invalid q_ptr in cmuxclose");
#endif
		return (ENXIO);
	}

	cmuxp->cmux_flg |= CMUX_CLOSE;

	/*
	 * check to see if principal stream linked underneath.
	 * If not, flush the queues and return
	 */

	wsp = cmuxp->cmux_wsp;
	linkp = wsp->w_princp + cmuxp->cmux_num;
	if (!linkp->cmlb_flg) {
		flushq(qp, FLUSHALL);
		flushq(WR(qp), FLUSHALL);
		return (0);
	}

	/*
	 * principal stream linked underneath. Allocate "channel closing"
	 * message and ship it to principal stream. It should respond
	 * with a "channel closed" message.
	 */

	while ((mp = allocb(sizeof (ch_proto_t), BPRI_HI)) == NULL) {
		(void) bufcall(sizeof (ch_proto_t), BPRI_HI, wakeup,
			(long) &qp->q_ptr);
		(void) sleep((caddr_t)&qp->q_ptr, STIPRI);
	}

	/*
	 * want this to be last message received on the channel,
	 * so make it normal priority so that it doesn't get ahead of
	 * user data
	 */

	mp->b_wptr += sizeof (ch_proto_t);
	mp->b_datap->db_type = M_PROTO;

	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHAN;
	protop->chp_stype_cmd = CH_CHANCLOSE;
	protop->chp_chan = cmuxp->cmux_num;

	q = cmuxp->cmux_wqp;

	/*
	 * put it on our write queue to ship to
	 * principal stream
	 */
	(void) putq(q, mp);
	(void) cmux_up_wsrv(q);

#ifdef DONT_INCLUDE
	/* Put this sleep back when the new code base is ported. */
	cmuxp->cmux_flg |= CMUX_WCLOSE;		/* waiting for close */
	while (cmuxp->cmux_flg & CMUX_WCLOSE)
		sleep((caddr_t)cmuxp, PZERO+1);
#endif

#ifdef DEBUG1
	cmn_err(CE_WARN,
	    "chanmux: woken up from close on channel %d", cmuxp->cmux_num);
#endif
	qp->q_ptr = NULL;
	WR(qp)->q_ptr = NULL;
	ws_clrcompatflgs(cmuxp->cmux_dev);

	/*
	 * release the channel.
	 */

	wsp->w_cmuxpp[cmuxp->cmux_num] = (cmux_t *) NULL;
	kmem_free(cmuxp, sizeof (cmux_t));
	return (0);
}


static int
cmux_unlink(mblk_t *mp, cmux_t *cmuxp)
{
	register cmux_ws_t *wsp;
	cmux_link_t *linkp;
	struct linkblk *ulinkbp;
	int i;

	wsp = cmuxp->cmux_wsp;
	ulinkbp = (struct linkblk *) mp->b_cont->b_rptr;

	linkp = wsp->w_princp + cmuxp->cmux_num;
#ifdef DEBUG1
	cmn_err(CE_NOTE, "In cmux_unlink. ");
#endif
	if (linkp->cmlb_lblk.l_index == ulinkbp->l_index) {
		kmem_free(linkp->cmlb_lblk.l_qbot->q_ptr,
		    sizeof (cmux_lstrm_t));
		linkp->cmlb_lblk.l_qbot->q_ptr = NULL;
		RD(linkp->cmlb_lblk.l_qbot)->q_ptr = NULL;
		bzero((caddr_t)linkp, sizeof (cmux_link_t));
#ifdef DEBUG1
		cmn_err(CE_NOTE, "unlinked principal stream. ");
#endif
		return (1);
	}

	linkp = wsp->w_lstrmsp;
	for (i = 0; i < wsp->w_numlstrms; i++, linkp++)
		if ((linkp->cmlb_flg) &&
		    (linkp->cmlb_lblk.l_index == ulinkbp->l_index)) {
			kmem_free(linkp->cmlb_lblk.l_qbot->q_ptr,
			    sizeof (cmux_lstrm_t));
			linkp->cmlb_lblk.l_qbot->q_ptr = NULL;
			RD(linkp->cmlb_lblk.l_qbot)->q_ptr = NULL;
			bzero((caddr_t)linkp, sizeof (cmux_link_t));
			wsp->w_lstrms--;
#ifdef DEBUG1
			cmn_err(CE_NOTE, "unlinked secondary stream. ");
#endif
			return (1);
		}

	return (0);
}


/*ARGSUSED*/
static void
cmux_close_chan(cmux_ws_t *wsp, cmux_lstrm_t *lstrmp)
{
#ifdef DONT_INCLUDE
	unsigned long chan;
	cmux_t *cmuxp;

	chan = lstrmp->lstrm_id;
	cmuxp = wsp->w_cmuxpp[chan];
	if (!cmuxp) {
#ifdef DEBUG1
		cmn_err(CE_WARN, "Found null cmuxp; do not wakeup");
#endif
		return;
	}

	cmuxp->cmux_flg &= ~CMUX_WCLOSE;
#ifdef DEBUG1
	cmn_err(CE_NOTE, "Close on channel %d", cmuxp->cmux_num);
#endif
	wakeup((caddr_t)cmuxp);
#ifdef DEBUG1
	cmn_err(CE_NOTE, "Called wakeup channel %d", cmuxp->cmux_num);
#endif
#endif
}


clock_t
cmux_striptime(register mblk_t *mp)
{
	register ch_proto_t *protop;

	protop = (ch_proto_t *) mp->b_rptr;
	return (protop->chp_tstmp);
}


int
cmux_foundit(clock_t mintime, clock_t maxtime, clock_t timeval)
{
	if (maxtime >= mintime)		/* no wrap */
		return (timeval <= maxtime && timeval >= mintime);
	else
		return (!(timeval < mintime && timeval > maxtime));
}


/*
 * 	100  <-- most recent switch
 *	 30
 *	930
 *	700  <-- last switch
 */

cmux_t *
cmux_findchan(cmux_ws_t *wsp, clock_t timestamp)
{
	struct cmux_swtch *switchp;
	clock_t curtime, mintime;
	int found, cnt;

	drv_getparm(LBOLT, (unsigned long *)&curtime);
	switchp = &wsp->w_swtchtimes[wsp->w_numswitch - 1];

	if (!cmux_foundit(switchp->sw_time, curtime, timestamp))
		return (NULL); /* chanmux will drop the message */

	switchp = &wsp->w_swtchtimes[0];
	cnt = 0;
	mintime = switchp->sw_time;
	found = cmux_foundit(mintime, curtime, timestamp);
	while ((cnt < wsp->w_numswitch - 1) && !found) {
		switchp++;
		cnt++;
		curtime = mintime;
		mintime = switchp->sw_time;
		found = cmux_foundit(mintime, curtime, timestamp);
	}

	if (!found)
		return (NULL);

	return (wsp->w_cmuxpp[switchp->sw_chan]);
}


void
cmux_clr_ioc(register cmux_ws_t *wsp)
{
	register cmux_link_t *linkp;
	register cmux_t *cmuxp;
	int i;

#ifdef DEBUG1
	cmn_err(CE_WARN, "In cmux_clrioc:");
#endif
	if (wsp->w_iocmsg) freemsg(wsp->w_iocmsg);
	wsp->w_iocmsg = (mblk_t *) NULL;

	linkp = wsp->w_princp + wsp->w_ioctlchan;
	linkp->cmlb_iocresp = 0;
	if (linkp->cmlb_iocmsg)
		freemsg(linkp->cmlb_iocmsg);

	for (i = 0, linkp = wsp->w_lstrmsp; i < wsp->w_numlstrms; i++) {
		linkp->cmlb_iocresp = 0;
		if (linkp->cmlb_iocmsg)
			freemsg(linkp->cmlb_iocmsg);
		linkp++;
	}

	wsp->w_ioctlcnt = wsp->w_ioctllstrm = 0;
	wsp->w_ioctlchan = 0;
	wsp->w_state &= ~CMUX_IOCTL;

	for (i = 0; i < wsp->w_numchan; i++) {
		cmuxp = wsp->w_cmuxpp[i];
		if (cmuxp && cmuxp->cmux_wqp)
			qenable(cmuxp->cmux_wqp);
	}
}

void
cmux_switch_chan(register cmux_ws_t *wsp, register ch_proto_t *protop)
{
	int i;
	struct cmux_swtch *switchp;
	unsigned long chan;

	wsp->w_numswitch = min(wsp->w_numswitch + 1, CMUX_NUMSWTCH);
	chan = protop->chp_chan;

	if (!wsp->w_cmuxpp[chan]) { /* invalid channel request? */
		cmn_err(CE_PANIC,
		    "invalid channel switch request %x", (int)chan);
	}

#ifdef DEBUG1
	cmn_err(CE_NOTE, "switch_chan: switching to channel %x", chan);
#endif
	/* must be a good channel. Update switchtime list */
	for (i = CMUX_NUMSWTCH - 1; i > 0; i--) {
#ifdef DEBUG1
		cmn_err(CE_WARN, "wsp %x, i %x, i-1 %x", wsp,
		    &wsp->w_swtchtimes[i], &wsp->w_swtchtimes[i-1]);
#endif
		bcopy((caddr_t)&wsp->w_swtchtimes[i-1],
		    (caddr_t)&wsp->w_swtchtimes[i],
		    sizeof (struct cmux_swtch));
	}
	switchp = &wsp->w_swtchtimes[0];
#ifdef DEBUG1
	cmn_err(CE_WARN, "wsp %x, switchp %x", wsp, switchp);
#endif
	switchp->sw_chan = protop->chp_chan;
	switchp->sw_time = protop->chp_tstmp;
}


/* STATIC */
void
cmux_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mblk_t	*tmp;

	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	if ((tmp = unlinkb(mp)) != (mblk_t *)NULL)
		freeb(tmp);
	qreply(qp, mp);
}


/* STATIC */
void
cmux_iocnak(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error, int rval)
{
	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp, mp);
}


static void
cmux_do_iocresp(cmux_ws_t *wsp)
{
	cmux_link_t *linkp;
	unsigned long ackcnt, nakcnt, acknum, naknum, i;
	struct iocblk *iocp;
	struct copyresp *resp;
	cmux_t *cmuxp;
	mblk_t *mp;

	ackcnt = nakcnt = 0;
	acknum = naknum = 0;
	cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];

	linkp = wsp->w_princp + wsp->w_ioctlchan;
	iocp = (struct iocblk *) linkp->cmlb_iocmsg->b_rptr;

	if (linkp->cmlb_iocresp == M_IOCACK)
		ackcnt++;
	else if (linkp->cmlb_iocresp == M_IOCNAK)
		nakcnt++;

	linkp = wsp->w_lstrmsp;
	for (i = 1; i <= wsp->w_numlstrms; i++, linkp++)
		if (linkp->cmlb_iocresp == M_IOCACK) {
			ackcnt++;
			acknum = i;
		} else if (linkp->cmlb_iocresp == M_IOCNAK) {
			nakcnt++;
			iocp = (struct iocblk *) linkp->cmlb_iocmsg->b_rptr;
			if (iocp->ioc_error != 0)
				naknum = i;
		}

	if (ackcnt == 0) {		/* failure all around */
#ifdef DEBUG1
		cmn_err(CE_NOTE, "cmux_do_iocresp: everyone nackd");
#endif
		if (naknum == 0) {
			linkp = wsp->w_princp + wsp->w_ioctlchan;
			putnext(cmuxp->cmux_rqp, linkp->cmlb_iocmsg);
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
		} else {
			linkp = wsp->w_lstrmsp + naknum -1;
			putnext(cmuxp->cmux_rqp, linkp->cmlb_iocmsg);
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
		}
		cmux_clr_ioc(wsp);
	} else if (ackcnt == 1) {	/* success! */
#ifdef DEBUG1
		cmn_err(CE_NOTE, "cmux_do_iocresp: only one ack");
#endif
		if (acknum == 0) {
			linkp = wsp->w_princp + wsp->w_ioctlchan;
		} else {
			linkp = wsp->w_lstrmsp + acknum -1;
		}
#ifdef DEBUG1
		cmn_err(CE_NOTE, "cmux_do_iocresp: about to call putnext");
#endif
		if (linkp->cmlb_iocmsg->b_datap->db_type != M_IOCACK) {
			putnext(cmuxp->cmux_rqp, linkp->cmlb_iocmsg);
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
			wsp->w_ioctllstrm = acknum;
		} else {
			putnext(cmuxp->cmux_rqp, linkp->cmlb_iocmsg);
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
			cmux_clr_ioc(wsp);
		}
	} else {
		/*
		 * multiple acks. Send up an M_IOCNAK with errno set
		 * to EACCES, and fail each M_COPYIN/M_COPYOUT that was
		 * passed up by sending down an M_IOCDATA message with
		 * ioc_rval set to EACCES.
		 */

#ifdef DEBUG1
		cmn_err(CE_NOTE, "cmux_do_iocresp: multiple acks");
#endif
		linkp = wsp->w_princp + wsp->w_ioctlchan;
		mp = linkp->cmlb_iocmsg;

		if (mp->b_datap->db_type == M_COPYIN ||
		    mp->b_datap->db_type == M_COPYOUT) {
			resp = (struct copyresp *) mp->b_rptr;
			resp->cp_rval = (caddr_t) EACCES;
			mp->b_datap->db_type = M_IOCDATA;
			putnext(linkp->cmlb_lblk.l_qbot, mp);
			linkp->cmlb_iocmsg = (mblk_t *) NULL;
		}

		linkp = wsp->w_lstrmsp;
		for (i = 1; i <= wsp->w_numlstrms; i++, linkp++) {
			if (!linkp->cmlb_flg)
				continue;
			mp = linkp->cmlb_iocmsg;
			if (mp->b_datap->db_type == M_COPYIN ||
			    mp->b_datap->db_type == M_COPYOUT) {
				resp = (struct copyresp *) mp->b_rptr;
				resp->cp_rval = (caddr_t) EACCES;
				mp->b_datap->db_type = M_IOCDATA;
				putnext(linkp->cmlb_lblk.l_qbot, mp);
				linkp->cmlb_iocmsg = (mblk_t *) NULL;
			}
		}

		mp = wsp->w_iocmsg;
		mp->b_datap->db_type = M_IOCNAK;
		iocp = (struct iocblk *) mp->b_rptr;
		iocp->ioc_error = EACCES;
		putnext(cmuxp->cmux_rqp, mp);
		wsp->w_iocmsg = (mblk_t *) NULL;
		cmux_clr_ioc(wsp);
	} /* multiple acks */
}


/*
 * this routine will only get called when a canput from a lower
 * stream to the q_next of this queue failed.
 */
int
cmux_up_rsrv(queue_t *qp)
{
	cmux_t *cmuxp;
	cmux_ws_t *wsp;
	unsigned long i;
	cmux_link_t *linkp;

	cmuxp = (cmux_t *) qp->q_ptr;
	if (cmuxp == (cmux_t *) NULL) {
#ifdef DEBUG1
		cmn_err(CE_WARN, "chanmux: Invalid q_ptr in up_rsrv");
#endif
		return (0);
	}

	wsp = cmuxp->cmux_wsp;

	/*
	 * qenable all secondary lower streams and the principal
	 * stream associated with channel
	 */

	linkp = wsp->w_lstrmsp;	/* do secondary streams first */

#ifdef DEBUG1
	if (linkp == (cmux_link_t *) NULL) {
		cmn_err(CE_WARN, "chanmux: NULL lstrms ptr in up_rsrv");
		return (0);
	}
#endif

	for (i = 0; i < wsp->w_numlstrms; i++, linkp++) {
		if (!linkp->cmlb_flg)
			continue;
		if (linkp->cmlb_lblk.l_qbot)
			qenable(RD(linkp->cmlb_lblk.l_qbot));
	}

	/* now enable principal stream */
#ifdef DEBUG1
	if (linkp == (cmux_link_t *) NULL) {
		cmn_err(CE_WARN,
			"chanmux: NULL princstrms ptr in up_rsrv");
		return (0);
	}
#endif
	linkp = wsp->w_princp + cmuxp->cmux_num;
	if (linkp->cmlb_lblk.l_qbot)
		qenable(RD(linkp->cmlb_lblk.l_qbot));

	return (0);
}


/*
 * service routine for all messages going downstream.  Do not service
 * any messages until a principal stream is linked below the channel,
 * nor while an ioctl is being processed on a channel different from
 * ours.
 * When the service routine is activated and we find an M_IOCTL
 * message, mark that this channel is servicing an ioctl.
 * Perform a copymsg of the message for each secondary stream
 * and ship the messages to the principal stream for the channel as well
 * as all secondary streams.
 * For M_FLUSH handling, only flush the principal stream.
 */

int
cmux_up_wsrv(queue_t *qp)
{
	mblk_t *mp;
	cmux_t *cmuxp;
	cmux_ws_t *wsp;
	cmux_link_t *linkp;
	unsigned long chan;
	int i;
	ch_proto_t *protop;

	cmuxp = (cmux_t *) qp->q_ptr;
	if (cmuxp == NULL) {
#ifdef DEBUG1
		cmn_err(CE_WARN, "chanmux: Invalid q_ptr in up_wsrv");
#endif
		return (0);
	}

	wsp = cmuxp->cmux_wsp;
	chan = cmuxp->cmux_num;
	linkp = wsp->w_princp + chan;
	if (!linkp->cmlb_flg)
		return (0);

	if ((wsp->w_state & CMUX_IOCTL) && (wsp->w_ioctlchan != chan)) {
#ifdef DEBUG1
		cmn_err(CE_WARN, "chanmux: blocked on ioctl");
#endif
		return (0);
	}
	/*
	 * keep getting messages until none left or we honor
	 * flow control and see that the stream above us is blocked
	 * or are set to enqueue messages while an ioctl is processed
	 */

	while ((mp = getq(qp)) != NULL) {
		switch (mp->b_datap->db_type) {

		case M_FLUSH:
			/*
			 * Flush everything we haven't looked at yet.
			 * Turn the message around if FLUSHR was set
			 */
			if (*mp->b_rptr & FLUSHW) {
				flushq(qp, FLUSHDATA);
				*mp->b_rptr &= ~FLUSHW;
			}
			if (*mp->b_rptr & FLUSHR)
				putnext(RD(qp), mp);
			else
				freemsg(mp);
			continue;

		case M_IOCDATA:
			if (!(wsp->w_state & CMUX_IOCTL) ||
			    wsp->w_ioctlchan != chan) {
#ifdef DEBUG1
				cmn_err(CE_WARN,
				    "unexpected M_IOCDATA msg; freeing it");
#endif
				freemsg(mp);
				continue;
			}

			if (wsp->w_ioctllstrm == 0)
				putnext(linkp->cmlb_lblk.l_qbot, mp);
			else {
				cmux_link_t *nlinkp;
				nlinkp = wsp->w_lstrmsp + wsp->w_ioctllstrm - 1;
				putnext(nlinkp->cmlb_lblk.l_qbot, mp);
			}

			continue;

		case M_IOCTL: {

			/*
			 * we could not have gotten in here if
			 * an ioctl was in process, on this
			 * stream or any other (STREAMS protects
			 * against multiple ioctls on the same
			 * stream, and we protect against multiple
			 * ioctls on different streams)
			 */

			struct iocblk *iocp;
			cmux_link_t *nlinkp;

			iocp = (struct iocblk *) mp->b_rptr;
			if (iocp->ioc_cmd == I_PUNLINK ||
			    iocp->ioc_cmd == I_UNLINK)
				if (cmux_unlink(mp, cmuxp)) {
#ifdef DEBUG1
					cmn_err(CE_NOTE, "unlinking cmux");
#endif
					cmux_iocack(qp, mp, iocp, 0);

					/*
					 * explicitly enable queue before
					 * returning so message processing
					 * can continue. We return rather
					 * than continue because we need to
					 * reset state
					 */
					qenable(qp);
					return (0);
				}
			if (iocp->ioc_cmd == CH_CHRMAP) {
				freemsg(mp);
				if ((mp = allocb(sizeof (ch_proto_t),
				    BPRI_HI)) == NULL) {
					cmn_err(CE_WARN,
					    "chanmux: unable to send CHRMAP");
					return (0);
				}

				mp->b_wptr += sizeof (ch_proto_t);
				mp->b_datap->db_type = M_PCPROTO;

				protop = (ch_proto_t *) mp->b_rptr;
				protop->chp_type = CH_CTL;
				protop->chp_stype = CH_CHAN;
				protop->chp_stype_cmd = CH_CHRMAP;
				protop->chp_chan = chan;
			}

#ifdef DEBUG1
			cmn_err(CE_WARN,
			    "cmuxioctl: ioctl %x starting", iocp->ioc_cmd);
#endif
			wsp->w_state |= CMUX_IOCTL;
			wsp->w_ioctlchan = chan;
			wsp->w_ioctlcnt = 1 + wsp->w_lstrms;

			/*
			 * ship copies of message to secondary streams
			 * adjust ioctlcnt so that if message copy
			 * fails, we aren't waiting for a response
			 * that will never come
			 */
			nlinkp = wsp->w_lstrmsp;
			for (i = 0; i < wsp->w_numlstrms; i++, nlinkp++) {
				if (!nlinkp->cmlb_flg)
					continue;
				nlinkp->cmlb_iocresp = 0;
				nlinkp->cmlb_iocmsg = copymsg(mp);
				if (nlinkp->cmlb_iocmsg)
					putnext(nlinkp->cmlb_lblk.l_qbot,
					    nlinkp->cmlb_iocmsg);
				else
					wsp->w_ioctlcnt -= 1;
			}

			/* ship message to principal stream */
			nlinkp = wsp->w_princp + chan;
			nlinkp->cmlb_iocresp = 0;
			nlinkp->cmlb_iocmsg = copymsg(mp);
			if (nlinkp->cmlb_iocmsg)
				putnext(nlinkp->cmlb_lblk.l_qbot,
				    nlinkp->cmlb_iocmsg);
			else
				wsp->w_ioctlcnt -= 1;
			wsp->w_iocmsg = mp;
			continue;
		} /* M_IOCTL */

		default:
			if (mp->b_datap->db_type <= QPCTL &&
			    !canputnext(linkp->cmlb_lblk.l_qbot)) {
				(void) putbq(qp, mp);
				return (0);	/* read side is blocked */
			}

			putnext(linkp->cmlb_lblk.l_qbot, mp);
			continue;
		}
	}
	return (0);
}



/*ARGSUSED*/
int
cmux_up_rput(queue_t *qp, mblk_t *mp)
{
	/* should not be called */

	freemsg(mp);
#ifdef DEBUG1
	cmn_err(CE_WARN, "chanmux: up_rput called");
#endif
	return (0);
}


/*
 * if non-priority messages are put before a principal stream has
 * been linked under, free them. For non-ioctl priority messages,
 * enqueue them, and for non-I_LINK/I_PLINK ioctls, NACK them.
 */

int
cmux_up_wput(queue_t *qp, mblk_t *mp)
{
	cmux_t *cmuxp;
	struct iocblk *iocp;
	cmux_ws_t *wsp;
	cmux_link_t *linkp;
	cmux_lstrm_t *lstrmp;
	int error, i;

	cmuxp = (cmux_t *) qp->q_ptr;
	if (cmuxp == NULL) {
#ifdef DEBUG1
		cmn_err(CE_WARN, "chanmux: Invalid q_ptr in up_wput");
#endif
		freemsg(mp);
		return (0);
	}

	wsp = cmuxp->cmux_wsp;
	linkp = wsp->w_princp + cmuxp->cmux_num;

	if (mp->b_datap->db_type < QPCTL && mp->b_datap->db_type != M_IOCTL) {
		if (!linkp->cmlb_flg)
			freemsg(mp);
		else
			(void) putq(qp, mp);
		return (0);
	}

	if (mp->b_datap->db_type != M_IOCTL) {
		(void) putq(qp, mp);
		return (0);
	}

	iocp = (struct iocblk *) mp->b_rptr;

	if (iocp->ioc_cmd != I_LINK && iocp->ioc_cmd != I_PLINK) {
#ifdef DEBUG1
		cmn_err(CE_NOTE, "up_wput: Have an ioctl");
#endif
		if (!linkp->cmlb_flg)
			cmux_iocnak(qp, mp, iocp, EAGAIN, -1);
		else
			(void) putq(qp, mp);
		return (0);
	}

	lstrmp = kmem_alloc(sizeof (cmux_lstrm_t), KM_NOSLEEP);
	if (lstrmp == (cmux_lstrm_t *) NULL) {
		cmux_iocnak(qp, mp, iocp, EAGAIN, -1);
		return (0);
	}

	if (linkp->cmlb_flg) {
		/* add secondary stream to set of lower streams */

		if (error = cmux_allocstrms(wsp, ++wsp->w_lstrms)) {
			wsp->w_lstrms--;
			kmem_free(lstrmp, sizeof (cmux_lstrm_t));
			cmux_iocnak(qp, mp, iocp, error, -1);
			return (0);
		}


		linkp = wsp->w_lstrmsp;
		for (i = 0; i < wsp->w_numlstrms; i++, linkp++)
			if (!linkp->cmlb_flg)
				break;

		/* now i is the first free link_t struct found */
		lstrmp->lstrm_wsp = wsp;
		lstrmp->lstrm_flg = CMUX_SECSTRM;
		lstrmp->lstrm_id = i;

		bcopy((caddr_t)mp->b_cont->b_rptr, (caddr_t)&linkp->cmlb_lblk,
		    sizeof (struct linkblk));
		linkp->cmlb_flg = CMUX_SECSTRM;
		linkp->cmlb_lblk.l_qbot->q_ptr = (caddr_t) lstrmp;
		RD(linkp->cmlb_lblk.l_qbot)->q_ptr = (caddr_t) lstrmp;
		cmux_iocack(qp, mp, iocp, 0);
		return (0);
	}

	lstrmp->lstrm_wsp = wsp;
	lstrmp->lstrm_flg = CMUX_PRINCSTRM;
	lstrmp->lstrm_id = cmuxp->cmux_num;
	bcopy((caddr_t)mp->b_cont->b_rptr, (caddr_t)&linkp->cmlb_lblk,
	    sizeof (struct linkblk));
	linkp->cmlb_flg = CMUX_PRINCSTRM;
	linkp->cmlb_lblk.l_qbot->q_ptr = (caddr_t) lstrmp;
	RD(linkp->cmlb_lblk.l_qbot)->q_ptr = (caddr_t) lstrmp;
	cmux_iocack(qp, mp, iocp, 0);

	/* enable processing on queue now that there is an active channel */
	qenable(qp);
	return (0);
}

/*
 * This routine will be invoked by flow control when the queue below
 * it is enabled because a canput() from up_wsrv() on the queue failed
 * This routine, if invoked on a principal stream linked below,
 * will enable only the upper write queue associated with the
 * principal stream. If invoked on a secondary stream, any of the
 * upper streams above could be the culprit, so enable them all.
 */

int
cmux_mux_wsrv(queue_t *qp)
{
	cmux_lstrm_t *lstrmp;
	cmux_ws_t *wsp;
	cmux_t *cmuxp;
	unsigned long i;

	lstrmp = (cmux_lstrm_t *) qp->q_ptr;

	if (lstrmp == NULL) {
#ifdef DEBUG1
		cmn_err(CE_WARN, "chanmux: Invalid q_ptr in mux_wsrv");
#endif
		return (0);
	}

	wsp = (cmux_ws_t *)lstrmp->lstrm_wsp;
	if (lstrmp->lstrm_flg & CMUX_PRINCSTRM) {
		/* this is the principal stream */
		cmuxp = wsp->w_cmuxpp[lstrmp->lstrm_id];
		if (cmuxp == NULL) {
#ifdef DEBUG1
			cmn_err(CE_WARN, "chanmux: Invalid cmuxp in mux_wsrv");
#endif
			return (0);
		}
		if (cmuxp->cmux_flg & CMUX_OPEN)
			qenable(cmuxp->cmux_wqp);
	} else {
		for (i = 0; i <= wsp->w_numchan; i++) {
			cmuxp = wsp->w_cmuxpp[i];
			if ((cmuxp == NULL) || !(cmuxp->cmux_flg & CMUX_OPEN))
				continue;
			qenable(cmuxp->cmux_wqp);
		}
	}

	return (0);
}

#define	IOCTL_TYPE(type)	((type == M_COPYIN)|| \
				(type == M_COPYOUT)|| \
				(type == M_IOCACK)||  \
				(type == M_IOCNAK))
/*
 * cmux_mux_rsrv is the service routine of all input messages from the lower
 * streams. For normal messages from principal streams, forward
 * directly to the associated upper stream, if it exists, otherwise
 * discard the message. Normal messages from the lower streams should
 * be timestamped with an M_PROTO header for non-priority messages,
 * M_PCPROTO for priority messages. Send the message to the channel
 * that was active in the range given by the timestamp. If the
 * channel was closed, drop the message on the floor.
 *
 * If the message is ioctl-related, make note of the
 * response in the cmux_link_t structure for the lower stream
 * and update the count of waiting responses. When zero, check
 * all STREAMS. If exactly 1 ack (M_IOCACK, M_COPYIN, M_COPYOUT) was
 * sent up, we are in good shape. If more than one ack was sent up,
 * NACK the ioctl, and send M_IOCDATA messages to all lower streams
 * requesting M_COPYINs/M_COPYOUTs.
 *
 * For the switch channel command message from the principal stream,
 * update the list of most recently active channels and its count.
 * Upon receipt of the "channel close acknowledge" message,
 * wakeup the process sleeping in the close.
 */

int
cmux_mux_rsrv(queue_t *qp)
{
	cmux_lstrm_t *lstrmp;
	mblk_t *mp;
	unsigned long princflg;
	time_t timestamp;
	cmux_t *cmuxp;
	ch_proto_t *protop;
	cmux_link_t *linkp;
	cmux_ws_t *wsp;

	lstrmp = (cmux_lstrm_t *) qp->q_ptr;
	if (lstrmp == (cmux_lstrm_t *) NULL) {
#ifdef DEBUG1
		cmn_err(CE_WARN, "chanmux: Invalid q_ptr in mux_rsrv");
#endif
		return (0);
	}

#ifdef DEBUG1
	if (!lstrmp->lstrm_flg) {
		cmn_err(CE_WARN, "chanmux: Invalid q_ptr in mux_rsrv");
		return (0);
	}
#endif
	wsp = lstrmp->lstrm_wsp;
	if (lstrmp->lstrm_flg & CMUX_PRINCSTRM) {
		princflg = 1;
		linkp = wsp->w_princp + lstrmp->lstrm_id;
	} else {
		princflg = 0;
		linkp = wsp->w_lstrmsp + lstrmp->lstrm_id;
	}

	while ((mp = getq(qp)) != NULL) {
		if (IOCTL_TYPE(mp->b_datap->db_type)) {
			goto msgproc;
		}

		if (!princflg) {
			/*
			 * message is from lower stream and should have
			 * header indicating timestamp.
			 */
			if ((mp->b_wptr - mp->b_rptr) != sizeof (ch_proto_t)) {
#ifdef DEBUG1
				cmn_err(CE_WARN,
				    "chanmux: illegal lower stream protocol "
				    "in mux_rsrv");
#endif
				freemsg(mp);
				continue;
			}
			timestamp = cmux_striptime(mp);
			cmuxp = cmux_findchan(wsp, timestamp);
			if (cmuxp == NULL) {
#ifdef DEBUG1
				cmn_err(CE_WARN,
				    "chanmux: illegal cmuxp found in mux_rsrv");
#endif
				freemsg(mp);
				continue;
			}
			if (mp->b_datap->db_type < QPCTL &&
			    !canputnext(cmuxp->cmux_rqp)) {
				(void) putbq(qp, mp);
				return (0);
			}
		} else {
			cmuxp = wsp->w_cmuxpp[lstrmp->lstrm_id];
			if (cmuxp == NULL) {
#ifdef DEBUG1
				cmn_err(CE_NOTE,
				    "did not find cmuxp; id %d %x",
				    lstrmp->lstrm_id, wsp->w_cmuxpp);
#endif
				freemsg(mp);
				continue;
			}
			if (mp->b_datap->db_type < QPCTL &&
			    !canputnext(cmuxp->cmux_rqp)) {
				(void) putbq(qp, mp);
				return (0);
			}
		}

msgproc:
		switch (mp->b_datap->db_type) {

		case M_FLUSH:
			if (*mp->b_rptr & FLUSHR) {
				flushq(qp, FLUSHDATA);
				*mp->b_rptr &= ~FLUSHR;
			}
			if (*mp->b_rptr & FLUSHW) {
				/* nothing to flush on the lower write side */
				qreply(qp, mp);
			} else
				freemsg(mp);
			continue;

		case M_IOCACK:
			/*
			 * ioctl handling. differentiate between waiting for
			 * ack and received ack. This treats M_COPYIN/M_COPYOUT
			 * messages differently.
			 */
#ifdef DEBUG1
			cmn_err(CE_NOTE, "Found M_IOCACK on queue in rsrv");
#endif
			if (wsp->w_ioctlcnt) {
				linkp->cmlb_iocresp = M_IOCACK;
				linkp->cmlb_iocmsg = mp;
#ifdef DEBUG1
				cmn_err(CE_NOTE, "ioctlcnt > 0");
#endif
				if (--wsp->w_ioctlcnt == 0)
					cmux_do_iocresp(wsp);
				continue;
			}
			/*
			 * ioctlcnt == 0 means that this messages
			 * comes after a M_COPYIN/M_COPYOUT.
			 * In this, send the message up to
			 * the next read queue
			 */

			cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
			putnext(cmuxp->cmux_rqp, mp);
			cmux_clr_ioc(wsp);
			continue;

		case M_IOCNAK:
#ifdef DEBUG1
			cmn_err(CE_NOTE, "Found M_IOCNAK on queue in rsrv");
#endif
			if (wsp->w_ioctlcnt) {
				linkp->cmlb_iocresp = M_IOCNAK;
				linkp->cmlb_iocmsg = mp;
				if (--wsp->w_ioctlcnt == 0)
					cmux_do_iocresp(wsp);
				continue;
			}
			/*
			 * ioctlcnt == 0 means that this message
			 * comes after M_COPYIN/M_COPYOUT.
			 * Send the message up to the next read queue
			 */

			cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
			putnext(cmuxp->cmux_rqp, mp);
			cmux_clr_ioc(wsp);
			continue;

		case M_COPYIN:
			if (wsp->w_ioctlcnt) {
				linkp->cmlb_iocresp = M_IOCACK;
				linkp->cmlb_iocmsg = mp;
				if (--wsp->w_ioctlcnt == 0)
					cmux_do_iocresp(wsp);
				continue;
			}
			/*
			 * ioctlcnt == 0 means that this message
			 * comes after another M_COPYIN/M_COPYOUT.
			 * Send the message up to the next read queue
			 */

			cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
			putnext(cmuxp->cmux_rqp, mp);
			continue;

		case M_COPYOUT:
			if (wsp->w_ioctlcnt) {
				linkp->cmlb_iocresp = M_IOCACK;
				linkp->cmlb_iocmsg = mp;
				if (--wsp->w_ioctlcnt == 0)
					cmux_do_iocresp(wsp);
				continue;
			}
			/*
			 * ioctlcnt == 0 means that this message
			 * comes after another M_COPYIN/M_COPYOUT.
			 * Send the message up to the next read queue
			 */

			cmuxp = wsp->w_cmuxpp[wsp->w_ioctlchan];
			putnext(cmuxp->cmux_rqp, mp);
			continue;

		case M_PCPROTO:
		case M_PROTO:
			/* check for "close ack" and "switch channel" */

			/* can it possibly be a "ch_proto_t" message? */
			if ((mp->b_wptr - mp->b_rptr) != sizeof (ch_proto_t))
				putnext(cmuxp->cmux_rqp, mp);	/* no */

			protop = (ch_proto_t *) mp->b_rptr;
			if (princflg && (protop->chp_type != CH_CTL ||
			    protop->chp_stype != CH_PRINC_STRM)) {
				if (cmuxp == NULL) {
					freemsg(mp);
					continue;
				}
				putnext(cmuxp->cmux_rqp, mp);
			} else if (princflg) {
				/* potentially a command for us */
				switch (protop->chp_stype_cmd) {

				case CH_CHANGE_CHAN:
					cmux_switch_chan(wsp, protop);
					freemsg(mp);	/* free msg */
					continue;

				case CH_OPEN_RESP:
					linkp->cmlb_err = protop->chp_stype_arg;
					freemsg(mp);

					if (linkp->cmlb_flg & CMUX_PRINCSLEEP) {
						wakeup(
						    (caddr_t)&cmuxp->cmux_flg);

						/* q will be enabled */
						return (0);
					}

					/* only if we were not sleeping */
					/* in open */
					continue;

				case CH_CLOSE_ACK:
#ifdef DEBUG1
					cmn_err(CE_WARN, "Found close_ack");
#endif
					cmux_close_chan(wsp, lstrmp);
					continue;

				default:
					putnext(cmuxp->cmux_rqp, mp);
					continue;
				} /* switch */

			} else { /* no CH_* protocol with lower streams */
				putnext(cmuxp->cmux_rqp, mp);
			}
			continue;

		default:
			putnext(cmuxp->cmux_rqp, mp);
			continue;

		} /* switch */
	} /* while */
	return (0);
}


int
cmux_mux_rput(queue_t *qp, mblk_t *mp)
{
	(void) putq(qp, mp);
	return (0);
}


/*ARGSUSED*/
int
cmux_mux_wput(queue_t *qp, mblk_t *mp)
{
	/* should not be called */

	freemsg(mp);
#ifdef DEBUG1
	cmn_err(CE_WARN, "chanmux: mux_wput called");
#endif
	return (0);
}
