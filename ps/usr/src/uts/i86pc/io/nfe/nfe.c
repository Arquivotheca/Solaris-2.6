/*
 * Copyright (c) 1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * nfe -- Netflex 2 Ethernet
 * Depends on the Generic LAN Driver utility functions in /kernel/misc/gld
 */

/*
 * This driver supports the netflex-2 board, with 2 ethernet interfaces
 */

/*
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
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
 * if Sun has been advised of the possibility of such damages
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#pragma ident "@(#)nfe.c	1.26	96/06/06 SMI"

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
#include <sys/eisarom.h>
#include <sys/ksynch.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/strsun.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/gld.h>
#include <sys/nfe.h>
#include <sys/nvm.h>
#include <sys/nfe_download.h>

/*
 *  Declarations and Module Linkage
 */

static char ident[] = "Netflex 2 Ethernet";

#ifdef NFEDEBUG
/* used for debugging */
int	nfedebug = 0;
#endif

/* Required system entry points */
static	nfeidentify(dev_info_t *);
static	nfedevinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static	nfeprobe(dev_info_t *);
static	nfeattach(dev_info_t *, ddi_attach_cmd_t);
static	nfedetach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
int	nfe_reset(gld_mac_info_t *);
int	nfe_start_board(gld_mac_info_t *);
int	nfe_stop_board(gld_mac_info_t *);
int	nfe_saddr(gld_mac_info_t *);
int	nfe_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
int	nfe_prom(gld_mac_info_t *, int);
int	nfe_gstat(gld_mac_info_t *);
int	nfe_send(gld_mac_info_t *, mblk_t *);
u_int	nfeintr(gld_mac_info_t *);

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/* static routines */

static int nfe_initialize(gld_mac_info_t *macinfo);
static int download_adapter_code(struct nfeinstance *nfep);
static int nfe_process_recv(gld_mac_info_t *macinfo);
static int wait_for_scb(gld_mac_info_t *macinfo);
static int wait_for_cmd_complete(gld_mac_info_t *macinfo, ushort command);
static int close_command(gld_mac_info_t *macinfo);
static int nfe_start_board_int(gld_mac_info_t *macinfo);

/* Standard Streams initialization */

static struct module_info minfo = {
	NFEIDNUM, "nfe", 0, INFPSZ, NFEHIWAT, NFELOWAT
    };

static struct qinit rinit = {	/* read queues */
	NULL, gld_rsrv, gld_open, gld_close, NULL, &minfo, NULL
    };

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, NULL, NULL, NULL, &minfo, NULL
    };

struct streamtab nfeinfo = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static	struct cb_ops cb_nfeops = {
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
	&nfeinfo,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
    };

struct dev_ops nfeops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	nfedevinfo,		/* devo_getinfo */
	nfeidentify,		/* devo_identify */
	nfeprobe,		/* devo_probe */
	nfeattach,		/* devo_attach */
	nfedetach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_nfeops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
    };

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&nfeops			/* driver specific ops */
    };

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
    };
static kmutex_t nfe_probe_lock;
ulong	pgmask = 0, pgoffset = 0, pgsize = 0;	/* set based on ptob() */

int
_init(void)
{
	mutex_init(&nfe_probe_lock, "MP probe protection", MUTEX_DRIVER, NULL);
	pgsize = ptob(1);
	pgoffset = pgsize - 1;
	pgmask = ~pgoffset;
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
 *  DDI Entry Points
 */

/* identify(9E) -- See if we know about this device */

nfeidentify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), "nfe") == 0)
	    return (DDI_IDENTIFIED);
	else
	    return (DDI_NOT_IDENTIFIED);
}

/* getinfo(9E) -- Get device driver information */

nfedevinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int error;

	/* This code is not DDI compliant: the correct semantics */
	/* for CLONE devices is not well-defined yet.		 */
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

/* probe(9E) -- Determine if a device is present */

nfeprobe(dev_info_t *devinfo)
{
	int	found_board = 0;
	int	base_io_address;
	static int lastslot = -1;
	int	regbuf[3];
	short	type;
	int	irq;
	int	i;
	int	len;
	int	slot;
	struct intrprop {
		int spl;
		int irq;
	} *intrprop;

#ifdef NFEDEBUG
	if (nfedebug & NFEDDI)
	    cmn_err(CE_CONT, "nfeprobe(0x%x)", devinfo);
#endif

	mutex_enter(&nfe_probe_lock);

	/*
	 * Loop through all EISA slots, and check the product ID for
	 * the Compaq Netflex2.	 Each time we come through this
	 * routine, we increment the static 'lastslot' so we can start
	 * searching from the one at which we left off last time.
	 */
	for (slot = lastslot + 1; slot < EISA_MAXSLOT; slot++) {

		if (!nfe_get_irq(slot, &irq, &type))
		    continue;

		base_io_address = slot * 0x1000;

		/* Check the ID bytes, make sure the card is really there */

		if (type == NFE_HW) {
			/* this is the Compaq dual ethernet product ID */
			if ((((inb(base_io_address + NFE_ID0))
					== DUALPORT_ID0) &&
				((inb(base_io_address + NFE_ID1))
					== DUALPORT_ID1) &&
				((inb(base_io_address + NFE_ID2))
					== DUALPORT_ID2) &&
				((inb(base_io_address + NFE_ID3))
					== (DUALPORT_ID3|DUALPORT_REV))))
			    goto found_board;

			/* This is the Compaq Token Ring/Ethernet Product ID */
			if ((((inb(base_io_address + NFE_ID0))
					== ENETTR_ID0) &&
				((inb(base_io_address + NFE_ID1))
					== ENETTR_ID1) &&
				((inb(base_io_address + NFE_ID2))
					== ENETTR_ID2) &&
				((inb(base_io_address + NFE_ID3))
					== (ENETTR_ID3|ENETTR_REV))))
			    goto found_board;


			cmn_err(CE_WARN,  "nfe: CMOS has card in slot %d,"
				" card not there.\n", slot);
			continue;
		}

	}

	lastslot = slot;
	mutex_exit(&nfe_probe_lock);
	return (DDI_PROBE_FAILURE);

	found_board:
#ifdef NFEDEBUG
	if (nfedebug & NFEDDI) {
		cmn_err(CE_CONT,
			"nfeprobe: found slot=%d irq=%d\n", slot, irq);
	}
#endif

	lastslot = slot;
	regbuf[0] = base_io_address;
	(void) ddi_prop_create(DDI_DEV_T_NONE, devinfo,
				DDI_PROP_CANSLEEP, "ioaddr", (caddr_t)regbuf,
				sizeof (int));

	if ((i = ddi_getlongprop(DDI_DEV_T_ANY, devinfo,
				DDI_PROP_DONTPASS, "intr", (caddr_t)&intrprop,
				&len)) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN, "nfe: Could not locate intr property.\n");
		mutex_exit(&nfe_probe_lock);
		return (DDI_PROBE_FAILURE);
	}

	len /= sizeof (struct intrprop);

	for (i = 0; i < len; i++)
	    if (irq == intrprop[i].irq)
		break;
	kmem_free(intrprop, len * sizeof (struct intrprop));

	if (i >= len) {
		cmn_err(CE_WARN,
			"nfe: irq in conf file does not match CMOS\n");
		mutex_exit(&nfe_probe_lock);
		return (DDI_PROBE_FAILURE);
	}

	ddi_set_driver_private(devinfo, (caddr_t)i);
	mutex_exit(&nfe_probe_lock);
	return (DDI_PROBE_SUCCESS);
}

int
nfe_check_boardid(NVM_SLOTINFO *nvm)
{
	if ((nvm->boardid[0] == DUALPORT_ID0) &&
	    (nvm->boardid[1] == DUALPORT_ID1) &&
	    (nvm->boardid[2] == DUALPORT_ID2) &&
	    ((nvm->boardid[3] & 0xf0) == DUALPORT_ID3))
	    return (1);

	if ((nvm->boardid[0] == ENETTR_ID0) &&
	    (nvm->boardid[1] == ENETTR_ID1) &&
	    (nvm->boardid[2] == ENETTR_ID2) &&
	    ((nvm->boardid[3] & 0xf0) == ENETTR_ID3))
	    return (1);

	return (0);
}

nfe_get_irq(int slot, int *irq, short *type)
{
	extern	int	eisa_nvmlength;
	struct {
		short			slotnum;
		NVM_SLOTINFO	slot;
		NVM_FUNCINFO	func;
	} buff;
	NVM_SLOTINFO	*nvm;
	int				 rc;
	int				 function_num;

	*irq = 0;

	for (function_num = 0; ; function_num++) {

		/* get slot info and just the next function record */
		rc = eisa_nvm(&buff, (EISA_SLOT | EISA_CFUNCTION), slot,
				function_num);
		if (function_num == 0) {
			if (rc == 0) {
				/* it's an unconfigured slot */
				return (FALSE);
			}

			if (slot != buff.slotnum) {
				/* shouldn't happen */
				return (FALSE);
			}

			nvm = (NVM_SLOTINFO *)&buff.slot;

			if (nfe_check_boardid(nvm)) {
				*type = NFE_HW;
			}
			else
			    return (FALSE);
		}
		if (rc == 0) {
			/* end of functions, no irq defined */
			break;
		}

		if (*type != NFE_HW) {
			if (!buff.func.fib.type)
			    continue;
			cmn_err(CE_WARN, "nfe: no irq found.\n");
			return (FALSE);
		}
		goto got_it;
	}
	got_it:
	*irq = buff.func.un.r.irq[0].line;
	return (TRUE);
}
/*
 *  attach(9E) -- Attach a device to the system
 *
 *  Called once for each board successfully probed.
 */

nfeattach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t *macinfo, *macinfo2; /* GLD structures */
	struct nfeinstance *nfep, *nfep2;   /* Our private device info */
	int base_io_address;
	int i;
	int rc, rc2;

#ifdef NFEDEBUG
	debug_enter("\n\nnfe attach\n\n");
	if (nfedebug & NFEDDI)
	    cmn_err(CE_CONT, "nfeattach(0x%x)", devinfo);
#endif

	if (cmd != DDI_ATTACH)
	    return (DDI_FAILURE);

	/*
	 * Allocate gld_mac_info_t and nfeinstance structures.	We need to
	 * allocate a pair of each for the dual-channel card, and also
	 * a per board structure that is being shared by both channels.
	 */

	macinfo = (gld_mac_info_t *)kmem_zalloc(
		(2 * sizeof (gld_mac_info_t)+
		2 * sizeof (struct nfeinstance)), KM_NOSLEEP);

	if (macinfo == NULL)
	    return (DDI_FAILURE);

	macinfo2 = macinfo + 1;
	nfep = (struct nfeinstance *)(macinfo+2);
	nfep2 = nfep + 1;

	/*  Initialize our private fields in macinfo and nfeinstance */
	macinfo->gldm_private = (caddr_t)nfep;
	macinfo2->gldm_private = (caddr_t)nfep2;
	macinfo->gldm_port = macinfo2->gldm_port = ddi_getprop(DDI_DEV_T_ANY,
				devinfo, DDI_PROP_DONTPASS, "ioaddr", 0);
	macinfo->gldm_state = macinfo2->gldm_state = NFE_IDLE;
	macinfo->gldm_flags = macinfo2->gldm_flags = 0;
	macinfo->gldm_irq_index = macinfo2->gldm_irq_index =
	    (long)ddi_get_driver_private(devinfo);
	nfep->dip = nfep2->dip = devinfo;
	nfep->flags = nfep2->flags = 0;

	/* Are we a dual board, or ENET/TR ? */

	nfep->nfe_type = inb(macinfo->gldm_port + NFE_ID2);

	/*
	 *  Initialize pointers to device specific functions which will be
	 *  used by the generic layer.
	 */

	macinfo->gldm_reset = macinfo2->gldm_reset  = nfe_reset;
	macinfo->gldm_start = macinfo2->gldm_start  = nfe_start_board;
	macinfo->gldm_stop  = macinfo2->gldm_stop   = nfe_stop_board;
	macinfo->gldm_saddr = macinfo2->gldm_saddr  = nfe_saddr;
	macinfo->gldm_sdmulti = macinfo2->gldm_sdmulti = nfe_dlsdmult;
	macinfo->gldm_prom  = macinfo2->gldm_prom   = nfe_prom;
	macinfo->gldm_gstat = macinfo2->gldm_gstat  = nfe_gstat;
	macinfo->gldm_send  = macinfo2->gldm_send  = nfe_send;
	macinfo->gldm_intr  = macinfo2->gldm_intr  = nfeintr;
	macinfo->gldm_ioctl = macinfo2->gldm_ioctl = NULL;

	/*
	 *  Initialize board characteristics needed by the generic layer.
	 */

	macinfo->gldm_ident = macinfo2->gldm_ident = ident;
	macinfo->gldm_type = macinfo2->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = macinfo2->gldm_minpkt = 0;
	macinfo->gldm_maxpkt = macinfo2->gldm_maxpkt = NFEMAXPKT;
	macinfo->gldm_addrlen = macinfo2->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = macinfo2->gldm_saplen = NFE_SAPLEN;

	nfep->nfe_framesize = nfep2->nfe_framesize = NFE_RX_BUFSIZE;

	/*
	 * Get the 'nframes' property...indicates the number of receive buffers
	 * the driver will *attempt* to allocate.
	 */

	nfep->nfe_nframes = nfep2->nfe_nframes =
	    ddi_getprop(DDI_DEV_T_NONE, devinfo,
			DDI_PROP_DONTPASS, "nframes", 0);
	if (nfep->nfe_nframes == 0) {
		cmn_err(CE_WARN, "nfe: Could not find nframes property.\n");
		goto afail;
	}

	if (nfep->nfe_nframes > NFE_NFRAMES) {
		cmn_err(CE_WARN, "nfe: Can't allocate more than %d frames\n",
			NFE_NFRAMES);
		nfep->nfe_nframes = nfep2->nfe_nframes = NFE_NFRAMES;
	}

	nfep->nfe_xbufsize = nfep2->nfe_xbufsize = NFE_XMIT_BUFSIZE;

	/*
	 * Get the 'xmits' property...indicates the number of xmit buffers
	 * the driver will attempt to allocate.
	 */
	nfep->nfe_xmits = nfep2->nfe_xmits =
	    ddi_getprop(DDI_DEV_T_NONE, devinfo,
			DDI_PROP_DONTPASS, "xmits", 0);

	if (nfep->nfe_xmits == 0) {
		cmn_err(CE_WARN, "nfe: Could not find xmits property.\n");
		cmn_err(CE_WARN, "nfe: Will not attach.\n");
		goto afail;
	}

	if (nfep->nfe_xmits > NFE_NFRAMES) {
		cmn_err(CE_WARN, "nfe: Can't allocate more than %d frames\n",
			NFE_NFRAMES);
		nfep->nfe_xmits = nfep2->nfe_xmits = NFE_NFRAMES;
	}

	base_io_address = macinfo->gldm_port;

	nfep->media = ((inw(base_io_address + NFE_IRQ) & CFG_PORT1_IF)
			? NFE_RJ45 : NFE_DB15);
	nfep2->media = ((inw(base_io_address + NFE_IRQ) & CFG_PORT2_IF)
			? NFE_RJ45 : NFE_DB15);


	/*
	 * This is so that we don't have to worry about which of the
	 * two ethernet ports we are dealing with.  The gld routines
	 * can just get the io addresses from the nfep structure.
	 */

	nfep->base_io_address = nfep2->base_io_address = base_io_address;
	nfep->sifdat = base_io_address + SIFDAT;
	nfep2->sifdat = nfep->sifdat + DUAL_OFFSET;
	nfep->sifdai = base_io_address + SIFDAI;
	nfep2->sifdai = nfep->sifdai + DUAL_OFFSET;
	nfep->sifadr = base_io_address + SIFADR;
	nfep2->sifadr = nfep->sifadr + DUAL_OFFSET;
	nfep->sifcmd = base_io_address + SIFCMD;
	nfep2->sifcmd = nfep->sifcmd + DUAL_OFFSET;
	nfep->sifacl = base_io_address + SIFACL;
	nfep2->sifacl = nfep->sifacl + DUAL_OFFSET;
	nfep->sifadr = base_io_address + SIFADR;
	nfep2->sifadr = nfep->sifadr + DUAL_OFFSET;
	nfep->sifadx = base_io_address + SIFADX;
	nfep2->sifadx = nfep->sifadx + DUAL_OFFSET;
	nfep->dmalen = base_io_address + DMALEN;
	nfep2->dmalen = nfep->dmalen + DUAL_OFFSET;
	nfep->enable_receive = 0;
	nfep2->enable_receive = 0;

	/*
	 * Perform a hardware reset on the board.  We must do this before
	 * attempting to read the ethernet address.  These operations are
	 * totally magic, from the specs.
	 */

	nfe_initialize(macinfo);

	/* set the connector/media type */
	macinfo->gldm_media = ((nfep->media == NFE_RJ45)
				? GLDM_TP : GLDM_AUI);
	macinfo2->gldm_media = ((nfep2->media == NFE_RJ45)
				? GLDM_TP : GLDM_AUI);

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo2->gldm_broadcast, ETHERADDRL);


	/* Allocate memory and give controllers default options */

	if (nfe_alloc_memory(nfep) == DDI_FAILURE)
		goto afail;

	if (nfe_init_board(macinfo, TRUE) == DDI_FAILURE)
		goto afail;

	if (nfep->nfe_type == NFE_TYPE_DUAL) {
		if (nfe_alloc_memory(nfep2) == DDI_FAILURE)
			goto afail;

		if (nfe_init_board(macinfo2, TRUE) == DDI_FAILURE)
			goto afail;
	}

	/*
	 *  Register ourselves with the GLD interface
	 *
	 *  gld_register will:
	 *  link us with the GLD system;
	 *  set our ddi_set_driver_private(9F) data to the macinfo pointer;
	 *  save the devinfo pointer in macinfo->gldm_devinfo;
	 *  map the registers, putting the kvaddr into macinfo->gldm_memp;
	 *  add the interrupt, putting the cookie in gldm_cookie;
	 *  init the gldm_intrlock mutex which will block that interrupt;
	 *  create the minor node.
	 */

	gld_register(devinfo, "nfe", macinfo);

	if (nfep->nfe_type == NFE_TYPE_DUAL)
	    gld_register(devinfo, "nfe", macinfo2);

	ddi_set_driver_private(devinfo, (caddr_t)macinfo);


	outw(nfep->sifacl,
		inw(nfep->base_io_address + SIFACL) & ACTL_MASK |
		ACTL_FPA | nfep->media | ACTL_SINTEN);

	nfep->flags |= NFE_INTR_ENABLED;

	if (nfep->nfe_type == NFE_TYPE_DUAL) {
		outw(nfep2->sifacl,
			inw(nfep->base_io_address + SIFACL) & ACTL_MASK |
			ACTL_FPA | nfep2->media | ACTL_SINTEN);

		nfep2->flags |= NFE_INTR_ENABLED;
	}


	/* Initialize transmit and receive lists, start receiver */

	nfe_start_board_int(macinfo);
	if (nfep->nfe_type == NFE_TYPE_DUAL)
	    nfe_start_board_int(macinfo2);

	macinfo->gldm_state = NFE_WAITRCV;
	macinfo2->gldm_state = NFE_WAITRCV;

	return (DDI_SUCCESS);

afail:

	kmem_free(macinfo,
	    2 * sizeof (gld_mac_info_t) + 2 *
	    sizeof (struct nfeinstance));
	return (DDI_FAILURE);
}

/*  detach(9E) -- Detach a device from the system */

nfedetach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t *macinfo, *macinfo2;	/* GLD structure */
	struct nfeinstance *nfep, *nfep2;	/* Our private device info */
	int r1, r2;

#ifdef NFEDEBUG
	if (nfedebug & NFEDDI)
	    cmn_err(CE_CONT, "nfedetach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/* Get the driver private (gld_mac_info_t) structure */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);
	macinfo2 = macinfo + 1;
	nfep = (struct nfeinstance *)(macinfo->gldm_private);
	nfep2 = (struct nfeinstance *)(macinfo2->gldm_private);

	/* stop the board if it is running */
	(void) nfe_stop_board(macinfo);

	if (wait_for_scb(macinfo) < 0)
		return (DDI_FAILURE);

	nfep->scb->cmd = CMD_CLOSE;
	nfep->ssb->cmd = 0;

	outw(nfep->sifcmd, SIF_CMDINT);
	if (wait_for_cmd_complete(macinfo, CMD_CLOSE) < 0)
		return (DDI_FAILURE);

	ddi_iopb_free((caddr_t)(nfep->params_v));
	kmem_free(nfep->nfe_rbufs, nfep->rtotal);
	kmem_free(nfep->tbufs, nfep->ttotal);

	if (nfep->nfe_type == NFE_TYPE_DUAL) {

		if (wait_for_scb(macinfo2) < 0)
			return (DDI_FAILURE);

		nfep2->scb->cmd = CMD_CLOSE;
		nfep2->ssb->cmd = 0;

		outw(nfep2->sifcmd, SIF_CMDINT);
		if (wait_for_cmd_complete(macinfo2, CMD_CLOSE) < 0)
			return (DDI_FAILURE);

		ddi_iopb_free((caddr_t)(nfep2->params_v));
		kmem_free(nfep2->nfe_rbufs, nfep2->rtotal);
		kmem_free(nfep2->tbufs, nfep->ttotal);
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

	r1 = gld_unregister(macinfo);
	r2 = DDI_SUCCESS;

	if (nfep->nfe_type == NFE_TYPE_DUAL)
		r2 = gld_unregister(macinfo2);

	if ((r1 == DDI_SUCCESS) && (r2 == DDI_SUCCESS)) {
	    kmem_free((caddr_t)macinfo, (2 * sizeof (gld_mac_info_t) +
	    2 * sizeof (struct nfeinstance)));
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}

/*
 *  GLD Entry Points
 */

/*
 *  nfe_reset() -- reset the board to initial state; restore the machine
 *  address afterwards.
 */

int
    nfe_reset(gld_mac_info_t *macinfo)
{
#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_CONT, "nfe_reset(0x%x)", macinfo);
#endif

	(void) nfe_stop_board(macinfo);
}


/*
 *  nfe_init_board() -- initialize the specified network board.
 *  This function issues the *  TMS "init" and "open" commands.
 *  These commands set up all of the default options.
 */

int
nfe_init_board(gld_mac_info_t *macinfo, int read_bia)
{
	struct nfeinstance *nfep =	    /* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;

	struct nfe_initcmd *init;
	struct nfe_opencmd *open;
	int i, k;
	short *p;
	short j;
	ushort length;
	paddr_t phys_addr;

	/*
	 * The SCB is the area from where the adapter will DMA,
	 * to receive commands.
	 */

	nfep->scb = &(nfep->params_v->scb);

	/*
	 * The SSB is the area to where the adapter will DMA,
	 * to issue command completion and interrupt statis
	 */

	nfep->ssb = &(nfep->params_v->ssb);

	/* Keep pointers to the start of the xmit and receive lists */

	nfep->tlist = &(nfep->params_v->tlist[0]);
	nfep->rlist = &(nfep->params_v->rlist[0]);

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_NOTE, "nfe: tlist[0] at %x, rlist[0] at %x tbuf %x\n",
		    nfep->tlist, nfep->rlist, nfep->tbufsv[0]);
#endif

	/* Multi-purpose parameter area */

	nfep->cparm = (caddr_t)&nfep->params_v->cparm;

	/* We don't use this */

	nfep->prod = (caddr_t)&nfep->params_v->prod;

	init = (struct nfe_initcmd *)nfep->cparm;
	bzero((caddr_t)init, sizeof (struct nfe_initcmd));

	init->options = INIT_OPTS;
	init->tvec = macinfo->gldm_irq_index;
	init->rvec = macinfo->gldm_irq_index;
	init->cvec = macinfo->gldm_irq_index;
	init->avec = macinfo->gldm_irq_index;
	init->rbsize = 0;		  /* 0 means use burst for all data */
	init->tbsize = 0;		  /* DITTO */
	init->dma_threshold = INIT_DMAT;

	/* Tell the adapter the physical address of SCB and SSB */

	phys_addr = (paddr_t)&nfep->params_p->scb;
	REVERSE_2_WORDS((ushort *)&phys_addr, &(init->scb));
	phys_addr = (paddr_t)&nfep->params_p->ssb;
	REVERSE_2_WORDS((ushort *)&phys_addr, &(init->ssb));

	/*
	 * We must wait for the self-test to complete before issuing the
	 * init command.
	 */

	i = 10000;
	while (--i) {
		if (inw(nfep->sifcmd) & SIF_DIAGOK)
		    break;
		drv_usecwait(1000);
	}

	if (!i) {
		short x;

		x = inw(nfep->sifcmd);

		cmn_err(CE_WARN, "nfe: diag not ok or not complete %x is %x\n",
			nfep->sifcmd, x);

		return (DDI_FAILURE);
	}

	/*
	 * The INIT command is issued by DIO, one word at a time.
	 */

	outw(nfep->sifadx, INIT_ADDR_HI);
	outw(nfep->sifadr, INIT_ADDR_LO);

	p = (short *)init;

	for (i = 0; i < (sizeof (struct nfe_initcmd) / 2); i++) {
		outw(nfep->sifdai, *p++);
	}
	outw(nfep->sifcmd, SIF_CMDINT);

	i = 100000;
	while (--i) {
		j = inw(nfep->sifcmd);
		if ((k = (j & SIF_INITOK)) == 0)
		    break;
		drv_usecwait(1000);
	}
	if (k != 0) {
		cmn_err(CE_WARN,
		    "initialization failed, status = %x\n scb at %x ssb at %x",
		    j, nfep->scb, nfep->ssb);
		return (DDI_FAILURE);
	}

	if (read_bia) {

		/* Get onboard location of ethernet hardware addresses */

		outw(nfep->sifadx, INIT_ADDR_HI);
		outw(nfep->sifadr, INIT_ADDR_LO+HARD_ADDR);
		nfep->bia = inw(nfep->sifdat);

		/* Use the READ_ADAPTER command to get the ethernet address */

		length = ETHERADDRL;
		REVERSE_2_BYTES(&length, nfep->cparm);
		REVERSE_2_BYTES(&nfep->bia, &nfep->cparm[2]);

		phys_addr = (paddr_t)&nfep->params_p->cparm;
		REVERSE_4_BYTES(&phys_addr, (caddr_t)&(nfep->scb->parm0));

		nfep->scb->cmd = CMD_READ_ADAP;
		nfep->ssb->cmd = 0;		/* clear cmd in SSB */
		outw(nfep->sifcmd, SIF_CMDINT);

		if (wait_for_cmd_complete(macinfo, CMD_READ_ADAP) < 0)
			return (DDI_FAILURE);

		/* copy MAC address to macinfo structure */
		bcopy((caddr_t)nfep->cparm, (caddr_t)macinfo->gldm_vendor,
			ETHERADDRL);

#ifdef NFEDEBUG
		if (nfedebug & NFETRACE) {
			cmn_err(CE_NOTE, "nfe: ethernet address is ");
			for (i = 0; i < ETHERADDRL; i++) {
			    cmn_err(CE_CONT, "%x:", macinfo->gldm_vendor[i]);
			}
			cmn_err(CE_CONT, "\n");
		}
#endif

		bcopy((caddr_t)macinfo->gldm_vendor,
			(caddr_t)macinfo->gldm_macaddr, ETHERADDRL);
	}

	/* Perform adapter OPEN command. */

	if (wait_for_scb(macinfo) < 0)
		return (DDI_FAILURE);

	phys_addr = (paddr_t)&nfep->params_p->cparm;
	REVERSE_4_BYTES(&phys_addr,
			(u_char *)&(nfep->scb->parm0));

	open = (struct nfe_opencmd *)nfep->cparm;

	open->options = OPEN_OPTS;
	for (i = 0; i < 6; i++)
	    open->node[i] = macinfo->gldm_macaddr[i];

	/* NOTE THAT THESE 3 NOT IN SPEC(!) */

	open->reserved1 = 0xffffffff;
	open->ram_start = RAM_START;
	open->ram_end = RAM_END;

	/*
	 * The next 5 parameters define the size of various structures
	 * as they should be in the adapters *internal* memory, not ours.
	 */

	open->rlist_size = 0;	  /* zero means default 26 */
	open->tlist_size = 0;	  /* DITTO */
	open->buffer_size = (DFL_BUF & 0xff) << 8 | (DFL_BUF & 0xff00) >> 8;
	open->trans_min = XMIT_BUF_MIN;
	open->trans_max = XMIT_BUF_MAX;

	/* Unused */

	nfep->prod[0] = '\0';
	phys_addr = (paddr_t)&nfep->params_p->prod;
	REVERSE_4_BYTES(&phys_addr,
			(u_char *)&(open->prod));

	/* We store the open params because we might change them later */
	nfep->open_parms = *open;

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_NOTE, "scb %x ssb %x open %x\n",
		    nfep->scb, nfep->ssb, open);
#endif

	nfep->scb->cmd = CMD_OPEN;
	nfep->ssb->cmd = 0;
	outw(nfep->sifcmd, SIF_CMDINT);

	if (wait_for_cmd_complete(macinfo, CMD_OPEN) < 0)
		return (DDI_FAILURE);

	return (DDI_SUCCESS);
}

/*
 * Allocate all of the memory.  Note that we must do some nasty things
 * to make sure everything is contiguous, so that the adapter can read
 * it through DMA.
 */

int
nfe_alloc_memory(struct nfeinstance *nfep)
{
	int s;
	caddr_t r;
	caddr_t *bb;
	unsigned long limit;
	int num_allocated;
	ddi_dma_lim_t limits;

	/*
	 * We use iopb alloc to allocate the control structures.  This
	 * guarantees contiguity
	 */

	limits.dlim_addr_lo = 0;
	limits.dlim_addr_hi = 0xffffffff;
	limits.dlim_cntr_max = 0;
	limits.dlim_burstsizes = 1;
	limits.dlim_minxfer = DMA_UNIT_8;
	limits.dlim_dmaspeed = 0;
	limits.dlim_version = DMALIM_VER0;
	limits.dlim_adreg_max = 0xffff;
	limits.dlim_ctreg_max = 0xffff;
	limits.dlim_granular = 512;
	limits.dlim_sgllen = 1;
	limits.dlim_reqsize = 0xffffffff;

	if (ddi_iopb_alloc(nfep->dip, (ddi_dma_lim_t *)&limits,
	    sizeof (struct nfe_shared_mem), (caddr_t *)&(nfep->params_v))) {
		cmn_err(CE_WARN,
			"nfe_alloc: unable to allocate param memory.\n");
		return (DDI_FAILURE);
	}

	bzero((caddr_t)nfep->params_v, sizeof (struct nfe_shared_mem));

	nfep->params_p = (struct nfe_shared_mem *)NFE_KVTOP(nfep->params_v);

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_CONT, "nfe: param V: %x P: %x\n", nfep->params_v,
		    nfep->params_p);
#endif

	/*
	 * Now allocate all of the receive buffers.  We will not be
	 * able to get as many as requested, as we will lose some in
	 * order to maintain contiguity within buffers.
	 */

	nfep->rtotal =
	    NFE_ALIGN((nfep->nfe_nframes+1) * NFEMAXFRAME, pgoffset);

	nfep->nfe_rbufs = kmem_alloc(nfep->rtotal, KM_SLEEP);
	if (nfep->nfe_rbufs == NULL) {
		cmn_err(CE_WARN, "nfe: Could not allocate receive buffers\n");
		return (DDI_FAILURE);
	}

	r = nfep->nfe_rbufs;
	limit = (ulong)r + (ulong)nfep->rtotal;

	if (!(NFE_SAMEPAGE(r, r+(NFEMAXFRAME-1)))) {
		r = (caddr_t)(NFE_ALIGN(r, pgoffset));
	}

	bzero(r, NFEMAXFRAME);
	nfep->rbufsv[0] = r;
	nfep->rbufsp[0] = NFE_KVTOP(r);

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_NOTE,
		    "nfe: rbufsp[0] is %x rbufsv[0] %x\n",
		    nfep->rbufsp[0], nfep->rbufsv[0]);
#endif

	r = r + NFEMAXFRAME;
	for (num_allocated = 1; (ulong)((r + NFEMAXFRAME)) < limit; ) {
		if (!(NFE_SAMEPAGE(r, r+(NFEMAXFRAME-1)))) {
			r = (caddr_t)(NFE_ALIGN(r, pgoffset));
			if ((u_long)(r + NFEMAXFRAME) >= limit)
			    break;
		}

		nfep->rbufsv[num_allocated] = r;
		nfep->rbufsp[num_allocated] = NFE_KVTOP(r);
		num_allocated++;
		bzero(r, NFEMAXFRAME);
		r = r + NFEMAXFRAME;
	}

#ifdef NFEDEBUG
	cmn_err(CE_WARN, "nfe: asked for %d recv buffers, got %d\n",
		nfep->nfe_nframes, num_allocated);
#endif

	nfep->nfe_nframes = num_allocated;

	/* Now do the transmit buffers.	 Same restrictions apply */

	nfep->ttotal = NFE_ALIGN((nfep->nfe_xmits+1) * NFEMAXFRAME, pgoffset);
	nfep->tbufs = kmem_zalloc(nfep->ttotal, KM_SLEEP);
	if (nfep->tbufs == NULL) {
		cmn_err(CE_WARN, "nfe: Could not allocate xmit buffers\n");
		kmem_free(nfep->nfe_rbufs, nfep->rtotal);
		return (DDI_FAILURE);
	}

	r = nfep->tbufs;
	limit = (ulong)r + nfep->ttotal;

	if (!(NFE_SAMEPAGE(r, r+(NFEMAXFRAME-1)))) {
		r = (caddr_t)(NFE_ALIGN((long)r, (long)pgoffset));
	}

	nfep->tbufsv[0] = r;
	nfep->tbufsp[0] = NFE_KVTOP(r);

	r += NFEMAXFRAME;
	for (num_allocated = 1; (long)(r + NFEMAXFRAME) < (long)limit; ) {

		if (!(NFE_SAMEPAGE(r, r+(NFEMAXFRAME-1)))) {
			r = (caddr_t)(NFE_ALIGN((long)r, (long)pgoffset));
			if ((long)(r+NFEMAXFRAME) >= (long)limit)
			    break;
		}

		nfep->tbufsv[num_allocated] = r;
		nfep->tbufsp[num_allocated] = NFE_KVTOP(r);
		num_allocated++;
		r += NFEMAXFRAME;
	}

#ifdef NFEDEBUG
	cmn_err(CE_WARN, "nfe: asked for %d xmit buffers, got %d\n",
		nfep->nfe_xmits, num_allocated);
#endif

	nfep->nfe_xmits = num_allocated;

	return (DDI_SUCCESS);
}

/*
 * This is where we form the xmit and receive lists and start the xmit
 * and receive processes.
 */

static int
nfe_start_board_int(gld_mac_info_t *macinfo)
{
	struct nfeinstance *nfep =	    /* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;
	struct tlist *tl;
	struct rlist *rl;
	int i;
	paddr_t phys_addr;

	/* Set up the transmit list */

	for (i = 0; i < nfep->nfe_xmits; i++) {
		tl = &(nfep->params_v->tlist[i]);
		phys_addr = (paddr_t)&(nfep->params_p->tlist[i+1]);
		REVERSE_4_BYTES(&phys_addr, &(tl->next)); /* Forward pointer */

		/* This is start and end of frame */
		tl->cstat =  XMT_SOF|XMT_EOF;
		tl->frsize = 0;
		tl->count = 0;

		/* point to buffer */
		phys_addr = (paddr_t)nfep->tbufsp[i];
		REVERSE_4_BYTES(&phys_addr, &(tl->address));
	}

	/* Last buffer points to first */

	phys_addr = (paddr_t)&(nfep->params_p->tlist[0]);
	REVERSE_4_BYTES(&phys_addr, &(tl->next));

	nfep->xmit_current = 0;
	nfep->xmit_first = -1;
	nfep->xmit_last = 0;

	/* Now do the receive buffers */

	for (i = 0; i < nfep->nfe_nframes; i++) {
		short size;

		rl = &(nfep->params_v->rlist[i]);
		phys_addr = (paddr_t)&(nfep->params_p->rlist[i+1]);
		REVERSE_4_BYTES(&phys_addr, &(rl->next)); /* Forward pointer */

		/*
		 * Set VALID bit so receiver will use immediately, and INT bit
		 * so that it will interrupt us when buffer is used.
		 */

		rl->cstat = RCV_VALID | RCV_FRM_INT;

		size = NFEMAXFRAME;
		REVERSE_2_BYTES(&size, &(rl->frame_size));
		REVERSE_2_BYTES(&size, &(rl->count));

		phys_addr = (paddr_t)nfep->rbufsp[i];
		REVERSE_4_BYTES(&phys_addr, &(rl->address));
	}

	rl->cstat = RCV_FRM_INT;		/* Last one VALID reset */

	phys_addr = (paddr_t)&(nfep->params_p->rlist[0]);
	REVERSE_4_BYTES(&phys_addr, &(rl->next));  /* and points to first */

	nfep->receive_current = 0;
	nfep->receive_end = i-1;

	if (wait_for_scb(macinfo) < 0)
		return (DDI_FAILURE);

	nfep->scb->cmd = CMD_RCV;
	phys_addr = (paddr_t)&(nfep->params_p->rlist[0]);
	REVERSE_4_BYTES(&phys_addr, (u_char *)&(nfep->scb->parm0));

	outw(nfep->sifcmd, SIF_CMDINT);		/* Start receive process */

	/* No status is returned from CMD_RCV */

	if (wait_for_scb(macinfo) < 0)
		return (DDI_FAILURE);

	nfep->scb->cmd = CMD_XMT;
	phys_addr = (paddr_t)&nfep->params_p->tlist[0];
	REVERSE_4_BYTES(&phys_addr, (u_char *)&(nfep->scb->parm0));

	outw(nfep->sifcmd, SIF_CMDINT);	    /* Start xmit process */

	/* No status is returned from CMD_XMT */

	return (DDI_SUCCESS);
}

/*
 *  nfe_start_board() -- start the board receiving and allow transmits.
 */

nfe_start_board(gld_mac_info_t *macinfo)
{
	struct nfeinstance *nfep =		/* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_CONT, "nfe_start_board(0x%x)", macinfo);
#endif

	nfep->enable_receive = 1;

}

/*
 *  nfe_stop_board() -- stop board receiving
 */

nfe_stop_board(gld_mac_info_t *macinfo)
{
	struct nfeinstance *nfep =		/* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_CONT, "nfe_stop_board(0x%x)", macinfo);
#endif

	nfep->enable_receive = 0;

}

/*
 *  nfe_saddr() -- set the physical network address on the board
 */

int
nfe_saddr(gld_mac_info_t *macinfo)
{
	struct nfeinstance *nfep =		/* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_CONT, "nfe_saddr(0x%x)", macinfo);
#endif

	/* Issue CLOSE command */

	if (close_command(macinfo) < 0)
		return (-1);

	/* Soft reset the controller */
	outw(nfep->sifcmd, SIF_RESET);

	nfep->flags &= ~NFE_INTR_ENABLED;

	/* Issue INIT and OPEN commands, using new macaddr */

	if (nfe_init_board(macinfo, FALSE) == DDI_FAILURE)
		return (-1);

	/* Enable interrupts */

	outw(nfep->sifacl,
		inw(nfep->base_io_address + SIFACL) & ACTL_MASK |
		ACTL_FPA | nfep->media | ACTL_SINTEN);

	nfep->flags |= NFE_INTR_ENABLED;

	/* Initialize transmit and receive lists, start receiver */

	if (nfe_start_board_int(macinfo) == DDI_FAILURE)
		return (-1);

	return (0);
}

/*
 *  nfe_dlsdmult() -- set (enable) or disable a multicast address
 *
 *  Program the hardware to enable/disable the multicast address
 *  in "mcast".	 Enable if "op" is non-zero, disable if zero.
 */

int
    nfe_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	struct nfeinstance *nfep =		/* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;
	struct nfe_mccmd *mc;
	paddr_t phys_addr;

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_CONT, "nfe_dlsdmult(0x%x, %s)", macinfo,
		    op ? "ON" : "OFF");
#endif

	if (wait_for_scb(macinfo) < 0)
		return (-1);

	mc = (struct nfe_mccmd *)nfep->cparm;
	nfep->nmcast += (op ? 1 : -1);
	mc->options = (op ? CET_ADD_MCA : CET_DEL_MCA);
	if (nfep->nmcast > 128)
	    mc->options = CET_SET_ALL_MCA;
	bcopy((caddr_t)&(mcast->ether_addr_octet[0]),
		(caddr_t)&mc->address, ETHERADDRL);

	phys_addr = (paddr_t)&nfep->params_p->cparm;
	REVERSE_4_BYTES(&phys_addr, (u_char *)&(nfep->scb->parm0));
	nfep->scb->cmd = CMD_SET_MCA;
	nfep->ssb->cmd = 0;
	outw(nfep->sifcmd, SIF_CMDINT);

	if (wait_for_cmd_complete(macinfo, CMD_SET_MCA) < 0)
		return (-1);

	return (0);
}


/*
 * nfe_prom() -- set or reset promiscuous mode on the board
 *
 *  Program the hardware to enable/disable promiscuous mode.
 *  Enable if "on" is non-zero, disable if zero.
 */

int
nfe_prom(gld_mac_info_t *macinfo, int on)
{
	struct nfeinstance *nfep =		/* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;
	ushort new_opt;
	paddr_t phys_addr;

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_CONT, "nfe_prom(0x%x, %s)", macinfo,
		    on ? "ON" : "OFF");
#endif

	new_opt = (on ? (nfep->open_parms.options | COPY_ALL_FRAMES) :
	    (nfep->open_parms.options & ~(COPY_ALL_FRAMES)));

	nfep->open_parms.options = new_opt;

	if (wait_for_scb(macinfo) < 0)
		return (-1);

	nfep->scb->cmd = CMD_MOD_OPEN;
	nfep->ssb->cmd = 0;
	nfep->scb->parm0 = new_opt;
	nfep->scb->parm1 = 0;

	outw(nfep->sifcmd, SIF_CMDINT);

	if (wait_for_cmd_complete(macinfo, CMD_MOD_OPEN) < 0)
		return (-1);

	return (0);
}

/*
 * nfe_gstat() -- update statistics
 *
 *  GLD calls this routine just before it reads the driver's statistics
 *  structure.	If your board maintains statistics, this is the time to
 *  read them in and update the values in the structure.  If the driver
 *  maintains statistics continuously, this routine need do nothing.
 */

int
nfe_gstat(gld_mac_info_t *macinfo)
{
	struct nfeinstance *nfep =		/* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;

	struct nfe_stats *statp;
	paddr_t phys_addr;

#ifdef NFEDEBUG
	if (nfedebug & NFETRACE)
	    cmn_err(CE_CONT, "nfe_gstat(0x%x)", macinfo);
#endif

	if (wait_for_scb(macinfo) < 0)
		return (-1);

	statp = (struct nfe_stats *)nfep->cparm;

	phys_addr = (paddr_t)&nfep->params_p->cparm;
	REVERSE_4_BYTES((caddr_t)&phys_addr, (caddr_t)&nfep->scb->parm0);

	nfep->scb->cmd = CMD_READ_LOG;
	nfep->ssb->cmd = 0;
	outw(nfep->sifcmd, SIF_CMDINT);

	if (wait_for_cmd_complete(macinfo, CMD_READ_LOG) < 0)
		return (-1);

	REVERSE_4_BYTES(&(statp->deferred_tx),
			&(macinfo->gldm_stats.glds_defer));
	REVERSE_4_BYTES(&(statp->xs_coll),
			&(macinfo->gldm_stats.glds_excoll));
	REVERSE_4_BYTES(&(statp->late_coll),
			&(macinfo->gldm_stats.glds_xmtlatecoll));
	REVERSE_4_BYTES(&(statp->carr_sense_err),
			&(macinfo->gldm_stats.glds_nocarrier));
	REVERSE_4_BYTES(&(statp->late_coll),
			&(macinfo->gldm_stats.glds_xmtlatecoll));

	return (0);
}

/*
 *  nfe_send() -- send a packet
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
nfe_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	register int len = 0;
	int reaper, last_reaped;
	struct tlist *tl;
	unsigned length;
	unsigned char *txbuf;
	struct nfeinstance *nfep =		/* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;

#ifdef NFEDEBUG
	if (nfedebug & NFESEND)
	    cmn_err(CE_CONT, "nfe_send(0x%x, 0x%x)", macinfo, mp);
#endif

	if (nfep->xmit_first != -1) {	 /* If -1, no buffers used yet */
		/*
		 * Loop through buffers in use, see if any are complete.
		 */

		reaper = nfep->xmit_first;
		last_reaped = -1;

		do {
			tl = &(nfep->tlist[reaper]);

			if (!(tl->cstat & XMT_CPLT))
			    break;

			tl->cstat = XMT_SOF|XMT_EOF;   /* It is now reaped. */
			tl->frsize = 0;
			tl->count = 0;

			last_reaped = reaper;
			reaper++;
			if (reaper == nfep->nfe_xmits)
			    reaper = 0;
		} while (last_reaped != nfep->xmit_last);

		if (last_reaped == nfep->xmit_last)  /* All reaped and avail */
		    nfep->xmit_first = -1;
		else
		    nfep->xmit_first = reaper; /* Some not reaped, get later */
	}

	if (nfep->xmit_current == nfep->xmit_first) { /* No bufs available */
		macinfo->gldm_stats.glds_defer++;
		return (1);
	}


	length = msgdsize(mp);
	if (length > ETHERMAX) {
		cmn_err(CE_WARN,
			"nfe: Transmit packet out of bounds, len %d.\n",
			msgdsize(mp));
		return (0);		/* Or else it will come back. */
	}

	length = 0;
	do {
		len = (int)(mp->b_wptr - mp->b_rptr);
		bcopy((caddr_t)mp->b_rptr,
			(caddr_t)(nfep->tbufsv[nfep->xmit_current])+length,
			len);
		length += (unsigned int)len;
		mp = mp->b_cont;
	} while (mp != NULL);

	/* Get to the current element of the xmit list */

	tl = &(nfep->tlist[nfep->xmit_current]);

	REVERSE_2_BYTES(&length, &(tl->count));
	REVERSE_2_BYTES(&length, &(tl->frsize));  /* Size of this packet */

	nfep->xmit_last = nfep->xmit_current;	  /* This is new last */

	if (nfep->xmit_first == -1)
	    nfep->xmit_first = nfep->xmit_current; /* First used packet */

	nfep->xmit_current++;
	if (nfep->xmit_current == nfep->nfe_xmits)
	    nfep->xmit_current = 0;		  /* Current moves up */

	if (nfep->xmit_current == nfep->xmit_first)
	    tl->cstat |= XMT_FRM_INT;

	tl->cstat |= XMT_VALID;			  /* Now set to valid */

	/*
	 * In case the transmit process is suspended waiting for work,
	 * interrupt it, telling it we have a new valid frame, right where
	 * it is waiting.
	 */

	outw(nfep->sifcmd, SIF_XMTVAL);

	return (0);		  /* successful transmit attempt */
}

/*
 *  nfeintr() -- interrupt from board.
 */

u_int
nfeintr(gld_mac_info_t *macinfo)
{
	struct nfeinstance *nfep =	    /* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;
	short status;

#ifdef NFEDEBUG
	if (nfedebug & NFEINT)
	    cmn_err(CE_CONT, "nfeintr(0x%x)", macinfo);
#endif

	if (!((status = inw(nfep->sifcmd)) & SIF_SYSINT))  /* Spurrious */
	    return (DDI_INTR_UNCLAIMED);

	macinfo->gldm_stats.glds_intr++;

	switch (status & INTERRUPT_TYPE) {

		case COMMAND_STATUS: {

		/*
		 * Command status interrupt:
		 *
		 * We poll for status on all commands but TRANSMIT and
		 * RECEIVE, so only work to do here is clear interrupt
		 * and check for REJECT of the TRANSMIT or RECEIVE
		 * command.
		 */

		    if ((nfep->ssb->cmd == COMMAND_REJECT)) {

			cmn_err(CE_WARN,
				"nfe: command %x rejected, status = %x",
					nfep->ssb->parm1, nfep->ssb->parm0);
		    }

		    /* Tell adapter it is clear to interrupt again */

		    outw(nfep->sifcmd, SIF_INT_ENABLE);
		    outw(nfep->sifcmd, SIF_SSBCLR);

		    break;
		}

		/* Frame received */

		case RECEIVE_STATUS: {
		    nfe_process_recv(macinfo);
		    /* Tell adapter it is clear to interrupt again */

		    outw(nfep->sifcmd, SIF_INT_ENABLE);
		    outw(nfep->sifcmd, SIF_SSBCLR);

		    break;
		}

		/* Should never get here */

		case TRANSMIT_STATUS: {
		    /* Tell adapter it is clear to interrupt again */

		    outw(nfep->sifcmd, SIF_INT_ENABLE);
		    outw(nfep->sifcmd, SIF_SSBCLR);

		    break;
		}

		/* Shouldn't get here either */

		case RING_STATUS: {
		    /* Tell adapter it is clear to interrupt again */

		    outw(nfep->sifcmd, SIF_INT_ENABLE);
		    outw(nfep->sifcmd, SIF_SSBCLR);

		    cmn_err(CE_WARN, "nfe: ring status interrupt, ignored.\n");
		    break;
		}

		/* We don't use this interrupt, either */

		case SCB_CLEAR: {
		    /* Tell adapter it is clear to interrupt again */

		    outw(nfep->sifcmd, SIF_INT_ENABLE);
		    outw(nfep->sifcmd, SIF_SSBCLR);

		    cmn_err(CE_WARN, "nfe: scb_clear interrupt, ignored.\n");
		    break;
		}

		/*
		 * This is a FATAL interrupt.  Only issued when something goes
		 * horribly wrong
		 */

		case ADAPTER_CHECK: {
		    ushort x1, x2, x3, x4;
		    outw(nfep->sifadx, ADAPTER_CHECK_HI);
		    outw(nfep->sifadr, ADAPTER_CHECK_LO);
		    x1 = inw(nfep->sifdai);
		    x2 = inw(nfep->sifdai);
		    x3 = inw(nfep->sifdai);
		    x4 = inw(nfep->sifdai);

		    cmn_err(CE_WARN,
	    "nfe: fatal adapter check, status %x par0 %x par1 %x par2 %x.\n",
			    x1, x2, x3, x4);

		}

		default: {
		    cmn_err(CE_WARN,
			    "nfe: interrupt with unknown stat %x\n", status);
		}
	}

	return (DDI_INTR_CLAIMED);    /* Indicate it was our interrupt */
}

/* Process the received frames, and send them up */

static int
nfe_process_recv(gld_mac_info_t *macinfo)
{
	int save_end;
	mblk_t *mp;
	ushort len;
	paddr_t adap_end;
	struct rlist *rl, *re;
	struct nfeinstance *nfep =	    /* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;


	/* Keep track of the end of the list */

	REVERSE_4_BYTES(&(nfep->ssb->parm1), &adap_end);

	do {
		/* Start with the current buffer descriptor */

		rl = &(nfep->rlist[nfep->receive_current]);

		/* If it isn't complete, that's all */

		if (!(rl->cstat & RCV_CPLT)) {
		    break;
		}

		/* Get the length */

		REVERSE_2_BYTES(&(rl->frame_size), &len);

		/* Send it up if start_board has been called */

		if (nfep->enable_receive) {

			/* Can we allocate that much (probably) */

			mp = allocb(len, BPRI_MED);
			if (mp) {
				/* Copy the data to an mblk */

				bcopy(nfep->rbufsv[nfep->receive_current],
					(caddr_t)mp->b_wptr, len);
				mp->b_wptr += len;

				gld_recv(macinfo, mp);
			}
			else
			    macinfo->gldm_stats.glds_norcvbuf++;
		}

		/* This descriptor becomes the last one */

		rl->cstat &= ~(RCV_VALID);
		rl->cstat &= ~(RCV_CPLT);

		/* The old last one becomes valid and usable */

		re = &(nfep->rlist[nfep->receive_end]);
		re->cstat = RCV_VALID | RCV_FRM_INT;

		nfep->receive_end = nfep->receive_current;

		/* Bump the current */

		nfep->receive_current++;
		if (nfep->receive_current == nfep->nfe_nframes)
		    nfep->receive_current = 0;

	} while ((paddr_t)(NFE_KVTOP((caddr_t)rl)) != adap_end);

	/* Let the receive process know that there are new VALID descriptors */

	outw(nfep->sifcmd, SIF_RCVVAL);

}

static int
download_adapter_code(struct nfeinstance *nfep)
{
	unsigned short acr_pre_open;

	/*
	 * Pointer to on-board code header which describes the various
	 * segments to be downloaded.
	 */

	OBC_HDR *obc_hdr = (OBC_HDR *)&(nfe_download_code[0]);

	/* Pointer to the first on-board code segment descriptor. */

	OBC_CHAP *obc_seg = (OBC_CHAP *)&obc_hdr->chap_hdr;

	/* Pointer to the first on-board code segment. */

	register u_char *obc_data = (u_char *)obc_hdr + obc_hdr->length;

	/*
	 * Downloads the on-board code to the TMS380.  It consists of
	 * segments of code/data, each of which is downloaded to a
	 * particular "address" within a "chapter".
	 */

	while (*(ushort *)obc_seg != OBC_HDR_END) {
		ushort bytes = obc_seg->bytes;

		/* Sets up "chapter" and "address" for a segment. */

		outw(nfep->sifadx, obc_seg->chapter);
		outw(nfep->sifadr, obc_seg->address);

		/* Transfers a segment to the TMS380 memory. */

		while (bytes) {
			TMS380_WORD word;
			register short i, j;

			/* Reverses code byte-order (8086 -> TMS380). */

			for (i = 0, j = sizeof (TMS380_WORD) - 1;
				j >= 0; i++, j--)
				((u_char *)&word)[i] = obc_data[j];

			/* Transfers a "word" of TMS380 code. */

			outw(nfep->sifdai, word);

			/* Points to next "word" of TMS380 code. */

			bytes -= sizeof (TMS380_WORD);
			obc_data += sizeof (TMS380_WORD);
		}
		obc_seg++;	/* Proceeds to next segment. */
	}
	acr_pre_open = inw(nfep->sifacl);
	acr_pre_open &= ~(ACTL_CPHALT);
	outw(nfep->sifacl, acr_pre_open);
}

static int
nfe_initialize(gld_mac_info_t *macinfo)
{
	gld_mac_info_t *macinfo2 = macinfo + 1;
	struct nfeinstance *nfep =	    /* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;
	struct nfeinstance *nfep2 =
	    (struct nfeinstance *)macinfo2->gldm_private;
	ushort i;

	outb(nfep->base_io_address + PROT_CFG1, RESET_ASSERT);
	drv_usecwait(5000);
	outb(nfep->base_io_address + PROT_CFG1, RESET_DEASSERT);

	i = inw(nfep->sifacl) & (ACTL_PSDMAEN);
	outw(nfep->sifacl, ACTL_ARESET);
	drv_usecwait(15);

	outw(nfep->sifacl,
		inw(nfep->base_io_address + SIFACL) & ACTL_MASK |i|
		ACTL_FPA | nfep->media);

	if (nfep->nfe_type == NFE_TYPE_DUAL) {
		i = inw(nfep2->sifacl) & (ACTL_PSDMAEN);
		outw(nfep2->sifacl, ACTL_ARESET);
		drv_usecwait(15);
		outw(nfep2->sifacl,
			inw(nfep->base_io_address + SIFACL) & ACTL_MASK | i |
			ACTL_FPA | nfep2->media);
	}

	download_adapter_code(nfep);

	if (nfep->nfe_type == NFE_TYPE_DUAL)
	    download_adapter_code(nfep2);
}

/*
 * wait_for_scb() -- waits up to 500 ms for TMS380 to read SCB
 */

static int
wait_for_scb(gld_mac_info_t *macinfo)
{
	struct nfeinstance *nfep =	    /* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;
	int iters = 5000;

	/* when SCB cmd = 0, indicates SCB is available for new cmd */
	while ((nfep->scb->cmd != 0) && --iters)
		drv_usecwait(100);

	if (!iters) {
		cmn_err(CE_WARN,
			"nfe: wait for SCB timed out, scb_cmd = %x\n",
			nfep->scb->cmd);
		return (-1);
	}

	return (0);
}

/*
 * wait_for_cmd_complete() -- waits up to 500 ms for TMS380 to
 *   interrupt with completion status from the command we
 *   just started.  We check the SSB to make sure status is OK.
 *   Note that while polling, we may pick up status from
 *   a transmit or receive completion.  If so, we call the
 *   interrupt handler to deal with that event.
 */

static int
wait_for_cmd_complete(gld_mac_info_t *macinfo, ushort command)
{
	struct nfeinstance *nfep =	    /* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;
	int iters = 5000;
	int rc = 0;
	short status;

wait:
	/* poll status register for completion interrupt */
	while ((!((status = inw(nfep->sifcmd)) & SIF_SYSINT)) && --iters)
		drv_usecwait(100);

	if (!iters) {
		cmn_err(CE_WARN,
		    "nfe: command %x timed out", command);
		return (-1);
	}

	/*
	 * if its not our interrupt, call nfeintr to process
	 * the interrupt and go back to waiting
	 */

	if ((status & INTERRUPT_TYPE) != COMMAND_STATUS) {
		(void) nfeintr(macinfo);
		goto wait;
	}

	if (nfep->ssb->cmd == COMMAND_REJECT) {
		cmn_err(CE_WARN,
		    "nfe: command %x rejected, status = %x",
			command, nfep->ssb->parm0);

		nfep->ssb->cmd = 0; /* clear REJECT so not seen by intr */
		rc = -1;
		goto out;
	}

	/* check returned command code */
	if (nfep->ssb->cmd != command) {
		cmn_err(CE_WARN,
		    "nfe: command %x has invalid ssb_cmd = %x",
			command, nfep->ssb->cmd);

		rc = -1;
		goto out;
	}

	/* check status */
	if (nfep->ssb->parm0 != SSB_DIROK) {
		cmn_err(CE_WARN,
		    "nfe: command %x failed, status = %x",
			command, nfep->ssb->parm0);

		rc = -1;
		goto out;
	}

	/*
	 * if interrupts are not enabled, clear SSB here.
	 * Otherwise, let interrupt handler take care of it
	 * so that we don't see spurious interrupts
	 */

out:
	if (!(nfep->flags & NFE_INTR_ENABLED)) {
		outw(nfep->sifcmd, SIF_INT_ENABLE);
		outw(nfep->sifcmd, SIF_SSBCLR);
	}
		

	return (rc);
}

static int
close_command(gld_mac_info_t *macinfo)
{
	struct nfeinstance *nfep =	    /* Our private device info */
	    (struct nfeinstance *)macinfo->gldm_private;

	if (wait_for_scb(macinfo) < 0)
		return (-1);

	nfep->scb->cmd = CMD_CLOSE;
	nfep->scb->parm0 = 0;
	nfep->scb->parm1 = 0;
	nfep->ssb->cmd = 0;

	outw(nfep->sifcmd, SIF_CMDINT);
	if (wait_for_cmd_complete(macinfo, CMD_CLOSE) < 0)
		return (-1);

	return (0);
}
