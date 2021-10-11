/*
 * Copyright 1995 Sun Microsystems, Inc.
 * All rights reserved.
 */
#pragma ident "@(#)riles.c	1.2	95/01/06 SMI"

/*
 * NAME
 *		riles.c  
 *
 *
 * SYNOPIS
 *		Source code of the driver for the Racal Interlan ES-3210 Ethernet
 * Adapter on Solaris 2.4 (x86)
 *		Depends on the gld module of Solaris 2.4 (Generic LAN Driver)
 *
 * A> Exported functions.
 * (i) Entry points for the kernel-
 *		_init()
 *		_fini()
 *		_info()
 *
 * (ii) DDI entry points-
 *		riles_identify()
 *		riles_devinfo()
 *		riles_probe()
 *		riles_attach()
 *		riles_detach()
 *
 * (iii) Entry points for gld-
 *		riles_reset()
 *		riles_start_board()
 *		riles_stop_board()
 *		riles_saddr()
 *		riles_dlsdmult()
 *		riles_intr()
 *		riles_prom()
 *		riles_send()
 *
 * B> Imported functions.
 * From gld-
 *		gld_register()
 *		gld_unregister()
 *		gld_recv()
 *
 * 
 * DESCRIPTION
 *		The riles Ethernet driver is a multi-threaded, dynamically loadable,
 * gld-compliant, clonable STREAMS hardware driver that supports the
 * connectionless service mode of the Data Link Provider Interface,
 * dlpi (7) over an Racal Interlan ES-3210 controller. The driver
 * can support multiple ES-3210 controllers on the same system. It provides
 * basic support for the controller such as chip initialization,
 * frame transmission and reception, multicasting and promiscuous mode support,
 * and maintenance of error statistic counters.
 *		The riles driver uses the Generic LAN Driver (gld) module of Solaris,
 * which handles all the STREAMS and DLPI specific functions for the driver.
 * It is a style 2 DLPI driver and supports only the connectionless mode of
 * data transfer. Thus, a DLPI user should issue a DL_ATTACH_REQ primitive
 * to select the device to be used. Refer dlpi (7) for more information.
 *		For more details on how to configure the driver, refer riles (7).
 *
 *
 * CAVEATS AND NOTES
 *      The "riles" protected mode driver does *not* support I/O-mapped I/O.
 * Hence, the driver will not work with the default configuration of the
 * board (i.e. with shared memory disabled). To get around this, you should
 * explicitly choose a memory area for the board at configuration time.
 *      The driver does not use DMA channels 0-3, since it has been noticed
 * that 32-bit burst mode DMA transfers cannot be accomplished on these
 * channels. The driver forcibly uses memory-mapped i/o even when one of the
 * DMA channels 0-3 are configured.
 *		Maximum number of boards supported is equal to 0x10. It has been
 * observed that for a board placed in slot i, probe succeeds at both slots
 * i and (i + 16).
 *
 *
 * SEE ALSO
 *	/kernel/misc/gld
 *  riles (7)
 *  dlpi (7)
 *	"Skeleton Network Device Drivers",
 *		Solaris 2.1 Device Driver Writer's Guide-- February 1993
 *
 *
 * MODIFICATION HISTORY
 *  Version 1.1 10 Jun '94
 *   * Incoporated the riles_force_reset() function in the driver. The
 *     devo_reset function pointer is now initialized to riles_force_reset().
 *     This has been added to fix the system hang problem on a soft reset.
 *
 *	Version 1.2 6th Dec '94
 *	* Removed the message when driver is loaded and removed the Oring with
 *	  with 0x40 when configuring conf register 2 in riles_init_board().
 *
 *
 * MISCELLANEOUS
 * 		vi options for viewing this file::
 *				set ts=4 sw=4 ai wm=4
 *
 *   General variable naming conventions followed in the file:
 *   a) base_io_address always refers to the start address of I/O registers.
 *   b) rilesp is always a pointer to the driver private structure
 *      rilesinstance, defined in riles.h.
 *   c) macinfop is always a pointer to the driver private data structure.
 *   d) board_no is the instance number (interface number) of the driver.
 *
 *
 * COPYRIGHTS
 *		This file is a product of Sun Microsystems, Inc. and is provided
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

#ifndef lint
static char     sccsid[] = "@(#)gldconfig 1.1 93/02/12 Copyright 1993 Sun Microsystems";
#endif lint

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
#include <sys/ksynch.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/strsun.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/gld.h>
#include <sys/eisarom.h>
#include <sys/nvm.h>
#include <sys/dma_engine.h>
#include <sys/riles/riles.h>


/*
 * externally used functions in this file
 */

extern int    eisa_nvm(char *, int, int);
extern unchar inb(int);
extern void   outb(int, unchar);

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "Racal Interlan ES-3210 driver v1.1";
static unchar broadcast_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*
 * Used for debugging
 * Possible options are ::
 *     DEBUG_ERRS   :: enable err messages during receipt/send
 *     DEBUG_RECV   :: to trace receive routine
 *     DEBUG_DDI    :: to trace all DDI entry points
 *     DEBUG_SEND   :: to trace send routine
 *     DEBUG_INT    :: to trace interrupt service routine
 *     DEBUG_DMA    :: to trace DMA setup routine
 *     DEBUG_BOARD  :: to trace board programming functions
 *     DEBUG_NVM    :: to trace functions that access nvm
 *     DEBUG_MCAST  :: to trace multicast address addition & deletion
 *     DEBUG_WDOG   :: to debug calls to riles_watchdog()
 *     DEBUG_TRACE  :: to trace everything ("inclusive or" of all the above)
 */

#ifdef RILESDEBUG
static int	rilesdebug = 0;
#endif RILESDEBUG

/*
 * Required system entry points
 */

static	riles_identify(dev_info_t *);
static	riles_devinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	riles_probe(dev_info_t *);
static	riles_attach(dev_info_t *, ddi_attach_cmd_t);
static	riles_detach(dev_info_t *, ddi_detach_cmd_t);
static  riles_force_reset(dev_info_t *, ddi_reset_cmd_t);

/*
 * Required driver entry points for GLD
 */

static int	riles_reset(gld_mac_info_t *);
static int	riles_start_board(gld_mac_info_t *);
static int	riles_stop_board(gld_mac_info_t *);
static int	riles_saddr(gld_mac_info_t *);
static int	riles_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
static int	riles_prom(gld_mac_info_t *, int);
static int	riles_send(gld_mac_info_t *, mblk_t *);
static uint riles_intr(gld_mac_info_t *);

/*
 * prototype declarations for other functions
 */

static int riles_read_nvm_data(int, int *, int *, int *, int *);
static int riles_dma_setup(gld_mac_info_t *, caddr_t, uint, int);
static void riles_init_board(gld_mac_info_t *);
static void riles_rcv_packet(gld_mac_info_t *, int);
static void riles_watchdog(caddr_t);
static void riles_transmit(struct gld_mac_info *);

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/*
 * Standard Streams initialization
 */

static struct module_info minfo =
{
	RILESIDNUM,
	"riles",
	0,
	INFPSZ,
	RILESHIWAT,
	RILESLOWAT
};

/*
 * read queues for STREAMS
 */

static struct qinit rinit =
{
	NULL,
	gld_rsrv,
	gld_open,
	gld_close,
	NULL,
	&minfo,
	NULL
};

/*
 * write queues for STREAMS
 */

static struct qinit winit =
{
	gld_wput,
	gld_wsrv,
	NULL,
	NULL,
	NULL,
	&minfo,
	NULL
};

static struct streamtab rilesinfo =
{
	&rinit,
	&winit,
	NULL,
	NULL
};

/*
 * Standard Module linkage initialization for a Streams driver
 */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_rilesops =
{
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
	ddi_prop_op,	/* cb_prop_op */
	&rilesinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

static struct dev_ops rilesops =
{
	DEVO_REV,				/* devo_rev */
	0,						/* devo_refcnt */
	riles_devinfo,			/* devo_getinfo */
	riles_identify,			/* devo_identify */
	riles_probe,			/* devo_probe */
	riles_attach,			/* devo_attach */
	riles_detach,			/* devo_detach */
	riles_force_reset,		/* devo_reset */
	&cb_rilesops,			/* devo_cb_ops */
	(struct bus_ops *) NULL	/* devo_bus_ops */
};

static struct modldrv modldrv =
{
	&mod_driverops,		/* Type of module. This one is a driver */
	ident,				/* short description */
	&rilesops			/* driver specific ops */
};

static struct modlinkage modlinkage =
{
	MODREV_1,
	(void *) &modldrv,
	NULL
};

/*
 * globals for this module
 */

static int riles_brds_attached[MAX_RILES_BOARDS];
static char nvm_data[16 * 1024];


/*
 * Name			: _init()
 * Purpose		: Load an instance of the driver
 * Called from	: Kernel
 * Arguments	: None
 * Returns		: Whatever mod_install() returns
 * Side effects	: None
 */

int
_init(void)
{
	return (mod_install(&modlinkage));
}


/*
 * Name			: _fini()
 * Purpose		: Unload an instance of the driver
 * Called from	: Kernel
 * Arguments	: None
 * Returns		: Whatever mod_remove() returns
 * Side effects	: None
 */

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}


/*
 * Name			: _info()
 * Purpose		: Obtain status information about the driver
 * Called from	: Kernel
 * Arguments	: modinfop - pointer to a modinfo structure that 
 *                contains information on the module
 * Returns		: Whatever mod_info() returns
 * Side effects	: None
 */

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}


/*
 *						DDI ENTRY POINTS
 */


/*
 * Name			: riles_identify()
 * Purpose		: Determine if the driver drives the device specified 
 *                by the devinfop pointer
 * Called from	: Kernel
 * Arguments	: devinfop - pointer to a devinfo structure
 * Returns		: DDI_IDENTIFIED, if we know about this device
 *				  DDI_NOT_IDENTIFIED, otherwise
 * Side effects	: None
 */

static int
riles_identify(dev_info_t *devinfop)
{
	if (strcmp(ddi_get_name(devinfop), "riles") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}


/*
 * Name			: riles_devinfo()
 * Purpose		: Reports the instance number and identifies the devinfo
 *				  node with which the instance number is associated
 * Called from	: Kernel
 * Arguments	: devinfop - pointer to a devinfo_t structure
 *				  cmd     - command argument: either 
 *                          DDI_INFO_DEVT2DEVINFO or 
 *                          DDI_INFO_DEVT2INSTANCE
 *				  arg     - command specific argument
 *				  resultp - pointer to where requested information is 
 *                          stored
 * Returns		: DDI_SUCCESS, on success
 *				  DDI_FAILURE, on failure
 * Side effects	: None
 */

static int
riles_devinfo(dev_info_t *devinfop, ddi_info_cmd_t cmd, void *arg,
			  void **resultp)
{
	register int error;

	/*
	 * This code is not DDI compliant: the correct semantics
	 * for CLONE devices is not well-defined yet.
	 */

	switch (cmd)
	{
	case DDI_INFO_DEVT2DEVINFO:
		if (devinfop == NULL)
		{
			error = DDI_FAILURE;	/* Unfortunate */
		}
		else
		{
			*resultp = (void *) devinfop;
			error = DDI_SUCCESS;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*resultp = (void *) 0;	/* This CLONEDEV always returns zero */
		error    = DDI_SUCCESS;
		break;
	default:
		error = DDI_FAILURE;
	}

	return (error);
}


/*
 * Name			: riles_probe()
 * Purpose		: Determine if the network controller is present 
 *                on the system
 * Called from	: Kernel
 * Arguments	: devinfop - pointer to a devinfo structure
 * Returns		: DDI_PROBE_SUCCESS, if the controller is detected
 *				  DDI_PROBE_FAILURE, otherwise
 * Side effects	: None
 */

static int
riles_probe(dev_info_t *devinfop)
{
	register int base_io_address;
	int regbuf[3];              /* space for "ioaddr" property */
	int i;                      /* scratch */

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_DDI)
	{
		cmn_err(CE_CONT, "riles_probe(0x%x)", (int) devinfop);
	}
#endif RILESDEBUG

	/*
	 * Change this if it does not work. May have to give an EISA_SLOT
	 * command to eisa_nvm to find out the board_id of the board in such
	 * a case
	 */

	for (i = 0; i < MAX_RILES_BOARDS; i++)
	{
		/*
		 * base_io_address depends on the slot for all EISA boards
		 */

		base_io_address = 0x1000 * i;

		/*
		 * Make sure that there is no other ES-3210 board attached at this
		 * address
		 */

		if (riles_brds_attached[i] == TRUE)
			continue;

		/*
		 * check if the product_id is 0x2949 (signature of ES-3210)
		 */

		if ((inb(PRODUCT_ID1(base_io_address)) != 0x49) ||
			(inb(PRODUCT_ID2(base_io_address)) != 0x29))
		{
			continue;
		}
		else
		{

#ifdef RILESDEBUG
			if (rilesdebug & DEBUG_DDI)
			{
				cmn_err(CE_NOTE, "riles: ES-3210 board found at slot %d\n",
						i);
			}
#endif RILESDEBUG

			regbuf[0] = base_io_address;
			(void) ddi_prop_create(DDI_DEV_T_NONE, devinfop,
				   DDI_PROP_CANSLEEP, "ioaddr", (caddr_t) regbuf,
				   sizeof (int));
			return (DDI_PROBE_SUCCESS);
		}
	}

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_DDI)
	{
		cmn_err(CE_NOTE, "riles: No board found in probe()");
	}
#endif RILESDEBUG

	return (DDI_PROBE_FAILURE);
}


/*
 * Name			: riles_attach()
 * Purpose		: Attach a driver instance to the system. This 
 *                function is called once for each board successfully 
 *                probed.
 *				  gld_register() is called after macinfop's fields are
 *				  initialized, to register the driver with gld.
 * Called from	: Kernel
 * Arguments	: devinfop - pointer to the device's devinfo structure
 *				  cmd      - should be set to DDI_ATTACH
 * Returns		: DDI_SUCCESS on success
 *				  DDI_FAILURE on failure
 * Side effects	: macinfop is initialized by the driver before calling
 *                gld_register()
 */

static int
riles_attach(dev_info_t *devinfop, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfop;         /* GLD structure */
	register ushort base_io_address;  /* I/O address of the board */
	ulong ram_base_addr=0;            /* base address of DPRAM */
	ulong ram_size = 0;               /* size of DPRAM */
	caddr_t devaddr;				  /* Virtual address of RAM base */
	int conf_mem = 0; 				  /* Configured mem settings */
	int conf_irq = 0;				  /* Configured IRQ level */
	int conf_dma = -1;			      /* Configured DMA channel */
	int i, len = 0;                   /* length field used in ddi_getlongprop */
	int media_type = GLDM_UNKNOWN;    /* connector type */
	struct rilesinstance *rilesp;     /* Our private device info */
	struct reg_t
	{					  /* structure used to get REG property */
		int	bustype;
		int	base_addr;
		int	mem_size;
	} *busreg;
	struct intrprop
	{				  /* structure used to get IRQ property */
		int	spl;
		int	irq;
	} *intrprop;

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_DDI)
	{
		cmn_err(CE_CONT, "riles_attach(0x%x)", (int) devinfop);
	}
#endif RILESDEBUG

	if (cmd != DDI_ATTACH)
	{
		return (DDI_FAILURE);
	}

	/*
	 *  Allocate gld_mac_info_t and rilesinstance structures
	 */

	macinfop = (gld_mac_info_t *) kmem_zalloc(sizeof (gld_mac_info_t) +
					sizeof (struct rilesinstance), KM_NOSLEEP);
	if (macinfop == NULL)
	{
		cmn_err(CE_WARN,
			"riles%d: cannot allocate private data structure (attach failed)",
			ddi_get_instance(devinfop));
		return (DDI_FAILURE);
	}
	rilesp = (struct rilesinstance *) (macinfop + 1);

	/*
	 * Initialize our private fields in macinfop and rilesinstance
	 */

	macinfop->gldm_private = (caddr_t) rilesp;
	macinfop->gldm_port = base_io_address =
	    ddi_getprop(DDI_DEV_T_ANY, devinfop, DDI_PROP_DONTPASS, "ioaddr", 0);
	macinfop->gldm_state = RILES_IDLE;
	macinfop->gldm_flags = 0;

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer
	 */

	macinfop->gldm_reset   = riles_reset;
	macinfop->gldm_start   = riles_start_board;
	macinfop->gldm_stop    = riles_stop_board;
	macinfop->gldm_saddr   = riles_saddr;
	macinfop->gldm_sdmulti = riles_dlsdmult;
	macinfop->gldm_prom    = riles_prom;
	macinfop->gldm_gstat   = NULL; /* statistics are gathered on the fly */
	macinfop->gldm_send    = riles_send;
	macinfop->gldm_intr    = riles_intr;
	macinfop->gldm_ioctl   = NULL;

	/*
	 *  Initialize board characteristics needed by the generic layer
	 */

	/*
	 * Medium characteristics needed by gld; fill up the appropriate fields
	 * of gld
	 */

	macinfop->gldm_ident   = ident;       /* name of the board */
	macinfop->gldm_type    = DL_ETHER;    /* ethernet device */
	macinfop->gldm_minpkt  = 0;           /* assumes we pad ourselves */
	macinfop->gldm_maxpkt  = RILESMAXPKT; /* max size of ethernet pkt */
	macinfop->gldm_addrlen = ETHERADDRL;  /* ethernet address length */
	macinfop->gldm_saplen  = -2;          /* SAP length */

	/*
	 * Get the configured values of memory, irq, dma value and the
	 * media (connector) type by reading the EISA non-volatile memory
	 */

	if (riles_read_nvm_data(base_io_address, &conf_mem, &conf_irq,
		&media_type, &conf_dma) == FALSE)
	{
		cmn_err(CE_WARN,
		        "riles%d: cannot read EISA nvm for slot %d (attach failed)",
				ddi_get_instance(devinfop), (base_io_address / 0x1000));
		kmem_free((caddr_t) macinfop, sizeof (gld_mac_info_t) +
				  sizeof (struct rilesinstance));
		return (DDI_FAILURE);
	}

	rilesp->dma_channel = conf_dma;

	/*
	 * Get the gldm_irq_index value
	 */

	if ((i = ddi_getlongprop(DDI_DEV_T_ANY, devinfop, DDI_PROP_DONTPASS,
		 "intr", (caddr_t) &intrprop, &len)) != DDI_PROP_SUCCESS) 
	{
		cmn_err(CE_NOTE,
		    "riles%d: no intr property in riles.conf (attach failed)",
			ddi_get_instance(devinfop));
		kmem_free((caddr_t) macinfop, sizeof (gld_mac_info_t) +
				  sizeof (struct rilesinstance));
		return (DDI_FAILURE);
	}
	for (i = 0; i < (len / (sizeof(struct intrprop))); i++) 
	{
		if (conf_irq == intrprop[i].irq)
			break;
	}
	if (conf_irq != intrprop[i].irq)
	{
		cmn_err(CE_NOTE,
			"riles%d: no match for board irq in riles.conf (attach failed)",
			ddi_get_instance(devinfop));
		kmem_free((caddr_t)macinfop,
			sizeof (gld_mac_info_t) + sizeof (struct rilesinstance));
		kmem_free(intrprop, len);
		return (DDI_FAILURE);
	}
	macinfop->gldm_irq_index = i;
	kmem_free(intrprop, len);
	rilesp->irq_level = (ushort) conf_irq;

	/*
	 * Get the gldm_reg_index value
	 */

	if (ddi_getproplen(DDI_DEV_T_NONE, devinfop, DDI_PROP_DONTPASS, "reg",
					   &len) == DDI_PROP_SUCCESS)
	{
		int alloced = len;

		busreg = (struct reg_t *) kmem_alloc(len, KM_NOSLEEP);
		if (ddi_prop_op(DDI_DEV_T_NONE, devinfop, PROP_LEN_AND_VAL_BUF,
			DDI_PROP_DONTPASS, "reg", (caddr_t) busreg, &len)
			== DDI_PROP_SUCCESS) 
		{
			for (i = 0; i < (len / (sizeof (struct reg_t))); i++) 
			{
				ram_base_addr = busreg[i].base_addr;
				ram_size = busreg[i].mem_size;
				if (ram_size != SHARED_MEM_SIZE) /* size of DPRAM */
					continue;
				if (ram_base_addr == conf_mem)
					break;
			} 
		} 
		kmem_free(busreg, alloced);
	} 

	macinfop->gldm_reg_index = i;

	/*
	 * Map in the device memory so we can look at it
	 */

	if (ddi_map_regs(devinfop, i, &devaddr, 0, 0) != 0)
	{
		cmn_err(CE_NOTE,"riles: cannot map RAM address (attach failed)");
		kmem_free((caddr_t) macinfop,
			sizeof (gld_mac_info_t) + sizeof (struct rilesinstance));
		return (DDI_FAILURE);
	}
	rilesp->ram_phys_addr = conf_mem;
	rilesp->ram_virt_addr_start = devaddr;
	rilesp->ram_virt_addr_end = devaddr + SHARED_MEM_SIZE;

	/*
	 * Do anything necessary to prepare the board for operation
	 * short of actually starting the board.
	 */

	riles_init_board(macinfop);

	/*
	 * Get the board's vendor-assigned hardware network address
	 */

	outb(NIC_CR(base_io_address), SELECT_PAGE1);
	for (i = 0; i < ETHERADDRL; i++)
	{
		macinfop->gldm_vendor[i] = macinfop->gldm_macaddr[i] =
			  (unchar) inb(PROM_PAR0(base_io_address) + i);
	}
	outb(NIC_CR(base_io_address), SELECT_PAGE0);

	/*
	 * Set the connector/media type field in macinfop to what has been
	 * determined during riles_read_nvm_data()
	 */

	macinfop->gldm_media = media_type;

	bcopy((caddr_t) gldbroadcastaddr, (caddr_t) macinfop->gldm_broadcast,
		  ETHERADDRL);

	/*
	 * Make sure we have our address set :: program the NIC's physical
	 * address registers
	 */

	(void) riles_saddr(macinfop);

	rilesp->xmit_bufp = (caddr_t) kmem_alloc(XMIT_BUF_SIZE, KM_NOSLEEP);
	if (rilesp->xmit_bufp == NULL)
	{
		cmn_err(CE_NOTE,
			"riles: cannot allocate transmit buffer (attach failed)");
		kmem_free((caddr_t)macinfop, sizeof (gld_mac_info_t) +
				  sizeof (struct rilesinstance));
		return (DDI_FAILURE);
	}

	/*
	 *  Register ourselves with the GLD interface
	 *
	 *  gld_register will:
	 *	link us with the GLD system;
	 *	set our ddi_set_driver_private(9F) data to the macinfop pointer;
	 *	save the devinfop pointer in macinfop->gldm_devinfo;
	 *	map the registers, putting the kvaddr into macinfop->gldm_memp;
	 *	add the interrupt, putting the cookie in gldm_cookie;
	 *	init the gldm_intrlock mutex which will block that interrupt;
	 *	create the minor node.
	 */
	
	if (gld_register(devinfop, "riles", macinfop) != DDI_SUCCESS)
	{
		cmn_err(CE_NOTE,
			"riles: gld_register() unsuccessful (attach failed)");
		kmem_free((caddr_t) macinfop, sizeof (gld_mac_info_t) +
				  sizeof (struct rilesinstance));
		return (DDI_FAILURE);
	}

	/*
	 * Shared memory access test :: check if a read/write test succeeds
	 */

	for (i = 0; i < 16 * 1024; i++)
	{
		*(rilesp->ram_virt_addr_start + i) = 0xa;
		if (*(rilesp->ram_virt_addr_start + i) != 0xa)
		{
			cmn_err(CE_NOTE,
				"riles: r/w test failed for shared memory (attach failed)");
			return (DDI_FAILURE);
		}
	}

	/*
	 * Update entry in the riles_brds_attached table
	 */

	riles_brds_attached[(base_io_address / 0x1000)] = TRUE;

	rilesp->timeout_id =
		timeout(riles_watchdog, (caddr_t) macinfop, RILES_WDOG_TICKS);
	rilesp->riles_watch = 0;

#ifdef RILESDEBUG
	cmn_err(CE_CONT,
	  "\nriles driver(ver. 1.1) for Racal Interlan ES-3210 in slot %d loaded\n",
		base_io_address / 0x1000);
#endif
	return (DDI_SUCCESS);
}


/*
 * Name			: riles_detach()
 * Purpose		: Detach a driver instance from the system. This 
 *                includes unregistering the driver from gld
 * Called from	: Kernel
 * Arguments	: devinfop - pointer to the device's dev_info structure
 *				  cmd      - type of detach, should be DDI_DETACH always
 * Returns		: DDI_SUCCESS if the state associated with the given 
 *                device was successfully removed
 *				  DDI_FAILURE otherwise
 * Side effects	: None
 */

static int
riles_detach(dev_info_t *devinfop, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfop;
	struct rilesinstance *rilesp;
	int base_io_address;

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_DDI)
	{
		cmn_err(CE_CONT, "riles_detach(0x%x)", (int) devinfop);
	}
#endif RILESDEBUG

	if (cmd != DDI_DETACH)
	{
		return (DDI_FAILURE);
	}

	/*
	 * Get the gld private (gld_mac_info_t) and the driver private
	 * data structures
	 */

	macinfop = (gld_mac_info_t *) ddi_get_driver_private(devinfop);
	rilesp = (struct rilesinstance *) (macinfop->gldm_private);
	base_io_address = macinfop->gldm_port;

	/*
	 * Undo all actions of riles_attach() ::
	 * free the temporary transmit buffer that was allocated in
	 * riles_attach()
	 */

	kmem_free(rilesp->xmit_bufp, XMIT_BUF_SIZE);

	/*
	 * cancel callbacks to riles_watchdog()
	 */
	
	if (rilesp->timeout_id >= 0)
	{
		if (untimeout(rilesp->timeout_id) == FAILURE)
		{
#ifdef RILESDEBUG
			int board_no = ddi_get_instance(devinfop);

			if (rilesdebug & DEBUG_WDOG)
			{
				cmn_err(CE_WARN,
			   		"riles%d: cannot cancel timeout (invalid id?)",
					board_no);
			}
#endif RILESDEBUG

		}
	}

	/*
	 * stop the board if it is running
	 */

	(void) riles_stop_board(macinfop);

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

	if (gld_unregister(macinfop) == DDI_SUCCESS)
	{
		/*
		 * - Release resources allocated in riles_attach()
		 * - Unset the entry in riles_brds_attached table
		 */

		kmem_free((caddr_t)macinfop,
			sizeof (gld_mac_info_t) + sizeof (struct rilesinstance));
		riles_brds_attached[(base_io_address / 0x1000)] = FALSE;
		return (DDI_SUCCESS);
	}

	return (DDI_FAILURE);
}



/*
 *							GLD ENTRY POINTS
 */


/*
 * Name			: riles_reset()
 * Purpose		: Reset the board to its initial state
 * Called from	: gld
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 * Returns		: TRUE, always
 * Side effects : All data structures and lists maintained by the 
 *                8390 are flushed
 */

static int
riles_reset(gld_mac_info_t *macinfop)
{
#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_BOARD)
	{
		cmn_err(CE_CONT, "riles_reset(0x%x)", (int) macinfop);
	}
#endif RILESDEBUG

	(void) riles_stop_board(macinfop);
	(void) riles_init_board(macinfop);
	(void) riles_saddr(macinfop);

	return (TRUE);
}


/*
 * Name			: riles_init_board()
 * Purpose		: Initialize the ES-3210 network board registers and
 *                configure the board. The receive unit is NOT enabled
 * Called from	: riles_attach()
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: Previous state of the 8390 and its data structures 
 *                are lost
 */

static void
riles_init_board(gld_mac_info_t *macinfop)
{
	register struct rilesinstance *rilesp =
		            (struct rilesinstance *)macinfop->gldm_private;
	register int base_io_address = macinfop->gldm_port;
	longword_union lwu;
	int    media_type;  /* connector type */
	unchar dmabit = (1 << rilesp->dma_channel); /* bit mask for DMA prog */
	unchar elcr_val;  /* value to be programmed in the EISA ELCR */
	int    slot = (base_io_address) / 0x1000;
	int    i;


	/*
	 * disable and enable the board;
	 * check if the board responds
	 */

    outb(EXBOCTL(base_io_address), IOCHKRST);
    outb(EXBOCTL(base_io_address), BOARD_ENABLE);
    for (i = 100; i; i--)
	{
		if (inb(EXBOCTL(base_io_address)) & BOARD_ENABLE)
			break;
		drv_usecwait(10);
    }
    if (!i)
	{
		cmn_err(CE_NOTE, "riles: unable to reset board in slot %d",
				(base_io_address) / 0x1000);
		return;
    }

	/*
	 * Board specific configuration ::
	 * - Set interrupt level, network type (thick/thin), and enable shared
	 *   memory in configuration registers 1 and 2.
	 * - Establish the shared memory address of the board by writing to the
	 *   configuration registers 3 and 4.
	 * - Inform the board of the DMA channel being used by writing to the
	 *   configuration register 5.
	 */

	media_type = (macinfop->gldm_media == GLDM_AUI) ? 1 : 0;

	/*
	 * Valid levels at which the board can interrupt are
	 * 3, 4, 5, 6, 7, 9, 10, 11, 12, 14 and 15
	 */

    if ((rilesp->irq_level < 3) || (rilesp->irq_level == 8) ||
		(rilesp->irq_level == 13) || (rilesp->irq_level > 15))
	{
		cmn_err(CE_NOTE,
				"riles: illegal irq value(%d) for board in slot %d", 
				rilesp->irq_level, slot);
		return;
    }
    if (rilesp->irq_level < 11)
	{
		unchar conf_byte = media_type | (1 <<
					(rilesp->irq_level - (rilesp->irq_level < 8 ? 2 : 3)));

		outb(CONF_REG1(base_io_address), conf_byte);
		outb(CONF_REG2(base_io_address),  SHARED_MEM_ENAB);
		
    }
	else
	{
		outb(CONF_REG1(base_io_address), media_type);
		outb(CONF_REG2(base_io_address), SHARED_MEM_ENAB |
			 1 << (rilesp->irq_level - (rilesp->irq_level < 13 ? 11 : 12)));
    }

	/*
	 * Establish shared memory address of board
	 */

    lwu.word[0] = (ushort)(rilesp->ram_phys_addr >> 14);
    outb(CONF_REG3(base_io_address), lwu.byte[0] );
    lwu.byte[2] = ~lwu.byte[1] & 0xfc;
    outb(CONF_REG4(base_io_address), (lwu.byte[1] & 3) |
		 (lwu.byte[2] & 0xfc));

	/*
	 * Establish DMA channel for the board 
	 */

    if (rilesp->dma_channel > 7 || rilesp->dma_channel == 4 )
	{
		cmn_err(CE_NOTE, "riles: illegal dma value(%d) for board in slot #%d", 
				rilesp->dma_channel, slot);
		return;
    }
    if (rilesp->dma_channel >= 0)
	{	
		/*
		 * Inform the board about the DMA channel being used
		 */

		outb(CONF_REG5(base_io_address), dmabit);

#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_DMA)
		{
			cmn_err(CE_NOTE, "riles: using channel %d for DMA transfers",
					rilesp->dma_channel);
		}
#endif RILESDEBUG
	}

	/*
	 * Set up the receive buffer ring in DPRAM and set aside the first 
	 * NUM_XMIT_BUFS in the shared memory for the transmit area
	 */

	outb(RAWAR(base_io_address), NUM_XMIT_BUFS);
	outb(RARC(base_io_address), NUM_XMIT_BUFS);
	outb(TARC(base_io_address), 0);

	/*
	 * program the EISA edge/level control registers for level-triggered
	 * interrupts
	 */

	if (rilesp->irq_level < 8)
	{
		elcr_val = inb(EISA_ELCR0) | (1 << rilesp->irq_level);
		outb(EISA_ELCR0, elcr_val);
	}
	else
	{
		elcr_val = inb(EISA_ELCR1) | (1 << (rilesp->irq_level - 8));
		outb(EISA_ELCR1, elcr_val);
	}

	/*
	 * NIC specific initializatons
	 */

	/*
	 * stop the NIC
	 */

    outb(NIC_CR(base_io_address), (SELECT_PAGE0 | CR_RD_ABOR | CR_STP));

    for (i = 100; i; i--)
	{
		if (inb(NIC_ISR(base_io_address)) & ISR_RST)
			break;
    }

    if (!i && !(inb(NIC_ISR(base_io_address)) & ISR_RST))
	{
		cmn_err(CE_NOTE, "riles: NIC initialization failed");
		return;
    }

	/*
	 * mask all interrupts and acknowledge all the pending ones before 
	 * enabling the relevant ones
	 */

    outb(NIC_IMR(base_io_address), 0);
    outb(NIC_ISR(base_io_address), 0xFF);

	/*
	 * default configuration ::
	 * DCR_LAS :: Single 32-bit DMA mode
	 * DCR_LS  :: No loopback (normal)
	 * DCR_FT0 :: 8 bytes FIFO threshold
	 */

    outb(NIC_DCR(base_io_address), (DCR_FT0 | DCR_LS | DCR_WTS));
    outb(NIC_RBCR0(base_io_address), 0); /* remote byte (low) cnt :: 0 */
    outb(NIC_RBCR1(base_io_address), 0); /* remote byte (hi) cnt :: 0 */

    /*
	 * receiver configuration default :: enable broadcast and disable
	 * multicast address reception
	 */

	rilesp->nic_rcr = RCR_BCAST;

    outb(NIC_RCR(base_io_address), rilesp->nic_rcr);
    outb(NIC_TCR(base_io_address), 0);	/* transmit configuration - normal */

	/*
	 * Indicate the limits of the receive buffer ring to the NIC
	 * (set apart NUM_XMIT_BUFS for transmission and allocate the rest for
	 * the receive buffer).
	 * Store a soft copy of the NIC boundary pointer in rilesp->nic_bndry.
	 */

    outb(NIC_PSTART(base_io_address), NUM_XMIT_BUFS);
    outb(NIC_PSTOP(base_io_address), RILES_NUM_PAGES);
    rilesp->nic_bndry = NUM_XMIT_BUFS;
    outb(NIC_BNDRY(base_io_address), RILES_NUM_PAGES - 1);
    outb(NIC_CR(base_io_address), SELECT_PAGE1);
    outb(NIC_CURR(base_io_address), NUM_XMIT_BUFS);
    outb(NIC_CR(base_io_address), SELECT_PAGE0);

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_BOARD)
	{
		cmn_err(CE_NOTE,
			"riles: initialization successsful for board in slot %d", slot);
	}
#endif RILESDEBUG

    return;
}

/*
 * Name			: riles_start_board()
 * Purpose		: Start the device's receiver and enable interrupts
 * Called from	: gld
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 * Returns		: TRUE on success
 *				  FALSE on failure
 * Side effects	: Receiver unit of the 8390 and interrupts get enabled
 */

static int
riles_start_board(gld_mac_info_t *macinfop)
{
	register struct rilesinstance *rilesp =
		            (struct rilesinstance *) macinfop->gldm_private;
	register int base_io_address = macinfop->gldm_port;
	int      slot = (base_io_address) / 0x1000;
	int      i;

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_BOARD)
		cmn_err(CE_CONT, "riles_start_board(0x%x)", (int) macinfop);
#endif RILESDEBUG


	/*
	 * Put the NIC in start mode 
	 */

    outb(NIC_CR(base_io_address), (SELECT_PAGE0 | CR_STA));
    for (i = 100; i; i--)
	{
		if (inb(NIC_CR(base_io_address)) & CR_STA)
			break;
		drv_usecwait(10);
    }
    if (!i)
	{
		cmn_err(CE_NOTE, "riles: cannot start NIC of board in slot %d",
				slot);
		return (FAILURE);
    }

    outb(NIC_ISR(base_io_address), 0x7f);   /* acknowledge all interrupts */

	/*
	 * Enable interrupts for the following events ::
	 * packet transmitted (IMR_PTXE)
	 * packet received (IMR_PRXE)
	 * receive error (IMR_RXEE)
	 * transmit error (IMR_TXEE)
	 * overwrite warning enable (IMR_OVWE)
	 * counter overflow enable (IMR_CNTE)
	 */

    outb(NIC_IMR(base_io_address), (IMR_PRXE | IMR_PTXE | IMR_OVWE | IMR_TXEE));

	rilesp->riles_flags |= RILES_RUNNING;
    return (SUCCESS);
}


/*
 * Name			: riles_stop_board()
 * Purpose		: Stop the device's receiver and disables interrupts
 * Called from	: gld
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 * Returns		: TRUE on success
 *				  FALSE on failure
 * Side effects	: Receiver unit of the 8390 and interrupts are disabled
 */

static int
riles_stop_board(gld_mac_info_t *macinfop)
{
	register struct rilesinstance *rilesp =
		            (struct rilesinstance *)macinfop->gldm_private;
	register int base_io_address = macinfop->gldm_port;
	int      i = 100;


#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_BOARD)
		cmn_err(CE_CONT, "riles_stop_board(0x%x)", (int) macinfop);
#endif RILESDEBUG

	/*
	 * stop the NIC if it is not already in STOP mode
	 */

    if (!(rilesp->riles_flags & RILES_RUNNING))
		return (TRUE);

    outb(NIC_CR(base_io_address), (SELECT_PAGE0 | CR_RD_ABOR | CR_STP));
    while (i--)
    {
		if (inb(NIC_ISR(base_io_address)) & ISR_RST)
			break;
		drv_usecwait(10);
    }
    if (!i && !(inb(NIC_ISR(base_io_address)) & ISR_RST))
    {
		cmn_err(CE_NOTE, "riles: cannot stop board in slot %d",
				(base_io_address / 0x1000));
		return (FALSE);
	}

	/*
	 * Disable all interrupts and acknowledge the pending ones
	 * before disabling the board
	 */

    outb(NIC_IMR(base_io_address), 0);
    outb(NIC_ISR(base_io_address), 0xFF);
    outb(EXBOCTL(base_io_address), IOCHKRST);
 
#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_BOARD)
	{
		cmn_err(CE_NOTE, "riles: stop board succeeded for board in slot %d",
				(base_io_address) / 0x1000);
	}
#endif RILESDEBUG

	rilesp->riles_flags &= ~RILES_RUNNING;
	return (TRUE);

}

/*
 * Name			: riles_saddr()
 * Purpose		: Set the physical network address on the board
 * Called from	: gld
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 * Returns		: SUCCESS on success
 *				  FAIL    on failure
 * Side effects	: None
 */

static int
riles_saddr(gld_mac_info_t *macinfop)
{
	register int base_io_address = macinfop->gldm_port;
	int      i;  /* scratch */

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_BOARD)
	{
		cmn_err(CE_CONT, "riles_saddr(0x%x)", (int) macinfop);
	}
#endif RILESDEBUG

	/*
	 * Copy the soft copy of the ethernet address to the physical address
	 * registers of the NIC; note that the physical address registers are in
	 * page 1
	 */

    outb(NIC_CR(base_io_address), SELECT_PAGE1);
	for (i = 5; i >= 0; i--)
	{
		outb(NIC_PAR0(base_io_address) + i, macinfop->gldm_macaddr[i]);
    }

    outb(NIC_CR(base_io_address), SELECT_PAGE0);
    return (SUCCESS);
}


/*
 * Name			: riles_dlsdmult()
 * Purpose		: Enable/disable device level reception of specific
 *				  multicast addresses
 * Called from	: gld
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 *				  mcast   - multicast address
 *				  op      - enable(1) / disable(0) flag
 * Returns		: TRUE   on success
 *				  FALSE  on failure
 * Side effects	: Chip is enabled/disabled to receive multicast adresses
 */

static int
riles_dlsdmult(gld_mac_info_t *macinfop, struct ether_addr *mcast, int op)
{
	register struct rilesinstance *rilesp =
		            (struct rilesinstance *) macinfop->gldm_private;
	register int base_io_address = macinfop->gldm_port;
	int      board_no = ddi_get_instance(macinfop->gldm_devinfo);
	int      i;

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_MCAST)
	{
		cmn_err(CE_CONT, "riles_dlsdmult(0x%x, %s)", (int) macinfop,
				op ? "ON" : "OFF");
	}
#endif RILESDEBUG

	/*
	 * enable or disable the multicast
	 */

	if (op)
	{
		/*
		 * add multicast address ::
		 * update driver private table
		 */

		if (rilesp->mcast_count == GLD_MAX_MULTICAST)
		{
			cmn_err(CE_NOTE, "riles%d: multicast table full", board_no);
			return (FALSE);
		}

		for (i = 0; i <  GLD_MAX_MULTICAST; i++)
		{
			if (rilesp->mcast_addr[i].byte[0] == 0)
			{
				/*
				 * free entry found
				 */

				bcopy((caddr_t) mcast->ether_addr_octet, 
					  (caddr_t)rilesp->mcast_addr[i].byte, ETHERADDRL);

#ifdef RILESDEBUG
				if (rilesdebug & DEBUG_MCAST)
				{
					cmn_err(CE_CONT, "Adding multicast address :: ");
					RILES_PRINT_EADDR(rilesp->mcast_addr[i].byte);
					cmn_err(CE_CONT, "\n");
				}
#endif RILESDEBUG

				rilesp->mcast_count++;
				rilesp->nic_rcr |= RCR_MCAST;
    			outb(NIC_CR(base_io_address), SELECT_PAGE0);
				outb(NIC_RCR(base_io_address), rilesp->nic_rcr);

				/*
				 * The NIC's multicast hashing algorithm is not perfect (see
				 * 8390 specs pg.1.26). So, we enable reception of all
				 * multicast addresses and check in the receive routine if
				 * the destination multicast address is intended for us.
				 */

    			outb(NIC_CR(base_io_address), SELECT_PAGE1);
				for (i = 0; i < 7; i++)
					outb(NIC_MAR0(base_io_address) + i, 0xff);
    			outb(NIC_CR(base_io_address), SELECT_PAGE0);
				break;
			}
		}
		if (i >= GLD_MAX_MULTICAST)
		{
			cmn_err(CE_NOTE, "riles%d: multicast table full", board_no);
			return (FALSE);
		}
	}
	else 
	{
		/* 
		 * Remove multicast address :: update driver private table
		 */

		for (i = 0; i <  GLD_MAX_MULTICAST; i++)
		{
			if (bcmp((caddr_t)mcast->ether_addr_octet, 
						(caddr_t)rilesp->mcast_addr[i].byte, 
						ETHERADDRL) == 0)
			{
				/*
				 * matching entry found - invalidate it
				 */

#ifdef RILESDEBUG
				if (rilesdebug & DEBUG_MCAST)
				{
					cmn_err(CE_CONT, "Removing multicast address :: ");
					RILES_PRINT_EADDR(rilesp->mcast_addr[i].byte);
					cmn_err(CE_CONT, "\n");
				}
#endif RILESDEBUG

				rilesp->mcast_addr[i].byte[0] = 0;
				rilesp->mcast_count--;
				break;
			}
		}
		if (i == GLD_MAX_MULTICAST)
		{
#ifdef RILESDEBUG
			if (rilesdebug & DEBUG_MCAST)
			{
				cmn_err(CE_WARN, "riles%d: no matching multicast entry found",
						board_no);
			}
#endif RILESDEBUG

			return (FALSE);
		}
		if (rilesp->mcast_count == 0)
		{
			rilesp->nic_rcr &= ~RCR_MCAST;
    		outb(NIC_CR(base_io_address), SELECT_PAGE0);
    		outb(NIC_RCR(base_io_address), rilesp->nic_rcr);
		}
	}

	return (TRUE);

}


/*
 * Name			: riles_prom()
 * Purpose		: Enable/disable physical level promiscuous mode
 * Called from	: gld
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 *                on      - when set to 1, prom mode is enabled
 *                        - when set to 0, prom mode is disabled
 * Returns		: SUCCESS always
 * Side effects	: Board gets thrown into (or returns from) promiscuous 
 *                mode
 */

static int
riles_prom(gld_mac_info_t *macinfop, int on)
{
	struct rilesinstance *rilesp =		/* Our private device info */
		(struct rilesinstance *)macinfop->gldm_private;
	int base_io_address = macinfop->gldm_port;

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_BOARD)
		cmn_err(CE_CONT, "riles_prom(0x%x, %s)", (int) macinfop,
				on ? "ON" : "OFF");
#endif RILESDEBUG

    outb(NIC_CR(base_io_address), SELECT_PAGE0);

	if (on)
	{
	    /*
	     * enable promiscuous mode
	     */

		rilesp->nic_rcr |= RCR_PROM;
	}
	else
	{
	    /*
	     * disable promiscuous mode
	     */

    	rilesp->nic_rcr &= ~RCR_PROM;
	}

    outb(NIC_RCR(base_io_address), rilesp->nic_rcr);
    return (SUCCESS);
}


/*
 * Name			: riles_intr()
 * Purpose		: Interrupt handler for the device
 * Called from	: gld
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 * Returns		: DDI_INTR_CLAIMED   if the interrupt was serviced
 *				  DDI_INTR_UNCLAIMED otherwise
 * Side effects	: Threads put to sleep in riles_send() are woken up when a
 *                transmit completed interrupt is received
 */

static uint
riles_intr(gld_mac_info_t *macinfop)
{
	struct rilesinstance *rilesp =
		(struct rilesinstance *)macinfop->gldm_private;
	register int base_io_address = macinfop->gldm_port;
	int      board_no = ddi_get_instance(macinfop->gldm_devinfo);
	register unchar isrval, save_status;
	int      errval;
	unchar   val;


#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_INT)
	{
		cmn_err(CE_CONT, "riles_intr(0x%x)", (int) macinfop);
	}
#endif RILESDEBUG

	if (!(rilesp->riles_flags & RILES_RUNNING))
	{
		return (DDI_INTR_UNCLAIMED);
	}
    save_status = (inb(NIC_CR(base_io_address)) & ~CR_TPX);
    outb(NIC_CR(base_io_address), SELECT_PAGE0);
    if (isrval = inb(NIC_ISR(base_io_address)))
	{
		/*
		 * acknowledge interrupt to the board
		 */

		outb(NIC_ISR(base_io_address), isrval);
		if (isrval & ISR_OVW)
		{

#ifdef RILESDEBUG
			if (rilesdebug & DEBUG_INT)
			{
				cmn_err(CE_NOTE, "riles%d: ring buffer overflow", board_no);
			}
#endif RILESDEBUG
   			macinfop->gldm_stats.glds_overflow++;
		}

		if (isrval & (ISR_PRX | ISR_OVW))
		{
			riles_rcv_packet(macinfop, isrval);
		}

		if (isrval & ISR_PTX)
		{
			/*
			 * currently we do not resend packets that could not
			 * be successfully transmitted
			 */

			rilesp->riles_flags &= ~RILES_XMTBUSY;
			val = inb(NIC_TSR(base_io_address));
			if ((val & TSR_CDH) && (val & TSR_CRS))
			{
				rilesp->riles_watch |= RILES_NOXVR;
				macinfop->gldm_stats.glds_nocarrier++;
			}
			if (val & TSR_COL)
			{
				macinfop->gldm_stats.glds_collisions += 
								inb(NIC_NCR(base_io_address));
			}
		}

		if (isrval & ISR_TXE)
		{
			if (!((errval = inb(NIC_TSR(base_io_address))) & TSR_PTX))
			{
				if (errval & TSR_FU)
				{
					macinfop->gldm_stats.glds_underflow++;
					macinfop->gldm_stats.glds_errxmt++;
				}
				if (errval & TSR_COL)
				{
					macinfop->gldm_stats.glds_collisions += 
								inb(NIC_NCR(base_io_address));
				}
				if (errval & TSR_ABT)
				{
					macinfop->gldm_stats.glds_errxmt++;
				}

				/*
				 * Retransmit the contents of the buffer when transmission
				 * was aborted
				 */

				if (errval & (TSR_FU | TSR_ABT))
				{
					if (++rilesp->retry_count > MAX_XMIT_RETRIES)
					{
#ifdef RILESDEBUG
						if (rilesdebug & DEBUG_INT)
						{
							cmn_err(CE_NOTE,
								"riles%d: transmit retry count exceeded",
									board_no);
						}
#endif RILESDEBUG
						rilesp->riles_flags &= ~RILES_XMTBUSY;
					}
					else
					{
						macinfop->gldm_stats.glds_xmtretry++;
#ifdef RILESDEBUG
						if (rilesdebug & DEBUG_INT)
						{
							cmn_err(CE_NOTE, "riles%d: retransmitting packet",
								board_no);
						}
#endif RILESDEBUG
						riles_transmit(macinfop);
					}
				}
			}
		}

		if (isrval & (ISR_RXE | ISR_CNT))
		{
			/*
			 * check for frame alignment errors (NIC_CNTR0)
			 */

			errval = inb(NIC_CNTR0(base_io_address));
			macinfop->gldm_stats.glds_frame += errval;
			macinfop->gldm_stats.glds_errrcv += errval;

			/*
			 * check for CRC errors (NIC_CNTR1)
			 */

			errval = inb(NIC_CNTR1(base_io_address));
			macinfop->gldm_stats.glds_crc += errval;
			macinfop->gldm_stats.glds_errrcv += errval;

			/*
			 * check for missed packets (NIC_CNTR2)
			 */

			errval = inb(NIC_CNTR2(base_io_address));
			macinfop->gldm_stats.glds_missed += errval;
			macinfop->gldm_stats.glds_errrcv += errval;

#ifdef RILESDEBUG
			if (rilesdebug & DEBUG_ERRS)
			{
				cmn_err(CE_NOTE, "riles%d: counter overflow or rcv err",
						board_no);
			}
#endif RILESDEBUG

		}
		if ((isrval & ISR_RST) && !(isrval & ISR_OVW))
		{
	    	cmn_err(CE_NOTE, "riles%d: serious NIC error (RST set)",
				    board_no);
		}
	}				
	else
	{
		return (DDI_INTR_UNCLAIMED);
	}

	outb(NIC_CR(base_io_address), save_status);

	macinfop->gldm_stats.glds_intr++;
	rilesp->riles_watch |= RILES_ACTIVE;
	return (DDI_INTR_CLAIMED);	/* indicate it was our interrupt */
}


/*
 * Name			: riles_send()
 * Purpose		: Prepare a packet for transmission on the network. This
 *                includes gathering the packet from different mblks,
 *                padding and verifying that the chip is not busy
 *                transmitting some other packet. Note that this 
 *                function returns even before transmission by the 
 *                8390 completes. Hence, return value of SUCCESS is 
 *                no guarantee that the packet was successfully 
 *                transmitted (that is, without errors during 
 *                transmission).
 * Called from	: gld, when a packet is ready to be transmitted
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 *				  mp      - pointer to an M_DATA message that contains 
 *                          the packet. The complete LLC header is 
 *                          contained in the message's first message 
 *                          block, and the remainder of the packet is 
 *                          contained within additional M_DATA message 
 *                          blocks linked to the first message block
 * Returns		: SUCCESS if a command was issued to the 8390 to 
 *                transmit a packet
 *				  RETRY   on failure so that gld may retry later
 * Side effects	: rilesp->riles_flags has the RILES_XMTBUSY flag set if
 *                the transmission attempt was successful. This would
 *                be reset later when a transmission completed interrupt
 *                is issued by the NIC.
 * Miscellaneous: This function is called by gld when a packet is ready to
 *                be transmitted. A pointer to an M_DATA message that
 *                contains the packet is passed to this routine. The complete
 *                LLC header is contained in the message's first message
 *                block, and the remainder of the packet is contained within
 *                additional M_DATA message blocks linked to the first
 *                message block. This routine should NOT free the packet.
 */

static int
riles_send(gld_mac_info_t *macinfop, mblk_t *mp)
{
	register struct rilesinstance *rilesp =
			        (struct rilesinstance *)macinfop->gldm_private;
	ushort   offset_tbuf = 0;
	ushort   xmit_size = 0;
	ushort   msg_size;
	mblk_t   *mp1;
	int      board_no = ddi_get_instance(macinfop->gldm_devinfo);

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_SEND)
	{
		cmn_err(CE_CONT, "riles_send(0x%x, 0x%x)", (int) macinfop,
				(int) mp);
	}
#endif RILESDEBUG

	if (rilesp->riles_flags & RILES_XMTBUSY)
	{
#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_SEND)
		{
			cmn_err(CE_NOTE, "riles%d: cannot transmit -- waiting",
					board_no); 
		}
#endif RILESDEBUG

		/*
		 * It has been obeserved that using a condition variable here\
		 * actually leads to a DECREASE in send performance by as much
		 * as 25%. So, we continue with the tim-tested method of returning
		 * RETRY to gld.
		 */

		macinfop->gldm_stats.glds_defer++;
		return (RETRY);
	}

	/*
	 * Form the ethernet header; remember that the 8390 is not intelligent
	 * enough to do insertion of the ethernet header automatically.
	 */

	rilesp->riles_flags |= RILES_XMTBUSY;

	/* 
	 * Copy the data to the transmit buffer by going through the mblks
	 */

	for (mp1 = mp; mp1 != NULL; mp1 = mp1->b_cont) 
	{
		msg_size = mp1->b_wptr - mp1->b_rptr;
		if (msg_size == 0)
			continue;

		/*
		 * Make sure that you do not overflow buffers!
		 */

		if (xmit_size + msg_size > XMIT_BUF_SIZE)
		{
			cmn_err(CE_NOTE, "riles%d: abnormal sized packet; not sent",
					board_no);
			rilesp->riles_flags &= ~RILES_XMTBUSY;
			return (RETRY);
		}
		bcopy((caddr_t)(mp1->b_rptr), (caddr_t) (rilesp->xmit_bufp +
			  offset_tbuf), msg_size);
		offset_tbuf += msg_size;
		xmit_size   += msg_size;
	}

	/*
	 * Pad if necessary ::
	 * minimum size of an ethernet frame should be 64 bytes. So, the size
	 * of the frame that we copy into the DPRAM should be at least
	 *              64 - (frame_check_sequence) = 60 bytes
	 */

	if (xmit_size < MIN_FRAME_LEN)
	{
		xmit_size = MIN_FRAME_LEN;
	}

	rilesp->pkt_size    = xmit_size;
	rilesp->retry_count = 0;

	/*
	 * Transmit it on the net
	 */

	riles_transmit(macinfop);

	return (SUCCESS);		/* successful transmit attempt */
}


/*
 * Name			: riles_rcv_packet()
 * Purpose		: Get a packet that has been received by the hardware 
 *                and pass it up to gld
 * Called from	: riles_intr()  :: interrupt context
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: Soft copy of the NIC boundary pointer is updated to point
 *                to the next page where a new frame would be copied to
 */

static void
riles_rcv_packet(gld_mac_info_t *macinfop, int isrval)
{
	struct rilesinstance *rilesp =      /* Our private device info */
		(struct rilesinstance *)macinfop->gldm_private;
	register int base_io_address = macinfop->gldm_port;
	int      board_no = ddi_get_instance(macinfop->gldm_devinfo);
	caddr_t  rcv_buf;        /* receive buffer */
	int      len;            /* length of the packet */
	int      dma_done;       /* flag to detect successful DMA completion */
	int      i, cont, found; /* scratch */
	mblk_t   *mp;            /* scratch */
	longword_union *rbhdr;   /* header of received packet */

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_RECV)
	{
		cmn_err(CE_CONT, "riles_rcv_packet(0x%x)", (int) macinfop);
	}
#endif RILESDEBUG

	/*
	 * Start at the page indicated by the boundary pointer, 
	 * the virtual address of this page is computed to be
	 *     rilesp->ram_virt_addr_start + (riles->nic_bndry * 256) 
	 * Note that each frame received is prepended with a 4-byte header which
	 * has the following format:
	 * byte 0: status
	 * byte 1: page number where the next packet begins
	 * byte 2: length (lower byte)
	 * byte 3: length (higher byte)
	 */

	outb(NIC_CR(base_io_address), SELECT_PAGE1);

	/*
	 * repeat while our soft copy of the boundary pointer does not hit
	 * the NIC's boundary pointer
	 */

	while (rilesp->nic_bndry != inb( NIC_CURR(base_io_address))) 
	{
		cont = 0;
		dma_done = FAILURE;
		rcv_buf = rilesp->ram_virt_addr_start + rilesp->nic_bndry * 256;
		rbhdr = (longword_union *) rcv_buf;

		if (rilesp->nic_bndry >= RILES_NUM_PAGES)
		{
#if RILESDEBUG
			cmn_err(CE_NOTE,
				"riles%d: NIC boundary pointer corrupted (resetting)",
				board_no);
#endif RILESDEBUG
			rilesp->nic_bndry = inb( NIC_CURR(base_io_address));
			outb(NIC_CR(base_io_address), SELECT_PAGE0);

			outb(NIC_BNDRY(base_io_address), (rilesp->nic_bndry == NUM_XMIT_BUFS)? (RILES_NUM_PAGES - 1): (rilesp->nic_bndry - 1));

			return;
		}

		/*
		 * sanity check; make sure that the size of the packet is legal
		 * and other pointers in the header are legal
		 * This checks for runt-sized and giant-sized packets
		 */

		if ((rbhdr->byte[1] < NUM_XMIT_BUFS) ||
			(rbhdr->byte[1] > (RILES_NUM_PAGES - 1))){
#if RILESDEBUG
			cmn_err(CE_NOTE,
				"riles%d: NIC next packet pointer corrupted",
				board_no);
#endif RILESDEBUG

			rilesp->nic_bndry = inb( NIC_CURR(base_io_address));
			outb(NIC_CR(base_io_address), SELECT_PAGE0);
			outb(NIC_BNDRY(base_io_address), (rilesp->nic_bndry == NUM_XMIT_BUFS)? (RILES_NUM_PAGES - 1): (rilesp->nic_bndry - 1));
			macinfop->gldm_stats.glds_errrcv++;
			return;
		}

	
		if ((rbhdr->word[1] < 64) || (rbhdr->word[1] > 1518)) {
		    rilesp->nic_bndry = rbhdr->byte[1];

			/*
			 * Update number of runt packets received.
			 * No field in gld_stats for updating the number of giant
			 * packets received!
			 */

			if (rbhdr->word[1] < 64)
			{
				macinfop->gldm_stats.glds_short++;
			}
			macinfop->gldm_stats.glds_errrcv++;

#ifdef RILESDEBUG
			if (rilesdebug & DEBUG_RECV)
			{
				cmn_err(CE_WARN,
					"riles%d: abnormal size ", board_no);
			}
#endif RILESDEBUG

		    continue;
		} 

		/*
		 * verify that the frame was received without any errors
		 * by checking the status field (sanity check)
		 */

		if (!(rbhdr->byte[0] & 1))
		{
		    rilesp->nic_bndry = rbhdr->byte[1];
			macinfop->gldm_stats.glds_errrcv++;
			cmn_err(CE_WARN, "riles%d: packet received in error (dropped)",
					board_no);
		    continue;
		} 

		/*
		 * Do multicast processing if multicast or promiscuous mode reception
		 * is enabled in the receive configuration register
		 */

		if ((*(rcv_buf + 4) & 0x1) &&
			(rilesp->nic_rcr & (RCR_MCAST | RCR_PROM)))
		{
			if (bcmp((caddr_t) (rcv_buf + 4), (caddr_t) broadcast_addr,
				ETHERADDRL) != 0) 
			{
				found = FALSE;

				/*
				 * Check in the driver's table if the multicast address
				 * in the received packet is of interest to one of the upper
				 * streams
				 */

				for (i = 0; i < GLD_MAX_MULTICAST; i++)
				{
					if (bcmp((caddr_t)rcv_buf + 4, 
							 (caddr_t)rilesp->mcast_addr[i].byte, 
							 ETHERADDRL) == 0)
					{
						found = TRUE; /* matching entry found */
						macinfop->gldm_stats.glds_multircv++;
						break;
					}
				}
				if (found != TRUE)
				{
					rilesp->nic_bndry = rbhdr->byte[1];
					continue;
				}
			}
		}

		len = rbhdr->word[1];

		if ((mp = allocb(len, BPRI_HI)) == NULL) 
		{
		    macinfop->gldm_stats.glds_norcvbuf++;
			cmn_err(CE_WARN, "riles%d: no STREAMS buffers; dropping packet",
					board_no);
			rilesp->nic_bndry = rbhdr->byte[1];	/* drop a frame */
		    continue;
		}

		/*
		 * strip the header added by the 8390 NIC before copying it
		 */

		len = (rbhdr->word[1] - 4);
		rcv_buf += 4;
		if ((rcv_buf + len) > rilesp->ram_virt_addr_end) 
		{
			register int tmplen = rilesp->ram_virt_addr_end - rcv_buf;
			int      ret = SUCCESS;

#ifdef RILESDEBUG
			if (rilesdebug & DEBUG_RECV)
			{
				cmn_err(CE_NOTE, "riles%d: buffer wrap around", board_no);
			}
#endif RILESDEBUG

			if (tmplen > DMASZ && rilesp->dma_channel > 4)
			{
				ret = riles_dma_setup(macinfop, (caddr_t) mp->b_wptr,
							(uint) (len + 4), RILESRECV); 
			}
			if (tmplen <= DMASZ || ret == FAILURE || !(rilesp->dma_channel >4))
			{
				bcopy(rcv_buf, (caddr_t) mp->b_wptr, tmplen);
			}
			mp->b_wptr += tmplen;
			len -= tmplen;
		    rcv_buf = rilesp->ram_virt_addr_start + NUM_XMIT_BUFS * 256;
			cont = 1;
		}

		/*
		 * Resort to bcopy if DMA setup failed or if the size of data
		 * transfer is less than DMASZ bytes or if the configured DMA
		 * channel is less than 5 (currently the driver cannot perform
		 * DMA on channels 0-3.
		 */

		if (len > DMASZ && rilesp->dma_channel > 4)
		{
			/*
			 * DMA can only be done on a page boundary, so transfer the
			 * entire frame including the 4 bytes of header then advance
			 * b_rptr by 4
			 */

			if (!cont)
			{
				dma_done = riles_dma_setup(macinfop, (caddr_t) mp->b_wptr,
							(uint) (len + 4), RILESRECV); 
				mp->b_rptr += 4;  /* skip header */
				mp->b_wptr += 4;
			}
			else
			{
				dma_done = riles_dma_setup(macinfop, (caddr_t) mp->b_wptr,
								(uint) (len), RILESRECV); 
			}
		}
		if (dma_done != SUCCESS)
		{
			bcopy(rcv_buf, (caddr_t) mp->b_wptr, len);
		}
		mp->b_wptr += len;
		gld_recv(macinfop, mp);
		rilesp->nic_bndry = rbhdr->byte[1];
	}

	outb(NIC_CR(base_io_address), SELECT_PAGE0);
	outb(NIC_BNDRY(base_io_address), (rilesp->nic_bndry == NUM_XMIT_BUFS)? (RILES_NUM_PAGES - 1): (rilesp->nic_bndry - 1));

	return;
}


/*
 * Name			: riles_dma_setup()
 * Purpose		: Set up DMA to transfer a packet from board memory to host
 *                or viceversa
 * Called from	: riles_send()
 *                riles_rcv_packet()
 * Arguments	: macinfop - pointer to a gld_mac_info_t structure
 *                bufp     - pointer to a buffer to which DMA transfers
 *                           should be done
 *                len      - length of the buffer pointed by bufp
 *                flag     - flag that indicates whether DMA transfers
 *                           should be done from host to board or viceversa
 *                           Can be RILESSEND or RILESRECV.
 * Returns		: SUCCESS, if the transfer was successfuly completed
 *                FAILURE, otherwise
 * Side effects	: None
 */

static int
riles_dma_setup(gld_mac_info_t *macinfop, caddr_t bufp, uint len, int flag)
{
	struct rilesinstance *rilesp =
		(struct rilesinstance *)macinfop->gldm_private;
	register int base_io_address = macinfop->gldm_port;
	struct ddi_dmae_req dma_req; /* our DMA request structure */
	ddi_dma_handle_t    handle;
	ddi_dma_cookie_t    cookie;
	ddi_dma_win_t       win;     /* current window used for DMA xfers */
	ddi_dma_seg_t       seg;     /* current segment used for DMA xfers */
	dev_info_t          *dip = macinfop->gldm_devinfo;
	int                 ret;         /* scratch */
	int                 count = 0;   /* bytes left to transfer */
	off_t               off, dmalen; /* scratch */

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_DMA)
	{
		cmn_err(CE_CONT, "riles_dma_setup(0x%x)", (int) macinfop);
	}
#endif RILESDEBUG

	/*
	 * Set up DMA for use with virtual addresses
	 */

	if (flag == RILESSEND)
	{
		ret = ddi_dma_addr_setup(dip, NULL, bufp, len, DDI_DMA_WRITE,
						   DDI_DMA_DONTWAIT, NULL, NULL, &handle);
		dma_req.der_command = DMAE_CMD_WRITE;
	}
	else
	{
		ret = ddi_dma_addr_setup(dip, NULL, bufp, len, DDI_DMA_READ,
						   DDI_DMA_DONTWAIT, NULL, NULL, &handle);
		dma_req.der_command = DMAE_CMD_READ;
	}

	if (ret != DDI_SUCCESS)
	{
#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_DMA)
		{
			int board_no = ddi_get_instance(macinfop->gldm_devinfo);

			cmn_err(CE_NOTE, "riles%d: ddi_dma_addr_setup failed", board_no);
		}
#endif RILESDEBUG

		ddi_dma_free(handle);
		return (FAILURE);
	}

	/*
	 * Extract the cookie needed for DMA transfers :: since the packet
	 * size is never greater than 1500 bytes, everything is accomodated in
	 * one segment itself
	 */

	if (ddi_dma_nextwin(handle, NULL, &win) != DDI_SUCCESS)
	{
		ddi_dma_free(handle);

#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_DMA)
		{
			int board_no = ddi_get_instance(macinfop->gldm_devinfo);

			cmn_err(CE_NOTE, "riles%d: cannot get next window", board_no);
		}
#endif RILESDEBUG

		return (FAILURE);
	}
	if (ddi_dma_nextseg(win, NULL, &seg) != DDI_SUCCESS)
	{
		ddi_dma_free(handle);

#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_DMA)
		{
			int board_no = ddi_get_instance(macinfop->gldm_devinfo);

			cmn_err(CE_NOTE, "riles%d: cannot get next segment", board_no);
		}
#endif RILESDEBUG

		return (FAILURE);
	}
	if (ddi_dma_segtocookie(seg, &off, &dmalen, &cookie) != DDI_SUCCESS)
	{
		ddi_dma_free(handle);

#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_DMA)
		{
			int board_no = ddi_get_instance(macinfop->gldm_devinfo);

			cmn_err(CE_NOTE, "riles%d: cannot get cookie for segment",
			        board_no);
		}
#endif RILESDEBUG

		return (FAILURE);
	}
	if (dmalen != len)
	{
#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_DMA)
		{
			int board_no = ddi_get_instance(macinfop->gldm_devinfo);

			cmn_err(CE_NOTE, "riles%d: segment length != given length",
					board_no);
		}
#endif RILESDEBUG

		ddi_dma_free(handle);
		return (FAILURE);
	}

	/*
	 * Complete the rest of host dma programming ::
	 * Program the host dma engine after setting up the DMA engine request
	 * structure
	 */
	
	dma_req.der_bufprocess = 0;
	dma_req.der_trans  = DMAE_TRANS_BLCK; /* block tranfers */
	dma_req.der_cycles = DMAE_CYCLES_4;  /* board supports only type C DMA */
	dma_req.der_step   = DMAE_STEP_INC;  /* count up from the start addr */
	dma_req.der_path   = DMAE_PATH_32;  /* 4 byte bursts in each bus cycle */
	dma_req.proc       = NULL;
	dma_req.procparms  = NULL;

	if (ddi_dmae_prog(dip, &dma_req, &cookie, (int) rilesp->dma_channel) !=
		DDI_SUCCESS)
	{
		ddi_dmae_release(dip, rilesp->dma_channel);
		ddi_dma_free(handle);

#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_DMA)
		{
			int board_no = ddi_get_instance(macinfop->gldm_devinfo);

			cmn_err(CE_NOTE, "riles%d: ddi_dmae_prog() failed", board_no);
		}
#endif RILESDEBUG

		return (FAILURE);
	}

	/*
	 * Allocate the DMA channel to be used for transfers
	 */

	if (ddi_dmae_alloc(dip, (int) rilesp->dma_channel, DDI_DMA_DONTWAIT, NULL)
		!= DDI_SUCCESS)
	{
#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_DMA)
		{
			int board_no = ddi_get_instance(macinfop->gldm_devinfo);

			cmn_err(CE_WARN, "riles%d: ddi_dmae_alloc() failed", board_no);
		}
#endif RILESDEBUG

		ddi_dmae_release(dip, rilesp->dma_channel);
		ddi_dma_free(handle);
		return (FAILURE);
	}

	if (flag == RILESSEND)
	{
		outb(TARC(base_io_address), 0);
							  /* transmit buffers always begin at page 0 */
	}
	else
	{
		outb(RARC(base_io_address), rilesp->nic_bndry);
							  /* current value of NIC boundary ptr */
	}

	outb(RAWAR(base_io_address), NUM_XMIT_BUFS);

	/*
	 * Initialize the remote start address registers (not implemented)
	 */

	outb(START_DMA(base_io_address), (1 << rilesp->dma_channel) | 0x10);
											  /* Go, DMA controller */

	/* 
	 * Wait for DMA completion
	 */

	do 
	{
		ddi_dmae_getcnt(macinfop->gldm_devinfo, rilesp->dma_channel, &count);
	}
	while (count);

	ddi_dmae_stop(macinfop->gldm_devinfo, rilesp->dma_channel);

	
    /*
 	 * Release the DMA channel that was acquired earlier
 	 */

	ddi_dmae_release(dip, rilesp->dma_channel);

    /*
	 * Release all DMA resources
	 */

	ddi_dma_free(handle);

	return (SUCCESS);
}


/*
 * Name			: riles_read_nvm_data()
 * Purpose		: Get information from the non-volatile memory on the
 *                board
 * Called from	: riles_attach()
 * Arguments	: base_io_address - start of i/o address of the board
 *                conf_mem        - (configured) start of memory address 
 *                                  on the board
 *                conf_irq        - configured irq setting on the board
 *                media_type      - configured value of the type of media
 *                                  connector used
 *                conf_dma        - configured value of the dma channel
 *                                  used by the board
 * Returns		: TRUE if the parameters could be read from the nvm
 *                     (conf_mem, conf_irq, media_type and conf_dma are
 *                      updated on return)
 *                FALSE otherwise
 * Side effects	: None
 */

static int
riles_read_nvm_data(register int base_io_address, int *conf_mem,
					int *conf_irq, int *media_type, int *conf_dma)
{
	NVM_FUNCINFO *funcp;
	eisaslot *slotp;              /* slot information */
	longword_union dpram_addr;    /* address of DPRAM */
	int      slot_found = FALSE;  /* flag to detect slot of interest */
	int      len;                 /* length of the nvm_data returned by
								   * eisa_nvm()
								   */
	int      slot_io_addr;        /* slot i/o address of this slot */
	unchar   board_id[4];         /* board id */
	char     *nvmp;               /* scratch */
	char     *cp;                 /* scratch */
	int      i;                   /* scratch */

	/*
	 * Read the board's id by calling eisa_nvm()
	 */

	len = eisa_nvm(nvm_data, EISA_SLOT, (base_io_address) / 0x1000);
	if (len)
	{
		slotp = (eisaslot *) nvm_data;
		for (i = 0; i < 4; i++)
		{
			board_id[i] = slotp->info.boardid[i];
		}
	}

	/*
	 * Get all the initialization and other board information by reading the
	 * EISA non volatile memory
	 */

	len = eisa_nvm(nvm_data, EISA_BOARD_ID, *((int *) board_id));
	slotp = (eisaslot *) nvm_data;

	for (nvmp = nvm_data; nvmp < nvm_data + len;
				 nvmp = nvmp + sizeof(eisaslot) +
				(slotp->info.functions * sizeof (NVM_FUNCINFO)))
	{
		slotp = (eisaslot *) nvmp;
		slot_io_addr = (slotp->num) * 0x1000;

		/*
		 * Check if the slot under consideration is the one that we need
		 */

		if (slot_io_addr != base_io_address)
		{
#ifdef RILESDEBUG
			if (rilesdebug & DEBUG_NVM)
			{
				cmn_err(CE_WARN,
				 "riles: slot_io_addr 0x%x != base_io_addr 0x%x; rejecting",
				 slot_io_addr, base_io_address);
			}
#endif RILESDEBUG

			continue;
		}

		slot_found = TRUE;
		dpram_addr.lw = 0;

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_NVM)
	{
		cmn_err(CE_CONT, "riles: no. of functions: %d\n",
				slotp->info.functions);
	}
#endif RILESDEBUG

		for (i = 0; i < slotp->info.functions; i++)
		{
			funcp = (NVM_FUNCINFO *) ((char *) slotp + sizeof (eisaslot) +
					(i * sizeof (NVM_FUNCINFO)));
			if (funcp->fib.init)
			{
				/*
				 * Get the connector type and detect if shared memory is
				 * enabled
				 */

				cp = (char *) &(((NVM_INIT *) funcp->un.r.init)->un.byte_v);
				*media_type = (*cp & (char) 1) ? GLDM_AUI : GLDM_TP;

#ifdef RILESDEBUG
				if (rilesdebug & DEBUG_NVM)
				{
					char mem_enab;     /* shared memory enabled flag */

					mem_enab = (*(cp + 4) & 0x80) ? 1 : 0;
					cmn_err(CE_NOTE,
					   "riles: shared memory enabled: %d", mem_enab);
				}
#endif RILESDEBUG

			}
			if (funcp->fib.memory)
			{
				/*
				 * Configured value of shared memory
				 */

				dpram_addr.lw = 0;
				dpram_addr.byte[1] = funcp->un.r.memory[0].start[0];
				dpram_addr.byte[2] = funcp->un.r.memory[0].start[1];
				dpram_addr.byte[3] = funcp->un.r.memory[0].start[2];
				*conf_mem = dpram_addr.lw;

#ifdef RILESDEBUG
				if (rilesdebug & DEBUG_NVM)
				{
					cmn_err(CE_NOTE,
						"riles: mem size 0x%x K, mem start in 0x%x",
						funcp->un.r.memory[0].size, *conf_mem);
				}
#endif RILESDEBUG

			}
			if (funcp->fib.irq)
			{
				/*
				 * Configured irq value
				 */

				*conf_irq = funcp->un.r.irq[0].line;

#ifdef RILESDEBUG
				if (rilesdebug & DEBUG_NVM)
				{
					cmn_err(CE_NOTE, "riles: Configured irq value: 0x%x\n",
					        *conf_irq);
				}
#endif RILESDEBUG

			}
			if (funcp->fib.dma)
			{
				/*
				 * Configured value of DMA
				 */

				*conf_dma = funcp->un.r.dma[0].channel;

#ifdef RILESDEBUG
				if (rilesdebug & DEBUG_NVM)
				{
					cmn_err(CE_NOTE, "riles: Configured dma value: 0x%x\n",
					        *conf_dma);
				}
#endif RILESDEBUG

			}
		}
		break;
	}

	return (slot_found);
}


/*
 * Name			: riles_watchdog()
 * Purpose		: Watchdog routine to handle board idiosyncrasies and
 *                dys/mal-functionality
 * Called from	: Kernel
 * Arguments	: ptr - a caddr_t pointer to be interpreted as a pointer
 *                      to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: riles_watch is reset to 0 on return.
 *                timeout_id is updated to have the new return value from
 *                timeout().
 *                Board is reset if it is detected to be dys/mal-functional.
 */

static void
riles_watchdog(caddr_t ptr)
{
	gld_mac_info_t *macinfop = (gld_mac_info_t *) ptr;
	struct rilesinstance *rilesp;
	int    board_no;
	int    i;

#if RILESDEBUG
	if (rilesdebug & DEBUG_WDOG)
	{
		cmn_err(CE_NOTE, "riles: watchdog() entered");
	}
#endif RILESDEBUG

	if (macinfop == NULL || macinfop->gldm_private == NULL)
	{
		cmn_err(CE_NOTE,
				"riles: fatal error in watchdog()- macinfop NULL");
		return;
	}
	mutex_enter(&macinfop->gldm_maclock);
	rilesp = (struct rilesinstance *) macinfop->gldm_private;

	/*
	 * Display cable related errors if necessary
	 */

	if (rilesp->riles_watch & RILES_NOXVR)
	{
		board_no = ddi_get_instance(macinfop->gldm_devinfo);

		cmn_err(CE_CONT,
		    "riles%d: lost carrier (cable or transceiver problem?)\n",
		    board_no);

		/*
		 * restart timeouts again and release the mutex
		 */

		rilesp->timeout_id =
			timeout(riles_watchdog, (caddr_t) macinfop, RILES_WDOG_TICKS);
		rilesp->riles_watch = 0;
		mutex_exit(&macinfop->gldm_maclock);
		return;
	}

	/*
	 * Check if the board is active ince the last call to this function
	 */

	if (!(rilesp->riles_watch & RILES_ACTIVE) &&
		rilesp->riles_flags & RILES_RUNNING)
	{
		int base_io_address;

		/*
		 * start the board again and check if it responds
		 */

#ifdef RILESDEBUG
		if (rilesdebug & DEBUG_WDOG)
		{
			board_no = ddi_get_instance(macinfop->gldm_devinfo);
			cmn_err(CE_WARN, "riles%d: restarting board in riles_watchdog()",
					board_no);
		}
#endif RILESDEBUG

		base_io_address = macinfop->gldm_port;
		outb(NIC_CR(base_io_address), (SELECT_PAGE0 | CR_STA));
		for (i = 100; i; i--)
		{
			if (inb(NIC_CR(base_io_address)) & CR_STA)
				break;
			drv_usecwait(10);
		}
		if (!i)
		{
			/*
			 * Board failure! Give up and reset the board
			 */

			cmn_err(CE_NOTE, "riles%d: board in slot %d failed- resetting",
					ddi_get_instance(macinfop->gldm_devinfo),
					base_io_address / 0x1000);
			(void) riles_reset(macinfop);
		}
	}

	/*
	 * Restart timeouts again and release the mutex
	 */

	rilesp->timeout_id =
		timeout(riles_watchdog, (caddr_t) macinfop, RILES_WDOG_TICKS);
	rilesp->riles_watch = 0;
	mutex_exit(&macinfop->gldm_maclock);
	return;
}


/*
 * Name			: riles_transmit()
 * Purpose		: Transmit a packet on the network by doing the appropriate
 *                programming for the board and the NIC
 * Called from	: Kernel
 * Arguments	: macinfop   - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: None
 */

static void
riles_transmit(struct gld_mac_info *macinfop)
{
	register struct rilesinstance *rilesp =
		            (struct rilesinstance *) macinfop->gldm_private;
	register int base_io_address = macinfop->gldm_port;
	int      dma_done = FAILURE;    /* flag to detect DMA completion */
	longword_union lwu;

    lwu.word[0] = rilesp->pkt_size;
    if ((rilesp->pkt_size > DMASZ) && (rilesp->dma_channel > 4))
	{
		dma_done = riles_dma_setup(macinfop, rilesp->xmit_bufp,
								   rilesp->pkt_size, RILESSEND); 
	}

   /*
	* Resort to bcopy for small transfers (to avoid the DMA overheads)
	* or if DMA fails
	*/

	if (dma_done != SUCCESS)
	{
		bcopy(rilesp->xmit_bufp, rilesp->ram_virt_addr_start,
			  rilesp->pkt_size);
	}

    outb(NIC_CR(base_io_address), SELECT_PAGE0);
    outb(NIC_TPSR(base_io_address), 0);

	/* 
	 * Program the byte count registers and the NIC
	 */

    outb(NIC_TBCR1(base_io_address), lwu.byte[1]);
    outb(NIC_TBCR0(base_io_address), lwu.byte[0]);
    outb(NIC_CR(base_io_address), CR_TPX);	/* Go, 8390! */

}


/*
 * Name			: riles_force_reset()
 * Purpose		: Forces reset of the board when the system is to be shut
 *                down
 * Called from	: Kernel
 * Arguments	: devinfop - pointer to a dev_info_t structure
 *              : cmd      - argument whoe value should be DDI_RESET_FORCE
 * Returns		: DDI_SUCCESS on success
 *                DDI_FAILURE on failure
 * Side effects	: None
 */

static int
riles_force_reset(dev_info_t *devinfop, ddi_reset_cmd_t cmd)
{
	gld_mac_info_t *macinfop;

#ifdef RILESDEBUG
	if (rilesdebug & DEBUG_DDI)
	{
		cmn_err(CE_CONT, "riles_force_reset(0x%x)", (int) devinfop);
	}
#endif RILESDEBUG

	if (cmd != DDI_RESET_FORCE)
	{
		return (DDI_FAILURE);
	}

	/*
	 * Get the gld private (gld_mac_info_t) and the driver private
	 * data structures
	 */

	macinfop = (gld_mac_info_t *) ddi_get_driver_private(devinfop);

	/*
	 * stop the board if it is running
	 */

	(void) riles_stop_board(macinfop);

	return (DDI_SUCCESS);
}
