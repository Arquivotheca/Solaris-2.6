/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */
#ident "@(#)elx.c	1.29	96/06/11 SMI"

/*
 * elx -- 3COM EtherLink III family of Ethernet controllers
 * Depends on the Generic LAN Driver utility functions in /kernel/misc/gld
 */

#if defined(i386)
#define	_mca_bus_supported
#define	_eisa_bus_supported
#define	_isa_bus_supported
extern int eisa_nvm();
#endif

#if defined(__ppc)
#define	_isa_bus_supported
#endif

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
#include "sys/gld.h"
#include "sys/byteorder.h"
#include "sys/elx.h"
#if defined(_eisa_bus_supported)
#include "sys/eisarom.h"
#include "sys/nvm.h"
#endif

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "3COM EtherLink III";

#ifdef ELXDEBUG
/* used for debugging */
int	elxdebug = 0x0;
#endif

/* Required system entry points */
static int elxdevinfo (dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg,
    void **result);
static int elxprobe(dev_info_t *devinfo);
static int elxattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd);
static int elxdetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd);
static void elx_repins(int, unsigned char *, int);
static void elx_repouts(int, unsigned char *, int);

/* Required driver entry points for GLD */
int	elx_reset (gld_mac_info_t *);
int	elx_start_board (gld_mac_info_t *);
int	elx_stop_board (gld_mac_info_t *);
int	elx_saddr (gld_mac_info_t *);
int	elx_dlsdmult (gld_mac_info_t *, struct ether_addr *, int);
int	elx_prom (gld_mac_info_t *, int);
int	elx_gstat (gld_mac_info_t *);
int	elx_send (gld_mac_info_t *, mblk_t *);
u_int	elxintr (gld_mac_info_t *);
static void elx_isa_sort(short *boards, int nboards);
static int elx_readid_prom(int id, int addr);
static void elx_isa_idseq(int id);
static int get_bustype(dev_info_t *devi);
static void elx_getp(gld_mac_info_t *macinfo);
static void elx_msdelay(int ms);
static void elx_discard(gld_mac_info_t *macinfo, struct elxinstance *elxvar,
    int port);
static int elx_init_board (gld_mac_info_t *macinfo);
static int elx_isa_init (int id, short *boards, int max);
static int elx_conv(char *str);
static int elx_verify_setup (dev_info_t *devinfo, int port, int irq,
    int media);
#if defined(_mca_bus_supported)
static void elx_readpos(int slot, unchar pos[6]);
#endif

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/* Standard Streams initialization */

static struct module_info minfo = {
	ELXIDNUM, "elx", 0, INFPSZ, ELXHIWAT, ELXLOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

struct streamtab elxinfo = {&rinit, &winit, NULL, NULL};

/* Generic 3C509/3C579 information */
	/* default I/O addresses for ISA bus */
#ifdef notdef
static int elxioaddr[] = {
	0x200, 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0x270,
	0x280, 0x290, 0x2a0, 0x2b0, 0x2c0, 0x2d0, 0x2e0, 0x2f0,
	0x300, 0x310, 0x320, 0x330, 0x340, 0x350, 0x360, 0x370,
	0x380, 0x390, 0x3a0, 0x3b0, 0x3c0, 0x3d0, 0x3e0
};
#endif

/* Standard Module linkage initialization for a Streams driver */

static 	struct cb_ops cb_elxops = {
	nodev,			/* cb_open */
	nodev,			/* cb_close */
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
	&elxinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

struct dev_ops elxops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	elxdevinfo,		/* devo_getinfo */
	nulldev,		/* devo_identify */
	elxprobe,		/* devo_probe */
	elxattach,		/* devo_attach */
	elxdetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_elxops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&elxops			/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

static kmutex_t elx_probe_lock;

int
_init (void)
{
	int	status;

	mutex_init (&elx_probe_lock, "elx probe serializer",
		    MUTEX_DRIVER, NULL);

	status = mod_install(&modlinkage);
	if (status != 0) {
		mutex_destroy(&elx_probe_lock);
	}
	return (status);
}

int
_fini (void)
{
	int	status;

	status = mod_remove(&modlinkage);
	if (status != 0)
		return (status);

	mutex_destroy (&elx_probe_lock);

	return (0);
}

int
_info (struct modinfo *modinfop)
{
	return mod_info(&modlinkage, modinfop);
}

/*
 *  DDI Entry Points
 */

/* getinfo(9E) -- Get device driver information */

/*ARGSUSED*/
static int
elxdevinfo (dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;

	/* This code is not DDI compliant: the correct semantics */
	/* for CLONE devices is not well-defined yet.            */
	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		/* we really should not be using 'devinfo' here */
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
	return error;
}

/* probe(9E) -- Determine if a device is present */

static int
elxprobe(dev_info_t *devinfo)
{
	int len, irq = 0;
	int found_board = 0;
	int *ioaddr, iolen;
	int port = 0;
	int media = 0;
#if defined(_eisa_bus_supported)
	NVM_SLOTINFO *nvm;
	NVM_FUNCINFO *func;
	caddr_t data;
	int	i;
#endif
#if defined(_mca_bus_supported)
	unchar mcapos[6];
#endif
#if defined(_eisa_bus_supported) || defined(_mca_bus_supported)
	static int lastslot = 0;
	int slot;
#endif

#ifdef ELXDEBUG
	if (elxdebug & ELXDDI)
		cmn_err(CE_CONT, "elxprobe(0x%x)", devinfo);
#endif
	/* defer if anyone else is probing (MP systems) */
	mutex_enter (&elx_probe_lock);

	iolen = 0;
	if (ddi_getlongprop (DDI_DEV_T_ANY, devinfo,
				DDI_PROP_DONTPASS, "ioaddr",
				(caddr_t) &ioaddr, &iolen) != DDI_PROP_SUCCESS) {
	   iolen = 0;
	} else iolen /= sizeof (int);

	/*
	 *  Probe for the board to see if it's there
	 */
	switch (get_bustype(devinfo)) {
#if defined(_mca_bus_supported)
	case ELX_MCA:		/* microchannel boards */
		for (slot = lastslot + 1;
		     slot < ELX_MCA_MAXSLOT && found_board == 0; slot++) {

		   /* get the MCA POS registers */
		   elx_readpos (slot, mcapos);

		   switch (*(ushort *)mcapos) {
		    case ELMCA_10BASE2:
		    case ELMCA_10BASET:
		    case ELMCA_TPONLY:
		      /* only consider enabled boards */
		      if (!(mcapos[ELX_POS_CDEN] & ELX_CDEN))
			continue;
		      port = mcapos[ELX_POS_IO_XCVR] >> ELXPOS_IOSHIFT;
		      port = ELXPOS_ADDR (port);
		      SET_WINDOW (port, 0);

		      /* although we really should have a board, check further */
		      if (inw (port + ELX_MFG_ID) != EL_3COM_ID)
			continue;

		      irq = mcapos[ELX_POS_IRQ] & ELXPOS_IRQ_MASK;
		      media = inw (port + ELX_ADDRESS_CFG);
		      if (elx_verify_setup (devinfo, port, irq, media)) {
			 found_board = 1;
			 lastslot = slot;
		      }
		      break;
		    default:
		      /* we don't care about non-matching boards */
		      break;
		   }
		}
		break;
#endif /* defined(_mca_bus_supported) */

#if defined(_eisa_bus_supported)
	case ELX_EISA:		/* EISA boards and ISA in EISA emulation */
		/* if an ioaddr is specified, this is ISA so skip EISA search */
		/* when EISA autoconf is setup, don't search */
		if (iolen != 0)
		  goto isa;
				/*
				 * board could be either a 3C509 (ISA) or a
				 * 3C579 (EISA) - both in EISA address mode
				 */
		   data = (caddr_t)kmem_zalloc (ELX_MAX_EISABUF, KM_NOSLEEP);
		   if (data == NULL)
		     break;

		for (slot = lastslot + 1; slot < EISA_MAXSLOT; slot++) {
		   int value;
				/*
				 * 3C579 boards have a copy of window 0
				 * at slot address + 0xC80
				 * {this seems to not be true all the time}
				 */
		   i = eisa_nvm (data, EISA_SLOT, slot);
		   if (i == 0)
		     continue;
		   nvm = (NVM_SLOTINFO *)(data + sizeof (short));
		   if (!gld_check_boardid(nvm, EL_3COM_ID)) {
			continue;
		   }

		   /* now look the info over */

		   for (i=0, func = (NVM_FUNCINFO *)(nvm+1); i < nvm->functions; i++, func++) {
		      int more;
		      /* check all functions present and extract info */

		      if (func->fib.init) {
			 NVM_INIT *init = (NVM_INIT *)(func->un.r.init);
			 do {
			    more = init->more;
			    switch (init->type) {
			     case NVM_IOPORT_BYTE:
			       if (init->mask) {
				  init = (NVM_INIT *)(((caddr_t)init)+5);
			       } else {
				  init = (NVM_INIT *)(((caddr_t)init)+3);
			       }
			       break;
			     case NVM_IOPORT_WORD:
			       if ((init->port&0xf) == ELX_ADDRESS_CFG){
				  media = init->un.word_vm.value;
			       }
			       if (init->mask) {
				  init = (NVM_INIT *)(((caddr_t)init)+7);
			       } else {
				  init = (NVM_INIT *)(((caddr_t)init)+5);
			       }
			       break;
			     case NVM_IOPORT_DWORD:
			       if (init->mask) {
				  init = (NVM_INIT *)(((caddr_t)init)+11);
			       } else {
				  init = (NVM_INIT *)(((caddr_t)init)+7);
			       }
			       break;
			    }
			 } while (more);
		      }
		      if (func->fib.irq) {
			 irq = func->un.r.irq[0].line;
		      }
		   }
		   port = ddi_getprop (DDI_DEV_T_NONE, devinfo, DDI_PROP_DONTPASS,
				       "ioaddr", 0);

		   /* check to see if this is second time being probe/attached */
		   if (port != 0 && port != (slot * 0x1000)) {
		      continue;
		   }

		   port = slot * 0x1000 + 0xC80;
		   value = inw (port + ELX_MFG_ID);
		   if (value != EL_3COM_ID){
		      continue;
		   }

		   value = inw (port + ELX_PRODUCT_ID);
		   if ((value & EL_PRODID_MASK) != EL_PRODUCT_ID){
		      continue;
		   }

		   port = slot * 0x1000;

		   SET_WINDOW (port, 0);
		   if (elx_verify_setup (devinfo, port, irq, media))
		     found_board++;

		   break;
		}
		lastslot = slot;
		kmem_free (data, ELX_MAX_EISABUF);
		if (found_board)
		  break;
		/* FALLTHROUGH */
#endif defined(_eisa_bus_supported)

#if defined(_isa_bus_supported)
	case ELX_ISA:		/* ISA only boards */
	      isa:
		/*
		 * note that most ISA boards would have been found above for EISA
		 * based systems.  This is just to catch those not configured and
		 * those on a real ISA system.
		 */
		{
		   static int nboards, lastboard;
		   static short boards[ELX_MAX_ISA];
		   if (nboards == 0) {
			   char idportstr[8];
			   int idport = ELX_IDPORT_BASE;
			   len = sizeof (idportstr);
			   if (ddi_getlongprop_buf (DDI_DEV_T_ANY,
						    ddi_root_node (), 0, "idport-3c509",
						    (caddr_t)idportstr, &len) ==
			       DDI_PROP_SUCCESS) {
				   idport = elx_conv(idportstr);
			   }
			   nboards = elx_isa_init(idport, boards, ELX_MAX_ISA);
		   }
		   if (nboards != 0 && lastboard < nboards) {
		      int value;
		      port = boards[lastboard++];
		      SET_WINDOW (port, 0);
		      value = inw (port + ELX_PRODUCT_ID);
		      irq = (inw (port + ELX_RESOURCE_CFG) >> 12) & 0xF;
		      media = inw (port + ELX_ADDRESS_CFG);
		      if ((value & EL_PRODID_MASK) != EL_PRODUCT_ID){
			 break;
		      }
		      if (elx_verify_setup (devinfo, port, irq, media)) {
			 found_board++;
		      }
		   }
		}
	       	break;
#endif /* defined(_isa_bus_supported) */
	default:
		cmn_err(CE_WARN, "!elx: unknown bus type 0x%x!",
			get_bustype(devinfo));
		break;
	}

	mutex_exit (&elx_probe_lock); /* the next one can run, if any */

	/*
	 *  Return whether the board was found.  If unable to determine
	 *  whether the board is present, return DDI_PROBE_DONTCARE.
	 */
	if (found_board)
	   	return DDI_PROBE_SUCCESS;
	else
		return DDI_PROBE_FAILURE;
}

/*
 *  attach(9E) -- Attach a device to the system
 *
 *  Called once for each board successfully probed.
 */

static int
elxattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	int port, i, value;
	static char media[] = {GLDM_TP, GLDM_AUI, GLDM_UNKNOWN, GLDM_BNC};
	struct elxinstance *elxp;

#ifdef ELXDEBUG
	if (elxdebug & ELXDDI)
		cmn_err(CE_CONT, "elxattach(0x%x)", devinfo);
#endif

	if (cmd != DDI_ATTACH)
		return DDI_FAILURE;

	/*
	 *  Allocate gld_mac_info_t and elxinstance structures
	 */
	macinfo = (gld_mac_info_t *)kmem_zalloc(sizeof (struct elxinstance) +
			sizeof (gld_mac_info_t),	KM_NOSLEEP);
	if (macinfo == NULL)
		return DDI_FAILURE;

	/*  Initialize our private fields in macinfo and elxinstance */
	macinfo->gldm_private = (caddr_t)(macinfo+1);
	elxp = (struct elxinstance *)macinfo->gldm_private;

	port = macinfo->gldm_port = ddi_getprop (DDI_DEV_T_ANY, devinfo,
			DDI_PROP_DONTPASS, "ioaddr", 0);
	macinfo->gldm_irq_index = ((long) ddi_get_driver_private (devinfo)) & 0xFF;
	elxp->elx_irq = (((long) ddi_get_driver_private (devinfo)) >> 8) & 0xFF;
	macinfo->gldm_reg_index = -1;
	macinfo->gldm_state = ELX_IDLE;
	macinfo->gldm_flags = 0;

	elxp->elx_bus = get_bustype (devinfo);
	if (elxp->elx_bus == ELX_EISA && macinfo->gldm_port < 0x1000)
	  elxp->elx_bus = ELX_ISA;

	elxp->elx_rxbits = ELRX_INIT_RX_FILTER;	/* initial receiver filter */

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */

	macinfo->gldm_reset   = elx_reset;
	macinfo->gldm_start   = elx_start_board;
	macinfo->gldm_stop    = elx_stop_board;
	macinfo->gldm_saddr   = elx_saddr;
	macinfo->gldm_sdmulti = elx_dlsdmult;
	macinfo->gldm_prom    = elx_prom;
	macinfo->gldm_gstat   = elx_gstat;
	macinfo->gldm_send    = elx_send;
	macinfo->gldm_intr    = elxintr;
	macinfo->gldm_ioctl   = NULL;    /* if you have one, NULL otherwise */

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */

	/***** Adjust the following values as necessary  *****/
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = ELXMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;

	/*
	 *  Do anything necessary to prepare the board for operation
	 *  short of actually starting the board.
	 */
	SET_WINDOW (port, 0);

	/***** set the connector/media type if it can be determined *****/
	if ((macinfo->gldm_media = ddi_getprop (DDI_DEV_T_NONE, devinfo,
						DDI_PROP_DONTPASS, "media",
						GLDM_UNKNOWN)) == GLDM_UNKNOWN) {
	   value = inw (port + ELX_ADDRESS_CFG);
	   value = ((ulong)ddi_get_driver_private(devinfo)) >> 16;
	   macinfo->gldm_media = media[(value >> 14) & 0x3];
	}

	elx_init_board (macinfo);

	/* Get the board's vendor-assigned hardware network address */
	SET_WINDOW (port, 0);
	for (i = 0; i < 3; i ++) {
	   	/* Address is in the EEPROM - this takes time */
	   	outw(port + ELX_EEPROM_CMD, EEPROM_CMD(EEPROM_READ, i));
		while ((value = inw (port + ELX_EEPROM_CMD)) & EEPROM_BUSY)
		  /* wait */
		  ;
		*(((u_short *)(macinfo->gldm_vendor))+i) =
			ntohs(inw (port + ELX_EEPROM_DATA));
	}

	/* check software configuration register */

	outw(port + ELX_EEPROM_CMD, EEPROM_CMD (EEPROM_READ, EEPROM_SOFTINFO));
	while ((value = inw (port + ELX_EEPROM_CMD)) & EEPROM_BUSY)
	  /* wait */;
	elxp->elx_softinfo = inw (port + ELX_EEPROM_DATA);


	/* must leave the EEPROM data showing the Product ID */
	outw(port + ELX_EEPROM_CMD, EEPROM_CMD(EEPROM_READ, EEPROM_PROD_ID));

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);
	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);

	/* Make sure we have our address set */
	elx_saddr(macinfo);

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
	if (gld_register(devinfo, "elx", macinfo) == DDI_SUCCESS)
		return DDI_SUCCESS;
	else {
		kmem_free(macinfo,
			sizeof (gld_mac_info_t)+sizeof (struct elxinstance));
		return DDI_FAILURE;
	}
}

/*  detach(9E) -- Detach a device from the system */

static int
elxdetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */

#ifdef ELXDEBUG
	if (elxdebug & ELXDDI)
		cmn_err(CE_CONT, "elxdetach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return DDI_FAILURE;
	}

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);

	/* stop the board if it is running */
	(void)elx_stop_board(macinfo);
	outw(macinfo->gldm_port + ELX_COMMAND, COMMAND (ELC_GLOBAL_RESET, 0));
	elx_msdelay(4);

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
		kmem_free(macinfo,
			sizeof (gld_mac_info_t)+sizeof (struct elxinstance));
		return DDI_SUCCESS;
	}
	return DDI_FAILURE;
}

/*
 * elx_poll_cip()
 *	check the command in progress bit to make sure things are OK
 *	There is a bound on the number of times to check and
 *	an optional call to hardware reset if things aren't working
 *	out.
 */
elx_poll_cip(gld_mac_info_t *macinfo, int port, int times, int reset)
{
	int loop;
	for (loop = times; loop > 0 &&
	     inw(port + ELX_STATUS) & ELSTATUS_CIP; loop--)
		;
	if (reset && inw(port + ELX_STATUS) & ELSTATUS_CIP) {
		cmn_err(CE_WARN, "!elx%d: rx discard failure (resetting)",
		    macinfo->gldm_ppa);
		elx_reset(macinfo);
	}
	return (inw(port + ELX_STATUS) & ELSTATUS_CIP);
}

/*
 *  GLD Entry Points
 */

/*
 *  elx_reset () -- reset the board to initial state; save the machine
 *  address and restore it afterwards.
 */

int
elx_reset (gld_mac_info_t *macinfo)
{
	unchar	macaddr[ETHERADDRL];
	struct elxinstance *elxp = (struct elxinstance *)macinfo->gldm_private;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_reset(0x%x)", macinfo);
#endif

	(void)elx_stop_board(macinfo);

	if (elxp->elx_bus == ELX_EISA) {
	   /* only reset if EISA */
	   outw(macinfo->gldm_port + ELX_COMMAND, COMMAND (ELC_GLOBAL_RESET,0));
	   elx_msdelay(3);
	}

	bcopy((caddr_t)macinfo->gldm_macaddr, (caddr_t)macaddr,
			macinfo->gldm_addrlen);
	(void)elx_init_board (macinfo);
	bcopy((caddr_t)macaddr, (caddr_t)macinfo->gldm_macaddr,
			macinfo->gldm_addrlen);
	(void)elx_saddr(macinfo);
	(void)elx_start_board (macinfo);
	SET_WINDOW (macinfo->gldm_port, 1);
	return (0);
}

/*
 *  elx_init_board () -- initialize the specified network board.
 */

static int
elx_init_board (gld_mac_info_t *macinfo)
{
	int port = macinfo->gldm_port;

	outw(port + ELX_COMMAND,
	      COMMAND (ELC_SET_READ_ZERO, 0));
	outw(port + ELX_COMMAND, COMMAND (ELC_SET_INTR, 0));
	outw(port + ELX_COMMAND, COMMAND (ELC_REQ_INTR, 0));
	return (0);
}

/*
 *  elx_start_board () -- start the board receiving and allow transmits.
 */

elx_start_board (gld_mac_info_t *macinfo)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;
	int value;
	unsigned char mediamap[] = {ELM_AUI, ELM_AUI, ELM_BNC, ELM_TP};
	register int port = macinfo->gldm_port;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_start_board(0x%x)", macinfo);
#endif
	elxp->elx_rcvbuf = NULL; /* paranoia */
	SET_WINDOW (port, 0);
	/* for EISA, may need to reconfigure IRQ */
	if (elxp->elx_irq != 0) {
	   value = inw (port + ELX_RESOURCE_CFG);
	   value = (value & 0xFFF) | (elxp->elx_irq<<12);
	   outw(port + ELX_RESOURCE_CFG, value);
	}

	value = inw(port + ELX_CONFIG_CTL);
	outb(port + ELX_CONFIG_CTL, (value | ELCONF_ENABLED) & 0xFF);
	outw(port + ELX_COMMAND, COMMAND (ELC_RX_RESET, 0));
	elx_poll_cip(macinfo, port, 0xffff, 0);
	outw(port + ELX_COMMAND, COMMAND (ELC_TX_RESET, 0));
	elx_poll_cip(macinfo, port, 0xffff, 0);
	elx_saddr(macinfo);
	outw(port + ELX_COMMAND, COMMAND (ELC_SET_INTR, ELINTR_DEFAULT));
	outw(port + ELX_COMMAND, COMMAND (ELC_SET_READ_ZERO, ELINTR_READ_ALL));
	value = inw(port + ELX_CONFIG_CTL);
	outw(port + ELX_COMMAND, COMMAND (ELC_STAT_ENABLE,0));
	outw(port + ELX_COMMAND, COMMAND (ELC_TX_ENABLE,0));
	outw(port + ELX_COMMAND, COMMAND (ELC_RX_ENABLE,0));
	outw(port + ELX_COMMAND,
	      COMMAND(ELC_SET_RX_FILTER, elxp->elx_rxbits));

	outw(port + ELX_COMMAND, COMMAND (ELC_SET_RX_EARLY, ELX_EARLY_RECEIVE));
	elxp->elx_earlyrcv = ELX_EARLY_RECEIVE;

	value = inw (port + ELX_ADDRESS_CFG);
	outw(port + ELX_ADDRESS_CFG,
	      (value & ELM_MEDIA_MASK) | (mediamap[macinfo->gldm_media] << 14));

	switch (macinfo->gldm_media) {
	 case GLDM_TP:		/* enable twisted pair stuff */
	   SET_WINDOW (port, 4);
	   value = inw (port + ELX_MEDIA_STATUS);
	   if (elxp->elx_softinfo & ELS_LINKBEATDISABLE)
	     value |= (ELD_MEDIA_JABBER_ENB);
	   else
	     value |= (ELD_MEDIA_LB_ENABLE|ELD_MEDIA_JABBER_ENB);
	   outw(port + ELX_MEDIA_STATUS, value);
	   break;
	 case GLDM_BNC:		/* enable BNC tranceiver */
	   outw(port + ELX_COMMAND, COMMAND (ELC_START_COAX, 0));
	   elx_msdelay(1);
	   break;
	 case GLDM_AUI:
	   break;
	 default:
	   break;
	}
	SET_WINDOW(port, 1);
	outw (port + ELX_COMMAND, COMMAND (ELC_REQ_INTR, 0));
	return (0);
}

/*
 *  elx_stop_board () -- stop board receiving
 */

int
elx_stop_board (gld_mac_info_t *macinfo)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;
	register port = macinfo->gldm_port;
	int window, value;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_stop_board(0x%x)", macinfo);
#endif

	value = inw (port + ELX_STATUS);
	window = GET_WINDOW (value);
	SET_WINDOW (port, 0);
	value = inw (port + ELX_CONFIG_CTL);
	outw (port + ELX_CONFIG_CTL, value &= ~ELCONF_ENABLED);
	SET_WINDOW (port, 1);
	outw (port + ELX_COMMAND, COMMAND (ELC_SET_READ_ZERO, 0));
	outw (port + ELX_COMMAND, COMMAND (ELC_SET_INTR, 0));
	SET_WINDOW (port, window);
	if (elxp->elx_rcvbuf != NULL) {
	   freeb (elxp->elx_rcvbuf);
	   elxp->elx_rcvbuf = NULL;
	}
	return (0);
}

/*
 *  elx_saddr() -- set the physical network address on the board
 */

int
elx_saddr(gld_mac_info_t *macinfo)
{
   	int i, window;
	register int port = macinfo->gldm_port;
#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_saddr(0x%x)", macinfo);
#endif
	window = GET_WINDOW (inw (port + ELX_STATUS));
	SET_WINDOW (port, 2);
	for (i = 0; i < ETHERADDRL; i++)
	  	outb (port + ELX_PHYS_ADDR + i, macinfo->gldm_macaddr[i]);
	SET_WINDOW (port, window);
	return (0);
}

/*
 *  elx_dlsdmult() -- set (enable) or disable a multicast address
 *
 *  Program the hardware to enable/disable the multicast address
 *  in "mcast".  Enable if "op" is non-zero, disable if zero.
 */

/*ARGSUSED*/
int
elx_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_dlsdmult(0x%x, %s)", macinfo,
				op ? "ON" : "OFF");
#endif

	if (op) {
	   	elxp->elx_rxbits |= ELRX_MULTI_ADDR;
		elxp->elx_mcount++;
	} else {
		if (--elxp->elx_mcount == 0)
		  	elxp->elx_rxbits &= ~ELRX_MULTI_ADDR;
	}
	outw (macinfo->gldm_port + ELX_COMMAND, COMMAND (ELC_SET_RX_FILTER,
					   elxp->elx_rxbits));
	return (0);
}

/*
 * elx_prom() -- set or reset promiscuous mode on the board
 *
 *  Program the hardware to enable/disable promiscuous mode.
 *  Enable if "on" is non-zero, disable if zero.
 */

int
elx_prom(gld_mac_info_t *macinfo, int on)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_prom(0x%x, %s)", macinfo,
				on ? "ON" : "OFF");
#endif
	if (on)
	  elxp->elx_rxbits = ELRX_PROMISCUOUS;
	else
	  elxp->elx_rxbits = (elxp->elx_mcount > 0) ?
	    (ELRX_MULTI_ADDR|ELRX_IND_ADDR) : ELRX_INIT_RX_FILTER;

	while (inw (macinfo->gldm_port + ELX_STATUS) & ELSTATUS_CIP)
	  ;
	outw (macinfo->gldm_port + ELX_COMMAND, COMMAND (ELC_SET_RX_FILTER,
					   elxp->elx_rxbits));
	while (inw (macinfo->gldm_port + ELX_STATUS) & ELSTATUS_CIP)
	  ;
	return (0);
}

/*
 * elx_gstat() -- update statistics
 *
 *  GLD calls this routine just before it reads the driver's statistics
 *  structure.  If your board maintains statistics, this is the time to
 *  read them in and update the values in the structure.  If the driver
 *  maintains statistics continuously, this routine need do nothing.
 */

int
elx_gstat(gld_mac_info_t *macinfo)
{
	register int port = macinfo->gldm_port;
	int window;

#ifdef ELXDEBUG
	if (elxdebug & ELXTRACE)
		cmn_err(CE_CONT, "elx_gstat(0x%x)", macinfo);
#endif

	window = GET_WINDOW (inw (port + ELX_STATUS));
	SET_WINDOW (port, 6);
	(void) inw(port + ELX_STATUS);
	outw (port + ELX_COMMAND, COMMAND (ELC_STAT_DISABLE, 0));
	(void) inw(port + ELX_STATUS);
	SET_WINDOW (port, 6);
	(void) inw(port + ELX_STATUS);

	(void) inb (port + ELX_CARRIER_LOST);
	(void) inb (port + ELX_NO_SQE);
	(void) inb (port + ELX_TX_FRAMES);
	(void) inb (port + ELX_RX_FRAMES);
	(void) inw (port + ELX_RX_BYTES);
	(void) inw (port + ELX_TX_BYTES);

	macinfo->gldm_stats.glds_defer += inb (port + ELX_TX_DEFER);
	macinfo->gldm_stats.glds_xmtlatecoll += inb (port + ELX_TX_LATE_COLL);
	macinfo->gldm_stats.glds_collisions += inb (port + ELX_TX_MULT_COLL) +
	  inb (port + ELX_TX_ONE_COLL);
	macinfo->gldm_stats.glds_missed += inb (port + ELX_RX_OVERRUN);
	outw (port + ELX_COMMAND, COMMAND (ELC_STAT_ENABLE, 0));
	SET_WINDOW (port, window);
	return (0);
}

/*
 *  elx_send () -- send a packet
 *
 *  Called when a packet is ready to be transmitted. A pointer to an M_PROTO
 *  or M_PCPROTO message that contains the packet is passed to this routine
 *  as a parameter. The complete LLC header is contained in the message block's
 *  control information block, and the remainder of the packet is contained
 *  within the M_DATA message blocks linked to the first message block.
 *
 *  This routine may NOT free the packet.
 */

int
elx_send (gld_mac_info_t *macinfo, mblk_t *mp)
{
	register int len = msgdsize (mp);
	register unsigned room, pad;
	register int port = macinfo->gldm_port;
	int status, txstat;
	int result = 1;		/* assume failure */
	static unsigned char padding[4];

#ifdef ELXDEBUG
	if (elxdebug & ELXSEND)
		cmn_err(CE_CONT, "elx_send(0x%x, 0x%x)", macinfo, mp);
#endif

	outw (port + ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));
	status = inw (port + ELX_STATUS);
	(void) GET_WINDOW (status);
	SET_WINDOW(port, 4);
	room = inw(port + ELX_NET_DIAGNOSTIC);
	SET_WINDOW (port, 1);
	if (!(room & ELD_NET_TX_ENABLED)) {
#if defined(ELXDEBUG)
		if (elxdebug & ELXSEND) {
			cmn_err(CE_WARN,
				"elx: transmit disabled! netdiag=%x, txstat=%x",
				room, inb(port + ELX_TX_STATUS));
		}
#endif
		outw (port + ELX_COMMAND, COMMAND(ELC_TX_ENABLE,0));
		macinfo->gldm_stats.glds_errxmt++;
	}

	while ((txstat = inb (port + ELX_TX_STATUS)) & ELTX_COMPLETE) {
		outb (port + ELX_TX_STATUS, txstat);
		if (txstat & ELTX_ERRORS){
			macinfo->gldm_stats.glds_errxmt++;
			if (txstat & (ELTX_JABBER|ELTX_UNDERRUN)){
				outw (port + ELX_COMMAND,
				      COMMAND (ELC_TX_RESET, 0));
				elx_poll_cip(macinfo, port, 0xffff, 1);
				macinfo->gldm_stats.glds_underflow++;
			}
			if (txstat & ELTX_MAXCOLL)
				macinfo->gldm_stats.glds_excoll++;
			outw (port + ELX_COMMAND, COMMAND(ELC_TX_ENABLE,0));
		}
	}
	room = inw (port + ELX_FREE_TX_BYTES);

	if ((len + 4) <= room) {
	   /*
	    *  Load the packet onto the board by chaining through the M_DATA
	    *  blocks attached to the M_PROTO header.  The list of data messages
	    *  ends when the pointer to the current message block is NULL.
	    *
	    *  Note that if the mblock is going to have to stay around, it
	    *  must be dupmsg() since the caller is going to freemsg() the
	    *  message.
	    */
	   outw (port + ELX_TX_PIO, ELTX_REQINTR|len);
	   outw (port + ELX_TX_PIO, 0);
	   pad = 4-(len&0x3);
	   while (mp != NULL) {
		len = MLEN (mp);
		elx_repouts(port+ELX_TX_PIO, mp->b_rptr, len);
		mp = mp->b_cont;
	   }
	   if (pad != 4) {
	      repoutsb(port + ELX_TX_PIO, padding, pad);
	   }
	   result = 0;
	}
	txstat = inb(port + ELX_TX_STATUS);
	if (txstat & (ELTX_UNDERRUN|ELTX_JABBER)) {
		cmn_err(CE_WARN, "elx: transmit or jabber underrun: %b",
			txstat,
			"\020\2RECLAIM\3STATOFL\4MAXCOLL\5UNDER\6JABBER\7INTR\10CPLT");
		outw (port + ELX_COMMAND, COMMAND (ELC_TX_RESET, 0));
		elx_poll_cip(macinfo, port, 0xffff, 1);
		outw (port + ELX_COMMAND, COMMAND (ELC_TX_ENABLE,0));
		result = 1;	/* force a retry */
		if (txstat & ELTX_UNDERRUN)
			macinfo->gldm_stats.glds_underflow++;
		else
			macinfo->gldm_stats.glds_errxmt++;
	}
	outw (port + ELX_COMMAND, COMMAND(ELC_SET_INTR, ELINTR_DEFAULT));
	return (result);
}

/*
 *  elxintr () -- interrupt from board to inform us that a receive or
 *  transmit has completed.
 */

u_int
elxintr (gld_mac_info_t *macinfo)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;
	register int port = macinfo->gldm_port;
	int window, status, value;

	status = inw (port + ELX_STATUS);

#ifdef ELXDEBUG
	if (elxdebug & ELXINT)
		cmn_err(CE_CONT, "elxintr(0x%x) status=%x\n", macinfo, status);
#endif

	if (!(status & ELINTR_LATCH)) {
	  	return (DDI_INTR_UNCLAIMED);
	}

	while (status & ELINTR_LATCH) {
		outw (port + ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));

		/* Acknowledge interrupt latch */
		outw (port + ELX_COMMAND,
		      COMMAND (ELC_ACK_INTR, ELINTR_LATCH));

		macinfo->gldm_stats.glds_intr++;

		window = GET_WINDOW (status);

		if (window != 1)
			SET_WINDOW (port, 1);

#if defined(ELXDEBUG)
		if (elxdebug & ELXINT) {
			SET_WINDOW(port, 4);
			value = inw(port + ELX_NET_DIAGNOSTIC);
			cmn_err(CE_CONT, "\tnetdiag=%x\n", value);
			SET_WINDOW(port, 1);
			value = inw(port + ELX_RX_STATUS);
			cmn_err(CE_CONT, "\trxstatus=%x\n", value);
		}
#endif
		if (status & ELINTR_INTR_REQUESTED) {
			/*
			 * this was requested at elx_start_board time
			 * it might be useful someday for optimizing the
			 * driver
			 */
			outw (port + ELX_COMMAND,
			      COMMAND (ELC_ACK_INTR, ELINTR_INTR_REQUESTED));
			elxp->elx_latency = inb (port + ELX_TIMER);
			elxp->elx_earlyrcv = ELX_EARLY_RECEIVE - (elxp->elx_latency * 4);

			if (elxp->elx_earlyrcv <= (ELXMAXPKT/2)) {
#if defined(ELXDEBUG)
				if (elxdebug && elxp->elx_latency > 300)
					cmn_err(CE_WARN, "elx%d: high latency %d us",
						macinfo->gldm_ppa,
						((elxp->elx_latency * 32) + 9) / 10);
#endif
				elxp->elx_earlyrcv = (ELXMAXPKT/2) + 4;
			}

			outw (port + ELX_COMMAND,
			      COMMAND (ELC_SET_RX_EARLY, elxp->elx_earlyrcv));
		}
		if (status & ELINTR_UPDATE_STATS) {
			outw (port + ELX_COMMAND,
			      COMMAND (ELC_ACK_INTR, ELINTR_UPDATE_STATS));
		}

		if (status & ELINTR_ADAPT_FAIL) {
			int x;
			outw (port + ELX_COMMAND,
			      COMMAND (ELC_ACK_INTR, ELINTR_ADAPT_FAIL));
			SET_WINDOW (port, 4);
			x = inw (port + ELX_FIFO_DIAGNOSTIC);
			if (x & ~(ELD_FIFO_RX_NORM|ELD_FIFO_RX_STATUS)) {
				/*
				 * Only reset if it is a real error
				 * According to spec (12/93), receive
				 * underruns can be spurious and should be
				 * essentially ignored -- not an error
				 * see page 10-2 for specifics.
				 * only do a reset
				 */
				if ((x & 0xfc00) & ~ELD_FIFO_RX_UNDER) {
					cmn_err (CE_WARN, "Adapter failed: fifo diag %b",x,
						 "\020\001TXBC\002TXBF\003TXBFC"
						 "\004TXBIST\005RXBC\006RXBF\007RXBFC"
						 "\010RXBIST\013TXO\014RXO\015RXSO"
						 "\016RXU\017RES\020RXR");
				}
				elx_reset(macinfo);
				status = inw(port + ELX_STATUS);
			}
		}

		/* always get the stats to make sure no overflow */
		elx_gstat (macinfo);

		if (status & (ELINTR_RX_COMPLETE|ELINTR_RX_EARLY))
			elx_getp (macinfo);

		if (!(status & ELINTR_TX_COMPLETE) &&
		    inb(port + ELX_TX_STATUS) & ELTX_COMPLETE) {
			status |= ELINTR_TX_COMPLETE;
		}
		if (status & ELINTR_TX_COMPLETE) {
			while ((value = inb (port + ELX_TX_STATUS)) & ELTX_COMPLETE) {
				outb (port + ELX_TX_STATUS, value);
				if (value & ELTX_ERRORS) {
					macinfo->gldm_stats.glds_errxmt++;
					if (value & (ELTX_JABBER|ELTX_UNDERRUN)){
						outw (port + ELX_COMMAND,
						      COMMAND(ELC_TX_RESET, 0));
						elx_poll_cip(macinfo, port, 0xffff, 1);
						macinfo->gldm_stats.glds_underflow++;
					}
#if defined(ELXDEBUG)
					if (elxdebug & ELXINT &&
					    value & ELTX_STAT_OVERFLOW) {
						cmn_err(CE_WARN,
							"elx%d: tx stat overflow", macinfo->gldm_ppa);
					}
#endif
				}
				if (value & ELTX_MAXCOLL) {
					macinfo->gldm_stats.glds_excoll++;
				}
				if (value & (ELTX_MAXCOLL|ELTX_ERRORS)) {
					outw (port + ELX_COMMAND,
					      COMMAND (ELC_TX_ENABLE,0));
				}
				status = inw (port + ELX_STATUS);
			}
		}

		/*
		 * error detection and recovery for strange conditions
		 */
		SET_WINDOW(port, 4);
		value = inw(port + ELX_NET_DIAGNOSTIC);
		if (!(value & ELD_NET_RX_ENABLED)) {
			elx_reset(macinfo);
		}
		value = inw(port + ELX_FIFO_DIAGNOSTIC);
		if (value & ELD_FIFO_RX_OVER) {
#if defined(ELXDEBUG)
			if (elxdebug & ELXERRS) {
				cmn_err(CE_WARN, "elx%d: rx fifo over",
					macinfo->gldm_ppa);
			}
#endif
			SET_WINDOW(port, 1);
			elx_reset(macinfo);
		}
		SET_WINDOW(port, 5);
		value = inw(port + ELX_RX_FILTER) & 0xF;
		if (value != elxp->elx_rxbits) {
			cmn_err(CE_WARN, "elx%d: rx filter %x/%x",
				macinfo->gldm_ppa, elxp->elx_rxbits, value);
			elx_reset(macinfo);
		}

		status = inw (port + ELX_STATUS);
	}

	SET_WINDOW (port, window);
	outw (port + ELX_COMMAND, COMMAND(ELC_SET_INTR, ELINTR_DEFAULT));
	return DDI_INTR_CLAIMED;	/* Indicate it was our interrupt */
}

/*
 * elx_discard(macinfo, elxvar, port)
 *	discard top packet and cleanup any partially received buffer
 */

static void
elx_discard(gld_mac_info_t *macinfo, struct elxinstance *elxvar, int port)
{
	outw (port + ELX_COMMAND, COMMAND (ELC_RX_DISCARD_TOP, 0));
	if (elxvar->elx_rcvbuf) {
		freeb (elxvar->elx_rcvbuf);
		elxvar->elx_rcvbuf = NULL;
	}
	elx_poll_cip(macinfo, port, 0xffff, 1);
	outw (port + ELX_COMMAND,
	      COMMAND (ELC_ACK_INTR, ELINTR_RX_COMPLETE));

}

/*
 * elx_getp (macinfo)
 *	get packet from the receive FIFO
 *	for performance, we allow an early receive interrupt
 *	and keep track of partial receives so we can free bytes
 *	fast enough to not lose packets (at least not as many as
 *	not doing this will result in)
 */
static void
elx_getp(gld_mac_info_t *macinfo)
{
   	register int port = macinfo->gldm_port;
	int value, status, discard;
	register int len;
	int calclen;
	struct elxinstance *elxvar = (struct elxinstance *)macinfo->gldm_private;

	status = inw (port + ELX_STATUS);

#if defined(ELXDEBUG)
	if (elxdebug & ELXRECV)
		cmn_err(CE_CONT, "elx_getp(%x) status=%x\n", macinfo, status);
#endif

	while (status & (ELINTR_RX_COMPLETE|ELINTR_RX_EARLY)) {
	   	mblk_t *mp;

		status = inw(port + ELX_STATUS);
		SET_WINDOW (port, 1);
		value = inw(port + ELX_RX_STATUS);
		if (value == 0xFFFF) {
			/*
			 * must be an MP system bashing us pretty badly
			 * do a hardware reset and then exit.  We lose
			 * the packet and recover.  This only happens
			 * at startup so no problem.
			 */
			elx_reset(macinfo);
			return;
		}

		len = value & ELRX_LENGTH_MASK;

		if (status & ELINTR_RX_EARLY) {
			/*
			 * we have an early receive interrupt
			 * now clear the interrupt status
			 */
			outw (port + ELX_COMMAND,
				COMMAND (ELC_ACK_INTR, ELINTR_RX_EARLY));

			if (!(value & ELRX_ERROR) ||
			    ELRX_GET_ERR(value) == ELRX_DRIBBLE) {
				if (elxvar->elx_rcvbuf == NULL) {
					/*
					 * 6 bytes extra are allocated to
					 * allow for pad bytes at the end
					 * of the frame and to allow aligning
					 * the data portion of the packet on
					 * a 4-byte boundary.  They shouldn't
					 * be necessary but don't hurt.
					 */
					mp = allocb (ELXMAXFRAME+8, BPRI_MED);
					if (mp == NULL) {
						elx_discard(macinfo, elxvar, port);
						status = inw(port + ELX_STATUS);
						continue;
					}
					if (((long)mp->b_rptr & 3) == 0) {
						/*
						 * 32-bit aligned but want 16-bit
						 * since that puts the data on a
						 * 32-bit boundary
						 */
						mp->b_rptr += 2;
						mp->b_wptr = mp->b_rptr;
					}
				} else {
					/*
					 * continue with this buffer as many times as
					 * needed in the event of very small early
					 * receive sizes
					 */
					mp = elxvar->elx_rcvbuf;
				}

				if (mp != NULL) {
					/*
					 * make sure that we never copy more data into
					 * the mblk_t than we requested to be allocated.
					 * MLEN is current length and MBLKSIZE is the
					 * allocated size.  The -2 compensates for 32-bit
					 * alignment factors.
					 */
					calclen = MLEN(mp) + len;
					if (calclen > (MBLKSIZE(mp) - 6) ||
					    calclen > ELXMAXFRAME) {
#if defined(ELXDEBUG)
						if (elxdebug & ELXRECV)
							cmn_err(CE_WARN,
								"elx%d: jumbogram received"
								" (%d) bytes from %s",
								macinfo->gldm_ppa,
								MLEN(mp) + len,
								ether_sprintf((struct ether_addr *)
								      (mp->b_rptr + 6)));
#endif
						elx_discard(macinfo, elxvar, port);
						status = inw(port + ELX_STATUS);
						macinfo->gldm_stats.glds_errrcv++;
						mp = NULL;
						continue;
					}
					elxvar->elx_rcvbuf = mp;
					elx_repins(port + ELX_RX_PIO,
						mp->b_wptr, len);
					mp->b_wptr += len;
				} else {
					macinfo->gldm_stats.glds_norcvbuf++;
					elx_discard(macinfo, elxvar, port);
				}
			}
			outw(port + ELX_COMMAND,
			      COMMAND(ELC_SET_RX_EARLY, elxvar->elx_earlyrcv));
		} else {
			/* ack the interrupt */
			outw(port + ELX_COMMAND,
			      COMMAND (ELC_ACK_INTR, ELINTR_RX_COMPLETE));

			/*
			 * we only want non-error or dribble packets
			 */
			if ((value & ELRX_STAT_MASK) == 0 ||
			    ((value & ELRX_ERROR) &&
			     ELRX_GET_ERR(value) == ELRX_DRIBBLE)) {
				/* we have a packet to read */
				if (elxvar->elx_rcvbuf != NULL) {
					outw(port + ELX_COMMAND,
					      COMMAND (ELC_SET_RX_EARLY,
						       elxvar->elx_earlyrcv));
					mp = elxvar->elx_rcvbuf;
				} else {
					mp = allocb (len+8, BPRI_MED);
					if (mp == NULL) {
						elx_discard(macinfo, elxvar, port);
						status = inw(port + ELX_STATUS);
						continue;
					}
					/* make sure long-aligned */
					if (((long)mp->b_rptr & 3) == 0) {
						mp->b_rptr += 2;
						mp->b_wptr = mp->b_rptr;
					}
				}
				if (mp != NULL) {
					/*
					 * need to make sure a jumbogram doesn't
					 * kill us
					 */
					calclen = MLEN(mp) + len;
					if (calclen > (MBLKSIZE(mp) - 6) ||
					    calclen > ELXMAXFRAME) {
#if defined(ELXDEBUG)
						if (elxdebug & ELXRECV)
							cmn_err (CE_WARN,
								 "elx%d: jumbogram received"
								 " (%d bytes) from %s",
								 macinfo->gldm_ppa,
								 MLEN (mp) + len,
								 ether_sprintf((struct ether_addr *)
									       (mp->b_rptr + 6)));
#endif
						freemsg(mp);
						macinfo->gldm_stats.glds_errrcv++;
					} else {
						elx_repins(port + ELX_RX_PIO,
							mp->b_wptr, len);
						mp->b_wptr += len;
						gld_recv (macinfo, mp);
					}
					elxvar->elx_rcvbuf = NULL;
					mp = NULL;
				} else {
					macinfo->gldm_stats.glds_norcvbuf++;
				}
				elx_discard(macinfo, elxvar, port);
			}

			if (value & ELRX_ERROR) {
				macinfo->gldm_stats.glds_errrcv++;
				switch ((value >> 11) & 0xF) {
				case ELRX_OVERRUN:
					macinfo->gldm_stats.glds_overflow++;
					discard=1;
					break;
				case ELRX_RUNT:
					macinfo->gldm_stats.glds_short++;
					discard=1;
					break;
				case ELRX_FRAME:
					macinfo->gldm_stats.glds_frame++;
					discard=1;
					break;
				case ELRX_CRC:
					macinfo->gldm_stats.glds_crc++;
					discard = 1;
					break;
				case ELRX_OVERSIZE:
					discard = 1;
					break;
				case ELRX_DRIBBLE:
					discard = 0;
					break;
				default:
					/* don't know what it is so discard */
					printf("elx: unknown error %x\n", value);
					discard = 1;
					break;
				}
				if (discard) {
					elx_discard(macinfo, elxvar, port);
				}
			}
		}
		value = inw(port + ELX_RX_STATUS);
		status = inw (port + ELX_STATUS);
	}
}

/*
 * get_bustype (devinfo)
 * 	return the bus type as encoded in the devinfo tree
 */
static int
get_bustype(dev_info_t *devi)
{
	char 		bus_type[16];
	int		len = sizeof (bus_type);

	if (ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
			"device_type", (caddr_t)&bus_type[0], &len) !=
	    DDI_PROP_SUCCESS) {
		if (ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
				"bus-type", (caddr_t)&bus_type[0], &len) !=
		    DDI_PROP_SUCCESS) {
			return 0;
		}
	}
#if defined(_mca_bus_supported)
	if (strcmp (bus_type, DEVI_MCA_NEXNAME) == 0)
		return ELX_MCA;
#endif
#if defined(_isa_bus_supported)
	if (strcmp (bus_type, DEVI_ISA_NEXNAME) == 0)
	  	return ELX_ISA;
#endif
#if defined(_eisa_bus_supported)
	if (strcmp (bus_type, DEVI_EISA_NEXNAME) == 0)
		return ELX_EISA;
#endif
	cmn_err(CE_WARN, "!elx: unknown bus type %s (assuming ISA)", bus_type);
	return ELX_ISA;
}

/*
 * elx_msdelay (ms)
 *	delay in terms of milliseconds.
 */
static void
elx_msdelay(int ms)
{
   drv_usecwait(1000 * ms);
}

/*
 * elx_isa_idseq (id)
 *	write the 3C509 ID sequence to the specified ID port
 *	in order to put all ISA boards into the ID_CMD state.
 */

static void
elx_isa_idseq(int id)
{
   int cx, al;

   if (id != 0) {
      /* get the boards' attention */
      outb (id, ELISA_RESET);
      outb (id, ELISA_RESET);
      /* send the ID sequence */
      for (cx = ELISA_ID_INIT , al = ELISA_ID_PATLEN; cx > 0; cx--){
	 outb (id, al);
	 al <<= 1;
	 if (al & 0x100) {
	    al = (al ^ ELISA_ID_OPAT) & 0xFF;
	 }
      }
   }
}
/*
 * elx_isa_init (id)
 *	initialize ISA boards (3c509) at the specified port ID
 *	this will get called once to find boards and
 *	activate them then return the list of boards found
 */
static int
elx_isa_init (int id, short *boards, int max)
{
   int value;
   int nboards;

   if (id != 0) {
      elx_isa_idseq (id);
      outb (id, ELISA_SET_TAG (0)); /* reset any tags to zero */
      outb (id, ELISA_GLOBAL_RESET); /* just to make all boards consistent */
      elx_msdelay (4);
   }


   for (nboards = 0; nboards < max; nboards++) {
      elx_isa_idseq (id);
      /* now we have board(s) in ID_CMD state */
      outb (id, ELISA_TEST_TAG(0)); /* keep only non-tagged boards */

      /* make sure all contention is done */
      value = elx_readid_prom (id, EEPROM_PHYS_ADDR);
      value = elx_readid_prom (id, EEPROM_PHYS_ADDR+1);
      value = elx_readid_prom (id, EEPROM_PHYS_ADDR+2);
      /* read in the product ID since that should always force things */
      value = elx_readid_prom (id, EEPROM_PROD_ID);

      /* if it doesn't match, then no more boards */
      if ((value & EL_PRODID_MASK) != EL_PRODUCT_ID)
	break;

      value = elx_readid_prom (id, EEPROM_ADDR_CFG);

      /* now have info, so tag board */
      outb (id, ELISA_SET_TAG (nboards+1));
      boards[nboards] = (value&0x1F)*0x10 + 0x200;
   }
   elx_isa_idseq (id);
   elx_isa_sort (boards, nboards);
   outb (id, ELISA_ACTIVATE);
   return nboards;
}

/*
 * elx_isa_sort (boards, nboards)
 *	need to sort boards by I/O address to avoid having
 *	boards found in random order.
 */

static void
elx_isa_sort(short *boards, int nboards)
{
   int i, j;
   short tmp;

   if (nboards == 1)
     return;
   for (i=0; i<nboards; i++)
     for (j=0; j<i; j++)
       if (boards[j] > boards[i]) {
	  tmp = boards[i];
	  boards[i] = boards[j];
	  boards[j] = tmp;
       }
}

/*
 * elx_readid_prom (id, addr)
 *	read the board's PROM at PROM address addr using
 *	the id port.  This is an ISA only function.
 */
static int
elx_readid_prom(int id, int addr)
{
   register int i, value;

   /* select EEPROM to resolve contention and get I/O port */
   outb (id, ELISA_READEEPROM (addr));

   /* wait for the EEPROM register to settle */
   drv_usecwait (ELISA_READ_DELAY*10);
   /*
    * now read the value in, 1 bit at a time
    * Note that this forces the boards to resolve their contention.
    */
   for (i=16, value = 0; i > 0; i--){
      value = (value << 1) | (inb (id) & 1);
   }
   return value;
}

/*
 * elx_readpos (slot, pos)
 *	given a slot number, read all 6 bytes of POS information
 *	This is an MCA only function.
 */
#if defined(_mca_bus_supported)
static void
elx_readpos(int slot, unchar pos[6])
{
   int i;
   outb (ELX_ADAP_ENABLE, (slot-1) + ELPOS_SETUP);
   for (i=0; i<6; i++)
     pos[i] = inb (ELX_POS_REG_BASE + i);
   outb (ELX_ADAP_ENABLE, ELPOS_DISABLE);
}
#endif

/*
 * elx_verify_setup (devinfo, port, irq, media)
 * 	verifies that the information passed in is valid per the
 *	devinfo_t information then saves it in the private data
 *	word.
 */
static int
elx_verify_setup (dev_info_t *devinfo, int port, int irq, int media)
{
   int regbuf[3], len, i;
   struct intrprop {
      int	spl;
      int	irq;
   } *intrprop;

   if ((i=ddi_getlongprop (DDI_DEV_T_ANY, devinfo,
			   DDI_PROP_DONTPASS, "intr",
			   (caddr_t) &intrprop, &len)) != DDI_PROP_SUCCESS) {
      return 0;
   }

   /* now check that our IRQ matches */
   len /= sizeof (struct intrprop);
   for (i=0; i<len; i++)
     if (irq == intrprop[i].irq)
       break;

   kmem_free (intrprop, len * sizeof (struct intrprop));

   if (i >= len) {
      return 0;			/* not found */
   }

   irq = i | (irq<<8);	/* index and value */

   /* create ioaddr property */
   regbuf[0] = port;
   (void) ddi_prop_create (DDI_DEV_T_NONE, devinfo, DDI_PROP_CANSLEEP,
			   "ioaddr", (caddr_t) regbuf, sizeof (int));

   /* save the IRQ, IRQ index and media type for later use */
   ddi_set_driver_private (devinfo, (caddr_t) (irq|(media<<16)));
   return 1;
}

static int
elx_conv(char *str)
{
	int value = 0;
	int base = 10;

	if (str[0] == '0' && str[1] == 'x') {
		base = 16;
		str += 2;
	}
	while (*str) {
		int i;
		i = *str - '0';
		if (i > 9)
			i = ((*str & ~0x20) - 'A') + 10;
		value = (value * base) + i;
		str++;
	}
	return value;
}

/* Copy data from memory to the board's buffer, using 4-byte-wide I/O	*/
/* when possible.  I don't believe this techique works in the general	*/
/* case - I believe a 4-byte I/O is roughly equivalent to one-byte I/Os	*/
/* to n, n+1, n+2, and n+3 - but apparently this board will treat such	*/
/* usefully.								*/
void
elx_repouts(int port, unsigned char *buf, int len)
{
	register int i;

	if (len >= sizeof (long)) {
		/* First, send bytes until the buffer's aligned */
		i = (4- ((unsigned long)buf & 3)) & 3;
		if (i > 0) {
			repoutsb(port, buf, i);
			buf += i;
			len -= i;
		}

		/* Next, send as many 4-byte units as possible */
		i = len & ~3;
		if (i > 0) {
			repoutsd(port, (unsigned long *)buf, i>>2);
			buf += i;
			len -= i;
		}
	}

	/* Finally, send any trailing bytes */
	if (len > 0) repoutsb(port, buf, len);
}

/* Copy data from the board's buffer into memory, using 4-byte-wide I/O	*/
/* when possible.  I don't believe this techique works in the general	*/
/* case - I believe a 4-byte I/O is roughly equivalent to one-byte I/Os	*/
/* to n, n+1, n+2, and n+3 - but apparently this board will treat such	*/
/* usefully.								*/
void
elx_repins(int port, unsigned char *buf, int len)
{
	register int i;

	if (len >= sizeof (long)) {
		/* First, get bytes until the buffer's aligned */
		i = (4- ((unsigned long)buf & 3)) & 3;
		if (i > 0) {
			repinsb(port, buf, i);
			buf += i;
			len -= i;
		}

		/* Next, get as many 4-byte units as possible */
		i = len & ~3;
		if (i > 0) {
			repinsd(port, (unsigned long *)buf, i>>2);
			buf += i;
			len -= i;
		}
	}

	/* Finally, get any trailing bytes */
	if (len > 0) repinsb(port, buf, len);
}
