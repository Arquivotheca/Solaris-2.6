/*
 * Copyright (c) 1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ident "@(#)tiqmouse.c	1.3	96/05/27 SMI"

/*
 * TI TM4000E PS/2 type Mouse Module - Streams
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/kmem.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/termio.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/strtty.h"
#include "sys/debug.h"
#include "sys/ddi.h"
#include "sys/cred.h"
#include <sys/stat.h>
#include <sys/sunddi.h>
#include "sys/proc.h"
#include "sys/cmn_err.h"
#include "sys/ws/chan.h"
#include "sys/mouse.h"
#include "sys/mse.h"

#define	PRF	printf

/*
 *
 * Local Static Data
 *
 */
/*
 * MSE_ANY used in 320 mouse command processing for case where we don't
 * care what return byte val is
 */

#define	MSE_ANY		0xFE

#define	M_IN_DATA	0
#define	M_OUT_DATA	1
#define	SNDERR2		0xfc

#define	MSM_MAXUNIT	1
#define	MSMUNIT(dev)	((dev) & 0xf)

/*
 * Private I/O port defines for the QuickPort on the TM4000E
 */
#define	TIQP_DATA	(mouse_base)
#define	TIQP_STATUS	(mouse_base+1)
#define	C710_INDEX	(mouse_base+4)
#define	C710_DATA	(mouse_base+5)

#define	SIG_PORT_1	(0x2FA)
#define	SIG_PORT_2	(0x3FA)

/*
 * Mouse Status port defines
 */

#define	TIQD_ENABLE	(0x80)
#define	TIQD_CLEAR	(0x40)
#define	TIQD_ERROR	(0x20)
#define	TIQD_INT_ENABLE	(0x10)
#define	TIQD_RESET	(0x08)
#define	TIQD_XMIT_IDLE	(0x04)
#define	TIQD_CHAR_AVAIL	(0x02)
#define	TIQD_IDLE	(0x01)

static dev_info_t *tiqmouseunits[MSM_MAXUNIT];

static struct driver_minor_data {
	char	*name;
	int	minor;
	int	type;
} tiqmouse_minor_data[] = {
	{"l", 0, S_IFCHR},
	{0}
};

void ps2parse();

#define	TIQMOUSE_DEBUG 1
#ifdef	TIQMOUSE_DEBUG
int tiqmouse_debug = 0;
#endif	TIQMOUSE_DEBUG


static struct strmseinfo *tiqmouseptr = 0;

int	mouse_base = 0x310;

static struct	mcastat mcastat;

static u_int tiqmouseintr(caddr_t arg);
static int tiqmouse_wput(queue_t *q, mblk_t *mp);
static int tiqmouseopen(queue_t *q, dev_t *devp, int flag, int sflag,
    cred_t *cred_p);
static int tiqmouseclose(queue_t *q, register flag, cred_t *cred_p);
uint tiqm_send_mouse_init();
uint tiqm_init_mouse_intr(caddr_t);

static int tiqmouseinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
		void **result);
static int tiqmouseprobe(dev_info_t *dev);
static int tiqmouseattach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int tiqmousedetach(dev_info_t *dev, ddi_detach_cmd_t cmd);
static int tiqmouseinit(dev_info_t *dip);
static void tiqm_clear_interface(void);
static int tiqm_read_byte(int fromintr);
static int tiqmfunc(struct cmd_320 *mca);

struct module_info	tiqmouseminfo = { 69, "tiqmouse", 0, INFPSZ, 256, 128};

static struct qinit tiqmouse_rinit = {
	NULL, NULL, tiqmouseopen, tiqmouseclose, NULL, &tiqmouseminfo, NULL};

static struct qinit tiqmouse_winit = {
	tiqmouse_wput, NULL, NULL, NULL, NULL, &tiqmouseminfo, NULL};

struct streamtab tiqmouse_info = {
	&tiqmouse_rinit,
	&tiqmouse_winit,
	NULL,
	NULL
};

ddi_iblock_cookie_t	tiqm_iblock_cookie;
ddi_softintr_t		tiqm_softint_id;

char	tiqmouseclosing = 0;

static	unchar		packet[3];	/* saved data bytes XXX */

/*
 * Local Function Declarations
 */

struct cb_ops	tiqmouse_cb_ops = {
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
	(&tiqmouse_info),			/* streamtab  */
	0	/* Driver compatibility flag */

};


struct dev_ops	tiqmouse_ops = {

	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	tiqmouseinfo,		/* info */
	nulldev,		/* identify */
	tiqmouseprobe,		/* probe */
	tiqmouseattach,		/* attach */
	tiqmousedetach,		/* detach */
	nodev,			/* reset */
	&tiqmouse_cb_ops,		/* driver operations */
	(struct bus_ops *)0	/* bus operations */

};

#ifndef BUILD_STATIC
#if 0
char _depends_on[] =  "drv/kd";
#endif

/*
 * This is the loadable module wrapper.
 */
#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

/*
 * Module linkage information for the kernel.
 */

static struct modldrv modldrv = {
	&mod_driverops, /* Type of module.  This one is a driver */
	"PS/2 TM4000E Mouse driver",
	&tiqmouse_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1,
	&modldrv,
	NULL
};

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


extern struct modctl *mod_getctl();
extern char *kobj_getmodname();

#endif

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}


static int
tiqmouseprobe(dev_info_t *dip)
{
	register int 	unit;

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug) {
		PRF("tiqmouseprobe: entry (%x)\n", (caddr_t)dip);
	}
#endif

	unit = ddi_get_instance(dip);
#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		PRF("unit is %x\n", unit);
#endif
	if (unit >= MSM_MAXUNIT || tiqmouseunits[unit])
		return (DDI_PROBE_FAILURE);

	return (tiqmouseinit(dip));
}

/*ARGSUSED*/
static int
tiqmouseattach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int unit;
	struct driver_minor_data *dmdp;

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug) {
		PRF("tiqmouseattach entry\n");
	}
#endif

	unit = ddi_get_instance(dip);

	for (dmdp = tiqmouse_minor_data; dmdp->name != NULL; dmdp++) {
		if (ddi_create_minor_node(dip, dmdp->name, dmdp->type,
		    dmdp->minor, 0, NULL) == DDI_FAILURE) {

			ddi_remove_minor_node(dip, NULL);
			ddi_prop_remove_all(dip);
#ifdef TIQMOUSE_DEBUG
			if (tiqmouse_debug)
				PRF("tiqmouseattach: "
				    "ddi_create_minor_node failed\n");
#endif
			return (DDI_FAILURE);
		}
	}
	tiqmouseunits[unit] = dip;

	if (ddi_add_intr(dip, (u_int) 0, &tiqm_iblock_cookie,
	    (ddi_idevice_cookie_t *)0, tiqmouseintr, (caddr_t)0)) {
#ifdef TIQMOUSE_DEBUG
		if (tiqmouse_debug)
			PRF("tiqmouseattach: ddi_add_intr failed\n");
#endif
		cmn_err(CE_WARN, "tiqmouse: cannot add intr\n");
		return (DDI_FAILURE);
	}

	/* register the soft interrupt to reset the mouse */
	if (ddi_add_softintr(dip, DDI_SOFTINT_LOW, &tiqm_softint_id, 0, 0,
	    tiqm_init_mouse_intr, 0) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "tiqmouse: cannot add softintr\n");
		return (DDI_FAILURE);
	}
	ddi_report_dev(dip);
	return (DDI_SUCCESS);
}

/*
 * tiqmousedetach:
 */
/*ARGSUSED*/
static int
tiqmousedetach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int instance;

	switch (cmd) {

	case DDI_DETACH:

		instance = ddi_get_instance(dip);
		ddi_remove_intr(dip, 0, tiqm_iblock_cookie);
		ddi_remove_softintr(tiqm_softint_id);
		tiqmouseunits[instance] = 0;
		ddi_prop_remove_all(dip);
		ddi_remove_minor_node(dip, NULL);
		return (DDI_SUCCESS);

	default:
#ifdef TIQMOUSE_DEBUG
		if (tiqmouse_debug) {
			PRF("tiqmousedetach: cmd = %d unknown\n", cmd);
		}
#endif
		return (DDI_FAILURE);
	}
}


/* ARGSUSED */
static int
tiqmouseinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	register dev_t dev = (dev_t)arg;
	register int unit;
	register dev_info_t *devi;

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		PRF("tiqmouseinfo: call\n");
#endif
	if (((unit = MSMUNIT(dev)) >= MSM_MAXUNIT) ||
		(devi = tiqmouseunits[unit]) == NULL)
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

/*
 * Set the interrupt control state on the QuickPort controller
 */
static int
tiqm_interrupt_enable(int enable)
{
	register int	i, o;

	i = inb(TIQP_STATUS);
	o = (i & TIQD_INT_ENABLE) != 0;
	if (!enable)
		i &= ~TIQD_INT_ENABLE;	/* disable interrupts */
	else
		i |= TIQD_INT_ENABLE;	/* enable interrupts */
	drv_usecwait(1);
	outb(TIQP_STATUS, i);
	return (o);			/* return old state */
}

/*
 *
 */
static void
tiqmwrite710(int reg, int val)
{
	outb(C710_INDEX, reg);
	drv_usecwait(1);
	outb(C710_DATA, val);
}

/*
 *
 */
static int
tiqmread710(int reg)
{
	outb(C710_INDEX, reg);
	drv_usecwait(1);
	return (inb(C710_DATA));
}

/*
 * Attempt to initialize the 82C710 and verify it is present.
 * This is based on information provided by TI, without the benefit of
 * the Chips and Technology 82C710 manual as of 3/7/94.
 *
 * The C&T 82C710 chip used to provide the QuickPort interface on the
 * TI TM4000E can be configured to locate I/O ports at desired positions.
 * They are restricted to integer multiples of 4.  This driver locates
 * the mouse I/O ports at x310 and the 82C710 ports at the next multiple
 * of 4, which is 0x314.  This driver effectively consumes the range of
 * 0x310..0x315.
 */
static int
tiqmouseinit(dev_info_t *dip)
{
	register int	i;
	int	ioaddr, conf_addr_4;
	int	len;

#ifdef TIQM_DEBUG
	if (tiqm_debug)
		PRF("tiqminit: call mouse_base = %x\n", mouse_base);
#endif
	len = sizeof (int);

	/*
	 * check if ioaddr is set in .conf file, it should be.  If it
	 * isn't then try the default i/o addr
	 */
	if (ddi_prop_op(DDI_DEV_T_ANY, dip, PROP_LEN_AND_VAL_BUF,
	    DDI_PROP_DONTPASS, "ioaddr", (caddr_t)&ioaddr,
	    &len) == DDI_PROP_SUCCESS) {
		mouse_base = ioaddr;
#ifdef TIQM_DEBUG
	if (tiqm_debug)
		PRF("tiqminit: call mouse_base = %x\n", mouse_base);
#endif
	}
	mcastat.present = 0;

/*
 * Convoluted sequence of port accesses to prove an 82C710 is present
 * and also initialize the controller.  Though the controller seems to
 * always initialize to the state where it will generate interrupts,
 * I don't think the kernel enables the interrupt in the PIC until
 * the intr_attach().  In any case, we don't need to take interrupts
 * to probe the hardware.
 */

/*
 * NEEDSWORK: in general, this is a destructive probe.  Needs to
 * be reviewed for collision if the driver is used on machines other
 * than the TI TM4000E laptop.  In particular, SIG_PORT_1 and SIG_PORT_2
 * and normally read-only ports on the serial controllers.  Ugh.
 */

	conf_addr_4 = (mouse_base>>2)+1;

	outb(SIG_PORT_1, 0x55);
	drv_usecwait(1);		/* I don't think this is required, */
	outb(SIG_PORT_2, ~0x55);
	drv_usecwait(1);		/* but this delay is cheap insurance. */
	outb(SIG_PORT_2, 0x36);
	drv_usecwait(1);
	outb(SIG_PORT_2, conf_addr_4);
	drv_usecwait(1);
	outb(SIG_PORT_1, ~conf_addr_4);
	drv_usecwait(1);

	i = tiqmread710(15);
	if (i != conf_addr_4) {
		/* failure to detect 82C710 */
		goto tiqminit_failure;
	}

	i = tiqmread710(0);
	if ((i & 0x60) == 0x60) {
		/* failure to detect 82C710 oscillator */
		goto tiqminit_failure;
	}

	i = tiqmread710(12);
	if ((i & 1) == 1) {
		/* failure: port powered down */
		goto tiqminit_failure;
	}

	i = tiqmread710(13);
	if ((i == 0) || (i == 0xFF)) {
		/* failure: invalid mouse address */
		goto tiqminit_failure;
	}

	ioaddr = i << 2;		/* note the reuse of 'ioaddr' */
	if (ioaddr != mouse_base) {
		/* failure: mouse port isn't where we expect */
		goto tiqminit_failure;
	}

	tiqmwrite710(15, 15);
	i = tiqmread710(15);
	if (i == conf_addr_4) {
		/* failure: sanity check */
		goto tiqminit_failure;
	}


	mcastat.present = 1;
	mcastat.mode = MSESTREAM;

#ifdef TIQM_DEBUG
	if (tiqm_debug)
		printf("tiqminit:Disable interrupts ioaddr %x\n", mouse_base);
#endif

/*
 * Be a nice guy and insure the 82C710 doesn't yank the interrupt line
 * quite yet.
 */
	tiqm_interrupt_enable(0);

	drv_usecwait(1);
	i = inb(TIQP_STATUS);
	i |= TIQD_ENABLE;			/* make sure mouse is enabled */
	drv_usecwait(1);
	outb(TIQP_STATUS, i);

/*
 * We go ahead and report success if the mouse controller is present.
 * If the mouse is not actually plugged into the mouse port, we do
 * not fail.  It is quite easy to knock the QuickPort mouse off on
 * a TM4000E (don't believe me? try using one) and this driver wants
 * be as resiliant as possible with respect to this.  When the user
 * replaces the mouse while the driver is open, we'll recover.
 */

#ifdef TIQM_DEBUG
	if (tiqm_debug)
		PRF("tiqminit: succeeded\n");
#endif
	return (DDI_SUCCESS);

tiqminit_failure:

#ifdef TIQM_DEBUG
	if (tiqm_debug)
		PRF("tiqminit: failed\n");
#endif
	return (DDI_PROBE_FAILURE);
}

/*
 * PS/2 Mouse initialization sequence
 */
uint
tiqm_send_mouse_init()
{
	struct	cmd_320	mca;

	mca.cmd = MSEON;		/* turn on 320 mouse */
	if (tiqmfunc(&mca) == FAILED)
		goto tiqm_smi_error;

	mca.cmd = MSESETRES;
	mca.arg1 = 0x03;
	if (tiqmfunc(&mca) == FAILED)
		goto tiqm_smi_error;

	mca.cmd = MSECHGMOD;
	mca.arg1 = 0x28;
	if (tiqmfunc(&mca) == FAILED)
		goto tiqm_smi_error;

	return (0);

tiqm_smi_error:
	return ((uint)FAILED);
}

/*
 *
 */
/*ARGSUSED*/
uint
tiqm_init_mouse_intr(caddr_t arg)
{

	if ((tiqmouseptr == NULL) || (tiqmouseptr->state != 3))
		return (DDI_INTR_UNCLAIMED);

	tiqmouseptr->state = 0;
	(void) tiqm_send_mouse_init();

	return (DDI_INTR_CLAIMED);
}


/*
 * mouse open function
 */
/*ARGSUSED1*/
static int
tiqmouseopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *cred_p)
{

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmouseopen:entered\n");
#endif

	if (q->q_ptr != NULL)
		return (0);
	if (mcastat.present != 1)
		return (ENXIO);
	while (tiqmouseclosing)
		sleep((caddr_t)&tiqmouseinfo, PZERO + 1);

	if (tiqm_send_mouse_init() == FAILED)
		return (ENXIO);

	/* allocate and initialize state structure */
	tiqmouseptr = kmem_zalloc(sizeof (struct strmseinfo), KM_SLEEP);
	q->q_ptr = (caddr_t)tiqmouseptr;
	WR(q)->q_ptr = (caddr_t)tiqmouseptr;
	tiqmouseptr->rqp = q;
	tiqmouseptr->wqp = WR(q);
	tiqmouseptr->old_buttons = 0x07; /* Initialize to all buttons up */

	tiqm_interrupt_enable(1);

	return (0);

}


/*ARGSUSED1*/
static int
tiqmouseclose(queue_t *q, int flag, cred_t *cred_p)
{
	struct	cmd_320	mca;
	struct strmseinfo *tmptiqmouseptr;

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmouseclose:entered\n");
#endif

	tiqmouseclosing = 1;
	mcastat.map_unit = -1;

	mca.cmd = MSEOFF;
	tiqmfunc(&mca);
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	/*
	 * Interrupts cannot be serviced once we give up tiqmouseptr,
	 * disable them before.
	 */
	tiqm_interrupt_enable(0);
	tiqmouseclosing = 0;
	wakeup((caddr_t)&tiqmouseinfo);
	tmptiqmouseptr = tiqmouseptr;
	tiqmouseptr = NULL;

	kmem_free(tmptiqmouseptr, sizeof (struct strmseinfo));
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
tiqmouse_wput(queue_t *q, mblk_t *mp)
{
	register struct iocblk *iocbp;

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmouse_wput:entered\n");
#endif
	if (tiqmouseptr == 0) {
		freemsg(mp);
#ifdef TIQMOUSE_DEBUG
		if (tiqmouse_debug)
			printf("tiqmouse_wput:tiqmouseptr == NULL\n");
#endif
		return (0);
	}
	iocbp = (struct iocblk *)mp->b_rptr;
	switch (mp->b_datap->db_type) {
		case M_FLUSH:
#ifdef TIQMOUSE_DEBUG
			if (tiqmouse_debug)
				printf("tiqmouse_wput:M_FLUSH\n");
#endif
			if (*mp->b_rptr & FLUSHW)
				flushq(q, FLUSHDATA);
			qreply(q, mp);
			break;
		case M_IOCTL:
#ifdef TIQMOUSE_DEBUG
			if (tiqmouse_debug)
				printf("tiqmouse_wput:M_IOCTL\n");
#endif
			kdm_iocnack(q, mp, iocbp, EINVAL, 0);
			break;
		case M_IOCDATA:
#ifdef TIQMOUSE_DEBUG
			if (tiqmouse_debug)
				printf("tiqmouse_wput:M_IOCDATA\n");
#endif
			kdm_iocnack(q, mp, iocbp, EINVAL, 0);
			break;
		default:
			freemsg(mp);
			break;
	}
#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmouse_wput:leaving\n");
#endif
	return (0);
}

/*
 * should use mutex_enter/exit here, but there's only
 * one thread active in this module at any one time.
 */

/*ARGSUSED*/
static u_int
tiqmouseintr(caddr_t arg)
{
	register int    mdata;


	if (!mcastat.present)
		return (DDI_INTR_UNCLAIMED);
	if (!tiqmouseptr)
		return (DDI_INTR_UNCLAIMED);
	if (mcastat.mode == MSESTREAM) {
		if ((mdata = tiqm_read_byte(1)) == FAILED)
			return (DDI_INTR_UNCLAIMED);
		ps2parse(mdata);
		return (DDI_INTR_CLAIMED);
	}
	return (DDI_INTR_UNCLAIMED);
}

/*
 *
 */
void
ps2parse(c)
register unchar	c;
{
	register char	tmp;
	register struct strmseinfo	*m = tiqmouseptr;
	static unsigned long then = 0;
	static unsigned long mouse_timeout = 20;
	unsigned long now, elapsed;

	drv_getparm(LBOLT, &now);	/* what time is it? */
	elapsed = now - then;	/* how long has it been since the mouse */
				/* talked to us? */
	then = now;

	if ((elapsed > mouse_timeout) && (m->state != 0)) {
		if (m->state != 3)
			m->state = 0;	/* reset state if it needs to be */
	}

	/* Parse the next byte of input.  */

	switch (m->state)
	{

	case 0:
		/*
		 * Interpretation of the bits in byte 1:
		 *
		 *	Yo Xo Ys Xs 1 M R L
		 *
		 *	L:  left button state (1 == down)
		 *	R:  right button state
		 *	M:  middle button state
		 *	1:  always 1, never used by mouse driver
		 *	Xs: X delta (byte 2) sign bit (1 == Negative)
		 *	Ys: Y delta (byte 3) sign bit
		 *	Xo: X overflow bit, never used by mouse driver
		 *	Yo: Y overflow bit, never used by mouse driver
		 */

		/*
		** Shift the buttons bits into the order required: LMR
		*/
		packet[0] = c;

		tmp = (c & 0x01) << 2;
		tmp |= (unsigned int) (c & 0x06) >> 1;
		m->button = (tmp ^ 0x07);	/* Buttons */

		m->x = (c & 0x10);		/* X sign bit */
		m->y = (c & 0x20);		/* Y sign bit */
		m->state = 1;
		break;

	case 1:
		/*
		 * Second byte is X movement as a delta
		 *
		 *	This byte should be interpreted as a 9-bit
		 *	2's complement number where the Most Significant
		 *	Bit (i.e. the 9th bit) is the Xs bit from byte 1.
		 *
		 *	But since we store the value as a 2's complement
		 *	8-bit number (i.e. signed char) we need to
		 *	truncate to 127 or -128 as needed.
		 */
		packet[1] = c;

		if ((packet[0] == 0xAA) && (packet[1] == 0x00)) {
			cmn_err(CE_WARN,
			    "tiqmouse: mouse connection detected\n");
			m->state = 3;
			ddi_trigger_softintr(tiqm_softint_id);
			return;
		}

		if (m->x) {
			/* Negative delta */
			/*
			 * The following blocks of code are of the form
			 *
			 *	statement1;
			 *	if ( condition )
			 *		statement2;
			 *
			 * rather than
			 *
			 *	if ( condition )
			 *		statement2;
			 *	else
			 *		statement1;
			 *
			 * because it generates more efficent assembly code.
			 */

			m->x = -128;		/* Set to signed char min */

			if (c & 0x80)		/* NO truncate    */
				m->x = c;
		} else {
			/* Positive delta */
			m->x = 127;		/* Set to signed char max */

			if (!(c & 0x80))	/* Truncate */
				m->x = c;
		}

		m->state = 2;
		break;

	case 2:
		/*
		 * Third byte is Y movement as a delta
		 *
		 *	See description of byte 2 above for how to
		 *	interpret byte 3.
		 *
		 *	The driver assumes position (0, 0) to be in the
		 *	upper left hand corner, BUT the PS/2 mouse
		 *	assumes (0, 0) is in the lower left hand corner
		 *	so the truncated delted also needs to be
		 *	negated for the Y movement.
		 *
		 * The logic is a little contorted, however if you dig
		 * through it, it should be correct.  Remember the part
		 * about the 9-bit 2's complement number system
		 * mentioned above.
		 *
		 * For complete details see "Logitech Technical
		 * Reference & Programming Guide."
		 */
		packet[2] = c;

		if (m->y) {
			/* Negative delta treated as Positive */
			m->y = 127;		/* Set to signed char max */

			if ((unsigned char)c > 128)	/* Just negate */
				m->y = -c;
		} else {
			/* Positive delta treated as Negative */
			m->y = -128;		/* Set to signed char min */

			if ((unsigned char)c < 128)	/* Just negate */
				m->y = -c;
		}

		m->state = 0;


		/* pass the raw data up the stream to the user */
		if (m->rqp) {
			mblk_t *bp;

			if ((bp = allocb(3, BPRI_MED)) == NULL) {
				break;
			}
			*(bp->b_wptr)++ = packet[0];	/* sync */
			*(bp->b_wptr)++ = packet[1];	/* x coordinate */
			*(bp->b_wptr)++ = packet[2];  	/* y coordinate */
			putnext(m->rqp, bp);
		}
		break;

	case 3:
		break;

	default:
		break;
	}
}

/*
 * Clear the mouse interface for a polled interaction
 */
static void
tiqm_clear_interface(void)
{
	int	status;

	status = inb(TIQP_STATUS);
	drv_usecwait(1);
	outb(TIQP_STATUS, status | TIQD_RESET);
	drv_usecwait(1);
	outb(TIQP_STATUS, status & (~TIQD_RESET));
}

/*
 * Polled read byte from the mouse with timeout
 */
static int
tiqm_read_byte(int fromintr)
{
	long w;
	int  status;

	w = fromintr ? 10 : 20000;	/* use a short timeout for interrupts */
	while (--w > 0) {
		drv_usecwait(1);
		status = inb(TIQP_STATUS);
		if (status & TIQD_ERROR) {
			drv_usecwait(1);
			outb(TIQP_STATUS, status | TIQD_CLEAR);
			drv_usecwait(1);
			outb(TIQP_STATUS, status & (~TIQD_CLEAR));
			continue;
		}
		if (status & TIQD_CHAR_AVAIL)
			break;
	}

	if (w <= 0)
		return (FAILED);
	else {
		drv_usecwait(1);
		return (inb(TIQP_DATA) & 0xFF);
	}
}


/*
 * Write a byte to the mouse
 */
int
tiqm_send_byte(unsigned char byte)
{
	long w;

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("entered tiqm_send_byte() byte = %x\n", byte);
#endif

	w = 20000;		/* this loop count is scaled by uS */
	while (--w > 0) {
		drv_usecwait(1);
		if ((inb(TIQP_STATUS)&(TIQD_XMIT_IDLE|TIQD_IDLE)) ==
		    (TIQD_XMIT_IDLE|TIQD_IDLE))
			break;
	}

	if (w <= 0)
		return (FAILED);

	outb(TIQP_DATA, byte);
	return (0);
}

/*
 * Send the following command to the mouse, expecting a certain reply.
 */
int
tiqm_send_cmd(unsigned char cmd, unsigned char *buf, int bufsize)
{
	int	i, s, data, rval;

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("entered tiqm_send_cmd() cmd = %x\n", cmd);
#endif

	rval = 1;
	s = tiqm_interrupt_enable(0);	/* disable interrupts */
	tiqm_clear_interface();		/* clear any lingering error state */

	if (tiqm_send_byte(cmd)) {
		rval = 0;
		goto tiqm_sc_error;
	}

	for (i = 0; i < bufsize; i++) {
		data = tiqm_read_byte(0);
		if (data == FAILED) {
			rval = 0;
			goto tiqm_sc_error;
		}
		buf[i] = (unsigned char)data;
	}

tiqm_sc_error:
	tiqm_interrupt_enable(s);		/* restore interrupts */
	return (rval);
}

/*
 * send command byte to the TI mouse device. Expect bufsize bytes
 * in return from TI mouse and store them in buf. Verify that the
 * first byte read back is ans. If command fails or first byte read
 * back is SNDERR or SNDERR2, retry. Give up after two attempts.
 */

int
snd_tiq_cmd(cmd, ans, bufsize, buf)
register unsigned char	cmd;
register unsigned char	ans;
unchar *buf;
int bufsize;
{
	register int sndcnt = 0;
	int rv;

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("entered snd_tiq_cmd() cmd = %x\n", cmd);
#endif
	while (sndcnt < 2) {
		rv = tiqm_send_cmd(cmd, buf, bufsize);

		if (rv && ((ans == MSE_ANY) || (buf[0] == ans)))
			return (0); /* command succeeded, first byte matches */

		if (buf[0] == SNDERR || buf[0] == SNDERR2) {
#ifdef TIQMOUSE_DEBUG
			if (tiqmouse_debug)
				printf("snd_tiq_cmd() SNDERR\n");
#endif
			if (buf[0] == SNDERR2 || sndcnt++ > 1) {
#ifdef TIQMOUSE_DEBUG
				if (tiqmouse_debug)
					printf("snd_tiq_cmd() "
					    "FAILED two resends\n");
#endif
				return (FAILED);
			}
		} else
			return (FAILED);
	}
#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("leaving snd_tiq_cmd() \n");
#endif
	return (FAILED);
}


/* 320 mouse command execution function */
static int
tiqmfunc(struct cmd_320 *mca)
{
	register int	retflg = 0;
	unchar	buf[10];

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("entered tiqmfunc(): cmd = %x\n", mca->cmd);
#endif

	/* must turn mouse off if streaming mode set */
	if (mcastat.mode == MSESTREAM) {
		if (snd_tiq_cmd(MSEOFF, MSE_ACK, 1, buf) == FAILED) {
#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmfunc(): MSEOFF failed\n");
#endif
			return (FAILED);
		}
		if (mca->cmd == MSEOFF) { /* we just did requested cmd */
			return (0);
		}
	}

#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmfunc: doing switch statement\n");
#endif
	switch (mca->cmd & 0xff) {
		case MSESETDEF: /* these commands have no args */
		case MSEOFF:
		case MSEON:
		case MSESPROMPT:
		case MSEECHON:
		case MSEECHOFF:
		case MSESTREAM:
		case MSESCALE2:
		case MSESCALE1:
			if (snd_tiq_cmd(mca->cmd, MSE_ACK, 1, buf) == FAILED) {
				retflg = FAILED;
				break;
			}
			if (mca->cmd == MSESTREAM || mca->cmd == MSESPROMPT)
				mcastat.mode = mca->cmd;
			break;

		case MSECHGMOD:
			if (snd_tiq_cmd(mca->cmd, MSE_ACK, 1, buf) == FAILED) {
				retflg = FAILED;
				break;
			}
			/* received ACK. Now send arg */
#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmfunc: do arg1 of MSECHGMOD = %x\n", mca->arg1);
#endif
			if (snd_tiq_cmd(mca->arg1, MSE_ACK, 1, buf) == FAILED) {
				retflg = FAILED;
#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmfunc(): MSECHGMOD failed\n");
#endif
			}
			break;

		case MSERESET: /* expecting ACK and 2 add'tl bytes */
			if (snd_tiq_cmd(mca->cmd, MSE_ACK, 3, buf) == FAILED) {
				retflg = FAILED;
				break;
			}
			/*
			 * command succeeded and got ACK as first byte
			 * Now verify next two bytes.
			 */
			if (buf[1] != 0xaa || buf[2] != 0x00) {
				retflg = FAILED;
#ifdef TIQMOUSE_DEBUG
	if (tiqmouse_debug)
		printf("tiqmfunc(): MSERESET failed\n");
#endif
			}
			break;

		case MSEREPORT: /* expect ACK and then 3 add'tl bytes */
		case MSESTATREQ:
			if (snd_tiq_cmd(mca->cmd, MSE_ACK, 4, buf) == FAILED) {
				retflg = FAILED;
				break;
			}
			mca->arg1 = buf[1];
			mca->arg2 = buf[2];
			mca->arg3 = buf[3];
			retflg = 1;
			break;

		case MSERESEND: /* expect 3 bytes back. Don't care what */
				/* the first byte is */
			if (snd_tiq_cmd(mca->cmd, MSE_ANY, 3, buf) == FAILED) {
				retflg = FAILED;
				break;
			}
			mca->arg1 = buf[0];
			mca->arg2 = buf[1];
			mca->arg3 = buf[2];
			retflg = 1;
			break;

		case MSEGETDEV: /* expect 2 bytes back */
			if (snd_tiq_cmd(mca->cmd, MSE_ACK, 2, buf) == FAILED) {
				retflg = FAILED;
				break;
			}
			/* got an ACK, the second byte is the return val */
			mca->arg1 = buf[1];
			retflg = 1;
			break;

		case MSESETRES: /* cmd has one arg */
			if (snd_tiq_cmd(mca->cmd, MSE_ACK, 1, buf) == FAILED) {
				retflg = FAILED;
				break;
			}
			/*
			 * sent cmd successfully. Now send arg. If
			 * return val is not arg echoed back, fail the cmd
			 */
			if (snd_tiq_cmd(mca->arg1, MSE_ACK, 1, buf) == FAILED)
				retflg = FAILED;
			break;
		default:
#ifdef TIQMOUSE_DEBUG
			if (tiqmouse_debug)
				printf("tiqmfunc:SWITCH default \n");
#endif
			retflg = FAILED;
			break;
	}

	/*
	 * turn the mouse back on if we were streaming and cmd was not
	 * MSEOFF
	 */
	if (mcastat.mode == MSESTREAM) {
		if (mca->cmd != MSEOFF) {
			if (snd_tiq_cmd(MSEON, MSE_ACK, 1, buf) == FAILED) {
				retflg = FAILED;
#ifdef TIQMOUSE_DEBUG
			if (tiqmouse_debug)
				printf("tiqmfunc(): MSEON failed\n");
#endif
			}
		}
	}

	return (retflg);
}
