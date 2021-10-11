/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma indent	"@(#)iee.c	1.8	94/10/26 SMI"

/*
 * NAME
 *		iee.c ver. 2.2
 *
 *
 * SYNOPIS
 *		Source code of the driver for the Intel EtherExpress-16 Ethernet
 * LAN adapter board on Solaris 2.1 (x86)
 *		Depends on the gld module of Solaris 2.1 (Generic LAN Driver)
 *
 * A> Exported functions.
 * (i) Entry points for the kernel-
 *		_init()
 *		_fini()
 *		_info()
 *
 * (ii) DDI entry points-
 *		iee_identify()
 *		iee_devinfo()
 *		iee_probe()
 *		iee_attach()
 *		iee_detach()
 *
 * (iii) Entry points for gld-
 *		iee_reset()
 *		iee_start_board()
 *		iee_stop_board()
 *		iee_saddr()
 *		iee_dlsdmult()
 *		iee_intr()
 *		iee_prom()
 *		iee_gstat()
 *		iee_send()
 *		iee_ioctl()
 *
 * B> Imported functions.
 * From gld-
 *		gld_register()
 *		gld_unregister()
 *		gld_recv()
 *
 * 
 * DESCRIPTION
 *		The iee Ethernet driver is a multi-threaded, dynamically loadable,
 * gld-compliant, clonable STREAMS hardware driver that supports the
 * connectionless service mode of the Data Link Provider Interface,
 * dlpi (7) over an Intel EtherExpress 16 (IEE) controller. The driver
 * can support multiple IEE controllers on the same system. It provides
 * basic support for the controller such as chip initialization,
 * frame transmission and reception, multicasting and promiscuous mode support,
 * maintenance of error statistic counters and the time domain reflectometry
 * tests.
 *	The iee driver uses the Generic LAN Driver (gld) module of Solaris,
 * which handles all the STREAMS and DLPI specific functions for the driver.
 * It is a style 2 DLPI driver and supports only the connectionless mode of
 * data transfer. Thus, a DLPI user should issue a DL_ATTACH_REQ primitive
 * to select the device to be used. Refer dlpi (7) for more information.
 *		For more details on how to configure the driver, refer iee (7).
 *
 *
 * CAVEATS
 *		Currently, the driver recognizes only the first value in each
 * property of the .conf file. For example, if the "intr" property has
 * a set of values, then the interrupt vector that is added corresponds
 * to the first one. This limitation is imposed by gld during
 * gld_register() where it calls ddi_add_intr().
 *
 *
 * NOTES
 *		Maximum number of boards supported is 0x20: hopefully, a
 * system administrator will run out of slots if he wishes to add more
 * than 0x20 boards to the system
 * 		Command chaining feature of the 82586 has not been exploited
 * in this version. 
 * 		"Dump" and "diagnose" commands of the 82586 are not supported.
 * 		"regs" property of the .conf file is unused, since the board
 * is I/O mapped.
 *		repoutsb() is being used in iee_send(), and not repoutsw()
 * as it was observed that the latter surprisingly gave a poorer
 * performance(!).
 *
 *
 * SEE ALSO
 *	/kernel/misc/gld
 *  iee (7)
 *  dlpi (7)
 *	"Skeleton Network Device Drivers",
 *		Solaris 2.1 Device Driver Writer's Guide-- February 1993
 *
 *
 * MODIFICATION HISTORY
 *      Interim version ver. 1.4 07/28/93.
 *			Prototype version, fully functional.
 *
 * Version 2.0 08/25/93 released on 3 Sep '93.
 *	* Code to read Ethernet address and irq level from EEPROM added.
 * 	* Checks for duplicate "ioaddr" entries in .conf file.
 *	* Bitmap structures used in iee_config().
 *	* Support added for multicast and changing physical address.
 *	* iee_saddr() changed: value of current Ethernet address is taken
 *	  from gldm_macaddr instead of gldm_vendor.
 *      
 * Version 2.1 09/16/93 released on 17 Sep '93
 *  Update for fixing the following bugs
 *       
 *  * Bug in iee_dlsdmult(): Since strncpy() was being used instead
 *    of bcopy(), a multicast address with one or more NULL bytes
 *    in it would result in a wrong multicast address being added.
 *    strncpy() and strncmp() have been replaced with bcopy()
 *    and bcmp() respectively, and tested.
 *  * In the irq_val array used in iee_attach(), the values for
 *    irq 9 and 11 were wrong (they were 5 and 2 instead of 6 and 1).
 *    This has been corrected, and tested.
 *
 * Version 2.1.1.1 10/18/93 (not released to the customer)
 *  Update for fixing the following bug reported by Sunsoft :
 *
 *  * Bugs in gathering statistics from the 82586 chip :: 
 *    - collision count was being incremented by 1. But the value returned
 *      by the 82586 after transmission of a packet is the *number* of
 *      unsuccessful attempts made to transmit the packet. This has been
 *      fixed.
 *    - glds_errxmt is now incremented only when SCB_INT_CNA is not set.
 *    - updating all these counters is now done in a mutually exclusive
 *      fashion.
 *
 * Version 2.2 10/28/93 released on 28 Oct '93
 *   * Prints out the connector type after reading it from EEPROM
 *     Supports RJ45 connector on EtherExpress/16C board.
 *   * Autoconfiguration support ::
 *     - multiple values accepted for "intr" property
 *     - no "ioaddr" property needed in the .conf file
 *     - static table of all possible base_io addresses maintained in
 *       iee_probe()
 *   * Changes that have led to improved performance of iee_send() :
 *     - New function iee_wait_active() has been introduced
 *     - Call to iee_wait_scb() has been moved to the beginning of the
 *       procedure. This is followed by a call to iee_wait_active()
 *     - repoutsw() rather than repoutsb() is now used in WRITE_TO_SRAM
 *     - Instead of explicitly padding frames less than MIN_FRAME_LEN with
 *       zeroes, xmit_size is merely incremented to MIN_FRAME_LEN
 *     These changes in iee_send() have substantially improved performance of
 *     the driver by almost an order of magnitude (in ttcp).
 *   * iee_wait_scb() :: drv_usecwait() introduced instead of a busy wait
 *     delay in the inner loop
 *   * Changes to solve problems related to bug no. 1146635 related to
 *     panic due to "out of recive buffers" condition in iee_rcv_packet().
 *     - The suspicion that giant sized packets received on the network
 *       may cause more than IEEMAXPKT bytes to be written to the mp has led
 *       to substantial changes in the receive logic. The exact length of
 *       the frame is now computed before the data is actually copied from
 *       SRAM. A sanity check has also been introduced that discards packets
 *       greater than IEEMAXPKT bytes.
 *     - Check for CS_OK bit being set in iee_rcv_packet() to make sure that
 *       the 82586 did not run out of buffers while receiving the packet.
 *     - Corrects erroneous handling of "out of receive buffers" situation
 *       by rebuilding RU whenever receive unit leaves ready state
 *
 * Deviations from the design document
 *
 *	a) Statistics are being updated in the iee_gstat() function and not
 *	in iee_intr(), for performance reasons.
 *
 *	b) Size of received frame is not being computed dynamically, in
 *	iee_rcv_packet(). The message block allocated has a size of
 *	either the size of the first RBD (if it contains the entire frame)
 *	or 1520 bytes. This has been done as part of performance tuning.
 *
 *	c) Only ioctl supported is TDR_TEST. Therefore, the card_conf_t 
 *	and stat_t structures, which were meant for other ioctls, are not 
 *	being used.
 *
 *	d) Multicast addition and deletion logic has been changed. A list 
 *	of currently programmed multicast addresses is maintained in the
 *	driver private data structure (struct ieeinstance). This list is 
 *	first updated to reflect the change (addition or deletion) and then 
 *	given as a parameter for the MC_SETUP command.
 *
 *	e) Unused fields n_fd, n_rbd, ofst_rxb, ofst_rbd and ofst_fd 
 *	have been removed from struct ieeinstance. 
 *
 *	f) New fields introduced in ieeinstance: 
 *	ushort irq_level ; {* irq level in EEPROM *}
 *	multicast_t iee_multaddr[GLD_MAXMULTICAST]; {* list of mcast addr *}
 *	int multicast_count; {* length of current mcast list *}
 *	int restart_count; {* # of times RU has been restarted - for statistics *}
 *
 *
 * MISCELLANEOUS
 * 		vi options for viewing this file::
 *				set ts=4 sw=4 ai wm=4
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
static char sccsid[] =
			"@(#)gldconfig 1.1 93/02/12 Copyright 1993 Sun Microsystems";
#endif lint

#ident "@(#)iee.c	2.2  28 Oct 1993"    /* SCCS identification string */ 

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
#include <sys/iee.h>


/*
 *  Declarations and Module Linkage
 */

static char ident[]   = "EtherExpress-16";
static char version[] = "driver version 2.2";

#if defined(IEEDEBUG) || defined(IEEDEBUGX)
/* used for debugging */
int	ieedebug = 0;
#endif

static int attached_board_addresses[MAX_IEE_BOARDS];
static ushort no_of_boards_attached;

/*
 * Required system entry points
 */
static	iee_identify(dev_info_t *);
static	iee_devinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	iee_probe(dev_info_t *);
static	iee_attach(dev_info_t *, ddi_attach_cmd_t);
static	iee_detach(dev_info_t *, ddi_detach_cmd_t);

/*
 * Required driver entry points for GLD
 */
static int		iee_reset(gld_mac_info_t *);
static int		iee_prom(gld_mac_info_t *, int);
static int		iee_gstat(gld_mac_info_t *);
static int		iee_send(gld_mac_info_t *, mblk_t *);
static int		iee_ioctl(queue_t *, mblk_t *);
static int		iee_saddr(gld_mac_info_t *);
static int		iee_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
static uint		iee_intr(gld_mac_info_t *);

/*
 * Utility functions used by the driver
 */
static int		iee_init_board(gld_mac_info_t *);
static int		iee_start_board(gld_mac_info_t *);
static int		iee_stop_board(gld_mac_info_t *);
static int		iee_config(gld_mac_info_t *, ushort);
static int		iee_wait_scb(int, int, char *);
static int		iee_wait_active(int, int);
static int		iee_ack(int);
static int		iee_tdr_test(gld_mac_info_t *);
static void		iee_build_ru(gld_mac_info_t *);
static void		iee_rfa_fix(gld_mac_info_t *);
static void		iee_build_cu(gld_mac_info_t *);
static ushort	iee_find_sram_size(int);
static void		iee_rcv_packet(gld_mac_info_t *);
static void		iee_restart_ru(gld_mac_info_t *);
static int		iee_start_ru(gld_mac_info_t *);
static ushort	peek_eeprom(int, ushort);
static void		serial_write(int, ushort, ushort);
static ushort	serial_read(int);

/*
 * prototypes of external functions
 */
extern unchar	inb(int);
extern ushort	inw(int);
extern void		outb(int, unchar);
extern void		outw(int, ushort);
extern void 	repinsb(int, unchar *, int);
extern void 	repinsw(int, ushort *, int);
extern void 	repoutsb(int, unchar *, int);
extern void 	repoutsw(int, ushort *, int);

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

#ifdef IEEDEBUGX
uint dbg_len;
uint dbg_cnt;
ushort dbg_fd;
ushort dbg_rb;
unchar dbgsram[0x8000];
int	dbgioaddr = 0x250;

dbgmsg(char *fmt, ...)
{
	va_list adx;

	READ_FROM_SRAM((caddr_t)dbgsram, 0, 0x8000, dbgioaddr);

	va_start(adx, fmt);
	vcmn_err(CE_CONT, fmt, adx);
	va_end(adx);
	debug_enter("\niee: debug\n");
}
#endif IEEDEBUGX

/* Standard Streams initialization */

static struct module_info minfo = 
{
	IEEIDNUM, "iee", 0, INFPSZ, IEEHIWAT, IEELOWAT
};

static struct qinit rinit = 
{	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = 
{	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

static struct streamtab ieeinfo = 
{
	&rinit, &winit, NULL, NULL
};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_ieeops = 
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
	&ieeinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

static struct dev_ops ieeops = 
{
	DEVO_REV,				/* devo_rev */
	0,						/* devo_refcnt */
	iee_devinfo,			/* devo_getinfo */
	iee_identify,			/* devo_identify */
	iee_probe,				/* devo_probe */
	iee_attach,				/* devo_attach */
	iee_detach,				/* devo_detach */
	nodev,					/* devo_reset */
	&cb_ieeops,				/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = 
{
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,				/* short description */
	&ieeops				/* driver specific ops */
};

static struct modlinkage modlinkage = 
{
	MODREV_1, (void *)&modldrv, NULL
};

/*
 * General variable naming conventions followed in the file:
 * a) base_io_address always refers to the start address of i/o
 *    registers, as specified by the "ioaddr" property in
 *    the .conf file.
 * b) ieep is always a pointer to the driver private structure
 *    ieeinstance, defined in iee.h
 * c) scb_statreg, scb_rfareg, scb_cblreg and scb_cmdreg are 
 *    the Etherexpress' host shadow registers
 */



/*
 * 					ROUTINES TO INTERFACE WITH THE KERNEL
 */


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
 * Name			: iee_identify()
 * Purpose		: Determine if the driver drives the device specified 
 *                by the devinfo pointer
 * Called from	: Kernel
 * Arguments	: devinfo - pointer to a devinfo structure
 * Returns		: DDI_IDENTIFIED, if we know about this device
 *				  DDI_NOT_IDENTIFIED, otherwise
 * Side effects	: None
 */

iee_identify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "iee") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}


/*
 * Name			: iee_devinfo()
 * Purpose		: Reports the instance number and identifies the devinfo
 *				  node with which the instance number is associated
 * Called from	: Kernel
 * Arguments	: devinfo - pointer to a devinfo_t structure
 *				  cmd     - command argument: either 
 *                          DDI_INFO_DEVT2DEVINFO or 
 *                          DDI_INFO_DEVT2INSTANCE
 *				  arg     - command specific argument
 *				  result  - pointer to where requested information is 
 *                          stored
 * Returns		: DDI_SUCCESS, on success
 *				  DDI_FAILURE, on failure
 * Side effects	: None
 */

iee_devinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, 
			void **result)
{
	register int error;

	/* 
	 * This code is not DDI compliant: the correct semantics 
	 * for CLONE devices is not well-defined yet.            
	 */

	switch(cmd) 
	{
		case DDI_INFO_DEVT2DEVINFO:
			if (devinfo == NULL) 
			{
				error = DDI_FAILURE;	/* Unfortunate */
			} 
			else 
			{
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


/*
 * Name			: iee_probe()
 * Purpose		: Determine if the network controller is present 
 *                on the system
 * Called from	: Kernel
 * Arguments	: devinfo - pointer to a devinfo structure
 * Returns		: DDI_PROBE_SUCCESS, if the controller is detected
 *				  DDI_PROBE_FAILURE, otherwise
 * Side effects	: None
 */

iee_probe(dev_info_t *devinfo)
{
	int 	found_board = 0; /* flag */
	int     base_io_address;    
	char 	signature[4];    /* board  signature */
	int 	i, j;            /* scratch */
	static uint base_io_tab[] = { 0x200, 0x210, 0x220, 0x230, 0x240, 0x250,
								  0x260, 0x270, 0x300, 0x310, 0x320, 0x330,
								  0x340, 0x350, 0x360, 0x370
								};
	int			tab_size  = sizeof (base_io_tab) / sizeof (uint);
	int			regbuf[3];

#ifdef IEEDEBUG
	if (ieedebug & IEEDDI)
		cmn_err(CE_CONT, "iee_probe(0x%x)\n", devinfo);
#endif IEEDEBUG

	for (i = 0; i < tab_size; i++)
	{
		base_io_address = base_io_tab[i];

		/*
		 * do not probe at an ioaddr if a board is already attached at
		 * base_io_address
		 */
		for (j = 0; j < no_of_boards_attached; j++)
		{
			if (attached_board_addresses[j] == base_io_address)
				break;
		}
		if (j < no_of_boards_attached)
			continue;

		/*
		 * Probe for the board to see if it's there
		 * Read in SIGLEN bytes of the signature and map them in such a way
		 * that it becomes an ascii string
		 * Note: normally, one would map in the device memory here, but
		 * we don't need to since the IEE is i/o mapped.
		 */
		for (j = 0; j < SIGLEN; j++)
		{
			signature[j] = (inb(base_io_address + AUTO_ID) >> 4) - 0xa + 'a';
		}

		/* 
		 * "abab" and "baba" are valid signatures 
		 */
		if ((strncmp(signature, "abab", SIGLEN) == 0) 
			|| (strncmp(signature, "baba", SIGLEN) == 0))
		{
			regbuf[0] = base_io_address;
			(void) ddi_prop_create(DDI_DEV_T_NONE, devinfo, DDI_PROP_CANSLEEP,
						"ioaddr", (caddr_t)regbuf, sizeof (int));
			found_board++;
			break;
		}
	}

	/*
	 *  Return whether the board was found.  If unable to determine
	 *  whether the board is present, return DDI_PROBE_DONTCARE.
	 */
	if (found_board)
	   	return (DDI_PROBE_SUCCESS);
	else
		return (DDI_PROBE_FAILURE);
}


/*
 * Name			: iee_attach()
 * Purpose		: Attach a driver instance to the system. This 
 *                function is called once for each board successfully 
 *                probed.
 *				  gld_register() is called after macinfo's fields are
 *				  initialized, to register the driver with gld.
 * Called from	: Kernel
 * Arguments	: devinfo - pointer to the device's devinfo structure
 *				  cmd     - should be set to DDI_ATTACH
 * Returns		: DDI_SUCCESS on success
 *				  DDI_FAILURE on failure
 * Side effects	: macinfo is initialized before calling gld_register()
 */

iee_attach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t     *macinfo;            /* GLD structure */
	struct ieeinstance *ieep;		        
	register int       base_io_address;       
	unchar             myaddr[ETHERADDRL];  /* Enet address read from EEPROM */
	ushort             irq_level = 0;       /* value of "intr" prop */
	ushort             eeprom_irq;          /* irq val read from EEPROM */
	int                len = 0;             /* used for ddi_getlongprop */
	int                rval;                /* scratch */
	int                word;                /* scratch */
	int                i;                   /* scratch */
	ushort             connect_type;        /* connector type */
	ushort             other;               /* connector type :: BNC/TPE */
	ushort             ecr1;                /* extended ctrl register */
	ushort             shadow_auto_id;      /* shadow auto-id register */
	char               iee_id[4];           /* id value from shadow_auto_id */
	/*
	 * mapping of irq level to irq val. to be written to
	 * SEL_IRQ register of the IEE. See EtherExpress EPS
	 */
	char               irqvals[] = { 0, 0, 1, 2, 3, 4, 0, 0,
									 0, 1, 5, 6, 0, 0, 0, 0 
								   };
	struct intrprop
	{
		int	spl;
		int	irq;
	} *intrprop;

#ifdef IEEDEBUG
	if (ieedebug & IEEDDI) {
		cmn_err(CE_CONT, "iee_attach(0x%x)\n", devinfo);
		debug_enter("\n\nIEE ATTACH\n\n");
	}
#endif IEEDEBUG

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	/*
	 *  Allocate gld_mac_info_t and ieeinstance structures
	 */

	macinfo = (gld_mac_info_t *) kmem_zalloc(sizeof (gld_mac_info_t) 
			  + sizeof (struct ieeinstance), KM_NOSLEEP);
	if (macinfo == NULL)
		return (DDI_FAILURE);
	ieep = (struct ieeinstance *)(macinfo + 1);

	/*  
	 * Initialize our private fields in macinfo and ieeinstance 
	 */

	macinfo->gldm_private = (caddr_t)ieep;
	macinfo->gldm_port = ddi_getprop(DDI_DEV_T_ANY, devinfo,
									 DDI_PROP_DONTPASS, "ioaddr", 0);
	macinfo->gldm_state = IEE_IDLE;
	macinfo->gldm_flags = 0;

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */

	macinfo->gldm_reset   = iee_reset;
	macinfo->gldm_start   = iee_start_board;
	macinfo->gldm_stop    = iee_stop_board;
	macinfo->gldm_saddr   = iee_saddr;
	macinfo->gldm_sdmulti = iee_dlsdmult;
	macinfo->gldm_prom    = iee_prom;
	macinfo->gldm_gstat   = iee_gstat;
	macinfo->gldm_send    = iee_send;
	macinfo->gldm_intr    = iee_intr;
	macinfo->gldm_ioctl   = NULL;    

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */

	macinfo->gldm_ident   = ident;
	macinfo->gldm_type    = DL_ETHER;
	macinfo->gldm_minpkt  = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt  = IEEMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen  = -2;

	/* 
	 * RESET bit has to be set and reset while configuring the
	 * EEPROM.
	 */

	base_io_address = (int) macinfo->gldm_port;

	/* 
	 * "warm-up" the dram by doing 16 bytes of buffer access
	 * See EtherExpress EPS section 3.6, pg. v
	 */

	outw(base_io_address + RD_PTR, 0);
	for (i = 0; i < 16; i++)
		(void)inb(base_io_address + DX_REG);

	/* 
	 * reset ASIC on board 
	 * need at least 240 usec of delay as recommended in IEE EPS
	 */

	outb(base_io_address + RESET, RESET_586);
	drv_usecwait(400);
	outb(base_io_address + RESET, RESET_586 | GA_RESET);
	drv_usecwait(400);
	outb(base_io_address + RESET, RESET_586);
	drv_usecwait(400);

	/* 
	 * Read the ethernet address from the EEPROM (3 words)
	 */

	word = 3;
	do
	{
		ushort tmp;

		tmp = peek_eeprom(base_io_address, INTEL_EADDR_L + 3 - word);
		myaddr[(2 * word) - 1] = tmp & 0xff;
		myaddr[(2 * word) - 2] = ((ushort) (tmp & 0xff00)) >> 8;
	}
	while (word--);

	eeprom_irq = peek_eeprom(base_io_address, BASE_IO_REG);
	eeprom_irq = (ushort)(eeprom_irq & 0xe000) >> (ushort)13;
#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT,"eeprom_irq read = 0x%x, len = %d\n",eeprom_irq,len);
#endif IEEDEBUG

	rval = ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
						   "intr", (caddr_t)&intrprop, &len);
	if (rval != DDI_PROP_SUCCESS)
	{
		cmn_err(CE_WARN, "iee: no intr property");
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t) + sizeof (struct ieeinstance));
		return (DDI_FAILURE);
	}
	for (i = 0; i < (len / (sizeof (struct intrprop))); i++)
	{
		/* 
		 * map irq_level to the value to be written to the SEL_IRQ
		 * register - see EtherExpress EPS pg. ix.
		 * Then compare with the value read from EEPROM
		 */
		irq_level = intrprop[i].irq;
		if ((irq_level >= 2) && (irq_level <= 15) &&
			(irqvals[irq_level] == eeprom_irq))
		{
			irq_level = irqvals[irq_level];
			break;
		}
	}
	if (i < (len / (sizeof (struct intrprop))))
	{
		kmem_free(intrprop, len);
		macinfo->gldm_irq_index = i;
		ieep->irq_level = irq_level;
	}
	else /* irq value in .conf file does not match that in EEPROM */
	{
		kmem_free(intrprop, len);
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t) + sizeof (struct ieeinstance));
		return (DDI_FAILURE);
	}

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT,"irq lev 0x%x\n",ieep->irq_level);
#endif IEEDEBUG

	/*
	 * read the connector type from EEPROM
	 * variable "connect_type" is to be encoded as follows ::
	 *     0 :: AUI
	 *     1 :: BNC
	 *     2 :: TPE (RJ45)
	 */
	connect_type = peek_eeprom(base_io_address, BASE_IO_REG);
	connect_type = (connect_type >> 12) & 1;
	if (connect_type)
	{
        other = peek_eeprom(base_io_address, SEL_CONNECTOR_REG);
	    other &= 1;
	    connect_type += other;
	}

    /*
	 * Determine the type of EtherExpress board. Set the connector type
	 * for 0xBABB (EtehrExpress 16C) boards.
	 * Register ecr1 should NOT be accessed for 0xBABA boards.
	 */

	shadow_auto_id = base_io_address | SHADOW_AUTO_ID;
	for (i = 0; i < 4; i++)
	{
		int		j;

		j = inb(shadow_auto_id);
		iee_id[(j & 0x03)] = (j >> 4) - 0xa + 'a';
	}

	if (strncmp(iee_id, "bbab", 4) == 0)
	{
		ecr1 = base_io_address | ECR1;
		i = inb(ecr1);
		switch (connect_type)
		{
		    case AUI :
				/* clear bits 7 and 1 */
				i &= 0x7d;
				outb(ecr1, i);
				break;
			case RJ45 :
				/* set bits 7 and 1 */
				i |= 0x82;
				outb(ecr1, i);
				break;
			case BNC :
				/* set bit 7, clear bit 1 */
				i |= 0x80;
				i &= 0xfd;
				outb(ecr1, i);
				break;
		}
	}

	/* 
	 * Release the 82586 from reset
	 */
	outb(base_io_address + RESET, 0); 

	/* 
	 * Get the board's vendor-assigned hardware network address 
	 */

	for (i = 0; i < ETHERADDRL; i++)
		macinfo->gldm_vendor[i] = myaddr[i];

	bcopy((caddr_t)gldbroadcastaddr, (caddr_t)macinfo->gldm_broadcast,
		  ETHERADDRL);
	bcopy((caddr_t)macinfo->gldm_vendor, (caddr_t)macinfo->gldm_macaddr,
		  ETHERADDRL);


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
	
	if (gld_register(devinfo, "iee", macinfo) != DDI_SUCCESS) {
		kmem_free((caddr_t)macinfo,
			  sizeof(gld_mac_info_t) + sizeof (struct ieeinstance));
		return (DDI_FAILURE);
	}

	mutex_enter(&macinfo->gldm_maclock);

	/*
	 *  Do anything necessary to prepare the board for operation
	 *  short of actually starting the board.
	 */

	if (iee_init_board(macinfo) == FAIL) {
		cmn_err(CE_WARN,"iee: init board failed");
		gld_unregister(macinfo);
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t) + sizeof (struct ieeinstance));
		mutex_exit(&macinfo->gldm_maclock);
		return (DDI_FAILURE);
	}

	/* 
	 * Make sure we have our address set 
	 */

	if (iee_saddr(macinfo) == FAIL) {
		cmn_err(CE_WARN, "iee: set physical address failed");
		gld_unregister(macinfo);
		kmem_free((caddr_t)macinfo,
			sizeof (gld_mac_info_t) + sizeof (struct ieeinstance));
		mutex_exit(&macinfo->gldm_maclock);
		return (DDI_FAILURE);
	}

	/*
	 * clear interrupt bit of SEL_IRQ register and set it again
	 */

	i = inb(base_io_address + SEL_IRQ);
	outb(base_io_address + SEL_IRQ, i & 0xf7);
	outb(base_io_address + SEL_IRQ, i | 0x08 | ieep->irq_level);

	attached_board_addresses[no_of_boards_attached++] = base_io_address;
	mutex_exit(&macinfo->gldm_maclock);
	return (DDI_SUCCESS);
}


/*
 * Name			: iee_detach()
 * Purpose		: Detach a driver instance from the system. This 
 *                includes unregistering the driver from gld
 * Called from	: Kernel
 * Arguments	: devinfo - pointer to the device's dev_info structure
 *				  cmd     - type of detach, should be DDI_DETACH always
 * Returns		: DDI_SUCCESS if the state associated with the given 
 *                device was successfully removed
 *				  DDI_FAILURE otherwise
 * Side effects	: None
 */

iee_detach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t     *macinfo;		/* GLD structure */
#ifdef STATS
	struct ieeinstance *ieep;           /* for printing statistics */
#endif STATS


#ifdef IEEDEBUG
	if (ieedebug & IEEDDI)
		cmn_err(CE_CONT, "iee_detach(0x%x)\n", devinfo);
#endif IEEDEBUG

	if (cmd != DDI_DETACH) 
		return (DDI_FAILURE);

	/* 
	 * Get the driver private (gld_mac_info_t) structure 
	 */

	macinfo = (gld_mac_info_t *) ddi_get_driver_private(devinfo);

#ifdef STATS
	ieep = (struct ieeinstance *) macinfo->gldm_private;
	cmn_err(CE_NOTE, "No. of restarts :: 0x%x\n", ieep->restart_count);
#endif STATS

	/* 
	 * stop the board if it is running 
	 */

	(void) iee_stop_board(macinfo);

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

	if (gld_unregister(macinfo) == DDI_SUCCESS) 
	{
		kmem_free((caddr_t) macinfo,
			sizeof (gld_mac_info_t) + sizeof (struct ieeinstance));
		return (DDI_SUCCESS);
	}
	else
		return (DDI_FAILURE);
}



/*
 *							GLD ENTRY POINTS
 */


/*
 * Name			: iee_reset()
 * Purpose		: Reset the board to its initial state
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: TRUE, if the board was successfully reset
 * 				  FALSE, otherwise
 * Side effects : All data structures and lists maintained by the 
 *                82586 are flushed
 */

int
iee_reset(gld_mac_info_t *macinfo)
{

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT, "iee_reset(0x%x)\n", macinfo);
#endif IEEDEBUG

	if (iee_stop_board(macinfo) == FALSE)
	{
		cmn_err(CE_WARN, "iee: stop board failed");
		return (FALSE);
	}
	(void) iee_init_board(macinfo);
	(void) iee_saddr(macinfo);

	return (TRUE);
}


/*
 * Name			: iee_start_board()
 * Purpose		: Start the device's receiver and enable interrupts
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: TRUE on success
 *				  FALSE on failure
 * Side effects	: Receiver unit of the 82586 and interrupts get enabled
 */

int
iee_start_board(gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	register int        base_io_address = (int) macinfo->gldm_port;
	ushort              scb_cmdreg   = (SCB_CMDREG | macinfo->gldm_port);
	ushort              scb_rfareg   = (SCB_RFAREG | macinfo->gldm_port);
	unchar              i;         /* scratch */

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
	{
		cmn_err(CE_CONT, "iee_start_board(0x%x)\n", macinfo);
		cmn_err(CE_CONT,"irq :: %d\n", ieep->irq_level);
	}
#endif IEEDEBUG

	/*
	 * clear interrupt bit of SEL_IRQ register and set it again
	 */

	i = inb(base_io_address + SEL_IRQ);
	outb(base_io_address + SEL_IRQ, i & 0xf7);
	outb(base_io_address + SEL_IRQ, i | 0x8 | (ieep->irq_level));

#ifdef IEEDEBUG
	if (ieedebug & IEEINT) {
		i = inb(base_io_address + SEL_IRQ);
		cmn_err(CE_CONT, "Interrupts enabled - SEL_REQ = 0x%x\n", i);
	}
#endif

	if (iee_wait_scb(1000, base_io_address, "start_board") == FAIL) {
		return (FALSE);
	}

	/* 
	 * enable 586 Receive Unit 
	 */
	return (iee_start_ru(macinfo));
}

/*
 * Name			: iee_start_ru()
 * Purpose		: Start the Receive Unit on the chip
 * Called from	: iee_start_board, iee_restart_ru()
 * Arguments	: ieep - pointer to a ieepinstance structure
 * Returns		: TRUE on success
 *				  FALSE on failure
 * Side effects	: The Receive Unit is started
 */

int
iee_start_ru(gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	int		base_io_address     = (int) macinfo->gldm_port;
	ushort	scb_cmdreg = (SCB_CMDREG | macinfo->gldm_port);
	ushort	scb_rfareg = (SCB_RFAREG | macinfo->gldm_port);

	if (iee_wait_scb(1000, base_io_address, "start_ru") == FAIL)
		return (FALSE);

	outw(scb_rfareg, ieep->begin_fd); 
	outw(scb_cmdreg, SCB_RUC_START); 

	/* Go 82586! */
	outb(base_io_address + CA_CTRL, 1);

	if (iee_wait_scb(1000, base_io_address, "ruc_start") == FAIL)
		return (FALSE);

	return (TRUE);
}


/*
 * Name			: iee_stop_board()
 * Purpose		: Stop the device's receiver and disables interrupts
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: TRUE on success
 *				  FALSE on failure
 * Side effects	: Receiver unit of the 82586 and interrupts are disabled
 */

int
iee_stop_board(gld_mac_info_t *macinfo)
{
	ushort       scb_cmdreg   = (SCB_CMDREG | macinfo->gldm_port);
	register int base_io_address = (int) macinfo->gldm_port;
	int		     i; /* scratch */

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT, "iee_stop_board(0x%x)\n", macinfo);
#endif IEEDEBUG

	if (iee_wait_scb(1000, base_io_address, "stop_board") == FAIL)
	{
		return (FALSE);
	}

	/*
	 * stop the RU
	 */

	outw(scb_cmdreg, SCB_CUC_SUSPEND | SCB_RUC_SUSPEND);	

	/* Go 82586! */
	outb(base_io_address + CA_CTRL, 1);

	/* 
	 * disable interrupts by lowering the inter_enable bit
	 */

	i = inb(base_io_address + SEL_IRQ);
	outb(base_io_address + SEL_IRQ, i & 0xf7);
	return (TRUE);
}

/*
 * Name			: iee_saddr()
 * Purpose		: Set the physical network address on the board
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: SUCCESS on success
 *				  FAIL    on failure
 * Side effects	: None
 */

int
iee_saddr(gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep  = (struct ieeinstance *)macinfo->gldm_private;
	register int base_io_address = (int) macinfo->gldm_port;
	ushort scb_cmdreg = (SCB_CMDREG | macinfo->gldm_port);
	ushort scb_cblreg = (SCB_CBLREG | macinfo->gldm_port);
	ushort i; /* scratch */

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT, "iee_saddr(0x%x)\n", macinfo);
#endif IEEDEBUG

	if (iee_wait_scb(1000, base_io_address, "saddr") == FAIL)
		return (FAIL);

	/* 
	 * initialise status, cmd, cbl fields
	 */

	outw(scb_cmdreg, SCB_CUC_START); 
	outw(scb_cblreg, ieep->offset_gencmd); 
	
	/*
	 * initialise the status, command and link fields of the
	 * gencmd structure. Note that we use the autoincrementing
	 * feature of DX_REG here for the three successive outw's
	 */

	WRITE_WORD(ieep->offset_gencmd + CMD_STATUS, 0, base_io_address);	
	outw(base_io_address + DX_REG, CS_CMD_IASET | CS_EL); 
	outw(base_io_address + DX_REG, 0xffff);			

	/* 
	 * add sizes of xmit_t and conf_t (+6) to the current offset 
	 * before writing the enet address
	 */

	WRITE_BYTE(ieep->offset_gencmd + 6, macinfo->gldm_macaddr[0], 
			   base_io_address);
	for (i = 1; i < ETHERADDRL; i++)
	    outb(base_io_address + DX_REG, macinfo->gldm_macaddr[i]);

	/* Go 82586! */
	outb(base_io_address + CA_CTRL, 1);

	if (iee_wait_scb(1000, base_io_address, "saddr2") == FAIL)
		return (FAIL);

	if (iee_ack(base_io_address) == FAIL)
		return (FAIL);

	return (SUCCESS);
}

/*
 * Name			: iee_dlsdmult()
 * Purpose		: Enable/disable device level reception of specific
 *				  multicast addresses
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 *				  mcast   - multicast address
 *				  op      - enable(1) / disable(0) flag
 * Returns		: TRUE   on success
 *				  FALSE  on failure
 * Side effects	: None
 */

int
iee_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	ushort scb_cmdreg = (SCB_CMDREG | macinfo->gldm_port);
	ushort scb_cblreg = (SCB_CBLREG | macinfo->gldm_port);
	register int base_io_address;
	ushort dest;               /* scratch */
	int i;                     /* scratch */

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT, "iee_dlsdmult(0x%x, %s)\n", macinfo,
				op ? "ON" : "OFF");
#endif IEEDEBUG


	base_io_address = macinfo->gldm_port;

	if (iee_wait_scb(1000, base_io_address, "dlsdmult") == FAIL)
		return (FALSE);

	if (op)
	{
		/*
		 * add multicast 
		 * update local table and then give list of addresses to 82586
		 */

		for (i = 0; i <  GLD_MAX_MULTICAST; i++)
		{
			if (ieep->iee_multiaddr[i].entry[0] == 0)
			{
				/* free entry found */
				bcopy((caddr_t) mcast->ether_addr_octet, 
					  (caddr_t)ieep->iee_multiaddr[i].entry, ETHERADDRL);
				ieep->multicast_count++;
				break;
			}
		}
		if (i >= GLD_MAX_MULTICAST)
		{
			cmn_err(CE_WARN, "iee: multicast table full");
			return (FALSE);
		}
	}
	else 
	{
		/* 
		 * remove multicast 
		 * update local table first
		 */
		for (i = 0; i <  GLD_MAX_MULTICAST; i++)
		{
			if (bcmp((caddr_t)mcast->ether_addr_octet, 
						(caddr_t)ieep->iee_multiaddr[i].entry, 
						ETHERADDRL) == 0)
			{
				/* matching entry found - invalidate it */
				ieep->iee_multiaddr[i].entry[0] = 0;
				ieep->multicast_count--;
				break;
			}
		}
		if (i == GLD_MAX_MULTICAST)
		{
			cmn_err(CE_WARN, "iee: no matching multicast entry found");
			return (FALSE);
		}
	} /* else */

	/* 
	 * initialise status, cmd, cbl fields
	 */

	outw(scb_cmdreg, SCB_CUC_START); 
	outw(scb_cblreg, ieep->offset_gencmd); 

	/*
	 * initialise the status, command and link fields of the
	 * gencmd structure. Note that we use the autoincrementing
	 * feature of DX_REG here for the three successive outw's
	 */

	WRITE_WORD(ieep->offset_gencmd + CMD_STATUS, 0, base_io_address);	
	outw(base_io_address + DX_REG, CS_CMD_MCSET | CS_EL);	
	outw(base_io_address + DX_REG, 0xffff);

	/*
	 * Now give the list of addresses to 82586
	 */

	dest = ieep->offset_gencmd + CMD_PARAMETER + 2;
	WRITE_WORD(ieep->offset_gencmd + CMD_PARAMETER, 
			   ETHERADDRL * ieep->multicast_count, 
			   base_io_address);

	for (i = 0; i < GLD_MAX_MULTICAST; i++)
	{
		if (ieep->iee_multiaddr[i].entry[0] == 0)
		{
			continue;
		}
#ifdef IEEDEBUG
		if (ieedebug & IEETRACE) {
			cmn_err(CE_CONT, "Adding multicast addr : ");
			IEEPRINT_EADDR(ieep->iee_multiaddr[i].entry);
		}
#endif IEEDEBUG

		WRITE_TO_SRAM((unchar *)ieep->iee_multiaddr[i].entry, dest, 
						(ushort) ETHERADDRL, base_io_address);
		dest += ETHERADDRL;
	}

	/* Go 82586! */
	outb(base_io_address + CA_CTRL, 1);

	if (iee_wait_scb(1000, base_io_address, "dlsdmult2") == FAIL)
		return (FALSE);

	if (iee_ack(base_io_address) == FAIL)
		return (FALSE);

	return (TRUE);
}


/*
 * Name			: iee_prom()
 * Purpose		: Enable/disable physical level promiscuous mode
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: SUCCESS on success
 *				  FAIL    on failure
 * Side effects	: Board gets thrown into (or returns from) promiscuous 
 *                mode
 */

int
iee_prom(gld_mac_info_t *macinfo, int on)
{

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT, "iee_prom(0x%x, %s)\n", macinfo,
				on ? "ON" : "OFF");
#endif IEEDEBUG

	/* 
	 * enable or disable promiscuous mode 
	 */

	if (on)
		return (iee_config(macinfo, (LPBK_OFF | PRO_ON)));
	else
		return (iee_config(macinfo, (LPBK_OFF | PRO_OFF)));
}


/*
 * Name			: iee_gstat()
 * Purpose		: Gather statistics from the hardware and update the
 *				  gldm_stats structure. 
 * Called from	: gld, just before it reads the driver's statistics 
 *                structure
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: TRUE always
 * Side effects	: None
 */

int
iee_gstat(gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep  = (struct ieeinstance *)macinfo->gldm_private;
	register int base_io_address = (int) macinfo->gldm_port;

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT, "iee_gstat(0x%x)\n", macinfo);
#endif IEEDEBUG

	/*
	 * Copy error statistics from SCB to the gldm_stats structure
	 */

	READ_WORD(ieep->offset_scb + SCB_CRC_ERR, base_io_address, 
			macinfo->gldm_stats.glds_crc);
	macinfo->gldm_stats.glds_frame   = inw(base_io_address + DX_REG);
	macinfo->gldm_stats.glds_missed  = inw(base_io_address + DX_REG);
	macinfo->gldm_stats.glds_overflow = inw(base_io_address + DX_REG);

	return (TRUE);
}


/*
 * Name			: iee_send()
 * Purpose		: Transmit a packet on the network. Note that this 
 *                function returns even before transmission by the 
 *                82586 completes. Hence, return value of SUCCESS is 
 *                no guarantee that the packet was successfully 
 *                transmitted (that is, without errors during 
 *                transmission)
 *				  
 * Called from	: gld, when a packet is ready to be transmitted
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 *				  mp      - pointer to an M_DATA message that contains 
 *                          the packet. The complete LLC header is 
 *                          contained in the message's first message 
 *                          block, and the remainder of the packet is 
 *                          contained within additional M_DATA message 
 *                          blocks linked to the first message block
 * Returns		: SUCCESS if a command was issued to the 82586 to 
 *                transmit a packet
 *				  RETRY   on failure so that gld may retry later
 * Side effects	: None
 */

int
iee_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	struct ieeinstance *ieep  = (struct ieeinstance *)macinfo->gldm_private;
	register int base_io_address = (int) macinfo->gldm_port;
	ushort scb_cmdreg = (SCB_CMDREG | macinfo->gldm_port);
	ushort scb_cblreg = (SCB_CBLREG | macinfo->gldm_port);
	ushort	offset_tbd;     /* transmit buffer descriptor */
	ushort	offset_tbuf;    /* transmit buffer */
	ushort	dest;           /* address in SRAM */
	mblk_t	*mp1;           /* scratch */
	ushort	xmit_size = 0;  /* counter */
	ushort	msg_size = 0;   /* counter */

	/*
	 * host address in SRAM 
	 */
	struct ether_header *eh = (struct ether_header *)mp->b_rptr;
	unchar *src = (unchar *)(eh->ether_dhost.ether_addr_octet);

#ifdef IEEDEBUG
	if (ieedebug & IEESEND)
		cmn_err(CE_CONT, "iee_send(0x%x, 0x%x)\n", macinfo, mp);
#endif IEEDEBUG

	offset_tbd  = ieep->offset_tbd;
	offset_tbuf = ieep->offset_tbuf;

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE) {
		cmn_err(CE_CONT, "Destination Address in iee_send :: ");
		IEEPRINT_EADDR(src);
		cmn_err(CE_CONT, "\n");
	}
#endif IEEDEBUG

	if (iee_wait_scb(1000, base_io_address, "send") == FAIL) {
		macinfo->gldm_stats.glds_defer++;	
		return (RETRY);
	}

	/* 
	 * Wait for the previous command to complete 
	 */

	if (iee_wait_active(1000, base_io_address) == FAIL)
	{
		macinfo->gldm_stats.glds_defer++;	
		return (RETRY);
	}


	dest = ieep->offset_cmd + CMD_PARAMETER + 2; 
	WRITE_TO_SRAM((unchar *)src, dest, (ushort) ETHERADDRL, base_io_address);
	WRITE_WORD(dest + ETHERADDRL, eh->ether_type, base_io_address); 
	mp->b_rptr += sizeof (struct ether_header);

	/* 
	 * copy the data to the transmit buffer by going through
	 * the mblks
	 */

	outw(base_io_address + WR_PTR, offset_tbuf);
	for (mp1 = mp; mp1 != NULL; mp1 = mp1->b_cont) 
	{
		msg_size = mp1->b_wptr - mp1->b_rptr;
		if (msg_size == 0)
			continue;
		WRITE_TO_SRAM((unchar *)(mp1->b_rptr), offset_tbuf, msg_size, 
					  base_io_address);
		offset_tbuf += msg_size;
		xmit_size += msg_size;
	}

	/*
	 * pad if necessary
	 * Minimum size of frame should be 64 bytes. So, the size of the 
	 * data field in the frame must be 64 - (sizeof (struct ether_
	 * header) :: 14 bytes + sizeof (Frame_check_sequence) :: 4 bytes
	 * which is 46 bytes.
	 */

	if (xmit_size < MIN_FRAME_LEN)
	{
		xmit_size = MIN_FRAME_LEN;
	}
	WRITE_WORD(offset_tbd, xmit_size | CS_EOF, base_io_address);	

	/*
	 * start the 82586 after initialising the SCB fields
	 */

	outw(scb_cmdreg, SCB_CUC_START);
	outw(scb_cblreg, ieep->offset_cmd);

	/* Go 82586! */

	outb(base_io_address + CA_CTRL, 1);

	return (SUCCESS);
}


/*
 * Name			: iee_intr()
 * Purpose		: Interrupt handler for the device
 * Called from	: gld
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: DDI_INTR_CLAIMED   if the interrupt was serviced
 *				  DDI_INTR_UNCLAIMED otherwise
 * Side effects	: None
 */

uint
iee_intr(gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep  = (struct ieeinstance *)macinfo->gldm_private;
	register int base_io_address = (int) macinfo->gldm_port;
	ushort scb_statreg = (SCB_STATREG | macinfo->gldm_port);
	ushort scb_cmdreg = (SCB_CMDREG | macinfo->gldm_port);
	ushort scb_status; /* SCB status */
	ushort cmd_status; /* status of xmit cmd */
	int	i;             /* scratch */

#ifdef IEEDEBUG
	if (ieedebug & IEEINT)
		cmn_err(CE_CONT, "iee_intr(0x%x)\n", macinfo);
#endif IEEDEBUG

	/*
	 * If scb command field doesn't get cleared, reset the board
	 */

	if (iee_wait_scb(1000, base_io_address, "intr") == FAIL) {
		return (DDI_INTR_UNCLAIMED);
	}

	/*
	 * acknowledge 82586 interrupt by copying the SCB status word to the
	 * SCB command word and issuing a channel attention
	 */

	scb_status = inw(scb_statreg);
	if ((scb_status & SCB_INT_MASK) == 0) {
		return (DDI_INTR_UNCLAIMED);
	}

	macinfo->gldm_stats.glds_intr++;
	outw(scb_cmdreg, scb_status & SCB_INT_MASK);
	outb(base_io_address + CA_CTRL, 1);

	if (iee_wait_scb(1000, base_io_address, "intr2") == FAIL) {
		return (DDI_INTR_UNCLAIMED);
	}


	/*
	 * interrupt for frame received?
	 */

#ifdef IEEDEBUGX
	if (ieedebug & IEEINT) {
		if (scb_status & SCB_INT_RNR)
			dbgmsg("iee: RNR\n");
	}
#endif IEEDEBUGX

	if (scb_status & SCB_INT_FR) {
		iee_rcv_packet(macinfo);
	}

	/* Check for Receiver No Resources condition */
	if (scb_status & SCB_INT_RNR) {
		/* make certain nothing arrived since the last time I checked */
		iee_rcv_packet(macinfo);

		/* rebuild the receive area and restart the receive unit */
		if (!(scb_status & SCB_RUS_READY)) {
			iee_restart_ru(macinfo);
			ieep->restart_count++;
		}
	}

#ifdef IEEDEBUGX
	if (ieedebug & IEEINT) {
		if (scb_status & SCB_INT_RNR)
			dbgmsg("iee: RNR\n");
	}
#endif IEEDEBUGX

	/*
	 * interrupt for frame transmitted?
	 */

	if (scb_status & (SCB_INT_CX | SCB_INT_CNA)) {
		READ_WORD(ieep->offset_cmd, base_io_address, cmd_status);

		/* 
		 * Read the status register to check transmission status 
		 * and update appropriate fields in the gldm_stats structure
		 */ 

		if (cmd_status & CS_COLLISIONS) {
			macinfo->gldm_stats.glds_collisions +=
				   (cmd_status & CS_COLLISIONS);

		} else if (cmd_status & CS_CARRIER) {
			macinfo->gldm_stats.glds_nocarrier++; 

		} else if (!(cmd_status & CS_OK) && !(scb_status & SCB_INT_CNA)) {
			macinfo->gldm_stats.glds_errxmt++; 
		}

		WRITE_WORD(ieep->offset_cmd, 0, base_io_address);
	}

	/*
	 * clear interrupt bit of SEL_IRQ register and set it again
	 */

	i = inb(base_io_address + SEL_IRQ);
	outb(base_io_address + SEL_IRQ, i & 0xf7);
	outb(base_io_address + SEL_IRQ, i | 0x08);

	return (DDI_INTR_CLAIMED);
}

/*
 * Name			: iee_ioctl()
 * Purpose		: Implement device-specific ioctl commands
 * Called from	: gld
 * Arguments	: q  - pointer to a queue_t structure
 *				  mp - pointer to an mblk_t structure
 * Returns		: TRUE  if the ioctl command was successful
 *				  FALSE otherwise
 * Side effects	: None
 */

static
int 
iee_ioctl(queue_t *q, mblk_t *mp)
{
	gld_t *gldp = (gld_t *)(q->q_ptr);            /* gld private */
	gld_mac_info_t *macinfo = gldp->gld_mac_info; /* macinfo struct */
	int cmd;                                      /* ioctl cmd val */
	int retval = 0;                               /* scratch */

	if (((struct iocblk *) mp->b_rptr)->ioc_count == TRANSPARENT)
	{
#ifdef IEEDEBUG
		if (ieedebug & IEETRACE)
			cmn_err(CE_WARN, "iee: xparent ioctl");
#endif
		goto err;
	}

	switch (cmd = ((struct iocblk *) mp->b_rptr)->ioc_cmd)
	{
		default:
			cmn_err(CE_WARN, "iee: unknown ioctl 0x%x", cmd);
			goto err;

		case TDR_TEST :
			if ((retval = iee_tdr_test(macinfo)) == FAIL)
				goto err;
			break;

	}	/* end of switch */

	/*
	 * acknowledge the ioctl
	 */
	((struct iocblk*) mp->b_rptr)->ioc_rval = retval;
	mp->b_datap->db_type = M_IOCACK;
	qreply(q, mp);
	return (TRUE);

err:
	((struct iocblk *) mp->b_rptr)->ioc_rval = -1;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(q, mp);
	return (FALSE);
}



/*
 * 					UTILITY ROUTINES SPECIFIC TO THE DRIVER
 */


/*
 * Name			: iee_init_board()
 * Purpose		: Initialize the specified network board. Initialize the
 *				  82586's SCP, ISCP and SCB; reset the 82586; do IA 
 *                setup command to initialize the 82586's individual 
 *                address.
 *				  DO NOT enable the Receive Unit. 
 * Called from	: iee_attach()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: SUCCESS if board initializations encountered no errors
 *				  FAIL    otherwise
 * Side effects	: Previous state of the 82586 and its data structures 
 *                are lost
 */

int
iee_init_board(gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	register int base_io_address = (int) macinfo->gldm_port;
	ushort offset_scp = MAX_SRAM_SIZE - sizeof (scp_t);	
						  /* offset of SCP in SRAM */
	ushort offset_iscp = offset_scp - sizeof (iscp_t);	
						  /* offset of ISCP in SRAM */
	ushort scb_statreg = (SCB_STATREG | macinfo->gldm_port);
	ushort i, sram_offset; /* scratch */
	ushort rb = 0;         /* scratch */
	iscp_t iscp;		   /* scratch :: ISCP structure */
	scb_t scb;		       /* scratch :: SCB structure */
	scp_t scp;             /* scratch :: SCP structure */

#ifdef IEEDEBUG
	if (ieedebug & IEETRACE)
		cmn_err(CE_CONT, "iee_init_board(0x%x)\n", macinfo);
#endif IEEDEBUG
	
	/*
	 * Memory (Static RAM) layout is as follows:
	 *  0x0000 - 0x0007 : Unused
	 *	Offset 0x0008   :
	 *      			SCB  System Control Block
	 *      			CB   Command Block for general (non-xmit) commands
	 *					CB   Command Block for xmit command
	 *					TBD  Transmit Buffer Descriptor
	 *					Transmit buffer
	 *					RFDs Receive Frame Descriptors
	 *					RBDs Receive Buffer Descriptors 
	 *					Receive buffers
	 * 					ISCP Intermediate System Configuration Pointer
	 * 					SCP  System Configuration Pointer
	 *	End of RAM (MAX_SRAM_SIZE)
	 *
	 * No space left for dump command !!
	 */

	sram_offset = iee_find_sram_size(base_io_address);

	/*
	 * initialize the SCP
	 */
	scp.scp_sysbus    = 0;
	scp.scp_iscp      = offset_iscp;
	scp.scp_iscp_base = 0;
	WRITE_TO_SRAM((unchar *) &scp, offset_scp, sizeof (scp_t), base_io_address);

	ieep->offset_scb    = sram_offset + 0x0008;

	/*
	 * initialise the ISCP
	 */

	iscp.iscp_busy 		 = 1;
	iscp.iscp_scb_offset = ieep->offset_scb;
	iscp.iscp_scb_base   = 0;
	WRITE_TO_SRAM((unchar *)&iscp, offset_iscp, sizeof (iscp_t)
								 , base_io_address);

	/*
	 * initialise relevant fields of SCB :: other fields shall be
	 * initialized when the board is started/when a command is issued
	 */

	scb.scb_crc_err	    = 0;
	scb.scb_aln_err		= 0;
	scb.scb_rsc_err     = 0;
	scb.scb_ovrn_err    = 0;
	WRITE_TO_SRAM((unchar *)&scb, ieep->offset_scb, sizeof (scb_t)
								, base_io_address);

	/*
	 * initialize driver private structure fields
	 * setup cmd and receive units
	 */
	ieep->offset_gencmd = ieep->offset_scb + sizeof (scb_t);

	iee_build_cu(macinfo);
	iee_build_ru(macinfo);
	
	/*
	 * start the 82586, by resetting the 586 & issuing a CA
	 */

	outb(base_io_address + RESET, RESET_586);	/* reset the 82586 */
	outb(base_io_address + RESET, 0);			/* release from reset */

	outb(base_io_address + CA_CTRL, 1);		/* Go 82586! */

	/*
	 * "busy bit" test
	 */

	for (i = 0; i < 100; i++) 
	{					
		READ_BYTE(offset_iscp + 0, base_io_address, rb);
		if ((rb & 1) == 0)	
			break;
		drv_usecwait(100);
	}
	if (i == 100)	
	{		
        cmn_err(CE_WARN, "iee: ISCP busy bit not cleared");
		return (FAIL);
    } 

	/*
	 * wait till CU finishes executing the command
	 */

	for (i = 0; i < 100; i++) 
	{
		if (inw(scb_statreg) == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait(100);
	}
	if (i == 100)
		return (FAIL);

	if (iee_ack(base_io_address) == FAIL)
		return (FAIL);

	/*
	 * configure 82586 with default parameters; neither loopback
	 * nor promiscuous mode is initially enabled
	 */

	if (iee_config(macinfo, (LPBK_OFF | PRO_OFF)) == FAIL)
		return (FAIL);

	return (SUCCESS);
}


/*
 * Name			: iee_rcv_packet()
 * Purpose		: Get a packet that has been received by the hardware 
 *                and pass it up to gld
 * Called from	: iee_intr()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: None
 */

static
void
iee_rcv_packet(gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	register int  base_io_address = (int) macinfo->gldm_port;
	fd_t	fd_s;           /* frame descriptor structure */
	mblk_t	*mp;            /* pointer to message block */
	ushort	last_flg;       /* last RBD associated with an FD */
	ushort	rb, fd, first_rbd, last_rbd;
							/* offsets of different 82586 structures */
	uint	len;			/* length of data in received buffer */

#ifdef IEEDEBUGX
	dbg_fd = ieep->begin_fd;
#endif IEEDEBUGX

#ifdef IEEDEBUG
	if (ieedebug & IEERECV)
		cmn_err(CE_CONT, "iee_rcv_packet(0x%x)\n", macinfo);
#endif IEEDEBUG

	/*
	 * scan each fd that have been filled up in the FDL
	 */

	for (fd = ieep->begin_fd ; ; fd = ieep->begin_fd) {
		mp = NULL;

		READ_FROM_SRAM(&fd_s, fd, sizeof (fd_t), base_io_address);
		if (!(fd_s.fd_status & CS_CMPLT))
			break;


		/* make certain the chip doesn't try to slip me a bad frame */
		if (!(fd_s.fd_status & CS_OK)) {
			cmn_err(CE_WARN, "CS_OK not set");
			goto frame_done;
		}

		/*
		 * pointers to first and last RBD are needed during the process
		 * of requeueing RBDs at the end of the RBL
		 */

		first_rbd = last_rbd = fd_s.fd_rbd_offset;

		/* skip this frame if it doesn't have a filled buffer */
		if (first_rbd == 0xffff)
			goto frame_done;

#ifdef IEEDEBUGX
		if (ieedebug & IEEINT) {
			if (fd_s.fd_status & 0xfc0)
				dbgmsg("iee: packet error\n");
		}
#endif IEEDEBUGX

		/*
		 * Compute the length of the msgb to be allocated for
		 * the frame, by going through all the RBD's associated
		 * the frame, and adding their lengths.
		 */

		READ_WORD(last_rbd + RBD_STATUS, base_io_address, rb);
		len = 0;
		for (;;) {
			ushort el_flag;

#ifdef IEEDEBUGX
			dbg_rb = rb;
			if (ieedebug & IEEINT) {
				/* The 0xfaa0 is usually correct
				 * if (last_rbd < 0xfaa0)
				 *	dbgmsg("iee: invalid fd=0x%x rbd=0x%x\n", dbg_fd,last_rbd);
				 */
				if (!(rb & 0x4000))
					dbgmsg("iee: frame not valid 0x%x\n", last_rbd);
				if ((rb & CS_RBD_CNT_MASK) > 512)
					dbgmsg("iee: cnt 0x%x not valid 0x%x\n", rb, last_rbd);
			}
#endif IEEDEBUGX

			last_flg = (rb & CS_EOF); /* check for last rbd */
			len += rb & CS_RBD_CNT_MASK; /* length of data */

#ifdef IEEDEBUGX
			if (ieedebug & IEEINT) {
				if (len > 1500) {
					dbgmsg("iee: len=%d not valid\n", len);
				}
			}
#endif IEEDEBUGX

			/* break if this is the last RBD associated with the frame */
			if (last_flg)
				break;

			/* check if EL bit has been set */
			READ_WORD(last_rbd + RBD_SIZE, base_io_address, el_flag);
			if (el_flag & CS_EL) {
				cmn_err(CE_WARN, "iee: out of receive buffers");
				break;
			}

			/* fetch the next receive buffer descriptor and its length */
			READ_WORD(last_rbd + RBD_NXT_OFFSET, base_io_address, last_rbd);
			READ_WORD(last_rbd + RBD_STATUS, base_io_address, rb); 
		} 

#ifdef IEEDEBUGX
		if (ieedebug & IEEINT) {
			if (len < 32)
				dbgmsg("iee: short packet\n");
		}
		dbg_len = len;
#endif IEEDEBUGX

		/*
		 * Before allocating a msgb, do a sanity check on the
		 * length, and reject the frame if it is too big.
		 * Such frames should not be sent up to gld.
		 */

		if (len > IEEMAXPKT) {
			cmn_err(CE_CONT,"?NOTICE: iee: Giant packet of size %d - rejected"
						   , len);
			goto skip_copy;
		}

		len += sizeof (struct ether_header);
		if ((mp = allocb(len, BPRI_MED)) == NULL) {
			cmn_err(CE_WARN, "iee: no STREAMS buffers");
			macinfo->gldm_stats.glds_norcvbuf++;
			goto skip_copy;
		}

		/* copy the ether_header */
		bcopy((caddr_t)fd_s.fd_dest, (caddr_t)mp->b_wptr,
				sizeof (struct ether_header));

		/*
		 * ether_header need NOT be copied again into the 
		 * message block being passed up
		 */
		 
		mp->b_wptr += sizeof (struct ether_header);

	skip_copy:
		/*
		 * If we reach this point and mp is still NULL we have
		 * to discard the buffer data but still reset the
		 * buffer descriptors.
		 */

#ifdef IEEDEBUG 
		/* print only non-broadcast recd. packets */
		if (ieedebug & IEETRACE) {
			if (bcmp((caddr_t) fd_s.fd_dest, (caddr_t) 
						gldbroadcastaddr, ETHERADDRL) != 0)
			{
				cmn_err(CE_CONT, "Destination Address in iee_rcv:");
				IEEPRINT_EADDR(fd_s.fd_dest);
				cmn_err(CE_CONT, "\n");
				cmn_err(CE_CONT, "Source Address in iee_rcv:");
				IEEPRINT_EADDR(fd_s.fd_src);
				cmn_err(CE_CONT, "\n");
			}
		}
#endif IEEDEBUG 

#ifdef IEEDEBUGX
		dbg_cnt = 0;
#endif IEEDEBUGX

		/* copy the data from each receive buffer */
		last_rbd = fd_s.fd_rbd_offset;
		for (;;)
		{
			READ_WORD(last_rbd + RBD_STATUS, base_io_address, rb);
			last_flg = (rb & CS_EOF); /* check for last rbd */
			len = rb & CS_RBD_CNT_MASK; /* length of data */

#ifdef IEEDEBUGX
			if (ieedebug & IEEINT) {
				if (dbg_cnt + len > dbg_len)
					dbgmsg("iee: invalid len, rbd=0x%x rb=0x%x\n", last_rbd
																 , rb);
			}
			dbg_cnt += len;
#endif IEEDEBUGX

			/* skip over zero length buffers; not certain why these happen */
			if (len != 0 && mp != NULL) {
				/* get the address of rb where the actual data is stored */
				READ_WORD(last_rbd + RBD_BUFF, base_io_address, rb);

				/* append to mp */
				READ_FROM_SRAM(mp->b_wptr, rb, len, base_io_address);
				mp->b_wptr += len;
			}

			/* reset status word in this RBD */
			WRITE_WORD(last_rbd + RBD_STATUS, 0, base_io_address);

			READ_WORD(last_rbd + RBD_SIZE, base_io_address, rb); 

#ifdef IEEDEBUGX
			if (ieedebug & IEEINT) {
				if ((rb & CS_RBD_CNT_MASK) == 0) {
					cmn_err(CE_CONT, "iee: rbd_size was zero");
				}
			}
#endif IEEDEBUGX

			/* the chip sometimes clobbers the buffer size, make it valid */
			WRITE_WORD(last_rbd + RBD_SIZE, RECV_BUFFER_SIZE, base_io_address);

			if (last_flg)
				break;
			if (rb & CS_EL)
				break;

			/* get address of next buffer descriptor */
			READ_WORD(last_rbd + RBD_NXT_OFFSET, base_io_address, last_rbd);
		} 

	frame_done:
			
		/*
		 * re-queue rbd and fd to the end of the respective lists
		 */

		/* Make this RBD the new last one */
		WRITE_WORD(last_rbd + RBD_SIZE, (CS_EL | RECV_BUFFER_SIZE)
									  , base_io_address);

		/* Clear the old end-of-list bit */
		WRITE_WORD(ieep->end_rbd + RBD_SIZE, RECV_BUFFER_SIZE, base_io_address);
		ieep->end_rbd = last_rbd;

		/*
		 * requeue FDs also
		 */

		/* Make this FD the new last one */
		ieep->begin_fd = fd_s.fd_nxt_offset;
		fd_s.fd_status   = 0;
		fd_s.fd_cmd      = CS_EL;
		fd_s.fd_rbd_offset = 0xffff;
		WRITE_TO_SRAM((unchar *)&fd_s, fd, sizeof (fd_t), base_io_address);

		/* clear EL bit of old last one */
		WRITE_WORD(ieep->end_fd + FD_CMD, 0, base_io_address);

		ieep->end_fd = fd;

		/*
		 * pass the message block up for further processing
		 * by gld
		 */
		if (mp != NULL)
			gld_recv(macinfo, mp);
	}
	return;
}


/*
 * Name			: iee_restart_ru()
 * Purpose		: Check the status of RU and restart it if necessary
 * Called from	: iee_intr()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: The Receive Unit is restarted
 */

static
void
iee_restart_ru( gld_mac_info_t *macinfo )
{

	/*
	 * Fix up the RFA
	 */
	iee_rfa_fix(macinfo);

	/*
	 * At this point, then RU is not ready and no completed fd's are 
	 * available. So, do a RU start .
	 */
	iee_start_ru(macinfo);

	return;
}


/*
 * Name			: iee_find_sram_size()
 * Purpose		: Find the total size of the Shared Ram (SRAM)
 *                by writing a pattern "witl" and reading it
 *                back
 * Called from	: iee_init_board(), before the 82586 is initialized
 * Arguments	: base_io_address - base address of the board's I/O 
 *				  registers
 * Returns		: Size of the SRAM
 * Side effects	: None
 */

static
ushort
iee_find_sram_size(register int base_io_address)
{
	unchar buf[5]; /* scratch */

	WRITE_TO_SRAM((unchar *)"witl", 0, 4, base_io_address);
	READ_FROM_SRAM((caddr_t)buf, 0x8000, 4, base_io_address);
	if (strncmp((caddr_t)buf, "witl", 4) == 0)
	{
		return(0x8000);
	}
	else
		return(0);
}


/*
 * Name			: iee_build_cu()
 * Purpose		: Initialize the transmit buffer, transmit buffer 
 *                descriptor and command block structures
 * Called from	: iee_init_board(), as part of board initializations
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: None
 */

static 
void 
iee_build_cu (gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	register int base_io_address = (int) macinfo->gldm_port;
	gen_cmd_t cmd;       /* scratch structure */
	tbd_t tbd;           /* scratch structure */

	/*
	 * Initializations ::
	 * ieep->offset_cmd is placed above SCB and ieep->gen_cmd, i.e.
	 * 		at ieep->offset_scb + sizeof (scb_t) + sizeof (gen_cmd_t)
	 */

	ieep->offset_cmd = ieep->offset_scb + sizeof (scb_t) + 
					   sizeof (gen_cmd_t);
	ieep->offset_tbd = ieep->offset_cmd + sizeof (gen_cmd_t);
	ieep->offset_tbuf = ieep->offset_tbd + sizeof (tbd_t);

	/* 
	 * initialize cmd - we set it to CMD_XMIT so that
	 * we don't have to do it every time in send()
	 */

	cmd.cmd_status		= 0;
	cmd.cmd_cmd			= CS_EL | CS_CMD_XMIT | CS_INT;
	cmd.cmd_nxt_offset	= 0xffff;
	cmd.parameter.prm_xmit.xmit_tbd_offset 	= ieep->offset_tbd;
	WRITE_TO_SRAM((unchar *)&cmd, ieep->offset_cmd, sizeof (gen_cmd_t)
								, base_io_address);

	/* 
	 * initialize tbd 
	 */

	tbd.tbd_count		= 0;
	tbd.tbd_nxt_offset	= 0xffff;
	tbd.tbd_buff		= ieep->offset_tbuf;
	tbd.tbd_buff_base	= 0;
	WRITE_TO_SRAM((unchar *)&tbd, ieep->offset_tbd, sizeof (tbd_t)
								, base_io_address);
}


/*
 * Name			: iee_build_ru()
 * Purpose		: Initialize the receive buffers, frame and buffer 
 *                descriptors in the shared RAM
 * Called from	: iee_init_board, as part of board initialization
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: None
 * Side effects	: None
 */

static 
void
iee_build_ru (gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	register int  base_io_address = (int) macinfo->gldm_port;
	int	mem_left;           /* mem left after allocating other structs */
	ushort num_fds;         /* number of FD's allocated */
	ushort num_rbds;        /* number of RBD's allocated */
	uint ratio;             /* Ratio of the above */
	ushort offset_rbuf = 0; /* scratch pointers */
	ushort offset_rbd = 0;  /* scratch pointers */
	ushort offset_fd = 0;   /* scratch pointers */
	fd_t fd;                /* scratch structure */
	rbd_t rbd;              /* scratch structure */
	ushort i, rb;           /* scratch */

	/* 
	 * receive buffer starts at the end of transmit data area 
	 */

	offset_rbuf = ieep->offset_scb + (sizeof (scb_t) + 2 * sizeof (gen_cmd_t)
				  + sizeof (tbd_t) + XMIT_BUFFER_SIZE);
	mem_left = (MAX_SRAM_SIZE - sizeof (scp_t) - sizeof (iscp_t)) - offset_rbuf; 
	/* 
	 * calculate number of rbd and fd 
	 */

	ratio = (int) (MAX_BUFFER_SIZE / RECV_BUFFER_SIZE);

	/*
	 * Divide fds, rbds and rcv buffers in the proportion 
	 * 1 : ratio : ratio
	 */

	num_fds = (int) (mem_left / (sizeof (fd_t) + ratio * sizeof (rbd_t) +
				  				 ratio * RECV_BUFFER_SIZE));
	num_rbds = (int) ((mem_left - num_fds * (sizeof (fd_t))) /
                         (sizeof (rbd_t) + RECV_BUFFER_SIZE));

	/* 
	 * rbd's follow rbuf's and fd's follow rbd's 
	 */

	offset_rbd = offset_rbuf + num_rbds * RECV_BUFFER_SIZE;
	offset_fd  = offset_rbd + num_rbds * sizeof (rbd_t);

	ieep->offset_fd = ieep->begin_fd = offset_fd;
	ieep->num_fds = num_fds;
	ieep->offset_rbd = ieep->begin_rbd = offset_rbd;
	ieep->num_rbds = num_rbds;

	/* 
	 * Set up FD list
	 */

	for (i = 0; i < num_fds; i++) 
	{
		fd.fd_status		= 0;
		fd.fd_cmd			= 0;
		fd.fd_nxt_offset		= offset_fd + sizeof (fd_t);
		fd.fd_rbd_offset		= 0xffff;
		WRITE_TO_SRAM((unchar *)&fd, offset_fd, sizeof (fd_t), base_io_address);
		offset_fd 			+= sizeof (fd_t);
	}
	ieep->end_fd = offset_fd - sizeof (fd_t);

	/* Set up link between the first FD and the first RBD */
	WRITE_WORD(ieep->begin_fd + FD_RBD_OFFSET, offset_rbd, base_io_address);	

	/* Set end-of-list flag in the last FD */
	WRITE_WORD(ieep->end_fd + FD_CMD, CS_EL, base_io_address);	

	/* The last FD points to the first FD */
	WRITE_WORD(ieep->end_fd + FD_NXT_OFFSET, ieep->begin_fd, base_io_address);	
   
	/*
	 * Setup RBD list
	 */

	for (i = 0; i < num_rbds; i++) 
	{
		rbd.rbd_status		= 0;
		rbd.rbd_nxt_offset	= offset_rbd + sizeof (rbd_t);
		rbd.rbd_buff		= offset_rbuf;
		rbd.rbd_buff_base	= 0;
		rbd.rbd_size		= RECV_BUFFER_SIZE;
		WRITE_TO_SRAM((unchar *)&rbd, offset_rbd, sizeof (rbd_t), 
					  base_io_address);
		offset_rbd			+= sizeof (rbd_t);
		offset_rbuf			+= RECV_BUFFER_SIZE;
	}

	ieep->end_rbd = offset_rbd - sizeof (rbd_t);

	/*
	 * For the last RBD,
	 * a) Link points to first RBD
	 * b) EL bit is set
	 */

	WRITE_WORD(ieep->end_rbd + RBD_NXT_OFFSET, ieep->begin_rbd
											 , base_io_address);	
	WRITE_WORD(ieep->end_rbd + RBD_SIZE, (RECV_BUFFER_SIZE | CS_EL)
									   , base_io_address);
}

/*
 * Name			: iee_rfa_fix()
 * Purpose		: Fix up the Receive Frame (and Buffer) Area after
 *				  a No Resources interrupt.
 * Called from	: iee_restart_ru
 * Arguments	: ieep - pointer to a ieeinstance structure
 * Returns		: None
 * Side effects	: The receive area on the board is re-established
 */

void
iee_rfa_fix( gld_mac_info_t *macinfo )
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	register int  base_io_address = (int) macinfo->gldm_port;

	/*
	 * Reset the RFA.  Since we have at least one RBD per RFD,
	 * we cannot run out of RFDs and still have RBDs.  If we simply
	 * reset the beginning and end pointers, and set the first
	 * RFD to point to the first RBD, we should be ready to start
	 * again.  We do not have to re-set-up the whole RFA.
	 */

	/* clear the EL flag and reset the size in old end of RBD list */
	WRITE_WORD(ieep->end_rbd + RBD_SIZE, RECV_BUFFER_SIZE, base_io_address);

	/* clear the EL flag and reset the size in old end of FD list */
	WRITE_WORD(ieep->end_fd + FD_CMD, 0, base_io_address);	

	/* clear the status word in old end of FD list */
	WRITE_WORD(ieep->end_fd + FD_STATUS, 0, base_io_address);	


	/* reset begin and end pointers to their original offsets*/
	ieep->begin_fd = ieep->offset_fd;
	ieep->end_fd = ieep->begin_fd + (ieep->num_fds * sizeof (fd_t));
	ieep->end_rbd = ieep->offset_rbd + (ieep->num_rbds * sizeof (rbd_t));

	/* Set end-of-list flag in the last FD */
	WRITE_WORD(ieep->end_fd + FD_CMD, CS_EL, base_io_address);	

	/* clear status word in the last FD */ 
	WRITE_WORD(ieep->end_fd + FD_STATUS, 0, base_io_address);	

	/* Set up link between the first FD and the first RBD */
	WRITE_WORD(ieep->begin_fd + FD_RBD_OFFSET, ieep->offset_rbd
											 , base_io_address);	

	/* Set the end-of-list bit in the last RBD */
	WRITE_WORD(ieep->end_rbd + RBD_SIZE, (RECV_BUFFER_SIZE | CS_EL)
									   , base_io_address);

	return;
}


/*
 * Name			: iee_config()
 * Purpose		: Configure the 82586
 * Called from	: iee_init_board(), as part of board initialization
 *				  iee_prom(), when a switch to/from promiscuous mode
 *							  is desired
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 *				  flag    - indicating which of the modes are to be 
 *                          enabled (loopback, promiscuous mode or 
 *                          both). Possible values are
 *							LPBK_MASK : enable loopback mode
 *							PRM_MASK  : enable promiscuous mode
 *							LPBK_MASK | PRM_MASK : both 
 *							0 : neither loopback nor promiscuous mode 
 * Returns		: SUCCESS if the board was successfully configured to 
 *                the desired configuration
 *				  FAIL    otherwise
 * Side effects	: None
 */

static
int
iee_config(gld_mac_info_t *macinfo, ushort flag)
{
	struct ieeinstance *ieep = (struct ieeinstance *)macinfo->gldm_private;
	register int base_io_address;
	ushort scb_cmdreg = (SCB_CMDREG | macinfo->gldm_port);
	ushort scb_cblreg = (SCB_CBLREG | macinfo->gldm_port);
	conf_t conf;     /* scratch config structure */

	base_io_address = macinfo->gldm_port;

	if (iee_wait_scb(1000, base_io_address, "config") == FAIL) {
		return (FAIL);
	}

	outw(scb_cmdreg, SCB_CUC_START); /* command */
	outw(scb_cblreg, ieep->offset_gencmd); /* cbl */

	/*
	 * initialise the status, command and link fields of the
	 * transmit cmd structure. Note that we use the autoincrementing
	 * feature of DX_REG here for the three successive outw's
	 */

	WRITE_WORD(ieep->offset_gencmd + CMD_STATUS, 0, base_io_address);	
	outw(base_io_address + DX_REG, CS_CMD_CONF | CS_EL);	
	outw(base_io_address + DX_REG, 0xffff);

	/* default configuration as in 586 manual */

	bzero((caddr_t) &conf, sizeof (conf_t));
	conf.word1.byte_count = 0xc;
	conf.word1.fifo_lim   = 0x8;

	conf.word2.addr_len = 6;
	conf.word2.pream_len = 2;
	conf.word2.ext_lpbk = flag & LPBK_MASK ? 1 : 0;

	conf.word3.if_space = 0x60;

	conf.word4.slot_time = 0x200;
	conf.word4.retrylim = 0xf;

	conf.word5.prm = flag & PRO_ON ? 1 : 0;

	conf.word6.min_frame_len = 0x40;

	WRITE_TO_SRAM((unchar *)&conf, ieep->offset_gencmd + CMD_PARAMETER, 
				 sizeof (conf_t), base_io_address);

	/* Go 82586! */
	outb(base_io_address + CA_CTRL, 1);

	if (iee_wait_scb(1000, base_io_address, "config2") == FAIL) {
		return (FAIL);
	}

	if (iee_ack(base_io_address) == FAIL)
		return (FAIL);

	return (SUCCESS);
}


/*
 * Name			: iee_wait_scb()
 * Purpose		: Detect if the 82586 has accepted the last command 
 *                issued to it
 * Called from	: Any function that needs to issue a command to the 
 *                82586 or read the latest status of the 82586
 * Arguments	: how_long - time out limit for busy-wait
 *				  base_io_address  - base address of the board's I/O 
 *                registers
 * Returns		: SUCCESS if the 82586 had accepted the previous command
 *				  FAIL    otherwise
 * Side effects	: None
 */

static
int
iee_wait_scb (int how_long, register int base_io_address, char * msg)
{
	ushort scb_cmdreg = (SCB_CMDREG | base_io_address);
	ushort  i = 0; /* scratch */
	ushort	rb;    /* scratch */

	/*
	 * 82586 clears the SCB command field when it has successfully
	 * accepted the command
	 */

	while (i++ != how_long)
	{
		rb = inw(scb_cmdreg); 
		if (rb == 0)
			return (SUCCESS); /* scb has been successfully cleared */
		drv_usecwait(10);
	}
	cmn_err(CE_WARN, "iee: %s: wait scb failed", msg);
	return (FAIL);
}


/*
 * Name			: iee_wait_active()
 * Purpose		: Checks whether the previous command is complete or not 
 * Called from	: iee_send(), before giving the command 
 * Arguments	: scb_p - Pointer to the scb_t structure
 * 				  count - time out limit for busy-wait
 * Returns		: SUCCESS if the 82586 had completed the previous command
 *				  FAILURE    otherwise
 * Side effects	: None
 */

static int
iee_wait_active (int count, register int base_io_address)
{
	ushort scb_statreg = (SCB_STATREG | base_io_address);
	ushort  i = 0; /* scratch */
	ushort	rb;    /* scratch */

	/*
	 * 82586 clears the SCB command field when it has successfully
	 * accepted the command
	 */

	while (i++ != count) 
	{
		rb = inw(scb_statreg); 
		if (!(rb & SCB_CUS_ACTIVE))
			return (SUCCESS); /* scb has been successfully cleared */
		drv_usecwait(10);
	}
	return (FAIL);
}


/*
 * Name			: iee_ack()
 * Purpose		: Acknowledge the interrupt sent by the 82586
 * Called from	: Any function that desires to acknowledge an interrupt
 * Arguments	: base_io_address - base address of the board's I/O 
 *                registers
 * Returns		: SUCCESS if the interrupt was successfully acknowledged
 *				  FAIL    otherwise
 * Side effects	: None
 */

static
int
iee_ack(register int base_io_address)
{
	ushort scb_statreg = (SCB_STATREG | base_io_address);
	ushort scb_cmdreg = (SCB_CMDREG | base_io_address);
	ushort cmd; /* cmd written back as ack */
	
	cmd = inw(scb_statreg) & SCB_INT_MASK;
	if (cmd)
	{	
		outw(scb_cmdreg, cmd); 
		outb(base_io_address + CA_CTRL, 1);
		return (iee_wait_scb(10000, base_io_address, "ack"));
	}
	return (SUCCESS);
}



/*
 *					Routines interfacing with EEPROM
 */


/*
 * Name			: peek_eeprom()
 * Purpose		: read a word from the eeprom
 * Called from	: iee_attach(), for reading the Ethernet address
 * Arguments	: base_io_address    - base address of the board's I/O regs
 *                register_number - Ethernet address register number in EEPROM
 * Returns		: one short word of data
 * Side effects	: None
 */

static
ushort
peek_eeprom(int base_io_address, ushort register_number)
{
	unchar byte; /* scratch val - written to RESET reg */
	ushort data; /* val returned */

	/*
	 * Reset the 586.
	 * GA_RESET bit of RESET register is write_only; so take care
	 * Clear EEDO, EEDI, EESK bits and enable EECS bit of RESET register
	 */

	byte = inb(base_io_address + RESET);
	byte &= ~(GA_RESET | EEDO | EEDI | EESK);
	byte |= (RESET_586 | EECS);
	outb(base_io_address + RESET, byte);

	/*
	 * Accesses to EEPROM are done with serial I/O
	 * Output command to RESET register first followed by the register
	 * number to read
	 * Then, read the bits out of the EEPROM using serial I/O
	 */

	serial_write(base_io_address, EEPROM_READ_OPCODE_REG, 3);
	serial_write(base_io_address, register_number, 6);
	data = serial_read(base_io_address);

	/*
	 * clean up the mess :: remember that access to the 82586 chip and
	 * EEPROM are mutually exclusive
	 */

	byte = inb(base_io_address + RESET) & ~(GA_RESET | EEDI | EECS);

	/*
	 * signal to board by raising and lowering clock
	 */

	outb(base_io_address + RESET, byte | EESK);
	drv_usecwait(25);
	outb(base_io_address + RESET, byte);
	drv_usecwait(25);

	return (data);
}


/*
 * Name			: serial_read()
 * Purpose		: read and return one short word of data from EEPROM
 * Called from	: peek_eeprom()
 * Arguments	: base_io_address - base address of the board's I/O regs
 * Returns		: None
 * Side effects	: None
 */

static
ushort
serial_read(int base_io_address)
{
	ushort value = 0; /* value returned */
	ushort x = inb(base_io_address + RESET) & ~(GA_RESET | EEDO | EEDI);
	int i;            /* scratch */

	/*
	 * Read 16 bits of data, msb down to lsb.
	 * For each read, 
	 * if EEDO is set, => corresponding bit is 1, else 0
	 * clock has to be raised before, and lowered after, the read.
	 */

	for (i = 15; i >= 0; i--)
	{
		outb(base_io_address + RESET, x | EESK); /* raise clock */
		drv_usecwait(25);
		x = inb(base_io_address + RESET) & ~(GA_RESET | EEDI);
		if (x & EEDO)
			value |= (1 << i);
		outb(base_io_address + RESET, (x & ~EESK)); /* lower clock */
		drv_usecwait(25);
	}
	return (value);
}


/*
 * Name			: serial_write()
 * Purpose		: write 'data' to EEPROM
 * Called from	: peek_eeprom()
 * Arguments	: data            - data that is to be written to EEPROM
 *                length          - length of data that is to be written
 *                base_io_address - base address of the board's I/O regs
 * Returns		: None
 * Side effects	: None
 */

void
serial_write(int base_io_address, ushort data, ushort length)
{
	ushort bitmask = (1 << (length - 1));
	ushort x = 0;
	const unchar init_byte =
				inb(base_io_address + RESET) & ~(GA_RESET | EEDO | EEDI);

	/*
	 * output bit by bit. 
	 * For every bit set, set EEDI, else don't.
	 */
	for (; bitmask; bitmask >>= 1)
	{
		if (data & bitmask)
			x = init_byte | EEDI;
		else
			x = init_byte;

		outb(base_io_address + RESET, x);
		drv_usecwait(25);

		/*
		 * Signal to board by raising and lowering clock
		 */

		outb(base_io_address + RESET, x | EESK);
		drv_usecwait(25);
		outb(base_io_address + RESET, x);
		drv_usecwait(25);
	}
	outb(base_io_address + RESET, x & ~EEDI);
}


/*
 *				Routines implementing the ioctls
 */

/*
 * Name			: iee_tdr_test()
 * Purpose		: Perform the TDR test to detect presence of any
 *				  cable/tranceiver faults
 * Called from	: iee_ioctl()
 * Arguments	: macinfo - pointer to a gld_mac_info_t structure
 * Returns		: Result of the TDR test outputted by 82586
 * Side effects	: None
 */

static
int
iee_tdr_test(gld_mac_info_t *macinfo)
{
	struct ieeinstance *ieep  = (struct ieeinstance *)macinfo->gldm_private;
	register int base_io_address = (int) macinfo->gldm_port;
	ushort scb_cmdreg         = (SCB_CMDREG | macinfo->gldm_port);
	ushort scb_cblreg         = (SCB_CBLREG | macinfo->gldm_port);
	ushort stat = 0;   /* scratch */
	int	i;             /* scratch */

	WRITE_WORD(ieep->offset_gencmd, CMD_STATUS, base_io_address);	

	/*
	 * command: TDR test; CS_INT not set, since we have to busy_wait
	 */

	outw(base_io_address + DX_REG, CS_EL | CS_CMD_TDR);
	outw(base_io_address + DX_REG, 0xffff);	
	outw(base_io_address + DX_REG, 0); /* result word cleared to zero initially */

	if (iee_wait_scb(1000, base_io_address, "tdr_test") == FAIL) {
		return (FAIL);
	}
	outw(scb_cmdreg, SCB_CUC_START); 
	outw(scb_cblreg, ieep->offset_gencmd); 

	/* Go 82586! */
	outb(base_io_address + CA_CTRL, 1);	

	/*
	 * busy wait for the status word of command to be set to CS_CMPLT
	 */
	for (i = 0; i < 1000; i++)
	{
		READ_WORD(ieep->offset_gencmd + CMD_STATUS, base_io_address, stat); 
		if (stat & CS_CMPLT) /* check if command is completed */
			break;
		drv_usecwait(100);
	}
	if (i < 1000)
	{
		if (stat & CS_OK == 0) /* unsuccessful completion? */
			return (0);
		READ_WORD(ieep->offset_gencmd + CMD_PARAMETER, base_io_address, stat);
	}
	return (stat);
}
