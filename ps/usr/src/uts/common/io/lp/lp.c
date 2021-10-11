/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.  All Rights Reserved.
 */

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved					*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)lp.c	1.22	96/02/13 SMI"

/*
 *	LP (Line Printer / parallel port) Driver	EUC handling version
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/termio.h>
#include <sys/termios.h>
#include <sys/cmn_err.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strtty.h>
#include <sys/debug.h>
#include <sys/eucioctl.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/bpp_io.h>
#include <sys/promif.h>
#include <sys/lp.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

/*
 * debug control
 */
static int lperrmask = LPEM_ALL;
static int lperrlevel = LPEP_LMAX;

static void *lp_state_head;		/* opaque handle top of state structs */
static kmutex_t lplock;
static int lp_max_unit = -1;

#define	MAXTIME	 2			/* 2 sec */
static char	lptmrun = 0;		/* timer set up? */
int timeflag = 0;

/*
 * The following variable controls how many times we try to get non-busy
 * status in the interrupt handler. See lpintr().
 */
unsigned int lp_busy_check_retries = 100;

#ifdef MERGE386

extern int	merge386enable;

#endif /* MERGE386 */


static int lpopen(queue_t *q, dev_t *devp, int flag, register sflag,
    cred_t *cred_p);
static int lpclose(queue_t *q, int flag, cred_t *cred_p);
static int lpoput(queue_t *q, mblk_t *bp);

static void lpdelay(struct lp_unit *lpp);
static void lpgetoblk(struct lp_unit *lpp);
static void lpputioc(queue_t *q, mblk_t *bp);
static void lpproc(struct lp_unit *lpp, int cmd);
static void lptimeout(caddr_t arg);
static void lpflush(struct lp_unit *lpp, int cmd);
static void lpxintr(struct lp_unit *lpp);
static void lpsrvioc(queue_t *q, mblk_t *bp);
static u_int lpintr(caddr_t arg);

struct module_info lpinfo = {
	/* id, name, min pkt siz, max pkt siz, hi water, low water */
	42, "lp", 0, INFPSZ, 256, 128
};
static struct qinit lp_rint = {
	putq, NULL, lpopen, lpclose, NULL, &lpinfo, NULL
};
static struct qinit lp_wint = {
	lpoput, NULL, lpopen, lpclose, NULL, &lpinfo, NULL
};
struct streamtab lp_str_info = {
	&lp_rint, &lp_wint, NULL, NULL
};

static int lp_getinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static int lp_identify(dev_info_t *);
static int lp_probe(dev_info_t *);
static int lp_attach(dev_info_t *, ddi_attach_cmd_t);
static int lp_detach(dev_info_t *, ddi_detach_cmd_t);

static unsigned char get_error_status(unsigned char);

#define	LP_CONF_FLAG	0

DDI_DEFINE_STREAM_OPS(\
	lp_ops,		\
	lp_identify,	\
	lp_probe,	\
	lp_attach,	\
	lp_detach,	\
	nodev,		\
	lp_getinfo,	\
	LP_CONF_FLAG,	\
	&lp_str_info	\
);

#define	getsoftc(unit) \
	((struct lp_unit *)ddi_get_soft_state(lp_state_head, (unit)))


/*
 * This is the loadable module wrapper.
 */
#include <sys/modctl.h>

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module  (driver) */
	"lp driver v1.22",
	&lp_ops,		/* driver ops */
};

static struct modlinkage modlinkage = {
    MODREV_1, &modldrv, NULL
};

int
_init(void)
{
	int	error;

	if ((error = ddi_soft_state_init(&lp_state_head,
	    sizeof (struct lp_unit), 0)) != 0) {
		return (error);
	}
	mutex_init(&lplock, "lplock", MUTEX_DRIVER, NULL);
	if ((error = mod_install(&modlinkage)) != 0) {
	    mutex_destroy(&lplock);
	    ddi_soft_state_fini(&lp_state_head);
	}
	return (error);
}

int
_fini(void)
{
	int error;

	if ((error = mod_remove(&modlinkage)) != 0)
		return (error);
	mutex_destroy(&lplock);
	ddi_soft_state_fini(&lp_state_head);
	return (error);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}


static int
lp_identify(dev_info_t *dip)
{
	if (strcmp(ddi_get_name(dip), "lp") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

static int
lp_probe(dev_info_t *dip)
{
	ushort	testval;
	int	debug[2];
	int	ioaddr;
	int	len;

	len = sizeof (debug);
	if (ddi_prop_op(DDI_DEV_T_ANY, dip, PROP_LEN_AND_VAL_BUF,
	    DDI_PROP_DONTPASS, "debug", (caddr_t)debug, &len) ==
	    DDI_PROP_SUCCESS) {
		lperrlevel = debug[0];
		lperrmask = debug[1];
	}
	LPERRPRINT(LPEP_L3, LPEM_MODS, ("lp_probe: dip=0x%x\n", dip));

	len = sizeof (int);
	if (ddi_prop_op(DDI_DEV_T_NONE, dip, PROP_LEN_AND_VAL_BUF,
	    DDI_PROP_DONTPASS, "ioaddr", (caddr_t)&ioaddr, &len)
	    != DDI_PROP_SUCCESS)
		return (DDI_PROBE_FAILURE);

	/* Probe for the board. */
	outb(ioaddr + LP_DATA, 0x55);
	testval = ((short)inb(ioaddr + LP_DATA) & 0xFF);

	if (testval != 0x55)
		return (DDI_PROBE_FAILURE);

	return (DDI_PROBE_SUCCESS);
}

static int
lp_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	struct lp_unit	*lpp;
	int	ioaddr;
	int	len;
	int	unit_no;

	LPERRPRINT(LPEP_L3, LPEM_MODS, ("lp_attach: dip=0x%x\n", dip));

	unit_no = ddi_get_instance(dip);

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	len = sizeof (int);
	if (ddi_prop_op(DDI_DEV_T_NONE, dip, PROP_LEN_AND_VAL_BUF,
	    DDI_PROP_DONTPASS, "ioaddr", (caddr_t)&ioaddr, &len)
	    != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);

	if (ddi_soft_state_zalloc(lp_state_head, unit_no) != 0)
		return (DDI_FAILURE);
	mutex_enter(&lplock);
	if (unit_no > lp_max_unit)
		lp_max_unit = unit_no;
	mutex_exit(&lplock);
	lpp = getsoftc(unit_no);

	lpp->flag = LPPRES;		/* controller present */
	lpp->lp_dip = dip;
	lpp->data = ioaddr + LP_DATA;
	lpp->status = ioaddr + LP_STATUS;
	lpp->control = ioaddr + LP_CONTROL;

	ddi_set_driver_private(dip, (caddr_t)lpp);

	if (ddi_create_minor_node(dip, "", S_IFCHR, unit_no, NULL, NULL)
	    == DDI_FAILURE) {
		ddi_remove_minor_node(dip, NULL);
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);
	return (DDI_SUCCESS);
}

static int
lp_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int	unit_no;

	LPERRPRINT(LPEP_L3, LPEM_MODS, ("lp_detach: dip=0x%x\n", dip));

	if (cmd != DDI_DETACH)
		return (DDI_FAILURE);

	unit_no = ddi_get_instance(dip);

	/* Remove the minor node created in attach */
	ddi_remove_minor_node(dip, NULL);

	/* Free the memory allocated for this unit's state struct */
	ddi_soft_state_free(lp_state_head, unit_no);

	ddi_set_driver_private(dip, NULL);

	return (DDI_SUCCESS);
}

static int
lp_getinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, void **result)
{
	struct lp_unit	*lpp;
	register int error;

	LPERRPRINT(LPEP_L2, LPEM_MODS, ("lp_getinfo: dip=0x%x\n", dip));

	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if ((lpp = getsoftc((dev_t)arg)) == NULL) {
			*result = NULL;
			error = DDI_FAILURE;
		} else {
			mutex_enter(&lpp->lp_lock);
			*result = (void *)lpp->lp_dip;
			mutex_exit(&lpp->lp_lock);
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *) getminor((dev_t) arg);
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}


/*ARGSUSED2*/
static int
lpopen(queue_t *q, dev_t *devp, int flag, register sflag, struct cred *cred_p)
{
	char	name[MAXNAMELEN];
	register struct stroptions *sop;
	register struct strtty *tp;
	struct lp_unit	*lpp;
	mblk_t *mop;
	int unit_no;

	LPERRPRINT(LPEP_L2, LPEM_OPEN,
	    ("lpopen: qp=0x%x devp=0x%x\n", q, devp));

	unit_no = getminor(*devp);
	if (!(lpp = getsoftc(unit_no)))
		return (ENXIO);

#ifdef MERGE386  /* Provide exclusive access to device for MERGE */

	if (merge386enable) {
		if ((lpp->flag & OPEN) == 0)
			if (!vm86_portalloc(lpp->data, lpp->control)) {
				return (EBUSY);
			}
	}

#endif /* MERGE386 */

	if (lpp->flag & OPEN)
		return (EBUSY);

	/*
	 * Avoid toggling reset to avoid losing front-panel settings, but
	 * make sure we're in SELECT state
	 */

	outb(lpp->control, SEL | RESET);

	if (!(mop = allocb(sizeof (struct stroptions), BPRI_MED)))
		return (EAGAIN);

	if (ddi_add_intr(lpp->lp_dip, (u_int) 0, &lpp->lp_iblock,
	    (ddi_idevice_cookie_t *)0, lpintr, (caddr_t)lpp) !=
	    DDI_SUCCESS) {
		cmn_err(CE_WARN, "lp: cannot add intr for lp%d\n", unit_no);
		ddi_remove_minor_node(lpp->lp_dip, NULL);
		freeb(mop);
		return (ENXIO);
	}
	sprintf(name, "lp%dlock", unit_no);
	mutex_init(&lpp->lp_lock, name, MUTEX_DRIVER, (void *)lpp->lp_iblock);


	tp = &lpp->lp_tty;
	q->q_ptr = (caddr_t) lpp;
	WR(q)->q_ptr = (caddr_t) lpp;
	tp->t_rdqp = q;
	tp->t_dev = unit_no;

	mop->b_datap->db_type = M_SETOPTS;
	mop->b_wptr += sizeof (struct stroptions);
	sop = (struct stroptions *)mop->b_rptr;
	sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
	sop->so_hiwat = 512;
	sop->so_lowat = 256;
	putnext(q, mop);

	if ((tp->t_state & (ISOPEN | WOPEN)) == 0) {
		tp->t_iflag = IGNPAR;
		tp->t_cflag = B300 | CS8 | CLOCAL;
		if (!lptmrun) {
			lpp->last_time = 0;
			lptimeout(NULL);
		}
		lptmrun++;
	}
	tp->t_state &= ~WOPEN;
	tp->t_state |=  CARR_ON | ISOPEN;
	lpp->flag |= OPEN;

	return (0);
}


/*ARGSUSED2*/
static int
lpclose(queue_t *q, int flag, cred_t *cred_p)
{
	register struct strtty *tp;
	struct lp_unit	*lpp;

	LPERRPRINT(LPEP_L2, LPEM_CLOS,
	    ("lpclose: qp=0x%x flag=0x%x\n", q, flag));

	lpp = (struct lp_unit *)q->q_ptr;
	tp = &lpp->lp_tty;

	if (!(tp->t_state & ISOPEN)) {	/* See if it's closed already */
		return (0);
	}

	if (!(flag & (FNDELAY | FNONBLOCK))) {
		/* Drain queued output to the printer. */
		while ((tp->t_state & CARR_ON)) {
			if ((tp->t_out.bu_bp == 0) && (WR(q)->q_first == NULL))
				break;
			tp->t_state |= TTIOW;
			if (sleep((caddr_t) & tp->t_oflag, TTIPRI | PCATCH)) {
				tp->t_state &= ~(TTIOW | CARR_ON);
				break;
			}
		}
	}

	/*
	 * do not >>> outb(lpp->control, 0) -- because
	 * close() gets called before all characters are sent, therefore,
	 * the last chars do not get output with the interrupt turned off
	 */
	if ((--lptmrun) == 0)
		(void) untimeout(timeflag);
	lpp->flag &= ~OPEN;

	ddi_remove_intr(lpp->lp_dip, 0, lpp->lp_iblock);
	mutex_destroy(&lpp->lp_lock);

	tp->t_state &= ~(ISOPEN | CARR_ON);

#ifdef MERGE386

	if (merge386enable)
		vm86_portfree(lpp->data, lpp->control);

#endif /* MERGE386 */

	tp->t_rdqp = NULL;
	q->q_ptr = WR(q)->q_ptr = NULL;
	return (0);
}


static int
lpoput(queue_t *q, mblk_t *bp)
{
	register struct msgb *bp1;
	register struct strtty *tp;
	struct lp_unit	*lpp;

	LPERRPRINT(LPEP_L1, LPEM_OPUT, ("lpoput: qp=0x%x bp=0x%x\n", q, bp));

	lpp = (struct lp_unit *)q->q_ptr;
	tp = &lpp->lp_tty;

	switch (bp->b_datap->db_type) {

	case M_DATA:
		while (bp) {
			bp->b_datap->db_type = M_DATA;
			bp1 = unlinkb(bp);
			bp->b_cont = NULL;
			if ((bp->b_wptr - bp->b_rptr) <= 0) {
				freeb(bp);
			} else {
				(void) putq(q, bp);
			}
			bp = bp1;
		}
		if (q->q_first) {
			lpgetoblk(lpp);
		}
		break;

	case M_IOCTL:
		lpputioc(q, bp);
		if (q->q_first) {
			lpgetoblk(lpp);
		}
		break;

	case M_IOCDATA:
	{
		struct copyresp *csp;
		struct iocblk *iocbp;

		csp = (struct copyresp *)bp->b_rptr;

		if (csp->cp_cmd != BPPIOC_GETERR) {
			break;
		}

		if (csp->cp_rval) {
			freemsg(bp);
			break;
		}

		bp->b_datap->db_type = M_IOCACK;
		bp->b_wptr = bp->b_rptr + sizeof (struct iocblk);

		iocbp = (struct iocblk *)bp->b_rptr;
		iocbp->ioc_error = 0;
		iocbp->ioc_count = 0;
		iocbp->ioc_rval  = 0;

		qreply(q, bp);
		break;
	}

	case M_DELAY:
		if ((tp->t_state & TIMEOUT) || q->q_first || tp->t_out.bu_bp) {
			(void) putq(q, bp);
			break;
		}
		tp->t_state |= TIMEOUT;
		(void) timeout(lpdelay, (caddr_t)lpp,
		    ((int)*(bp->b_rptr)) * HZ / 60);
		freemsg(bp);
		break;

	case M_FLUSH:
		switch (*(bp->b_rptr)) {

		case FLUSHRW:
			lpflush(lpp, (FREAD | FWRITE));
			*(bp->b_rptr) = FLUSHR;
			qreply(q, bp);
			break;

		case FLUSHR:
			lpflush(lpp, FREAD);
			qreply(q, bp);
			break;

		case FLUSHW:
			lpflush(lpp, FWRITE);
			freemsg(bp);
			break;

		default:
			break;
		}
		break;

	case M_START:
		lpproc(lpp, T_RESUME);
		freemsg(bp);
		break;

	case M_STOP:
		lpproc(lpp, T_SUSPEND);
		freemsg(bp);
		break;

	default:
		freemsg(bp);
		break;
	}
	return (0);
}

static void
lpgetoblk(struct lp_unit *lpp)
{
	register struct strtty *tp;
	register struct queue *q;
	register struct msgb    *bp;

	LPERRPRINT(LPEP_L1, LPEM_GOBK, ("lpgetoblk: lpp=0x%x\n", lpp));

	tp = &lpp->lp_tty;
	if (tp->t_rdqp == NULL) {
		return;
	}
	q = WR(tp->t_rdqp);

	while (!(tp->t_state & BUSY) && (bp = getq(q))) {

		switch (bp->b_datap->db_type) {

		case M_DATA:
			if (tp->t_state & (TTSTOP | TIMEOUT)) {
				(void) putbq(q, bp);
				return;
			}

			/* start output processing for bp */
			tp->t_out.bu_bp = bp;
			lpproc(lpp, T_OUTPUT);
			break;

		case M_DELAY:
			if (tp->t_state & TIMEOUT) {
				(void) putbq(q, bp);
				return;
			}
			tp->t_state |= TIMEOUT;
			(void) timeout(lpdelay, (caddr_t)lpp,
			    ((int)*(bp->b_rptr)) * HZ / 60);
			freemsg(bp);
			break;

		case M_IOCTL:
			lpsrvioc(q, bp);
			break;

		default:
			freemsg(bp);
			break;
		}
	}
	/* Wakeup any process sleeping waiting for drain to complete */
	if ((tp->t_out.bu_bp == 0) && (tp->t_state & TTIOW)) {
		tp->t_state &= ~(TTIOW);
		wakeup((caddr_t) &tp->t_oflag);
	}
}

/*
 * ioctl handler for output PUT procedure
 */
static void
lpputioc(queue_t *q, mblk_t *bp)
{
	struct strtty *tp;
	struct iocblk *iocbp;
	struct lp_unit	*lpp;
	mblk_t *bp1;

	iocbp = (struct iocblk *)bp->b_rptr;
	lpp = (struct lp_unit *)q->q_ptr;
	tp = &lpp->lp_tty;

	LPERRPRINT(LPEP_L1, LPEM_IOCT,
	    ("lpputioc: qp=0x%x bp=0x%x ioctl=0x%x\n", q, bp, iocbp->ioc_cmd));

	switch (iocbp->ioc_cmd) {

	case TCSBRK:
	case TCSETAW:
	case TCSETSW:
	case TCSETSF:
	case TCSETAF:	/* run these now, if possible */

		if (q->q_first || (tp->t_state & (BUSY | TIMEOUT)) ||
		    tp->t_out.bu_bp) {
			(void) putq(q, bp);
			break;
		}
		lpsrvioc(q, bp);
		break;

	case TCSETS:
	case TCSETA:    /* immediate parm set   */

		if (tp->t_state & BUSY) {
			(void) putbq(q, bp);	/* queue these for later */
			break;
		}
		lpsrvioc(q, bp);
		break;

	case TCGETS:
	case TCGETA:    /* immediate parm retrieve */
		lpsrvioc(q, bp);
		break;

	case EUC_MSAVE:
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case BPPIOC_GETERR:
	{

		struct copyreq *cqp;
		struct bpp_error_status *bpp_status;

		cqp = (struct copyreq *)bp->b_rptr;
		cqp->cq_size = sizeof (struct bpp_error_status);
		cqp->cq_addr = (caddr_t)*(long *)bp->b_cont->b_rptr;
		cqp->cq_flag = 0;

		if (bp->b_cont) {
			freemsg(bp->b_cont);
			bp->b_cont = NULL;
		}

		bp->b_cont = allocb(sizeof (struct bpp_error_status), BPRI_MED);

		if (bp->b_cont == NULL) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EAGAIN;
			qreply(q, bp);
			break;
		}

		bpp_status = (struct bpp_error_status *)bp->b_cont->b_rptr;
		bpp_status->timeout_occurred = 0;
		bpp_status->bus_error = 0;
		bpp_status->pin_status = get_error_status(inb(lpp->status));

		bp->b_cont->b_wptr = bp->b_cont->b_rptr +
			sizeof (struct bpp_error_status);

		bp->b_datap->db_type = M_COPYOUT;
		bp->b_wptr = bp->b_rptr + sizeof (struct copyreq);

		qreply(q, bp);
		break;
	}

	case BPPIOC_TESTIO:
		if ((inb(lpp->status) & (UNBUSY|ONLINE)) == (UNBUSY|ONLINE)) {
			iocbp->ioc_rval = 0;
		} else {
			iocbp->ioc_error = EIO;
			iocbp->ioc_rval = -1;
		}

		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	default:
		if ((iocbp->ioc_cmd&IOCTYPE) == LDIOC) {
			bp->b_datap->db_type = M_IOCACK; /* ignore LDIOC cmds */
			bp1 = unlinkb(bp);
			if (bp1) {
				freeb(bp1);
			}
			iocbp->ioc_count = 0;
		} else {
			/*
			 * Unknown IOCTLs aren't errors, they just may have
			 * been intended for an upper module that isn't
			 * present.  NAK them...
			 */
			iocbp->ioc_error = EINVAL;
			iocbp->ioc_rval = (-1);
			bp->b_datap->db_type = M_IOCNAK;
		}
		qreply(q, bp);
		break;
	}
}

/*
 * Ioctl processor for queued ioctl messages.
 *
 */
static void
lpsrvioc(queue_t *q, mblk_t *bp)
{
	struct strtty *tp;
	struct iocblk *iocbp;
	struct termio  *cb;
	struct termios *scb;
	struct lp_unit	*lpp;
	mblk_t *bpr;
	mblk_t *bp1;

	iocbp = (struct iocblk *)bp->b_rptr;
	lpp = (struct lp_unit *)q->q_ptr;
	tp = &lpp->lp_tty;

	LPERRPRINT(LPEP_L0, LPEM_IOCT,
	    ("lpsrvioc: qp=0x%x bp=0x%x ioctl=0x%x\n", q, bp, iocbp->ioc_cmd));

	switch (iocbp->ioc_cmd) {

	case TCSETSF: /* The output has drained now. */
		lpflush(lpp, FREAD);
		/* (couldn't get block before...) */
		/*FALLTHROUGH*/
	case TCSETS:
	case TCSETSW:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			qreply(q, bp);
			break;
		}

		scb = (struct termios *)bp->b_cont->b_rptr;
		tp->t_cflag = scb->c_cflag;
		tp->t_iflag = scb->c_iflag;
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1) {
			freeb(bp1);
		}
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case TCSETAF: /* The output has drained now. */
		lpflush(lpp, FREAD);
		/*FALLTHROUGH*/
	case TCSETA:
	case TCSETAW:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			qreply(q, bp);
			break;
		}
		cb = (struct termio *)bp->b_cont->b_rptr;
		tp->t_cflag = cb->c_cflag;
		tp->t_iflag = cb->c_iflag;
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1) {
			freeb(bp1);
		}
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case TCGETS:    /* immediate parm retrieve */
		if (bp->b_cont)
			freemsg(bp->b_cont);

		if ((bpr = allocb(sizeof (struct termios), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void) putbq(q, bp);
			bufcall((ushort)sizeof (struct termios), BPRI_MED,
					(void(*)())lpgetoblk, (long)lpp);
			return;
		}
		bp->b_cont = bpr;

		scb = (struct termios *)bp->b_cont->b_rptr;

		scb->c_iflag = tp->t_iflag;
		scb->c_cflag = tp->t_cflag;

		bp->b_cont->b_wptr += sizeof (struct termios);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof (struct termios);
		qreply(q, bp);
		break;

	case TCGETA:    /* immediate parm retrieve */
		if (bp->b_cont)
			freemsg(bp); /* bad user supplied parameter */

		if ((bpr = allocb(sizeof (struct termio), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void) putbq(q, bp);
			bufcall((ushort)sizeof (struct termio), BPRI_MED,
					(void(*)())lpgetoblk, (long)lpp);
			return;
		}
		bp->b_cont = bpr;
		cb = (struct termio *)bp->b_cont->b_rptr;

		cb->c_iflag = tp->t_iflag;
		cb->c_cflag = tp->t_cflag;

		bp->b_cont->b_wptr += sizeof (struct termio);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof (struct termio);
		qreply(q, bp);
		break;

	case TCSBRK:
		/* Skip the break since it's a parallel port. */
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1) {
			freeb(bp1);
		}
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	case EUC_MSAVE: /* put these here just in case... */
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;

	default: /* unexpected ioctl type */
		if (canput(RD(q)->q_next) == 1) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			qreply(q, bp);
		} else {
			(void) putbq(q, bp);
		}
		break;
	}
}

static void
lpflush(struct lp_unit *lpp, int cmd)
{
	register struct strtty *tp;
	queue_t *q;

	LPERRPRINT(LPEP_L1, LPEM_FLSH,
	    ("lpflush: lpp=0x%x cmd=0x%x\n", lpp, cmd));

	tp = &lpp->lp_tty;

	if (cmd & FWRITE) {
		q = WR(tp->t_rdqp);
		/* Discard all messages on the output queue. */
		flushq(q, FLUSHDATA);
		tp->t_state &= ~(BUSY | TBLOCK);
		if (tp->t_state & TTIOW) {
			tp->t_state ^= TTIOW;
			wakeup((caddr_t) &tp->t_oflag);
		}

	}
	if (cmd & FREAD) {
		tp->t_state &= ~(BUSY);
	}
	lpgetoblk(lpp);
}


/*
 * lpintr is the entry point for all interrupts.
 */
static u_int
lpintr(caddr_t arg)
{
	register unsigned char  status;
	struct lp_unit *lpp = (struct lp_unit *)arg;
	unsigned int statusloop = 0;

	if (!(lpp->flag & OPEN)) {
		return (DDI_INTR_UNCLAIMED);
	}

	/*
	 * Many (most?) printers are ready, i.e. UNBUSY, as soon as we enter
	 * this interrupt handler.  Some, like the HP540 (see bug 1192665)
	 * aren't ready for a relatively-long time after entering this
	 * interrupt handler. However, they may be ready in less than 10us,
	 * so rather than calling a busy-wait, check status repeatedly
	 * here in an attempt to compensate for slow BUSY lines. If we
	 * don't succeed in lp_busy_check_retries, then let lptimeout()
	 * try again in about a second. lp_busy_check_retries can be set in
	 * /etc/system for really pathological cases. We expect about 16-20
	 * retries max on an HP540 and a 200MHz Pentium Pro.
	 */

	while ((((status = inb(lpp->status)) & UNBUSY) == 0) &&
		(statusloop < lp_busy_check_retries))
		statusloop++;

	lpp->lp_state = status & STATUS_MASK;

	if ((status & (UNBUSY | ONLINE)) == (UNBUSY | ONLINE)) {
		LPERRPRINT(LPEP_L0, LPEM_INTR,
		    ("lpintr: lpp=0x%x status=0x%x\n", lpp, status));

		lpxintr(lpp);
		return (DDI_INTR_CLAIMED);
	} else {
		if ((status & (UNBUSY | ONLINE)) == UNBUSY) {
			LPERRPRINT(LPEP_L1, LPEM_INTR,
			    ("lpintr: powered off??, lpp=0x%x status=0x%x\n",
			    lpp, status));
			outb(lpp->control, SEL | RESET);
		} else if (status & NOPAPER) {
			LPERRPRINT(LPEP_L3, LPEM_INTR,
			    ("lpintr: no paper, lpp=0x%x status=0x%x\n",
			    lpp, status));
		} else if (!(status & ONLINE)) {
			LPERRPRINT(LPEP_L3, LPEM_INTR,
			    ("lpintr: off-line, lpp=0x%x status=0x%x\n",
			    lpp, status));
		} else if (!(status & ERROR)) {
			LPERRPRINT(LPEP_L3, LPEM_INTR,
			    ("lpintr: error, lpp=0x%x status=0x%x\n",
			    lpp, status));
		} else
			LPERRPRINT(LPEP_L3, LPEM_INTR,
			    ("lpintr: unclaimed lpp=0x%x status=0x%x\n",
			    lpp, status));
		return (DDI_INTR_UNCLAIMED);
	}
}

/*
 * This is logically a part of lpintr.  This code
 * handles transmit buffer empty interrupts,
 * It works in  conjunction with lptimeout() to insure that lost
 * interrupts don't hang  the driver:
 * if a char is xmitted and we go more than 2s (MAXTIME) without
 * an interrupt, lptimeout will supply it.
 */
static void
lpxintr(struct lp_unit *lpp)
{
	struct strtty	*tp;

	LPERRPRINT(LPEP_L1, LPEM_INTR, ("lpxintr: lpp=0x%x\n", lpp));

	lpp->last_time = 0x7fffffffL;  /* don't time out */
	tp = &lpp->lp_tty;

	tp->t_state &= ~BUSY;
	lpproc(lpp, T_OUTPUT);

#ifdef REDUNDANT_CODE
	/* if output didn't start get a new message */
	if (!(tp->t_state & BUSY)) {
		if (tp->t_out.bu_bp) {
			freemsg(tp->t_out.bu_bp);
			tp->t_out.bu_bp = 0;
		}
		lpgetoblk(lpp);
	}
#endif
}

/*
 * General command routine that performs device specific operations for
 * generic i/o commands.  All commands are performed with tty level interrupts
 * disabled.
 */
static void
lpproc(struct lp_unit *lpp, int cmd)
{
	int	strobe_on = 0;
	register struct strtty *tp;
	register struct msgb *bp;

	LPERRPRINT(LPEP_L0, LPEM_PROC,
	    ("lpproc: lpp=0x%x cmd=0x%x\n", lpp, cmd));

	/*
	 * get device number and control port
	 */
	tp = &lpp->lp_tty;

	/*
	 * based on cmd, do various things to the device
	 */
	switch (cmd) {

	case T_TIME:	/* stop sending a break -- disabled for LP */
		goto start;

	case T_RESUME:	/* enable output */
		tp->t_state &= ~TTSTOP;
		/*FALLTHROUGH*/

	case T_OUTPUT:	/* do some output */
start:
		/* If we are busy, do nothing */
		if (tp->t_state & (TTSTOP | TIMEOUT))
			break;

		/*
		 * Check for characters ready to be output.
		 * If there are any, ship one out.
		 */
		bp = tp->t_out.bu_bp;
		if (bp == NULL || bp->b_wptr <= bp->b_rptr) {
			LPERRPRINT(LPEP_L1, LPEM_PROC,
			    ("lpproc: lpp=0x%x End of Buffer\n", lpp));
			if (tp->t_out.bu_bp) {
				freemsg(tp->t_out.bu_bp);
				tp->t_out.bu_bp = 0;
			}
			lpgetoblk(lpp);
			break;
		}
		/* output a char and set busy */
		if ((inb(lpp->status) & (UNBUSY | ONLINE)) ==
		    (UNBUSY | ONLINE)) {
			outb(lpp->data, *bp->b_rptr++);
			outb(lpp->control, SEL | RESET | INTR_ON);
			outb(lpp->control, SEL | RESET | INTR_ON | STROBE);
			/* XXX drv_usecwait(5); */
			strobe_on++;
		} else
			LPERRPRINT(LPEP_L2, LPEM_PROC,
			    ("lpproc: lpp=0x%x BUSY!\n", lpp));
		/*
		 * reset the time so we can catch a missed interrupt
		 */
		drv_getparm(TIME, (unsigned long *)&lpp->last_time);
		tp->t_state |= BUSY;
		if (strobe_on)
			outb(lpp->control, SEL | RESET | INTR_ON);
		break;

	case T_SUSPEND:		/* block on output */
		tp->t_state |= TTSTOP;
		break;

	case T_BREAK:		/* send a break -- disabled for LP */
		break;
	}
}


/*
 * Watchdog timer handler.
 */
/*ARGSUSED*/
static void
lptimeout(caddr_t arg)
{
	struct lp_unit	*lpp;
	time_t lptime;
	register int    unit_no;

	LPERRPRINT(LPEP_L0, LPEM_WATC, ("lptimeout\n"));

	for (unit_no = 0; unit_no <= lp_max_unit; unit_no++) {
		lpp = getsoftc(unit_no);
		if ((lpp != NULL) && (lpp->flag & OPEN)) {
			LPERRPRINT(LPEP_L2, LPEM_WATC,
			    ("lptimeout: lpp=0x%x\n", lpp));

			drv_getparm(TIME, (unsigned long *)&lptime);
			if ((lptime - lpp->last_time) > 0)  {
				lpxintr(lpp);
			}
		}
	}
	/* lptmrun = 1;	XXX	*/
	timeflag = timeout(lptimeout, NULL, drv_usectohz(1000000));
}


static void
lpdelay(struct lp_unit *lpp)
{
	register struct strtty *tp;

	tp = &lpp->lp_tty;
	tp->t_state &= ~TIMEOUT;
	lpproc(lpp, T_OUTPUT);
}


static unsigned char
get_error_status(unsigned char status)
{
	register unsigned char pin_status = 0;

	if ((status & (UNBUSY | ONLINE)) == UNBUSY) {
		/* power off */
		pin_status |= BPP_PE_ERR;
	} else if (status & NOPAPER) {
		/* no paper */
		pin_status |= BPP_SLCT_ERR;
		pin_status |= BPP_PE_ERR;
		pin_status |= BPP_ERR_ERR;
	} else if (!(status & ONLINE)) {
		/* off-line */
		pin_status |= BPP_SLCT_ERR;
	} else if (!(status & ERROR)) {
		/* printer error */
		pin_status |= BPP_ERR_ERR;
	}

	return (pin_status);
}
