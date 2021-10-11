/*
 * iprb --	Intel PRO100/B Fast Ethernet Driver
 * Depends on the Generic LAN Driver utility functions in /kernel/misc/gld
 */
/*
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part. Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California	94043
 */

/*
 * Copyright (c) 1995, 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident "@(#)iprb.c	1.11	96/10/01 SMI"

#include <sys/types.h>
#include <sys/debug.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/devops.h>
#if defined(PCI_DDI_EMULATION) || defined(COMMON_IO_EMULATION)
#include <sys/xpci/sunddi_2.5.h>
#else
#include <sys/sunddi.h>
#endif
#include <sys/eisarom.h>
#include <sys/ksynch.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/strsun.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/gld.h>
#include "iprb.h"
#include <sys/pci.h>

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "Intel 82557 Driver";

#ifdef IPRBDEBUG
/* used for debugging */
int	iprbdebug = 0;
#endif


/* Required system entry points */
static	iprbidentify(dev_info_t *);
static	iprbdevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	iprbprobe(dev_info_t *);
static	iprbattach(dev_info_t *, ddi_attach_cmd_t);
static	iprbdetach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
int	iprb_reset(gld_mac_info_t *);
int	iprb_start_board(gld_mac_info_t *);
int	iprb_stop_board(gld_mac_info_t *);
void	iprb_hard_reset(gld_mac_info_t *);
void    iprb_get_ethaddr(gld_mac_info_t *);
int	iprb_saddr(gld_mac_info_t *);
int	iprb_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
int	iprb_prom(gld_mac_info_t *, int);
int	iprb_gstat(gld_mac_info_t *);
int	iprb_send(gld_mac_info_t *, mblk_t *);
u_int	iprbintr(gld_mac_info_t *);


static void iprb_configure(gld_mac_info_t *, int);
static int iprb_init_board(gld_mac_info_t *);
static void iprb_add_command(gld_mac_info_t *);
static void iprb_reap_commands(gld_mac_info_t *macinfo);
static void iprb_readia(gld_mac_info_t *macinfo, unsigned short *addr,
		    unsigned short offset);
static void iprb_shiftout(gld_mac_info_t *macinfo, unsigned short data,
			    unsigned short count);
static void iprb_raiseclock(gld_mac_info_t *macinfo, unsigned short *eex);
static void iprb_lowerclock(gld_mac_info_t *macinfo, unsigned short *eex);
static int iprb_shiftin(gld_mac_info_t *macinfo);
static void iprb_eeclean(gld_mac_info_t *macinfo);
static void iprb_mdi_read(gld_mac_info_t *macinfo, unsigned char reg_addr,
			unsigned char phy_addr, unsigned short *result);
static void iprb_release_dma_resources(struct iprbinstance *iprbp);
static void iprb_rcv_complete(struct iprb_buf_free *);
static void iprb_release_mblks(struct iprbinstance *);
static void iprb_wait(caddr_t);

#if ! defined(_DDI_DMA_MEM_ALLOC_FIXED)
static int iprb_dma_mem_alloc(ddi_dma_handle_t, uint_t, ddi_device_acc_attr_t *,
			      ulong_t, int (*)(caddr_t), caddr_t, caddr_t *,
			      uint_t *, ddi_acc_handle_t *);
static void iprb_dma_mem_free(ddi_acc_handle_t *);
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */

#if defined(PCI_DDI_EMULATION) || defined(COMMON_IO_EMULATION)
char _depends_on[] = "misc/xpci misc/gld";
#else
char _depends_on[] = "misc/gld";
#endif

/* DMA attributes for a control command block */
static ddi_dma_attr_t control_cmd_dma_attr = {
	DMA_ATTR_V0,		/* version of this structure */
	0,			/* lowest usable address */
	0xffffffffU,		/* highest usable address */
	0x7fffffff,		/* maximum DMAable byte count */
	4,			/* alignment in bytes */
	0x7ff,			/* burst sizes (any?) */
	1,			/* minimum transfer */
	0xffffffffU,		/* maximum transfer */
	0xffffffffU,		/* maximum segment length */
	1,			/* maximum number of segments */
	1,			/* granularity */
	0,			/* flags (reserved) */
};

/* DMA attributes for a transmit buffer descriptor array */
static ddi_dma_attr_t tx_buffer_desc_dma_attr = {
	DMA_ATTR_V0,		/* version of this structure */
	0,			/* lowest usable address */
	0xffffffffU,		/* highest usable address */
	0x7fffffff,		/* maximum DMAable byte count */
	4,			/* alignment in bytes */
	0x7ff,			/* burst sizes (any?) */
	1,			/* minimum transfer */
	0xffffffffU,		/* maximum transfer */
	0xffffffffU,		/* maximum segment length */
	1,			/* maximum number of segments */
	1,			/* granularity */
	0,			/* flags (reserved) */
};

/* DMA attributes for a receive frame descriptor */
static ddi_dma_attr_t rcv_frame_dma_attr = {
	DMA_ATTR_V0,		/* version of this structure */
	0,			/* lowest usable address */
	0xffffffffU,		/* highest usable address */
	0x7fffffff,		/* maximum DMAable byte count */
	4,			/* alignment in bytes */
	0x7ff,			/* burst sizes (any?) */
	1,			/* minimum transfer */
	0xffffffffU,		/* maximum transfer */
	0xffffffffU,		/* maximum segment length */
	1,			/* maximum number of segments */
	1,			/* granularity */
	0,			/* flags (reserved) */
};

static ddi_dma_attr_t rcv_buffer_desc_dma_attr = {
	DMA_ATTR_V0,		/* version of this structure */
	0,			/* lowest usable address */
	0xffffffffU,		/* highest usable address */
	0x7fffffff,		/* maximum DMAable byte count */
	4,			/* alignment in bytes */
	0x7ff,			/* burst sizes (any?) */
	1,			/* minimum transfer */
	0xffffffffU,		/* maximum transfer */
	0xffffffffU,		/* maximum segment length */
	1,			/* maximum number of segments */
	1,			/* granularity */
	0,			/* flags (reserved) */
};

/* DMA attributes for a receive buffer */
static ddi_dma_attr_t rcv_buffer_dma_attr = {
	DMA_ATTR_V0,		/* version of this structure */
	0,			/* lowest usable address */
	0xffffffffU,		/* highest usable address */
	0x7fffffff,		/* maximum DMAable byte count */
	2,			/* alignment in bytes */
	0x7ff,			/* burst sizes (any?) */
	1,			/* minimum transfer */
	0xffffffffU,		/* maximum transfer */
	0xffffffffU,		/* maximum segment length */
	1,			/* maximum number of segments */
	1,			/* granularity */
	0,			/* flags (reserved) */
};

/* DMA attributes for a transmit buffer */
static ddi_dma_attr_t tx_buffer_dma_attr = {
	DMA_ATTR_V0,		/* version of this structure */
	0,			/* lowest usable address */
	0xffffffffU,		/* highest usable address */
	0x7fffffff,		/* maximum DMAable byte count */
	1,			/* alignment in bytes */
	0x7ff,			/* burst sizes (any?) */
	1,			/* minimum transfer */
	0xffffffffU,		/* maximum transfer */
	0xffffffffU,		/* maximum segment length */
	2,			/* maximum number of segments */
	1,			/* granularity */
	0,			/* flags (reserved) */
};

/* DMA attributes for a statistics buffer */
static ddi_dma_attr_t stats_buffer_dma_attr = {
	DMA_ATTR_V0,		/* version of this structure */
	0,			/* lowest usable address */
	0xffffffffU,		/* highest usable address */
	0x7fffffff,		/* maximum DMAable byte count */
	16,			/* alignment in bytes */
	0x7ff,			/* burst sizes (any?) */
	1,			/* minimum transfer */
	0xffffffffU,		/* maximum transfer */
	0xffffffffU,		/* maximum segment length */
	1,			/* maximum number of segments */
	1,			/* granularity */
	0,			/* flags (reserved) */
};

/* DMA access attributes */
static ddi_device_acc_attr_t accattr = {
	DDI_DEVICE_ATTR_V0,
	DDI_NEVERSWAP_ACC,
	DDI_STRICTORDER_ACC,
};

/* Standard Streams initialization */

static struct module_info minfo = {
	IPRBIDNUM, "iprb", 0, INFPSZ, IPRBHIWAT, IPRBLOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

struct streamtab iprbinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_iprbops = {
	nulldev,		/* cb_open */
	nulldev,		/* cb_close */
	nodev,			/* cb_strategy */
	nodev,			/* cb_print */
	nodev,			/* cb_dump */
	nodev,			/* cb_read */
	nodev,			/* cb_write */
	nodev,			/* cb_ioctl */
	nodev,			/* cb_devmap */
	nodev,			/* cb_mmap */
	nodev,			/* cb_segmap */
	nochpoll,		/* cb_chpoll */
	ddi_prop_op,		/* cb_prop_op */
	&iprbinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

struct dev_ops iprbops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	iprbdevinfo,		/* devo_getinfo */
	iprbidentify,		/* devo_identify */
	iprbprobe,		/* devo_probe */
	iprbattach,		/* devo_attach */
	iprbdetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_iprbops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&iprbops			/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

static kmutex_t iprb_probe_lock;
static void iprb_process_recv(gld_mac_info_t *macinfo);
static int iprb_pci_get_irq(dev_info_t *devinfo);
extern int pci_config_setup(dev_info_t *dip, ddi_acc_handle_t *handle);

int
_init(void)
{
	int	status;

	mutex_init(&iprb_probe_lock,
		"MP probe protection", MUTEX_DRIVER, NULL);
	status = mod_install(&modlinkage);

	if (status != 0)
		mutex_destroy(&iprb_probe_lock);

	return (status);
}

int
_fini(void)
{
	int	status;

	status = mod_remove(&modlinkage);
	if (status == 0)
		mutex_destroy(&iprb_probe_lock);

	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 *  DDI Entry Points
 */

/* identify(9E) -- See if we know about this device */

iprbidentify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "iprb") == 0)
		return (DDI_IDENTIFIED);
	else if (strcmp(ddi_get_name(devinfo), "pci8086,1229") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

/* getinfo(9E) -- Get device driver information */

/* ARGSUSED2 */
iprbdevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;

	/* This code is not DDI compliant: the correct semantics	*/
	/* for CLONE devices is not well-defined yet.			*/
	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (devinfo == NULL) {
			error = DDI_FAILURE;	/* Unfortunate */
		} else {
			*result = (void *)devinfo;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)0;	/* This CLONEDEV always returns zero */
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}


static int
iprb_pci_probe(dev_info_t *devinfo)
{
	ddi_acc_handle_t	handle;
	ushort	vendor_id, device_id;
	unsigned char iline, cmdreg;
	int	pci_probe_result;

	pci_probe_result = DDI_PROBE_FAILURE;

	if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS)
		return (pci_probe_result);

	vendor_id = pci_config_getw(handle, PCI_CONF_VENID);
	device_id = pci_config_getw(handle, PCI_CONF_DEVID);

	if ((vendor_id == IPRB_PCI_VENID)
	&&  (device_id == IPRB_PCI_DEVID)) {

		cmdreg = pci_config_getb(handle, PCI_CONF_COMM);
		iline = pci_config_getb(handle, PCI_CONF_ILINE);
#if defined(i86pc)
		if ((iline == 0) || (iline > 15)) {
			cmn_err(CE_WARN, "iprb: iline value out of range: %d\n",
						iline);
			pci_config_teardown(&handle);
			return (pci_probe_result);
		}
#endif
		pci_probe_result = DDI_PROBE_SUCCESS;

		/* This code is needed to workaround a bug in the framework */
		if (! (cmdreg & PCI_COMM_MAE)) {
			pci_config_putb(handle, PCI_CONF_COMM,
					cmdreg | PCI_COMM_MAE);
		}

	}
	pci_config_teardown(&handle);
	return (pci_probe_result);
}

iprbprobe(dev_info_t *devinfo)
{
	int ddi_probe_result;

	ddi_probe_result = iprb_pci_probe(devinfo);

	return (ddi_probe_result);
}

/* ARGSUSED */
static
int
iprb_pci_get_irq(dev_info_t *devinfo)
{
#if defined(PCI_DDI_EMULATION) || defined(COMMON_IO_EMULATION)
	ddi_acc_handle_t	handle;
	int	iline;
	int i;
	int len;
	struct intrprop {
		int spl;
		int irq;
	} *intrprop;


	if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS)
		return (-1);

	iline = pci_config_getb(handle, PCI_CONF_ILINE);
	pci_config_teardown(&handle);

	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
			"interrupts", (caddr_t)&intrprop,
			&len) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
			"iprb: Could not locate interrupts property.\n");
		return (DDI_PROBE_FAILURE);
	}

	for (i = 0; i < (len / sizeof(struct intrprop)); i++)
		if (iline == intrprop[i].irq)
			break;
	kmem_free(intrprop, len);

	if (i >= (len / sizeof(struct intrprop))) {
		cmn_err(CE_WARN,
			"iprb: irq in conf file does not match PCI config.\n");
		return (-1);
	}

	return (i);
#else
	return (0);	/* always index 0 for PCI in 2.5 */
#endif
}

/*
 *  attach(9E) -- Attach a device to the system
 *
 *  Called once for each board successfully probed.
 */
iprbattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	struct iprbinstance *iprbp;		/* Our private device info */
	unsigned short mdicreg, mdisreg;

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBDDI) {
		debug_enter("\n\niprb attach\n\n");
		cmn_err(CE_CONT, "iprbattach(0x%x)", (int)devinfo);
	}
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	/*
	 *  Allocate gld_mac_info_t and iprbinstance structures
	 */
	macinfo = (gld_mac_info_t *) kmem_zalloc(
		sizeof (gld_mac_info_t) + sizeof (struct iprbinstance),
		KM_NOSLEEP);
	if (macinfo == NULL) {
		cmn_err(CE_WARN, "iprb: kmem_zalloc failure for macinfo");
		return (DDI_FAILURE);
	}

	iprbp = (struct iprbinstance *)(macinfo + 1);

	/*  Initialize our private fields in macinfo and iprbinstance */
	macinfo->gldm_private = (caddr_t)iprbp;

	{
		int base1;
		ddi_acc_handle_t handle;

		if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS) {
			kmem_free(macinfo, sizeof (gld_mac_info_t) +
				sizeof (struct iprbinstance));
			return (DDI_FAILURE);
		}

		base1 = pci_config_getl(handle, PCI_CONF_BASE1);
		pci_config_teardown(&handle);
		macinfo->gldm_port = base1 & PCI_BASE_IO_ADDR_M;
		macinfo->gldm_irq_index = (long)iprb_pci_get_irq(devinfo);
		if (macinfo->gldm_irq_index == -1) {
			kmem_free(macinfo, sizeof (gld_mac_info_t) +
				sizeof (struct iprbinstance));
			return (DDI_FAILURE);
		}
	}

	macinfo->gldm_state = IPRB_IDLE;
	macinfo->gldm_flags = 0;

	iprbp->iprb_dip = devinfo;
	macinfo->gldm_reg_index = -1;

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */
	macinfo->gldm_reset   = iprb_reset;
	macinfo->gldm_start   = iprb_start_board;
	macinfo->gldm_stop    = iprb_stop_board;
	macinfo->gldm_saddr   = iprb_saddr;
	macinfo->gldm_sdmulti = iprb_dlsdmult;
	macinfo->gldm_prom    = iprb_prom;
	macinfo->gldm_gstat   = iprb_gstat;
	macinfo->gldm_send    = iprb_send;
	macinfo->gldm_intr    = iprbintr;
	macinfo->gldm_ioctl   = NULL;    /* if you have one, NULL otherwise */

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = IPRBMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;


	/* set the connector/media type if it can be determined */
	macinfo->gldm_media = GLDM_UNKNOWN;

	/* Tell gld that we will free the mblks */
	macinfo->gldm_options = GLDOPT_DONTFREE; /* don't do freemsg() */

	/* Reset the hardware */
	(void) iprb_hard_reset(macinfo);

	/* Get the board's vendor-assigned hardware network address */
	(void) iprb_get_ethaddr(macinfo);

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);
	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);

	ddi_set_driver_private(devinfo, (caddr_t)macinfo);

	/*
	 *  Register ourselves with the GLD interface
	 *
	 *  gld_register will:
	 *	link us with the GLD system;
	 *	set our ddi_set_driver_private(9F) data to the macinfo ptr;
	 *	save the devinfo pointer in macinfo->gldm_devinfo;
	 *	map the registers, putting the kvaddr into macinfo->gldm_memp;
	 *	add the interrupt, putting the cookie in gldm_cookie;
	 *	init the gldm_intrlock mutex which will block that interrupt;
	 *	create the minor node.
	 */
	if (gld_register(devinfo, "iprb", macinfo) != DDI_SUCCESS) {
		goto afail;
	}

	/* need to hold the lock to add init commands to device queue */
	mutex_enter(&macinfo->gldm_maclock);

	/*
	 *  Do anything necessary to prepare the board for operation
	 *  short of actually starting the board.
	 */
	if ((iprb_init_board(macinfo)) == DDI_FAILURE) {
		mutex_exit(&macinfo->gldm_maclock);
		gld_unregister(macinfo);
		goto afail;
	}

	/* Tell the board what its address is (from gldm_macaddr) */
	if ((iprb_saddr(macinfo))) {
		mutex_exit(&macinfo->gldm_maclock);
		gld_unregister(macinfo);
		goto afail;
	}


	/*
	 * Code to figure out what type of physical interface
	 * exists.  For the D100, it can either be MII or 503.
	 */
	/* NEEDSWORK: this is (nearly) known to be insufficient */
	(void) iprb_mdi_read(macinfo, IPRB_MDI_CREG, 1, &mdicreg);
	(void) iprb_mdi_read(macinfo, IPRB_MDI_SREG, 1, &mdisreg);
	if ((mdicreg == 0xffff) || ((mdisreg == 0) && (mdicreg == 0))) {
		iprbp->iprb_phy_type = IPRB_TYPE_503;
	} else {
		iprbp->iprb_phy_type = IPRB_TYPE_MII;
	}

	/* Set board's initial configuration, without promiscuous mode */
	iprb_configure(macinfo, 0);

	macinfo->gldm_state = IPRB_WAITRCV;
	mutex_exit(&macinfo->gldm_maclock);

	return (DDI_SUCCESS);

afail:
	kmem_free(macinfo, sizeof (gld_mac_info_t) +
				sizeof (struct iprbinstance));
	return (DDI_FAILURE);
}

void
iprb_hard_reset(gld_mac_info_t *macinfo)
{
	outl(macinfo->gldm_port + IPRB_SCB_PORT, IPRB_PORT_SW_RESET);
	drv_usecwait(10);		/* As per specs - RSS */
}

/*  detach(9E) -- Detach a device from the system */

iprbdetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	struct iprbinstance *iprbp;		/* Our private device info */
	int i;
#define WAITTIME 10000					/* usecs to wait */
#define MAX_TIMEOUT_RETRY_CNT	(30 * (1000000 / WAITTIME))	/* 30 seconds */

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBDDI)
		cmn_err(CE_CONT, "iprbdetach(0x%x)", (int)devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);
	iprbp = (struct iprbinstance *)(macinfo->gldm_private);

	/* Stop the receiver */
	(void) iprb_stop_board(macinfo);

	/* Stop the board if it is running */
	iprb_hard_reset(macinfo);

	/* Release any pending xmit mblks */
	iprb_release_mblks(iprbp);

	/* Wait for all receive buffers to be returned */
	i = MAX_TIMEOUT_RETRY_CNT;

	mutex_enter(&iprbp->iprb_rcv_buf_mutex);
	while (iprbp->iprb_rcv_bufs_outstanding > 0) {
		mutex_exit(&iprbp->iprb_rcv_buf_mutex);
		timeout(iprb_wait, (caddr_t)NULL, drv_usectohz(WAITTIME));
		if (--i == 0) {
			cmn_err(CE_WARN, "iprb: never reclaimed all the "
					 "receive buffers.  Still have %d "
					 "buffers outstanding.\n",
				iprbp->iprb_rcv_bufs_outstanding);
			return (DDI_FAILURE);
		}
		mutex_enter(&iprbp->iprb_rcv_buf_mutex);
	}
	mutex_exit(&iprbp->iprb_rcv_buf_mutex);

	/* Release all DMA resources */
	iprb_release_dma_resources(iprbp);

	/* Reset xmit buffer descriptor counters */
	for (i = 0; i < iprbp->iprb_nxmits; ++i)
		iprbp->iprb_xmit_buf_desc_cnt[i] = 0;
	

	/* Get rid of receive buffer pool mutex */
	mutex_destroy(&iprbp->iprb_rcv_buf_mutex);

	/*
	 *  Unregister ourselves from the GLD interface
	 *
	 *  gld_unregister will:
	 *	remove the minor node;
	 *	unmap the registers;
	 *	remove the interrupt;
	 *	destroy the gldm_intrlock mutex;
	 *	unlink us from the GLD system.
	 */
	if (gld_unregister(macinfo) == DDI_SUCCESS) {
		kmem_free((caddr_t)macinfo, sizeof (gld_mac_info_t) +
					sizeof (struct iprbinstance));
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}

/*
 *  GLD Entry Points
 */

/*
 *  iprb_reset() -- reset the board to initial state; restore the machine
 *  address afterwards.
 */

int
iprb_reset(gld_mac_info_t *macinfo)
{

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBTRACE)
		cmn_err(CE_CONT, "iprb_reset(0x%x)", (int)macinfo);
#endif

	(void) iprb_stop_board(macinfo);

	return (DDI_SUCCESS);
}

/*
 *  iprb_init_board() -- initialize the specified network board.
 */
int
iprb_init_board(gld_mac_info_t *macinfo)
{
	struct iprbinstance *iprbp;		/* Our private device info */
	register int i;
	dev_info_t *devinfo;
	uint_t len;
	unsigned long last_dma_addr;
	ddi_dma_cookie_t dma_cookie;
	uint_t ncookies;

	iprbp =	(struct iprbinstance *)macinfo->gldm_private;
	devinfo = iprbp->iprb_dip;

	/* Get the number of requested receive frames from property */
	iprbp->iprb_nframes = ddi_getprop(DDI_DEV_T_NONE, devinfo,
					DDI_PROP_DONTPASS,
					"num-recv-bufs", 0);
	if (iprbp->iprb_nframes <= 0)
		iprbp->iprb_nframes = IPRB_DEFAULT_RECVS;

	if (iprbp->iprb_nframes > IPRB_MAX_RECVS)
		iprbp->iprb_nframes = IPRB_MAX_RECVS;

	iprbp->iprb_nxmits = ddi_getprop(DDI_DEV_T_NONE, devinfo,
					DDI_PROP_DONTPASS, "num-xmit-bufs", 0);
	if (iprbp->iprb_nxmits <= 0)
		iprbp->iprb_nxmits = IPRB_DEFAULT_XMITS;

	if (iprbp->iprb_nxmits > IPRB_MAX_XMITS)
		iprbp->iprb_nxmits = IPRB_MAX_XMITS;

	/*
	 * The code below allocates all the DMA data structures that
	 * need to be release when the driver is detached.
	 * NB: there are quite a few!
	 */
	/* Allocate a DMA handle for the control unit blocks */
	if (ddi_dma_alloc_handle(devinfo, &control_cmd_dma_attr,
				 DDI_DMA_DONTWAIT, 0,
				 &iprbp->iprb_dma_handle_cu)
			!= DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"iprb: could not allocate control block handle\n");
		return (DDI_FAILURE);
	}

	/* Allocate the CU command blocks (aka transmit control blocks) */
	for (i = 0; i < iprbp->iprb_nxmits; ++i) {
		if (ddi_dma_mem_alloc(iprbp->iprb_dma_handle_cu,
				      sizeof(union iprb_generic_cmd),
				      &accattr, DDI_DMA_STREAMING,
				      DDI_DMA_DONTWAIT, 0,
				      (caddr_t *)&iprbp->iprb_cu_cmd_block[i],
				      &len, &iprbp->iprb_cu_dma_acchdl[i])
				!= DDI_SUCCESS) {
			cmn_err(CE_WARN, "iprb: could not allocate control "
					 "blocks\n");
			while (--i >= 0) {
				ddi_dma_mem_free(&iprbp->iprb_cu_dma_acchdl[i]);
				iprbp->iprb_cu_dma_acchdl[i] = NULL;
			}
			goto error;
		}
	}

	last_dma_addr = IPRB_NULL_PTR;	/* for linking blocks together */

	/* 
	 * Now initialize the ring of CU command blocks
	 */
	for (i = iprbp->iprb_nxmits - 1; i >= 0; --i) {
		bzero((caddr_t)iprbp->iprb_cu_cmd_block[i],
		      sizeof(union iprb_generic_cmd));
		if (ddi_dma_addr_bind_handle(iprbp->iprb_dma_handle_cu, NULL,
					     (caddr_t)iprbp->iprb_cu_cmd_block[i],
					     sizeof(union iprb_generic_cmd),
					     DDI_DMA_RDWR | DDI_DMA_STREAMING,
					     DDI_DMA_DONTWAIT, 0, &dma_cookie,
					     &ncookies) != DDI_DMA_MAPPED) {
			cmn_err(CE_WARN, "iprb: could not get DMA address for "
					 "CU command block\n");
			goto error;
		}
		ASSERT(ncookies == 1);
		iprbp->iprb_cu_cmd_block[i]->xmit_cmd.xmit_next = last_dma_addr;
		iprbp->iprb_cu_cmd_block[i]->xmit_cmd.xmit_bits = 0;
		iprbp->iprb_cu_cmd_block[i]->xmit_cmd.xmit_cmd
			= IPRB_XMIT_CMD | IPRB_SF;
		last_dma_addr = dma_cookie.dmac_address;
		ddi_dma_unbind_handle(iprbp->iprb_dma_handle_cu);
	}
	iprbp->iprb_cu_cmd_block[iprbp->iprb_nxmits - 1]->xmit_cmd.xmit_next
			= last_dma_addr;

	/* Allocate a DMA handle for the xmit buffer descriptors */
	if (ddi_dma_alloc_handle(devinfo, &tx_buffer_desc_dma_attr,
				 DDI_DMA_DONTWAIT, 0,
				 &iprbp->iprb_dma_handle_txbda)
			!= DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"iprb: could not allocate xmit buffer descriptor array "			"block handle\n");
		goto error;
	}

	/* Now start on the receive side of things */
	if (ddi_dma_alloc_handle(devinfo, &rcv_frame_dma_attr, DDI_DMA_DONTWAIT,
				 0, &iprbp->iprb_dma_handle_ru)
			!= DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"iprb: could not allocate receive unit handle\n");
		goto error;
	}

	/* Allocate the RU frame descriptors (aka RFD) */
	for (i = 0; i < iprbp->iprb_nframes; ++i) {
		if (ddi_dma_mem_alloc(iprbp->iprb_dma_handle_ru,
				      sizeof(struct iprb_rfd),
				      &accattr, DDI_DMA_STREAMING,
				      DDI_DMA_DONTWAIT, 0,
				      (caddr_t *)&iprbp->iprb_ru_frame_desc[i],
				      &len, &iprbp->iprb_ru_dma_acchdl[i])
				!= DDI_SUCCESS) {
			cmn_err(CE_WARN, "iprb: could not allocate receive "
					 "frame descriptor\n");
			/* Release all resources acquired so far */
			while (--i >= 0) {
				ddi_dma_mem_free(&iprbp->iprb_ru_dma_acchdl[i]);
				iprbp->iprb_ru_dma_acchdl[i] = NULL;
			}
			goto error;
		}
	}

	/* 
	 * Now initialize the ring of RU command blocks
	 */
	last_dma_addr = IPRB_NULL_PTR;

	for (i = iprbp->iprb_nframes - 1; i >= 0; --i) {
		bzero((caddr_t)iprbp->iprb_ru_frame_desc[i],
		      sizeof(struct iprb_rfd));
		if (ddi_dma_addr_bind_handle(iprbp->iprb_dma_handle_ru, NULL,
					     (caddr_t)iprbp->iprb_ru_frame_desc[i],
					     sizeof(struct iprb_rfd),
					     DDI_DMA_RDWR | DDI_DMA_STREAMING,
					     DDI_DMA_DONTWAIT, 0, &dma_cookie,
					     &ncookies) != DDI_DMA_MAPPED) {
			cmn_err(CE_WARN, "iprb: could not get DMA address for "
					 "receive unit frame descriptor\n");
			goto error;
		}
		ASSERT(ncookies == 1);
		iprbp->iprb_ru_frame_desc[i]->rfd_control = IPRB_RFD_SF;
		iprbp->iprb_ru_frame_desc[i]->rfd_next = last_dma_addr;
		iprbp->iprb_ru_frame_desc[i]->rfd_rbd = IPRB_NULL_PTR;
		last_dma_addr = dma_cookie.dmac_address;
		/* It is convient to save this information for RU starts */
		iprbp->iprb_ru_frame_addr[i] = last_dma_addr;
		ddi_dma_unbind_handle(iprbp->iprb_dma_handle_ru);
	}
	iprbp->iprb_ru_frame_desc[iprbp->iprb_nframes - 1]->rfd_next
			= last_dma_addr;

	/* Set up last RFD to have end-of-list bit on */
	iprbp->iprb_ru_frame_desc[iprbp->iprb_nframes - 1]->rfd_control
			|= IPRB_RFD_EL;

	/* Now allocate a handle for the receive buffer descriptors */
	if (ddi_dma_alloc_handle(devinfo, &rcv_buffer_desc_dma_attr,
				 DDI_DMA_DONTWAIT, 0,
				 &iprbp->iprb_dma_handle_rcvbda)
			!= DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"iprb: could not allocate receive buffer descriptor "
			"array\n");
		goto error;
	}

	/*
	 * Now allocate memory for the receive buffer descriptors
	 */
	for (i = 0; i < iprbp->iprb_nframes; ++i) {
		if (ddi_dma_mem_alloc(iprbp->iprb_dma_handle_rcvbda,
				      iprbp->iprb_nframes
						* sizeof(struct iprb_rbd),
				      &accattr, DDI_DMA_STREAMING,
				      DDI_DMA_DONTWAIT, 0,
				      (caddr_t *)&iprbp->iprb_rcv_buf_desc[i],
				      &len, &iprbp->iprb_rcv_desc_dma_acchdl[i])
				!= DDI_SUCCESS) {
			cmn_err(CE_WARN, "iprb: could not allocate memory "
					 "for receive buffer descriptor\n");
			while (--i >= 0) {
				ddi_dma_mem_free(&iprbp->iprb_rcv_desc_dma_acchdl[i]);
				iprbp->iprb_rcv_desc_dma_acchdl[i] = NULL;
			}
		}
	}

	/*
	 * Link receive buffer descriptors together and point first RFD at
	 * the first RBD.
	 */
	last_dma_addr = IPRB_NULL_PTR;

	for (i = iprbp->iprb_nframes - 1; i >= 0; --i) {
		bzero((caddr_t)iprbp->iprb_rcv_buf_desc[i],
		      sizeof(struct iprb_rbd));
		if (ddi_dma_addr_bind_handle(iprbp->iprb_dma_handle_rcvbda,
					     NULL,
					     (caddr_t)iprbp->iprb_rcv_buf_desc[i],
					     sizeof(struct iprb_rbd),
					     DDI_DMA_RDWR | DDI_DMA_STREAMING,
					     DDI_DMA_DONTWAIT, 0, &dma_cookie,
					     &ncookies) != DDI_DMA_MAPPED) {
			cmn_err(CE_WARN, "iprb: could not get DMA address for "
					 "buffer descriptor address\n");
			goto error;
		}
		ASSERT(ncookies == 1);
		iprbp->iprb_rcv_buf_desc[i]->rbd_next = last_dma_addr;
		last_dma_addr = dma_cookie.dmac_address;
		ddi_dma_unbind_handle(iprbp->iprb_dma_handle_rcvbda);
	}
	iprbp->iprb_rcv_buf_desc[iprbp->iprb_nframes - 1]->rbd_next
			= last_dma_addr;
	iprbp->iprb_ru_frame_desc[0]->rfd_rbd = last_dma_addr;

	/* Preallocate receive buffers for each receive descriptor */

	/* Allocate a handle for the receive buffers  */
	if (ddi_dma_alloc_handle(devinfo, &rcv_buffer_dma_attr,
				 DDI_DMA_DONTWAIT, 0,
				 &iprbp->iprb_dma_handle_rcv_buf)
			!= DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"iprb: could not allocate receive buffer handle\n");
		goto error;
	}

	/* Allocate DMAable memory for each receive buffer */
	for (i = 0; i < iprbp->iprb_nframes; ++i) {
		/* Change "iprb" to "ddi" when ddi_dma_mem_alloc() is fixed */
#if defined(_DDI_DMA_MEM_ALLOC_FIXED)
		if (ddi_dma_mem_alloc(iprbp->iprb_dma_handle_rcv_buf,
#else /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
		if (iprb_dma_mem_alloc(iprbp->iprb_dma_handle_rcv_buf,
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
				       IPRB_FRAMESIZE,
				       &accattr, DDI_DMA_STREAMING,
				       DDI_DMA_DONTWAIT, 0,
				       (caddr_t *)&iprbp->iprb_rcv_pool[i].iprb_rcv_buf,
				       &len,
				       &iprbp->iprb_rcv_pool[i].iprb_rcv_buf_dma_acchdl)
				!= DDI_SUCCESS) {
			cmn_err(CE_WARN, "iprb: could not allocate memory "
					 "for receive buffer\n");
			while (--i >= 0) {
#if defined(_DDI_DMA_MEM_ALLOC_FIXED)
				ddi_dma_mem_free(&iprbp->iprb_rcv_pool[i].iprb_rcv_buf_dma_acchdl);
#else /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
				iprb_dma_mem_free(&iprbp->iprb_rcv_pool[i].iprb_rcv_buf_dma_acchdl);
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
				iprbp->iprb_rcv_pool[i].iprb_rcv_buf_dma_acchdl
						= NULL;
			}
			goto error;
		}
		bzero((caddr_t)iprbp->iprb_rcv_pool[i].iprb_rcv_buf, len);
	}

	/* Now get DMA addresses for each receive buffer */
	for (i = 0; i < iprbp->iprb_nframes; ++i) {
		if (ddi_dma_addr_bind_handle(iprbp->iprb_dma_handle_rcv_buf,
					     NULL,
					     (caddr_t)iprbp->iprb_rcv_pool[i].iprb_rcv_buf,
					     IPRB_FRAMESIZE,
					     DDI_DMA_WRITE | DDI_DMA_STREAMING,
					     DDI_DMA_DONTWAIT, 0, &dma_cookie,
					     &ncookies) != DDI_DMA_MAPPED) {
			cmn_err(CE_WARN, "iprb: could not translate receive "
					 "buffer address\n");
			goto error;
		}
		ASSERT(ncookies == 1);
		iprbp->iprb_rcv_pool[i].iprb_rcv_buf_addr
				= dma_cookie.dmac_address;
		ddi_dma_unbind_handle(iprbp->iprb_dma_handle_rcv_buf);
	}

	/* Now associate a receive buffer with each receive descriptor */
	for (i = 0; i < iprbp->iprb_nframes; ++i) {
		iprbp->iprb_rcv_buf_desc[i]->rbd_buffer
				= iprbp->iprb_rcv_pool[i].iprb_rcv_buf_addr;
		iprbp->iprb_rcv_buf_desc[i]->rbd_size = IPRB_FRAMESIZE;
	}

	mutex_init(&iprbp->iprb_rcv_buf_mutex,
		   "iprb receive buffer mutex",
		   MUTEX_DRIVER, NULL);

	iprbp->iprb_first_rfd = 0;	/* First RFD index in list */
	iprbp->iprb_last_rfd = iprbp->iprb_nframes - 1; /* Last RFD index */
	iprbp->iprb_current_rfd = 0;	/* Next RFD index to be filled */

	/*
	 * iprb_first_cmd is the first command used but not yet processed
	 * by the D100.  -1 means we don't have any commands pending.
	 *
	 * iprb_last_cmd is the last command used but not yet processed by
	 * the D100.
	 *
	 * iprb_current_cmd is the one we can next use.
	 */
	iprbp->iprb_first_cmd = -1;
	iprbp->iprb_last_cmd = 0;
	iprbp->iprb_current_cmd = 0;

	/* Note that the next command will be the first command */

	iprbp->iprb_initial_cmd = TRUE;

	return (DDI_SUCCESS);

error:
	iprb_release_dma_resources(iprbp);
	return (DDI_FAILURE);
}

/*
 * Read the EEPROM (one bit at a time!!!) to get the ethernet address
 */

void
iprb_get_ethaddr(gld_mac_info_t *macinfo)
{
	register int i;

	for (i = 0; i < (ETHERADDRL / sizeof (short)); i++) {
		iprb_readia(macinfo, (unsigned short *)
				&macinfo->gldm_vendor[i * sizeof (short)], i);
	}
}

/*
 *  iprb_saddr() -- set the physical network address on the board
 */

int
iprb_saddr(gld_mac_info_t *macinfo)
{
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	struct iprb_ias_cmd *ias;
	int i;

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBTRACE)
		cmn_err(CE_CONT, "iprb_saddr(0x%x)", (int)macinfo);
#endif

	/* Make available any command buffers already processed */
	iprb_reap_commands(macinfo);

	/* Any command buffers left? */
	if (iprbp->iprb_current_cmd == iprbp->iprb_first_cmd) {
		cmn_err(CE_WARN,
		    "IPRB: Address setup failed, out of resources.\n");
		return (1);
	}

	/*
	 * Make an individual address setup command out
	 * of an old xmit command.
	 */

	ias = &iprbp->iprb_cu_cmd_block[iprbp->iprb_current_cmd]->ias_cmd;
	ias->ias_cmd = IPRB_IAS_CMD;

	/* Get the ethernet address from the macaddr */
	for (i = 0; i < ETHERADDRL; i++)
		ias->addr[i] = macinfo->gldm_macaddr[i];

	iprb_add_command(macinfo);
	return (0);
}


/*
 * Set up the configuration of the D100.  If promiscuous mode is to
 * be on, we change certain things.
 */
static
void
iprb_configure(gld_mac_info_t *macinfo, int prom)
{
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	struct iprb_cfg_cmd *cfg;

	iprb_reap_commands(macinfo);

	if (iprbp->iprb_current_cmd == iprbp->iprb_first_cmd) {
		cmn_err(CE_WARN, "IPRB: config failed, out of resources.\n");
		return;
	}

	cfg = &iprbp->iprb_cu_cmd_block[iprbp->iprb_current_cmd]->cfg_cmd;
	cfg->cfg_cmd = IPRB_CFG_CMD;
	cfg->cfg_byte0 = IPRB_CFG_B0;
	cfg->cfg_byte1 = IPRB_CFG_B1;
	cfg->cfg_byte2 = IPRB_CFG_B2;
	cfg->cfg_byte3 = IPRB_CFG_B3;
	cfg->cfg_byte4 = IPRB_CFG_B4;
	cfg->cfg_byte5 = IPRB_CFG_B5;
	cfg->cfg_byte6 = IPRB_CFG_B6 | (prom ? IPRB_CFG_B6PROM : 0);
	cfg->cfg_byte7 = IPRB_CFG_B7 |
		(prom ? IPRB_CFG_B7PROM : IPRB_CFG_B7NOPROM);
	cfg->cfg_byte8 =
		((iprbp->iprb_phy_type == IPRB_TYPE_MII) ?
		IPRB_CFG_B8_MII : IPRB_CFG_B8_503);
	cfg->cfg_byte9 = IPRB_CFG_B9;
	cfg->cfg_byte10 = IPRB_CFG_B10;
	cfg->cfg_byte11 = IPRB_CFG_B11;
	cfg->cfg_byte12 = IPRB_CFG_B12;
	cfg->cfg_byte13 = IPRB_CFG_B13;
	cfg->cfg_byte14 = IPRB_CFG_B14;
	cfg->cfg_byte15 = IPRB_CFG_B15 | (prom ?
					IPRB_CFG_B15_PROM :
					IPRB_CFG_B15_NOPROM);
	cfg->cfg_byte16 = IPRB_CFG_B16;
	cfg->cfg_byte17 = IPRB_CFG_B17;
	cfg->cfg_byte18 = IPRB_CFG_B18;
	cfg->cfg_byte19 = IPRB_CFG_B19;
	cfg->cfg_byte20 = IPRB_CFG_B20;
	cfg->cfg_byte21 = IPRB_CFG_B21;
	cfg->cfg_byte22 = IPRB_CFG_B22;
	cfg->cfg_byte23 = IPRB_CFG_B23;

	iprb_add_command(macinfo);
}

/*
 *  iprb_dlsdmult() -- set (enable) or disable a multicast address
 *
 *  Program the hardware to enable/disable the multicast address
 *  in "mcast".  Enable if "op" is non-zero, disable if zero.
 */

/*
 * We keep a list of multicast addresses, because the D100
 * requires that the entire list be uploaded every time a
 * change is made.
 */

int
iprb_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	int  i, s;
	int found = 0;
	int free_index = -1;
	struct iprb_mcs_cmd *mcmd;

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBTRACE)
		cmn_err(CE_CONT, "iprb_dlsdmult(0x%x, %s)", (int)macinfo,
			op ? "ON" : "OFF");
#endif
	for (s = 0; s < IPRB_MAXMCSN; s++) {
		if (iprbp->iprb_mcs_addrval[s] == 0) {
			if (free_index == -1)
				free_index = s;
		} else {
			if (bcmp((caddr_t)&(iprbp->iprb_mcs_addrs[s]),
				(caddr_t)mcast->ether_addr_octet,
				ETHERADDRL) == 0) {
				found = 1;
				break;
			}
		}
	}

	mcmd = &iprbp->iprb_cu_cmd_block[iprbp->iprb_current_cmd]->mcs_cmd;

	if (!op) {
		/* We want to disable the multicast address. */
		if (found) {
			iprbp->iprb_mcs_addrval[s] = 0;
		} else {
			/* Trying to remove non-existant mcast addr */
			cmn_err(CE_WARN,
			    "iprb: Cannot remove non-existing mcast addr.\n");
			return (0);
		}
	} else {
		/* Enable a mcast addr */
		if (!found) {
			if (free_index == -1) {
				cmn_err(CE_WARN,
				    "iprb: No more multicast addrs.\n");
				return (0);
			}

			iprb_reap_commands(macinfo);
			if
				(iprbp->iprb_current_cmd ==
					iprbp->iprb_first_cmd) {
				cmn_err(CE_WARN,
			    "IPRB: Mcast setup failed.  Out of resources.\n");
				return (0);
			}

			bcopy((caddr_t)(mcast->ether_addr_octet),
				(caddr_t)
				&(iprbp->iprb_mcs_addrs[free_index]),
				ETHERADDRL);
			iprbp->iprb_mcs_addrval[free_index] = 1;
		} else {
			cmn_err(CE_WARN, "IPRB: Multicast already in use.\n");
		}


		/* CSTYLED */
		mcmd = &iprbp->iprb_cu_cmd_block[iprbp->iprb_current_cmd]->mcs_cmd;
		mcmd->mcs_cmd = IPRB_MCS_CMD;


		i = 0;

		for (s = 0; s < IPRB_MAXMCSN; s++) {
			if (iprbp->iprb_mcs_addrval[s] != 0) {
				bcopy((caddr_t)&iprbp->iprb_mcs_addrs[s],
					&mcmd->mcs_bytes[i * ETHERADDRL],
					ETHERADDRL);
				i++;
			}
		}

		mcmd->mcs_count = i * ETHERADDRL;

		}

		iprb_add_command(macinfo);

	return (0);
}

/*
 * iprb_prom() -- set or reset promiscuous mode on the board
 *
 *  Program the hardware to enable/disable promiscuous mode.
 *  Enable if "on" is non-zero, disable if zero.
 */

int
iprb_prom(gld_mac_info_t *macinfo, int on)
{

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBTRACE)
		cmn_err(CE_CONT, "iprb_prom(0x%x, %s)", (int)macinfo,
				on ? "ON" : "OFF");
#endif
	iprb_configure(macinfo, on);
	return (0);

}

/*
 * iprb_gstat() -- update statistics
 */
int
iprb_gstat(gld_mac_info_t *macinfo)
{
	dev_info_t *devinfo;
	struct iprbinstance *iprbp;
	struct iprb_stats *stats;
	ddi_dma_handle_t dma_handle_stats;
	ddi_acc_handle_t stats_dma_acchdl;
	ddi_dma_cookie_t dma_cookie;
	uint_t len;
	uint_t ncookies;
	int warning = 1000;

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBTRACE)
		cmn_err(CE_CONT, "iprb_gstat(0x%x)", (int)macinfo);
#endif

	iprbp =	(struct iprbinstance *)macinfo->gldm_private;
	devinfo = iprbp->iprb_dip;

	/* Allocate a DMA handle for the statistics buffer*/
	if (ddi_dma_alloc_handle(devinfo, &stats_buffer_dma_attr,
				 DDI_DMA_SLEEP, 0,
				 &dma_handle_stats)
			!= DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"iprb: could not allocate statistics dma handle\n");
		return (1);
	}

	/* Now allocate memory for the statistics buffer */
	if (ddi_dma_mem_alloc(dma_handle_stats,
			      sizeof(struct iprb_stats),
			      &accattr, 0,
			      DDI_DMA_SLEEP, 0,
		    	      (caddr_t *)&stats, &len,
		    	      &stats_dma_acchdl)
				!= DDI_SUCCESS) {
		cmn_err(CE_WARN, "iprb: could not allocate memory "
				 "for statistics buffer\n");
		ddi_dma_free_handle(&dma_handle_stats);
		return (1);
	}

	/* Clear the buffer */
	bzero((caddr_t)stats, len);

	/* and finally get the DMA address associated with the buffer */
	if (ddi_dma_addr_bind_handle(dma_handle_stats, NULL,
				     (caddr_t)stats, sizeof(struct iprb_stats),
				     DDI_DMA_WRITE, DDI_DMA_SLEEP, 0,
				     &dma_cookie, &ncookies)
			!= DDI_DMA_MAPPED) {
		cmn_err(CE_WARN, "iprb: could not get dma address for "
				 "statistics buffer\n");
		ddi_dma_mem_free(&stats_dma_acchdl);
		ddi_dma_free_handle(&dma_handle_stats);
		return (1);
	}
	ASSERT(ncookies == 1);

	IPRB_SCBWAIT(macinfo);
	outl(macinfo->gldm_port + IPRB_SCB_PTR, dma_cookie.dmac_address);
	bzero((caddr_t)stats, IPRB_STATSIZE);
	outw(macinfo->gldm_port + IPRB_SCB_CMD, IPRB_CU_LOAD_DUMP_ADDR);
	IPRB_SCBWAIT(macinfo);
	outw(macinfo->gldm_port + IPRB_SCB_CMD, IPRB_CU_DUMPSTAT);

	do {
		if (stats->iprb_stat_chkword == IPRB_STAT_COMPLETE)
			break;
		drv_usecwait(10);
	} while (--warning > 0);

	ddi_dma_unbind_handle(dma_handle_stats);

	if (warning == 0) {
		cmn_err(CE_WARN, "IPRB: Statistics generation failed.\n");
		ddi_dma_mem_free(&stats_dma_acchdl);
		ddi_dma_free_handle(&dma_handle_stats);
		return (1);
	}

	macinfo->gldm_stats.glds_collisions = stats->iprb_stat_totcoll;
	macinfo->gldm_stats.glds_crc = stats->iprb_stat_crc;
	macinfo->gldm_stats.glds_short = stats->iprb_stat_short;
	macinfo->gldm_stats.glds_xmtlatecoll = stats->iprb_stat_latecol;
	macinfo->gldm_stats.glds_nocarrier = stats->iprb_stat_crs;

	ddi_dma_mem_free(&stats_dma_acchdl);
	ddi_dma_free_handle(&dma_handle_stats);
	return (0);
}

/*
 *  iprb_reap_commands() - reap commands already processed
 */
static
void
iprb_reap_commands(gld_mac_info_t *macinfo)
{
	int reaper, last_reaped;
	struct iprb_xmit_cmd *xcmd;
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	/* Any commands to be processed ? */
	if (iprbp->iprb_first_cmd != -1) { /* yes */
		reaper = iprbp->iprb_first_cmd;
		last_reaped = -1;

		do {
			/* Get the command to be reaped */
			xcmd = &iprbp->iprb_cu_cmd_block[reaper]->xmit_cmd;

			/* Is it done? */
			if (! (xcmd->xmit_bits & IPRB_CMD_COMPLETE))
				break; /* No */

			/* Make it an XMIT again */
			xcmd->xmit_cmd = IPRB_XMIT_CMD | IPRB_SF;
			xcmd->xmit_count = 0;
			xcmd->xmit_bits = 0;

			/* Free mblks (if any) */
			if (iprbp->iprb_xmit_mp[reaper] != NULL) {
				freemsg(iprbp->iprb_xmit_mp[reaper]);
				iprbp->iprb_xmit_mp[reaper] = NULL;
			}

			/*
			 * Increment to next command to be processed,
			 * looping around if necessary.
			 */

			last_reaped = reaper++;
			if (reaper == iprbp->iprb_nxmits)
				reaper = 0;
		} while (last_reaped != iprbp->iprb_last_cmd);

		/* Did we get them all? */
		if (last_reaped == iprbp->iprb_last_cmd)
			iprbp->iprb_first_cmd = -1; /* Yes */
		else
			iprbp->iprb_first_cmd = reaper;	/* No */
	}
}

/*
 * iprb_add_command() - Modify the command chain so that the current
 *			command is known to be ready.
 */
void
iprb_add_command(gld_mac_info_t *macinfo)
{
	register struct iprb_gen_cmd *current_cmd, *last_cmd;
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	current_cmd
		= &iprbp->iprb_cu_cmd_block[iprbp->iprb_current_cmd]->gen_cmd;
	last_cmd = &iprbp->iprb_cu_cmd_block[iprbp->iprb_last_cmd]->gen_cmd;

	/* Make it so the D100 will suspend upon completion of this cmd */
	current_cmd->gen_cmd |= IPRB_SUSPEND;

	/* Make it so that the D100 will no longer suspend on last command */
	if (iprbp->iprb_current_cmd != iprbp->iprb_last_cmd)
		last_cmd->gen_cmd &= ~IPRB_SUSPEND;

	/* This one is the new last command */
	iprbp->iprb_last_cmd = iprbp->iprb_current_cmd;

	/* If there were previously no commands, this one is first */
	if (iprbp->iprb_first_cmd == -1)
		iprbp->iprb_first_cmd = iprbp->iprb_current_cmd;

	/* Make a new current command, looping around if necessary */
	++iprbp->iprb_current_cmd;

	if (iprbp->iprb_current_cmd == iprbp->iprb_nxmits)
		iprbp->iprb_current_cmd = 0;

	/*
	 * If we think we are about to run out of resources, generate a
	 * software interrupt, so we will ping gld to try again.
	 */
	IPRB_SCBWAIT(macinfo);
	
	if ((iprbp->iprb_current_cmd == iprbp->iprb_first_cmd)
	||  (macinfo->gldm_GLD_flags & GLD_INTR_WAIT)) {
		outw(macinfo->gldm_port + IPRB_SCB_CMD, IPRB_GEN_SWI);
	}

	IPRB_SCBWAIT(macinfo);

	/*
	 * RESUME the D100.  RESUME will be ignored if the CU is
	 * either IDLE or ACTIVE, leaving the state IDLE or Active,
	 * respectively; after the first command, the CU will be
	 * either ACTIVE or SUSPENDED.
	 */

	if (iprbp->iprb_initial_cmd) {
		ddi_dma_cookie_t dma_cookie;
		uint_t ncookies;

		if (ddi_dma_addr_bind_handle(iprbp->iprb_dma_handle_cu, NULL,
					     (caddr_t)iprbp->iprb_cu_cmd_block[iprbp->iprb_last_cmd],
					     sizeof(union iprb_generic_cmd),
					     DDI_DMA_RDWR | DDI_DMA_STREAMING,
					     DDI_DMA_SLEEP, 0, &dma_cookie,
					     &ncookies) != DDI_DMA_MAPPED) {
			cmn_err(CE_WARN, "iprb: could not get DMAable address "
					 "for starting control unit\n");
			/* XXX: perhaps something better could be done here */
			return;
		}
		ddi_dma_unbind_handle(iprbp->iprb_dma_handle_cu);
		iprbp->iprb_initial_cmd = FALSE;
		outl(macinfo->gldm_port + IPRB_SCB_PTR,
			(unsigned long)dma_cookie.dmac_address);
		outw(macinfo->gldm_port + IPRB_SCB_CMD, IPRB_CU_START);
	}
	else
		outw(macinfo->gldm_port + IPRB_SCB_CMD, IPRB_CU_RESUME);
}

/*
 *  iprb_send() -- send a packet
 *
 *  Called when a packet is ready to be transmitted. A pointer to an
 *  M_DATA message that contains the packet is passed to this routine.
 *  The complete LLC header is contained in the message's first message
 *  block, and the remainder of the packet is contained within
 *  additional M_DATA message blocks linked to the first message block.
 *
 *  This routine may NOT free the packet.
 */
int
iprb_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	register int length = msgdsize(mp);
	struct iprbinstance *iprbp;	/* Our private device info */
	dev_info_t *devinfo;
	unsigned int len, i;
	struct iprb_xmit_cmd *xcmd;
	ddi_dma_handle_t dma_handle_tx_buf;
	ddi_dma_cookie_t dma_cookie;
	uint_t ncookies;
	int current;
	mblk_t *lmp;

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBSEND)
		cmn_err(CE_CONT, "iprb_send(0x%x, 0x%x)",
			(int)macinfo, (int)mp);
#endif

	iprbp =	(struct iprbinstance *)macinfo->gldm_private;
	devinfo = iprbp->iprb_dip;

	/* Free up any processed command buffers */
	iprb_reap_commands(macinfo);

	/* If there are no xmit control blocks available, return */
	if ((current = iprbp->iprb_current_cmd) == iprbp->iprb_first_cmd) {
		macinfo->gldm_stats.glds_defer++;
		return (1);
	}

	/* Make sure packet isn't too large */
	if (length > ETHERMAX) {
		cmn_err(CE_WARN,
			"iprb: Transmit packet out of bounds, len %d.\n",
			length);
		freemsg(mp);
		return (0);
	}

	length = 0;
	
	/* Count the number of mblks in list */
	for (i = 0, lmp = mp; lmp != NULL; lmp = lmp->b_cont, ++i)
		;
	/* Now assume worst case, that each mblk crosses (one) page boundry */
	i <<= 1;

	/* Now check to see whether we have enough room */
	if (iprbp->iprb_xmit_buf_desc_cnt[current] < i) {
		/* Release old descriptor array */
		if (iprbp->iprb_xmit_desc_dma_acchdl[current] != NULL) {
			ddi_dma_mem_free(&iprbp->iprb_xmit_desc_dma_acchdl[current]);
			iprbp->iprb_xmit_desc_dma_acchdl[current] = NULL;
		}
		/* Allocate new array (known to be large enough) */
		if (ddi_dma_mem_alloc(iprbp->iprb_dma_handle_txbda,
				      sizeof(struct iprb_xmit_buffer_desc) * i,
				      &accattr, DDI_DMA_STREAMING,
				      DDI_DMA_DONTWAIT, 0,
				      (caddr_t *)&iprbp->iprb_xmit_buf_desc[current],
				      &len,
				      &iprbp->iprb_xmit_desc_dma_acchdl[current])
				!= DDI_SUCCESS) {
			cmn_err(CE_WARN, "iprb: could not allocate transmit "
					 "buffer descriptor array\n");
			iprbp->iprb_xmit_buf_desc_cnt[current] = 0;
			iprbp->iprb_xmit_buf_desc_dma_addr[current] = 0;
			macinfo->gldm_stats.glds_defer++;
			return (1);
		}
		if (ddi_dma_addr_bind_handle(iprbp->iprb_dma_handle_txbda,
					     NULL,
					     (caddr_t)iprbp->iprb_xmit_buf_desc[current],
					     len,
					     DDI_DMA_RDWR | DDI_DMA_STREAMING,
					     DDI_DMA_SLEEP, 0, &dma_cookie,
					     &ncookies) != DDI_DMA_MAPPED) {
			cmn_err(CE_WARN, "iprb: could not get DMAable address "
					 "for transmit buffer descriptor "
					 "array\n");
			macinfo->gldm_stats.glds_defer++;
			return (1);
		}
		ASSERT(ncookies == 1);
		iprbp->iprb_xmit_buf_desc_cnt[current] = i;
		iprbp->iprb_xmit_buf_desc_dma_addr[current]
				= dma_cookie.dmac_address;
		/*
		 * XXX: this really shouldn't be unbound until DMA completes
		 *	but since this is i86pc only code and one i86pc boxes
		 *	all caches are coherent and all DMA addresses are
		 *	physical memory address, this is ok for now.
		 */
		ddi_dma_unbind_handle(iprbp->iprb_dma_handle_txbda);
	}

	/* Prepare current command */
	xcmd = &iprbp->iprb_cu_cmd_block[current]->xmit_cmd;
	xcmd->xmit_bits = 0;
	xcmd->xmit_cmd = IPRB_XMIT_CMD | IPRB_SF;
	xcmd->xmit_tbd = iprbp->iprb_xmit_buf_desc_dma_addr[current];
	xcmd->xmit_count = 0;
	xcmd->xmit_threshold = IPRB_XMIT_THRESHOLD;

	/* Save mp pointer so it can be freed when DMA complete */
	iprbp->iprb_xmit_mp[current] = mp;

	/* Set up transmit buffers */
	i = 0;
	do {
		/*
		 * XXX the allocation of handles needs to be here, but
		 * the freeing needs to be done when the DMA is complete,
		 * i.e. when iprb_reap_commands() is done.
		 * This means we need to keep track of each handle
		 * allocated in this routine and since there may be multiple
		 * handles per xmit, this gets sticky.
		 */
		/* Prepare for DMA mapping of tx buffer(s) */
		if (ddi_dma_alloc_handle(devinfo, &tx_buffer_dma_attr,
					 DDI_DMA_SLEEP, 0,
					 &dma_handle_tx_buf)
				!= DDI_SUCCESS) {
			cmn_err(CE_WARN,
				"iprb: could not allocate transmit buffer dma "
				"handle\n");
			macinfo->gldm_stats.glds_defer++;
			iprbp->iprb_xmit_mp[current] = NULL;
			return (1);
		}

		len = mp->b_wptr - mp->b_rptr;
		if (ddi_dma_addr_bind_handle(dma_handle_tx_buf,
					     NULL, (caddr_t) mp->b_rptr, len,
					     DDI_DMA_READ | DDI_DMA_STREAMING,
					     DDI_DMA_SLEEP, 0, &dma_cookie,
					     &ncookies) != DDI_DMA_MAPPED) {
			cmn_err(CE_WARN, "iprb: couldn't translate TX buffer's "
					 "DMA address\n");
			ddi_dma_free_handle(&dma_handle_tx_buf);
			macinfo->gldm_stats.glds_defer++;
			iprbp->iprb_xmit_mp[current] = NULL;
			return (1);
		}

		for (;;) {
			iprbp->iprb_xmit_buf_desc[current][i].iprb_buffer_size
					= dma_cookie.dmac_size;
			iprbp->iprb_xmit_buf_desc[current][i].iprb_buffer_addr
					= dma_cookie.dmac_address;
			++i;
			if (--ncookies == 0)
				break;
			ddi_dma_nextcookie(dma_handle_tx_buf, &dma_cookie);
		}
		length += (unsigned int)len;
		mp = mp->b_cont;
		ddi_dma_unbind_handle(dma_handle_tx_buf);
		ddi_dma_free_handle(&dma_handle_tx_buf);
	} while (mp != NULL);

	xcmd->xmit_tbdnum = i;

	iprb_add_command(macinfo);

	return (0);		/* successful transmit attempt */
}

/*
 *  iprbintr() -- interrupt from board to inform us that a receive or
 *  transmit has completed.
 */

/*
 *
 *    IMPORTANT NOTE - Due to a bug in the D100 controller, the
 *			suspend/resume functionality for receive
 *			is not reliable.  We are therefore using
 *			the EL bit, along with RU_START.  In order
 *			to do this properly, it is very important
 *			that the order of interrupt servicing remains
 *			intact; more specifically, it is important
 *			that the FR interrupt bit is serviced before
 *			the RNR interrupt bit is serviced, because
 *			both bits will be set when an RNR condition
 *			occurs, and we must process the received
 *			frames before we restart the receive unit.
 */
u_int
iprbintr(gld_mac_info_t *macinfo)
{
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	short scb_status;
	int intr_status;
	int ret;

#ifdef IPRBDEBUG
	if (iprbdebug & IPRBINT)
		cmn_err(CE_CONT, "iprbintr(0x%x)", macinfo);
#endif

	ret = DDI_INTR_UNCLAIMED;

	/* Free up any processed command buffers */
	iprb_reap_commands(macinfo);

	/* Get interrupt bits */
	scb_status = inw(macinfo->gldm_port + IPRB_SCB_STATUS);
	while ((intr_status = (scb_status & IPRB_SCB_INTR_MASK)) != 0) {
		ret = DDI_INTR_CLAIMED;

		/* Acknowledge all interrupts */
		outw(macinfo->gldm_port + IPRB_SCB_STATUS, intr_status);

		/* Frame received */
		if (intr_status & IPRB_INTR_FR)
			iprb_process_recv(macinfo);

		/* Out of resources on receive condition */
		if (intr_status & IPRB_INTR_RNR) {

			/* Reset End-of-List */
			iprbp->iprb_ru_frame_desc[iprbp->iprb_last_rfd]
					->rfd_control &= IPRB_RFD_EL;
			iprbp->iprb_ru_frame_desc[iprbp->iprb_nframes - 1]
					->rfd_control |= IPRB_RFD_EL;

			/* Reset driver's pointers */
			iprbp->iprb_first_rfd = 0;
			iprbp->iprb_last_rfd = iprbp->iprb_nframes - 1;
			iprbp->iprb_current_rfd = 0;

			/* and start at first RFD again */
			outl(macinfo->gldm_port + IPRB_SCB_PTR,
			     iprbp->iprb_ru_frame_addr[0]);
			outw(macinfo->gldm_port + IPRB_SCB_CMD, IPRB_RU_START);
		}

		if (intr_status & IPRB_INTR_CXTNO)
			iprbp->iprb_cxtnos++;

		if (intr_status & IPRB_INTR_CNACI)
			iprbp->iprb_cnacis++;

		/* Should never get this interrupt */

		if (intr_status & IPRB_INTR_MDI)
			cmn_err(CE_WARN, "IPRB: Received MDI interrupt.\n");

		scb_status = inw(macinfo->gldm_port + IPRB_SCB_STATUS);
	}
	if (ret == DDI_INTR_CLAIMED)
		macinfo->gldm_stats.glds_intr++;

	/* Make sure we generate a new interrupt if we are deferring xmits */
	IPRB_SCBWAIT(macinfo);
	if ((iprbp->iprb_current_cmd == iprbp->iprb_first_cmd)
	||  (macinfo->gldm_GLD_flags & GLD_INTR_WAIT)) {
		outw(macinfo->gldm_port + IPRB_SCB_CMD, IPRB_GEN_SWI);
	}

	return (ret);	/* Indicate whether or not it was our interrupt */
}


/*
 *  iprb_start_board() -- start the board receiving.
 */

int
iprb_start_board(gld_mac_info_t *macinfo)
{
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	iprbp->iprb_receive_enabled = TRUE;
	iprbp->iprb_cxtnos = 0;
	iprbp->iprb_cnacis = 0;

	/* Start the receiver unit */
	outl(macinfo->gldm_port + IPRB_SCB_PTR,
	     iprbp->iprb_ru_frame_addr[iprbp->iprb_current_rfd]);
	outw(macinfo->gldm_port + IPRB_SCB_CMD, IPRB_RU_START);

	return (DDI_SUCCESS);
}


/* iprb_stop_board() - disable receiving */
int
iprb_stop_board(gld_mac_info_t *macinfo)
{
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	iprbp->iprb_receive_enabled = FALSE;

	return (DDI_SUCCESS);
}

/* Code to process all of the receive packets */
static
void
iprb_process_recv(gld_mac_info_t *macinfo)
{
	struct iprbinstance *iprbp =	/* Our private device info */
		(struct iprbinstance *)macinfo->gldm_private;

	int save_end, current;
	mblk_t *mp;
	unsigned short rcv_len;
	uint_t len;
	struct iprb_rfd *rfd, *rfd_end;
	struct iprb_rbd *rbd;
	struct iprb_buf_free *free_pkt;
	caddr_t newbuf;
	ddi_acc_handle_t new_acchdl;
	ddi_dma_cookie_t dma_cookie;
	uint_t ncookies;
	extern mblk_t *desballoc(unsigned char *, int, int, frtn_t *);

	save_end = iprbp->iprb_last_rfd;   /* Remember the last RFD */

	do {
		/* Start with the current one */
		current = iprbp->iprb_current_rfd;
		rfd = iprbp->iprb_ru_frame_desc[current];

		/* Is it complete? */
		if (! (rfd->rfd_status & IPRB_RFD_COMPLETE))
			break;

		if (! (rfd->rfd_status & IPRB_RFD_OK)) {
			int found_reason = FALSE;

			if (rfd->rfd_status & IPRB_RFD_CRC_ERR) {
				cmn_err(CE_WARN, "iprb: CRC error in received "
						 "frame\n");
				found_reason = TRUE;
			}
			if (rfd->rfd_status & IPRB_RFD_ALIGN_ERR) {
				cmn_err(CE_WARN, "iprb: alignment error in "
						 "received frame\n" );
				found_reason = TRUE;
			}
			if (rfd->rfd_status & IPRB_RFD_NO_BUF_ERR) {
				cmn_err(CE_WARN, "iprb: no buffer space while"
						 " receiving frame\n");
				found_reason = TRUE;
				break;
			}
			if (rfd->rfd_status & IPRB_RFD_DMA_OVERRUN) {
				cmn_err(CE_WARN, "iprb: DMA overrung while "
						 "receiving frame\n");
				found_reason = TRUE;
			}
			if (rfd->rfd_status & IPRB_RFD_SHORT_ERR) {
				cmn_err(CE_WARN, "iprb: received short "
						 "frame\n");
				found_reason = TRUE;
			}
			if (rfd->rfd_status & IPRB_RFD_PHY_ERR) {
				cmn_err(CE_WARN, "iprb: physical media error "
						 "while receiving frame\n");
				found_reason = TRUE;
			}
			if (rfd->rfd_status & IPRB_RFD_COLLISION) {
				cmn_err(CE_WARN, "iprb: collision occured "
						 "while receiving frame\n");
				found_reason = TRUE;
			}
			if (! found_reason) {
				cmn_err(CE_WARN, "iprb: received frame is not "
						 "ok, but for an unknown "
						 "reason\n");
			}
			macinfo->gldm_stats.glds_errrcv++;
			goto failed_receive;
		}

		rbd = iprbp->iprb_rcv_buf_desc[current];
		/* Get the length from the RFD */
		rcv_len = IPRB_RBD_COUNT(rbd->rbd_count);

		if (iprbp->iprb_receive_enabled) {
			/* Get buffer tracking data structure */
			free_pkt = kmem_alloc(sizeof(*free_pkt), KM_NOSLEEP);
			if (free_pkt == NULL) {
				macinfo->gldm_stats.glds_norcvbuf++;
				goto failed_receive;
			}
			/* Fill in completion routine... */
			free_pkt->free_rtn.free_func = iprb_rcv_complete;
			/* Pointer to ourself... */
			free_pkt->free_rtn.free_arg = (char *)free_pkt;
			/* Pointer to iprb instance */
			free_pkt->iprbp = iprbp;

			/* Try to replace the buf we are about to give away */
#if defined(_DDI_DMA_MEM_ALLOC_FIXED)
			if (ddi_dma_mem_alloc(iprbp->iprb_dma_handle_rcv_buf,
#else /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
			if (iprb_dma_mem_alloc(iprbp->iprb_dma_handle_rcv_buf,
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
					       IPRB_FRAMESIZE,
					       &accattr, DDI_DMA_STREAMING,
					       DDI_DMA_DONTWAIT, 0,
					       &newbuf,
					       &len,
					       &new_acchdl) != DDI_SUCCESS) {
				/* Give back tracking structure */
				kmem_free((void *)free_pkt, sizeof(*free_pkt));
				macinfo->gldm_stats.glds_norcvbuf++;
				goto failed_receive;
			}

			/* CSTYLED */
			if (ddi_dma_addr_bind_handle(iprbp->iprb_dma_handle_rcv_buf,
						     NULL,
						     newbuf,
						     IPRB_FRAMESIZE,
						     DDI_DMA_WRITE
						     | DDI_DMA_STREAMING,
						     DDI_DMA_DONTWAIT, 0,
						     &dma_cookie, &ncookies)
							!= DDI_DMA_MAPPED) {
				cmn_err(CE_WARN, "iprb: could not translate "
						 "receive buffer address\n");
				/* Give back new buffer */
#if defined(_DDI_DMA_MEM_ALLOC_FIXED)
				ddi_dma_mem_free(&new_acchdl);
#else /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
				iprb_dma_mem_free(&new_acchdl);
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
				/* Give back tracking structure */
				kmem_free((void *)free_pkt, sizeof(*free_pkt));
				macinfo->gldm_stats.glds_norcvbuf++;
				goto failed_receive;
			}
			ASSERT(ncookies == 1);

			ddi_dma_unbind_handle(iprbp->iprb_dma_handle_rcv_buf);

			mp = desballoc(
				(unsigned char *)
				iprbp->iprb_rcv_pool[current].iprb_rcv_buf,
				rcv_len, BPRI_MED, &free_pkt->free_rtn);
			if (mp == NULL) {
				/* Give back resources garnered so far */
#if defined(_DDI_DMA_MEM_ALLOC_FIXED)
				ddi_dma_mem_free(&new_acchdl);
#else /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
				iprb_dma_mem_free(&new_acchdl);
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
				kmem_free((void *)free_pkt, sizeof(*free_pkt));
				macinfo->gldm_stats.glds_norcvbuf++;
				goto failed_receive;
			}

			/* Fill in dma access handle of received buffer */
			/* CSTYLED */
			free_pkt->dma_acchdl
				= iprbp->iprb_rcv_pool[current].iprb_rcv_buf_dma_acchdl;
			/* Now replace old buffer with the new one */
			iprbp->iprb_rcv_pool[current].iprb_rcv_buf = newbuf;
			iprbp->iprb_rcv_pool[current].iprb_rcv_buf_addr
				= dma_cookie.dmac_address;
			iprbp->iprb_rcv_pool[current].iprb_rcv_buf_dma_acchdl
				= new_acchdl;
			rbd->rbd_buffer = dma_cookie.dmac_address;

			mutex_enter(&iprbp->iprb_rcv_buf_mutex);
			++iprbp->iprb_rcv_bufs_outstanding;
			mutex_exit(&iprbp->iprb_rcv_buf_mutex);
			mp->b_wptr += rcv_len;

			/* Send it up */
			gld_recv(macinfo, mp);
		}

failed_receive:
		rfd_end = iprbp->iprb_ru_frame_desc[iprbp->iprb_last_rfd];

		/* This one becomes the new end-of-list */
		rfd->rfd_control |= IPRB_RFD_EL;
		rfd->rfd_count = 0;
		rfd->rfd_status = 0;

		rbd->rbd_count = 0;

		/* Turn off end-of-list in previous buffer descriptor */
		rbd = iprbp->iprb_rcv_buf_desc[iprbp->iprb_last_rfd];

		iprbp->iprb_last_rfd = current;

		/* Turn off EL bit on old end-of-list */
		rfd_end->rfd_control &= ~IPRB_RFD_EL;

		/* Current one moves up, looping around if necessary */
		iprbp->iprb_current_rfd++;
		if (iprbp->iprb_current_rfd == iprbp->iprb_nframes)
			iprbp->iprb_current_rfd = 0;

	} while (iprbp->iprb_last_rfd != save_end);
}

/*
 * Code directly from Intel spec to read EEPROM data for the ethernet
 * address, one bit at a time.
 */

static
void
iprb_readia(
	gld_mac_info_t *macinfo,
	unsigned short *addr,
	unsigned short offset)
{

	register unsigned short eex;

	eex = inb(macinfo->gldm_port + IPRB_SCB_EECTL);
	eex &= ~(IPRB_EEDI | IPRB_EEDO | IPRB_EESK);
	eex |= IPRB_EECS;
	outb(macinfo->gldm_port + IPRB_SCB_EECTL, eex);
	iprb_shiftout(macinfo, IPRB_EEPROM_READ, 3);
	iprb_shiftout(macinfo, offset, 6);
	*addr = iprb_shiftin(macinfo);
	iprb_eeclean(macinfo);


}

static
void
iprb_shiftout(
	gld_mac_info_t *macinfo,
	unsigned short data,
	unsigned short count)
{
	unsigned short eex, mask;

	mask = 0x01 << (count - 1);
	eex = inb(macinfo->gldm_port + IPRB_SCB_EECTL);
	eex &= ~(IPRB_EEDO | IPRB_EEDI);

	do {
		eex &= ~IPRB_EEDI;
		if (data & mask)
		    eex |= IPRB_EEDI;

		outb(macinfo->gldm_port + IPRB_SCB_EECTL, eex);
		drv_usecwait(100);
		iprb_raiseclock(macinfo, (unsigned short *)&eex);
		iprb_lowerclock(macinfo, (unsigned short *)&eex);
		mask = mask >> 1;
	} while (mask);

	eex &= ~IPRB_EEDI;
	outb(macinfo->gldm_port + IPRB_SCB_EECTL, eex);
}

static void
iprb_raiseclock(gld_mac_info_t *macinfo, unsigned short *eex)
{
	*eex = *eex | IPRB_EESK;
	outb(macinfo->gldm_port + IPRB_SCB_EECTL, *eex);
	drv_usecwait(100);
}

static void
iprb_lowerclock(gld_mac_info_t *macinfo, unsigned short *eex)
{
	*eex = *eex & ~IPRB_EESK;
	outb(macinfo->gldm_port + IPRB_SCB_EECTL, *eex);
	drv_usecwait(100);
}

int
iprb_shiftin(gld_mac_info_t *macinfo)
{
	unsigned short x, d, i;
	x = inb(macinfo->gldm_port + IPRB_SCB_EECTL);
	x &= ~(IPRB_EEDO|IPRB_EEDI);
	d = 0;

	for (i = 0; i < 16; i++) {
		d = d << 1;
		iprb_raiseclock(macinfo, &x);
		x = inb(macinfo->gldm_port + IPRB_SCB_EECTL);
		x &= ~(IPRB_EEDI);
		if (x & IPRB_EEDO)
		    d |= 1;

		iprb_lowerclock(macinfo, &x);
	}

	return (d);
}

static void
iprb_eeclean(gld_mac_info_t *macinfo)
{
	unsigned short eex;

	eex = inb(macinfo->gldm_port + IPRB_SCB_EECTL);
	eex &= ~(IPRB_EECS | IPRB_EEDI);
	outb(macinfo->gldm_port + IPRB_SCB_EECTL, eex);

	iprb_raiseclock(macinfo, &eex);
	iprb_lowerclock(macinfo, &eex);
}

static void
iprb_mdi_read(gld_mac_info_t *macinfo, unsigned char reg_addr,
		unsigned char phy_addr, unsigned short *result)
{
	unsigned long phy_command = 0;
	unsigned long out_data = 0;
	short timeout = 10000;

	phy_command = (reg_addr << 16) | (phy_addr << 21) |
		(IPRB_MDI_READ << 26);
	outl(macinfo->gldm_port + IPRB_SCB_MDICTL, phy_command);

	while ((--timeout) &&
		(!((out_data = inl(macinfo->gldm_port + IPRB_SCB_MDICTL)) &
		IPRB_MDI_READY)))
		drv_usecwait(100);

	*result = (unsigned short) out_data;
}

/*
 * Release all DMA resources in the opposite order from acquisition
 */
static
void
iprb_release_dma_resources(register struct iprbinstance *iprbp)
{
	register int i;

	/* Free the receive buffers */
	for (i = 0; i < iprbp->iprb_nframes; ++i) {
		if (iprbp->iprb_rcv_pool[i].iprb_rcv_buf_dma_acchdl != NULL) {
#if defined(_DDI_DMA_MEM_ALLOC_FIXED)
			/* CSTYLED */
			ddi_dma_mem_free(&iprbp->iprb_rcv_pool[i].iprb_rcv_buf_dma_acchdl);
#else /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
			/* CSTYLED */
			iprb_dma_mem_free(&iprbp->iprb_rcv_pool[i].iprb_rcv_buf_dma_acchdl);
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
			iprbp->iprb_rcv_pool[i].iprb_rcv_buf_dma_acchdl = NULL;
		}
	}
	/* Release the receive buffer dma handle */
	if (iprbp->iprb_dma_handle_rcv_buf != NULL) {
		ddi_dma_free_handle(&iprbp->iprb_dma_handle_rcv_buf);
		iprbp->iprb_dma_handle_rcv_buf = NULL;
	}
	/* Free the receive buffer descriptors */
	for (i = 0; i < iprbp->iprb_nframes; ++i) {
		if (iprbp->iprb_rcv_desc_dma_acchdl[i] != NULL) {
			ddi_dma_mem_free(&iprbp->iprb_rcv_desc_dma_acchdl[i]);
			iprbp->iprb_rcv_desc_dma_acchdl[i] = NULL;
		}
	}
	/* Release the receive buffer descriptor dma handle */
	if (iprbp->iprb_dma_handle_rcvbda != NULL) {
		ddi_dma_free_handle(&iprbp->iprb_dma_handle_rcvbda);
		iprbp->iprb_dma_handle_rcvbda = NULL;
	}
	/* Free the receive frame descriptors */
	for (i = 0; i < iprbp->iprb_nframes; ++i) {
		if (iprbp->iprb_ru_dma_acchdl[i] != NULL) {
			ddi_dma_mem_free(&iprbp->iprb_ru_dma_acchdl[i]);
			iprbp->iprb_ru_dma_acchdl[i] = NULL;
		}
	}
	/* Release the receive frame descriptor handle */
	if (iprbp->iprb_dma_handle_ru != NULL) {
		ddi_dma_free_handle(&iprbp->iprb_dma_handle_ru);
		iprbp->iprb_dma_handle_ru = NULL;
	}
	/* Free the transmit buffer descriptors */
	for (i = 0; i < iprbp->iprb_nxmits; ++i) {
		if (iprbp->iprb_xmit_desc_dma_acchdl[i] != NULL) {
			ddi_dma_mem_free(&iprbp->iprb_xmit_desc_dma_acchdl[i]);
			iprbp->iprb_xmit_desc_dma_acchdl[i] = NULL;
		}
	}
	if (iprbp->iprb_dma_handle_txbda != NULL) {
		ddi_dma_free_handle(&iprbp->iprb_dma_handle_txbda);
		iprbp->iprb_dma_handle_txbda = NULL;
	}
	for (i = 0; i < iprbp->iprb_nxmits; ++i) {
		if (iprbp->iprb_cu_dma_acchdl[i] != NULL) {
			ddi_dma_mem_free(&iprbp->iprb_cu_dma_acchdl[i]);
			iprbp->iprb_cu_dma_acchdl[i] = NULL;
		}
	}
	if (iprbp->iprb_dma_handle_cu != NULL) {
		ddi_dma_free_handle(&iprbp->iprb_dma_handle_cu);
		iprbp->iprb_dma_handle_cu = NULL;
	}
}

static
void
iprb_rcv_complete(struct iprb_buf_free *freep)
{
	struct iprbinstance *iprbp;

	iprbp = freep->iprbp;

	/* One less outstanding receive buffer */
	mutex_enter(&iprbp->iprb_rcv_buf_mutex);
	--iprbp->iprb_rcv_bufs_outstanding;
	mutex_exit(&iprbp->iprb_rcv_buf_mutex);

	/* We can give back this receive buffer now */
#if defined(_DDI_DMA_MEM_ALLOC_FIXED)
	ddi_dma_mem_free(&freep->dma_acchdl);
#else /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
	iprb_dma_mem_free(&freep->dma_acchdl);
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */

	/* and the structure we used to keep track of things */
	kmem_free((void *)freep, sizeof(*freep));
}

static
void
iprb_release_mblks(register struct iprbinstance *iprbp)
{
	register int i;

	for (i = 0; i < iprbp->iprb_nxmits; ++i) {
		if (iprbp->iprb_xmit_mp[i] != NULL)
			freemsg(iprbp->iprb_xmit_mp[i]);
	}
}

/* ARGSUSED */
static
void
iprb_wait(caddr_t arg)
{
}

#if ! defined(_DDI_DMA_MEM_ALLOC_FIXED)
/*
 * Functions to temporarily replace ddi_dma_mem_xxx() for receive buffer
 * memory as the x86 versions of ddi_dma_mem_xxx() rely on a scarce
 * resource.  Remove when the x86 ddi_dma_mem functions are fixed.
 * N.B.: This is not a general purpose DMA memory allocator.  It assumes
 * that DMA addresses are physical address and that any given allocation
 * can span only one page boundry.
 */
struct dma_mem {
	caddr_t vaddr;
	unsigned int length;
};

/* Maximum number of retries for contiguous memory */
#define MAX_RETRIES 10
/* Statistics counter for number of times first try was not contiguous */
static unsigned long phys_contig_miss;
/* Statistics counters for number of times retries were not contiguous */
static unsigned long phys_contig_retry_miss[MAX_RETRIES];

/* ARGSUSED */
static
int
iprb_dma_mem_alloc(
	ddi_dma_handle_t handle,
	uint_t length,
	ddi_device_acc_attr_t *accattrp,
	ulong_t flags,
	int (*waitfp)(caddr_t),
	caddr_t arg,
	caddr_t *kaddrp,
	uint_t *real_length,
	ddi_acc_handle_t *handlep)
{
	int cansleep;
	u_int start_dma_page, end_dma_page;
	int retries;
	caddr_t unusable_addrs[MAX_RETRIES];

	/* translate ddi_dma_mem_alloc sleep flag to kmem_alloc's */
	cansleep = (waitfp == DDI_DMA_SLEEP) ? KM_SLEEP : KM_NOSLEEP;
	/* get some memory to keep track of this allocation */
	*handlep = (ddi_acc_handle_t *) kmem_alloc(sizeof(struct dma_mem),
						  cansleep);
	/* If we can't keep track, we cannot service the request */
	if (*handlep == NULL)
		return (DDI_FAILURE);

	/* Now try for the requested memory itself */
	*kaddrp = (caddr_t) kmem_alloc(length, cansleep);

	/* No? Free the tracking structure */
	if (*kaddrp == NULL) {
		kmem_free((void *) *handlep, sizeof(struct dma_mem));
		return (DDI_FAILURE);
	}

	/* If memory is physically contiguous great */
	start_dma_page = hat_getkpfnum(*kaddrp);
	end_dma_page = hat_getkpfnum(*kaddrp + length - 1);
	if ((start_dma_page == end_dma_page)
	||  ((start_dma_page + 1) == end_dma_page)) {
		((struct dma_mem *) *handlep)->vaddr = *kaddrp;
		((struct dma_mem *) *handlep)->length = length;
		*real_length = length;
		return (DDI_SUCCESS);
	}

	++phys_contig_miss;

	/* Was not physically contiguous, try harder */
	for (retries = 0; retries < MAX_RETRIES; ++retries) {
		unusable_addrs[retries] = *kaddrp;
		*kaddrp = (caddr_t) kmem_alloc(length, cansleep);
		if (*kaddrp == NULL)
			break;

		/* If memory is physically contiguous great */
		start_dma_page = hat_getkpfnum(*kaddrp);
		end_dma_page = hat_getkpfnum(*kaddrp + length - 1);
		if ((start_dma_page == end_dma_page)
		||  ((start_dma_page + 1) == end_dma_page)) {
			((struct dma_mem *) *handlep)->vaddr = *kaddrp;
			((struct dma_mem *) *handlep)->length = length;
			*real_length = length;
			while (retries >= 0) {
				kmem_free((void *) unusable_addrs[retries],
					  length);
				--retries;
			}
			return (DDI_SUCCESS);
		}
		++phys_contig_retry_miss[retries];
	}
	while (retries >= 0) {
		kmem_free((void *) unusable_addrs[retries], length);
		--retries;
	}
	if (*kaddrp != NULL)
		kmem_free((void *) *kaddrp, length);

	kmem_free((void *) *handlep, sizeof(struct dma_mem));

	return (DDI_FAILURE);
}

static
void
iprb_dma_mem_free(ddi_acc_handle_t *handlep)
{
	kmem_free((void *) ((struct dma_mem *) *handlep)->vaddr,
		  ((struct dma_mem *) *handlep)->length);
	kmem_free((void *) *handlep, sizeof(struct dma_mem));
}
#endif /* ! defined(_DDI_DMA_MEM_ALLOC_FIXED) */
