/*
 * Copyright (c) 1995, 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * dnet --	DEC 21040/21140
 * Depends on the Generic LAN Driver utility functions in /kernel/misc/gld
 */
/*
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.	Users
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

#pragma ident "@(#)dnet.c	1.20	96/10/09 SMI"

#include <sys/types.h>
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
#include <sys/sunddi.h>
#include <sys/debug.h>
#include <sys/ksynch.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/strsun.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/gld.h>
#include <sys/dnet.h>
#include <sys/pci.h>

#ifdef PCI_DDI_EMULATION
char _depends_on[] = "misc/xpci misc/gld";
#else
char _depends_on[] = "misc/gld";
#endif

/*
 *	Declarations and Module Linkage
 */

static char ident[] = "DNET 21040/21140";
/*
 * Uncomment if DL_TPR device
static int Use_Group_Addr = 0;
 */

#ifdef DNETDEBUG
/* used for debugging */
/* int	dnetdebug = DNETTRACE|DNETSEND|DNETINT|DNETRECV|DNETDDI; */
int dnetdebug = 0;
#endif

/* Required system entry points */
static	dnetidentify(dev_info_t *);
static	dnetdevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	dnetprobe(dev_info_t *);
static	dnetattach(dev_info_t *, ddi_attach_cmd_t);
static	dnetdetach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
int	dnet_reset(gld_mac_info_t *);
int	dnet_start_board(gld_mac_info_t *);
int	dnet_stop_board(gld_mac_info_t *);
int	dnet_saddr(gld_mac_info_t *);
int	dnet_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
int	dnet_prom(gld_mac_info_t *, int);
int	dnet_gstat(gld_mac_info_t *);
int	dnet_send(gld_mac_info_t *, mblk_t *);
u_int	dnetintr(gld_mac_info_t *);

/* Internal functions used by the above entry points */
static void dnet_getp(gld_mac_info_t *);
static int dnet_setup_conf(gld_mac_info_t *, dev_info_t *);
static unsigned int hashindex(u_char *);
static int dnet_init_txrx_bufs(gld_mac_info_t *, caddr_t);
static int dnet_alloc_bufs(gld_mac_info_t *);
static void dnet_read21040addr(gld_mac_info_t *);
static void dnet_read21140srom(gld_mac_info_t *, char *, int, int);
static int dnet_init21140csr12(gld_mac_info_t *);
static int dnet_getIRQ_PCI(dev_info_t *devinfo, int irq);
static int dnet_init_board(gld_mac_info_t *macinfo);
static void update_tx_stats(gld_mac_info_t *macinfo, int index);
static void update_rx_stats(gld_mac_info_t *macinfo, int index);
static void dnet_detectmedia(gld_mac_info_t *macinfo);
static int dnet_init21040(gld_mac_info_t *macinfo);
static void dnet_ProcessXmitIntr(gld_mac_info_t *macinfo, struct dnetinstance
					*dnetp);

/* Standard Streams initialization */
static struct module_info minfo = {
	DNETIDNUM, "dnet", 0, INFPSZ, DNETHIWAT, DNETLOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

struct streamtab dnetinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */
extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_dnetops = {
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
	&dnetinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

struct dev_ops dnetops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	dnetdevinfo,		/* devo_getinfo */
	dnetidentify,		/* devo_identify */
	dnetprobe,		/* devo_probe */
	dnetattach,		/* devo_attach */
	dnetdetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_dnetops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&dnetops		/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
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

/*
 *	DDI Entry Points
 */

/*
 * identify(9E) -- See if we know about this device
 */
dnetidentify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "dnet") == 0)
		return (DDI_IDENTIFIED);
	else if (strcmp(ddi_get_name(devinfo), "pci1011,2") == 0)
		return (DDI_IDENTIFIED);
	else if (strcmp(ddi_get_name(devinfo), "pci1011,9") == 0)
		return (DDI_IDENTIFIED);
	else if (strcmp(ddi_get_name(devinfo), "pci1011,14") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

/*
 * getinfo(9E) -- Get device driver information
 */
/*ARGSUSED*/
dnetdevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int 	error;

	/*
	 * This code is not DDI compliant: the correct semantics
	 * for CLONE devices is not well-defined yet.
	 */
	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if (devinfo == NULL) {
			error = DDI_FAILURE;	/* Unfortunate */
		} else {
			*result = (void *) devinfo;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *) 0;	/* This CLONEDEV always returns zero */
		error = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}
	return (error);
}

/*
 * probe(9E) -- Determine if a device is present
 */
dnetprobe(dev_info_t *devinfo)
{
	ddi_acc_handle_t handle;
	short		vendorid;
	short		deviceid;


#ifdef DNETDEBUG
	if (dnetdebug & DNETDDI)
		cmn_err(CE_CONT, "dnetprobe(0x%x)", devinfo);
#endif

	if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS) {
		return (DDI_PROBE_FAILURE);
	}

	vendorid = pci_config_getw(handle, PCI_CONF_VENID);

	if (vendorid != DEC_VENDOR_ID) {
		pci_config_teardown(&handle);
		return (DDI_PROBE_FAILURE);
	}

	deviceid = pci_config_getw(handle, PCI_CONF_DEVID);
	switch (deviceid) {

	case DEVICE_ID_21040 :
	case DEVICE_ID_21041 :
	case DEVICE_ID_21140 :
		break;
	default :
		pci_config_teardown(&handle);
		return (DDI_PROBE_FAILURE);
	}


#if	defined(__ppc)
	/*
	 * The Motorola conventional firmware leaves the 21040 set to
	 * I/O address 0x1000100.	No, that's not a typo, and it's
	 * not a memory address.	PCI allows 32-bit I/O space,
	 * even though Intel processors can generate only 16 bits.
	 *
	 * Our framework (1) doesn't re-init this and (2) can't
	 * handle big addresses like this.	Full porting to the
	 * 2.5 DDI "common access" mechanisms should allow this
	 * to work.	Until then, we'll just force it to a lower
	 * address.
	 */
	if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS) {
		return (DDI_PROBE_FAILURE);
	}
	cmn_err(CE_WARN,
		"DNET:	I/O address forced to dc00, should be dynamic");
	pci_config_putl(handle, DNET_PCI_BASEADDR, 0xdc00);
	pci_config_teardown(&handle);
#endif
	return (DDI_PROBE_SUCCESS);
}

static void
set_vendor_type(struct dnetinstance *dnetp, char vendorinfo[])
{
	unsigned long ether_mfg = 0;
	int i;

	/*
	 * Different vendors have different ways of using the SROM.
	 * in case of Asante, there is no other way to check the vendor
	 * other than checking the ethernet address.
	 * For Cogent card two bytes in SROM specify the vendor and revision
	 * of card.
	 */

	for (i = 0; i < 3; i++) {
		ether_mfg = ((ether_mfg << 8) | (unsigned char) vendorinfo[i]);
	}

	if (ether_mfg == ASANTE_ETHER) {
		dnetp->vendor_21140 = ASANTE_TYPE;
		dnetp->vendor_revision = 0;
	} else if ((vendorinfo[VENDOR_ID_OFFSET] == COGENT_EM100) ||
	    (vendorinfo[VENDOR_ID_OFFSET] == COGENT_EM110TX)) {
		dnetp->vendor_21140 = COGENT_EM100_TYPE;
		dnetp->vendor_revision = vendorinfo[VENDOR_REVISION_OFFSET];
	} else {
		dnetp->vendor_21140 = DEFAULT_TYPE;
		dnetp->vendor_revision = 0;
	}

}




/*
 * attach(9E) -- Attach a device to the system
 *
 * Called once for each board successfully probed.
 */
dnetattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	struct dnetinstance *dnetp;		/* Our private device info */
	gld_mac_info_t	*macinfo;		/* GLD structure */
	int		iobase;
	char 		vendor_info[SROM_SIZE];
	int		irq;
	int		board_type;
	int		csr;
	short		deviceid;
	ddi_acc_handle_t handle;

#ifdef DNETDEBUG
	if (dnetdebug & DNETDDI)
		cmn_err(CE_CONT, "dnetattach(0x%x)", devinfo);
#endif


	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);
	if (pci_config_setup(devinfo, &handle) != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	deviceid = pci_config_getw(handle, PCI_CONF_DEVID);
	switch (deviceid) {

	case DEVICE_ID_21040 :
		board_type = DEVICE_ID_21040;
		break;
	case DEVICE_ID_21041 :
		board_type = DEVICE_ID_21041;
		break;
	case DEVICE_ID_21140 :
		board_type = DEVICE_ID_21140;
		break;
	default :
		pci_config_teardown(&handle);
		return (DDI_FAILURE);
	}

	iobase = pci_config_getw(handle, PCI_CONF_BASE0);
	if (iobase & 0xffff0000) {
		cmn_err(CE_NOTE, "DNET : I/O Base address out of range\n");
		pci_config_teardown(&handle);
		return (DDI_FAILURE);
	}
	iobase = iobase & 0xFFFE;
	/*
	 * Get the IRQ
	 */
	irq = pci_config_getb(handle, PCI_CONF_ILINE);
	irq = irq & 0xff;
	if (irq > 15 || irq == 0) {
		cmn_err(CE_NOTE, "DNET : IRQ out of range\n");
		pci_config_teardown(&handle);
		return (DDI_FAILURE);
	}

	/*
	 * Get the Master bit. This should have been set.
	 */
	csr = pci_config_getl(handle, PCI_CONF_COMM);
	if (!(csr & DNET_PCI_MASTER)) {
		cmn_err(CE_CONT, "DNET : Master bit not set. Setting it");
		pci_config_putl(handle, PCI_CONF_COMM, (csr | DNET_PCI_MASTER));
	}
	pci_config_teardown(&handle);

	/*
	 *	Allocate gld_mac_info_t and dnetinstance structures
	 */
	macinfo = (gld_mac_info_t *)kmem_zalloc(
			sizeof (gld_mac_info_t)+sizeof (struct dnetinstance),
			KM_NOSLEEP);

	if (macinfo == NULL) {
		return (DDI_FAILURE);
	}
	dnetp = (struct dnetinstance *)(macinfo+1);
	dnetp->devinfo = devinfo;

	dnetp->board_type = board_type;

	/*
	 * Initialize our private fields in macinfo and dnetinstance
	 */
	macinfo->gldm_private = (caddr_t)dnetp;
	macinfo->gldm_state = DNET_IDLE;
	macinfo->gldm_flags = 0;

	/*
	 * Initialize pointers to device specific functions which will be
	 * used by the generic layer.
	 */

	macinfo->gldm_reset	= dnet_reset;
	macinfo->gldm_start	= dnet_start_board;
	macinfo->gldm_stop	= dnet_stop_board;
	macinfo->gldm_saddr	= dnet_saddr;
	macinfo->gldm_sdmulti = dnet_dlsdmult;
	macinfo->gldm_prom	= dnet_prom;
	macinfo->gldm_gstat	= dnet_gstat;
	macinfo->gldm_send	= dnet_send;
	macinfo->gldm_intr	= dnetintr;
	macinfo->gldm_ioctl	= NULL;

	/*
	 *	Initialize board characteristics needed by the generic layer.
	 */

	macinfo->gldm_port = iobase;
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = DNETMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;
	macinfo->gldm_media = GLDM_UNKNOWN;

	if ((macinfo->gldm_irq_index = dnet_getIRQ_PCI(devinfo, irq)) == -1)
		return (DDI_FAILURE);

	if (dnet_setup_conf(macinfo, devinfo) == FAILURE) {
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t)+sizeof (struct dnetinstance));
		return (DDI_FAILURE);
	}

	/*
	 * Reset the chip
	 */
	outl(iobase + BUS_MODE_REG, SW_RESET);
	drv_usecwait(3);
	outl(iobase + BUS_MODE_REG, 0);
	drv_usecwait(8);

	/*
	 * Read the vendor address from ROM
	 */
	switch (dnetp->board_type) {
	case DEVICE_ID_21040:
		dnet_read21040addr(macinfo);
		break;
	case DEVICE_ID_21041:
		dnet_read21140srom(macinfo, vendor_info, 0, SROM_SIZE);
		/* SMC 8232 21041 macaddr is at byte offset 20 */
		bcopy((caddr_t)&vendor_info[20],
			(caddr_t)macinfo->gldm_vendor, ETHERADDRL);
		break;
	case DEVICE_ID_21140:
		dnet_read21140srom(macinfo, vendor_info, 0, SROM_SIZE);
		bcopy((caddr_t)vendor_info,
			(caddr_t)macinfo->gldm_vendor, ETHERADDRL);
		set_vendor_type(dnetp, vendor_info);
		break;
	}
	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);

	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);

#ifndef PCI_DDI_EMULATION
	/*
	 * XXX
	 * Set reg index to -1 so gld doesn't map_reg anythings for us.
	 */
	macinfo->gldm_reg_index = -1;
#endif

	/*
	 *	Register ourselves with the GLD interface
	 *
	 *	gld_register will:
	 *	link us with the GLD system;
	 *	set our ddi_set_driver_private(9F) data to the macinfo pointer;
	 *	save the devinfo pointer in macinfo->gldm_devinfo;
	 *	map the registers, putting the kvaddr into macinfo->gldm_memp;
	 *	add the interrupt, putting the cookie in gldm_cookie;
	 *	init the gldm_intrlock mutex which will block that interrupt;
	 *	create the minor node.
	 */

	if (gld_register(devinfo, "dnet", macinfo) == DDI_SUCCESS) {
		/*
		 * Do anything necessary to prepare the board for operation
		 * short of actually starting the board. We call gld_register()
		 * first in case hardware initialization requires memory mapping
		 * or is interrupt-driven. In this case we need to check for
		 * the return code of initialization.	If initialization fails,
		 * gld_unregister() should be called, data structures be freed,
		 * and DDI_FAILURE returned.
		 */
		int	rc;

		bzero((char *)dnetp->setup_buffer, SETUPBUF_SIZE);
		bzero((char *)dnetp->multicast_cnt, MCASTBUF_SIZE);
		rc = dnet_init_board(macinfo);
		/*
		 * release all the resources
		 */
		if (rc == FAILURE) {
			gld_unregister(macinfo);
			kmem_free(dnetp->base_mem, dnetp->base_memsize);
			kmem_free((caddr_t)macinfo, sizeof (gld_mac_info_t) +
				sizeof (struct dnetinstance));
			return (DDI_FAILURE);
		}
		(void) dnet_saddr(macinfo);
		return (DDI_SUCCESS);
	} else {
		kmem_free(dnetp->base_mem, dnetp->base_memsize);
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t)+sizeof (struct dnetinstance));
		return (DDI_FAILURE);
	}
}

/*
 * detach(9E) -- Detach a device from the system
 */
dnetdetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t	*macinfo;		/* GLD structure */
	struct dnetinstance *dnetp;		/* Our private device info */

#ifdef DNETDEBUG
	if (dnetdebug & DNETDDI)
		cmn_err(CE_CONT, "dnetdetach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);
	dnetp = (struct dnetinstance *)(macinfo->gldm_private);

	mutex_destroy(&dnetp->init_lock);

	/* stop the board if it is running */
	(void) dnet_stop_board(macinfo);

	/*
	 *	Unregister ourselves from the GLD interface
	 *
	 *	gld_unregister will:
	 *	remove the minor node;
	 *	unmap the registers;
	 *	remove the interrupt;
	 *	destroy the gldm_intrlock mutex;
	 *	unlink us from the GLD system.
	 */
	if (gld_unregister(macinfo) == DDI_SUCCESS) {
		kmem_free(dnetp->base_mem, dnetp->base_memsize);
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t)+sizeof (struct dnetinstance));
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}

/*
 *	GLD Entry Points
 */

/*
 *	dnet_reset() -- reset the board to initial state; restore the machine
 *	address afterwards.
 */
int
dnet_reset(gld_mac_info_t *macinfo)
{
	struct dnetinstance *dnetp =		/* Our private device info */
				(struct dnetinstance *)macinfo->gldm_private;
	u_long		iobase = macinfo->gldm_port;

#ifdef DNETDEBUG
	if (dnetdebug & DNETTRACE)
		cmn_err(CE_CONT, "dnet_reset(0x%x)", macinfo);
#endif

	(void) dnet_stop_board(macinfo);

	/*
	 * Reset the chip
	 */
	outl(iobase + BUS_MODE_REG, SW_RESET);
	drv_usecwait(3);
	outl(iobase + BUS_MODE_REG, 0);
	drv_usecwait(8);

	/*
	 * Initialize internal data structures
	 */
	bzero((char *)dnetp->setup_buffer, SETUPBUF_SIZE);
	bzero((char *)dnetp->multicast_cnt, MCASTBUF_SIZE);
	(void) dnet_init_board(macinfo);
	drv_usecwait(1000);	/* Some board needed this delay here */
	(void) dnet_saddr(macinfo);
	return (0);
}



/*
 * dnet_init_board() -- initialize the specified network board short of
 * actually starting the board.
 */
int
dnet_init_board(gld_mac_info_t *macinfo)
{
	struct dnetinstance *dnetp =		/* Our private device info */
				(struct dnetinstance *)macinfo->gldm_private;
	int		iobase = macinfo->gldm_port;
	u_long 		val;
	caddr_t 	mem_ptr;
	unsigned int	openmode_prop;
	int 		openmode_len;

#ifdef DNETDEBUG
	if (dnetdebug & DNETTRACE)
		cmn_err(CE_CONT, "dnet_init_board(0x%x)", macinfo);
#endif

	/*
	 * before initializing dnet should be in STOP state
	 */
	val = inl(iobase + OPN_MODE_REG);
	outl(iobase + OPN_MODE_REG, val & ~(START_TRANSMIT | START_RECEIVE));
	outl(iobase + STATUS_REG, CLEAR_INTR); 	/* Clear all interrupts */

	if (dnetp->board_type == DEVICE_ID_21140) {
		/* Check for custom "openmode_reg" property */
		openmode_len = sizeof (int);
		if (ddi_prop_op(DDI_DEV_T_ANY, dnetp->devinfo,
		    PROP_LEN_AND_VAL_BUF, DDI_PROP_DONTPASS, "openmode_reg",
		    (caddr_t)&openmode_prop, &openmode_len)
		    != DDI_PROP_SUCCESS)
			openmode_len = 0;

		if (openmode_len) {
			/* .conf file specified explicit value */
			if (openmode_prop & PORT_SELECT)
				outl(iobase + OPN_MODE_REG,
				    PORT_SELECT | OPN_REG_MB1);
			else
				outl(iobase + OPN_MODE_REG, OPN_REG_MB1);
		} else if (dnetp->vendor_21140 == ASANTE_TYPE)
			/* ASANTE seems to always use MII port */
			outl(iobase + OPN_MODE_REG, PORT_SELECT | OPN_REG_MB1);
		else if (dnetp->mode == DNET_100MBPS)
			/* Others seem to use SRL for 10, MII for 100 */
			outl(iobase + OPN_MODE_REG, PORT_SELECT | OPN_REG_MB1);
		else
			outl(iobase + OPN_MODE_REG, OPN_REG_MB1);

		/*
		 * Setting PORT_SELECT requires a software reset
		 */
		outl(iobase + BUS_MODE_REG, SW_RESET);
		drv_usecwait(3);
		outl(iobase + BUS_MODE_REG, 0);
		drv_usecwait(8);

		if (dnet_init21140csr12(macinfo) == FAILURE)
			return (FAILURE);

	}
	outl(iobase + BUS_MODE_REG, CACHE_ALIGN | BURST_SIZE); /* CSR0 */

	/*
	 * Initialize the TX and RX buffers
	 */
	mem_ptr = dnetp->base_mem;
	if ((u_long)dnetp->base_mem % 4)
		mem_ptr = (caddr_t)(((u_long)mem_ptr + 3) & 0xFFFFFFFC);

	if (dnet_init_txrx_bufs(macinfo, mem_ptr) == FAILURE)
		return (FAILURE);

	/*
	 * Set the base address of the Rx descrptor list in CSR4
	 */
	outl(iobase + RX_BASE_ADDR_REG, (ulong)dnetp->p_addr.rx_desc);

	/*
	 * Set the base address of the Tx descrptor list in CSR3
	 */
	outl(iobase + TX_BASE_ADDR_REG, (ulong)dnetp->p_addr.tx_desc);

	dnetp->tx_current_desc = dnetp->rx_current_desc = 0;
	dnetp->transmitted_desc = 0;
	dnetp->free_desc = MAX_TX_DESC;


	/*
	 * Set the interrupt masks in CSR7
	 */
	outl(iobase + INT_MASK_REG, NORMAL_INTR_MASK | ABNORMAL_INTR_MASK |
		TX_INTERRUPT_MASK | RX_INTERRUPT_MASK | SYSTEM_ERROR_MASK |
		TX_JABBER_MASK);

	/*
	 * 21040 SIA registers have to be initialized, not present in 21140
	 */
	if ((dnetp->board_type == DEVICE_ID_21040) ||
	    (dnetp->board_type == DEVICE_ID_21041)) {
		outl(iobase + SIA_CONNECT_REG, 0);
		drv_usecwait(1);
		outl(iobase + SIA_CONNECT_REG, SIA_CONNECT_MASK);
		outl(iobase + SIA_TXRX_REG, SIA_TXRX_MASK);
		outl(iobase + SIA_GENERAL_REG, SIA_GENERAL_MASK);
		/*
		 * Do media-detection only if the property "bncaui" is not
		 * present
		 */
		switch (dnetp->bnc_indicator) {
		case -1:
/* Disable completely broken autodetect code */
/*
			mutex_enter(&dnetp->init_lock);
			dnet_detectmedia(macinfo);
			mutex_exit(&dnetp->init_lock);
			break;
*/

		case 0 :
/*
			outl(iobase + SIA_GENERAL_REG, SIA_GENRL_MASK_TP);
*/
			outl(iobase + SIA_CONNECT_REG, AUTO_CONFIG);
			drv_usecwait(10000);	/* transceiver relay */
			break;

		case 1 :
/*
			outl(iobase + SIA_GENERAL_REG, SIA_GENRL_MASK_BNCAUI);
*/
			outl(iobase + SIA_CONNECT_REG, BNC_CONFIG);
			drv_usecwait(10000);	/* transceiver relay */
			break;
		}
	}

	if (dnetp->board_type == DEVICE_ID_21140) {
		val = inl(iobase + OPN_MODE_REG);

		if (openmode_len)
			/* .conf file specified explicit value */
			outl(iobase + OPN_MODE_REG, openmode_prop);
		else if (dnetp->vendor_21140 == ASANTE_TYPE)
			outl(iobase + OPN_MODE_REG, val |
			    PORT_SELECT | HEARTBEAT_DISABLE |
			    STORE_AND_FORWARD);
		else {
			if (dnetp->mode == DNET_100MBPS) {
				outl(iobase + OPN_MODE_REG, val |
				    PORT_SELECT | HEARTBEAT_DISABLE |
				    PCS_FUNCTION | SCRAMBLER_MODE |
				    STORE_AND_FORWARD);
			} else {
				outl(iobase + OPN_MODE_REG, (val &
				    ~(PORT_SELECT | HEARTBEAT_DISABLE)) |
				    TX_THRESHOLD_MODE);
			}
		}
	}

	/*
	 * Enable Fullduplex mode. Promiscuous mode is enabled after a reset...
	 * Turn it off
	 */
	val = inl(iobase + OPN_MODE_REG);
#ifdef FULLDUPLEX
	/*
	 * We turn on full duplex only if the property "fulldup=1" is present
	 */
	if (dnetp->full_duplex)
		outl(iobase + OPN_MODE_REG, val & (~PROM_MODE) | FULL_DUPLEX);
	else
		outl(iobase + OPN_MODE_REG, val & (~PROM_MODE));
#else
	/*
	 * As required we turn on the full duplex mode
	 */
	outl(iobase + OPN_MODE_REG, val & (~PROM_MODE) | FULL_DUPLEX);
#endif

	return (SUCCESS);
}


/*
 *	dnet_start_board() -- start the board receiving and allow transmits.
 */
dnet_start_board(gld_mac_info_t *macinfo)
{
	int		iobase = macinfo->gldm_port;
	u_long 		val;

#ifdef DNETDEBUG
	if (dnetdebug & DNETTRACE)
		cmn_err(CE_CONT, "dnet_start_board(0x%x)", macinfo);
#endif

	/*
	 * start the board and enable receiving
	 */
	val = inl(iobase + OPN_MODE_REG);
	outl(iobase + OPN_MODE_REG, val | START_TRANSMIT | START_RECEIVE);
	return (0);
}

/*
 *	dnet_stop_board() -- stop board receiving
 */
dnet_stop_board(gld_mac_info_t *macinfo)
{
	int 		iobase = macinfo->gldm_port;
	u_long 		val;

#ifdef DNETDEBUG
	if (dnetdebug & DNETTRACE)
		cmn_err(CE_CONT, "dnet_stop_board(0x%x)", macinfo);
#endif
	/*
	 * stop the board and disable transmit/receive
	 */
	val = inl(iobase + OPN_MODE_REG);
	outl(iobase + OPN_MODE_REG, val & ~(START_TRANSMIT | START_RECEIVE));
	return (0);
}

/*
 *	dnet_saddr() -- set the physical network address on the board
 */
int
dnet_saddr(gld_mac_info_t *macinfo)
{
	struct dnetinstance *dnetp =		/* Our private device info */
		(struct dnetinstance *)macinfo->gldm_private;
	struct tx_desc_type *desc;
	register 	current_desc;
	int 		iobase = macinfo->gldm_port;
	u_char		*virtual_address;
	int 		i;
	unsigned int	index;
	u_char 		m[7];
	int 		delay = 0xFF;
	u_long		*hashp;


#ifdef DNETDEBUG
	if (dnetdebug & DNETTRACE)
		cmn_err(CE_CONT, "dnet_saddr(0x%x)", macinfo);
#endif
	/* **** program current gldm_macaddr address into the hardware **** */
	current_desc = dnetp->tx_current_desc;
	desc = &dnetp->v_addr.tx_desc[current_desc];


	/*
	 * Wait till 21040 releases the next descriptor
	 */
	while (desc->desc0.own && --delay)
		drv_usecwait(10);

	if (!delay) {
		cmn_err(CE_CONT, "DNET :Cannot acquire descriptor\n");
		return (DDI_FAILURE);
	}

	*(u_long *)&desc->desc0 	= 0;			/* init descs */
	*(u_long *)&desc->desc1 	&= DNET_END_OF_RING;
	desc->desc1.buffer_size1 	= SETUPBUF_SIZE;
	desc->desc1.buffer_size2 	= 0;
	desc->desc1.setup_packet	= 1;
	desc->desc1.first_desc		= 0;
	desc->desc1.last_desc 		= 0;
	desc->desc1.filter_type0 	= 1;
	if ((dnetp->board_type == DEVICE_ID_21040) ||
	    (dnetp->board_type == DEVICE_ID_21041))
		desc->desc1.filter_type1 	= 0;
	else
		desc->desc1.filter_type1 	= 1;
	desc->desc1.int_on_comp		= 1;

	virtual_address = dnetp->tx_virtual_addr[current_desc];
	hashp = (u_long *)virtual_address;

	bcopy((caddr_t)dnetp->setup_buffer, (caddr_t)virtual_address,
		SETUPBUF_SIZE);
	virtual_address[156] = macinfo->gldm_macaddr[0];
	virtual_address[157] = macinfo->gldm_macaddr[1];
	virtual_address[160] = macinfo->gldm_macaddr[2];
	virtual_address[161] = macinfo->gldm_macaddr[3];
	virtual_address[164] = macinfo->gldm_macaddr[4];
	virtual_address[165] = macinfo->gldm_macaddr[5];
	index = hashindex((u_char *)macinfo->gldm_macaddr);
	hashp[ index / 16 ] |= 1 << (index % 16);

	/*
	 * As we are using Imperfect filtering, the broadcast address
	 * has to be set explicitly in the 512 bit hash table.
	 * Hence the index into the hash table is calculated and the bit set to
	 * enable reception of broadcast packets.
	 */
	for (i = 0; i < 6; i++)
		m[i] = 0xFF;
	index = hashindex(&m[0]);
	hashp[ index / 16 ] |= 1 << (index % 16);
	desc->desc0.own = 1; 				/* Ownership to chip */
	dnetp->tx_current_desc = ++dnetp->tx_current_desc % MAX_TX_DESC;
	bcopy((caddr_t)virtual_address, (caddr_t)dnetp->setup_buffer,
		SETUPBUF_SIZE);
	outb(iobase + TX_POLL_REG, TX_POLL_DEMAND);
	return (DDI_SUCCESS);
}

/*
 *	dnet_dlsdmult() -- set (enable) or disable a multicast address
 *
 *	Program the hardware to enable/disable the multicast address
 *	in "mcast".  Enable if "op" is non-zero, disable if zero.
 */
int
dnet_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct dnetinstance *dnetp =		/* Our private device info */
		(struct dnetinstance *)macinfo->gldm_private;
	struct 		tx_desc_type *desc;
	int 		current_desc;
	unsigned int	index;
	int 		iobase = macinfo->gldm_port;
	u_char 		*virtual_address;
	int 		delay = 0xFF;
	u_long		*hashp;

#ifdef DNETDEBUG
	if (dnetdebug & DNETTRACE)
		cmn_err(CE_CONT, "dnet_dlsdmult(0x%x, %s)", macinfo,
				op ? "ON" : "OFF");
#endif

	/*
	 * Enable or disable the multicast address in "mcast"
	 */
	index = hashindex((u_char *)mcast->ether_addr_octet);
	current_desc = dnetp->tx_current_desc;
	desc = &dnetp->v_addr.tx_desc[current_desc];

	/*
	 * Wait till the chip releases the * next descriptor
	 */
	while (desc->desc0.own && --delay)
		drv_usecwait(10);

	*(u_long *)&desc->desc0 	= 0;			/* init descs */
	*(u_long *)&desc->desc1 	&= DNET_END_OF_RING;
	desc->desc1.buffer_size1 	= SETUPBUF_SIZE;
	desc->desc1.buffer_size2	= 0;
	desc->desc1.setup_packet	= 1;
	desc->desc1.first_desc		= 0;
	desc->desc1.last_desc 		= 0;
	desc->desc1.filter_type0 	= 1;
	if ((dnetp->board_type == DEVICE_ID_21040) ||
	    (dnetp->board_type == DEVICE_ID_21041))
		desc->desc1.filter_type1 	= 0;
	else
		desc->desc1.filter_type1 	= 1;
	desc->desc1.int_on_comp		= 1;

	virtual_address = dnetp->tx_virtual_addr[current_desc];
	hashp = (u_long *)virtual_address;

	bcopy((caddr_t)dnetp->setup_buffer, (caddr_t)virtual_address,
		SETUPBUF_SIZE);

	if (op) {
		hashp[ index / 16 ] |= 1 << (index % 16);
		desc->desc0.own = 1;		 /* Ownership to chip */
		outb(iobase + TX_POLL_REG, TX_POLL_DEMAND);
		dnetp->tx_current_desc = ++dnetp->tx_current_desc % MAX_TX_DESC;

		bcopy((caddr_t)virtual_address,
			(caddr_t)dnetp->setup_buffer, SETUPBUF_SIZE);
		dnetp->multicast_cnt[index]++;
	} else {
		if (--dnetp->multicast_cnt[index] == 0) {
			hashp[ index / 16 ] &= (~ (1 << (index % 16)));
			desc->desc0.own = 1;
			outb(iobase + TX_POLL_REG, TX_POLL_DEMAND);
			dnetp->tx_current_desc =
				++dnetp->tx_current_desc % MAX_TX_DESC;
			bcopy((caddr_t)virtual_address,
				(caddr_t)dnetp->setup_buffer, SETUPBUF_SIZE);
		}
	}
	return (DDI_SUCCESS);
}

/*
 * dnet_prom() -- set or reset promiscuous mode on the board
 *
 *	Program the hardware to enable/disable promiscuous mode.
 *	Enable if "on" is non-zero, disable if zero.
 */

int
dnet_prom(gld_mac_info_t *macinfo, int on)
{
	int		iobase = macinfo->gldm_port;
	u_long 		val;

#ifdef DNETDEBUG
	if (dnetdebug & DNETTRACE)
		cmn_err(CE_CONT, "dnet_prom(0x%x, %s)", macinfo,
				on ? "ON" : "OFF");
#endif

	/*
	 * enable or disable promiscuous mode
	 */
	(void) dnet_stop_board(macinfo);
	val = inl(iobase + OPN_MODE_REG);
	if (on)
		outl(iobase + OPN_MODE_REG, val | PROM_MODE);
	else
		outl(iobase + OPN_MODE_REG, val & (~PROM_MODE));
	(void) dnet_start_board(macinfo);
	return (DDI_SUCCESS);
}

/*
 * dnet_gstat() -- update statistics
 *
 *	GLD calls this routine just before it reads the driver's statistics
 *	structure.  If your board maintains statistics, this is the time to
 *	read them in and update the values in the structure.  If the driver
 *	maintains statistics continuously, this routine need do nothing.
 */

int
dnet_gstat(gld_mac_info_t *macinfo)
{
#ifdef DNETDEBUG
	if (dnetdebug & DNETTRACE)
		cmn_err(CE_CONT, "dnet_gstat(0x%x)", macinfo);
#endif

	/* **** update statistics from board if necessary **** */

	return (DDI_SUCCESS);
}

/*
 *	dnet_send() -- send a packet
 *
 *	Called when a packet is ready to be transmitted. A pointer to an
 *	M_DATA message that contains the packet is passed to this routine.
 *	The complete LLC header is contained in the message's first message
 *	block, and the remainder of the packet is contained within
 *	additional M_DATA message blocks linked to the first message block.
 *
 *	This routine may NOT free the packet.
 */

#define	NextTXIndex(index)	(((index)+1)%MAX_TX_DESC)
#define	PrevTXIndex(index)	(((index)-1) < 0 ?MAX_TX_DESC-1:(index)-1)

int
dnet_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	struct dnetinstance *dnetp =		/* Our private device info */
			(struct dnetinstance *)macinfo->gldm_private;
	register struct tx_desc_type	*ring = dnetp->v_addr.tx_desc;
	int	iobase = macinfo->gldm_port;
	int	mblen, seglen, bclen, totlen;
	int	index, end_index, start_index;
	int	avail;
	char	*segptr;

#ifdef DNETDEBUG
	if (dnetdebug & DNETSEND)
		cmn_err(CE_CONT, "dnet_send(0x%x, 0x%x)", macinfo, mp);
#endif

	/*
	 * Make sure we have room
	 */
	avail = dnetp->free_desc;
	ASSERT(avail >= 0);
	start_index = dnetp->tx_current_desc;

	/* temporary use of totlen here */
	totlen = msgdsize(mp);
	ASSERT(totlen <= ETHERMAX);
	if (totlen > (avail*TX_BUF_SIZE)) {
		macinfo->gldm_stats.glds_defer++;
		return (1);
	}

	/*
	 * Copy the mbufs into txbufs
	 */
	totlen = seglen = 0;
	do {
		if (seglen <= 0) {
			dnetp->free_desc--;
			ASSERT(dnetp->free_desc >= 0);
			end_index = dnetp->tx_current_desc;
			/*
			 * Check for TX out of space. This shouldn't
			 * ever happen. NEEDSWORK: completely drop packet?
			 */
			if (ring[end_index].desc0.own) {
				dnetp->tx_current_desc = start_index;
				dnetp->free_desc = avail;
				return (0);
			}
			segptr = (char *)dnetp->tx_virtual_addr[end_index];
			seglen = TX_BUF_SIZE;
			*(u_long *)&ring[end_index].desc0 = 0;	/* init descs */
			*(u_long *)&ring[end_index].desc1 &= DNET_END_OF_RING;
			dnetp->tx_current_desc = NextTXIndex(end_index);
		}
		mblen = (int)(mp->b_wptr - mp->b_rptr);
		bclen = (mblen > seglen) ? seglen : mblen;
		bcopy((char *)mp->b_rptr, segptr, bclen);
		seglen -= bclen;
		segptr += bclen;
		totlen += bclen;
		mp->b_rptr += bclen;
		ring[end_index].desc1.buffer_size1 += bclen;
		ASSERT(ring[end_index].desc1.buffer_size1 <= TX_BUF_SIZE);
		if (mblen <= bclen)
			mp = mp->b_cont;
	} while (mp != NULL);

	/*
	 * Now set the first/last buffer and own bits
	 * Since the 21040 looks for these bits set in the
	 * first buffer, work backwards in multiple buffers.
	 */
	if (start_index == end_index) {
		if (totlen < ETHERMIN)
			ring[end_index].desc1.buffer_size1 = ETHERMIN;
		ring[end_index].desc1.first_desc = 1;
		ring[end_index].desc1.last_desc = 1;
		ring[end_index].desc1.int_on_comp = 1;
		ring[end_index].desc0.own = 1;
	} else {
		index = end_index;
		while (index != start_index) {
			if (index == end_index) {
				ring[index].desc1.last_desc = 1;
				ring[index].desc1.int_on_comp = 1;
				ring[index].desc0.own = 1;
			} else {
				ring[index].desc0.own = 1;
			}
			index = PrevTXIndex(index);
		}
		ring[index].desc1.first_desc = 1;
		ring[index].desc0.own = 1;
	}

	/*
	 * Safety check: make sure end-o-ring is set in last desc.
	 */
	ASSERT(ring[MAX_TX_DESC-1].desc1.end_of_ring != 0);

	/*
	 * Kick the transmitter
	 */
	outl(iobase+TX_POLL_REG, TX_POLL_DEMAND);

	return (0);		/* successful transmit attempt */
}

/*
 *	dnetintr() -- interrupt from board to inform us that a receive or
 *	transmit has completed.
 */
u_int
dnetintr(gld_mac_info_t *macinfo)
{
	struct dnetinstance *dnetp =	/* Our private device info */
				(struct dnetinstance *)macinfo->gldm_private;
	register u_long		int_status;
	register int 		iobase = macinfo->gldm_port;

#ifdef DNETDEBUG
	if (dnetdebug & DNETINT)
		cmn_err(CE_CONT, "linkintr(0x%x)\n", macinfo);
#endif

	int_status = inl(iobase + STATUS_REG);

	/*
	 * If interrupt was not from this board
	 */
	if (!(int_status & (NORMAL_INTR_SUMM | ABNORMAL_INTR_SUMM))) {
		return (DDI_INTR_UNCLAIMED);
	}
	outl(iobase + INT_MASK_REG, 0);			 /* Disable all ints */

	macinfo->gldm_stats.glds_intr++;

	while (int_status & (NORMAL_INTR_SUMM | ABNORMAL_INTR_SUMM)) {
		/*
		 * Check if Link is down
		 */

		if (int_status & LINK_INTR) {
			outl(iobase + STATUS_REG, LINK_INTR);	/* Clear intr */
			cmn_err(CE_CONT, "DNET : Link Gone Down... \n");
			int_status = inl(iobase + STATUS_REG);
			continue;
		}

		/*
		 * Check for system error
		 */
		if (int_status & SYS_ERR) {
			outl(iobase + STATUS_REG, SYS_ERR);	/* Clear intr */
			cmn_err(CE_CONT, "\nDNET : System Error.  "
			    "Trying to reset.. \n");
			if (int_status & MASTER_ABORT)
				cmn_err(CE_CONT, "DNET : Bus Master\
					Abort\n");
			if (int_status & TARGET_ABORT)
				cmn_err(CE_CONT, "DNET: Bus Target\
					Abort\n");
			if (int_status & PARITY_ERROR) {
				cmn_err(CE_CONT,
					"DNET: Parity error\n");
			}
			(void) dnet_reset(macinfo);
			(void) dnet_start_board(macinfo);
			int_status = inl(iobase + STATUS_REG);
			continue;
		}

		/*
		 * If the jabber has timedout the reset the chip
		 */
		if (int_status & TX_JABBER_TIMEOUT) {
			outl(iobase + STATUS_REG, TX_JABBER_TIMEOUT);
			cmn_err(CE_CONT, "\nDNET:Jabber timeout. \
				Trying to reset...\n");
			(void) dnet_reset(macinfo);
			(void) dnet_start_board(macinfo);
			int_status = inl(iobase + STATUS_REG);
			continue;
		}

		/*
		 * If an underflow has occurred, reset the chip
		 */
		if (int_status & TX_UNDERFLOW) {
			outl(iobase + STATUS_REG, TX_UNDERFLOW);
			(void) dnet_reset(macinfo);
			(void) dnet_start_board(macinfo);
			int_status = inl(iobase + STATUS_REG);
			continue;
		}

		/*
		 * Check if receive interrupt bit is set
		 */
		if (int_status & RX_INTR) {
			outl(iobase + STATUS_REG, RX_INTR);
			dnet_getp(macinfo);
		}

		if (int_status & TX_INTR) {
			outl(iobase + STATUS_REG, TX_INTR);
			dnet_ProcessXmitIntr(macinfo, dnetp);
		}

		int_status = inl(iobase + STATUS_REG);
	}

	/*
	 * Enable the interrupts
	 */
	outl(iobase + INT_MASK_REG, NORMAL_INTR_MASK | ABNORMAL_INTR_MASK |
		TX_INTERRUPT_MASK | RX_INTERRUPT_MASK | SYSTEM_ERROR_MASK |
		LINK_INTR_MASK | TX_JABBER_MASK);

	return (DDI_INTR_CLAIMED);	/* Indicate it was our interrupt */
}


/*
 *
 */
static void
dnet_ProcessXmitIntr(gld_mac_info_t *macinfo, struct dnetinstance *dnetp)
{
	struct tx_desc_type	*desc = dnetp->v_addr.tx_desc;
	int index;

	index = dnetp->transmitted_desc;
	while (((dnetp->free_desc == 0) || (index != dnetp->tx_current_desc)) &&
		!(desc[index].desc0.own)) {

		/*
		 * Setup packets are ugly; they don't
		 * adjust free_desc, but they shouldn't
		 * actually compete for descriptors like sends do
		 * Ignore the setup complete interrupt; maybe
		 * this NEEDSWORK to deal with setup errors?
		 */
		if (desc[index].desc1.setup_packet)
			goto next;

		/*
		 * Check for Tx Error that gets set
		 * in the last desc.
		 */
		if ((desc[index].desc1.last_desc) &&
		    (desc[index].desc0.err_summary))
			update_tx_stats(macinfo, index);

		dnetp->free_desc++;
next:
		index = (index+1) % MAX_TX_DESC;
	}

	dnetp->transmitted_desc = index;
}


/*
 * finds out which physical memory to be mapped to get the proper mapping
 * and which irq to configure by reading from the .conf file.
 */
static int
dnet_setup_conf(gld_mac_info_t *macinfo, dev_info_t *devinfo)
{
	struct	dnetinstance	*dnetp =
			(struct dnetinstance *)macinfo->gldm_private;
	int 		*mode_prop, mode_len;
	int 		*bnc_prop, bnc_len;

#ifdef FULLDUPLEX
	int 		*fulldup_prop, fulldup_len;

	dnetp->full_duplex = 0;
	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
			"fulldup", (caddr_t)&fulldup_prop, &fulldup_len) ==
				DDI_PROP_SUCCESS) {
		if (fulldup_prop[0] == 1)
			dnetp->full_duplex = 1;
		kmem_free(fulldup_prop, fulldup_len);
	}
#endif

	if ((dnetp->board_type == DEVICE_ID_21040) ||
	    (dnetp->board_type == DEVICE_ID_21041))
		dnetp->max_rx_desc = MAX_RX_DESC_21040;
	else
		dnetp->max_rx_desc = MAX_RX_DESC_21140;
	if (dnet_alloc_bufs(macinfo) == FAILURE) {
		return (FAILURE);
	}

	/*
	 * Get the BNC/TP indicator from the conf file for 21040
	 */
	if ((dnetp->board_type == DEVICE_ID_21040) ||
	    (dnetp->board_type == DEVICE_ID_21041)) {
		dnetp->bnc_indicator = -1;
		if ((ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
					"bncaui", (caddr_t)&bnc_prop,
					&bnc_len)) == DDI_PROP_SUCCESS) {
			if (bnc_prop[0] == 1)
				dnetp->bnc_indicator = 1;
			else
				dnetp->bnc_indicator = 0;
			kmem_free(bnc_prop, bnc_len);
		}
		else
			mutex_init(&dnetp->init_lock,
			    "DNET 21040/21041 init lock", MUTEX_DRIVER, NULL);
	}

	/*
	 * For 21140 check the date rate set in the conf file.Default is 100Mb/s
	 */
	if (dnetp->board_type == DEVICE_ID_21140) {
		dnetp->mode = DNET_100MBPS;
		if ((ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
					"mode", (caddr_t)&mode_prop,
					&mode_len)) == DDI_PROP_SUCCESS) {
			if (mode_prop[0] == 10) {
				dnetp->mode = DNET_10MBPS;
			}
			kmem_free(mode_prop, mode_len);
		}
	}
	return (SUCCESS);
}

static void
dnet_getp(gld_mac_info_t *macinfo)
{
	struct dnetinstance *dnetp =
			(struct dnetinstance *)macinfo->gldm_private;
	register int packet_length, index, count;
	mblk_t	*mp;
	u_char 	*virtual_address;
	struct	rx_desc_type *desc = dnetp->v_addr.rx_desc;
	int	iobase = macinfo->gldm_port;

#ifdef DNETDEBUG
	if (dnetdebug & DNETRECV)
		cmn_err(CE_CONT, "dnet_getp(0x%x)\n", macinfo);
#endif

	/* While host owns the current descriptor */
	while (!(desc[dnetp->rx_current_desc].desc0.own)) {

		/* First segment bit not set in current descriptor */
		if (!desc[dnetp->rx_current_desc].desc0.first_desc)
			return;

		/*
		 * Scan the desc. list starting from First Segment desc. to
		 * Last Segment desc. limited to the entire list scanned once
		 */

		index = dnetp->rx_current_desc;
		for (count = 0; count < dnetp->max_rx_desc; count++) {
			if (desc[index].desc0.own) { /* LS not received yet */
				return;
			}
			if (desc[index].desc0.last_desc)
				break;
			index = ++index % dnetp->max_rx_desc;
		}

		packet_length = desc[index].desc0.frame_len;

		/* Error bit set in last descriptor */
		/* allocate a message block to hold the packet  */
		mp = (mblk_t *)-1;
		if (desc[index].desc0.err_summary ||
		    (mp = allocb(packet_length, 0)) == NULL) {

			/* Update gld statistics */
			if (desc[index].desc0.err_summary)
				update_rx_stats(macinfo, index);

			/*
			 * Reset ownership of all descriptors
			 */
			for (; !desc[dnetp->rx_current_desc].desc0.last_desc;
			    dnetp->rx_current_desc =
				(++dnetp->rx_current_desc) % dnetp->max_rx_desc)
				desc[dnetp->rx_current_desc].desc0.own = 1;

			/* For last descriptor */
			desc[dnetp->rx_current_desc].desc0.own = 1;
			dnetp->rx_current_desc =
			    ++dnetp->rx_current_desc % dnetp->max_rx_desc;

			/* Demand receive polling by the chip */
			outl(iobase + RX_POLL_REG, RX_POLL_DEMAND);

			if (!mp) {
				macinfo->gldm_stats.glds_norcvbuf++;
				return;
			}
			continue;
		}

		/* get the virtual address of the packet received */
		virtual_address =
		    dnetp->rx_virtual_addr[dnetp->rx_current_desc];

		/*
		 * copy data from First desc buffer onwards to Last Segment
		 * desc. buffer into the message block
		 */
		while (!desc[dnetp->rx_current_desc].desc0.last_desc) {
			/* receive the packet  */
			bcopy((caddr_t)virtual_address, (caddr_t)mp->b_wptr,
			    RX_BUF_SIZE);
			mp->b_wptr += RX_BUF_SIZE;

			packet_length -= RX_BUF_SIZE;	/* For last desc. len */
			/* Change ownership of the desc. to the chip. */
			desc[dnetp->rx_current_desc].desc0.own =  1;

			dnetp->rx_current_desc =
			    ++dnetp->rx_current_desc % dnetp->max_rx_desc;
			virtual_address =
			    dnetp->rx_virtual_addr[dnetp->rx_current_desc];
		}
		/* For the last descriptor, do the needful */
		bcopy((caddr_t)virtual_address, (caddr_t)mp->b_wptr,
			packet_length);
		mp->b_wptr += packet_length;
		desc[dnetp->rx_current_desc].desc0.own = 1;

		/* Demand polling by chip */
		outl(iobase + RX_POLL_REG, RX_POLL_DEMAND);

		/* send the packet upstream */
		gld_recv(macinfo, mp);

		/*
		 * Increment receive desc index. This is for the next scan of
		 * next FS to LS packet
		 */
		dnetp->rx_current_desc =
			(++dnetp->rx_current_desc) % dnetp->max_rx_desc;
	}
}

/*
 * Function to update receive statistics
 */
static void
update_rx_stats(register gld_mac_info_t *macinfo, int index)
{
	struct	dnetinstance	*dnetp =
			(struct dnetinstance *)macinfo->gldm_private;
	register struct rx_desc_type *descp = &(dnetp->v_addr.rx_desc[index]);
	u_long 		val;
	int 		iobase = macinfo->gldm_port;

	/*
	 * Update gld statistics
	 */
	macinfo->gldm_stats.glds_errrcv ++;

	if (descp->desc0.overflow)	{
		descp->desc0.overflow = 0;
		macinfo->gldm_stats.glds_overflow ++;
	}

	if (descp->desc0.crc) {
		descp->desc0.crc = 0;
		macinfo->gldm_stats.glds_crc ++;
	}

	if (descp->desc0.runt_frame) {
		descp->desc0.runt_frame = 0;
		macinfo->gldm_stats.glds_frame ++;
	}

	if ((descp->desc0.length_err) || (descp->desc0.frame2long)) {
		descp->desc0.length_err = 0;
		descp->desc0.frame2long = 0;
		macinfo->gldm_stats.glds_frame ++;
	}

	val = inl(iobase + MISSED_FRAME_REG);
	macinfo->gldm_stats.glds_missed += (val & MISSED_FRAME_MASK);
	outl(iobase + MISSED_FRAME_REG, 0);
	descp->desc0.err_summary = 0;
}

/*
 * Function to update transmit statistics
 */
static void
update_tx_stats(gld_mac_info_t *macinfo, int index)
{
	struct	dnetinstance	*dnetp =
			(struct dnetinstance *)macinfo->gldm_private;
	register struct tx_desc_type *descp = &(dnetp->v_addr.tx_desc[index]);


	/* Update gld statistics */
	macinfo->gldm_stats.glds_errxmt ++;

	if (descp->desc0.collision_count)	{
		macinfo->gldm_stats.glds_collisions +=
			descp->desc0.collision_count;
		descp->desc0.collision_count = 0;
	}

	if (descp->desc0.late_collision) {
		macinfo->gldm_stats.glds_xmtlatecoll ++;
		descp->desc0.late_collision = 0;
	}

	if (descp->desc0.excess_collision) {
		macinfo->gldm_stats.glds_excoll ++;
		descp->desc0.excess_collision = 0;
	}

	if (descp->desc0.underflow) {
		macinfo->gldm_stats.glds_underflow ++;
		descp->desc0.underflow = 0;
	}

	if (descp->desc0.no_carrier) {
		macinfo->gldm_stats.glds_nocarrier ++;
		descp->desc0.no_carrier = 0;
	}
	descp->desc0.err_summary = 0;
}

/*
 * A hashing function used for setting the
 * node address or a multicast address
 */
static unsigned
hashindex(u_char *address)
{
	unsigned long	crc = (unsigned long)HASH_CRC;
	unsigned long	const POLY = HASH_POLY;
	unsigned long	msb;
	int 		byteslength = 6;
	unsigned char 	currentbyte;
	unsigned 	index;
	int 		bit;
	int 		shift;

	for (byteslength = 0; byteslength < 6; byteslength++) {
		currentbyte = address[byteslength];

		for (bit = 0; bit < 8; bit++) {
			msb = crc >> 31;
			crc <<= 1;

			if (msb ^ (currentbyte & 1)) {
				crc ^= POLY;
				crc |= 0x00000001;
			}
			currentbyte >>= 1;
		}
	}

	for (index = 0, bit = 23, shift = 8;
		shift >= 0;
		bit++, shift--) {
			index |= (((crc >> bit) & 1) << shift);
	}
	return (index);
}

/*
 * The function reads the ethernet address of the 21040 adapter
 */
static void
dnet_read21040addr(gld_mac_info_t *macinfo)
{
	u_long		val;
	int		iobase;
	int		i;

	iobase = macinfo->gldm_port;
	outl(iobase + ETHER_ROM_REG, 0);		/*	reset pointer */
	for (i = 0; i < ETHERADDRL; i ++) {

		val = inl(iobase + ETHER_ROM_REG);
		while ((val >> 31))	/* Is data read complete */
			val = inl(iobase + ETHER_ROM_REG);
		macinfo->gldm_vendor[i] = val & 0xFF;
	}
}

/*
 * The function reads the SROM	of the 21140 adapter
 */
static void
dnet_read21140srom(gld_mac_info_t *macinfo, char *addr, int offset, int len)
{
	int		iobase;
	int 		i, j;
	unsigned char	rom_addr;
	unsigned char	bit;
	unsigned long	dout;
	unsigned short	word;

	iobase = macinfo->gldm_port;
	rom_addr = (unsigned char)offset;
	i = inl(iobase + ETHER_ROM_REG);
	outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM);
	for (i = 0; i < SROM_MAX_CYCLES; i++) {
		outl(iobase + ETHER_ROM_REG,
			READ_OP | SEL_ROM | SEL_CHIP | SEL_CLK);
		drv_usecwait(1);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP);
		drv_usecwait(1);

	}
	for (i = 0; i <	len; i += 2) {
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP);
		outl(iobase + ETHER_ROM_REG,
			READ_OP | SEL_ROM | SEL_CHIP | SEL_CLK);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP);

		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP |
			DATA_IN);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP |
			DATA_IN | SEL_CLK);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP |
			DATA_IN);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP |
			DATA_IN | SEL_CLK);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP |
			DATA_IN);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP);
		outl(iobase + ETHER_ROM_REG,
			READ_OP | SEL_ROM | SEL_CHIP | SEL_CLK);
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM | SEL_CHIP);

		for (j = 0; j <= LAST_ADDRESS_BIT; j++) {
			bit = ((rom_addr << (j + 2) & 0x80)) ? 1 : 0;
			bit = bit << 2;
			outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM |
				SEL_CHIP | bit);
			outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM |
				SEL_CHIP | bit | SEL_CLK);
			outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM |
				SEL_CHIP | bit);
			drv_usecwait(1);
		}
		drv_usecwait(1);
		dout = inl(iobase + ETHER_ROM_REG);
		dout = (dout >> 3) & 1;
		if (dout != 0)
			return;
		word = 0;
		for (j = 0; j <= 15; j++) {
			outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM |
				SEL_CHIP | SEL_CLK);
			dout = inl(iobase + ETHER_ROM_REG);
			word |= ((dout >> 3) & 1) << (15 - j);
			outl(iobase + ETHER_ROM_REG,
				READ_OP | SEL_ROM | SEL_CHIP);
			drv_usecwait(1);
		}
		addr[i] = (word & 0x0000FF);
		addr[i + 1] = (word >> 8);
		rom_addr++;
		outl(iobase + ETHER_ROM_REG, READ_OP | SEL_ROM);
		drv_usecwait(1);
	}
}

/*
 * This function allocates the receive and transmit buffers and descriptors
 */
static int
dnet_alloc_bufs(gld_mac_info_t *macinfo)
{
	struct dnetinstance	*dnetp = (struct dnetinstance *)\
					(macinfo->gldm_private);

	unsigned long 	pages_needed;
	unsigned long	page_size;
	int		i;
	int 		len;
	int		ram_size;

	page_size = ddi_ptob(dnetp->devinfo, 1);

	ram_size = (MAX_TX_DESC * TX_BUF_SIZE) +
			(MAX_TX_DESC * sizeof (struct tx_desc_type)) +
			(dnetp->max_rx_desc * RX_BUF_SIZE) +
			(dnetp->max_rx_desc * sizeof (struct rx_desc_type));
	pages_needed = ddi_btopr(dnetp->devinfo, ram_size);
	dnetp->base_memsize = (pages_needed * page_size);

	/*
	 * We need some extra memory as some memory near page boundaries will
	 * not be used
	 */
/*
	if (dnetp->board_type == DEVICE_ID_21040)
		dnetp->base_memsize += (page_size / 2);
	else
		dnetp->base_memsize += (page_size * 2);
*/
	dnetp->base_memsize *= 8;

	if ((dnetp->base_mem = kmem_zalloc(dnetp->base_memsize,
							KM_NOSLEEP)) == NULL) {
		return (FAILURE);
	}
	for (i = page_size, len = 0; i > 1; len++)
		i >>= 1;
	dnetp->pgmask = page_size - 1;
	dnetp->pgshft = len;
	return (SUCCESS);
}

/*
 * Function to initialize all the descriptors
 * and buffers for transmit and receive
 */
static int
dnet_init_txrx_bufs(gld_mac_info_t *macinfo, caddr_t v_addr)
{
	struct dnetinstance	*dnetp = (struct dnetinstance *)\
					(macinfo->gldm_private);

	int 		page_size = ddi_ptob(dnetp->devinfo, 1);
	int 		cur_page;
	paddr_t		p_addr;
	int		i;
	int		incr;
	int		desclist_size;

	/*
	 * Check if we can fit the Tx descriptor list in the current page
	 */
	cur_page = hat_getkpfnum(v_addr);
	desclist_size = (MAX_TX_DESC * sizeof (struct tx_desc_type));
	if (hat_getkpfnum(v_addr + desclist_size) != cur_page) {
		v_addr = (v_addr + (page_size - ((ulong)v_addr % page_size)));
		cur_page = hat_getkpfnum(v_addr);
	}
	p_addr = DNET_KVTOP(v_addr);

	/*
	 * Set base address of the transmit descriptor list
	 */
	dnetp->v_addr.tx_desc = (struct tx_desc_type *)v_addr;
	dnetp->p_addr.tx_desc = (struct tx_desc_type *)p_addr;
	incr = desclist_size;

	/*
	 * Initilize all the Tx buffers and descriptors
	 */
	i = 0;
	do {
		if (hat_getkpfnum(v_addr + incr + TX_BUF_SIZE) != cur_page) {
			v_addr = (v_addr + (page_size -
					((ulong)v_addr % page_size)));
			cur_page = hat_getkpfnum(v_addr);
			p_addr = DNET_KVTOP(v_addr);
		} else {
			v_addr += incr;
			p_addr += incr;
		}

		dnetp->v_addr.tx_desc[i].buffer1 = (u_char *)p_addr;
		dnetp->v_addr.tx_desc[i].desc0.own = 0;
		dnetp->v_addr.tx_desc[i].desc1.setup_packet = 0;
		dnetp->v_addr.tx_desc[i].desc1.first_desc = 0;
		dnetp->v_addr.tx_desc[i].desc1.last_desc = 0;
		dnetp->v_addr.tx_desc[i].desc1.filter_type0 = 0;
		dnetp->v_addr.tx_desc[i].desc1.filter_type1 = 0;
		dnetp->v_addr.tx_desc[i].desc1.buffer_size2 = 0;
		dnetp->tx_virtual_addr[i] = (u_char *)v_addr;

		incr = TX_BUF_SIZE;
		i++;
	} while (i < MAX_TX_DESC);
	dnetp->v_addr.tx_desc[MAX_TX_DESC - 1].desc1.end_of_ring = 1;

	/*
	 * Check if we have room to fit the Rx descriptor list
	 */
	desclist_size = (dnetp->max_rx_desc * sizeof (struct rx_desc_type));
	if (hat_getkpfnum(v_addr + incr + desclist_size) != cur_page) {
		v_addr = (v_addr + (page_size - ((ulong)v_addr % page_size)));
		cur_page = hat_getkpfnum(v_addr);
		p_addr = DNET_KVTOP(v_addr);
	} else {
		p_addr += incr;
		v_addr += incr;
	}
	dnetp->v_addr.rx_desc = (struct rx_desc_type *)v_addr;
	dnetp->p_addr.rx_desc = (struct rx_desc_type *)p_addr;
	incr = desclist_size;

	/*
	 * Initialize the Rx buffers and descriptors
	 */
	i = 0;
	do {
		if (hat_getkpfnum(v_addr + incr + RX_BUF_SIZE) != cur_page) {
			v_addr = (v_addr + (page_size -
					((ulong)v_addr % page_size)));
			cur_page = hat_getkpfnum(v_addr);
			p_addr = DNET_KVTOP(v_addr);
		} else {
			v_addr += incr;
			p_addr += incr;
		}

		dnetp->v_addr.rx_desc[i].buffer1 = (u_char *)p_addr;
		dnetp->v_addr.rx_desc[i].desc0.own = 1;
		dnetp->v_addr.rx_desc[i].desc1.buffer_size1 = RX_BUF_SIZE;
		dnetp->v_addr.rx_desc[i].desc1.buffer_size2 = 0;
		dnetp->rx_virtual_addr[i] = (u_char *)v_addr;

		incr = RX_BUF_SIZE;
		i++;
	} while (i < dnetp->max_rx_desc);
	dnetp->v_addr.rx_desc[dnetp->max_rx_desc - 1].desc1.end_of_ring = 1;

	return (0);
}

/*
 * Vendor specific initializing of 21140 CSR12
 */
static int
dnet_init21140csr12(gld_mac_info_t *macinfo)
{
	struct dnetinstance	*dnetp = (struct dnetinstance *)\
					(macinfo->gldm_private);

	int 		iobase = macinfo->gldm_port;
	int 		genreg_len, nregs, i;
	unsigned int	*genreg_prop;

	if (ddi_getlongprop(DDI_DEV_T_ANY, dnetp->devinfo, DDI_PROP_DONTPASS,
			"general_reg", (caddr_t)&genreg_prop, &genreg_len) ==
				DDI_PROP_SUCCESS) {
		cmn_err(CE_CONT, "!DNET: Custom 21140");
		nregs = genreg_len / sizeof (*genreg_prop);
		for (i = 0; i < nregs; i++) {
			outl(iobase + SIA_STATUS_REG,
					genreg_prop[i] & 0xFFFF);
			if (genreg_prop[i]>>16)
				drv_usecwait(genreg_prop[i]>>16);
		}
		kmem_free(genreg_prop, genreg_len);
		return (SUCCESS);
	}

	switch (dnetp->vendor_21140) {

		case ASANTE_TYPE:
			cmn_err(CE_CONT, "!DNET: Asante 21140");
			/* select pin 7 as output */
			outl(iobase + SIA_STATUS_REG,
					CSR12_M_CONTROL | 0x80);
			outl(iobase + SIA_STATUS_REG,
					CSR12_M_ASANTE_PHY_RESET_1);
			drv_usecwait(1);
			outl(iobase + SIA_STATUS_REG,
					CSR12_M_ASANTE_DEFAULT);
			return (SUCCESS);

		default:
			cmn_err(CE_CONT, "!DNET: Generic 21140");
			return (SUCCESS);

		case COGENT_EM100_TYPE:
			cmn_err(CE_CONT, "!DNET: Cogent EM100 21140");
			/*
			 * Cogent EM100 does not support 10Mb/s, so we
			 * set the mode to 100Mb/s
			 */
			dnetp->mode = DNET_100MBPS;
			outl(iobase + SIA_STATUS_REG, CSR12_K_PIN_SELECT);
			outl(iobase + SIA_STATUS_REG, CSR12_K_RESET);
#ifdef FULLDUPLEX
			if (dnetp->full_duplex)
				outl(iobase + SIA_STATUS_REG, CSR12_K_100TX);
			else
				outl(iobase + SIA_STATUS_REG, CSR12_K_100TX |
					CSR12_M_10T_OPN_MODE);
#else
			outl(iobase + SIA_STATUS_REG, CSR12_K_100TX);

#endif
			return (SUCCESS);
	}
}


static int
dnet_getIRQ_PCI(dev_info_t *devinfo, int irq)
{
	int intarr[3];


	intarr[0] = 2;
	intarr[1] = PRIORITY_LEVEL;
	intarr[2] = irq;

	if (ddi_ctlops(devinfo, devinfo, DDI_CTLOPS_XLATE_INTRS,
			(caddr_t)intarr, ddi_get_parent_data(devinfo)) !=
			DDI_SUCCESS)
		return (-1);

	return (0);
}

/*
 * function to detect media
 */
static void
dnet_detectmedia(gld_mac_info_t *macinfo)
{
	int		iobase = macinfo->gldm_port;
	register struct dnetinstance *dnetp = (struct dnetinstance *)
						macinfo->gldm_private;
	struct tx_desc_type *desc = dnetp->v_addr.tx_desc;
	int		delay = 0xFF;

	/*
	 * Try 10-BaseT. Chip will not release if descriptor if there is
	 * no media
	 */
	outl(iobase + SIA_CONNECT_REG, 0);
	drv_usecwait(1);
	outl(iobase + SIA_CONNECT_REG, SIA_CONNECT_MASK);
	outl(iobase + SIA_TXRX_REG, SIA_TXRX_MASK_TP);
	outl(iobase + SIA_GENERAL_REG, SIA_GENRL_MASK_TP);
	outl(iobase + SIA_CONNECT_REG, AUTO_CONFIG);
	drv_usecwait(10000);	/* transceiver relay */

	*(u_long *)&desc[dnetp->tx_current_desc].desc0 = 0;	/* init descs */
	*(u_long *)&desc[dnetp->tx_current_desc].desc1 &= DNET_END_OF_RING;
	desc[dnetp->tx_current_desc].desc1.buffer_size1 = TX_BUF_SIZE;
	desc[dnetp->tx_current_desc].desc1.first_desc = 1;
	desc[dnetp->tx_current_desc].desc1.last_desc = 1;
	desc[dnetp->tx_current_desc].desc1.int_on_comp = 1;
	desc[dnetp->tx_current_desc].desc0.own = 1;

	outl(iobase + OPN_MODE_REG, START_TRANSMIT);
	outb(iobase + TX_POLL_REG, TX_POLL_DEMAND);

	/*
	 * Give enough time for the chip to transmit the packet
	 */
	while (desc[dnetp->tx_current_desc].desc0.own && delay--)
		drv_usecwait(10);

	if (!desc[dnetp->tx_current_desc].desc0.own) {
		/*
		 * 10-BaseT has succeeded
		 */
		(void) dnet_init21040(macinfo);
		outl(iobase + SIA_CONNECT_REG, AUTO_CONFIG);
		drv_usecwait(10000);	/* transceiver relay */
		return;
	}
	/*
	 * Try BNC/AUI
	 */
	(void) dnet_init21040(macinfo);
	outl(iobase + SIA_CONNECT_REG, SIA_CONNECT_MASK);
	outl(iobase + SIA_TXRX_REG, SIA_TXRX_MASK_BNCAUI);
	outl(iobase + SIA_GENERAL_REG, SIA_GENRL_MASK_BNCAUI);
	outl(iobase + SIA_CONNECT_REG, SIA_CONN_MASK_BNCAUI);
	*(u_long *)&desc[dnetp->tx_current_desc].desc0 = 0;	/* init descs */
	*(u_long *)&desc[dnetp->tx_current_desc].desc1 &= DNET_END_OF_RING;
	desc[dnetp->tx_current_desc].desc1.buffer_size1 = TX_BUF_SIZE;
	desc[dnetp->tx_current_desc].desc1.first_desc = 1;
	desc[dnetp->tx_current_desc].desc1.last_desc = 1;
	desc[dnetp->tx_current_desc].desc0.own = 1;
	desc[dnetp->tx_current_desc].desc1.int_on_comp = 1;
	outl(iobase + OPN_MODE_REG, START_TRANSMIT);
	outb(iobase + TX_POLL_REG, TX_POLL_DEMAND);
	delay = 0xFF;
	while (desc[dnetp->tx_current_desc].desc0.own && delay--)
		drv_usecwait(10);

	if (!desc[dnetp->tx_current_desc].desc0.own) {
		/*
		 * BNS/AUI has succeeded
		 */
		(void) dnet_init21040(macinfo);
		outl(iobase + SIA_GENERAL_REG, SIA_GENRL_MASK_BNCAUI);
		outl(iobase + SIA_CONNECT_REG, BNC_CONFIG);
		drv_usecwait(10000);	/* transceiver relay */
		return;
	}
	/*
	 * BNC/AUI has failed. Revert back to 10-BaseT
	 */
	(void) dnet_init21040(macinfo);
	outl(iobase + SIA_TXRX_REG, SIA_TXRX_MASK_TP);
	outl(iobase + SIA_GENERAL_REG, SIA_GENRL_MASK_TP);
	outl(iobase + SIA_CONNECT_REG, AUTO_CONFIG);
	drv_usecwait(10000);	/* transceiver relay */
}

int
dnet_init21040(gld_mac_info_t *macinfo)
{
	struct dnetinstance *dnetp =		/* Our private device info */
				(struct dnetinstance *)macinfo->gldm_private;
	int		iobase = macinfo->gldm_port;
	u_long 		val;
	caddr_t 	mem_ptr;

	/*
	 * before initializing the dnet should be in STOP state
	 */
	val = inl(iobase + OPN_MODE_REG);
	outl(iobase + OPN_MODE_REG, val & ~(START_TRANSMIT | START_RECEIVE));
	/*
	 * Reset the chip
	 */
	outl(iobase + BUS_MODE_REG, SW_RESET);
	drv_usecwait(3);
	outl(iobase + BUS_MODE_REG, 0);
	drv_usecwait(8);
	val = inl(iobase + OPN_MODE_REG);
	outl(iobase + STATUS_REG, CLEAR_INTR); 	/* Clear all interrupts */

	outl(iobase + BUS_MODE_REG, CACHE_ALIGN | BURST_SIZE); /* CSR0 */

	/*
	 * Initialize the TX and RX buffers
	 */
	mem_ptr = dnetp->base_mem;
	if ((u_long)dnetp->base_mem % 4)
		mem_ptr = (caddr_t)(((u_long)mem_ptr + 3) & 0xFFFFFFFC);

	if (dnet_init_txrx_bufs(macinfo, mem_ptr) == FAILURE)
		return (FAILURE);

	/*
	 * Set the base address of the Rx descrptor list in CSR4
	 */
	outl(iobase + RX_BASE_ADDR_REG, (ulong)dnetp->p_addr.rx_desc);

	/*
	 * Set the base address of the Tx descrptor list in CSR3
	 */
	outl(iobase + TX_BASE_ADDR_REG, (ulong)dnetp->p_addr.tx_desc);

	dnetp->tx_current_desc = dnetp->rx_current_desc = 0;
	dnetp->transmitted_desc = 0;
	dnetp->free_desc = MAX_TX_DESC;

	/*
	 * Set the interrupt masks in CSR7
	 */
	outl(iobase + INT_MASK_REG, NORMAL_INTR_MASK | ABNORMAL_INTR_MASK |
		TX_INTERRUPT_MASK | RX_INTERRUPT_MASK | SYSTEM_ERROR_MASK |
		TX_JABBER_MASK);

	outl(iobase + SIA_CONNECT_REG, 0);
	drv_usecwait(1);

	return (SUCCESS);
}
