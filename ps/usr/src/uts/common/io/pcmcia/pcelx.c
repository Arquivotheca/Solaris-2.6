/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *	  All Rights Reserved
 */

#ident "96/08/01	@(#)pcelx.c	1.34 SMI"

/*
 * elx -- 3COM EtherLink III family of Ethernet controllers
 * Depends on the Generic LAN Driver utility functions in /kernel/misc/gld
 *
 * NOTE: this driver is portable across all platforms (sparc, i386, ppc)
 *	 so care must be taken to ensure that any changes do not break
 *	 this.
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
#include "sys/dditypes.h"
#include "sys/devops.h"
#include "sys/sunddi.h"
#include "sys/ksynch.h"
#include "sys/dlpi.h"
#include "sys/ethernet.h"
#include "sys/strsun.h"
#include "sys/stat.h"
#include "sys/modctl.h"
#if defined(i386)
#include "sys/eisarom.h"
#include "sys/nvm.h"
#endif
#include <sys/byteorder.h>

#include <sys/pctypes.h>
#include <sys/cis.h>
#include <sys/cis_handlers.h>
#include <sys/cs_types.h>
#include <sys/cs.h>

#include "sys/pcgld.h"
#include "sys/pcelx.h"
#include <sys/debug.h>

extern u_int gld_intr();
extern u_int gld_intr_hi();

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "3COM EtherLink III (PCMCIA)";

#ifdef ELXDEBUG
int	pcelx_debug = 0x0;
void pcelx_dmp_regs(acc_handle_t);
#endif

void pcelx_repinsd(acc_handle_t, unsigned long *, int, int);
void pcelx_repoutsd(acc_handle_t, unsigned long *, int, int);

/* Required system entry points */
static	pcelxidentify(dev_info_t *);
static	pcelxdevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	pcelxprobe(dev_info_t *);
static	pcelxattach(dev_info_t *, ddi_attach_cmd_t);
static	pcelxdetach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
int	pcelx_reset(gld_mac_info_t *);
int	pcelx_start_board(gld_mac_info_t *);
int	pcelx_stop_board(gld_mac_info_t *);
int	pcelx_saddr(gld_mac_info_t *);
int	pcelx_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
int	pcelx_prom(gld_mac_info_t *, int);
int	pcelx_gstat(gld_mac_info_t *);
int	pcelx_send(gld_mac_info_t *, mblk_t *);
u_int	pcelxintr(gld_mac_info_t *);
u_int	pcelxintr_hi(gld_mac_info_t *);

/* Other prototypes */
static int get_bustype(dev_info_t *);
int pcelx_readpos(unchar *);
int pcelx_verify_setup(dev_info_t *, acc_handle_t, int, int);
int pcelx_register(dev_info_t *, gld_mac_info_t *);
int pcelx_init_board(gld_mac_info_t *);
void pcelx_get_addr(gld_mac_info_t *, u_short *);
int pcelx_read_prom(gld_mac_info_t *, acc_handle_t, int);
void pcelx_announce_devs(dev_info_t *, gld_mac_info_t *);
void pcelx_msdelay(int);
void pcelx_unregister_client(dev_info_t *, gld_mac_info_t *);
void pcelx_getp(gld_mac_info_t *);
int pcelx_card_setup(dev_info_t *, gld_mac_info_t *);
void pcelx_patch_addr(gld_mac_info_t *);
void pcelx_remove_card(dev_info_t *, gld_mac_info_t *);
int pcelx_request_config(gld_mac_info_t *, struct elxinstance *);
void pcelx_pcmcia_reginit(gld_mac_info_t *);
void pcelx_announce_devs(dev_info_t *, gld_mac_info_t *);
void pcelx_probe_media(gld_mac_info_t *);

/* DEPENDS_ON_GLD;	this forces misc/gld to load -- DO NOT REMOVE */

/* Standard Streams initialization */

static struct module_info minfo = {
	ELXIDNUM, "pcelx", 0, INFPSZ, ELXHIWAT, ELXLOWAT
	};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
	};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
	};

struct streamtab elxinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static	struct cb_ops cb_elxops = {
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
	&elxinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
	};

struct dev_ops elxops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	pcelxdevinfo,		/* devo_getinfo */
	pcelxidentify,		/* devo_identify */
	pcelxprobe,		/* devo_probe */
	pcelxattach,		/* devo_attach */
	pcelxdetach,		/* devo_detach */
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

int
_init(void)
{
	extern void gld_init();
	(void) gld_init();
	return (mod_install(&modlinkage));
}

int
_fini(void)
{
	int ret;

	ret = mod_remove(&modlinkage);
	return (ret);
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
pcelxidentify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "pcelx") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

/* getinfo(9E) -- Get device driver information */
pcelxdevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;
#ifdef lint
	arg = arg;
#endif

	/* This code is not DDI compliant: the correct semantics */
	/* for CLONE devices is not well-defined yet. */
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

#define	INW(h, o)	leshort(csx_Get16(h, o))
#define	INL(h, o)	lelong(csx_Get32(h, o))
#define	OUTW(h, o, v)	csx_Put16(h, o, leshort(v))
#define	OUTL(h, o, v)	csx_Put32(h, o, lelong(v))

/* probe(9E) -- Determine if a device is present */
pcelxprobe(dev_info_t *devinfo)
{
	int found_board = 0;

#ifdef ELXDEBUG
	if (pcelx_debug & ELXDDI)
		cmn_err(CE_CONT, "pcelxprobe(0x%x)", (unsigned)devinfo);
#endif
	/*
	 *  Probe for the board to see if it's there
	 */
	switch (get_bustype(devinfo)) {
	case ELX_PCMCIA:
		/* if we know this than we are ok */
		found_board++;
		break;
	default:
		break;
	}

	if (found_board)
		return (DDI_PROBE_SUCCESS);
	else
		return (DDI_PROBE_FAILURE);
}

/*
 *  attach(E) -- Attach a device to the system
 *
 *  Called once for each board successfully probed.
 */
pcelxattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	acc_handle_t port;
	int value, cfaddr;
	static char media[] = {GLDM_TP, GLDM_AUI, GLDM_UNKNOWN, GLDM_BNC};
	struct elxinstance *elxp;

#ifdef ELXDEBUG
	if (pcelx_debug & ELXDDI)
		cmn_err(CE_CONT, "pcelxattach(0x%x)", (int)devinfo);
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	/*
	 *  Allocate gld_mac_info_t and elxinstance structures
	 */
	macinfo = (gld_mac_info_t *)kmem_zalloc(sizeof (struct elxinstance) +
						sizeof (gld_mac_info_t),
						KM_NOSLEEP);
	if (macinfo == NULL)
		return (DDI_FAILURE);

	/*  Initialize our private fields in macinfo and elxinstance */
	macinfo->gldm_private = (caddr_t)(macinfo+1);
	elxp = (struct elxinstance *)macinfo->gldm_private;

	macinfo->gldm_state = ELX_IDLE;
	macinfo->gldm_flags = 0;

	elxp->elx_bus = get_bustype(devinfo);

	elxp->elx_rxbits = ELRX_INIT_RX_FILTER; /* initial receiver filter */

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */

	macinfo->gldm_reset   = pcelx_reset;
	macinfo->gldm_start   = pcelx_start_board;
	macinfo->gldm_stop    = pcelx_stop_board;
	macinfo->gldm_saddr   = pcelx_saddr;
	macinfo->gldm_sdmulti = pcelx_dlsdmult;
	macinfo->gldm_prom    = pcelx_prom;
	macinfo->gldm_gstat   = pcelx_gstat;
	macinfo->gldm_send    = pcelx_send;
	macinfo->gldm_intr    = pcelxintr;

	if (ddi_intr_hilevel(devinfo, 0)) {
		macinfo->gldm_intr_hi = pcelxintr_hi;
	}

	macinfo->gldm_ioctl   = NULL;	 /* if you have one, NULL otherwise */

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */

	/* Adjust the following values as necessary */
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;		/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = ELXMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;

	if (elxp->elx_bus != ELX_PCMCIA) {
		if (elxp->elx_bus == ELX_EISA &&
			macinfo->gldm_port < (acc_handle_t)0x1000)
			elxp->elx_bus = ELX_ISA;

		macinfo->gldm_port = (acc_handle_t)ddi_getprop(DDI_DEV_T_ANY,
							devinfo,
							DDI_PROP_DONTPASS,
							"ioaddr", 0);
		macinfo->gldm_irq_index =
			((long)ddi_get_driver_private(devinfo)) & 0xFF;
		elxp->elx_irq =
			(((long)ddi_get_driver_private(devinfo)) >> 8) & 0xFF;
		macinfo->gldm_flags |= ELX_CARD_PRESENT; /* always */
	} else {
		cv_init(&elxp->elx_condvar, "3c589 event wait",
			CV_DRIVER, NULL);

		if (ddi_getprop(DDI_DEV_T_NONE, devinfo, DDI_PROP_DONTPASS,
				"force-8bit", 0))
			elxp->elx_pcinfo |= ELPC_INFO_FORCE_8BIT;
		if (pcelx_register(devinfo, macinfo) < 0) {
			pcelx_unregister_client(devinfo, macinfo);
			return (DDI_FAILURE);
		}
		elxp->elx_irq = ELX_PCMCIA_IRQ;
		/* we need the card insertion event to let us go further */
		while ((macinfo->gldm_flags & ELX_CS_READY) != ELX_CS_READY &&
			!(macinfo->gldm_flags & ELX_CARD_REMOVED)) {
			cv_wait(&elxp->elx_condvar, &elxp->elx_cslock);
		}
		mutex_exit(&elxp->elx_cslock);
		/* interrupts are different for PCMCIA */
		elxp->elx_features |= ELF_PCMCIA;
		value = INW(macinfo->gldm_port, ELX_PRODUCT_ID);
		if (value == 0 || value == 0xffff) {
			pcelx_unregister_client(devinfo, macinfo);
			return (DDI_FAILURE);
		}
	}

	port = macinfo->gldm_port;
	/*
	 *  Do anything necessary to prepare the board for operation
	 *  short of actually starting the board.
	 */
	SET_WINDOW(port, 0);

	/* set the connector/media type if it can be determined */
	if ((macinfo->gldm_media = ddi_getprop(DDI_DEV_T_NONE, devinfo,
						DDI_PROP_DONTPASS, "media",
						-1)) ==
	    -1) {
		cfaddr = pcelx_read_prom(macinfo, port, EEPROM_ADDR_CFG);
		macinfo->gldm_media = media[(cfaddr >> 14) & 0x3];
	}
	(void) pcelx_init_board(macinfo);

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "pcelx_attach: finished init_board\n");
#endif

	/* Get the board's vendor-assigned hardware network address */
	pcelx_get_addr(macinfo, (u_short *)macinfo->gldm_vendor);

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "pcelx_attach: finished get_addr\n");
#endif

	/* check software configuration register */

	elxp->elx_softinfo = pcelx_read_prom(macinfo, port, EEPROM_SOFTINFO);

	/*
	 * now we need to get information about card revision level
	 * The "B" versions have new features.	The ASIC level will
	 * tell us.
	 */
	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);
	bcopy((caddr_t)macinfo->gldm_vendor,
		(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);

	/* Make sure we have our address set */
	(void) pcelx_saddr(macinfo);

	SET_WINDOW(port, 0);
	value = INW(port, ELX_CONFIG_CTL);
	if (value & ELCONF_TYPEB) {
		/* there are some new features so record them */
		elxp->elx_features |= ELF_TYPE_B;
		value = pcelx_read_prom(macinfo, port, EEPROM_CAPABILITIES);
		elxp->elx_softinfo |= value << 16;
		SET_WINDOW(port, 3);
		value = INL(port, ELX_INTERNAL_CONFIG);
		elxp->elx_fifosize = 8192 << (value & ELICONF_SIZE_MASK);
		OUTL(port, ELX_INTERNAL_CONFIG,
			ELICONF_SET_PARTITION(value, 1));
		(void) pcelx_probe_media(macinfo);
	} else {
		elxp->elx_fifosize = 4096;
	}
#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "fifosize = %d\n", elxp->elx_fifosize);
#endif
	SET_WINDOW(port, 0);
	/* must leave the EEPROM data showing the Product ID */
	OUTW(port, ELX_EEPROM_CMD, EEPROM_CMD(EEPROM_READ, EEPROM_PROD_ID));
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
	macinfo->gldm_ppa = ddi_getprop(DDI_DEV_T_NONE,
					devinfo,
					DDI_PROP_DONTPASS,
					"socket",
					-1);
	/* let GLD know that we are a PCMCIA driver */
	macinfo->gldm_options |= GLDOPT_PCMCIA;
	if (gld_register(devinfo, "pcelx", macinfo) == DDI_SUCCESS) {
		if (elxp->elx_bus == ELX_PCMCIA) {
			modify_config_t mod;
				/* turn on interrupts */
			mod.Attributes = CONF_ENABLE_IRQ_STEERING |
				CONF_IRQ_CHANGE_VALID;
			mod.Vpp1 = 0;
			mod.Vpp2 = 0;

			/*
			 * Note that PPA is supposed to be socket based
			 * for PCMCIA. This means we shouldn't share
			 * namespace with non-PCMCIA devices
			 */
			macinfo->gldm_ppa = ddi_getprop(DDI_DEV_T_NONE,
							devinfo,
							DDI_PROP_DONTPASS,
							"socket",
							-1);
			csx_ModifyConfiguration(elxp->elx_handle,
					&mod);
			pcelx_announce_devs(devinfo, macinfo);

		}
		return (DDI_SUCCESS);
	} else {
		kmem_free((caddr_t)macinfo,
				sizeof (gld_mac_info_t) +
				sizeof (struct elxinstance));
		return (DDI_FAILURE);
	}
}

/*  detach(9E) -- Detach a device from the system */

pcelxdetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;		/* GLD structure */
	struct elxinstance *elxp;		/* Our private device info */

#ifdef ELXDEBUG
	if (pcelx_debug & ELXDDI)
		cmn_err(CE_CONT, "pcelxdetach(0x%x)", (int)devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);
	elxp = (struct elxinstance *)(macinfo->gldm_private);

	mutex_enter(&macinfo->gldm_maclock);
	/* stop the board if it is running */
	(void) pcelx_stop_board(macinfo);
	macinfo->gldm_flags &= ~ELX_CARD_PRESENT;
	mutex_exit(&macinfo->gldm_maclock);

	/* unregister from CS if a PCMCIA device */
	if (elxp->elx_bus == ELX_PCMCIA) {
		pcelx_unregister_client(devinfo, macinfo);
		mutex_destroy(&macinfo->gldm_intrlock);
	}
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
				sizeof (gld_mac_info_t) +
				sizeof (struct elxinstance));
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}

/*
 * pcelx_poll_cip()
 *	check the command in progress bit to make sure things are OK
 *	There is a bound on the number of times to check and
 *	an optional call to hardware reset if things aren't working
 *	out.
 */
pcelx_poll_cip(gld_mac_info_t *macinfo, acc_handle_t port, int times, int reset)
{
	int loop;
	register value;

	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (0);

	for (loop = times; loop > 0 &&
		macinfo->gldm_flags & ELX_CARD_PRESENT &&
		(value = INW(port, ELX_STATUS)) & ELSTATUS_CIP &&
		value != 0xFFFF; loop--)
		;
	if (reset && INW(port, ELX_STATUS) & ELSTATUS_CIP) {
		cmn_err(CE_WARN, "!elx%d: rx discard failure (resetting)",
			(int)macinfo->gldm_ppa);
		(void) pcelx_reset(macinfo);
	}
	return (INW(port, ELX_STATUS) & ELSTATUS_CIP);
}

/*
 *  GLD Entry Points
 */

/*
 *  pcelx_reset() -- reset the board to initial state; save the machine
 *  address and restore it afterwards.
 */

int
pcelx_reset(gld_mac_info_t *macinfo)
{
	unchar	macaddr[ETHERADDRL];
	struct elxinstance *elxp = (struct elxinstance *)macinfo->gldm_private;
	acc_handle_t port = macinfo->gldm_port;

#ifdef ELXDEBUG
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "pcelx_reset(0x%x)", (int)macinfo);
#endif
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (0);

	/*
	 * it is possible to get stuck in here
	 * need to check the result of the reset and
	 * prevent a loop in the interrupt handler on
	 * failure.  It is typically a problem with
	 * suspend/resume.
	 */

	(void) pcelx_stop_board(macinfo);

	if (elxp->elx_bus == ELX_EISA) {
		/* only reset if EISA */
		OUTW(port, ELX_COMMAND,
			COMMAND(ELC_GLOBAL_RESET, 0));
		pcelx_msdelay(3);
	}

	bcopy((caddr_t)macinfo->gldm_macaddr, (caddr_t)macaddr,
		macinfo->gldm_addrlen);
	(void) pcelx_init_board(macinfo);
	bcopy((caddr_t)macaddr, (caddr_t)macinfo->gldm_macaddr,
		macinfo->gldm_addrlen);
	(void) pcelx_saddr(macinfo);
	(void) pcelx_start_board(macinfo);
	SET_WINDOW(port, 1);

	/*
	 * check here for hardware failure
	 * since we just set the window to 1, we can't
	 * get 0xffff back from the status register
	 */
	if (INW(port, ELX_STATUS) == 0xFFFF) {
		macinfo->gldm_flags &= ~ELX_CARD_PRESENT;
	}
	return (0);
}

/*
 *  pcelx_init_board () -- initialize the specified network board.
 */

int
pcelx_init_board(gld_mac_info_t *macinfo)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;
	acc_handle_t port = macinfo->gldm_port;

#ifdef lint
	elxp = elxp;
#endif

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "init_board: entered\n");
#endif
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (0);

	OUTW(port, ELX_COMMAND,
		COMMAND(ELC_SET_READ_ZERO, 0));
	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));
	OUTW(port, ELX_COMMAND, COMMAND(ELC_REQ_INTR, 0));
#if defined(ELXDEBUG)
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "init_board: exit\n");
#endif
	return (0);
}

/*
 *  pcelx_start_board) -- start the board receiving and allow transmits.
 */

pcelx_start_board(gld_mac_info_t *macinfo)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;
	int value;
	unsigned char mediamap[] = {ELM_AUI, ELM_AUI, ELM_BNC, ELM_TP};
	register acc_handle_t port = macinfo->gldm_port;

#ifdef ELXDEBUG
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "pcelx_start_board(0x%x)", (int)macinfo);
#endif
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (0);

	elxp->elx_rcvbuf = NULL; /* paranoia */

	if (elxp->elx_fifosize >= 8192) {
		SET_WINDOW(port, 3);
		value = INL(port, ELX_INTERNAL_CONFIG);
		OUTL(port, ELX_INTERNAL_CONFIG,
			ELICONF_SET_PARTITION(value, 1));
	}
	SET_WINDOW(port, 0);
	/* for EISA, may need to reconfigure IRQ */
	if (elxp->elx_irq != 0) {
		value = INW(port, ELX_RESOURCE_CFG);
		value = (value & 0xFFF) | (elxp->elx_irq<<12);
		OUTW(port, ELX_RESOURCE_CFG, value);
	}

	value = INW(port, ELX_CONFIG_CTL);
	csx_Put8(port, ELX_CONFIG_CTL, (value | ELCONF_ENABLED) & 0xFF);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_RX_RESET, 0));
	(void) pcelx_poll_cip(macinfo, port, 0xffff, 0);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_TX_RESET, 0));
	(void) pcelx_poll_cip(macinfo, port, 0xffff, 0);
	(void) pcelx_saddr(macinfo);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_INTR, ELINTR_DEFAULT));
	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_READ_ZERO, ELINTR_READ_ALL));
	value = INW(port, ELX_CONFIG_CTL);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_STAT_ENABLE, 0));
	OUTW(port, ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));
	OUTW(port, ELX_COMMAND, COMMAND(ELC_RX_ENABLE, 0));
	OUTW(port, ELX_COMMAND,
		COMMAND(ELC_SET_RX_FILTER, elxp->elx_rxbits));

	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_RX_EARLY, ELX_EARLY_RECEIVE));
	elxp->elx_earlyrcv = ELX_EARLY_RECEIVE;

	value = INW(port, ELX_ADDRESS_CFG);
	OUTW(port, ELX_ADDRESS_CFG,
		(value & ELM_MEDIA_MASK) |
		((int)mediamap[macinfo->gldm_media] << 14));

	switch (macinfo->gldm_media) {
	case GLDM_TP:		/* enable twisted pair stuff */
		SET_WINDOW(port, 4);
		value = INW(port, ELX_MEDIA_STATUS);
		if (elxp->elx_softinfo & ELS_LINKBEATDISABLE)
			value |= (ELD_MEDIA_JABBER_ENB);
		else
			value |= (ELD_MEDIA_LB_ENABLE|ELD_MEDIA_JABBER_ENB);
		OUTW(port, ELX_MEDIA_STATUS, value);
		break;
	case GLDM_BNC:		/* enable BNC tranceiver */
		OUTW(port, ELX_COMMAND, COMMAND(ELC_START_COAX, 0));
		pcelx_msdelay(1);
		break;
	case GLDM_AUI:
		break;
	default:
		break;
	}
	SET_WINDOW(port, 1);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_REQ_INTR, 0));
	return (0);
}

/*
 *  pcelx_stop_board() -- stop board receiving
 */

pcelx_stop_board(gld_mac_info_t *macinfo)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;
	register acc_handle_t port = macinfo->gldm_port;
	int window, value;

#ifdef ELXDEBUG
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "pcelx_stop_board(0x%x)", (int)macinfo);
#endif
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (0);

	value = INW(port, ELX_STATUS);
	window = GET_WINDOW(value);
	SET_WINDOW(port, 0);
	value = INW(port, ELX_CONFIG_CTL);
	value &= ~ELCONF_ENABLED;
	OUTW(port, ELX_CONFIG_CTL, value);
	SET_WINDOW(port, 1);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_READ_ZERO, 0));
	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));
	SET_WINDOW(port, window);
	if (elxp->elx_rcvbuf != NULL) {
		freeb(elxp->elx_rcvbuf);
		elxp->elx_rcvbuf = NULL;
	}
	return (0);
}

/*
 *  pcelx_saddr() -- set the physical network address on the board
 */

int
pcelx_saddr(gld_mac_info_t *macinfo)
{
	int i, window;
	register acc_handle_t port = macinfo->gldm_port;
#ifdef ELXDEBUG
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "pcelx_saddr(0x%x)", (int)macinfo);
#endif
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (0);
	window = GET_WINDOW(INW(port, ELX_STATUS));

	SET_WINDOW(port, 2);
	for (i = 0; i < ETHERADDRL; i++) {
		csx_Put8(port, ELX_PHYS_ADDR + i, macinfo->gldm_macaddr[i]);
	}
	SET_WINDOW(port, window);
	return (0);
}

/*
 *  pcelx_dlsdmult() -- set (enable) or disable a multicast address
 *
 *  Program the hardware to enable/disable the multicast address
 *  in "mcast".	 Enable if "op" is non-zero, disable if zero.
 */

int
pcelx_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;

#ifdef lint
	mcast = mcast;
#endif
#ifdef ELXDEBUG
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "pcelx_dlsdmult(0x%x, %s)", (int)macinfo,
			op ? "ON" : "OFF");
#endif

	if (op) {
		elxp->elx_rxbits |= ELRX_MULTI_ADDR;
		elxp->elx_mcount++;
	} else {
		if (--elxp->elx_mcount == 0)
			elxp->elx_rxbits &= ~ELRX_MULTI_ADDR;
	}
	OUTW(macinfo->gldm_port, ELX_COMMAND, COMMAND(ELC_SET_RX_FILTER,
							elxp->elx_rxbits));
	return (0);
}

/*
 * pcelx_prom() -- set or reset promiscuous mode on the board
 *
 *  Program the hardware to enable/disable promiscuous mode.
 *  Enable if "on" is non-zero, disable if zero.
 */
int
pcelx_prom(gld_mac_info_t *macinfo, int on)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;

#ifdef ELXDEBUG
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "pcelx_prom(0x%x, %s)", (int)macinfo,
			on ? "ON" : "OFF");
#endif
	if (on)
		elxp->elx_rxbits = ELRX_PROMISCUOUS;
	else
		elxp->elx_rxbits = (elxp->elx_mcount > 0) ?
			(ELRX_MULTI_ADDR|ELRX_IND_ADDR) : ELRX_INIT_RX_FILTER;
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT)) {
		return (0);
	}
	while (macinfo->gldm_flags & ELX_CARD_PRESENT &&
		INW(macinfo->gldm_port, ELX_STATUS) & ELSTATUS_CIP)
		;
	OUTW(macinfo->gldm_port, ELX_COMMAND, COMMAND(ELC_SET_RX_FILTER,
							elxp->elx_rxbits));
	while (INW(macinfo->gldm_port, ELX_STATUS) & ELSTATUS_CIP &&
		macinfo->gldm_flags & ELX_CARD_PRESENT)
		;
	return (0);
}

/*
 * pcelx_gstat() -- update statistics
 *
 *  GLD calls this routine just before it reads the driver's statistics
 *  structure.	If your board maintains statistics, this is the time to
 *  read them in and update the values in the structure.  If the driver
 *  maintains statistics continuously, this routine need do nothing.
 */

int
pcelx_gstat(gld_mac_info_t *macinfo)
{
	register acc_handle_t port = macinfo->gldm_port;
	int window;

#ifdef ELXDEBUG
	if (pcelx_debug & ELXRECV)
		cmn_err(CE_CONT, "pcelx_gstat(0x%x)", (int)macinfo);
#endif
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (0);

	window = GET_WINDOW(INW(port, ELX_STATUS));
	SET_WINDOW(port, 6);
	(void) INW(port, ELX_STATUS);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_STAT_DISABLE, 0));
	(void) INW(port, ELX_STATUS);
	SET_WINDOW(port, 6);
	(void) INW(port, ELX_STATUS);

	(void) csx_Get8(port, ELX_CARRIER_LOST);
	(void) csx_Get8(port, ELX_NO_SQE);
	(void) csx_Get8(port, ELX_TX_FRAMES);
	(void) csx_Get8(port, ELX_RX_FRAMES);
	(void) INW(port, ELX_RX_BYTES);
	(void) INW(port, ELX_TX_BYTES);

	macinfo->gldm_stats.glds_defer += csx_Get8(port, ELX_TX_DEFER);
	macinfo->gldm_stats.glds_xmtlatecoll +=
		csx_Get8(port, ELX_TX_LATE_COLL);
	macinfo->gldm_stats.glds_collisions +=
		csx_Get8(port, ELX_TX_MULT_COLL) +
		csx_Get8(port, ELX_TX_ONE_COLL);
	macinfo->gldm_stats.glds_missed += csx_Get8(port, ELX_RX_OVERRUN);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_STAT_ENABLE, 0));
	SET_WINDOW(port, window);
	return (0);
}

/*
 *  pcelx_send () -- send a packet
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
pcelx_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	register int len = msgdsize(mp);
	register int room, pad;
	register acc_handle_t port = macinfo->gldm_port;
	int status, txstat;
	int result = 1, window;		/* assume failure */
	static char padding[4];
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;

#ifdef lint
	elxp = elxp;
#endif

#ifdef ELXDEBUG
	if (pcelx_debug & ELXSEND)
		cmn_err(CE_CONT, "pcelx_send(0x%x, 0x%x)",
			(int)macinfo, (int)mp);
#endif
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (0);

	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));
	status = INW(port, ELX_STATUS);
	window = GET_WINDOW(status);
	SET_WINDOW(port, 4);
	room = INW(port, ELX_NET_DIAGNOSTIC);
	SET_WINDOW(port, 1);
	if (!(room & ELD_NET_TX_ENABLED)) {
#if defined(ELXDEBUG)
		if (pcelx_debug & ELXSEND) {
			cmn_err(CE_WARN,
				"pcelx: transmit disabled! "
				"netdiag=%x, txstat=%x",
				room, csx_Get8(port, ELX_TX_STATUS));
		}
#endif
		OUTW(port, ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));
		macinfo->gldm_stats.glds_errxmt++;
	}

	while (macinfo->gldm_flags & ELX_CARD_PRESENT &&
		(txstat = csx_Get8(port, ELX_TX_STATUS)) & ELTX_COMPLETE) {
		csx_Put8(port, ELX_TX_STATUS, txstat);
		if (txstat & ELTX_ERRORS) {
			macinfo->gldm_stats.glds_errxmt++;
			if (txstat & (ELTX_JABBER|ELTX_UNDERRUN)) {
				OUTW(port, ELX_COMMAND,
					COMMAND(ELC_TX_RESET, 0));
				(void) pcelx_poll_cip(macinfo, port, 0xffff, 1);
				macinfo->gldm_stats.glds_underflow++;
			}
			if (txstat & ELTX_MAXCOLL)
				macinfo->gldm_stats.glds_excoll++;
			OUTW(port, ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));
		}
	}
	room = INW(port, ELX_FREE_TX_BYTES);

	if ((len + 4) <= room) {
		/*
		 * Load the packet onto the board by chaining
		 * through the M_DATA blocks attached to the
		 * M_PROTO header.  The list of data messages
		 * ends when the pointer to the current
		 * message block is NULL.

		 * Note that if the mblock is going to have
		 * to stay around, it must be dupmsg() since
		 * the caller is going to freemsg() the
		 * message.
		 */
		OUTW(port, ELX_TX_PIO, ELTX_REQINTR|len);
		OUTW(port, ELX_TX_PIO, 0);
		pad = 4-(len&0x3);
		while (mp != NULL) {
			len = MLEN(mp);
			room = len >> 2;
			if (room > 0)
				pcelx_repoutsd(port,
						(unsigned long *)mp->b_rptr,
						ELX_TX_PIO, room);
			room = len & 0x3;
			if (room > 0) {
				csx_RepPut8(port,
						mp->b_rptr + (len & ~0x3),
						ELX_TX_PIO, room, 0);
			}
			mp = mp->b_cont;
		}
		if (pad != 4) {
			csx_RepPut8(port, (unsigned char *)padding,
					ELX_TX_PIO, pad, 0);
		}
		result = 0;
	}
	txstat = csx_Get8(port, ELX_TX_STATUS);
	if (txstat & (ELTX_UNDERRUN|ELTX_JABBER)) {
		cmn_err(CE_WARN, "pcelx: jabber or transmit underrun: %b",
			(long)txstat,
			"\020\2RECLAIM\3STATOFL\4MAXCOLL\5UNDER"
			"\6JABBER\7INTR\10CPLT");
		OUTW(port, ELX_COMMAND, COMMAND(ELC_TX_RESET, 0));
		(void) pcelx_poll_cip(macinfo, port, 0xffff, 1);
		OUTW(port, ELX_COMMAND, COMMAND(ELC_TX_ENABLE, 0));
		result = 1;	/* force a retry */
		if (txstat & ELTX_UNDERRUN)
			macinfo->gldm_stats.glds_underflow++;
		else
			macinfo->gldm_stats.glds_errxmt++;
	}
	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_INTR, ELINTR_DEFAULT));
	if (window != 1)
		SET_WINDOW(port, window);
	return (result);
}

/*
 * pcelxintr_hi() -- above lock level interrupt handler
 *	just disable interrupts and return after accepting
 *	the interrupt if it belongs to the driver
 */

u_int
pcelxintr_hi(gld_mac_info_t *macinfo)
{
	/* Our private device info */
	struct elxinstance *elxp =
		(struct elxinstance *)macinfo->gldm_private;
	register acc_handle_t port = macinfo->gldm_port;
	int window, status, ret = DDI_INTR_UNCLAIMED;

	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (DDI_INTR_UNCLAIMED);

	status = INW(port, ELX_STATUS);

#ifdef ELXDEBUG
	if (pcelx_debug & ELXINT)
		cmn_err(CE_CONT, "pcelxintr_hi(0x%x) status=%x\n",
			(int)macinfo, status);
#endif

	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT) ||
	    status == 0xFFFF) {
		if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
#if defined(ELXDEBUG)
			if (pcelx_debug & ELXTRACE)
				cmn_err(CE_WARN, "%s%d: interrupt after"
					"card removed",
					ddi_get_name(macinfo->gldm_devinfo),
					(int)macinfo->gldm_ppa);
#endif
		return (DDI_INTR_CLAIMED);
	}
	mutex_enter(&macinfo->gldm_intrlock);
	if (status & ELINTR_LATCH) {
		elxp->elx_intrstat = status;
		if ((window = GET_WINDOW(status)) != 1)
			SET_WINDOW(port, 1);
		/*
		 * it is my interrupt so mask it off
		 * XXX: check on how to block interrupts without
		 *	clearing bits
		 */
		OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));
		macinfo->gldm_flags |= ELX_INTR_PENDING;
		/* Acknowledge interrupt latch */
		OUTW(port, ELX_COMMAND,
			COMMAND(ELC_ACK_INTR, ELINTR_LATCH));

		if (status & ELINTR_INTR_REQUESTED) {
			elxp->elx_latency_hi = csx_Get8(port, ELX_TIMER);
			/* start over just in case of overflow */
			OUTW(port, ELX_COMMAND, COMMAND(ELC_REQ_INTR, 0));
		}
#if 0
		if (status & ELINTR_RX_COMPLETE &&
		    elxp->elx_rcvhi != NULL &&
		    elxp->elx_rcvbuf == NULL) {
			register mblk_t *mp = elxp->elx_rcvhi;
			if (MLEN(mp) == 0) {
				int len;
				len = INW(macinfo->gldm_port, ELX_RX_STATUS);
				len &= ELRX_LENGTH_MASK;
				if (len > ELXMAXFRAME)
					len = ELXMAXFRAME;
				len -= 6;
				csx_RepGet16(macinfo->gldm_port,
						(unsigned short *)mp->b_wptr,
						ELX_RX_PIO, 3,
						DDI_DEV_NO_AUTOINCR);
				mp->b_wptr += 6;
				pcelx_repinsd(macinfo->gldm_port,
						(ulong_t *)mp->b_wptr,
						ELX_RX_PIO, (len + 3) >> 2);
				OUTW(macinfo->gldm_port, ELX_COMMAND,
					COMMAND(ELC_RX_DISCARD_TOP, 0));
				mp->b_wptr += len;
			}
		}
#endif
		if (window != 1)
			SET_WINDOW(port, window);

		ret = DDI_INTR_CLAIMED;
	}
	mutex_exit(&macinfo->gldm_intrlock);

	return (ret);
}
/*
 *  pcelxintr() -- interrupt from board to inform us that a receive or
 *  transmit has completed.
 */

u_int
pcelxintr(gld_mac_info_t *macinfo)
{
	struct elxinstance *elxp =		/* Our private device info */
		(struct elxinstance *)macinfo->gldm_private;
	register acc_handle_t port = macinfo->gldm_port;
	int window, status, value;

	if (macinfo->gldm_intr_hi &&
	    (macinfo->gldm_flags & (ELX_CARD_PRESENT | ELX_INTR_PENDING)) !=
	    (ELX_CARD_PRESENT | ELX_INTR_PENDING))
		return (DDI_INTR_UNCLAIMED);

	status = INW(port, ELX_STATUS);
	window = GET_WINDOW(status);

#ifdef ELXDEBUG
	if (pcelx_debug & ELXINT)
		cmn_err(CE_CONT, "pcelxintr(0x%x) status=%x\n",
			(int)macinfo, status);
#endif

	if (macinfo->gldm_intr_hi) {
		status = elxp->elx_intrstat | (status & ELINTR_DEFAULT);
		elxp->elx_intrstat = 0;
	}
	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT) ||
	    status == 0xFFFF) {
		if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
#if defined(ELXDEBUG)
			if (pcelx_debug & ELXTRACE)
				cmn_err(CE_WARN, "%s%d: interrupt after"
					"card removed",
					ddi_get_name(macinfo->gldm_devinfo),
					(int)macinfo->gldm_ppa);
#endif
		return (DDI_INTR_CLAIMED);
	}
	if (!(status & ELINTR_LATCH)) {
		if (macinfo->gldm_flags & ELX_INTR_PENDING) {
			status |= ELINTR_LATCH;	/* lost it somehow */
		} else {
			return (DDI_INTR_UNCLAIMED);
		}
	}
	if (window != 1)
		SET_WINDOW(port, 1);

	while (macinfo->gldm_flags & ELX_CARD_PRESENT &&
		status & ELINTR_LATCH) {
		mutex_enter(&macinfo->gldm_intrlock);
		macinfo->gldm_flags &= ~ELX_INTR_PENDING;
		OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_INTR, 0));

		/* Acknowledge interrupt latch */
		OUTW(port, ELX_COMMAND,
			COMMAND(ELC_ACK_INTR, ELINTR_LATCH));
		mutex_exit(&macinfo->gldm_intrlock);

		macinfo->gldm_stats.glds_intr++;

		if (window != 1)
			SET_WINDOW(port, 1);

#if defined(ELXDEBUG)
		if (pcelx_debug & ELXINT) {
			SET_WINDOW(port, 4);
			value = INW(port, ELX_NET_DIAGNOSTIC);
			cmn_err(CE_CONT, "\tnetdiag=%x\n", value);
			SET_WINDOW(port, 1);
			value = INW(port, ELX_RX_STATUS);
			cmn_err(CE_CONT, "\trxstatus=%x\n", value);
		}
#endif
		if (status & ELINTR_INTR_REQUESTED) {
			/*
			 * this was requested at elx_start_board time
			 * it might be useful someday for optimizing the
			 * driver
			 */
			OUTW(port, ELX_COMMAND,
				COMMAND(ELC_ACK_INTR, ELINTR_INTR_REQUESTED));
			elxp->elx_latency = csx_Get8(port, ELX_TIMER);
#if defined(ELXDEBUG)
			if (pcelx_debug)
				cmn_err(CE_CONT,
					"elx: latency=%d, latency_hi=%d\n",
					elxp->elx_latency,
					elxp->elx_latency_hi);
#endif
			elxp->elx_latency += elxp->elx_latency_hi;
			elxp->elx_earlyrcv = ELX_EARLY_RECEIVE -
				(elxp->elx_latency * 4);

			if (elxp->elx_earlyrcv <= (ELXMAXPKT/2)) {
#if defined(ELXDEBUG)
				if (pcelx_debug && elxp->elx_latency > 300)
					cmn_err(CE_WARN, "pcelx%d: high "
						"latency of %d us",
						(int)macinfo->gldm_ppa,
						((elxp->elx_latency * 32) + 9) /
						10);
#endif
				elxp->elx_earlyrcv = (ELXMAXPKT/3) + 4;
			}

			OUTW(port, ELX_COMMAND,
				COMMAND(ELC_SET_RX_EARLY,
					elxp->elx_earlyrcv));
		}
		if (status & ELINTR_UPDATE_STATS) {
			OUTW(port, ELX_COMMAND,
				COMMAND(ELC_ACK_INTR,
					ELINTR_UPDATE_STATS));
		}

		if (status & ELINTR_ADAPT_FAIL) {
			int x;
			OUTW(port, ELX_COMMAND,
				COMMAND(ELC_ACK_INTR, ELINTR_ADAPT_FAIL));
			SET_WINDOW(port, 4);
			x = INW(port, ELX_FIFO_DIAGNOSTIC);
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
					cmn_err(CE_WARN, "Adapter %d failed:"
						"fifo diag %b",
						(int)macinfo->gldm_ppa,
						(long)x,
						"\020\001TXBC\002TXBF\003TXBFC"
						"\004TXBIST\005RXBC\006RXBF"
						"\007RXBFC"
						"\010RXBIST\013TXO\014RXO"
						"\015RXSO"
						"\016RXU\017RES\020RXR");
				}
				(void) pcelx_reset(macinfo);
				status = INW(port, ELX_STATUS);
			}
		}

		/* always get the stats to make sure no overflow */
		(void) pcelx_gstat(macinfo);
		if (status & (ELINTR_RX_COMPLETE|ELINTR_RX_EARLY))
			pcelx_getp(macinfo);

		if (!(status & ELINTR_TX_COMPLETE) &&
		    csx_Get8(port, ELX_TX_STATUS) & ELTX_COMPLETE) {
			status |= ELINTR_TX_COMPLETE;
		}
		if (status & ELINTR_TX_COMPLETE) {
			while (macinfo->gldm_flags & ELX_CARD_PRESENT &&
				(value = csx_Get8(port, ELX_TX_STATUS)) &
					ELTX_COMPLETE) {
				csx_Put8(port, ELX_TX_STATUS, value);
				if (value == 0xff) {
					break;
				}
				if (value & ELTX_ERRORS) {
					macinfo->gldm_stats.glds_errxmt++;
					if (value &
					    (ELTX_JABBER|ELTX_UNDERRUN)) {
						OUTW(port, ELX_COMMAND,
							COMMAND(ELC_TX_RESET,
								0));
						(void) pcelx_poll_cip(macinfo,
								port,
								0xffff, 1);
						/* CSTYLED */
						macinfo->gldm_stats.glds_underflow++;
					}
#if defined(ELXDEBUG)
					if (pcelx_debug & ELXINT &&
					    value & ELTX_STAT_OVERFLOW) {
						cmn_err(CE_WARN,
							"pcelx%d: tx stat "
							"overflow: %b",
							(int)macinfo->gldm_ppa,
							(long)value,
							"\20\1UNDEF\2RE\3OF\4MC"
							"\5UN\6JB\7IS\10CM");
					}
#endif
				}
				if (value & ELTX_MAXCOLL) {
					macinfo->gldm_stats.glds_excoll++;
				}
				if (value & (ELTX_MAXCOLL|ELTX_ERRORS)) {
					OUTW(port, ELX_COMMAND,
						COMMAND(ELC_TX_ENABLE, 0));
				}
				status = INW(port, ELX_STATUS);
			}
		}

		/*
		 * error detection and recovery for strange conditions
		 */
		SET_WINDOW(port, 4);
		value = INW(port, ELX_NET_DIAGNOSTIC);
		if (!(value & ELD_NET_RX_ENABLED)) {
			(void) pcelx_reset(macinfo);
		}
		value = INW(port, ELX_FIFO_DIAGNOSTIC);
		if (value & ELD_FIFO_RX_OVER) {
#if defined(ELXDEBUG)
			if (pcelx_debug & ELXERRS) {
				cmn_err(CE_WARN, "pcelx%d: rx fifo over",
					(int)macinfo->gldm_ppa);
			}
#endif
			SET_WINDOW(port, 1);
			(void) pcelx_reset(macinfo);
		}
		SET_WINDOW(port, 5);
		value = INW(port, ELX_RX_FILTER) & 0xF;
		if (value != elxp->elx_rxbits) {
			cmn_err(CE_WARN, "pcelx%d: rx filter %x/%x",
				(int)macinfo->gldm_ppa,
				elxp->elx_rxbits, value);
			(void) pcelx_reset(macinfo);
		}
		SET_WINDOW(port, 1);
		status = INW(port, ELX_STATUS);
		if (macinfo->gldm_flags & ELX_INTR_PENDING) {
			status |= ELINTR_LATCH;
		}
	}

	SET_WINDOW(port, 1);

	if (window != 1)
		SET_WINDOW(port, window);
	OUTW(port, ELX_COMMAND, COMMAND(ELC_SET_INTR, ELINTR_DEFAULT));
	return (DDI_INTR_CLAIMED);	/* Indicate it was our interrupt */
}

/*
 * pcelx_discard(macinfo, elxvar, port)
 *	discard top packet and cleanup any partially received buffer
 */
void
pcelx_discard(gld_mac_info_t *macinfo,
		struct elxinstance *elxvar, acc_handle_t port)
{
	OUTW(port, ELX_COMMAND, COMMAND(ELC_RX_DISCARD_TOP, 0));
	if (elxvar->elx_rcvbuf) {
		freeb(elxvar->elx_rcvbuf);
		elxvar->elx_rcvbuf = NULL;
	}
	(void) pcelx_poll_cip(macinfo, port, 0xffff, 1);
	OUTW(port, ELX_COMMAND,
		COMMAND(ELC_ACK_INTR, ELINTR_RX_COMPLETE));

}

/*
 * pcelx_getp(macinfo)
 *	get packet from the receive FIFO
 *	for performance, we allow an early receive interrupt
 *	and keep track of partial receives so we can free bytes
 *	fast enough to not lose packets (at least not as many as
 *	not doing this will result in)
 */
void
pcelx_getp(gld_mac_info_t *macinfo)
{
	register acc_handle_t port = macinfo->gldm_port;
	int value, status, discard;
	register int len;
	int calclen, header;
	struct elxinstance *elxvar =
		(struct elxinstance *)macinfo->gldm_private;

	status = INW(port, ELX_STATUS);

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXRECV)
		cmn_err(CE_CONT, "pcelx_getp(%x) status=%x\n",
			(int)macinfo, status);
#endif

#if 0
	if (elxvar->elx_rcvhi && MLEN(elxvar->elx_rcvhi)) {
		mblk_t *xx = elxvar->elx_rcvhi;
		elxvar->elx_rcvhi = NULL;
		gld_recv(macinfo, xx);
	}

	if (elxvar->elx_rcvhi == NULL) {
		mblk_t *xx = allocb(ELXMAXFRAME+8, BPRI_MED);
		if (xx != NULL) {
			if ((((ulong_t)xx->b_rptr & 3)) == 0) {
				xx->b_rptr += 2; /* half word */
			}
			xx->b_wptr = xx->b_rptr;
			elxvar->elx_rcvhi = xx;
		}
	}
#endif
	header = 0;		/* assume worst case */
	while (macinfo->gldm_flags & ELX_CARD_PRESENT &&
		status & (ELINTR_RX_COMPLETE|ELINTR_RX_EARLY)) {
		mblk_t *mp;
		status = INW(port, ELX_STATUS);
		SET_WINDOW(port, 1);
		value = INW(port, ELX_RX_STATUS);
		if (value == 0xFFFF) {
			/*
			 * must be an MP system bashing us pretty badly
			 * do a hardware reset and then exit.  We lose
			 * the packet and recover.  This only happens
			 * at startup so no problem.
			 */
			(void) pcelx_reset(macinfo);
			return;
		}

		len = value & ELRX_LENGTH_MASK;
#if defined(ELXDEBUG)
		if (pcelx_debug & ELXRECV)
			cmn_err(CE_CONT,
				"packet received with length = %d\n", len);
#endif
		if (status & ELINTR_RX_EARLY) {
			/*
			 * we have an early receive interrupt
			 * now clear the interrupt status
			 */
			OUTW(port, ELX_COMMAND,
				COMMAND(ELC_ACK_INTR, ELINTR_RX_EARLY));

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
					mp = allocb(ELXMAXFRAME+8, BPRI_MED);
					if (mp == NULL) {
						pcelx_discard(macinfo, elxvar,
								port);
						status = INW(port, ELX_STATUS);
						continue;
					}
					if (((long)mp->b_rptr & 3) == 0) {
						/*
						 * 32-bit aligned but want
						 * 16-bit since that puts
						 * the data on a 32-bit
						 * boundary
						 */
						mp->b_rptr += 2;
						mp->b_wptr = mp->b_rptr;
						header = 1;
					}
				} else {
					/*
					 * continue with this buffer as
					 * many times as needed in the
					 * event of very small early
					 * receive sizes
					 */
					mp = elxvar->elx_rcvbuf;
				}
				if (mp != NULL) {
					/*
					 * make sure that we never copy
					 * more data into the mblk_t than
					 * we requested to be allocated.
					 * MLEN is current length and
					 * MBLKSIZE is the allocated size
					 * The -2 compensates for 32-bit
					 * alignment factors.
					 */
					calclen = MLEN(mp) + len;
					if (calclen > (MBLKSIZE(mp) - 6) ||
					    calclen > ELXMAXFRAME) {
#if defined(ELXDEBUG)
						if (pcelx_debug & ELXRECV)
							cmn_err(CE_WARN,
								"pcelx%d: "
								"jumbogram"
								"received"
								" (%d) bytes"
								"from %s",
								/* CSTYLED */
								(int)macinfo->gldm_ppa,
								MLEN(mp) + len,
								/* CSTYLED */
								ether_sprintf((struct ether_addr *)
								/* CSTYLED */
										(int)(mp->b_rptr + 6)));
#endif
						pcelx_discard(macinfo,
								elxvar, port);
						status = INW(port,
								ELX_STATUS);
						/* CSTYLED */
						macinfo->gldm_stats.glds_errrcv++;
						mp = NULL;
						continue;
					}
					elxvar->elx_rcvbuf = mp;
					if (header) {
						header = min(len >> 1, 3);
						csx_RepGet16(port,
							/* CSTYLED */
							(unsigned short *)mp->b_wptr,
							ELX_RX_PIO,
							header,
							DDI_DEV_NO_AUTOINCR);
						/*
						 * NOTE: should check for
						 *	 multicast and discard
						 *	 packets we won't use
						 */
						header <<= 1;
						len -= header;
						mp->b_wptr += header;
						header = 0;
					}
					pcelx_repinsd(port,
							(ulong_t *)mp->b_wptr,
							ELX_RX_PIO, len>>2);
					mp->b_wptr += (len & ~3);
				} else {
					macinfo->gldm_stats.glds_norcvbuf++;
					pcelx_discard(macinfo, elxvar, port);
				}
			}
			OUTW(port, ELX_COMMAND,
				COMMAND(ELC_SET_RX_EARLY,
					elxvar->elx_earlyrcv));
		} else {
		    /* ack the interrupt */
		    OUTW(port, ELX_COMMAND,
			    COMMAND(ELC_ACK_INTR, ELINTR_RX_COMPLETE));

			/*
			 * we only want non-error or dribble packets
			 */
		    if ((value & ELRX_STAT_MASK) == 0 ||
			((value & ELRX_ERROR) &&
			    ELRX_GET_ERR(value) == ELRX_DRIBBLE)) {
			    /* we have a packet to read */
			    if (elxvar->elx_rcvbuf != NULL) {
				OUTW(port, ELX_COMMAND,
						COMMAND(ELC_SET_RX_EARLY,
							elxvar->elx_earlyrcv));
				mp = elxvar->elx_rcvbuf;
			    } else {
				mp = allocb(len+8, BPRI_MED);
				if (mp == NULL) {
				    pcelx_discard(macinfo, elxvar,
							port);
				    status = INW(port, ELX_STATUS);
				    continue;
				}
				/* make sure long-aligned */
				if (((long)mp->b_rptr & 3) == 0) {
					mp->b_rptr += 2;
					mp->b_wptr = mp->b_rptr;
					header = 1;
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
				    if (pcelx_debug & ELXRECV)
					cmn_err(CE_WARN,
						"pcelx%d: "
						"jumbogram"
						"received"
						" (%d bytes) "
						"from %s",
						/* CSTYLED */
						(int)macinfo->gldm_ppa,
						MLEN(mp) + len,
						/* CSTYLED */
						ether_sprintf((struct ether_addr *)
							/* CSTYLED */
								(int)(mp->b_rptr + 6)));
#endif
				    freemsg(mp);
				    /* CSTYLED */
				    macinfo->gldm_stats.glds_errrcv++;
				} else {
				    if (header) {
					header = min(len >> 1, 3);
					csx_RepGet16(port,
							(ushort *)mp->b_wptr,
							ELX_RX_PIO,
							header,
							/* CSTYLED */
							DDI_DEV_NO_AUTOINCR);
					/*
					 * NOTE: should check for
					 *	 multicast and discard
					 *	 packets we won't use
					 */
					header <<= 1;
					len -= header;
					mp->b_wptr += header;
					header = 0;
#if defined(ELXDEBUG)
					if (pcelx_debug & ELXRECV) {
					    /* BEGIN CSTYLED */
					    cmn_err(CE_CONT,
						    "dst=[%s] hlen=%d\n",
						    /* CSTYLED */
						    ether_sprintf((struct ether_addr *)mp->b_rptr),
						    header);
					    /* END CSTYLED */
					}
#endif
				    }
				    pcelx_repinsd(port,
						    /* CSTYLED */
						    (ulong *)mp->b_wptr,
						    ELX_RX_PIO,
						    (len+3)>>2);
				    mp->b_wptr += len;
				    gld_recv(macinfo, mp);
				}
				elxvar->elx_rcvbuf = NULL;
				mp = NULL;
			    } else {
				    macinfo->gldm_stats.glds_norcvbuf++;
			    }
				pcelx_discard(macinfo, elxvar, port);
			}

			if (value & ELRX_ERROR) {
				macinfo->gldm_stats.glds_errrcv++;
				switch ((value >> 11) & 0xF) {
				case ELRX_OVERRUN:
					macinfo->gldm_stats.glds_overflow++;
					discard = 1;
					break;
				case ELRX_RUNT:
					macinfo->gldm_stats.glds_short++;
					discard = 1;
					break;
				case ELRX_FRAME:
					macinfo->gldm_stats.glds_frame++;
					discard = 1;
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
					/* don't know what it is, discard */
					cmn_err(CE_WARN,
						"pcelx: unknown error %x\n",
						value);
					discard = 1;
					break;
				}
				if (discard) {
					pcelx_discard(macinfo, elxvar, port);
				}
			}
		}
		value = INW(port, ELX_RX_STATUS);
		status = INW(port, ELX_STATUS);
	}
}

/*
 * get_bustype(devinfo)
 *	return the bus type as encoded in the devinfo tree
 */
static int
get_bustype(dev_info_t *devi)
{
	static int	return_val;
	static int	not_first_call = 0;
	char		bus_type[16];
	int		len = sizeof (bus_type);

	if (not_first_call)
		return (return_val);

	devi = ddi_get_parent(devi);
	if (ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
			PCELX_DEVICETYPE, (caddr_t)&bus_type[0], &len) !=
	    DDI_PROP_SUCCESS) {
		if (ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
				"bus-type", (caddr_t)&bus_type[0], &len) !=
		    DDI_PROP_SUCCESS) {
			return (0);
		}
	}
#if defined(i386)
	if (strcmp(bus_type, DEVI_MCA_NEXNAME) == 0) {
		return (ELX_MCA);
	} else if (strcmp(bus_type, DEVI_ISA_NEXNAME) == 0) {
		return (ELX_ISA);
	} else if (strcmp(bus_type, DEVI_EISA_NEXNAME) == 0) {
		return (ELX_EISA);
	} else {
#endif
		if (strcmp(bus_type, "pccard") == 0 ||
			strcmp(bus_type, "pcmcia") == 0) {
			return (ELX_PCMCIA);
		} else {
			return (ELX_BUS_DEFAULT);
		}
#if defined(i386)
	}
#endif
}
/*
 * pcelx_msdelay(ms)
 *	delay in terms of milliseconds.
 */
void
pcelx_msdelay(int ms)
{
	drv_usecwait(1000 * ms);
}

/*
 * pcelx_verify_setup(devinfo, port, irq, media)
 *	verifies that the information passed in is valid per the
 *	devinfo_t information then saves it in the private data
 *	word.
 */
pcelx_verify_setup(dev_info_t *devinfo, acc_handle_t port, int irq, int media)
{
	int regbuf[3], len, i;
	struct intrprop {
		int	  spl;
		int	  irq;
	} *intrprop;

	if ((i = ddi_getlongprop(DDI_DEV_T_ANY, devinfo,
				DDI_PROP_DONTPASS, "intr",
				(caddr_t)&intrprop, &len)) !=
	    DDI_PROP_SUCCESS) {
		return (0);
	}

	/* now check that our IRQ matches */
	len /= sizeof (struct intrprop);
	for (i = 0; i < len; i++)
		if (irq == intrprop[i].irq)
			break;

	kmem_free(intrprop, len * sizeof (struct intrprop));

	if (i >= len) {
		return (0);		  /* not found */
	}

	irq = i | (irq << 8);  /* index and value */

	/* create ioaddr property */
	regbuf[0] = (long)port;
	(void) ddi_prop_create(DDI_DEV_T_NONE, devinfo, DDI_PROP_CANSLEEP,
				"ioaddr", (caddr_t)regbuf, sizeof (int));

	/* save the IRQ, IRQ index and media type for later use */
	ddi_set_driver_private(devinfo, (caddr_t)(irq|(media<<16)));
	return (1);
}

int
pcelx_read_prom(gld_mac_info_t *macinfo, acc_handle_t port, int promaddr)
{
	int value, window;
	struct elxinstance *elxvar =
		(struct elxinstance *)macinfo->gldm_private;

	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return (-1);

	if (elxvar->elx_features & ELF_PROM_IN_CIS) {
		return ((int)elxvar->elx_promcopy[promaddr]);
	}
	value = INW(port, ELX_STATUS);
	window = GET_WINDOW(value);

	if (value == 0xffff)
		return (-1);
	if (window != 0)
		SET_WINDOW(port, 0);
	while (macinfo->gldm_flags & ELX_CARD_PRESENT &&
		(value = INW(port, ELX_EEPROM_CMD)) & EEPROM_BUSY)
		/* wait */
		;
	OUTW(port, ELX_EEPROM_CMD, EEPROM_CMD(EEPROM_READ, promaddr));
	while (macinfo->gldm_flags & ELX_CARD_PRESENT &&
		(value = INW(port, ELX_EEPROM_CMD)) & EEPROM_BUSY)
		/* wait */
		;
	if (macinfo->gldm_flags & ELX_CARD_PRESENT)
		value = INW(port, ELX_EEPROM_DATA);
	else
		value = -1;
	if (window != 0)
		SET_WINDOW(port, window);
	return (value);
}

pcelx_check_compat(gld_mac_info_t *macinfo,
			struct elxinstance *elx, acc_handle_t port)
{
	int value;
	value = pcelx_read_prom(macinfo, port, EEPROM_COMPATIBILITY);
	if (ELCOMPAT_FAIL(value) > ELCOMPAT_FAIL_LEVEL &&
	    !(elx->elx_features & ELF_PCMCIA))
		return (value);
	if (ELCOMPAT_WARN(value) > ELCOMPAT_WARN_LEVEL &&
	    !((elx->elx_features & (ELF_TYPE_B|ELF_PCMCIA)) !=
		(ELF_TYPE_B|ELF_PCMCIA)))
		return (value);
	return (0);
}

/*
 * PCMCIA specific
 */

pcelx_pc_event(event_t event, int priority, event_callback_args_t *arg)
{
	int ret;
	gld_mac_info_t *macinfo;
	struct elxinstance *elx;
	client_info_t *ci = (client_info_t *)&arg->client_info;

	macinfo = (gld_mac_info_t *)arg->client_data;
	elx = (struct elxinstance *)macinfo->gldm_private;

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXTRACE)
		cmn_err(CE_CONT, "pxelx(%d): got an event %x (%x, %x)\n",
			(int)macinfo->gldm_ppa, (int)event,
			priority, (int)arg);
#endif
	mutex_enter(&elx->elx_cslock);

	switch (event) {
	case CS_EVENT_CARD_INSERTION:
		ret = pcelx_card_setup(macinfo->gldm_devinfo, macinfo);
		if (macinfo->gldm_intr != NULL && ret == CS_SUCCESS) {
			if (macinfo->gldm_flags & ELX_CARD_REMOVED) {
				macinfo->gldm_flags &= ~ELX_CARD_REMOVED;
				(void) pcelx_reset(macinfo);
			}
			/* pcelx_patch_addr(macinfo); */

		}
		if (ret != CS_SUCCESS)
			macinfo->gldm_flags |= ELX_CONFIG_FAILED;
		cv_broadcast(&elx->elx_condvar);
		break;

	case CS_EVENT_REGISTRATION_COMPLETE:
		macinfo->gldm_flags |= ELX_REGISTERED;
		cv_broadcast(&elx->elx_condvar);
		ret = CS_SUCCESS;
		break;

	case CS_EVENT_CARD_REMOVAL:
		macinfo->gldm_flags &= ~ELX_CARD_PRESENT;
		macinfo->gldm_flags |= ELX_CARD_REMOVED;
		if (!(priority & CS_EVENT_PRI_HIGH)) {
			pcelx_remove_card(macinfo->gldm_devinfo,
						macinfo);
			cv_broadcast(&elx->elx_condvar);
		}
		ret = CS_SUCCESS;
		break;

	case CS_EVENT_CLIENT_INFO:
		if (GET_CLIENT_INFO_SUBSVC(ci->Attributes) ==
		    CS_CLIENT_INFO_SUBSVC_CS) {
		    ci->Revision = 0x0101;
		    ci->CSLevel = CS_VERSION;
		    ci->RevDate = CS_CLIENT_INFO_MAKE_DATE(9, 12, 14);
		    strcpy(ci->ClientName,
				"PCMCIA 3Com Ethernet card driver");
		    strcpy(ci->VendorName, CS_SUN_VENDOR_DESCRIPTION);
		    ci->Attributes |= CS_CLIENT_INFO_VALID;
		    ret = CS_SUCCESS;
		} /* CS_CLIENT_INFO_SUBSVC_CS */
		break;

	default:
		ret = CS_UNSUPPORTED_EVENT;
		break;
	}
#if 0
	if (priority & CS_EVENT_PRI_HIGH)
#endif
		mutex_exit(&elx->elx_cslock);
#if 0
	else
		mutex_exit(&macinfo->gldm_maclock);
#endif

	return (ret);
}

/*
 * pcelx_register()
 *	Register the driver with Card Services
 *	Note that this triggers lots of things
 *	Actual card setup doesn't occur until
 *	the registration event occurs.
 */
pcelx_register(dev_info_t *devinfo, gld_mac_info_t *macinfo)
{
	client_reg_t client;
	struct elxinstance *elx = (struct elxinstance *)macinfo->gldm_private;
	map_log_socket_t mapsock;
	sockmask_t sockreq;
	int ret;
#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "pcelx_register entered(%x, %x)\n",
			(int)devinfo, (int)macinfo);
#endif
	client.Attributes = INFO_IO_CLIENT|INFO_CARD_EXCL|INFO_CARD_SHARE;
	client.EventMask = CS_EVENT_CARD_INSERTION |
		CS_EVENT_CARD_REMOVAL | CS_EVENT_REGISTRATION_COMPLETE |
			CS_EVENT_CARD_REMOVAL_LOWP | CS_EVENT_CARD_READY |
				CS_EVENT_PM_RESUME | CS_EVENT_PM_SUSPEND |
					CS_EVENT_CLIENT_INFO;
	client.Version = _VERSION(2, 1);
	client.dip = devinfo;
	strcpy(client.driver_name, "pcelx");

	client.event_handler = (csfunction_t *)pcelx_pc_event;
	client.event_callback_args.client_data = macinfo;

	elx->elx_handle = (client_handle_t)-1;
	if ((ret = csx_RegisterClient(&elx->elx_handle,
				&client)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: RegisterClient failed: %x\n", ret);
		return (-1);
	}
	mutex_init(&elx->elx_cslock, "pcelx event lock", MUTEX_DRIVER,
		    client.iblk_cookie);
	macinfo->gldm_flags |= ELX_MUTEX_INIT;
	mutex_enter(&elx->elx_cslock);
	bzero((caddr_t)&mapsock, sizeof (mapsock));

	if ((ret = csx_MapLogSocket(elx->elx_handle,
				&mapsock)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: MapLogSocket failed: %x\n", ret);
		mutex_exit(&elx->elx_cslock);
		return (-1);
	}
	elx->elx_socket = mapsock.LogSocket;

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "\tsocket = %x\n", elx->elx_socket);
#endif

	/* now allow callbacks to happen */
	bzero((caddr_t)&sockreq, sizeof (sockreq));
	sockreq.EventMask =
		CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
			CS_EVENT_REGISTRATION_COMPLETE;
	if ((ret = csx_RequestSocketMask(elx->elx_handle,
				&sockreq)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: RequestSocketMask failed: %x\n", ret);
		mutex_exit(&elx->elx_cslock);
		return (-1);
	}
	macinfo->gldm_flags |= ELX_SOCKET_MASK;

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "\twait for registered event\n");
#endif
	while ((macinfo->gldm_flags & ELX_CS_READY) != ELX_CS_READY &&
		!(macinfo->gldm_flags & ELX_CONFIG_FAILED)) {
		cv_wait(&elx->elx_condvar, &elx->elx_cslock);
	}

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "\tdone\n");
#endif
	if (!(macinfo->gldm_flags & ELX_CONFIG_CONFIG)) {
		mutex_exit(&elx->elx_cslock);
		return (-1);
	}

	return (0);
}

pcelx_card_setup(dev_info_t *devinfo, gld_mac_info_t *macinfo)
{
	struct elxinstance *elx = (struct elxinstance *)macinfo->gldm_private;
	int ret, i, hi, lo;
	io_req_t io;
	irq_req_t irq;
	tuple_t tuple;
	cisparse_t cisparse;
	cistpl_config_t config;
#ifdef lint
	devinfo = devinfo;
#endif

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "pcelx_card_setup: entered\n");
#endif
	bzero((caddr_t)&tuple, sizeof (tuple));
	tuple.DesiredTuple = CISTPL_CONFIG;
	if ((ret = csx_GetFirstTuple(elx->elx_handle,
				&tuple)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: no configuration! %x\n", ret);
		return (ret);
	}

	bzero((caddr_t)&tuple, sizeof (tuple));
	tuple.DesiredTuple = CISTPL_MANFID;
	if ((ret = csx_GetFirstTuple(elx->elx_handle,
				&tuple)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: get manfid failed %x\n", ret);
		return (ret);
	}
	bzero((caddr_t)&cisparse, sizeof (cisparse));
	if ((ret = csx_Parse_CISTPL_MANFID(elx->elx_handle,
				&tuple, &cisparse.manfid)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: parse manfid failed\n");
		return (ret);
	}
	if (cisparse.manfid.manf != ELPC_MANFID_MANF ||
	    (cisparse.manfid.card != ELPC_MANFID_CARD_589 &&
		cisparse.manfid.card != ELPC_MANFID_CARD_562)) {
		cmn_err(CE_WARN, "pcelx: manfid doesn't match %x.%x (%x)\n",
			cisparse.manfid.manf, cisparse.manfid.card,
			ELPC_MANFID_MANF);
		return (ret);
	}
	if (cisparse.manfid.card == ELPC_MANFID_CARD_562) {
		macinfo->gldm_flags |= ELX_NEED_INTRBIT;
		elx->elx_features |= ELF_PROM_IN_CIS;
	}

	bzero((caddr_t)&tuple, sizeof (tuple));
	tuple.DesiredTuple = CISTPL_FUNCID;
	if ((ret = csx_GetFirstTuple(elx->elx_handle,
				&tuple)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: get funcid failed %x\n", ret);
		return (ret);
	}
	bzero((caddr_t)&cisparse, sizeof (cisparse));
	if ((ret = csx_Parse_CISTPL_FUNCID(elx->elx_handle,
				&tuple, &cisparse.funcid)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: parse funcid failed\n");
		return (ret);
	}
	if (cisparse.funcid.function != TPLFUNC_LAN) {
		cmn_err(CE_WARN, "pcelx: funcid wrong type: %x\n",
			cisparse.funcid.function);
		return (ret);
	}

	bzero((caddr_t)&tuple, sizeof (tuple));
	tuple.DesiredTuple = CISTPL_FUNCE;
	if ((ret = csx_GetFirstTuple(elx->elx_handle,
				&tuple)) == CS_SUCCESS) {
		if ((ret = csx_Parse_CISTPL_FUNCE(elx->elx_handle,
				&tuple, &cisparse.funce, TPLFUNC_LAN)) ==
		    CS_SUCCESS) {
			cmn_err(CE_WARN, "have funce!!!!!\n");
		}
	}
	bzero((caddr_t)&tuple, sizeof (tuple));
	tuple.DesiredTuple = CISTPL_CONFIG;
	if ((ret = csx_GetFirstTuple(elx->elx_handle,
				&tuple)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: no configuration!\n");
		return (ret);
	}
	bzero((caddr_t)&config, sizeof (config));
	if ((ret = csx_Parse_CISTPL_CONFIG(elx->elx_handle,
				&tuple, &config)) != CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: config failed to parse\n");
		return (ret);
	}

	if (elx->elx_features & ELF_PROM_IN_CIS) {
		bzero((caddr_t)&tuple, sizeof (tuple));
		tuple.DesiredTuple = CISTPL_VEND_SPEC_88;
		if ((ret = csx_GetFirstTuple(elx->elx_handle,
					    &tuple)) != CS_SUCCESS) {
			cmn_err(CE_WARN, "pcelx: no vend88!\n");
			return (ret);
		}
		if ((ret = csx_GetTupleData(elx->elx_handle, &tuple)) !=
		    CS_SUCCESS)
			return (ret);
		bzero((caddr_t)elx->elx_promcopy, ELX_PROM_SIZE);
		bcopy((caddr_t)tuple.TupleData, (caddr_t)elx->elx_promcopy,
			min(tuple.TupleDataLen, ELX_PROM_SIZE));
	}

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT) {
		cmn_err(CE_CONT, "\tconfig: present=%x, nr=%x, hr=%x, base=%x"
			", last=%x\n", config.present, config.nr, config.hr,
			config.base, config.last);
	}
#endif
	elx->elx_config_base = config.base;
	elx->elx_config_present = config.present;
	hi = 0;
	lo = (int)-1;		/* really big number */
	for (cisparse.cftable.index = (int)~0, i = 0;
		i != config.hr; i = cisparse.cftable.index) {
		tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
		if ((ret = csx_GetNextTuple(elx->elx_handle,
					&tuple)) != CS_SUCCESS) {
			cmn_err(CE_WARN, "pcelx: get cftable failed\n");
			break;
		}
		bzero((caddr_t)&cisparse, sizeof (cisparse));
		if ((ret = csx_Parse_CISTPL_CFTABLE_ENTRY(elx->elx_handle,
					&tuple, &cisparse.cftable)) !=
		    CS_SUCCESS) {
			cmn_err(CE_WARN, "pcelx: parse cftable failed\n");
			break;
		}
		if (cisparse.cftable.flags & CISTPL_CFTABLE_TPCE_FS_PWR &&
		    cisparse.cftable.pd.flags &
		    CISTPL_CFTABLE_TPCE_FS_PWR_VCC) {
			if (cisparse.cftable.pd.pd_vcc.avgI > hi) {
				hi = cisparse.cftable.pd.pd_vcc.avgI;
				elx->elx_config_hi = cisparse.cftable.index;
			}
			if (cisparse.cftable.pd.pd_vcc.avgI < lo) {
				lo = cisparse.cftable.pd.pd_vcc.avgI;
				elx->elx_config = cisparse.cftable.index;
			}
		}
		if (cisparse.cftable.flags &
		    CISTPL_CFTABLE_TPCE_DEFAULT) {
			if (cisparse.cftable.pd.flags &
			    CISTPL_CFTABLE_TPCE_FS_PWR_VCC) {
				elx->elx_vcc = cisparse.cftable.pd.pd_vcc.nomV;
			}
			if (cisparse.cftable.flags &
			    CISTPL_CFTABLE_TPCE_FS_IO) {
				elx->elx_iodecode =
					cisparse.cftable.io.addr_lines;
				if (cisparse.cftable.io.flags & 0x20)
					elx->elx_pcinfo |= ELPC_INFO_8BIT;
				if (cisparse.cftable.io.flags & 0x40)
					elx->elx_pcinfo |= ELPC_INFO_16BIT;
			}
		}
	}

	bzero((caddr_t)&io, sizeof (io));
	io.BasePort1.base = 0;
	io.NumPorts1 = 1 << elx->elx_iodecode;

	if (elx->elx_pcinfo & ELPC_INFO_16BIT &&
	    !(elx->elx_pcinfo & ELPC_INFO_FORCE_8BIT))
		io.Attributes1 = IO_DATA_PATH_WIDTH_16;

	io.IOAddrLines = elx->elx_iodecode;

	if ((ret = csx_RequestIO(elx->elx_handle, &io)) !=
	    CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: RequestIO failed %x\n", ret);
		return (ret);
	}


	macinfo->gldm_flags |= ELX_CONFIG_IO;
	macinfo->gldm_port = (acc_handle_t)io.BasePort1.handle;

	if (macinfo->gldm_intr_hi) {
		irq.Attributes = IRQ_TYPE_EXCLUSIVE;
		irq.irq_handler = (csfunction_t *)gld_intr_hi;
	} else {
		irq.Attributes = IRQ_TYPE_EXCLUSIVE;
		irq.irq_handler = (csfunction_t *)gld_intr;
	}
	irq.irq_handler_arg = (caddr_t)macinfo;
	if ((ret = csx_RequestIRQ(elx->elx_handle, &irq)) !=
	    CS_SUCCESS) {
		cmn_err(CE_WARN, "pcelx: RequestIRQ failed %x\n", ret);
		csx_ReleaseIO(elx->elx_handle, &io);
		macinfo->gldm_flags &= ~ELX_CONFIG_IO;
		return (ret);
	}

	mutex_init(&macinfo->gldm_intrlock, "above lock level handler",
			MUTEX_DRIVER, *irq.iblk_cookie);
	macinfo->gldm_flags |= ELX_CONFIG_IRQ;
	macinfo->gldm_cookie = *irq.iblk_cookie; /* for gld_register */

	elx->elx_irq = ELX_PCMCIA_IRQ; /* only one */

	/* now have what we need so need to finalize it */

	if ((ret = pcelx_request_config(macinfo, elx)) != CS_SUCCESS) {
		csx_ReleaseIO(elx->elx_handle, &io);
		macinfo->gldm_port = 0;
		csx_ReleaseIRQ(elx->elx_handle, &irq);
		macinfo->gldm_flags &= ~(ELX_CONFIG_IO|ELX_CONFIG_IRQ);
		return (ret);
	}

	macinfo->gldm_flags |= ELX_CONFIG_CONFIG;

#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_CONT, "calling pcelx_pcmcia_reginit(%x)\n",
			(int)macinfo);
#endif
	/* now let the card be used */
	macinfo->gldm_flags |= ELX_CARD_PRESENT;
	pcelx_pcmcia_reginit(macinfo); /* set unset registers */
#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT)
		cmn_err(CE_WARN, "returned from pcelx_pcmcia_reginit(%x)\n",
			(int)macinfo);
#endif

	return (CS_SUCCESS);
}

pcelx_request_config(gld_mac_info_t *macinfo, struct elxinstance *elx)
{
	config_req_t config;
	int ret;

	bzero((caddr_t)&config, sizeof (config));

	/* is this first time or re-insertion */
	if (!(macinfo->gldm_flags & ELX_CARD_REMOVED))
		config.Attributes = 0;
	else
		config.Attributes = CONF_ENABLE_IRQ_STEERING;

	config.Vcc = elx->elx_vcc;
	config.IntType = SOCKET_INTERFACE_MEMORY_AND_IO;

	config.ConfigBase = elx->elx_config_base;
	/* note: documentation is wrong - use the index as is */
	config.ConfigIndex = elx->elx_config;
	if (macinfo->gldm_flags & ELX_NEED_INTRBIT) {
		config.ConfigIndex &= ~0x8;
	}
	config.Status = 0x20;

	config.Present = elx->elx_config_present;
#if defined(ELXDEBUG)
	if (pcelx_debug & ELXINIT) {
		cmn_err(CE_CONT, "RequestConfiguration:\n");
		cmn_err(CE_WARN, "\tVcc=%d, ConfigBase=%x,"
			"ConfigIndex=%d, Present=%x\n",
			(int)config.Vcc, (int)config.ConfigBase,
			(int)config.ConfigIndex, (int)config.Present);
	}
#endif
	if ((ret = csx_RequestConfiguration(elx->elx_handle,
				&config)) != CS_SUCCESS) {
		cmn_err(CE_WARN,
			"pcelx: RequestConfiguration failed %x\n", ret);
	}

	macinfo->gldm_flags |= ELX_CONFIG_CONFIG;
	return (ret);
}

void
pcelx_remove_card(dev_info_t *devinfo, gld_mac_info_t *macinfo)
{
	struct elxinstance *elx = (struct elxinstance *)macinfo->gldm_private;
	int ret;
	io_req_t io;
	irq_req_t irq;
#ifdef lint
	devinfo = devinfo;
#endif

	if (macinfo->gldm_flags & ELX_CONFIG_CONFIG) {
		if ((ret = csx_ReleaseConfiguration(elx->elx_handle, NULL)) !=
		    CS_SUCCESS) {
			cmn_err(CE_WARN,
				"pcelx: couldn't release Config (%x)\n", ret);
		}
		macinfo->gldm_port = 0;
		macinfo->gldm_flags &= ~ELX_CONFIG_CONFIG;
	}

	if (macinfo->gldm_flags & ELX_CONFIG_IO) {
		bzero((caddr_t)&io, sizeof (io));
		io.BasePort1.handle = (void *) macinfo->gldm_port;
		io.NumPorts1 = 16;
		if ((ret = csx_ReleaseIO(elx->elx_handle, &io)) !=
		    CS_SUCCESS) {
			cmn_err(CE_WARN,
				"pcelx: couldn't release IO (%x)\n", ret);
		} else {
			macinfo->gldm_port = 0;
			macinfo->gldm_flags &= ~ELX_CONFIG_IO;
		}
	}

	if (macinfo->gldm_flags & ELX_CONFIG_IRQ) {
		bzero((caddr_t)&irq, sizeof (irq));
		if ((ret = csx_ReleaseIRQ(elx->elx_handle, &irq)) !=
		    CS_SUCCESS) {
			cmn_err(CE_WARN,
				"pcelx: couldn't release IRQ (%x)\n", ret);
		} else {
			macinfo->gldm_flags &= ~ELX_CONFIG_IRQ;
		}
	}
}

void
pcelx_unregister_client(dev_info_t *devinfo, gld_mac_info_t *macinfo)
{
	struct elxinstance *elx = (struct elxinstance *)macinfo->gldm_private;
	int ret;
	release_socket_mask_t mask;

	pcelx_remove_card(devinfo, macinfo);
	if (macinfo->gldm_flags & ELX_SOCKET_MASK) {
		/*
		 * want to remove socket mask to make sure
		 * we don't get events at all at this point
		 * since we are going away.
		 */
		mask.Socket = macinfo->gldm_ppa;
		(void) csx_ReleaseSocketMask(elx->elx_handle, &mask);
		macinfo->gldm_flags &= ~ELX_SOCKET_MASK;
	}

	/*
	 * we grab the mutex to make sure a callback
	 * that was in progress has completed before we
	 * go any further.
	 */
	if (macinfo->gldm_flags & ELX_MUTEX_INIT)
		mutex_enter(&elx->elx_cslock);

	if (macinfo->gldm_flags & ELX_REGISTERED) {
		if ((ret = csx_DeregisterClient(elx->elx_handle)) !=
		    CS_SUCCESS)
			cmn_err(CE_WARN, "pcelx: unregister failed %x\n", ret);
	}
	if (macinfo->gldm_flags & ELX_MUTEX_INIT) {
		mutex_destroy(&elx->elx_cslock);
		macinfo->gldm_flags &= ~ELX_MUTEX_INIT;
	}
	macinfo->gldm_flags &= ~ELX_REGISTERED;
}

void
pcelx_pcmcia_reginit(gld_mac_info_t *macinfo)
{
	struct elxinstance *elx = (struct elxinstance *)macinfo->gldm_private;
	register acc_handle_t port = macinfo->gldm_port;
	ushort value;
#ifdef lint
	elx = elx;
#endif

	/*
	 * there are a number of board registers that should be set from
	 * here since the hardware doesn't initialize them
	 */
	SET_WINDOW(port, 0);

	/* ELX_PRODUCT_ID */
	value = pcelx_read_prom(macinfo, port, EEPROM_PROD_ID);
	csx_Put16(port, ELX_PRODUCT_ID, value);

	/* ELX_ADDRESS_CFG (Address Configuration) */
	value = pcelx_read_prom(macinfo, port, EEPROM_ADDR_CFG);
	value &= 0xc000;
	csx_Put16(port, ELX_ADDRESS_CFG, value);

	/* ELX_RESOURCE_CFG (Resource Configuration) */
	value = pcelx_read_prom(macinfo, port, EEPROM_RESOURCE_CFG);
	if (GET_IRQ(value) != ELX_PCMCIA_IRQ) {
		value = (ELX_PCMCIA_IRQ << 12) | (value & 0xfff);
		csx_Put16(port, ELX_RESOURCE_CFG, value);
	}
}

void
pcelx_get_addr(gld_mac_info_t *macinfo, u_short *addr)
{
	acc_handle_t port;
	int i, which;
	struct elxinstance *elxp =
		(struct elxinstance *)macinfo->gldm_private;

	if (!(macinfo->gldm_flags & ELX_CARD_PRESENT))
		return;

	/*
	 * NOTE: for SPARC should use the system's address
	 */

	port = macinfo->gldm_port;
	if (elxp->elx_features & ELF_USE_3COM_NODE)
		which = EEPROM_PHYS_ADDR;
	else
		which = EEPROM_OEM_ADDR;

	/* Get the board's vendor-assigned hardware network address */
	SET_WINDOW(port, 0);
	for (i = 0; i < 3 && macinfo->gldm_flags & ELX_CARD_PRESENT; i ++) {
		int val;
		val = pcelx_read_prom(macinfo, port, i + which);
		if (val != -1) {
			*(addr + i) =
				ntohs(val);
		}
	}
}
void
pcelx_patch_addr(gld_mac_info_t *macinfo)
{
	unchar addr[ETHERADDRL+2];

	pcelx_get_addr(macinfo, (u_short *)addr);

	if (bcmp((caddr_t)addr, (caddr_t)macinfo->gldm_vendor,
		    ETHERADDRL) != 0 &&
	    bcmp((caddr_t)macinfo->gldm_vendor,
		    (caddr_t)macinfo->gldm_macaddr, ETHERADDRL) == 0) {
#if defined(ELXDEBUG)
		if (pcelx_debug & ELXTRACE) {
			cmn_err(CE_CONT, "pcelx: not the same card: new=%s\n",
				ether_sprintf((struct ether_addr *)addr));
			cmn_err(CE_CONT, "	 old=%s\n",
				ether_sprintf((struct ether_addr *)
						macinfo->gldm_macaddr));
			cmn_err(CE_CONT, "     vendor=%s\n",
				ether_sprintf((struct ether_addr *)
						macinfo->gldm_vendor));
		}
#endif
		bcopy((caddr_t)addr, (caddr_t)macinfo->gldm_vendor,
			ETHERADDRL);
	}
}

void
pcelx_announce_devs(dev_info_t *devinfo, gld_mac_info_t *macinfo)
{
	struct elxinstance *elx =
		(struct elxinstance *)macinfo->gldm_private;
	make_device_node_t mdev;
	struct devnode_desc desc[2];
	char name[64];
#ifdef lint
	devinfo = devinfo;
#endif
	mdev.Action = CREATE_DEVICE_NODE;
	mdev.NumDevNodes = 2;
	mdev.devnode_desc = desc;
	desc[0].name = "pcelx";
	desc[0].spec_type = S_IFCHR;
	desc[0].minor_num = 0;
	desc[0].node_type = DDI_NT_NET;
	sprintf(name, "%s%d", "pcelx", (int)macinfo->gldm_ppa);
	desc[1].name = name;
	desc[1].spec_type = S_IFCHR;
	desc[1].minor_num = macinfo->gldm_ppa + 1;
	desc[1].node_type = DDI_NT_NET;
	(void) csx_MakeDeviceNode(elx->elx_handle, &mdev);
}

#ifdef ELXDEBUG
void
pcelx_dmp_regs(acc_handle_t port)
{
	int value, window, i;
	cmn_err(CE_CONT, "pcelx: register dump\n");
	value = INW(port, ELX_STATUS);
	window = GET_WINDOW(value);
	SET_WINDOW(port, 0);
	cmn_err(CE_CONT, "    window 0\n");
	cmn_err(CE_CONT, "\tstatus	: %b\n", (long)INW(port, 0xe),
		"\20\1IL\2AF\3TC\4TA\5RC\6RE\7IR\10US\15IP\17ISA");
	cmn_err(CE_CONT, "\tresource cfg: %x IRQ: %x\n",
		INW(port, 0x8), (int)INW(port, 0x8) >> 12);
	value = INW(port, 6);
	cmn_err(CE_CONT,
		"\taddress cfg : %x IO: %x ROM: %x ROM Size: %x XCVR: %x\n",
		value, value & 0x3F, (value >> 8) & 0xF,
		(value >> 12) & 0x3,
		value >> 14);
	cmn_err(CE_CONT, "\tconfig ctl	: %b\n", (long)INW(port, 4),
		"\20\1ENA\3RST\11INTDEC\12TP\13N1\14N2\15BNC"
		"\16AUI\17ISA\20TYPEB");
	cmn_err(CE_CONT, "\tproduct id	: %x\n", INW(port, 2));
	cmn_err(CE_CONT, "\tmanufacturer: %x\n", INW(port, 0));
	cmn_err(CE_CONT, "    window 1\n");
	SET_WINDOW(port, 1);
	cmn_err(CE_CONT, "\tfree tx byte: %x\n", INW(port, 0xc));
	cmn_err(CE_CONT, "\ttx status	: %b\n", (long)csx_Get8(port, 0xb),
		"\20\1UNDEF\2RE\3OF\4MC\5UN\6JB\7IS\10CM");
	cmn_err(CE_CONT, "\tas short: %x\n", INW(port, 0xa));
	value = INW(port, 0x8);
	cmn_err(CE_CONT, "\trx status	: %b len: %x\n",
		(long)(value & 0x3FF), "\20\17ER\20IC", value & 0xFFF);
	cmn_err(CE_CONT, "    window 2\n");
	SET_WINDOW(port, 2);
	for (i = 0; i < 6; i++)
		cmn_err(CE_CONT, "\tetheraddr %d: %x\n", i, csx_Get8(port, i));
	cmn_err(CE_CONT, "    window 3\n");
	SET_WINDOW(port, 3);
	value = INL(port, ELX_INTERNAL_CONFIG);
	cmn_err(CE_CONT, "\tinternal cf: <%x> rsize=%d, width=%d, speed=%d, "
		"partition=%x, activate=%x\n",
		value,
		value & 0x7, 8 << ((value >> 3) & 1), (value >> 4) & 0x3,
		(value	>> 16) & 0x3, (value >> 18) & 0x3);
	SET_WINDOW(port, 4);
	cmn_err(CE_CONT, "    window 4\n");
	cmn_err(CE_CONT, "\tmedia type	: %b\n", (long)INW(port, 0xa),
		"\20\3CSD\4SQE\5CO\6CS\7JE\10LK\11US\21J\13PL\14LB"
		"\15SQ\16IN\17CE\20TPE");
	cmn_err(CE_CONT, "\tnet diag	: %b\n", (long)INW(port, 0x6),
		"\20\1TLD\10SE\11TXR\12TXT\13RXE\14TXE\15FL\16EXL\17ENL\20EL");
	SET_WINDOW(port, 5);
	cmn_err(CE_CONT, "    window 5\n");
	cmn_err(CE_CONT, "\tint mask	: %b\n", (long)INW(port, 0xa),
		"\20\1IL\2AF\3TC\4TA\5RC\6RE\7IR\10US\15IP");
	cmn_err(CE_CONT, "\tzero mask	: %b\n", (long)INW(port, 0xc),
		"\20\1IL\2AF\3TC\4TA\5RC\6RE\7IR\10US\15IP");
	cmn_err(CE_CONT, "\trx filter	: %b\n", (long)INW(port, 8),
		"\20\1IN\2MC\3BD\4PR");
	SET_WINDOW(port, window);
}
#endif

/*
 * sparc/PPC require in/out routines that handle
 * non 32-bit aligned data for the FIFO.  We just
 * make all systems do it
 */
void
pcelx_repinsd(acc_handle_t port, unsigned long *buff, int off, int count)
{
	int i, j;

	if (count < 0) {
		return;
	}

	i = (u_long)buff & 0x3;
	j = i;

	if (i > 0) {
		i = 4 - i;
		csx_RepGet8(port, (unsigned char *)buff, off, i, 0);
		count -= 1;
		buff = (unsigned long *)((unsigned char *)buff + i);
	}
	csx_RepGet32(port, buff, off, count, 0);
	buff += count;
	if (i > 0) {
		csx_RepGet8(port, (unsigned char *)buff, off, j, 0);
	}
}

void
pcelx_repoutsd(acc_handle_t port, unsigned long *buff, int off, int count)
{
	register int i, j;

	if (count < 0) {
		return;
	}
	i = (u_long)buff & 0x3;
	j = i;
	if (i > 0) {
		i = 4 - i;
		csx_RepPut8(port, (unsigned char *)buff, off, i,
				DDI_DEV_NO_AUTOINCR);
		count -= 1;
		buff = (unsigned long *)((unsigned char *)buff + i);
	}
	csx_RepPut32(port, buff, off, count, DDI_DEV_NO_AUTOINCR);
	buff += count;
	if (i > 0) {
		csx_RepPut8(port, (unsigned char *)buff, off, j,
				DDI_DEV_NO_AUTOINCR);
	}
}

/*
 * pcelx_probe_media(gld_mac_info_t *macinfo)
 */
void
pcelx_probe_media(gld_mac_info_t *macinfo)
{
#ifdef lint
	macinfo = macinfo;
#endif
}
