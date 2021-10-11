/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */
#pragma ident "@(#)el.c	1.10	96/06/03 SMI"

/*
 * el - 3COM 3C503 Ethernet Driver STREAMS based driver for the 3COM 3C503
 * Ethernet controller board. This driver depends on the existence of the
 * Generic LAN Driver utility functions in /kernel/misc/gld
 */

#include "sys/types.h"
#include "sys/errno.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/stropts.h"
#include "sys/stream.h"
#include "sys/kmem.h"
#include "sys/conf.h"
#include "sys/ddi.h"
#include "sys/devops.h"
#include "sys/sunddi.h"
#include "sys/ksynch.h"
#include "sys/dlpi.h"
#include "sys/ethernet.h"
#include "sys/strsun.h"
#include "sys/stat.h"
#include "sys/modctl.h"
#include "sys/kstat.h"
#include "sys/gld.h"
#include "sys/el.h"


/*
 * declarations of just about everything that gets used in
 * the driver.
 */

int	el_send (gld_mac_info_t *, mblk_t *), el_stop_board (gld_mac_info_t *);
int	el_start_board (gld_mac_info_t *), el_canwrite (gld_mac_info_t *);
int	el_prom (gld_mac_info_t *, int);
int	el_saddr (gld_mac_info_t *), el_reset (gld_mac_info_t *);
int	el_dlsdmult (gld_mac_info_t *, struct ether_addr *, int);
void	el_watchdog (gld_mac_info_t *);
u_int	elintr ();
static	eldevinfo (dev_info_t *, ddi_info_cmd_t, void *, void **);
extern	eldetach (dev_info_t *, ddi_detach_cmd_t);
extern	elattach (dev_info_t *, ddi_attach_cmd_t);
extern	elprobe (dev_info_t *);
static void el_NIC_reset (gld_mac_info_t * elp);
static void el_gstat (gld_mac_info_t *);
static void el_getp (gld_mac_info_t *);
static void el_init_board (gld_mac_info_t *);
static int elsetup (dev_info_t * devinfo, gld_mac_info_t * elp);
static int el_media_probe (gld_mac_info_t *elp, dev_info_t *devinfo);

#ifdef ELDEBUG
int	eldebug = 0x0;
#endif

/*
 * Standard streams initialization for a driver
 */
static struct module_info minfo = {
	0, "el", 0, INFPSZ, ELHIWAT, ELLOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, gld_close, NULL, &minfo, NULL
};

struct streamtab elinfo = { &rinit, &winit, NULL, NULL };

/*
 * Module linkage information for the kernel. This is standard
 * across most drivers.
 */

extern struct mod_ops mod_driverops;

/* define the "ops" structure for a STREAMS driver */
DDI_DEFINE_STREAM_OPS (elops, nulldev, elprobe, \
			elattach, eldetach, nodev, \
			eldevinfo, D_MP, &elinfo);

static char ident[] = "3COM 3C503 Driver";
static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,
	&elops			/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *) &modldrv, NULL
};

int
_init (void)
{
	return (mod_install (&modlinkage));
}

int
_fini (void)
{
	return (mod_remove (&modlinkage));
}

int
_info (struct modinfo * modinfop)
{
	return (mod_info (&modlinkage, modinfop));
}

DEPENDS_ON_GLD;			/* this forces misc/gld to load */

#define	nextsize(len) ((len+64)*2)

/* list of possible I/O port addresses */
static uint_t el_ioports[] = {
	0x250, 0x280, 0x2a0, 0x2e0, 0x300, 0x310, 0x330, 0x350
};

/* same list except in the order the bit mask has them */
static uint_t el_ioportmap[] = {
	0x2e0, 0x2a0, 0x280, 0x250, 0x350, 0x330, 0x310, 0x300
};

/* list of shared memory addresses as found in the config register */
static ulong_t el_baseaddr[] = {
	0, 0, 0, 0, 0xc8000, 0xcc000, 0xd8000, 0xdc000
};

/*
 * elprobe (devinfo)
 * the elprobe routine is called at system initialization
 * time in order to determine if any boards are present and what their
 * configuration(s) are.
 */

elprobe (dev_info_t * devinfo)
{
	int	port, i, j, addr;
	int regbuf[15], reglen, *ioaddr, iolen, ioaddrlen;
	struct regspec {
	   int unit;
	   int base;
	   int len;
	} *regspec;

#if defined(ELDEBUG)
	if (eldebug & ELDDI)
		printf ("elprobe(%x)\n", devinfo);
#endif
	reglen = sizeof(regbuf);
	if ((j=ddi_getlongprop_buf(DDI_DEV_T_NONE, devinfo, DDI_PROP_DONTPASS,
				"reg", (caddr_t)regbuf, &reglen)) != DDI_PROP_SUCCESS) {
		return (DDI_PROBE_FAILURE);
	}
	if (ddi_getlongprop (DDI_DEV_T_ANY, devinfo,
				DDI_PROP_DONTPASS, "ioaddr",
				(caddr_t) &ioaddr, &iolen) != DDI_PROP_SUCCESS) {
	   ioaddr = (int *)el_ioports;	/* default case */
	   iolen = sizeof (el_ioports);
	   ioaddrlen = 0;
	} else
	  ioaddrlen = iolen;
	iolen /= sizeof (int);

	/*
	 * probe for boards being present
	 */
	for (i = 0; i < iolen; i++) {
		uchar_t addrbuff[8];
		uchar_t csum;
		uchar_t ga;
		uchar_t ctl;
		uchar_t io;

		port = ioaddr[i];
		if (port == 0)
		  continue;

		/* reset controller, etc. */
		outb (port + ELCTRL, ELARST | ELAXCVR);
		outb (port + ELCTRL, 0);
		ctl = inb (port + ELCTRL);
		outb (port + ELCTRL, ELAEALO);
		/* get address bytes */
		for (csum = j = 0; j < ETHERADDRL + 2; j++)
			csum += addrbuff[j] = inb (port + j);
		if (csum == 0xFF || addrbuff[0] & 1) {
			/* must be a SMC/compatible or invalid address */
			continue;
		}
				/* it is possible, test some more */
		ga = inb (port + ELGACFR);
		io = inb (port + ELBCFR);
		if (io == 0 || ga != 0 || (ctl != 0xA && ctl != 0xB) ||
		    el_ioportmap[ddi_ffs (io) - 1] != port) {
				/* must be something else */
			continue;
		}
		addr = el_baseaddr[ddi_ffs(inb(port+ELPCFR))-1];
		/* we have to be in the .conf file for now */
		for (j=0, regspec = (struct regspec *)regbuf;
		     j < reglen/sizeof(struct regspec); j++) {
		   if (addr == regspec[j].base)
		     break;
		}
		if (j >= (reglen/sizeof(struct regspec)))
		  continue;

		/* passed the tests so we must have a board */
		ddi_set_driver_private (devinfo,
					(caddr_t) (port | (j << 16)));

		for (j=0; j < (sizeof (el_ioports)/sizeof (int)); j++)
		  if (port == el_ioports[j]) {
		     break;
		  }

		/* was this a valid I/O addr for this devinfo ??? */
		if (j >= (sizeof (el_ioports)/sizeof (int))){
		   break;
		}

		if (ioaddrlen != 0)
		  	kmem_free (ioaddr, ioaddrlen);
		return (DDI_PROBE_SUCCESS);
	}
	if (ioaddrlen != 0)
	  	kmem_free (ioaddr, ioaddrlen);
	return (DDI_PROBE_FAILURE);
}

/*
 * elattach (devinfo, cmd)
 * called at boot time to do any initialization needed. It is
 * called with a devinfo tree and a command for each board found during the
 * probe calls. If multiple boards exist, they must get handled at this time.
 * Full board configuration should be determined at this time.
 */
elattach (dev_info_t * devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *elp;
	register i;
	struct elvar *elvar;

#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("elattach()\n");
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	elp = (gld_mac_info_t *)
		kmem_zalloc (sizeof (gld_mac_info_t) +
				sizeof (struct elvar), KM_NOSLEEP);
	elvar = (struct elvar *) (elp + 1);
	elp->gldm_private = (caddr_t) elvar;

	/* probe saved the I/O port in private now allocate the real private */
	elp->gldm_port = ((long) ddi_get_driver_private (devinfo)) & 0xFFFF;
	elp->gldm_reg_index = ((long) ddi_get_driver_private (devinfo)) >> 16;

	i = elsetup (devinfo, elp);

	if (i == DDI_FAILURE) {
		kmem_free ((caddr_t) elp,
			sizeof (gld_mac_info_t) + sizeof (struct elvar));
		return (DDI_FAILURE);
	}

	/* don't allow a probe to find this again */
	for (i=0; i < (sizeof (el_ioports)/sizeof (int)); i++)
	  if (elp->gldm_port == el_ioports[i]) {
	     el_ioports[i] = 0;
	     break;
	  }

	/*
	 * setup pointers to device specific functions which will be used by
	 * the generic layer.
	 */
	elp->gldm_reset = el_reset;
	elp->gldm_stop = el_stop_board;
	elp->gldm_start = el_start_board;
	elp->gldm_saddr = el_saddr;
	elp->gldm_send = el_send;
	elp->gldm_prom = el_prom;
	elp->gldm_sdmulti = el_dlsdmult;
	elp->gldm_intr = elintr;

	/*
	 * now setup board characteristics for inforeq, etc.
	 */
	elp->gldm_ident = ident;
	elp->gldm_type = DL_ETHER;
	elp->gldm_maxpkt = ELMAXPKT;
	elp->gldm_minpkt = 0;
	elp->gldm_addrlen = ETHERADDRL;
	elp->gldm_saplen = -2;
	bcopy ((caddr_t) gldbroadcastaddr,
		(caddr_t) elp->gldm_broadcast, ETHERADDRL);
	bcopy ((caddr_t) elp->gldm_vendor,
		(caddr_t) elp->gldm_macaddr, ETHERADDRL);

	(void) el_saddr(elp);

	elp->gldm_state = ELB_IDLE;	/* set board specific state */

	elp->gldm_irq_index = 0; /* always zero */

	/* make sure system knows about us */
	return (gld_register (devinfo, "el", elp));
}

/*
 * elsetup (devinfo, elp)
 * setup the board we found.  This does the bulk of the autoconfigure
 * work necessary for this type of board.
 */
static int
elsetup (dev_info_t * devinfo, gld_mac_info_t * elp)
{
	int	i, regbuf[3];
	uchar_t tst;
	struct elvar *elvar = (struct elvar *) elp->gldm_private;
	char	media[32];

	regbuf[0] = elp->gldm_port;
	(void) ddi_prop_create (DDI_DEV_T_NONE, devinfo, DDI_PROP_CANSLEEP,
				"ioaddr", (caddr_t) regbuf, sizeof (int));
	/*
	 * get configuration information from the Ga configuration
	 */
	(void) inb (elp->gldm_port + ELPCFR);	/* PROM configuration */

	outb (elp->gldm_port + ELGACFR, ELGRSEL | elvar->el_curbank);
	tst = inb (elp->gldm_port + ELGACFR);

	outb (elp->gldm_port + ELCTRL, 0); /* make sure talking to NIC */
	tst = inb (elp->gldm_port + ELDCR);
	outb (elp->gldm_port + ELCR, ELCDMA|ELCSTP);
	outb (elp->gldm_port + ELDCR, tst & 0x7e); /* make Ga scan and set if 16 */
	outb (elp->gldm_port + ELCR, ELCPG2|ELCDMA|ELCSTP);
	tst = inb (elp->gldm_port + ELDCR);

	outb (elp->gldm_port + ELCR, ELCPG0|ELCDMA);

	if ((tst & 1) == 0)
	  elvar->el_memsize = 8192 >> 13;
	else
	  elvar->el_memsize = 16384 >> 13; /* also means a 16-bit board */
	if (elvar->el_memsize == 1) {
		elvar->el_rcvstart = RCV_START8K;
		elvar->el_nxtpkt   = RCV_START8K;
		elvar->el_curbank  = 1;	/* for 8K always in bank 1. */
	} else {
		elvar->el_rcvstart = RCV_START16K;
		elvar->el_nxtpkt   = RCV_START16K;
		elvar->el_curbank  = 0;	/* for other sizes always start in 0. */
	}
	outb (elp->gldm_port + ELGACFR, ELGRSEL | elvar->el_curbank);

	/*
	 * get the IRQ we need to use, if none, we'll need to do something
	 */
	i = 2 * sizeof (long);
	if (ddi_getlongprop_buf (DDI_DEV_T_NONE, devinfo, DDI_PROP_DONTPASS,
			"intr", (caddr_t) regbuf, &i) == DDI_PROP_SUCCESS) {
		elvar->el_irq = regbuf[1];
		if (elvar->el_irq == 9)
			elvar->el_irq = 2;
	}

	if ((i=ddi_getprop (DDI_DEV_T_NONE, devinfo,
			    	DDI_PROP_DONTPASS, "share", 0)) ==
						DDI_PROP_SUCCESS) {
		if (i) {
			elvar->el_ifopts |= ELASHAR;
		}
	} else {
		elvar ->el_ifopts |= ELASHAR;
	}

	/* get the MAC address -- vendor provided */
	outb (elp->gldm_port + ELCTRL, ELAEALO);
	/* get address bytes */
	for (i = 0; i < ETHERADDRL; i++)
		elp->gldm_vendor[i] = inb (elp->gldm_port + i);

	/*
	 * find out what media has been specified in the el.conf file
	 */
	i = sizeof (media) - 1;
	if (ddi_getlongprop_buf (DDI_DEV_T_NONE, devinfo, DDI_PROP_DONTPASS,
					"media", media, &i) ==
						DDI_PROP_SUCCESS) {
		if (strcmp (media, "thick") == 0 ||
		    strcmp (media, "dix") == 0)
		  	elp->gldm_media = GLDM_AUI;
		else if (strcmp (media, "tp") == 0)
			elp->gldm_media = GLDM_TP;
		else if (strcmp (media, "thin") == 0 ||
			 strcmp (media, "bnc") == 0)
		  	elp->gldm_media = GLDM_BNC;
		if (elp->gldm_media == GLDM_AUI)
			elvar->el_ifopts = 0;
		else
			elvar->el_ifopts = ELAXCVR;
	} else {
	   	i = el_media_probe (elp, devinfo);
		if (i == GLDM_BNC)
			elp->gldm_media = GLDM_BNC;
		else
		  	elp->gldm_media = GLDM_AUI;
	}
	if (elp->gldm_media == GLDM_AUI)
	  	elvar->el_ifopts = 0;
	else
	  	elvar->el_ifopts = ELAXCVR;

	return (DDI_SUCCESS);
}

/*
 * eldevinfo (devinfo, cmd, arg, result)
 * standard kernel devinfo lookup function
 */
/*ARGSUSED*/
static int
eldevinfo (dev_info_t * devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;

	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (devinfo == NULL) {
			error = DDI_FAILURE;
		} else {
			/* we really shouldn't be looking at devinfo here */
			*result = (void *) devinfo;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *) 0;
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}

/*
 * eldetach (devinfo, cmd)
 * used to unload the driver when not needed
 * called once for each dev_info node associated with the driver
 */
eldetach (dev_info_t * devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *elp;

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/*
	 * elminate the hardware specifics from the kernel
	 * that is, no more shared memory or interrupt handler
	 */
	elp = (gld_mac_info_t *) ddi_get_driver_private (devinfo);
	(*elp->gldm_stop) (elp); /* stop the board if it is running */

	/* now have GLD remove us from its structures */
	if (gld_unregister (elp) == DDI_SUCCESS) {
		kmem_free ( (caddr_t) elp,
			   sizeof (gld_mac_info_t) + sizeof (struct elvar));
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}

/*
 * el_init_board (elp)
 * initialize the ethernet board specified by the elp (macinfo)
 */
static void
el_init_board (gld_mac_info_t * elp)
{
	static	elirq[ELMAXIRQ + 1] = {	/* IRQ bits */
		0, 0, 0x10, 0x20, 0x40, 0x80
	};
	unchar	ch, baseconf, promconf, revcode;
	register long elbase = elp->gldm_port;	/* base of board registers */
	register int i;
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("el_init_board(%x)\n", elp);
#endif
	baseconf = inb (elbase + ELBCFR);	/* read jumper settings */
	promconf = inb (elbase + ELPCFR);
	revcode = inb (elbase + ELSTREG);
#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("base = %x, prom = %x, revcode = %x\n",
				baseconf, promconf, revcode);
#endif
	/*
	 * Initialize the gate array (at elbase + 0x400) reset controller
	 * turn off reset select addr prom
	 */
	outb (elbase + ELCTRL, (elvar->el_ifopts | ELARST));
	outb (elbase + ELCTRL, elvar->el_ifopts);
	outb (elbase + ELCTRL, (elvar->el_ifopts | ELAEALO));

	/*
	 * Get my ethernet address and compare against the expected value.
	 * Only do this the first time, because the user may have specified a
	 * new ethernet address and we don't want to overwrite it.
	 */
	for (i = 0; i < ETHERADDRL; i++) {
		elp->gldm_macaddr[i] = inb (elbase + i);
	}

	outb (elbase + ELCTRL, elvar->el_ifopts); /* select onboard transceiver */
	outb (elbase + ELPSTR, elvar->el_rcvstart); /* recv page start = 1536 */
	outb (elbase + ELPSPR, RCV_STOP);	/* recv page stop = 8192 */

	if (elvar->el_irq > ELMAXIRQ || elvar->el_irq < ELMINIRQ) {
		printf ("elinit: bad IRQ for 3C503 board: %d\n", elvar->el_irq);
		elp->gldm_state = ELB_ERROR;
	} else {
		ch = elirq[elvar->el_irq];
		outb (elbase + ELIDCFR, ch);
		outb (elbase + ELGACFR, (ELGRSEL | elvar->el_curbank));
	}
	/*
	 * Initialize the DP8390 NIC (at elbase)
	 */
	outb (elbase + ELCR, (ELCPG0 | ELCDMA | ELCSTP));
	outb (elbase + ELDCR, 0x48);
	outb (elbase + ELTCR, ELTLOOP);
	outb (elbase + ELRCR, ELRMON);
	outb (elbase + ELPSTART, elvar->el_rcvstart);
	outb (elbase + ELPSTOP, RCV_STOP);
	outb (elbase + ELBNDY, RCV_STOP - 1);
	outb (elbase + ELCR, (ELCPG1 | ELCDMA | ELCSTP));
	outb (elbase + ELCURR, elvar->el_rcvstart);
	outb (elbase + ELCR, (ELCPG0 | ELCDMA | ELCSTP));

	/* initialize multicast address	 support */
	outb (elbase + ELCTRL, elvar->el_ifopts);
	ch = inb (elbase + ELCR);
	ch &= ELPGMSK;
	outb (elbase + ELCR, (ELCPG1 | ch));
	for (i = 0; i < 8; i++)
		outb (elbase + ELMADR + i, 0);
	outb (elbase + ELCR, (ELCPG0 | ch));
}


/*
 * el_reset (elp)
 * reset the board to initial state Save the machine address and
 * restore it afterwards.
 */
int
el_reset (gld_mac_info_t * elp)
{
	unchar	macaddr[ETHERADDRL];

	(void) el_stop_board (elp);
	bcopy ((caddr_t) elp->gldm_macaddr, (caddr_t) macaddr, ETHERADDRL);
	el_init_board (elp);
	bcopy ((caddr_t) macaddr, (caddr_t) elp->gldm_macaddr, ETHERADDRL);
	return (0);
}


/*
 * el_saddr (elp)
 * set the ethernet address on the board
 */
int
el_saddr (gld_mac_info_t * elp)
{
	register int i;
	register long elbase = elp->gldm_port;	/* base of board registers */
	register char ch;
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

	if (elp->gldm_state == ELB_ERROR)
		return (0);

	outb (elbase + ELCTRL, elvar->el_ifopts);	/* select NIC */
	ch = inb (elbase + ELCR);
	ch &= ELPGMSK;
	outb (elbase + ELCR, (ELCPG1 | ch) & ~ELCTXP);	/* select page 1 */
	/*
	 * put station address in controller
	 */
	for (i = 0; i < ETHERADDRL; i++)
		outb (elbase + ELPADR + i, elp->gldm_macaddr[i]);

	outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);	/* select page 0 */
	return (0);
}

/*
 * el_dlsdmult (elp, mcast, op)
 * set(enable) or disable a multicast address on the interface
 */
int
el_dlsdmult (gld_mac_info_t * elp, struct ether_addr * mcast, int op)
{
	register long elbase = elp->gldm_port;
	int	row, col;
	unsigned char val;
	unsigned char ch, i;
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

	if (elp->gldm_state == ELB_ERROR)
		return (0);

	/* calculate row/col here */
	i = gldcrc32 (mcast->ether_addr_octet) >> 26;
	row = i / 8;
	col = i % 8;
	outb (elbase + ELCTRL, elvar->el_ifopts);
	ch = inb (elbase + ELCR);
	ch &= ELPGMSK;
	outb (elbase + ELCR, (ELCPG1 | ch));
	val = inb (elbase + ELMADR + row);
	if (op == 1) {
		val |= 0x01 << col;
		elvar->el_mcastcnt[i]++;
	} else {
		/* only turn off if last stream with it on */
		if (--elvar->el_mcastcnt[i] == 0)
			val &= ~(0x01 << col);
	}
	outb (elbase + ELMADR + row, val);
	outb (elbase + ELCR, (ELCPG0 | ch));
	return (0);
}

/*
 * elintr (elp)
 * interrupt routine from board to inform us that a receive or
 * transmit has completed.
 */
u_int
elintr (gld_mac_info_t * elp)
{
	register long elbase;	/* base of board registers */
	register unchar ch;
	unchar orig;
	unchar	chc, chr;
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("elintr(%x)\n", elp);
#endif

	elp->gldm_stats.glds_intr++;

	elbase = elp->gldm_port;
	outb (elbase + ELCTRL, elvar->el_ifopts);	/* select NIC */
	chc = inb (elbase + ELISR);	/* get interrupt status */

	if (!(chc & ISRMASK)){
	  	/* wasn't for us */
		return (0);
	}

	outb (elbase + ELIMR, 0);	/* turn off interrupts */

	orig = ch = inb (elbase + ELCR);
	ch &= ELPGMSK;
	outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);	/* select page 0 */
	outb (elbase + ELISR, chc);	/* reset interrupts */

	if (chc & ELPTXE) {
#ifdef ELDEBUG
		if (eldebug & ELINT)
			printf ("elintr: xmit complete %x\n", chc);
#endif
	}

	/* receive error */
	if (chc & ELRXEE) {
		chr = inb (elbase + ELRSR); 	/* get RECV status */
		if (chr & ELRFO)		/* fifo overrun */
			elp->gldm_stats.glds_overflow++;
#ifdef ELDEBUG
		if (eldebug & ELERRS)
			printf ("elintr: receive error %x %x\n", chc, chr);
#endif
		elp->gldm_stats.glds_errrcv++;
	}
	/* transmit error */
	if (chc & ELTXEE) {
		chr = inb (elbase + ELTSR);	/* get XMIT status */
		elp->gldm_state = ELB_WAITRCV;
		if (chr & ELTABT)
			elp->gldm_stats.glds_excoll++;
		if (chr & ELTFU)
			elp->gldm_stats.glds_underflow++;
		if (chr & ELTCRS)
			elp->gldm_stats.glds_nocarrier++;
#ifdef ELDEBUG
		if (eldebug & ELERRS)
			printf ("elintr: transmit error %x %x\n", chc, chr);
#endif
		if (chr & ELTCOL) {
			/* get number of collisions */
			chr = inb (elbase + ELNCR);
			elp->gldm_stats.glds_collisions += chr;
		}
		if (chr & (ELTABT|ELTFU|ELTCRS))
			elp->gldm_stats.glds_errxmt++;
	}
	/* overwrite warning */
	if (chc & ELOVWE) {
		el_NIC_reset (elp);
#ifdef ELDEBUG
		if (eldebug & ELERRS)
			printf ("elintr: overwrite warning %x\n", chc);
#endif
	}
	/* counter overflow */
	if (chc & ELCNTE) {
		el_gstat (elp);	/* read and reset counters */
#ifdef ELDEBUG
		if (eldebug & ELINT)
			printf ("elintr: counter overflow %x\n", chc);
#endif
	}
	/* remote dma complete */
	if (chc & ELRDCE) {
#ifdef ELDEBUG
		if (eldebug & ELERRS)
			printf ("elintr: remote dma complete %x\n", chc);
#endif
	}
	/* receive completed */
	if (chc & ELPRXE)
		el_getp (elp);

	/* transmit completed */
	if (chc & (ELPTXE|ELTXEE)) {
		elp->gldm_state = ELB_WAITRCV;
		if (elvar->el_watch) {
			mutex_exit (&elp->gldm_maclock);
			(void) untimeout(elvar->el_watch);
			mutex_enter (&elp->gldm_maclock);
			elvar->el_watch = 0;
		}
	}

	/* enable interrupts */
	outb (elbase + ELIMR, (ELPRXE | ELPTXE | ELTXEE | ELOVWE | ELCNTE));

	/* if we were in the middle of a transmit, make sure it runs */
	if (elp->gldm_state == ELB_XMTBUSY){
		outb(elbase + ELCR, orig);
	} else
		outb(elbase + ELCR, orig & ~ELCTXP);
	return (1);
}

/*
 * el_getp (elp)
 * read an ethernet packet in from the 3C503 board. Called by the
 * device interrupt handler to process an incoming packet.  Store the packet
 * in a mblk_t and call gld_recv to queue it for the appropriate service
 * routines.
 */
static void
el_getp (gld_mac_info_t * elp)
{
	unchar	ch;
	register int i;
	int	length;
	mblk_t *mp = NULL;
	register long elbase = elp->gldm_port;	/* base of board registers */
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("el_getp(%x)\n", elp);
#endif

	ch = inb (elbase + ELRSR);	/* get recv status */
	/* we don't have a well-formed packet */
	if ((ch & ELRPRX) == 0) {
#ifdef ELDEBUG
		if (eldebug & ELERRS)
			printf ("el_getp: errors detected: %x\n", ch);
#endif
		if (ch & ELRFO)
			elp->gldm_stats.glds_overflow++;
		return;
	}
	ch = inb (elbase + ELCR);
	ch &= ELPGMSK;
	outb (elbase + ELCR, (ELCPG1 | ch) & ~ELCTXP);	/* select page 1 */
#ifdef ELDEBUG
	if (eldebug & ELRECV)
		printf ("el_getp: nxtpkt=%x, current page=%x\n",
			elvar->el_nxtpkt, inb (elbase + ELCURR));
#endif

	while (elvar->el_nxtpkt != inb (elbase + ELCURR)) {
		rcv_buf_t *rp;
		caddr_t dp, cp;
		caddr_t rcv_stop = elp->gldm_memp + 8192;
		int	cnt;

		/* compute ptr to packet header */
		if (!el_inbank (elvar->el_nxtpkt, elvar->el_curbank)) {
		   elvar->el_curbank ^= 1; /* only two banks */
		   outb (elbase + ELGACFR, ELGRSEL | elvar->el_curbank);
		}
		rp = (rcv_buf_t *) (elp->gldm_memp +
				    ((elvar->el_nxtpkt & 0x1f) << 8));
#ifdef ELDEBUG
		if (eldebug & ELRECV)
			printf ("el_getp: rp=%x, stat=%x, nxtpg=%x, len=%d\n",
				rp, rp->status, rp->nxtpg, rp->datalen);
#endif
		/*
		 * Test for NIC receiver anomoly.  Bytes will be shifted left
		 * and status lost.  Detect by comparing current + datalen =
		 * nxtpg. Entire buffer must be discarded and the NIC
		 * restarted.
		 */
		i = elvar->el_nxtpkt + (rp->datalen >> 8) + 1;
		if (i >= RCV_STOP)
			i += elvar->el_rcvstart - RCV_STOP;
		if (i != rp->nxtpg) {
			i++;
			if (i >= RCV_STOP)
				i += elvar->el_rcvstart - RCV_STOP;
			if (i != rp->nxtpg) {
#ifdef ELDEBUG
				if (eldebug & ELERRS)
					printf ("el_getp: receiver anomoly: byte shift: %x %x\n",
						i, rp->nxtpg);
#endif
				elvar->el_nxtpkt = elvar->el_rcvstart;
				el_NIC_reset (elp);
				break;
			}
		}
		/*
		 * Test for NIC receiver anomoly.  All bytes have bits
		 * shifted. Detect by checking for DFR or DIS bits being on
		 * in status byte. Discard the packet.	Other packets in the
		 * buffer should be good. NXTPG will be good.
		 */
		if ((rp->status & 0xc0) != 0) {
			elvar->el_nxtpkt = rp->nxtpg;
			/* skip packet */
			outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);
			if (elvar->el_nxtpkt <= elvar->el_rcvstart)
				outb (elbase + ELBNDY, RCV_STOP - 1);
			else
				outb (elbase + ELBNDY, elvar->el_nxtpkt - 1);
			outb (elbase + ELCR, (ELCPG1 | ch) & ~ELCTXP);
#ifdef ELDEBUG
			if (eldebug & ELERRS)
				printf ("el_getp: receiver anomoly: bit shift: %x\n",
					rp->status);
#endif
			continue;
		}
		/*
		 * did the hardware get confused and run off the end?
		 */
		if ((elvar->el_nxtpkt = rp->nxtpg) > (RCV_STOP - 1)) {
#ifdef ELDEBUG
			if (eldebug & ELERRS)
				printf ("Bad nxtpkt value: %x\n", elvar->el_nxtpkt);
#endif
			break;
		}
		/* length of packet w/o CRC field */
		length = rp->datalen - 4;
		if (length < ELMINSEND || length > ELMAXPKT + GLD_EHDR_SIZE) {
			/* skip packet */
			outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);
			if (elvar->el_nxtpkt <= elvar->el_rcvstart)
				outb (elbase + ELBNDY, RCV_STOP - 1);
			else
				outb (elbase + ELBNDY, elvar->el_nxtpkt - 1);
			outb (elbase + ELCR, (ELCPG1 | ch) & ~ELCTXP);
#ifdef ELDEBUG
			if (eldebug & ELERRS)
				printf ("el_getp: received bad packet size: %x\n", length);
#endif
			/* bundle short and long together */
			elp->gldm_stats.glds_short++;
			continue;
		}
		/* get buffer to put packet in */
		if ((mp = allocb (length, BPRI_MED)) == NULL) {
			/* skip packet */
			outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);
			if (elvar->el_nxtpkt <= elvar->el_rcvstart)
				outb (elbase + ELBNDY, RCV_STOP - 1);
			else
				outb (elbase + ELBNDY, elvar->el_nxtpkt - 1);
			outb (elbase + ELCR, (ELCPG1 | ch) & ~ELCTXP);
			elp->gldm_stats.glds_norcvbuf++;
#ifdef ELDEBUG
			if (eldebug & ELERRS)
				printf ("el_getp: no buffers (%d)\n", length);
#endif
			continue;
		}
		dp = (caddr_t) mp->b_wptr;	/* dp is data dest */
		cp = (caddr_t) & rp->pkthdr;	/* rp is packet hdr */

		mp->b_wptr = mp->b_rptr + length;

		/*
		 * See if there is a wraparound. If there is remove the
		 * packet from its start to the end of the receive buffer,
		 * set cp to the start of the receive buffer and remove the
		 * rest of the packet. Otherwise, remove the entire packet
		 * from the given location.
		 */

		if (cp + length >= rcv_stop) {	/* copy the start of packet */
			cnt = (int) rcv_stop - (int) cp;
			bcopy (cp, dp, cnt);
			length -= cnt;
			/* need to determine if bank switching is necessary */
			if (elvar->el_memsize > 1) {
				/* it is needed so do it */
			   elvar->el_curbank ^= 1; /* toggle the bank (only 2) */
			   if (elvar->el_curbank == 0) {
				   /* leave room for transmit buffer */
				   cp = elp->gldm_memp + ELTXBUFLEN;
			   }else
			        cp = elp->gldm_memp;
			   outb (elbase + ELGACFR, ELGRSEL | elvar->el_curbank);
			} else {
				/* it isn't needed so just move pointers */
				cp = elp->gldm_memp + ELTXBUFLEN;
			}
			dp += cnt;
		}
		bcopy (cp, dp, length);	/* copy the end of packet */
		/* move boundary pointer forward */
		outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);
		if (elvar->el_nxtpkt <= elvar->el_rcvstart)
			outb (elbase + ELBNDY, RCV_STOP - 1);
		else
			outb (elbase + ELBNDY, elvar->el_nxtpkt - 1);
		outb (elbase + ELCR, (ELCPG1 | ch) & ~ELCTXP);
		gld_recv (elp, mp);
	}
	outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);	/* select page 0 */
}


/*
 * el_gstat (elp)
 * get board statistics.  Update the statistics structure from the
 * board statistics registers.
 */
static void
el_gstat (gld_mac_info_t * elp)
{
	register long elbase = elp->gldm_port;	/* base of board registers */
	register ch;
	int	cnt0, cnt1;
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

	outb (elbase + ELCTRL, elvar->el_ifopts);	/* select NIC */
	ch = inb (elbase + ELCR);
	ch &= ELPGMSK;
	outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);	/* select page 0 */
	/* get frame align errors */
	cnt0 = inb (elbase + ELCNTR0);
	elp->gldm_stats.glds_frame += cnt0;
	/* get CRC errors */
	cnt1 = inb (elbase + ELCNTR1);
	elp->gldm_stats.glds_crc += cnt1;
	elp->gldm_stats.glds_errrcv += cnt0+cnt1;
	/* get missed packets */
	elp->gldm_stats.glds_overflow += inb (elbase + ELCNTR2);
}


/*
 * el_send (lld, mp)
 * called when a packet is ready to be transmitted. A pointer to an
 * M_DATA message that contains the packet is passed to this
 * routine as a parameter.
 */
int
el_send (gld_mac_info_t * elp, mblk_t * mp)
{
	register int len = 0;
	register unsigned length;
	unsigned char ch, *txbuf;
	register long	elbase = elp->gldm_port; /* base of board registers */
	register struct elvar *elvar = (struct elvar *) elp->gldm_private;
#ifdef ELDEBUG

	if (eldebug & ELTRACE)
		printf ("el_send(%x %x)\n", elp, mp);
#endif
	if (!el_canwrite (elp)) {	/* see if the board is busy */
#ifdef ELDEBUG
		if (eldebug & ELSEND)
			printf ("el_send: board busy\n");
#endif
		return (1);
	}

	/*
	 * are we in the right bank of memory?
	 * need to be in bank 1 for 8K boards and 0 for 16K boards
	 * we don't need to switch back later since el_getp() will
	 * always test for being in the correct bank.
	 */
	if (elvar->el_memsize > 1 && elvar->el_curbank != 0) {
	   	elvar->el_curbank = 0; /* force it */
		outb (elbase + ELGACFR, ELGRSEL | elvar->el_curbank);
	}

	/*
	 * load the packet header onto the board
	 */
#ifdef ELDEBUG
	if (eldebug & ELSEND) {
	  	int i;
	  	len = (int) MBLKL(mp);
		printf ("el_send: machdr=<");
		for (i = 0; i < len; i++)
			printf ("%s%x", (i == 0) ? "" : ":", *(mp->b_rptr + i));
		printf (">\n");
		printf ("el_send: (1) copy from %x, len = %x\n",
				mp->b_rptr, len);
	}
#endif
	txbuf = (unsigned char *) elp->gldm_memp;
	length = 0;

	/* load the packet header onto the board */

#ifdef ELDEBUG
	if (eldebug & ELSEND)
		printf ("el_send: bcopy(%x, %x, %x) base=%x\n",
			mp->b_rptr, txbuf,
				len, elp->gldm_memp);
#endif

	/*
	 * load the rest of the packet onto the board by chaining through the
	 * M_DATA blocks attached to the M_PROTO header. The list of data
	 * messages ends when the pointer to the current message block is
	 * NULL.
	 */
	do {

		len = (int) (mp->b_wptr - mp->b_rptr);
#ifdef ELDEBUG
		if (eldebug & ELSEND)
			printf ("el_send: (2) bcopy(%x, %x, %x) base=%x\n",
				mp->b_rptr,
				txbuf + length, len, elp->gldm_memp);
#endif

		bcopy ((caddr_t) mp->b_rptr, (caddr_t) txbuf + length, len);
		length += (unsigned int) len;
		mp = mp->b_cont;
	} while (mp != NULL);

	elvar->el_watch = timeout (el_watchdog, (caddr_t) elp, hz * 4);

	if (length < ELMINSEND)	/* pad packet length if needed */
		length = ELMINSEND;

	outb (elbase + ELCTRL, elvar->el_ifopts);	/* select NIC */
	ch = inb (elbase + ELCR);
	ch &= ELPGMSK;		/* make sure NIC is started */
	ch &= ~ELCSTP;
	ch |= ELCSTA;
	outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);	/* select page 0 */
	outb (elbase + ELTCR, 0); /* normal transmit */
	if (elvar->el_memsize == 1)
	  outb (elbase + ELTPSR, XMT_START8K); /* transmit start page */
	else
	  outb (elbase + ELTPSR, XMT_START16K); /* transmit start page */
	outb (elbase + ELTBCR0, LOW (length)); /* set packet length */
	outb (elbase + ELTBCR1, HIGH (length));
	outb (elbase + ELCR, (ELCTXP | ELCPG0 | ch)); /* start transmitter */
#ifdef ELDEBUG
	if (eldebug & ELSEND) {
		printf ("el_send: length=%d\n", length);
	}
#endif

#ifdef ELDEBUG
	if (eldebug & ELSEND)
		printf ("el_send:...\n");
#endif
	return (0);
}


/*
 * el_canwrite (lld)
 * check to see if board is already transmitting; if not,
 * reserve it.
 */
int
el_canwrite (gld_mac_info_t *elp)
{

#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("el_canwrite(%x)\n", elp);
#endif
	if (elp->gldm_state == ELB_ERROR)
		return (0);
	/* lock out an interrupt handler */
	if (elp->gldm_state == ELB_XMTBUSY) {
		return (0);
	} else {
		/* buffer is free; claim it and return true */
		elp->gldm_state = ELB_XMTBUSY;
		return (1);
	}
}


/*
 * el_prom (elp, on)
 * set promiscuous mode on 3C503 board
 */
int
el_prom (gld_mac_info_t *elp, int on)
{
	register long elbase = elp->gldm_port;	/* base of board registers */
	register char ch;
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("el_prom(%x)\n", elp);
#endif
	if (elp->gldm_state == ELB_ERROR)
		return (0);

	outb (elbase + ELCTRL, elvar->el_ifopts);	/* select NIC */
	ch = inb (elbase + ELCR);
	ch &= ELPGMSK;
	outb (elbase + ELCR, (ELCPG0 | ch) & ~ELCTXP);	/* select page 0 */
	if (on) {		/* set receiver modes */
		elp->gldm_flags |= GLD_PROMISC;
		/* promiscuous mode enabled */
		outb (elbase + ELRCR, (ELRPRO | ELRAB | ELRAM));
	} else {
		outb (elbase + ELRCR, (ELRAB | ELRAM));
		elp->gldm_flags &= ~GLD_PROMISC;
	}
	return (0);
}


/*
 * el_start_board (elp)
 * start 3C503 board receiver
 */
int
el_start_board (gld_mac_info_t * elp)
{
	register long elbase = elp->gldm_port;	/* base of board registers */
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("el_start_board(%x)\n", elp);
#endif
	if (elp->gldm_state == ELB_ERROR) {
		printf ("3C503 board not present or in error\n");
		return (0);
	}
	elp->gldm_state = ELB_WAITRCV;
	outb (elbase + ELCTRL, elvar->el_ifopts);	/* select NIC */
	/* select page 0 */
	outb (elbase + ELCR, (ELCPG0 | ELCDMA | ELCSTP));
	outb (elbase + ELISR, 0xff);	/* reset interrupts */
	/* interrupts must be on */
	outb (elbase + ELIMR, (ELPRXE | ELPTXE | ELTXEE | ELOVWE | ELCNTE));
	(void) inb (elbase + ELCNTR0);	/* zero frame align errors */
	(void) inb (elbase + ELCNTR1);	/* zero CRC errors */
	(void) inb (elbase + ELCNTR2);	/* zero missed packets */
	/* turn off reset */
	outb (elbase + ELCR, (ELCPG0 | ELCDMA));
	/* start receiver */
	outb (elbase + ELCR, (ELCPG0 | ELCDMA | ELCSTA));
	outb (elbase + ELTCR, 0);	/* normal transmit */

	/* set receiver modes */
	if (elp->gldm_flags & GLD_PROMISC)
		/* promiscuous mode */
		outb (elbase + ELRCR, (ELRPRO | ELRAB | ELRAM));
	else
		outb (elbase + ELRCR, (ELRAB | ELRAM));
	return (0);
}


/*
 * el_stop_board (elp)
 * stop board receiver
 */
int
el_stop_board (gld_mac_info_t * elp)
{
	register long elbase = elp->gldm_port;	/* base of board registers */
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

#ifdef ELDEBUG
	if (eldebug & ELTRACE)
		printf ("el_stop_board(%x)\n", elp);
#endif
	outb (elbase + ELCTRL, elvar->el_ifopts);	/* select NIC */
	/* select page 0 */
	outb (elbase + ELCR, (ELCPG0 | ELCDMA | ELCSTP));
	outb (elbase + ELRCR, ELRMON);
	if (elp->gldm_state != ELB_ERROR)
		elp->gldm_state = ELB_IDLE;
	return (0);
}


/*
 * el_NIC_reset (elp)
 * stop the NIC, reset registers and restart it.
 */
static void
el_NIC_reset (gld_mac_info_t * elp)
{
	register long elbase = elp->gldm_port;	/* base of board registers */
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

	(void) el_stop_board (elp);	/* reset to clear NIC */
	if (elvar->el_watch) {
		(void) untimeout(elvar->el_watch);
		elvar->el_watch = 0;
	}
	outb (elbase + ELCR, (ELCPG0 | ELCDMA | ELCSTP));
	outb (elbase + ELPSTART, elvar->el_rcvstart);
	outb (elbase + ELPSTOP, RCV_STOP);
	outb (elbase + ELBNDY, RCV_STOP - 1);
	outb (elbase + ELCR, (ELCPG1 | ELCDMA | ELCSTP));
	outb (elbase + ELCURR, elvar->el_rcvstart);
	(void) el_start_board (elp);
}

/*
 *  el_watchdog (elp)
 *	check that a transmit has completed in a reasonable amount of
 *	time.  Restart everything if it has failed for some reason
 */
void
el_watchdog (gld_mac_info_t *elp)
{
	struct elvar *elvar = (struct elvar *) elp->gldm_private;

	mutex_enter (&elp->gldm_maclock);
	elvar->el_watch = 0;	/* don't want NIC_reset to untimeoutx */
	if (elp->gldm_state == ELB_XMTBUSY) {
		el_NIC_reset (elp);
	}
	gld_sched(elp);
	mutex_exit (&elp->gldm_maclock);
}

/*
 * el_media_probe (elp, devinfo)
 *	probe to determine which of the two LAN interfaces is in use.
 *	This is needed since there is no way to determine which to use
 * 	from reading the hardware.
 *	Note that it is important to probe the AUI interface first since
 *	if the BNC is enabled while the AUI is connected to the LAN, you
 *	can blow a fuse on the board.  Also, if the second port is TP rather
 *	than BNC, you can't tell if it is connected or not since a transmit
 *	will always appear to succeed.
 */

static int
el_media_probe (gld_mac_info_t *elp, dev_info_t *devinfo)
{
   caddr_t addr;
   int i, port, value;

   port = elp->gldm_port;

   /* map memory temporarily */
   ddi_map_regs (devinfo, elp->gldm_reg_index, &addr, 0, 0);

   /* format a packet to transmit */
   bcopy ((caddr_t)elp->gldm_macaddr, addr, ETHERADDRL);
   bcopy ((caddr_t)elp->gldm_macaddr, addr+ETHERADDRL, ETHERADDRL);
   *(ushort *) (addr + 2*ETHERADDRL) = 0;
   for (i = 2*ETHERADDRL + 2; i < 64; i++)
     *(addr + i) = i&0xFF;

   /* setup basic transmitter data */
   outb (port + ELCR, (ELCPG0 | ELCDMA | ELCSTP));
   outb (port + ELDCR, 0x48);
   outb (port + ELTCR, ELTLOOP);
   outb (port + ELCR, (ELCPG0 | ELCDMA | ELCSTP));
   outb (port + ELIMR, 0);	/* no interrupts */
   outb (port + ELTCR, 0);	/* normal transmit */
   outb (port + ELTPSR, 0x20);	/* start page */
   /* transmit it on AUI first*/
   outb (port + ELCTRL, 0);
   outb (port + ELTBCR0, 64);
   outb (port + ELTBCR1, 0);
   value = inb (port + ELCR) & ELPGMSK;
   value &= ~ELCSTP;
   value |= ELCSTA;     
   outb (port + ELCR, ELCTXP | ELCPG0 | value); /* start transmit */
   while (!((value = inb (port + ELTSR)) & (ELTPTX | ELTABT | ELTCRS)))
     drv_usecwait (100);
   if ((value & (ELTPTX|ELTCRS|ELTABT)) == ELTPTX) {
      ddi_unmap_regs (devinfo, 0, &addr, 0, 0);
      return (GLDM_AUI);
   }

   /* AUI failed so try BNC/TP */
   
   outb (port + ELCTRL, ELAXCVR);
   outb (port + ELTBCR0, 64);
   outb (port + ELTBCR1, 0);
   value = inb (port + ELCR) & ELPGMSK;
   value &= ~ELCSTP;
   value |= ELCSTA;     
   outb (port + ELCR, ELCTXP | ELCPG0 | value); /* start transmit */
   while (!((value = inb (port + ELTSR)) & (ELTPTX | ELTABT | ELTCRS)))
     drv_usecwait (100);
   if ((value & (ELTPTX|ELTCRS|ELTABT)) != ELTPTX)
     cmn_err (CE_WARN, "el (@0x%x): no tranceiver attached", port);
   ddi_unmap_regs (devinfo, 0, &addr, 0, 0);
   return (GLDM_BNC);
}
