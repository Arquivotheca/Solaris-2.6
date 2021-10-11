/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)kdmouse.c	1.26	96/08/06 SMI"

/*
 * PS/2 type Mouse Module - Streams
 */
#ifdef _DDICT
typedef unsigned int	pid_t;
#define	PZERO	25
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/termio.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strtty.h>
#include <sys/debug.h>
#include <sys/stat.h>
#include <sys/cmn_err.h>
#include "sys/ws/8042.h"
#include "sys/mouse.h"
#include "sys/mse.h"
#include "sys/kd.h"
#include <sys/modctl.h>
#include <sys/cred.h>
#include <sys/promif.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#define	PRF	prom_printf

#define	KDMOUSE_DEBUG

/*
 *
 * Local Static Data
 *
 */

#define	MSM_MAXUNIT	1
#define	MSMUNIT(dev)	((dev) & 0xf)

static dev_info_t *kdmouseunits[MSM_MAXUNIT];

static struct driver_minor_data {
	char	*name;
	int	minor;
	int	type;
} kdmouse_minor_data[] = {
	{"l", 0, S_IFCHR},
	{0}
};
int	rdkdmouse();

#ifdef	KDMOUSE_DEBUG
int kdmouse_debug = 0;
int kdmouse_debug_minimal = 0;
#endif	KDMOUSE_DEBUG


static struct strmseinfo *kdmouseptr = 0;

extern	int i8042_spin_time;
#ifdef	KD_IS_MT_SAFE
extern wstation_t	Kdws;
#endif

static struct	mcastat mcastat;

#define	SEND8042(port, byte) { \
	int waitcnt = 200000; \
	while ((inb(MSE_STAT) & 0x02) != 0 && waitcnt-- != 0) \
		; \
	outb(port, byte); \
	waitcnt = 200000; \
	while ((inb(MSE_STAT) & 0x02) != 0 && waitcnt-- != 0) \
		; \
}

static u_int kdmouseintr(caddr_t arg);
static int kdmouseopen(queue_t *q, dev_t *devp, int flag, int sflag,
    cred_t *cred_p);
static int kdmouse_wput(queue_t *q, mblk_t *mp);
static int kdmouseclose(queue_t *q, int flag, cred_t *cred_p);

static int kdmouseinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
		void **result);
static int kdmouseprobe(dev_info_t *dev);
static int kdmouseattach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int kdmousedetach(dev_info_t *dev, ddi_detach_cmd_t cmd);
static int kdmouseinit(dev_info_t *dev);


struct module_info	kdmouseminfo = { 23, "kdmouse", 0, INFPSZ, 256, 128};

static struct qinit kdmouse_rinit = {
	NULL, NULL, kdmouseopen, kdmouseclose, NULL, &kdmouseminfo, NULL};

static struct qinit kdmouse_winit = {
	kdmouse_wput, NULL, NULL, NULL, NULL, &kdmouseminfo, NULL};

struct streamtab kdmouse_info = { &kdmouse_rinit, &kdmouse_winit, NULL, NULL};

ddi_iblock_cookie_t 	kdmouse_iblock_cookie;

char	kdmouseclosing = 0;

extern int nulldev(), nodev();
/*
 * Local Function Declarations
 */

struct cb_ops	kdmouse_cb_ops = {
	nodev,			/* open */
	nodev,			/* close */
	nodev,			/* strategy */
	nodev,			/* print */
	nodev,			/* dump */
	nodev,			/* read */
	nodev,			/* write */
	nodev,			/* ioctl */
	nodev,			/* devmap */
	nodev,			/* mmap */
	nodev,			/* segmap */
	nochpoll,		/* poll */
	ddi_prop_op,		/* cb_prop_op */
	(&kdmouse_info),		/* streamtab  */
	0	/* Driver compatibility flag */

};


struct dev_ops	kdmouse_ops = {

	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	kdmouseinfo,		/* info */
	nulldev,		/* identify */
	kdmouseprobe,		/* probe */
	kdmouseattach,		/* attach */
	kdmousedetach,		/* detach */
	nodev,			/* reset */
	&kdmouse_cb_ops,		/* driver operations */
	(struct bus_ops *)0	/* bus operations */

};

char _depends_on[] =  "drv/kd";

/*
 * Module linkage information for the kernel.
 */

static struct modldrv modldrv = {
	&mod_driverops, /* Type of module.  This one is a driver */
	"PS/2 Bus Mouse driver",
	&kdmouse_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,
	&modldrv,
	NULL
};

/*
 * kdmintr -- enable/disable mouse interrupts.
 */
kdmintr(int on)
{
	int	waitcnt;
	unchar ps2_cont;

	i8042_acquire(FROM_DRIVER);
#ifdef	KD_IS_MT_SAFE
	mutex_enter(&Kdws.w_hw_mutex);
#endif

	SEND8042(MSE_ICMD, MSE_RCB);
	waitcnt = 200000;
	while ((KB_OUTBF & inb(MSE_STAT)) == 0 && waitcnt-- != 0)
		;

	if (waitcnt < 0) {
#ifdef	KD_IS_MT_SAFE
		mutex_exit(&Kdws.w_hw_mutex);
#endif
		i8042_release(FROM_DRIVER);
		return (-1);
	}

	ps2_cont = inb(MSE_OUT);	/* get cmd byte to change */
					/* aux interrupt status */
	SEND8042(MSE_ICMD, MSE_WCB);
	drv_usecwait(i8042_spin_time);
	if (on)
		ps2_cont |= 0x02;
	else
		ps2_cont &= ~0x02;
	SEND8042(MSE_IDAT, ps2_cont); /* change interrupts status */

	drv_usecwait(i8042_spin_time);

#ifdef	KD_IS_MT_SAFE
	mutex_exit(&Kdws.w_hw_mutex);
#endif
	i8042_release(FROM_DRIVER);

	return (0);
}

/*
 * This is the driver initialization routine.
 */
int
_init()
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

static int
kdmouseprobe(dev_info_t *dip)
{
	register int 	unit;

#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug) {
		PRF("kdmouseprobe: entry\n");
	}
#endif

	unit = ddi_get_instance(dip);
#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("unit is %x\n", unit);
#endif
	if (unit >= MSM_MAXUNIT || kdmouseunits[unit])
		return (DDI_PROBE_FAILURE);

	return (kdmouseinit(dip));
}

/*ARGSUSED*/
static int
kdmouseattach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int unit;
	struct driver_minor_data *dmdp;

#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug) {
		PRF("kdmouseattach entry\n");
	}
#endif

	unit = ddi_get_instance(dip);

	for (dmdp = kdmouse_minor_data; dmdp->name != NULL; dmdp++) {
		if (ddi_create_minor_node(dip, dmdp->name, dmdp->type,
		    dmdp->minor, 0, NULL) == DDI_FAILURE) {

			ddi_remove_minor_node(dip, NULL);
			ddi_prop_remove_all(dip);
#ifdef KDMOUSE_DEBUG
			if (kdmouse_debug)
				PRF("kdmouseattach: "
				    "ddi_create_minor_node failed\n");
#endif
			return (DDI_FAILURE);
		}
	}
	kdmouseunits[unit] = dip;

	if (ddi_add_intr(dip, (u_int)0, &kdmouse_iblock_cookie,
		(ddi_idevice_cookie_t *)0, kdmouseintr, (caddr_t)0)) {
#ifdef KDMOUSE_DEBUG
		if (kdmouse_debug)
			PRF("kdmouseattach: ddi_add_intr failed\n");
#endif
		cmn_err(CE_WARN, "kdmouse: cannot add intr\n");
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);
	return (DDI_SUCCESS);
}

/*
 * kdmousedetach:
 */
/*ARGSUSED*/
static int
kdmousedetach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int instance;

	instance = ddi_get_instance(dip);
	if (instance >= MSM_MAXUNIT)
		return (DDI_FAILURE);

	switch (cmd) {

	case DDI_DETACH:
		if (kdmouseunits[instance] == NULL)
			return (DDI_FAILURE);
		kdmouseunits[instance] = NULL;
		ddi_remove_intr(dip, 0, kdmouse_iblock_cookie);
		ddi_prop_remove_all(dip);
		ddi_remove_minor_node(dip, NULL);
		return (DDI_SUCCESS);

	default:
#ifdef KDMOUSE_DEBUG
		if (kdmouse_debug) {
			PRF("kdmousedetach: cmd = %d unknown\n", cmd);
		}
#endif
		return (DDI_FAILURE);
	}
}


/* ARGSUSED */
static int
kdmouseinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	register dev_t dev = (dev_t)arg;
	register int unit;
	register dev_info_t *devi;

#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouseinfo: call\n");
#endif
	if (((unit = MSMUNIT(dev)) >= MSM_MAXUNIT) ||
		(devi = kdmouseunits[unit]) == NULL)
		return (DDI_FAILURE);

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		*result = (void *)devi;
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)unit;
		break;
	default:
		return (DDI_FAILURE);
	}
	return (DDI_SUCCESS);
}


static int
kdmouseinit(dev_info_t *dip)
{
	int	old_probe;

#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouseinit()\n");
#endif
	old_probe = ddi_getprop(DDI_DEV_T_ANY, dip, 0,
		"ignore-hardware-nodes", 0);

	mcastat.present = 0;
	if (!old_probe) {
		int reglen;
		struct {
			int bustype;
			int base;
			int size;
		} *reglist;

		if (ddi_getlongprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
		    "reg", (caddr_t)&reglist, &reglen) != DDI_PROP_SUCCESS) {
			cmn_err(CE_WARN, "msm_probe: reg property not found "
			    "in devices property list");
			return (DDI_PROBE_FAILURE);
		}
		kmem_free(reglist, reglen);
		if ((reglen / sizeof (*reglist)) == 0)
			return (DDI_PROBE_FAILURE);
		mcastat.present = 1;
		return (DDI_PROBE_SUCCESS);
	}

	if (i8042_aux_port()) {
		mcastat.present = 1;

		/*
		 * obtain ownership of 8042 and disable
		 * interfaces
		 */
		i8042_acquire(FROM_DRIVER);

		/*
		 * the following only sets the software status
		 * variable for the 8042. If the aux interface was enabled on
		 * the 8042 prior to the call to i8042_acquire, it won't be
		 * disabled until the call to i8042_release()
		 */
		i8042_program(P8042_AUXDISAB); /* we'll enable in open */

		i8042_release(FROM_DRIVER);

		/*
		 * Mouse interrupts cannot be serviced till open time,
		 * disable them till then.
		 */
		kdmintr(0);
#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouseinit: succeeded\n");
#endif
		return (DDI_PROBE_SUCCESS);
	} else {
#ifdef KDMOUSE_DEBUG
		if (kdmouse_debug)
			PRF("kdmouseinit: failed\n");
#endif
		return (DDI_PROBE_FAILURE);
	}
}

/*ARGSUSED1*/
static int
kdmouseopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *cred_p)
{
#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouseopen:entered\n");
#endif

	if (q->q_ptr != NULL)
		return (0);
	if (mcastat.present != 1)
		return (ENXIO);
	while (kdmouseclosing)
		sleep((caddr_t)&kdmouseinfo, PZERO + 1);

	/* allocate and initialize state structure */
	kdmouseptr = kmem_zalloc(sizeof (struct strmseinfo), KM_SLEEP);
	q->q_ptr = (caddr_t)kdmouseptr;
	WR(q)->q_ptr = (caddr_t)kdmouseptr;
	kdmouseptr->rqp = q;
	kdmouseptr->wqp = WR(q);

	/* enable the aux interface on the 8042 */
	if (kdmintr(1) == -1)
		return (EIO);

	i8042_acquire(FROM_DRIVER);
	i8042_program(P8042_AUXENAB);
	i8042_release(FROM_DRIVER);

	return (0);

}


/*ARGSUSED1*/
static int
kdmouseclose(queue_t *q, int flag, cred_t *cred_p)
{
	struct strmseinfo *tmpkdmouseptr;

#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouseclose:entered\n");
#endif

	kdmouseclosing = 1;
	/* leave aux interface disabled */
	i8042_program(P8042_AUXDISAB);
	/* Do these two *solely* to cause the "disable" above to take effect */
	i8042_acquire(FROM_DRIVER);
	i8042_release(FROM_DRIVER);
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	/*
	 * Interrupts cannot be serviced once we give up kdmouseptr,
	 * disable them before.
	 */
	kdmintr(0);
	kdmouseclosing = 0;
	wakeup((caddr_t)&kdmouseinfo);
	tmpkdmouseptr = kdmouseptr;
	kdmouseptr = NULL;
	rdkdmouse();		/* empty 8042 of aux port input */
	kmem_free(tmpkdmouseptr, sizeof (struct strmseinfo));
	return (0);
}

void
kdm_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error, int rval)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp, mp);
}

static int
kdmouse_wput(queue_t *q, mblk_t *mp)
{
	register struct iocblk *iocbp;
	register mblk_t *bp;
	register mblk_t *next;

#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouse_wput:entered\n");
#endif
	if (kdmouseptr == 0) {
		freemsg(mp);
#ifdef KDMOUSE_DEBUG
		if (kdmouse_debug)
			PRF("kdmouse_wput:kdmouseptr == NULL\n");
#endif
		return (0);
	}
	iocbp = (struct iocblk *)mp->b_rptr;
	switch (mp->b_datap->db_type) {
	case M_FLUSH:
#ifdef KDMOUSE_DEBUG
		if (kdmouse_debug)
			PRF("kdmouse_wput:M_FLUSH\n");
#endif
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		qreply(q, mp);
		break;
	case M_IOCTL:
#ifdef KDMOUSE_DEBUG
		if (kdmouse_debug)
			PRF("kdmouse_wput:M_IOCTL\n");
#endif
		kdm_iocnack(q, mp, iocbp, EINVAL, 0);
		break;
	case M_IOCDATA:
#ifdef KDMOUSE_DEBUG
		if (kdmouse_debug)
			PRF("kdmouse_wput:M_IOCDATA\n");
#endif
		kdm_iocnack(q, mp, iocbp, EINVAL, 0);
		break;
	case M_DATA:
		bp = mp;
		do {
			while (bp->b_rptr < bp->b_wptr) {
#ifdef	KDMOUSE_DEBUG
if (kdmouse_debug) PRF("kdmouse:  send %2x\n", *bp->b_rptr);
if (kdmouse_debug_minimal) PRF(">%2x ", *bp->b_rptr);
#endif
				(void) i8042_send_cmd(*bp->b_rptr++, 2,
					(unsigned char *)NULL, 0, FROM_DRIVER);
			}
			next = bp->b_cont;
			freeb(bp);
		} while ((bp = next) != NULL);
		break;
	default:
		freemsg(mp);
		break;
	}
#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouse_wput:leaving\n");
#endif
	return (0);
}

/*ARGSUSED*/
static u_int
kdmouseintr(caddr_t arg)
{
	register int    mdata;
	mblk_t *mp;

#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouseintr()\n");
#endif
	if (!mcastat.present)
		return (DDI_INTR_UNCLAIMED);
	if (!kdmouseptr)
		return (DDI_INTR_UNCLAIMED);
	if ((mdata = rdkdmouse()) == FAILED)
		return (DDI_INTR_UNCLAIMED);
	if ((mp = allocb(1, BPRI_MED)) != NULL) {
		*mp->b_wptr++ = (char)mdata;
		putnext(kdmouseptr->rqp, mp);
	}
#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouseintr() ok\n");
#endif
	return (DDI_INTR_CLAIMED);
}

/* 320 mouse read data function */
int
rdkdmouse()
{
	register int data;
	register int wait;

#ifdef KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("rdkdmouse() called\n");
#endif
	/* wait until the 8042 output buffer is full */
	for (wait = 0; wait < 6; wait++) {
		if ((MSE_OUTBF & inb(MSE_STAT)) == MSE_OUTBF)
			break;
		drv_usecwait(10);
	}
	if (wait == 6) {
#ifdef KDMOUSE_DEBUG
		if (kdmouse_debug)
			PRF("rdkdmouse() FAILED timeout\n");
#endif
		return (FAILED);
	}
	data = inb(MSE_OUT); 	/* get data byte from controller */
#ifdef	KDMOUSE_DEBUG
	if (kdmouse_debug)
		PRF("kdmouse:  got %2x\n", data & 0xff);
	if (kdmouse_debug_minimal)
		PRF("<%2x ", data&0xff);
#endif
	return (data & 0xff);
}
