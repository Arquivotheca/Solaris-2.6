/*
 * Copyright (c) 1993, 1996 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)elink.c	1.13	96/10/09 SMI"

/*
 *
 * NAME		elink.c
 *
 * SYNOPIS
 *
 * Source code of the driver for the 3Com EtherLink 16 Ethernet
 * LAN adapter board on Solaris 2.1 (x86)
 * Depends on the gld module of Solaris 2.1 (Generic LAN Driver)
 *
 * A> Exported functions.
 * (i) Entry points for the kernel-
 *		_init()
 *		_fini()
 *		_info()
 *
 * (ii) DDI entry points-
 *		elink_identify()
 *		elink_devinfo()
 *		elink_probe()
 *		elink_attach()
 *		elink_detach()
 *
 * (iii) Entry points for gld-
 *		elink_reset()
 *		elink_start_board()
 *		elink_stop_board()
 *		elink_saddr()
 *		elink_dlsdmult()
 *		elink_intr()
 *		elink_prom()
 *		elink_gstat()
 *		elink_send()
 *		elink_ioctl()
 *
 * B> Imported functions.
 * From gld
 *		gld_register()
 *		gld_unregister()
 *		gld_recv()
 *
 *
 * DESCRIPTION
 *
 * The elink Ethernet driver is a multi-threaded, dynamically loadable,
 * gld-compliant, clonable STREAMS hardware driver that supports the
 * connectionless service mode of the Data Link Provider Interface,
 * dlpi (7) over 3Com EtherLink 16 (ELINK) controller. The driver
 * can support multiple ELINK controllers on the same system. It provides
 * basic support for the controller such as chip initialization,
 * frame transmission and reception, multicasting and promiscuous mode support,
 * maintenance of error statistic counters and the time domain reflectometry
 * tests.
 *
 * The elink driver uses the Generic LAN Driver (gld) module of Solaris,
 * which handles all the STREAMS and DLPI specific functions for the driver.
 * It is a style 2 DLPI driver and supports only the connectionless mode of
 * data transfer. Thus, a DLPI user should issue a DL_ATTACH_REQ primitive
 * to select the device to be used. Refer dlpi (7) for more information.
 * For more details on how to configure the driver, refer to elink (7).
 *
 * CAVEATS
 *
 * NOTES
 * Maximum number of boards supported is 0x20: hopefully, a
 * system administrator will run out of slots if he wishes to add more
 * than 0x20 boards to the system
 *
 * Command chaining feature of the 82586 has not been exploited
 * in this version.
 *
 * "Dump" and "diagnose" commands of the 82586 are not supported.
 *
 * SEE ALSO
 *
 * /kernel/misc/gld
 * elink (7)
 * dlpi (7)
 * "Skeleton Network Device Drivers",
 *	Solaris 2.1 Device Driver Writer's Guide-- February 1993
 *
 * MODIFICATION HISTORY
 *
 *  Version 1.2 10/28/93 released on 28 Oct '93
 *  Update for fixing the following bugs reported by Sunsoft:
 *
 *  * Bugs in gathering statistics from the 82586 chip ::
 *    - collision count was being incremented by 1. But the value returned
 *      by the 82586 after transmission of a packet is the *number* of
 *      unsuccessful attempts made to transmit the packet. This has been
 *      fixed.
 *    - glds_errxmt is now incremented only when SCB_INT_CNA is not set.
 *    - updating all these counters is now done in a mutually exclusive
 *      fashion.
 *  * Limitation in configuration file
 *	  - The previous release expects the system administrator to give
 *		the correct configuration values in the conf file.
 *       - This version of the driver takes care of all possible I/O
 *         addresses, IRQ values and RAM settings.
 *
 * MISCELLANEOUS
 * 		vi options for viewing this file::
 *				set ts=4 sw=4 ai wm=4
 *
 * COPYRIGHTS
 * This file is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
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
 * Mountain View, California  94043
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/modctl.h>

#ifdef	_DDICT

/* from sys/dlpi.h */
#define	DL_ETHER	0x4	/* Ethernet Bus */

#include "sys/ethernet.h"
#include "sys/gld.h"

#else	/* not _DDICT */

#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/gld.h>

#endif	/* not _DDICT */

#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "sys/elink.h"

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "Ether Link 16";

#ifdef ELINKDEBUG
/* used for debugging */
int	elinkdebug = 0;
#endif

extern unchar inb(int);
extern void outb(int, unchar);

/* Required system entry points */
static	elink_identify(dev_info_t *);
static	elink_devinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	elink_probe(dev_info_t *);
static	elink_attach(dev_info_t *, ddi_attach_cmd_t);
static	elink_detach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
static int	elink_reset(gld_mac_info_t *);
static int	elink_start_board(gld_mac_info_t *);
static int	elink_stop_board(gld_mac_info_t *);
static int	elink_saddr(gld_mac_info_t *);
static int	elink_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
static int	elink_prom(gld_mac_info_t *, int);
static int	elink_gstat(gld_mac_info_t *);
static int	elink_send(gld_mac_info_t *, mblk_t *);
static u_int 	elink_intr(gld_mac_info_t *);
#ifdef	ELINK_TDR_TEST
static int 		elink_ioctl(queue_t *, mblk_t *);
#endif	/* ELINK_TDR_TEST */


/* Utility functions */
static	elink_old_probe(dev_info_t *);
static int 	elink_sig_chk(ushort);
static int 	elink_mem_conf(ulong, ulong);
static int 	elink_init_board(gld_mac_info_t *);

static void elink_init_rfa(gld_mac_info_t *);
static void	elink_init_cbl(gld_mac_info_t *);
static void elink_write_id_pat();

static int 	elink_ack(scb_t *, ushort);

static int 	elink_config(gld_mac_info_t *, ushort);
static int 	elink_wait_scb(scb_t *, int);
static void	elink_recv(gld_mac_info_t *);
static int 	elink_restart_ru(gld_mac_info_t *);
static int  elink_wait_active(scb_t *, int);
#ifdef	ELINK_TDR_TEST
static int  elink_tdr_test(gld_mac_info_t *);
#endif	/* ELINK_TDR_TEST */

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/* Standard Streams initialization */

static struct module_info minfo =
{
	ELINKIDNUM, "elink", 0, INFPSZ, ELINKHIWAT, ELINKLOWAT
};

static struct qinit rinit = 		/* read queues */
{
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = 	/* write queues */
{
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

static struct streamtab elinkinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_elinkops = {
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
	&elinkinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

static struct dev_ops elinkops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	elink_devinfo,		/* devo_getinfo */
	elink_identify,		/* devo_identify */
	elink_probe,		/* devo_probe */
	elink_attach,		/* devo_attach */
	elink_detach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_elinkops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&elinkops		/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

/*
 *
 * Auto configuration entry points
 *
 */

/*
 * Name			: _init()
 * Purpose		: Loads an instance of the driver dynamically
 * Called from	: Kernel
 * Arguments	: None
 * Returns		: Whatever mod_install() returns
 */

int
_init(void)
{
	return (mod_install(&modlinkage));
}

/*
 * Name			: _fini()
 * Purpose		: Unloads the driver dynamically
 * Called from	: Kernel
 * Arguments	: None
 * Returns		: Whatever mod_remove() returns
 */

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}

/*
 * Name			: _info()
 * Purpose		: Reports the status information about the driver
 * Called from	: Kernel
 * Arguments	: A pointer to a modinfo structure
 * Returns		: Whatever mod_info() returns
 */

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 *  DDI Entry Points
 */

/*
 * Name			: elink_identify()
 * Purpose		: Verifies whether this driver is elink driver
 * Called from	: Kernel
 * Arguments	: devinfo - pointer to a dev_info_t structure
 * Returns		: DDI_IDENTIFIED or DDI_NOT_IDENTIFIED
 */


elink_identify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "elink") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

/*
 * Name		: elink_devinfo()
 * Purpose	: Reports the instance number and identifies the devinfo
 *		  node with which the instance number is associated
 * Called from	: Kernel
 * Arguments	: devinfo - pointer to a devinfo_t structure
 *		  cmd     - command argument: either DDI_INFO_DEVT2DEVINFO or
 *						DDI_INFO_DEVT2INSTANCE
 *		  arg     - command specific argument
 *		  result  - pointer to where requested information is stored
 * Returns	: DDI_SUCCESS, on success
 *				  DDI_FAILURE, on failure
 */


/* ARGSUSED */
elink_devinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;

	/* This code is not DDI compliant: the correct semantics */
	/* for CLONE devices is not well-defined yet.		 */
	switch (cmd) {
		case DDI_INFO_DEVT2DEVINFO:
			if (devinfo == NULL) {
				error = DDI_FAILURE;	/* Unfortunate */
			}
			else
			{
				*result = (void *)devinfo;
				error = DDI_SUCCESS;
			}
			break;
		case DDI_INFO_DEVT2INSTANCE:
			/* CLONEDEV always returns zero */
			*result = (void *)0;
			error = DDI_SUCCESS;
			break;
		default:
			error = DDI_FAILURE;
	}
	return (error);
}

/*
 * Name			: elink_probe()
 * Called from  : Kernel
 * Purpose		: Verifies whether the target is really present
 * Arguments	: devinfo - pointer to a dev_info_t structure
 * Returns		: DDI_PROBE_SUCCESS or DDI_PROBE_FAILURE
 */

elink_probe(dev_info_t *devinfo)
{

	ushort ioaddr;
	int i;
	static int first_probe = 1;
	int board_irq;
	int board_mem;
	int *irqarr, irqlen;
	int reglen, nregs;
	struct {
		int bustype;
		int base;
		int size;
	} *reglist;

	if (ddi_getprop(DDI_DEV_T_ANY, devinfo, 0,
		"ignore-hardware-nodes", 0) != 0) {
		return (elink_old_probe(devinfo));
	}

	if (first_probe) {
		first_probe = 0;
		outb(ELINK_ID_PORT, 0);
		elink_write_id_pat();
		outb(ELINK_ID_PORT, 0);
	}

	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo,
			    DDI_PROP_DONTPASS, "reg",
			    (caddr_t)&reglist, &reglen) !=
		DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
		    "elink: reg property not found in devices property list");
		return (DDI_PROBE_FAILURE);
	}

	nregs = reglen / sizeof (*reglist);
	for (i = 0; i < nregs; i++)
		if (reglist[i].bustype == 1) {
			ioaddr = reglist[i].base;
			break;
		}

	if (i >= nregs) {
		cmn_err(CE_NOTE,
		    "elink: reg property, valid base address not specified");
		kmem_free(reglist, reglen);
		return (DDI_PROBE_FAILURE);
	}

/* Verify 3COM Adapter Exists */

	if (elink_sig_chk(ioaddr) == FALSE) {
		kmem_free(reglist, reglen);
		return (DDI_PROBE_FAILURE);
	}

/* Verify IRQ */

	board_irq = inb((int)ioaddr + IO_INTR_CFG) & IRQ_CONF_MSK;

	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo,
			DDI_PROP_DONTPASS, "interrupts",
			(caddr_t)&irqarr, &irqlen) !=
			DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
		    "elink: interrupts property not found in "
		    "devices property list");
		kmem_free(reglist, reglen);
		return (DDI_PROBE_FAILURE);
	}

	if (board_irq != irqarr[(irqlen/sizeof (int))-1]) {
		cmn_err(CE_WARN,
		    "elink: interrupts property does not match "
		    "that of adapter");
		kmem_free(irqarr, irqlen);
		kmem_free(reglist, reglen);
		return (DDI_PROBE_FAILURE);
	}
	kmem_free(irqarr, irqlen);

/* Verify RAM base */

	i = 0;
	board_mem = inb((int)ioaddr + IO_RAM_CFG) & RAM_CONF_MSK;

	while (i < nregs) {
		if ((reglist[i].bustype == 0) && (board_mem ==
		    elink_mem_conf(reglist[i].base, reglist[i].size))) {
			kmem_free(reglist, reglen);
			return (DDI_PROBE_SUCCESS);
		}
		i++;
	}

/* Failure to Verify */
	cmn_err(CE_WARN,
	    "elink: RAM base property does not match that of the adapter");
	kmem_free(reglist, reglen);
	return (DDI_PROBE_FAILURE);
}


elink_old_probe(dev_info_t *devinfo)
{
	ushort	io_base_addr; /* Port number of the I/O addr property */
	int		i;
	int		found = 0;
	int		regbuf[3];
	/* Posssible I/O addresses for the board */
	static ushort IO_Addrs[] = { 0x200, 0x210, 0x220, 0x230, 0x240, 0x250,
				    0x260, 0x280, 0x290, 0x2a0, 0x2b0, 0x2c0,
				    0x2d0, 0x2e0, 0x300, 0x310, 0x320, 0x330,
				    0x340, 0x350, 0x360, 0x380, 0x390, 0x3a0,
				    0x3e0 };

	static int	lastindex = -1;
	static int	probe_cnt = 0;
	int		size = sizeof (IO_Addrs)/sizeof (ushort);

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKDDI)
		cmn_err(CE_CONT, "\nelink_probe(0x%x)", devinfo);
#endif

	/*
	 * Change the adapter to RUN state
	 * Do it only once, because ID_PORT address is fixed for all the
	 * boards. So doing once will bring all the boards to RUN state
	 */
	if (probe_cnt == 0) {
		probe_cnt++;
		outb(ELINK_ID_PORT, 0);
		elink_write_id_pat();
		outb(ELINK_ID_PORT, 0);		/* RUN state */
	}

	if (lastindex >= size)
		return (DDI_PROBE_FAILURE);

	for (i = lastindex + 1; i < size; i++) {
		io_base_addr = IO_Addrs[i];
		if (elink_sig_chk(io_base_addr) == TRUE) {
			regbuf[0] = io_base_addr;
			(void) ddi_prop_create(DDI_DEV_T_NONE, devinfo,
			    DDI_PROP_CANSLEEP, "ioaddr", (caddr_t)regbuf,
			    sizeof (int));
			found++;
			break;
		}
	}
	lastindex = i;

	if (found)
		return (DDI_PROBE_SUCCESS);
	else
		return (DDI_PROBE_FAILURE);
}

/*
 * Name		: elink_attach()
 * Called from  : Kernel
 * Purpose	: Attaches a driver instance to the system.
 * Arguments	: devinfo - pointer to a dev_info_t structure
 *		: cmd	  - ddi_attach_cmd_t structure
 * Returns	: DDI_SUCCESS or DDI_FAILURE
 */

elink_attach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;	/* GLD structure */
	struct elinkinstance *elinkp;	/* Our private device info */
	register ushort io_base_addr;	/* I/O address of the board */
	ulong ram_base_addr = 0, ram_size = 0;   /* DPRAM */
	caddr_t devaddr;		/* Virtual address of RAM base */
	char conf_mem;			/* Configured mem settings */
	unchar conf_irq;		/* Configured IRQ level */
	int i, index, j;
	int num_intrs, *irqarr, irqlen;
	int new_boot;
	char val = -1;			/* Scratch */
	int reglen, nregs;
	struct reg_t {		/* structure used to get REG property */
		int	bustype;
		int	base_addr;
		int	mem_size;
	}  *busreg;

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKDDI)
		cmn_err(CE_CONT, "\nelink_attach(0x%x)", devinfo);
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);
	/*
	 *  Allocate gld_mac_info_t and elinkinstance structures
	 */
	macinfo = (gld_mac_info_t *)kmem_zalloc(sizeof (gld_mac_info_t) +
	    sizeof (struct elinkinstance), KM_NOSLEEP);
	if (macinfo == NULL) {
		cmn_err(CE_WARN, "elink: kmem_zalloc failure for macinfo");
		return (DDI_FAILURE);
	}
	elinkp = (struct elinkinstance *)(macinfo + 1);

	/*  Initialize our private fields in macinfo and elinkinstance */
	macinfo->gldm_private = (caddr_t)elinkp;
	macinfo->gldm_state = ELINK_IDLE;
	macinfo->gldm_flags = 0;
	elinkp->restart_count = 0;

	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS, "reg",
	    (caddr_t)&busreg, &reglen) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
		    "elink: reg property not found in devices property list");
		goto failure3;
	}
	nregs = reglen / sizeof (struct reg_t);

	/*
	 * the probe routine guarantees that io_base_addr will be non-zero
	 * after this loop
	*/
	if (ddi_getprop(DDI_DEV_T_ANY, devinfo, 0,
	    "ignore-hardware-nodes", 0) != 0) {
		io_base_addr = macinfo->gldm_port = ddi_getprop(DDI_DEV_T_ANY,
		    devinfo, DDI_PROP_DONTPASS, "ioaddr", 0);
		new_boot = 0;
	} else {
		new_boot = 1;
		for (i = 0; i < nregs; i++)
			if (busreg[i].bustype == 1) {
				io_base_addr = macinfo->gldm_port =
				    busreg[i].base_addr;
				break;
			}
	}

	/*
	 *  Check whether the configured irq and mem settings matches
	 *  with conf file values. If not, return DDI_FAILURE so that
	 *  it will be called with the next set of conf entries
	 */
	conf_mem = inb((int)io_base_addr + IO_RAM_CFG) & RAM_CONF_MSK;

	conf_irq = inb((int)io_base_addr + IO_INTR_CFG) & IRQ_CONF_MSK;

	/* Get the gldm_irq_index value */
	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
	    "interrupts", (caddr_t)&irqarr, &irqlen) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN, "elink: interrupts property not found "
		    "in devices property list");
		goto failure2;
	}

	index = 0;
	num_intrs = irqlen / sizeof (int);
	if (new_boot) {
	    if (irqarr[num_intrs-1] != conf_irq) {
		cmn_err(CE_WARN, "elink: interrupts property does not match "
		    "that of adapter");
		goto failure1;
	    }
	} else {
		for (i = 1; i < num_intrs; i += 2) {
		    if (irqarr[i] == conf_irq)
			break;
		    index++;
		}
		if (i >= num_intrs) {
			cmn_err(CE_WARN, "elink: interrupts property "
			    "does not match that of adapter");
			goto failure1;
		}
	}
	macinfo->gldm_irq_index = index;
	kmem_free(irqarr, irqlen);

	/*
	 *  Get start of DPRAM and size of DPRAM  from elink.conf and store
	 *  it in our private structure elinkinstance
	 *
	 * Get the gldm_req_index value
	 *
	 */

	for (i = 0; i < nregs; i++) {
		if (busreg[i].bustype != 0)
			continue;
		ram_base_addr = busreg[i].base_addr;
		ram_size = busreg[i].mem_size;
		if ((val = elink_mem_conf(ram_base_addr, ram_size)) == FAILURE)
			continue;

		if (val == conf_mem) {
			macinfo->gldm_reg_index = i;
			break;
		}
	}
	kmem_free(busreg, reglen);

	/* Check whether adapter configuration and reg property have matched */
	if (i >= nregs) {
		cmn_err(CE_CONT,
		    "elink: reg property does not match adapter configuration");
		goto failure3;
	}
	/* Map in the device memory to see if we can look at it successfully */
	if (ddi_map_regs(devinfo, i, &devaddr, 0, 0) != 0) {
		cmn_err(CE_CONT, "elink: ddi_map_regs can't map RAM address");
		goto failure3;
	}
	elinkp->ram_virt_addr = devaddr;
	elinkp->ram_size = ram_size;

	/*
	 * If the configured RAM is < 64k, change the ram base depending
	 * on the ram size. This is to make sure that always scp,iscp
	 * starts at the bottom of 64k.
	 */
	elinkp->ram_virt_addr -= (MAX_RAM_SIZE - elinkp->ram_size);

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */

	macinfo->gldm_reset   = elink_reset;
	macinfo->gldm_start   = elink_start_board;
	macinfo->gldm_stop    = elink_stop_board;
	macinfo->gldm_saddr   = elink_saddr;
	macinfo->gldm_sdmulti = elink_dlsdmult;
	macinfo->gldm_prom    = elink_prom;
	macinfo->gldm_gstat   = elink_gstat;
	macinfo->gldm_send    = elink_send;
	macinfo->gldm_intr    = elink_intr;
#ifdef	ELINK_TDR_TEST
	macinfo->gldm_ioctl   = elink_ioctl;
#else	/* not ELINK_TDR_TEST */
	macinfo->gldm_ioctl   = NULL; /* if you have one, NULL otherwise */
#endif	/* not ELINK_TDR_TEST */

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */

	macinfo->gldm_ident 	= ident;
	macinfo->gldm_type 	= DL_ETHER;
	macinfo->gldm_minpkt 	= 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt 	= ELINKMAXPKT;
	macinfo->gldm_addrlen 	= ETHERADDRL;
	macinfo->gldm_saplen 	= -2;

	if ((val = elink_sig_chk(io_base_addr)) != TRUE) {
		cmn_err(CE_CONT,
		    "elink: Board was not found at address 0x%x ",
		    io_base_addr);
		goto failure3;
	}

	if (inb(io_base_addr + IO_ROM_CFG) & CFG_MSK) {
		macinfo->gldm_media = GLDM_BNC;
#ifdef ELINKDEBUG
		if (elinkdebug & ELINKTRACE)
			cmn_err(CE_CONT, "(BNC)");
#endif
	} else {
		macinfo->gldm_media = GLDM_AUI;
#ifdef ELINKDEBUG
		if (elinkdebug & ELINKTRACE)
			cmn_err(CE_CONT, "(AUI)");
#endif
	}

	/* Get the ethernet address from CR_VB1 and put it in macinfo */
	outb(io_base_addr + IO_CTRL_REG, CR_VB1);
	for (j = 0; j < ETHERADDRL; j++)
		macinfo->gldm_vendor[j] = inb((int)macinfo->gldm_port + j);

	if (elink_init_board(macinfo) == FAILURE) {
		cmn_err(CE_CONT, "\nelink : init_board failed");
		goto failure3;
	}

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);
	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);


	/* Make sure we have our address set */
	if (elink_saddr(macinfo) == FAILURE) {
		cmn_err(CE_CONT, "\nelink : Couldn't setup the IA address ");
		goto failure3;
	}

	/* clear interrupt */
	outb(io_base_addr + IO_INTCLR, 1);

	/*
	 *  Register ourselves with the GLD interface
	 *
	 *  gld_register will:
	 *	link us with the GLD system;
	 *	set our ddi_set_driver_private(9F) data to the macinfo pointer;
	 *	save the devinfo pointer in macinfo->gldm_devinfo;
	 *	map the registers, putting the kvaddr into macinfo->gldm_memp;
	 *	add the interrupt, putting the cookie in gldm_cookie;
	 *	init the gldm_intrlock mutex which will block that interrupt;
	 *	create the minor node.
	 */

	if (gld_register(devinfo, "elink", macinfo) == DDI_SUCCESS)
		return (DDI_SUCCESS);
	else
		goto failure3;

failure1:
	kmem_free(irqarr, irqlen);

failure2:
	kmem_free(busreg, reglen);

failure3:
	kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t)+sizeof (struct elinkinstance));

	return (DDI_FAILURE);


}

/*
 * Name		: elink_detach()
 * Purpose	: Detach a driver instance from the system.This
 *                includes unregistering the driver from gld
 * Called from	: Kernel
 * Arguments	: devinfo - pointer to the device's dev_info structure
 *		  cmd     - type of command, should be DDI_DETACH always
 * Returns	: DDI_SUCCESS if the device was successfully removed
 *				  DDI_FAILURE otherwise
 * Side effects	: None
 */

elink_detach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */

#ifdef STATS
	struct elinkinstance *elinkp;
#endif STATS

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKDDI)
		cmn_err(CE_CONT, "\nelink_detach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);

#ifdef STATS
	elinkp = (struct elinkinstance *)macinfo->gldm_private;
	cmn_err(CE_NOTE, "No. of restarts :: 0x%x\n", elinkp->restart_count);
#endif STATS

	/* stop the board if it is running */
	(void) elink_stop_board(macinfo);

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
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t)+sizeof (struct elinkinstance));
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}

/*
 *
 *  				GLD Entry Points
 *
 */

/*
 * Name		: elink_reset()
 * Purpose	: Reset the board to its initial state
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns	: TRUE, if the board was successfully reset
 *				FALSE, otherwise
 * Side effects : All data structures and lists maintained by the
 *                82586 are flushed
 */

static int
elink_reset(gld_mac_info_t *macinfo)
{
#ifdef ELINKDEBUG
	if (elinkdebug & ELINKTRACE)
		cmn_err(CE_CONT, "\nelink_reset(0x%x)", macinfo);
#endif

	(void) elink_stop_board(macinfo);
	if (elink_init_board(macinfo) == FAILURE) {
		cmn_err(CE_CONT, "\nelink : init_board failed");
		return (FALSE);
	}
	if (elink_saddr(macinfo) == FAILURE) {
		cmn_err(CE_CONT, "\nelink : Couldn't setup the IA address");
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Name		: elink_start_board()
 * Purpose	: Start the device by enabling the receiver and interrupts
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns	: SUCCESS on success, FAILURE on failure
 * Side effects	: Receiver unit of the 82586 and interrupts get enabled
 */

static int
elink_start_board(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;

	scb_t	*scb_p  = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);
	register ushort	io_base_addr  = macinfo->gldm_port;

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKTRACE)
		cmn_err(CE_CONT, "\nelink_start_board(0x%x)", macinfo);
#endif

	/*
	 * enable 586 Receive Unit
	 */
	scb_p->scb_status   = 0;
	scb_p->scb_cmd	    = SCB_RUC_STRT;
	scb_p->scb_rfa_ofst = elinkp->fd_ofst;

	outb(io_base_addr + IO_CHAN_ATN, 1);

	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	if (elink_ack(scb_p, io_base_addr))
		return (FAILURE);

	/*
	 * enable interrupt
	 */
	outb(io_base_addr + IO_INTCLR, 1);
	outb(io_base_addr + IO_CTRL_REG, CR_RST | CR_IEN);
	outb(io_base_addr + IO_INTCLR, 1);

	return (SUCCESS);

}

/*
 *  elink_stop_board() -- stop board receiving
 */
/*
 * Name		: elink_stop_board()
 * Purpose	: Stop the device by disabling the receiver and interrupts
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns	: TRUE on success
 *				  FALSE on failure
 * Side effects	: Receiver unit of the 82586 and interrupts are disabled
 */

static int
elink_stop_board(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;
	register ushort io_base_addr = macinfo->gldm_port;

	scb_t	*scb_p = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKTRACE)
		cmn_err(CE_CONT, "\nelink_stop_board(0x%x)", macinfo);
#endif

	/*
	 * stop the RU
	 */
	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE) {
		cmn_err(CE_NOTE, "\nelink : SCB command word not cleared");
		return (FAILURE);
	}

	scb_p->scb_status   = 0;
	scb_p->scb_cmd	    = SCB_CUC_SUSPND | SCB_RUC_SUSPND;
	outb(macinfo->gldm_port + IO_CHAN_ATN, 1);	/* channel attention */

	/* disable interrupts */
	outb(io_base_addr + IO_INTCLR, 1);
	outb(io_base_addr + IO_CTRL_REG, CR_RST);
	outb(io_base_addr + IO_INTCLR, 1);

	return (SUCCESS);

}

/*
 * Name			: elink_saddr()
 * Purpose		: Program the 82586 with the physical network address
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: SUCCESS on success
 *				  FAILURE on failure
 * Side effects	: None
 */

static int
elink_saddr(gld_mac_info_t *macinfo)
{
	/* Our private device info */
	struct elinkinstance *elinkp =
		(struct elinkinstance *)macinfo->gldm_private;
	/* Pointer to the SCB structure */
	scb_t	*scb_p = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);
	/* Pointer to the Command(gen) structure */
	gencmd_t *cmd_p = (gencmd_t *)
	    (elinkp->ram_virt_addr + elinkp->gencmd_ofst);
	/* I/O address of the board */
	register ushort	io_base_addr = macinfo->gldm_port;
	int		i;

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKTRACE)
		cmn_err(CE_CONT, "\nelink_saddr(0x%x)", macinfo);
#endif

	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	if (elink_wait_active(scb_p, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	scb_p->scb_status   = 0;
	scb_p->scb_cmd	    = SCB_CUC_STRT;
	scb_p->scb_cbl_ofst = elinkp->gencmd_ofst;
	scb_p->scb_rfa_ofst = elinkp->fd_ofst;

	cmd_p->cmd_status   = 0;
	cmd_p->cmd_cmd	    = CS_CMD_IASET | CS_EL;
	cmd_p->cmd_nxt_ofst = 0xffff;

	for (i = 0; i < ETHERADDRL; i++)
		cmd_p->cmd_prm.pr_ind_addr_set[i] = macinfo->gldm_macaddr[i];

	outb(io_base_addr + IO_CHAN_ATN, 1);

	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	if (elink_ack(scb_p, io_base_addr))
		return (FAILURE);

	return (SUCCESS);

}

/*
 * Name			: elink_dlsdmult()
 * Purpose		: Enable/disable device level reception at specific
 *				  multicast addresses
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 *				  mcast   - multicast address
 *				  op      - enable(1) / disable(0) flag
 * Returns		: TRUE   on success
 *				  FALSE  on failure
 * Side effects	: None
 */

static int
elink_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct elinkinstance *elinkp =	/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;
	/* Pointer to the SCB structure */
	scb_t	*scb = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);
	/* Pointer to the command(gen) structure */
	gencmd_t *cmd = (gencmd_t *)
	    (elinkp->ram_virt_addr + elinkp->gencmd_ofst);
	/* I/O address of the board */
	register ushort	io_base_addr = macinfo->gldm_port;
	int	i;
	short cnt = 0;

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKTRACE)
		cmn_err(CE_CONT, "\nelink_dlsdmult(0x%x, %s)", macinfo,
				op ? "ON" : "OFF");
#endif


	if (elink_wait_scb(scb, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	if (op) {
		/*
		 * add multicast
		 * update local table and then give list of addresses to 82586
		 */
		for (i = 0; i <  GLD_MAX_MULTICAST; i++) {
			if (elinkp->elink_multiaddr[i].entry[0] == 0) {
				bcopy((caddr_t)mcast->ether_addr_octet,
				    (caddr_t)elinkp->elink_multiaddr[i].entry,
				    ETHERADDRL);
				elinkp->mcast_count++;
				break;
			}
		}
		if (i >= GLD_MAX_MULTICAST) {
			cmn_err(CE_CONT, "\nelink : Multicast table full");
			return (FAILURE);
		}
	}
	else
	{
		/*
		 * remove multicast
		 * update local table first
		 */
		for (i = 0; i <  GLD_MAX_MULTICAST; i++) {
			if (bcmp((caddr_t)mcast->ether_addr_octet,
			    (caddr_t)elinkp->elink_multiaddr[i].entry,
			    ETHERADDRL) == 0) {
				/* matching entry found - invalidate it */
				elinkp->elink_multiaddr[i].entry[0] = 0;
				elinkp->mcast_count--;
				break;
			}
		}
		if (i >= GLD_MAX_MULTICAST) {
			cmn_err(CE_CONT, "\nelink : No matching "
			    "multicast entry found");
			return (FAILURE);
		}
	} /* else */

	if (elink_wait_active(scb, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	scb->scb_status   = 0;
	scb->scb_cmd	  = SCB_CUC_STRT;
	scb->scb_cbl_ofst = elinkp->gencmd_ofst;  /* ofst is general cmd */

	cmd->cmd_status   = 0;
	cmd->cmd_cmd	  = CS_CMD_MCSET | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

	/*
	 * Now give the list of addresses to 82586
	 */
	for (i = 0; i < GLD_MAX_MULTICAST; i++) {
		if (elinkp->elink_multiaddr[i].entry[0] == 0) {
			continue;
		}
		bcopy((caddr_t)elinkp->elink_multiaddr[i].entry,
		    (caddr_t)&cmd->cmd_prm.pr_mcaddr.mcast_addr[cnt],
		    ETHERADDRL);
#ifdef ELINKDEBUG
		if (elinkdebug & ELINKTRACE) {
			cmn_err(CE_CONT, "\nAdding mulicast : ");
			elink_display_eaddr(elinkp->elink_multiaddr[i].entry);
		}
#endif
		cnt += ETHERADDRL;
	}

	cmd->cmd_prm.pr_mcaddr.mcast_cnt = cnt;

	outb(io_base_addr + IO_CHAN_ATN, 1);

	if (elink_wait_scb(scb, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	if (elink_ack(scb, io_base_addr) == FAILURE)
		return (FAILURE);

	return (SUCCESS);

}

/*
 * Name			: elink_prom()
 * Purpose		: Enable/disable physical level promiscuous mode
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: SUCCESS on success
 *				  FAILURE on failure
 * Side effects	:
 */

static int
elink_prom(gld_mac_info_t *macinfo, int on)
{

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKTRACE)
		cmn_err(CE_CONT, "\nelink_prom(0x%x, %s)", macinfo,
				on ? "ON" : "OFF");
#endif

	if (on) {	/* Enable promiscuous mode */
		return (elink_config(macinfo, PRO_ON));
	} else {	/* Disable promiscuous mode */
		return (elink_config(macinfo, PRO_OFF));
	}
}

/*
 * Name			: elink_gstat()
 * Purpose		: Gather statistics from the hardware and update the
 *				  gldm_stats structure.
 * Called from	: gld, just before it reads the driver's statistics
 *                structure
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: SUCCESS always
 * Side effects	: None
 */

static int
elink_gstat(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;

	scb_t *scb_p = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKTRACE)
		cmn_err(CE_CONT, "\nelink_gstat(0x%x)", macinfo);
#endif

	macinfo->gldm_stats.glds_crc = scb_p->scb_crc_err;
	macinfo->gldm_stats.glds_frame = scb_p->scb_aln_err;
	macinfo->gldm_stats.glds_missed = scb_p->scb_rsc_err;
	macinfo->gldm_stats.glds_overflow = scb_p->scb_ovrn_err;

	return (SUCCESS);
}

/*
 * Name		: elink_send()
 * Purpose	: Transmit a packet on the network. Note that this
 *                function returns even before transmission by the
 *                82586 completes. Hence, return value of SUCCESS is
 *                no guarantee that the packet was successfully
 *                transmitted (that is, without errors during
 *                transmission)
 *
 * Called from	: gld, when a packet is ready to be transmitted
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 *		  mp      - pointer to an M_DATA message that contains
 *                          the packet. The complete LLC header is
 *                          contained in the message's first message
 *                          block, and the remainder of the packet is
 *                          contained within additional M_DATA message
 *                          blocks linked to the first message block
 * Returns	: SUCCESS if a command was issued to the 82586 to
 *                transmit a packet
 *		  FAILURE   on failure so that gld may retry later
 * Side effects	: None
 */

static int
elink_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	struct elinkinstance *elinkp =
	    (struct elinkinstance *)macinfo->gldm_private;
	struct ether_header *hdr = (struct ether_header *)mp->b_rptr;

	gencmd_t *cmd_p = (gencmd_t *)
	    (elinkp->ram_virt_addr + elinkp->cmd_ofst);
	tbd_t *tbd_p = (tbd_t *)(elinkp->ram_virt_addr +
	    cmd_p->cmd_prm.pr_xmit.xmt_tbd_ofst);
	/* Pointer to the xmit buffer */
	caddr_t txb_p = (caddr_t)(elinkp->ram_virt_addr + tbd_p->tbd_buff);
	scb_t *scb_p = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);
	char *src = (char *)(hdr->ether_dhost.ether_addr_octet);
	char		*dest;				/* Scratch */
	mblk_t		*local_mp;			/* Scratch */
	int			data_size = 0; 		/* Counter */
	int			msg_size = 0;		/* Counter */

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKSEND)
		cmn_err(CE_CONT, "\nelink_send(0x%x, 0x%x)", macinfo, mp);
#endif

	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE) {
		cmn_err(CE_CONT, "\nelink : SCB command word not cleared");
		macinfo->gldm_stats.glds_defer++;
		return (RETRY);
	}

	/*
	 * If previous command is not complete, let gld handle retry.
	 */

	if (scb_p->scb_status & SCB_CUS_ACTV) {
		macinfo->gldm_stats.glds_defer++;
		return (RETRY);
	}

	/*
	 * Copy the dest addr to cmd structure
	 */

	dest = (char *)(cmd_p->cmd_prm.pr_xmit.xmt_dest);
	bcopy(src, dest, ETHERADDRL);
	cmd_p->cmd_prm.pr_xmit.xmt_length = hdr->ether_type;
	mp->b_rptr += ETHER_HDR_LEN;

	/*
	 * copy data to transmit buffer.
	 */

	for (local_mp = mp; local_mp != NULL; local_mp = local_mp->b_cont) {
		msg_size = local_mp->b_wptr - local_mp->b_rptr;
		if (msg_size == 0)
			continue;
		bcopy((caddr_t)local_mp->b_rptr, txb_p, msg_size);
		txb_p += msg_size;
		data_size += msg_size;
	}

	if (data_size < MIN_DATA_SIZE)
		data_size = MIN_DATA_SIZE;

	tbd_p->tbd_count = data_size | CS_EOF;

	scb_p->scb_cmd	    = SCB_CUC_STRT;
	scb_p->scb_cbl_ofst = elinkp->cmd_ofst;

	/*
	 * Issue a channel attention, to start 82586
	 */

	outb(macinfo->gldm_port + IO_CHAN_ATN, 1);

	return (SUCCESS);
}

/*
 * Name			: elink_intr()
 * Purpose		: Interrupt handler for the device
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: DDI_INTR_CLAIMED   if the interrupt was serviced
 *				  DDI_INTR_UNCLAIMED otherwise
 * Side effects	: None
 */

static u_int
elink_intr(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;

	register ushort	io_base_addr = macinfo->gldm_port;
	ushort		scb_status;
	ushort		cmd_status;
	scb_t *scb_p = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);
	gencmd_t *cmd_p = (gencmd_t *)
	    (elinkp->ram_virt_addr + elinkp->cmd_ofst);

#ifdef ELINKDEBUG
	if (elinkdebug & ELINKINT)
		cmn_err(CE_CONT, "\nelink_intr(0x%x)", macinfo);
#endif
	/*
	 * Wait for scb command to be cleared
	 */
	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE) {
		cmn_err(CE_CONT, "\nelink_intr: SCB command word not cleared");
		return (DDI_INTR_UNCLAIMED);
	}
	scb_status = scb_p->scb_status;
	cmd_status = cmd_p->cmd_status;

	macinfo->gldm_stats.glds_intr++;

	/*
	 * acknowledge 82586 interrupt by giving channel attention
	 */
	if ((scb_p->scb_cmd = scb_status & SCB_INT_MSK) != 0)
		outb(io_base_addr + IO_CHAN_ATN, 1);

	if (scb_status & (SCB_INT_FR | SCB_INT_RNR))
		elink_recv(macinfo);
	if (scb_status & (SCB_INT_CX | SCB_INT_CNA)) {
		if (cmd_status & CS_COLLISIONS)
			macinfo->gldm_stats.glds_collisions +=
			    cmd_status & CS_COLLISIONS;
		else if (cmd_status & CS_CARRIER)
			macinfo->gldm_stats.glds_nocarrier++;
		else if (!(cmd_status & CS_OK) && (!(scb_status & SCB_INT_CNA)))
			macinfo->gldm_stats.glds_errxmt++;
		cmd_p->cmd_status = 0;
	}
	outb(io_base_addr + IO_INTCLR, 1);		/* clear interrupt */
	return (DDI_INTR_CLAIMED);	/* Indicate it was our interrupt */
}

#ifdef	ELINK_TDR_TEST
/*
 * Name			: elink_ioctl()
 * Purpose		: Implement device-specific ioctl commands
 * Called from	: gld
 * Arguments	: q  - pointer to a queue_t structure
 *				  mp - pointer to an mblk_t structure
 * Returns		: SUCCESS  if the ioctl command was successful
 *				  FAILURE otherwise
 * Side effects	: None
 */

static int
elink_ioctl(queue_t *q, mblk_t *mp)
{
	gld_t *gldp = (gld_t *)(q->q_ptr);	/* gld private */
	gld_mac_info_t *macinfo = gldp->gld_mac_info; /* macinfo struct */
	int cmd;				/* ioctl cmd val */
	int retval = 0;

	if (((struct iocblk *)mp->b_rptr)->ioc_count == TRANSPARENT) {
#ifdef ELINKDEBUG
		if (elinkdebug & ELINKTRACE)
			cmn_err(CE_CONT, "\nelink: xparent ioctl");
#endif
		goto err;
	}

	switch (cmd = ((struct iocblk *)mp->b_rptr)->ioc_cmd) {
		default:
			cmn_err(CE_CONT, "\nelink: unknown ioctl 0x%x", cmd);
			goto err;

		case TDR_TEST :
			if ((retval = elink_tdr_test(macinfo)) == FAILURE)
				goto err;
			break;

	}	/* end of switch */

	/*
	 * acknowledge the ioctl
	 */
	((struct iocblk *)mp->b_rptr)->ioc_rval = retval;
	mp->b_datap->db_type = M_IOCACK;
	qreply(q, mp);
	return (SUCCESS);

err:
	((struct iocblk *)mp->b_rptr)->ioc_rval = -1;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(q, mp);
	return (FAILURE);
}
#endif	/* ELINK_TDR_TEST */

/*
 *
 *    Utility functions
 *
 */

/*
 * Name		: elink_init_board()
 * Purpose	: Initialize the specified network board. Initialize the
 *		  82586's SCP, ISCP and SCB; reset the 82586; do IA
 *                setup command to initialize the 82586's individual
 *                address.
 *		  DO NOT enable the Receive Unit and interrupts.
 * Called from	: elink_attach()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns	: SUCCESS if board initializations encountered no errors
 *		  FAIL    otherwise
 * Side effects	: Previous state of the 82586 and its data structures
 *                are lost
 */

static int
elink_init_board(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;

	ushort	scp_ofst  = MAX_RAM_SIZE - sizeof (scp_t);
	ushort	iscp_ofst = scp_ofst - sizeof (iscp_t);
	ushort	scb_ofst  = iscp_ofst - sizeof (scb_t);

	scp_t	*scp_p  = (scp_t *)(elinkp->ram_virt_addr + scp_ofst);
	iscp_t	*iscp_p = (iscp_t *)(elinkp->ram_virt_addr + iscp_ofst);
	scb_t	*scb_p  = (scb_t *)(elinkp->ram_virt_addr + scb_ofst);

	ushort	io_base_addr  = macinfo->gldm_port;
	int		i;


	/* fill in scp */
	scp_p->scp_sysbus = 0;
	scp_p->scp_iscp = iscp_ofst;
	scp_p->scp_iscp_base = 0;

	/* fill in iscp */
	iscp_p->iscp_busy = 1;
	iscp_p->iscp_scb_ofst = scb_ofst;
	iscp_p->iscp_scb_base = 0;

	/* fill in scb */
	scb_p->scb_status = 0;
	scb_p->scb_cmd = 0;
	scb_p->scb_cbl_ofst = 0xffff;
	scb_p->scb_rfa_ofst = 0xffff;

	scb_p->scb_crc_err  = 0;
	scb_p->scb_aln_err  = 0;
	scb_p->scb_rsc_err  = 0;
	scb_p->scb_ovrn_err = 0;

	elinkp->scb_ofst = scb_ofst;
	elinkp->gencmd_ofst = scb_ofst - sizeof (gencmd_t);

	elink_init_cbl(macinfo);
	elink_init_rfa(macinfo);

	/*
	 * start the 82586, by resetting the 586 then issuing a channel
	 * attention
	 */
	outb(io_base_addr + IO_CTRL_REG, 0);
	outb(io_base_addr + IO_CTRL_REG, CR_RST);
	outb(io_base_addr + IO_CHAN_ATN, 1);	/* channel attention */

	for (i = 0; i < 100; i++) {
		if (!iscp_p->iscp_busy)
			break;
		drv_usecwait(100);
	}
	if (i == 100) {
		return (FAILURE);
	}

	/* wait for scb status */
	for (i = 0; i < 100; i++) {
		if (scb_p->scb_status == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait(100);
	}
	if (i == 100) {			/* if CX & CNA aren't set */
		return (FAILURE);
	}
	if (elink_ack(scb_p, io_base_addr)) {
		return (FAILURE);
	}
	/* configure 586 */
	if (elink_config(macinfo, PRO_OFF)) {
		return (FAILURE);
	}

	return (SUCCESS);
}

/*
 * Name			: elink_recv()
 * Purpose		: Get a packet that has been received by the hardware
 *                and pass it up to gld
 * Called from	: elink_intr()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: None
 */

static void
elink_recv(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;

	fd_t	*fd_p;
	rbd_t	*first_rbd, *last_rbd;
	mblk_t	*mp;
	long	length;


	/* for all fds */
	for (fd_p = elinkp->begin_fd;
	    (char *)fd_p != elinkp->ram_virt_addr+0xffff;
	    fd_p = elinkp->begin_fd) {
		if (!(fd_p->fd_status & CS_CMPLT))
			break;
		length = 0;
		first_rbd = last_rbd = (rbd_t *)(elinkp->ram_virt_addr +
		    fd_p->fd_rbd_ofst);
		if (fd_p->fd_rbd_ofst != 0xffff) {
			if ((last_rbd->rbd_status & CS_EOF) == CS_EOF)
				length = last_rbd->rbd_status & CS_RBD_CNT_MSK;
			else length = MAX_RXB_SIZE;
			if ((mp = allocb(ETHER_HDR_LEN + length, BPRI_MED))
			    == NULL) {
				cmn_err(CE_CONT,
				    "\nelink:No buffers for receive frame ");
				return;
			} else {
				int		i, l;

				/* fill in the mac header */
				bcopy((caddr_t)fd_p->fd_dest,
				    (caddr_t)mp->b_wptr,
				    ETHER_HDR_LEN);
				mp->b_wptr += ETHER_HDR_LEN;
				/* fill in the data from rcv buffers */
				i = 0;
				while (TRUE) {
					i++;
					l = last_rbd->rbd_status &
					    CS_RBD_CNT_MSK;
					bcopy((caddr_t)elinkp->ram_virt_addr +
					    last_rbd->rbd_buff,
					    (caddr_t)mp->b_wptr, l);
					mp->b_wptr += l;
					if ((last_rbd->rbd_status & CS_EOF) ==
					    CS_EOF) {
						last_rbd->rbd_status = 0;
						break;
					}
					if ((last_rbd->rbd_size & CS_EL) ==
					    CS_EL) {
						cmn_err(CE_CONT,
						    "\nelink :out of "
						    "receive buffers ");
						break;
					}
					last_rbd->rbd_status = 0;
					last_rbd =
					    (rbd_t *)(elinkp->ram_virt_addr +
					    last_rbd->rbd_nxt_ofst);
				}
				gld_recv(macinfo, mp);
			}

			/* re-queue rbd */
			elinkp->begin_rbd = (rbd_t *)(elinkp->ram_virt_addr +
			    last_rbd->rbd_nxt_ofst);

			last_rbd->rbd_status = 0;
			last_rbd->rbd_size   |= CS_EL;
			last_rbd->rbd_nxt_ofst = 0xffff;
			elinkp->end_rbd->rbd_nxt_ofst = (char *)first_rbd -
			    elinkp->ram_virt_addr;
			elinkp->end_rbd->rbd_size &= ~CS_EL;
			elinkp->end_rbd = last_rbd;
		}

		/* re-queue fd */
		elinkp->begin_fd = (fd_t *)(elinkp->ram_virt_addr +
		    fd_p->fd_nxt_ofst);
		elinkp->end_fd->fd_nxt_ofst = (char *)fd_p -
		    elinkp->ram_virt_addr;
		elinkp->end_fd->fd_cmd = 0;
		elinkp->end_fd = fd_p;

		fd_p->fd_status = 0;
		fd_p->fd_cmd = CS_EL;
		fd_p->fd_nxt_ofst = 0xffff;
		fd_p->fd_rbd_ofst = 0xffff;

	}
	(void) elink_restart_ru(macinfo);
}

/*
 * Name			: elink_restart_ru()
 * Purpose		: Check the status of RU and restart it if necessary
 * Called from	: elink_rcv_packet()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: Previous state of the 82586 is lost if the RU is restarted
 */

static int
elink_restart_ru(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;

	scb_t	*scb_p = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);
	fd_t	*begin_fd = elinkp->begin_fd;

	/*
	 *  Already running, so no need to start again
	 */
	if ((scb_p->scb_status & SCB_RUS_READY) == SCB_RUS_READY)
		return (TRUE);

	/*
	 * Receive queue is exhausted, don't restart ru
	 */
	if (begin_fd == NULL)
		return (FALSE);

	/*
	 *  If fd is complete - don't start ru. If necessary, it
	 *      will be called again.
	 */
	if (begin_fd->fd_status & CS_CMPLT)
		return (TRUE);

	elinkp->restart_count++;
	begin_fd->fd_rbd_ofst = (ushort) ((char *)elinkp->begin_rbd -
	    elinkp->ram_virt_addr);
	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE) {
		cmn_err(CE_CONT, "\nelink : SCB command word not cleared");
		return (FALSE);
	}
	scb_p->scb_rfa_ofst = (ushort) ((char *)elinkp->begin_fd -
	    elinkp->ram_virt_addr);
	scb_p->scb_cmd = SCB_RUC_STRT;
	outb(macinfo->gldm_port + IO_CHAN_ATN, 1);
	return (TRUE);
}

/*
 * Name			: elink_init_cbl()
 * Purpose		: Initialize the transmit buffer, transmit buffer
 *                descriptor and command block structures
 * Called from	: elink_init_board()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: Initializes some of the structures
 */

static void
elink_init_cbl(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;

	/* Offset of command block from RAM start */
	ushort	cmd_ofst = elinkp->cmd_ofst = MAX_RAM_SIZE - elinkp->ram_size;
	/* Offset of tbd from RAM start */
	ushort	tbd_ofst = elinkp->tbd_ofst = cmd_ofst + sizeof (gencmd_t);
	/* Offset of transmit buffer from RAM start */
	ushort	txb_ofst = elinkp->txb_ofst = tbd_ofst + sizeof (tbd_t);

	gencmd_t *cmd_p = (gencmd_t *)(elinkp->ram_virt_addr + cmd_ofst);
	tbd_t *tbd_p = (tbd_t *)(elinkp->ram_virt_addr + tbd_ofst);

	cmd_p->cmd_status = 0;
	cmd_p->cmd_cmd = CS_EL | CS_CMD_XMIT | CS_INT;
	cmd_p->cmd_nxt_ofst = 0xffff;
	cmd_p->cmd_prm.pr_xmit.xmt_tbd_ofst = tbd_ofst;

	tbd_p->tbd_count = 0;
	tbd_p->tbd_nxt_ofst = 0xffff;
	tbd_p->tbd_buff = txb_ofst;
	tbd_p->tbd_buff_base = 0;
}

/*
 * Name			: elink_init_rfa()
 * Purpose		: Initialize the receive buffers, frame and buffer
 *                descriptors in the shared RAM
 * Called from	: elink_init_board
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: None
 */

static void
elink_init_rfa(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp =		/* Our private device info */
		(struct elinkinstance *)macinfo->gldm_private;

	ushort	rxb_ofst = 0;
	ushort	rbd_ofst = 0;
	ushort	fd_ofst  = 0;
	ulong	mem_left;
	int 	num_rbd_to_fd;
	int		num_fd;
	int		num_rbd;
	ushort	i;
	fd_t    *fd_p;
	rbd_t   *rbd_p;

	/* First findout how much memory is left out */

	rxb_ofst = elinkp->rxb_ofst = elinkp->txb_ofst + TXB_SIZE;
	mem_left = elinkp->gencmd_ofst - elinkp->rxb_ofst;

	/* Allocate the remaining space for rbd's and fd's  */
	/* Findout the number of rbd's and fd'd we can have */
	if (MAX_RXB_SIZE == RXB_SIZE)
		num_rbd_to_fd = 1;
	else num_rbd_to_fd = (int)(MAX_RXB_SIZE / RXB_SIZE) + 1;

	num_fd = mem_left/(sizeof (fd_t)+
	    num_rbd_to_fd * sizeof (rbd_t) + num_rbd_to_fd * RXB_SIZE);
	num_rbd = num_rbd_to_fd * num_fd;
	rbd_ofst = elinkp->rbd_ofst = rxb_ofst + num_rbd * RXB_SIZE;
	fd_ofst  = elinkp->fd_ofst  = rbd_ofst + num_rbd * sizeof (rbd_t);

	/* initialize fd, rbd, begin_fd, end_fd pointers */

	fd_p  = (fd_t *)(elinkp->ram_virt_addr + fd_ofst);
	rbd_p = (rbd_t *)(elinkp->ram_virt_addr + rbd_ofst);

	elinkp->begin_fd = fd_p;
	elinkp->end_fd   = fd_p + num_fd - 1;

	elinkp->begin_rbd = rbd_p;
	elinkp->end_rbd   = rbd_p + num_rbd - 1;

	/*
	 * initialize all fds
	 */
	for (i = 0; i < num_fd; i++, fd_p++) {
		fd_p->fd_status = 0;
		fd_p->fd_cmd    = 0;
		fd_p->fd_nxt_ofst = fd_ofst + sizeof (fd_t);
		fd_p->fd_rbd_ofst = 0xffff;
		fd_ofst += sizeof (fd_t);
	}
	/* Initialise first fd's rbd */
	elinkp->begin_fd->fd_rbd_ofst = elinkp->rbd_ofst;

	(--fd_p)->fd_nxt_ofst = 0xffff;	/* Initialise last fd */
	fd_p->fd_cmd = CS_EL;

	/*
	 * Initialize all rbds
	 */
	for (i = 0; i < num_rbd; i++, rbd_p++) {
		rbd_p->rbd_status    = 0;
		rbd_ofst += sizeof (rbd_t);
		rbd_p->rbd_nxt_ofst = rbd_ofst;
		rbd_p->rbd_buff = rxb_ofst;
		rxb_ofst += RXB_SIZE;
		rbd_p->rbd_buff_base = 0;
		rbd_p->rbd_size = RXB_SIZE;
	}

	(--rbd_p)->rbd_nxt_ofst  = 0xffff;	/* Initialise last rbd */
	rbd_p->rbd_size |= CS_EL;
}

/*
 * Name		: elink_config()
 * Purpose	: Configure the 82586
 * Called from	: elink_init_board(), as part of board initialization
 *		  elink_prom(), when a switch to/from promiscuous mode
 *		  is desired
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 *		  flag    - indicating which of the modes are to be
 *			enabled (loopback, promiscuous mode or
 *			both). Possible values are
 *				LPBK_MASK : enable loopback mode
 *				PRM_MASK  : enable promiscuous mode
 *				LPBK_MASK | PRM_MASK : both
 *				0 : neither loopback nor promiscuous mode
 * Returns	: SUCCESS if the board was successfully configured to
 *			the desired configuration
 *		  FAILURE    otherwise
 * Side effects	: None
 */

static int
elink_config(gld_mac_info_t *macinfo, ushort prm_flag)
{
	struct elinkinstance *elinkp =		/* Our private device info */
				(struct elinkinstance *)macinfo->gldm_private;
	scb_t *scb_p = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);
	gencmd_t *cmd_p = (gencmd_t *)(elinkp->ram_virt_addr +
	    elinkp->gencmd_ofst);
	register ushort	io_base_addr = macinfo->gldm_port;

	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	if (elink_wait_active(scb_p, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	scb_p->scb_status   = 0;
	scb_p->scb_cmd = SCB_CUC_STRT;
	scb_p->scb_cbl_ofst = elinkp->gencmd_ofst;  /* ofst is general cmd */

	cmd_p->cmd_status = 0;
	cmd_p->cmd_cmd = CS_CMD_CONF | CS_EL;
	cmd_p->cmd_nxt_ofst = 0xffff;

	/*
	 * default config as in 586 manual
	 */
	bzero((caddr_t)&(cmd_p->cmd_prm.pr_conf), sizeof (conf_t));
	cmd_p->cmd_prm.pr_conf.word1.byte_count = 0xc;
	cmd_p->cmd_prm.pr_conf.word1.fifo_lim   = 0x8;

	cmd_p->cmd_prm.pr_conf.word2.addr_len = 6;
	cmd_p->cmd_prm.pr_conf.word2.pream_len = 2;
	cmd_p->cmd_prm.pr_conf.word2.ext_lpbk = prm_flag & LPBK_MASK ? 1 : 0;

	cmd_p->cmd_prm.pr_conf.word3.if_space = 0x60;

	cmd_p->cmd_prm.pr_conf.word4.slot_time = 0x200;
	cmd_p->cmd_prm.pr_conf.word4.retrylim = 0xf;

	cmd_p->cmd_prm.pr_conf.word5.prm = prm_flag & PRO_ON ? 1 : 0;

	cmd_p->cmd_prm.pr_conf.word6.min_frame_len = 0x40;

	outb(io_base_addr + IO_CHAN_ATN, 1);

	if (elink_wait_scb(scb_p, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	if (elink_ack(scb_p, io_base_addr))
		return (FAILURE);

	return (SUCCESS);
}

/*
 * Name		: elink_ack()
 * Purpose	: Acknowledge the interrupt sent by the 82586
 * Called from	: Any function that desires to acknowledge an interrupt
 * Arguments	: scb_p  - Pointer to the scb_t structure
 *		  io_base_address - base address of the board's I/O
 *                registers
 * Returns	: SUCCESS if the interrupt was successfully acknowledged
 *		  FAILURE otherwise
 * Side effects	: None
 */

static int
elink_ack(scb_t *scb_p, register ushort io_base_addr)
{
	if ((scb_p->scb_cmd = scb_p->scb_status & SCB_INT_MSK) != 0) {
		outb(io_base_addr + IO_CHAN_ATN, 1);	/* channel attention */
		return (elink_wait_scb(scb_p, 10 * MAX_SCB_WAIT_TIME));
	}
	return (SUCCESS);
}

/*
 * Name		: elink_wait_scb()
 * Purpose	: Detect if the 82586 has accepted the last command
 *                issued to it
 * Called from	: Any function that needs to issue a command to the
 *                82586 or to read the latest status of the 82586
 * Arguments	: scb_p - Pointer to the scb_t structure
 *		  count - time out limit for busy-wait
 * Returns	: SUCCESS if the 82586 had accepted the previous command
 *		  FAILURE    otherwise
 * Side effects	: None
 */

static int
elink_wait_scb(scb_t *scb_p, int count)
{
	int i = 0;			/* Scratch */

	while (i++ != count * 1000) {
		if (scb_p->scb_cmd == 0)
			return (SUCCESS);
	}
	return (FAILURE);
}
/*
 * Name		: elink_wait_active()
 * Purpose	: Checks whether the previous command is complete or not
 * Called from	: elink_send(), before giving the command
 * Arguments	: scb_p - Pointer to the scb_t structure
 *		  count - time out limit for busy-wait
 * Returns	: SUCCESS if the 82586 had completed the previous command
 *		  FAILURE    otherwise
 * Side effects	: None
 */

static int
elink_wait_active(scb_t *scb_p, int count)
{
	int i = 0;		   /* Scratch */

	while (i++ != count * 1000) {
		if (!(scb_p->scb_status & SCB_CUS_ACTV))
		return (SUCCESS);
	}
	return (FAILURE);
}

/*
 * Name			: elink_sig_chk()
 * Purpose		: Reads from CR_VB0 and checks whether the signature is
 *				  valid or not
 * Called from	: elink_probe(), elink_attach()
 * Arguments	: io_base_addr - The I/O base address of the board
 * Returns		: TRUE if the signature is valid
 *				  FALSE otherwise
 * Side effects	: None
 */

static int
elink_sig_chk(register ushort io_base_addr)
{
	int 	i;			   /* Scratch */

	for (i = 0; i < ETHERADDRL; i++)
		if ((char)inb(io_base_addr+i) != "*3COM*"[i])
			return (FALSE);

	return (TRUE);
}

/*
 * Name			: elink_mem_conf()
 * Purpose		: Generates a value for the particular pair of RAM base
 *				  and RAM size [ Table  of H/w specification]
 * Called from	: elink_attach()
 * Arguments	: start - The start of RAM base
 *				  size  - The size of RAM
 * Returns		: The generated value, if the start and size are proper
 *				  FAILURE otherwise
 * Side effects	: None
 */

static int
elink_mem_conf(wb, ws)
ulong wb; /* window base */
ulong ws; /* window size */
{
	int val = -1;

	if (wb < 0xc0000 || wb > 0xf80000 || !ws)
		return (FAILURE);

	/*
	 * Make sure that only window base addresses xx0x x000 0000 0000 0000
	 * are recognized (i.e. 0xc0000, 0xc8000, 0xd0000 and 0xd8000.
	 * Ensure that the window size is 16K, 32K, 48K or 64K
	 */

	if (wb <= 0xd8000 && !(wb & 0x27fff) && !(ws % 0x4000)) {
		/*
		 * special case for 0xd8000: only 16K and 32K are valid
		 */

		if (wb == 0xd8000 && ((ws >> 14) > 2))
			return (FAILURE);

		val = ((ws / 0x4000) - 1);

		if (wb & 0x8000)
			val |= 0x8;
		if ((wb & 0xd0000) == 0xd0000)
			val |= 0x10;
	} else if (wb >= 0xf00000 && !(wb % 0x20000) && (ws == 0x10000)) {
		/*
		 * spl. case for high end of memory
		 */

		if (wb == 0xf80000)
			return (0x38);
		val = 0x30;
		val |= ((wb & 0xf0000) / 0x20000);
	}
	return (val);
}

/*
 * Name			: elink_write_id_pat()
 * Purpose		: Changes the Adapter's state by Writing a sequence of
 *                255 bytes into the adapter's ID port.
 * Called from	: elink_probe()
 * Arguments	: None
 * Returns		: None
 * Side effects	: Adapter's IDSM state will get changed
 */

static void
elink_write_id_pat()
{
	unchar pat = 0xff;		/* Pattern to be written into ID port */
	unchar cnt = 0xff;		/* Counter */

	/*
	*  The Algorithm to generate a sequence of 255 bytes is given in the
	*  fig 2.2 of ref manual]
	*/
	while (cnt--) {
		outb(ELINK_ID_PORT, pat);

		if (pat & 0x80) {
			pat <<= 1;
			pat ^= 0xe7;
		} else
			pat <<= 1;
	}
}

#ifdef	ELINK_TDR_TEST
/*
 * Name			: elink_tdr_test()
 * Purpose		: Perform the TDR test to detect presence of any
 *				  cable/tranceiver faults
 * Called from	: elink_ioctl()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: Result of the TDR test outputted by 82586
 * Side effects	: None
 */

static int
elink_tdr_test(gld_mac_info_t *macinfo)
{
	struct elinkinstance *elinkp  =
	    (struct elinkinstance *)macinfo->gldm_private;
	scb_t	*scb = (scb_t *)(elinkp->ram_virt_addr + elinkp->scb_ofst);
	gencmd_t *cmd = (gencmd_t *)(elinkp->ram_virt_addr +
	    elinkp->gencmd_ofst);
	register ushort	io_base_addr = macinfo->gldm_port;
	ushort stat = 0;   /* scratch */
	int	i;

	if (elink_wait_scb(scb, MAX_SCB_WAIT_TIME) == FAILURE)
		return (FAILURE);

	scb->scb_status = 0;
	scb->scb_cmd = SCB_CUC_STRT;
	scb->scb_cbl_ofst = elinkp->gencmd_ofst;

	/*
	 * command: TDR test; CS_INT not set, since we have to busy_wait
	 */
	cmd->cmd_status = 0;
	cmd->cmd_cmd = CS_CMD_TDR | CS_EL;
	cmd->cmd_nxt_ofst = 0xffff;

	/* Go 82586! */
	outb(io_base_addr + IO_CHAN_ATN, 1);

	/*
	 * busy wait for the status word of command to be set to CS_CMPLT
	 */
	for (i = 0; i < 1000; i++) {
		/* check if command is completed */
		if (cmd->cmd_status & CS_CMPLT)
			break;
		drv_usecwait(100);
	}
	if (i < 1000) {
		if (cmd->cmd_status & CS_OK == 0) /* unsuccessful completion? */
			return (FAILURE);
		stat = cmd->cmd_prm.pr_tdr.tdr_status;
	}
	return (stat);
}
#endif	/* ELINK_TDR_TEST */
