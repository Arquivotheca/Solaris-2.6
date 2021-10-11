/*
 * smce -- SMC Elite32/EISA Depends on the Generic LAN Driver utility
 * functions in /kernel/misc/gld
 */

/* Copyright (c) 1995 Sun Microsystems, Inc. */

#pragma ident	"@(#)smce.c	1.12	95/03/22 SMI"

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
#include <sys/debug.h>

#ifdef	_DDICT
#include "sys/dlpi.h"
#include "sys/ethernet.h"
#include "sys/gld.h"
#include "sys/eisarom.h"
#include "sys/nvm.h"
#else
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/gld.h>
#include <sys/eisarom.h>
#include <sys/nvm.h>
#endif

#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "sys/smce.h"
#include "sys/smce_lm.h"

/*
 * Declarations and Module Linkage
 */

static char ident[] = "SMC Elite32/EISA 2-channel Ethernet driver";

#ifdef SMCEDEBUG
int	smcedebug = 0;
#endif

/* Required system entry points */
static int smceidentify(dev_info_t *);
static int smcedevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static int smceprobe(dev_info_t *);
static int smceattach(dev_info_t *, ddi_attach_cmd_t);
static int smcedetach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
static int smce_reset(gld_mac_info_t *);
static int smce_start_board(gld_mac_info_t *);
static int smce_stop_board(gld_mac_info_t *);
static int smce_saddr(gld_mac_info_t *);
static int smce_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
static int smce_prom(gld_mac_info_t *, int);
static int smce_gstat(gld_mac_info_t *);
static int smce_send(gld_mac_info_t *, mblk_t *);
static u_int smceintr(gld_mac_info_t *);

static int smce_init_board(gld_mac_info_t *, int);
static void flush_packet(unsigned long, int);
static int smce_getioaddr(dev_info_t *);

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/* Standard Streams initialization */

static struct module_info minfo = {
	SMCEIDNUM, "smce", 0, INFPSZ, SMCEHIWAT, SMCELOWAT
};

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
};

static struct streamtab smceinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static struct cb_ops cb_smceops = {
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
	&smceinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

static struct dev_ops  smceops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	smcedevinfo,		/* devo_getinfo */
	smceidentify,		/* devo_identify */
	smceprobe,		/* devo_probe */
	smceattach,		/* devo_attach */
	smcedetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_smceops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&smceops		/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *) &modldrv, NULL
};

_init(void)
{
	return (mod_install(&modlinkage));
}

_fini(void)
{
	return (mod_remove(&modlinkage));
}

_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 * DDI Entry Points
 */

/* identify(9E) -- See if we know about this device */

static int
smceidentify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "smce") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

/* getinfo(9E) -- Get device driver information */

static int
smcedevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int    error;

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
 * old probe(9E) -- Determine if a device is present.
 * This is the pre-2.5 code.
 */

static int
smce_old_probe(dev_info_t *devinfo)
{
	int		i;
	int		found_board = 0;
	int		slot;
	static int	lastslot = 0;
	NVM_SLOTINFO	*nvm;
	caddr_t		data;
	int		port = 0;
	int		regbuf[3];
	static unsigned char smce_id[4] = {0x4d, 0xa3, 0x01, 0x10};

#ifdef SMCEDEBUG
	if (smcedebug & SMCEDDI)
		cmn_err(CE_CONT, "smce_old_probe(0x%x)", devinfo);
#endif

	data = (caddr_t)kmem_zalloc(SMCE_MAX_EISABUF, KM_NOSLEEP);
	if (data == NULL)
		return (DDI_PROBE_FAILURE);

	for (slot = lastslot + 1; slot < EISA_MAXSLOT; slot++) {
		i = eisa_nvm(data, EISA_SLOT, slot);
		if (i == 0)
			continue;
		nvm = (NVM_SLOTINFO *) (data + sizeof (short));
		/*
		 * Check for board identification match.
		 * Ignore versioning differences (4th byte - low nibble)
		 */
		if ((nvm->boardid[0] != smce_id[0]) ||
		    (nvm->boardid[1] != smce_id[1]) ||
		    (nvm->boardid[2] != smce_id[2]) ||
		    ((nvm->boardid[3] & 0xf0) != smce_id[3]))
			continue;
		/* create ioaddr property */
		port = slot * 0x1000;
		regbuf[0] = port;
		(void) ddi_prop_create(DDI_DEV_T_NONE, devinfo,
				DDI_PROP_CANSLEEP, "ioaddr", (caddr_t)regbuf,
				sizeof (int));
		found_board++;
		break;
	}
	lastslot = slot;
	kmem_free(data, SMCE_MAX_EISABUF);

	/*
	 * Return whether the board was found.  If unable to determine
	 * whether the board is present, return DDI_PROBE_DONTCARE.
	 */
	if (found_board) {
#ifdef SMCEDEBUG
		if (smcedebug & SMCETRACE)
			cmn_err(CE_CONT,
			    "smce_old_probe:found board at 0x%x", port);
#endif
		return (DDI_PROBE_SUCCESS);
	} else
		return (DDI_PROBE_FAILURE);
}

/* probe(9E) -- Determine if a device is present */

static int
smceprobe(dev_info_t *devinfo)
{
	int *irqarr, irqlen, nintrs;
	int ioaddr = 0;
	caddr_t 	data;
	NVM_SLOTINFO	*nvm;
	static unsigned char smce_id[4] = {0x4d, 0xa3, 0x01, 0x10};
	static ushort smce_irq_array[] = {3, 4, 5, 7, 9, 10, 11, 12};
	unsigned char c;

#ifdef SMCEDEBUG
		cmn_err(CE_CONT, "smceprobe(0x%x)", devinfo);
#endif
	if (ddi_getprop(DDI_DEV_T_ANY, devinfo, 0,
	    "ignore-hardware-nodes", 0) != 0) {
		return (smce_old_probe(devinfo));
	}

	if ((ioaddr = smce_getioaddr(devinfo)) < 0)
		goto failure;

	if ((ioaddr == 0) || (ioaddr & 0xFFF)) {
		cmn_err(CE_WARN, "smce:bad I/O address 0x%x not z000", ioaddr);
		goto failure;
	}

	/*
	 * Check the smce card signature at the slot indicated by the ioaddr
	 */
	data = (caddr_t)kmem_zalloc(SMCE_MAX_EISABUF, KM_NOSLEEP);
	if (data == NULL)
		goto failure;

	if (!eisa_nvm(data, EISA_SLOT, (int)(ioaddr/0x1000)))
		goto failure1;

	nvm = (NVM_SLOTINFO *) (data + sizeof (short));
	/*
	 * Check for board identification match.
	 * Ignore versioning differences (4th byte - low nibble)
	 */
	if ((nvm->boardid[0] != smce_id[0]) ||
	    (nvm->boardid[1] != smce_id[1]) ||
	    (nvm->boardid[2] != smce_id[2]) ||
	    ((nvm->boardid[3] & 0xf0) != smce_id[3]))
		goto failure1;

	kmem_free(data, SMCE_MAX_EISABUF);

	/*
	 * Verify the board IRQ against IRQ in node passed to probe
	 */
	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo,
			    DDI_PROP_DONTPASS, "interrupts",
			    (caddr_t)&irqarr, &irqlen) !=
				DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN, "smce: interrupts property not found "
		    "in devices property list");
		goto failure;
	}

	if (ddi_dev_nintrs(devinfo, &nintrs) != DDI_SUCCESS || nintrs != 1) {
		cmn_err(CE_WARN, "smce: interrupts property must specify "
				"exactly one interrupt.");
		goto failure;
	}

	c = inb(ioaddr + MODE_REGISTER_OFFSET + 1);
	c &= 0x0E;
	c >>= 1;
	if (smce_irq_array[c] != irqarr[(irqlen / sizeof (int)) - 1]) {
		cmn_err(CE_WARN,
		"smce: interrupts property IRQ does not match board");
		goto failure;
	}

	return (DDI_PROBE_SUCCESS);

failure1:
	kmem_free(data, SMCE_MAX_EISABUF);
failure:
	return (DDI_PROBE_FAILURE);
}

/*
 * attach(9E) -- Attach a device to the system
 *
 * Called once for each board successfully probed.
 */

static int
smceattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo, *macinfo2;	/* GLD structure */
	struct smceinstance *smcep, *smcep2;	/* Our private device info */
	struct smparam *smp, *smp2;	/* lower MAC wrapper support */
	struct sm_common *smcp, *smcp2;
	struct smceboard *smcebp;	/* per board structure */
	int    old_config;

#ifdef SMCEDEBUG
	if (smcedebug & SMCEDDI)
		cmn_err(CE_CONT, "smceattach(0x%x)", devinfo);
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	old_config = (ddi_getprop(DDI_DEV_T_ANY, devinfo, 0,
	    "ignore-hardware-nodes", 0) != 0);
	/*
	 * Allocate gld_mac_info_t and smceinstance structures. We need to
	 * allocate a pair of each for the dual-channel card, and also
	 * a per board structure that is being shared by both channels.
	 */
	macinfo = (gld_mac_info_t *)kmem_zalloc((2 * sizeof (gld_mac_info_t) +
					2 * sizeof (struct smceinstance) +
					2 * sizeof (struct smparam) +
					2 * sizeof (struct sm_common) +
					sizeof (struct smceboard)),
					KM_NOSLEEP);
	if (macinfo == NULL)
		return (DDI_FAILURE);
	macinfo2 = macinfo + 1;
	smcep = (struct smceinstance *)(macinfo + 2);
	smcep2 = smcep + 1;
	smp = (struct smparam *)(smcep + 2);
	smp2 = smp + 1;
	smcp = (struct sm_common *)(smp + 2);
	smcp2 = smcp + 1;
	smcebp = (struct smceboard *)(smcp + 2);

	/*
	 * Since this module is acting as a wrapper to the LMAC module, we
	 * also allocate a pair of struct smparam to talk to the LMAC. We
	 * allocate a structure pair because there are 2 ethernet channels,
	 * thus 2 ethernet controllers on 1 board.
	 */

	/* Initialize our private fields in macinfo and smceinstance */
	macinfo->gldm_private = (caddr_t)smcep;
	macinfo2->gldm_private = (caddr_t)smcep2;
	if (old_config)
		macinfo->gldm_port = macinfo2->gldm_port =
			ddi_getprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
				    "ioaddr", 0);
	else
		macinfo->gldm_port = macinfo2->gldm_port =
						smce_getioaddr(devinfo);
	macinfo->gldm_state = macinfo2->gldm_state = SMCE_IDLE;
	macinfo->gldm_flags = macinfo2->gldm_flags = 0;
	smcep->smp = smp;
	smcep2->smp = smp2;
	smcep->channel = 1;
	smcep2->channel = 2;
	smcep->smcebp = smcebp;		/* both channels point to the same */
	smcep2->smcebp = smcebp;	/* ... board structure */
	smp->sm_cp = smcp;
	smp2->sm_cp = smcp2;
	smp->private = (caddr_t)macinfo;
	smp2->private = (caddr_t)macinfo2;
	smp->devinfo = smp2->devinfo = devinfo;
	smcebp->smce1p = smcep;
	smcebp->smce2p = smcep2;

	/*
	 * Initialize pointers to device specific functions which will be
	 * used by the generic layer.
	 */
	macinfo->gldm_reset = macinfo2->gldm_reset = smce_reset;
	macinfo->gldm_start = macinfo2->gldm_start = smce_start_board;
	macinfo->gldm_stop = macinfo2->gldm_stop = smce_stop_board;
	macinfo->gldm_saddr = macinfo2->gldm_saddr = smce_saddr;
	macinfo->gldm_sdmulti = macinfo2->gldm_sdmulti = smce_dlsdmult;
	macinfo->gldm_prom = macinfo2->gldm_prom = smce_prom;
	macinfo->gldm_gstat = macinfo2->gldm_gstat = smce_gstat;
	macinfo->gldm_send = macinfo2->gldm_send = smce_send;
	macinfo->gldm_ioctl = macinfo2->gldm_ioctl = NULL;
	macinfo->gldm_intr = macinfo2->gldm_intr = smceintr;

	/*
	 * Initialize board characteristics needed by the generic layer.
	 */
	macinfo->gldm_ident = macinfo2->gldm_ident = ident;
	macinfo->gldm_type = macinfo2->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = macinfo2->gldm_minpkt = 0;
	macinfo->gldm_maxpkt = macinfo2->gldm_maxpkt = SMCEMAXPKT;
	macinfo->gldm_addrlen = macinfo2->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = macinfo2->gldm_saplen = -2;

	/*
	 * Prepare the wrapper structure before calling.  The LMAC expects
	 * these to have been initialized and it will fill in some of the
	 * rest: io_base, irq_value
	 */
	smp->io_base = smp2->io_base = (ushort) macinfo->gldm_port;
	{
		static ushort smce_irq_array[] = {3, 4, 5, 7, 9, 10, 11, 12};
		unsigned char c = inb(smp->io_base + MODE_REGISTER_OFFSET + 1);
		unsigned char d;

#ifdef SMCEDEBUG
		if (smcedebug & SMCEALAN)
			cmn_err(CE_CONT, "smceattach: c = 0x%x ", c);
#endif
		d = c;
		c &= 0x0E;
		c >>= 1;
		smp->irq_value = smp2->irq_value = smce_irq_array[c];
		if (old_config)
			macinfo->gldm_irq_index = macinfo2->gldm_irq_index = c;
		else
			macinfo->gldm_irq_index = macinfo2->gldm_irq_index = 0;

#ifdef SMCEDEBUG
		if (smcedebug & SMCEALAN)
			cmn_err(CE_CONT, "smceattach: irq = %d ",
				smp->irq_value);
#endif
		smp->media_type = smp2->media_type = (d & 0x10) ?
							MEDIA_BNC : AUI_UTP;
		smp->sm_cp->sm_init = smp2->sm_cp->sm_init = 0;
	}
	macinfo->gldm_reg_index = macinfo2->gldm_reg_index = -1;

	/*
	 * Do anything necessary to prepare the board for operation short of
	 * actually starting the board.
	 */
	(void) smce_init_board(macinfo, 0);

	/* Get the board's vendor-assigned hardware network address */
	bcopy((caddr_t)smp->node_address, (caddr_t)macinfo->gldm_vendor,
	    ETHERADDRL);
	bcopy((caddr_t)smp2->node_address, (caddr_t)macinfo2->gldm_vendor,
	    ETHERADDRL);

#ifdef SMCEDEBUG
	if (smcedebug & SMCEALAN) {
		int i;

		cmn_err(CE_CONT, "\nEthernet address 1");
		for (i = 0; i < ETHERADDRL; i++)
			cmn_err(CE_CONT, ":%2x",
				smp->node_address[i]);
		cmn_err(CE_CONT, "\nEthernet address 2");
		for (i = 0; i < ETHERADDRL; i++)
			cmn_err(CE_CONT, ":%2x",
				smp2->node_address[i]);
		cmn_err(CE_CONT, "\n");
	}
#endif


	/*
	 * set the connector/media type if it can be determined
	 * determined by Mode register High byte bit 4: 0:AUI, 1=TP/BNC
	 */
	macinfo->gldm_media = macinfo2->gldm_media = GLDM_UNKNOWN;

	bcopy((caddr_t)gldbroadcastaddr,
	    (caddr_t)macinfo->gldm_broadcast, ETHERADDRL);
	bcopy((caddr_t)gldbroadcastaddr,
	    (caddr_t)macinfo2->gldm_broadcast, ETHERADDRL);

	bcopy((caddr_t)macinfo->gldm_vendor,
	    (caddr_t)macinfo->gldm_macaddr, ETHERADDRL);
	bcopy((caddr_t)macinfo2->gldm_vendor,
	    (caddr_t)macinfo2->gldm_macaddr, ETHERADDRL);

	/* Make sure we have our address set */
	(void) smce_saddr(macinfo);
	(void) smce_saddr(macinfo2);

	/*
	 * Register ourselves with the GLD interface
	 *
	 * gld_register will: link us with the GLD system; set our
	 * ddi_set_driver_private(9F) data to the macinfo pointer; save the
	 * devinfo pointer in macinfo->gldm_devinfo; map the registers,
	 * putting the kvaddr into macinfo->gldm_memp; add the interrupt,
	 * putting the cookie in gldm_cookie; init the gldm_intrlock mutex
	 * which will block that interrupt; create the minor node.
	 */
	/*
	 * Since the card has 2 channels, call gld_register() twice to
	 * have 2 ppa's assigned.  However, this will have the effect of
	 * creating 2 mutex locks for 1 physical board, and will lose
	 * the protection that mutexes are intended for.  Therefore,
	 * create another mutex in the per board structure and let the
	 * 2 mutexes funnel through this single mutex.
	 */
	mutex_init(&(smcebp->smcelock), "smce board lock", MUTEX_DRIVER,
				macinfo->gldm_cookie);

	if (gld_register(devinfo, "smce", macinfo) != DDI_SUCCESS) {
		mutex_destroy(&(smcebp->smcelock));
		kmem_free((caddr_t)macinfo,
		    2 * sizeof (gld_mac_info_t) +
		    2 * sizeof (struct smceinstance) +
		    2 * sizeof (struct smparam) +
		    2 * sizeof (struct sm_common) +
		    sizeof (struct smceboard));
		return (DDI_FAILURE);
	}


	if (gld_register(devinfo, "smce", macinfo2) != DDI_SUCCESS) {
		/* clean up */
		gld_unregister(macinfo);
		mutex_destroy(&(smcebp->smcelock));
		kmem_free((caddr_t)macinfo,
		    2 * sizeof (gld_mac_info_t) +
		    2 * sizeof (struct smceinstance) +
		    2 * sizeof (struct smparam) +
		    2 * sizeof (struct sm_common) +
		    sizeof (struct smceboard));
		return (DDI_FAILURE);
	}
	return (DDI_SUCCESS);
}

/* detach(9E) -- Detach a device from the system */

static int
smcedetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo;	/* GLD structure */
	struct smceinstance *smcep;	/* Our private device info */
	struct smceboard *smcebp;	/* per board structure pointer */

#ifdef SMCEDEBUG
	if (smcedebug & SMCEDDI)
		cmn_err(CE_CONT, "smcedetach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH)
		return (DDI_FAILURE);

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);
	smcep = (struct smceinstance *)(macinfo->gldm_private);
	smcebp = (struct smceboard *)(smcep->smcebp);

	/* stop the board if it is running */
	(void) smce_stop_board(macinfo);

	/*
	 * Unregister ourselves from the GLD interface
	 *
	 * gld_unregister will: remove the minor node; unmap the registers;
	 * remove the interrupt; destroy the gldm_intrlock mutex; unlink us
	 * from the GLD system.
	 */
	/* Should do 2 gld_unregister()'s because of the 2 channels */
	/* First unregister channel 2 */
	if (smcep->channel == 1)
		macinfo++;	/* bring it to channel 2 */
	if (gld_unregister(macinfo) != DDI_SUCCESS)
		return (DDI_FAILURE);

	/* Then unregister channel 1 */
	macinfo--;
	if (gld_unregister(macinfo) == DDI_SUCCESS) {
		mutex_destroy(&(smcebp->smcelock));
		kmem_free((caddr_t)macinfo,
		    2 * sizeof (gld_mac_info_t) +
		    2 * sizeof (struct smceinstance) +
		    2 * sizeof (struct smparam) +
		    2 * sizeof (struct sm_common) +
		    sizeof (struct smceboard));
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}

/*
 * GLD Entry Points
 */

/*
 * smce_reset() -- reset the board to initial state; save the machine address
 * and restore it afterwards.
 */

static int
smce_reset(gld_mac_info_t *macinfo)
{
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	unchar  macaddr[ETHERADDRL];

#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE)
		cmn_err(CE_CONT, "smce_reset(0x%x), channel = %d",
			macinfo, smcep->channel);
#endif

	(void) smce_stop_board(macinfo);
	bcopy((caddr_t)macinfo->gldm_macaddr, (caddr_t)macaddr,
	    macinfo->gldm_addrlen);
	(void) smce_init_board(macinfo, smcep->channel);
	bcopy((caddr_t)macaddr, (caddr_t)macinfo->gldm_macaddr,
	    macinfo->gldm_addrlen);
	(void) smce_saddr(macinfo);
	return (0);
}

/*
 * smce_init_board() -- initialize the specified network board.
 * 2nd argument channel
 * 0: init both channels and the board too
 * 1: init only channel 1, do not touch the board
 * 2: init only channel 2, do not touch the board
 */

static int
smce_init_board(gld_mac_info_t *macinfo, int channel)
{
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	struct smparam *smp;		/* lower MAC structure */
	struct smceboard *smcebp; 	/* board structure */

	/* Don't initialize the whole board more than once */

	/* get pointer to board struct */
	smcebp = smcep->smcebp;

	if (channel == 0) {
		if (smcebp->board_initialized)
			return (0);
		else
			smcebp->board_initialized = 1;
	}

	/*
	 * In order to support the dual-channel board, the LM interface
	 * required that a pair of "struct smparam" structures be passed to
	 * lm_initialize_adapter().  We need to find out the first channel's
	 * smp and pass that to lm_initialize_adapter.
	 */

	/* get channel 1's smparam pointer from the board struct */
	smp = smcebp->smce1p->smp;

	smce_lm_initialize_adapter(smp, channel);
	return (0);
}

/*
 * smce_start_board() -- start the board receiving and allow transmits.
 */

static int
smce_start_board(gld_mac_info_t *macinfo)
{
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	struct smparam *smp = smcep->smp;	  /* lower MAC structure */
	struct smceboard *smcebp = smcep->smcebp; /* per board structure */

#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE)
		cmn_err(CE_CONT, "smce_start_board(0x%x), channel %d\n",
			macinfo, smcep->channel);
#endif

	mutex_enter(&(smcebp->smcelock));
	smce_lm_open_adapter(smp);
	mutex_exit(&(smcebp->smcelock));
	return (0);
}

/*
 * smce_stop_board() -- stop board receiving
 */

static int
smce_stop_board(gld_mac_info_t *macinfo)
{
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	struct smparam *smp = smcep->smp;	  /* lower MAC structure */
	struct smceboard *smcebp = smcep->smcebp; /* per board structure */

#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE)
		cmn_err(CE_CONT, "smce_stop_board(0x%x), channel %d\n", macinfo,
			smcep->channel);
#endif

	mutex_enter(&(smcebp->smcelock));
	smce_lm_close_adapter(smp);
	mutex_exit(&(smcebp->smcelock));
	return (0);
}

/*
 * smce_saddr() -- set the physical network address on the board
 */

static int
smce_saddr(gld_mac_info_t *macinfo)
{
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	struct smparam *smp = smcep->smp;
	unsigned long   nice_offset;
	unsigned long   ctrl_addr;
	unsigned char   ctrl_reg;
	unsigned char   save_reg;
	int		disabled;
	int		i;

#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE)
		cmn_err(CE_CONT, "smce_saddr(0x%x), channel %d\n", macinfo,
			smcep->channel);
#endif

	/*
	 * We need to disable the NICE by writing a '1' into DLC6<7>.
	 * This diables the datalink controller and enables access to the
	 * node ID registers (DLCR8 through DLCR13) through the selection of
	 * that register bank. After this, re-enable the NICE.
	 *
	 * This is a channel only operation so do not need to acquire
	 * the board mutex.
	 */
	ctrl_addr = smp->io_base + smp->sm_nice_addr + CONTROL1;
	ctrl_reg = inb(ctrl_addr);
	disabled = (ctrl_reg & 0x80) ? 1 : 0;
	if (!disabled) {
		ctrl_reg |= 0x80;	/* disable the nice */
		outb(ctrl_addr, ctrl_reg);
	}

	/* select the register bank 00 */
	ctrl_addr = smp->io_base + smp->sm_nice_addr + CONTROL2;
	ctrl_reg = inb(ctrl_addr);
	save_reg = ctrl_reg;
	ctrl_reg &= 0xf3;
	outb(ctrl_addr, ctrl_reg);

	nice_offset = smp->io_base + smp->sm_nice_addr + NODEID0;
	for (i = 0; i < ETHERADDRL; i++)
		outb(nice_offset + i, macinfo->gldm_macaddr[i]);

	/* restore whatever register bank was selected */
	outb(ctrl_addr, save_reg);

	/* enable the NICE */
	ctrl_addr = smp->io_base + smp->sm_nice_addr + CONTROL1;
	if (!disabled) {
		ctrl_reg = inb(ctrl_addr);
		ctrl_reg &= 0x7f;
		outb(ctrl_addr, ctrl_reg);
	}
	return (0);
}

/*
 * smce_dlsdmult() -- set (enable) or disable a multicast address
 *
 * Program the hardware to enable/disable the multicast address in "mcast".
 * Enable if "op" is non-zero, disable if zero.
 */

static int
smce_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	struct smparam *smp = smcep->smp;	  /* lower MAC structure */
	struct smceboard *smcebp = smcep->smcebp; /* per board structure */

#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE) {
		int		i;

		cmn_err(CE_CONT, "smce_dlsdmult(0x%x, %s), channel %d",
				macinfo, op ? "ON" : "OFF", smcep->channel);
		for (i = 0; i < 6; i++)
			cmn_err(CE_CONT, ":%x", mcast->ether_addr_octet[i]);
	}
#endif


	bcopy((caddr_t)mcast->ether_addr_octet,
	    (caddr_t)smp->multi_address, ETHERADDRL);
	mutex_enter(&(smcebp->smcelock));
	if (op)
		smce_lm_add_multi_address(smp);
	else
		smce_lm_delete_multi_address(smp);
	mutex_exit(&(smcebp->smcelock));

	return (0);
}

/*
 * smce_prom() -- set or reset promiscuous mode on the board
 *
 * Program the hardware to enable/disable promiscuous mode. Enable if "on" is
 * non-zero, disable if zero.
 */

static int
smce_prom(gld_mac_info_t *macinfo, int on)
{
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	struct smparam *smp = smcep->smp;
	unsigned char   reg;

#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE)
		cmn_err(CE_CONT, "smce_prom(0x%x, %s)", macinfo,
			on ? "ON" : "OFF");
#endif

	/*
	 * This is only a per channel operation so do not need the
	 * board mutex
	 */
	reg = inb(smp->io_base + smp->sm_nice_addr + RECV_MODE);
	if (on) {
		reg |= 0x03;
		smp->receive_mask &= PROMISCUOUS_MODE;
	} else {
		reg &= ~0x03;
		if (smp->sm_multicnt == 0)
			reg |= 0x01;
		else
			reg |= 0x02;
		smp->receive_mask &= ~PROMISCUOUS_MODE;
	}
	outb(smp->io_base + smp->sm_nice_addr + RECV_MODE, reg);
	return (0);
}

/*
 * smce_gstat() -- update statistics
 *
 * GLD calls this routine just before it reads the driver's statistics
 * structure.  If your board maintains statistics, this is the time to read
 * them in and update the values in the structure.  If the driver maintains
 * statistics continuously, this routine need do nothing.
 */

static int
smce_gstat(gld_mac_info_t *macinfo)
{
#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE)
		cmn_err(CE_CONT, "smce_gstat(0x%x)", macinfo);
#else
#ifdef lint
	macinfo = macinfo;
#endif
#endif

	/* **** update statistics from board if necessary **** */
	return (0);
}

/*
 * smce_send() -- send a packet
 *
 * Called when a packet is ready to be transmitted. A pointer to an M_PROTO or
 * M_PCPROTO message that contains the packet is passed to this routine as a
 * parameter. The complete LLC header is contained in the message block's
 * control information block, and the remainder of the packet is contained
 * within the M_DATA message blocks linked to the first message block.
 *
 * This routine may NOT free the packet.
 */

static int
smce_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	char	*buf = NULL;
	register unsigned index, length;
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	struct smparam *smp = smcep->smp;	  /* lower MAC structure */
	struct smceboard *smcebp = smcep->smcebp; /* per board structure */
	struct DataBuffStructure mb;
	mblk_t	*tmp;
	int	i = 0;
	int	rc;

#ifdef SMCEDEBUG
	if (smcedebug & SMCESEND)
		cmn_err(CE_CONT, "smce_send(0x%x, 0x%x)", macinfo, mp);
#endif

	/*
	 * Load the packet onto the board by chaining through the M_DATA
	 * blocks attached to the M_PROTO header.  The list of data messages
	 * ends when the pointer to the current message block is NULL.
	 *
	 * Note that if the mblock is going to have to stay around, it must be
	 * dupmsg() since the caller is going to freemsg() the message.
	 */
	/* need to package the mb below which the lower MAC expects */
	mb.fragment_count = 0;
	tmp = mp;
	while (tmp != NULL) {
		if (mb.fragment_count++ == (MAX_FRAGS - 2)) {
			index = length = 0;
			buf = kmem_alloc(SMMAXPKT, KM_NOSLEEP);
			if (buf == NULL)
				return (1);
			while (tmp) {
			    length += tmp->b_wptr - tmp->b_rptr;
			    if (length > SMMAXPKT) {
				cmn_err(CE_WARN,
					"smce: dropping huge packet\n");
				kmem_free(buf, SMMAXPKT);
				return (0);
			    }
			    bcopy((caddr_t)tmp->b_rptr, &buf[index],
					    tmp->b_wptr - tmp->b_rptr);
			    index = length;
			    tmp = tmp->b_cont;
			}
			mb.fragment_list[i].fragment_ptr = buf;
			mb.fragment_list[i].fragment_length = (ushort)length;
			break;
		}
		mb.fragment_list[i].fragment_ptr = (caddr_t)tmp->b_rptr;
		mb.fragment_list[i].fragment_length =
			(ushort) (tmp->b_wptr - tmp->b_rptr);
		tmp = tmp->b_cont;
		i++;
	}

#ifdef SMCEDEBUG
	if (smcedebug & 0x01)
		cmn_err(CE_CONT, "<");
#endif

	mutex_enter(&(smcebp->smcelock));
	rc = smce_lm_send(&mb, smp);	/* Send it out */
	mutex_exit(&(smcebp->smcelock));

	if (buf) kmem_free(buf, SMMAXPKT);

#ifdef SMCEDEBUG
	if (smcedebug & 0x01)
		cmn_err(CE_CONT, ">");
#endif

	return (rc);
}

/*
 * smceintr() -- interrupt from board to inform us that a receive or transmit
 * has completed.
 */

static u_int
smceintr(gld_mac_info_t *macinfo)
{
	struct smceinstance *smcep =	/* Our private device info */
		(struct smceinstance *)macinfo->gldm_private;
	struct smparam *smp = smcep->smp;	  /* lower MAC structure */
	struct smceboard *smcebp = smcep->smcebp; /* per board structure */
	int rc = 0;

#ifdef SMCEDEBUG
	if (smcedebug & SMCEINT)
		cmn_err(CE_CONT, "smceintr(0x%x), smp=0x%x", macinfo, smp);
#endif

	/*
	 * All the checking outlined below will be handled, and between the
	 * handshaking with LM, gld_recv() will be called by
	 * smce_um_receive_packet().
	 */
	if (smp->sm_enabled) {
#ifdef SMCEDEBUG
		if (smcedebug & 0x01)
			cmn_err(CE_CONT, "{NICE %d:", smcep->channel);
#endif

		mutex_enter(&(smcebp->smcelock));
		rc = smce_lm_service_events(smp);
		mutex_exit(&(smcebp->smcelock));

#ifdef SMCEDEBUG
		if (smcedebug & 0x01)
			cmn_err(CE_CONT, "}");
#endif
	}

	if (rc)
		return (DDI_INTR_CLAIMED);
	else
		return (DDI_INTR_UNCLAIMED);
}

#if 0
/* Following yanked from SMC's sm.c, their original UMAC for ISC UNIX */
unsigned char
smhash(unsigned char addr[])
{
	register int    i, j;
	union crc_reg   crc;
	unsigned char   fb, ch;

	crc.value = 0xFFFFFFFF;
	for (i = 0; i < LLC_ADDR_LEN; i++) {
		ch = addr[i];
		for (j = 0; j < 8; j++) {
			fb = crc.bits.a31 ^ ((ch >> j) & 0x01);
			crc.bits.a25 ^= fb;
			crc.bits.a22 ^= fb;
			crc.bits.a21 ^= fb;
			crc.bits.a15 ^= fb;
			crc.bits.a11 ^= fb;
			crc.bits.a10 ^= fb;
			crc.bits.a9 ^= fb;
			crc.bits.a7 ^= fb;
			crc.bits.a6 ^= fb;
			crc.bits.a4 ^= fb;
			crc.bits.a3 ^= fb;
			crc.bits.a1 ^= fb;
			crc.bits.a0 ^= fb;
			crc.value = (crc.value << 1) | fb;
		}
	}
	return ((unsigned char) (crc.value & 0x3F));
}
#endif


/*
 * Following are routines to interface with the LMAC
 */


#if 0
/* Notify the completion status of the send request associated with smp */
int
smce_um_send_complete(int result, struct smparam *smp)
{

#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE)
		cmn_err(CE_CONT, "smce_um_send_complete(0x%x):%d", smp, result);
#else
#ifdef lint
	result = result;
	smp = smp;
#endif
#endif
	/* do not need to do anything */
	return (0);
}
#endif /* 0 */


/* Called from lm_service_events() to receive a packet */
int
smce_um_receive_packet(unsigned short length, struct smparam *smp)
{
	mblk_t *mp;
	struct DataBuffStructure dbuf;

#ifdef SMCEDEBUG
	if (smcedebug & SMCETRACE)
		cmn_err(CE_CONT, "smce_um_receive_packet(0x%x):%d",
			smp, length);
	if (smcedebug)
		cmn_err(CE_CONT, "smce_um_receive_packet(smp=0x%x):length=%d\n",
			smp, length);
#endif

	if ((length > SMMAXPKT + LLC_EHDR_SIZE) || (length < LLC_EHDR_SIZE)) {
		flush_packet(smp->io_base + smp->sm_nice_addr + BMPORT_LSB,
				length);
		return (-1);
	}

	/* get buffer to put packet in & move it there */
	if ((mp = allocb(length+2, BPRI_MED)) != NULL) {
		/*
		 * 2 bytes extra are allocated to
		 * allow for aligning
		 * the data portion of the packet on
		 * a 4-byte boundary.
		 */
		mp->b_wptr += 2;
		mp->b_rptr = mp->b_wptr;
		dbuf.fragment_count = 1;
		dbuf.fragment_list[0].fragment_ptr = (caddr_t)mp->b_wptr;
		dbuf.fragment_list[0].fragment_length = length;
		smce_lm_receive_copy(length, (long)BMPORT_LSB, &dbuf, smp);
		mp->b_wptr += length;

		/* prepare to call gld_recv() */
		gld_recv((gld_mac_info_t *)smp->private, mp);
		return (0);
	} else {
		flush_packet(smp->io_base + smp->sm_nice_addr + BMPORT_LSB,
				length);
		return (-1);
	}
}

static void
flush_packet(unsigned long addr, int length)
{
	int i, len;

#ifdef SMCEDEBUG
	if (smcedebug)
		cmn_err(CE_CONT, "%d ", length);
#endif

	if (length & 0x01) {
		len = length >> 1;
		len++;
	} else {
		len = length >> 1;
	}
	for (i = 0; i < len; i++)
		(void) inw(addr);
}

static int
smce_getioaddr(dev_info_t *devinfo)
{
	int i;
	int nregs, reglen;
	int baseaddr;
	struct {
		int bustype;
		int base;
		int size;
	} *reglist;

	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo,
			    DDI_PROP_DONTPASS, "reg",
			    (caddr_t)&reglist, &reglen) !=
				DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN, "smce: reg property not found in "
				    "devices property list");
		return (-1);
	}

	nregs = reglen / sizeof (*reglist);
	for (i = 0; i < nregs; i++)
		if (reglist[i].bustype == 1) {
			baseaddr = reglist[i].base;
			break;
		}

	kmem_free(reglist, reglen);
	if (i >= nregs) {
		cmn_err(CE_WARN, "smce: invalid reg property, "
				    "base I/O address not specified");
		return (-2);
	}
	return (baseaddr);
}
