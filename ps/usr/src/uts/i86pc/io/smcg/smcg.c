/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)smcg.c 1.1	95/07/18 SMI"

/*
 * smcg -- SMC Generic Upper MAC Solaris driver
 * Depends on the Generic LAN Driver utility functions in /kernel/misc/gld
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

#include SMC_INCLUDE
#include "smcg.h"

DEPENDS_ON_GLD;		/* this forces misc/gld to load -- DO NOT REMOVE */

/*
 * Declarations and Module Linkage
 */

static char ident[] = SMCG_IDENT;

#ifdef	DEBUG
static int	SMCG_debug = 0xffff;
#endif

/* Required system entry points */
static int	SMCG_identify(dev_info_t *);
static int	SMCG_devinfo(dev_info_t *, ddi_info_cmd_t, void *, void **);
static int	SMCG_probe(dev_info_t *);
static int	SMCG_attach(dev_info_t *, ddi_attach_cmd_t);
static int	SMCG_detach(dev_info_t *, ddi_detach_cmd_t);

/* Required driver entry points for GLD */
static int 	SMCG_saddr(gld_mac_info_t *);
static int	SMCG_reset(gld_mac_info_t *);
static int	SMCG_start_board(gld_mac_info_t *);
static int	SMCG_stop_board(gld_mac_info_t *);
static int	SMCG_dlsdmult(gld_mac_info_t *, struct ether_addr *, int);
static int	SMCG_prom(gld_mac_info_t *, int);
static int	SMCG_gstat(gld_mac_info_t *);
static int	SMCG_send(gld_mac_info_t *, mblk_t *);
static u_int	SMCG_intr(gld_mac_info_t *);

/* Internal functions */
static int	SMCG_get_bustype(dev_info_t *);
static int	SMCG_init_board(gld_mac_info_t *macinfo);

/* Standard Streams initialization */

static struct module_info minfo = {
	0, SMCG_NAME, 0, INFPSZ, SMCGHIWAT, SMCGLOWAT
};

static struct qinit rinit = {	/* read queues */
	0, gld_rsrv, gld_open, gld_close, 0, &minfo, 0
};

static struct qinit winit = {	/* write queues */
	gld_wput, gld_wsrv, 0, 0, 0, &minfo, 0
};

static struct streamtab smcg_info = {&rinit, &winit, NULL, NULL};

/* Standard Module linkage initialization for a Streams driver */

extern struct mod_ops mod_driverops;

static 	struct cb_ops cb_smcg_ops = {
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
	&smcg_info,		/* cb_stream */
	(int)(D_MP)		/* cb_flag */
};

static struct dev_ops smcg_ops = {
	DEVO_REV,		/* devo_rev */
	0,			/* devo_refcnt */
	SMCG_devinfo,		/* devo_getinfo */
	SMCG_identify,		/* devo_identify */
	SMCG_probe,		/* devo_probe */
	SMCG_attach,		/* devo_attach */
	SMCG_detach,		/* devo_detach */
	nodev,			/* devo_reset */
	&cb_smcg_ops,		/* devo_cb_ops */
	(struct bus_ops *)NULL	/* devo_bus_ops */
};

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module.  This one is a driver */
	ident,			/* short description */
	&smcg_ops		/* driver specific ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

static kmutex_t	SMCG_old_probe_lock;
static int	SMCG_old_probe_end = 0;

int
_init(void)
{
	int	status;

	mutex_init(&SMCG_old_probe_lock, SMCG_NAME " driver old_probe mutex",
	    MUTEX_DRIVER, NULL);

	status = mod_install(&modlinkage);
	if (status != 0) {
		mutex_destroy(&SMCG_old_probe_lock);
	}
	return (status);
}

int
_fini(void)
{
	int	status;

	status = mod_remove(&modlinkage);
	if (status != 0)
		return (status);

	mutex_destroy(&SMCG_old_probe_lock);
	return (0);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 * DDI Entry Points
 */

/* identify(9E) -- See if we know about this device */
static int
SMCG_identify(dev_info_t *devinfo)
{
	if (strcmp(ddi_get_name(devinfo), SMCG_NAME) == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

/* getinfo(9E) -- Get device driver information */
static int
SMCG_devinfo(dev_info_t *devinfo, ddi_info_cmd_t cmd, void *arg, void **result)
{
	register int	error;

	/*
	 * This code is not DDI compliant: the correct semantics
	 * for CLONE devices is not well-defined yet.
	 */
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
static int
SMCG_probe(dev_info_t *devinfo)
{
	Adapter_Struc Ad;
	int old_probe;
	int reglen, nregs;
	int i, rc;
	int bus_type;
	struct {
		int bustype;
		int base;
		int size;
	} *reglist;
	int ioaddr;

	old_probe = ddi_getprop(DDI_DEV_T_ANY, devinfo, 0,
	    "ignore-hardware-nodes", 0);

	if ((bus_type = SMCG_get_bustype(devinfo)) == -1)
		return (DDI_PROBE_FAILURE);

	/* We currently only support ISA and EISA */
	if (bus_type != SMCG_EISA_BUS && bus_type != SMCG_AT_BUS)
		return (DDI_PROBE_FAILURE);

	Ad.pc_bus = (unsigned char)bus_type;

	if (old_probe) {
		mutex_enter(&SMCG_old_probe_lock);
		if (SMCG_old_probe_end) {
			mutex_exit(&SMCG_old_probe_lock);
			return (DDI_PROBE_FAILURE);
		}
		while (LM_Nextcard(&Ad) == SUCCESS) {
			rc = LM_GetCnfg(&Ad);
			if (rc == ADAPTER_AND_CONFIG || rc == SUCCESS) {
				ioaddr = Ad.io_base;
				(void) ddi_prop_create(DDI_DEV_T_NONE, devinfo,
				    DDI_PROP_CANSLEEP, "ioaddr",
				    (caddr_t)&ioaddr, sizeof (int));
#ifdef	DEBUG
				if (SMCG_debug & SMCGDDI)
					cmn_err(CE_CONT, SMCG_NAME
					    "_probe (old): ioaddr 0x%x probed",
					    ioaddr);
#endif
				mutex_exit(&SMCG_old_probe_lock);
				return (DDI_PROBE_SUCCESS);
			}
#ifdef	DEBUG
			if (SMCG_debug & SMCGDDI)
				cmn_err(CE_CONT, SMCG_NAME "_probe (old):"
				    " ioaddr 0x%x failed probe", Ad.io_base);
#endif
		}
		SMCG_old_probe_end++;
		mutex_exit(&SMCG_old_probe_lock);
		return (DDI_PROBE_FAILURE);
	}

	if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
	    "reg", (caddr_t)&reglist, &reglen) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN, SMCG_NAME
		    "_probe: reg property not found in devices property list");
		return (DDI_PROBE_FAILURE);
	}
	nregs = reglen / sizeof (*reglist);
	for (i = 0; i < nregs; i++)
		if (reglist[i].bustype == 1) {
			Ad.io_base = reglist[i].base;
			break;
		}
	kmem_free(reglist, reglen);
	if (i >= nregs) {
		cmn_err(CE_WARN, SMCG_NAME
		    "_probe: reg property I/O base address not specified");
		return (DDI_PROBE_FAILURE);
	}

/*
	if (bus_type == SMCG_MCA_BUS)
		Ad.slot_num = ???;
*/

	if (bus_type == SMCG_EISA_BUS)
		Ad.slot_num = Ad.io_base >> 12;

	rc = LM_GetCnfg(&Ad);
	if (rc != ADAPTER_AND_CONFIG && rc != SUCCESS) {
#ifdef	DEBUG
		if (SMCG_debug & SMCGDDI)
			cmn_err(CE_CONT, SMCG_NAME "_probe (new):"
			    " ioaddr 0x%x failed probe", Ad.io_base);
#endif
		return (DDI_PROBE_FAILURE);
	}

#ifdef	DEBUG
	if (SMCG_debug & SMCGDDI)
		cmn_err(CE_CONT, SMCG_NAME
		    "_probe (new): ioaddr 0x%x probed", Ad.io_base);
#endif

	return (DDI_PROBE_SUCCESS);
}

/*
 * Return the bus type as encoded in the devinfo tree
 */
static int
SMCG_get_bustype(dev_info_t *devi)
{
	char	bus_type[16];
	int	len = sizeof (bus_type);

	if (ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
	    "device_type", (caddr_t)bus_type, &len) != DDI_PROP_SUCCESS &&
	    ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
	    "bus-type", (caddr_t)bus_type, &len) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN, SMCG_NAME " cannot find bus type");
		return (-1);
	}

	if (strcmp(bus_type, "eisa") == 0)
		return (SMCG_EISA_BUS);
	else if (strcmp(bus_type, "isa") == 0)
		return (SMCG_AT_BUS);
	else if (strcmp(bus_type, "mc") == 0)
		return (SMCG_MCA_BUS);

#ifdef	DEBUG
	cmn_err(CE_WARN, SMCG_NAME " bus type not supported: %s", bus_type);
#endif
	return (-1);
}

/*
 * attach(9E) -- Attach a device to the system
 *
 * Called once for each board successfully probed.
 */
static int
SMCG_attach(dev_info_t *devinfo, ddi_attach_cmd_t cmd)
{
	gld_mac_info_t	*macinfo;
	Adapter_Struc	*pAd;
	smcg_t		*smcg;
	struct intr_t {
		int	spl;
		int	irq;
	} *intrprop;
	int	irqlen;
	struct {
	    int bustype;
	    int base;
	    int size;
	} *reglist;
	int reglen, nregs;
	int	i, rc, bus_type, channel, ioaddr;
	int	old_config;

#ifdef	DEBUG
	if (SMCG_debug & SMCGDDI)
		cmn_err(CE_CONT, SMCG_NAME "_attach(0x%x)", devinfo);
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	old_config = ddi_getprop(DDI_DEV_T_ANY, devinfo, 0,
	    "ignore-hardware-nodes", 0);

	if (old_config)
		ioaddr = ddi_getprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
		    "ioaddr", 0);
	else {
		if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
		    "reg", (caddr_t)&reglist, &reglen) != DDI_PROP_SUCCESS) {
			cmn_err(CE_WARN, SMCG_NAME "_attach: reg property"
			    " not found in devices property list");
			return (DDI_FAILURE);
		}
		nregs = reglen / sizeof (*reglist);
		for (i = 0; i < nregs; i++)
			if (reglist[i].bustype == 1) {
				ioaddr = reglist[i].base;
				break;
			}
		kmem_free(reglist, reglen);
		if (i >= nregs) {
			cmn_err(CE_WARN, SMCG_NAME "_attach: reg property"
			    " I/O base address not specified");
			return (DDI_FAILURE);
		}
	}

	if ((bus_type = SMCG_get_bustype(devinfo)) == -1)
		return (DDI_FAILURE);

	/*
	 * Allocate gld_mac_info_t and Lower MAC Adapter_Struc structures
	 */
	if ((macinfo = (gld_mac_info_t *)kmem_zalloc(
	    sizeof (gld_mac_info_t) * SMNUMPORTS, KM_NOSLEEP)) == NULL)
		return (DDI_FAILURE);
	if ((pAd = (Adapter_Struc *)kmem_zalloc(
	    sizeof (Adapter_Struc) * SMNUMPORTS, KM_NOSLEEP)) == NULL) {
		kmem_free((caddr_t)macinfo,
		    sizeof (gld_mac_info_t) * SMNUMPORTS);
		return (DDI_FAILURE);
	}
	if ((smcg = (smcg_t *)kmem_zalloc(
	    sizeof (smcg_t) * SMNUMPORTS, KM_NOSLEEP)) == NULL) {
		kmem_free((caddr_t)macinfo,
		    sizeof (gld_mac_info_t) * SMNUMPORTS);
		kmem_free((caddr_t)pAd,
		    sizeof (Adapter_Struc) * SMNUMPORTS);
		return (DDI_FAILURE);
	}

	pAd->io_base = macinfo->gldm_port = ioaddr;
	pAd->pc_bus = (unsigned char)bus_type;
/*
	if (bus_type == SMCG_MCA_BUS)
		pAd->slot_num = ???;
*/
	if (bus_type == SMCG_EISA_BUS)
		pAd->slot_num = pAd->io_base >> 12;

	/*
	 * Query the LMAC for the device information
	 */
	rc = LM_GetCnfg(pAd);

	if (rc != ADAPTER_AND_CONFIG && rc != SUCCESS) {
		cmn_err(CE_WARN,
		    SMCG_NAME "_attach: LM_GetCnfg failed (0x%X)", rc);
		goto attach_fail_cleanup;
	}

	/*
	 * Initialize pointers to device specific functions which will be
	 * used by the generic layer.
	 */
	macinfo->gldm_reset   = SMCG_reset;
	macinfo->gldm_start   = SMCG_start_board;
	macinfo->gldm_stop    = SMCG_stop_board;
	macinfo->gldm_saddr   = SMCG_saddr;
	macinfo->gldm_sdmulti = SMCG_dlsdmult;
	macinfo->gldm_prom    = SMCG_prom;
	macinfo->gldm_gstat   = SMCG_gstat;
	macinfo->gldm_send    = SMCG_send;
	macinfo->gldm_intr    = SMCG_intr;
	macinfo->gldm_ioctl   = 0;

	/*
	 * Initialize board characteristics needed by the generic layer.
	 */
	macinfo->gldm_ident = ident;
	macinfo->gldm_type = DL_ETHER;
	macinfo->gldm_minpkt = 0;	/* assumes we pad ourselves */
	macinfo->gldm_maxpkt = SMCGMAXPKT;
	macinfo->gldm_addrlen = ETHERADDRL;
	macinfo->gldm_saplen = -2;
	macinfo->gldm_media = GLDM_UNKNOWN;

#ifdef	DEBUG
	if (SMCG_debug & SMCGALAN)
		cmn_err(CE_CONT, SMCG_NAME "_attach: media type value is 0x%x",
		    pAd->media_type);
#endif

	if (old_config) {
		if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
		    "interrupts", (caddr_t)&intrprop, &irqlen)
		    != DDI_PROP_SUCCESS) {
			cmn_err(CE_WARN, SMCG_NAME "_attach: interrupts"
			    " property not found in devices property list");
			goto attach_fail_cleanup;
		}
		for (i = 0; i < irqlen/sizeof (*intrprop); i++)
			if (intrprop[i].irq == pAd->irq_value)
				break;
		kmem_free(intrprop, irqlen);
		if (i >= (irqlen / sizeof (*intrprop))) {
			cmn_err(CE_WARN, SMCG_NAME "_attach: interrupts"
			    " property not matched by adapter (0x%x)",
			    pAd->irq_value);
			goto attach_fail_cleanup;
		}
		macinfo->gldm_irq_index = i;
	} else { /* new config */
		if (ddi_dev_nintrs(devinfo, &i) == DDI_FAILURE || i != 1) {
			cmn_err(CE_WARN, SMCG_NAME "_attach (new):"
			    " must have exactly 1 interrupt specification");
			goto attach_fail_cleanup;
		}
		if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
		    "interrupts", (caddr_t)&intrprop, &irqlen)
		    != DDI_PROP_SUCCESS) {
			cmn_err(CE_WARN, SMCG_NAME "_attach: interrupts"
			    " property not found in devices property list");
			goto attach_fail_cleanup;
		}
		/* compute irq in old or new format -- slightly tricky */
		i = ((int *)intrprop)[(irqlen/sizeof (int))-1];
		kmem_free(intrprop, irqlen);
		if (i != pAd->irq_value) {
			cmn_err(CE_WARN, SMCG_NAME "_attach: interrupts"
			    " property (0x%x) not matched by adapter (0x%x)",
			    i, pAd->irq_value);
			goto attach_fail_cleanup;
		}
		macinfo->gldm_irq_index = 0;
	}

#ifdef	DEBUG
	if (SMCG_debug & SMCGALAN)
		cmn_err(CE_CONT, SMCG_NAME "_attach: gldm_irq_index = %d",
		    macinfo->gldm_irq_index);
#endif

	/*
	 * Find out the correct gldm_reg_index also for shared memory
	 */
	macinfo->gldm_reg_index = -1;
	if (pAd->ram_base != 0) {
		if (ddi_getlongprop(DDI_DEV_T_ANY, devinfo, DDI_PROP_DONTPASS,
		    "reg", (caddr_t)&reglist, &reglen) != DDI_PROP_SUCCESS) {
			cmn_err(CE_WARN, SMCG_NAME "_attach: reg property"
			    " not found in devices property list");
			goto attach_fail_cleanup;
		}
		nregs = reglen / sizeof (*reglist);
		for (i = 0; i < nregs; i++)
			if ((reglist[i].bustype == 0) &&
			    (reglist[i].base == (int)pAd->ram_base) &&
			    (reglist[i].size  == (pAd->ram_usable * 1024))) {
				macinfo->gldm_reg_index = i;
				break;
			}
		kmem_free(reglist, reglen);
		if (i >= nregs) {
			cmn_err(CE_WARN, SMCG_NAME "_attach: ram base/size"
			    " mismatch: card 0x%x/0x%x",
			    pAd->ram_base, pAd->ram_usable * 1024);
			goto attach_fail_cleanup;
		}
	}
#ifdef	DEBUG
	if (SMCG_debug & SMCGALAN)
		cmn_err(CE_CONT, SMCG_NAME, "_attach: gldm_reg_index = %d", i);
#endif

	pAd->receive_mask = ACCEPT_BROADCAST;
	pAd->max_packet_size = SMMAXPKT;
	pAd->num_of_tx_buffs = SMTRANSMIT_BUFS;
	if (pAd->ram_size == 8)
		pAd->num_of_tx_buffs = 1;

	bcopy((caddr_t)gldbroadcastaddr,
		(caddr_t)macinfo->gldm_broadcast, ETHERADDRL);

	smcg->smcg_numchannels = SMNUMPORTS;
	smcg->smcg_first = smcg;

	/*
	 * For multiport boards, initialize the secondary structures
	 */
	for (channel = 1; channel < SMNUMPORTS; channel++) {
		*(macinfo + channel) = *macinfo;
		*(pAd + channel) = *pAd;
		*(smcg + channel) = *smcg;
	}

	if (smcg->smcg_numchannels > 1)
		mutex_init(&smcg->smcg_dual_port_lock, SMCG_NAME "board lock",
		    MUTEX_DRIVER, 0);

	/*
	 * Get the board's vendor-assigned hardware network address.
	 *
	 * We needed to break out the get board address from the LM code,
	 * because for the 8232, we need to have the kernel memory pointer
	 * from gld_register to card shared memory, in order to call
	 * the LM_Initialise_Adapter() to initialise
	 * the shared memory. However to call gld_register() you need to
	 * have pAd->gldm_vendor which is initialised from node_address.
	 */
	LM_Get_Addr(pAd);

	for (channel = 0; channel < SMNUMPORTS; channel++) {
		bcopy((caddr_t)(pAd + channel)->node_address,
		    (caddr_t)(macinfo + channel)->gldm_vendor, ETHERADDRL);

		bcopy((caddr_t)(macinfo + channel)->gldm_vendor,
			(caddr_t)(macinfo + channel)->gldm_macaddr, ETHERADDRL);

		/* Link macinfo, smcg, and LMAC Adapter Structs */
		(macinfo + channel)->gldm_private = (caddr_t)(smcg + channel);
		(pAd + channel)->sm_private = (void *)(smcg + channel);
		(smcg + channel)->smcg_pAd = (pAd + channel);
		(smcg + channel)->smcg_macinfo = (macinfo + channel);

	/*
	 * Register ourselves with the GLD interface
	 *
	 * gld_register will:
	 *	link us with the GLD system;
	 *	set our ddi_set_driver_private(9F) data to the macinfo pointer;
	 *	save the devinfo pointer in macinfo->gldm_devinfo;
	 *	map the registers, putting the kvaddr into macinfo->gldm_memp;
	 *	add the interrupt, putting the cookie in gldm_cookie;
	 *	init the gldm_intrlock mutex which will block that interrupt;
	 *	create the minor node.
	 */
		if (gld_register(devinfo, SMCG_NAME, (macinfo + channel))
		    == DDI_SUCCESS) {
			/*
			 * Store the virtual address returned after mapping the
			 * shared memory.
			 */
			(pAd + channel)->ram_access =
			    (ulong)(macinfo + channel)->gldm_memp;
		} else {
			for (i = 0; i < channel; i++)
				gld_unregister(macinfo + i);
			goto attach_fail_cleanup;
		}

	}


	/*
	 * The spec. says we have to set up the interrupt vector and
	 * enable the system interrupt for the adapter before we call
	 * LM_Initialize_Adapter().  However this causes a hang on a
	 * soft reset on some cards when interrupts are generated
	 * before the LMAC is ready to handle them correctly.  This
	 * issue must be resolved with SMC.  For now we don't deliver
	 * any interrupts to the LMAC until after LM_Initialize_Adapter.
	 */
	rc = LM_Initialize_Adapter(pAd);
	for (channel = 0; channel < SMNUMPORTS; channel++)
		(smcg + channel)->smcg_ready++;	/* ready for interrupts */

	if (rc != SUCCESS) {
		for (i = 0; i < SMNUMPORTS; i++)
			gld_unregister(macinfo + i);
		goto attach_fail_cleanup;
	}

	return (DDI_SUCCESS);

attach_fail_cleanup:
	if (smcg->smcg_numchannels > 1)
		mutex_destroy(&smcg->smcg_dual_port_lock);
	kmem_free((caddr_t)macinfo,
		sizeof (gld_mac_info_t) * SMNUMPORTS);
	kmem_free((caddr_t)pAd, sizeof (Adapter_Struc) * SMNUMPORTS);
	kmem_free((caddr_t)smcg, sizeof (smcg_t) * SMNUMPORTS);
	return (DDI_FAILURE);
}

/* detach(9E) -- Detach a device from the system */
static int
SMCG_detach(dev_info_t *devinfo, ddi_detach_cmd_t cmd)
{
	gld_mac_info_t	*macinfo;
	Adapter_Struc	*pAd;
	smcg_t		*smcg;
	int		i;

#ifdef	DEBUG
	if (SMCG_debug & SMCGDDI)
		cmn_err(CE_CONT, SMCG_NAME "_detach(0x%x)", devinfo);
#endif

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

	/*
	 * The driver_private points to the *last* macinfo we registered.
	 * We need the pointer to the *first* one.
	 */
	macinfo = (gld_mac_info_t *)ddi_get_driver_private(devinfo);
	macinfo = ((smcg_t *)macinfo->gldm_private)->smcg_first->smcg_macinfo;

	smcg = (smcg_t *)macinfo->gldm_private;		/* first smcg */
	pAd = smcg->smcg_pAd;				/* first Ad */

	/* stop the board if it is running */
	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_dual_port_lock);

#if SMNUMPORTS > 1
	/*
	 * This unfortunate hack is due to the UMAC/LMAC spec's confusion
	 * over what is an adapter and what is a port.
	 */
	pAd->sm_first_init_done = 0;
#endif
	(void) LM_Initialize_Adapter(pAd);

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_dual_port_lock);

	/*
	 * Unregister ourselves from the GLD interface
	 *
	 * gld_unregister will:
	 *	remove the minor node;
	 *	unmap the registers;
	 *	remove the interrupt;
	 *	destroy the gldm_intrlock mutex;
	 *	unlink us from the GLD system.
	 */
	for (i = 0; i < SMNUMPORTS; i++)
		(void) gld_unregister(macinfo + i);	/* Better not fail */

	if (smcg->smcg_numchannels > 1)
		mutex_destroy(&smcg->smcg_dual_port_lock);
	kmem_free((caddr_t)macinfo,
		sizeof (gld_mac_info_t) * SMNUMPORTS);
	kmem_free((caddr_t)pAd, sizeof (Adapter_Struc) * SMNUMPORTS);
	kmem_free((caddr_t)smcg, sizeof (smcg_t) * SMNUMPORTS);

	return (DDI_SUCCESS);
}

/*
 * GLD Entry Points
 */

/*
 * SMCG_reset() -- reset the board to initial state.
 */
static int
SMCG_reset(gld_mac_info_t *macinfo)
{
	int rc;

#ifdef	DEBUG
	if (SMCG_debug & SMCGTRACE)
		cmn_err(CE_CONT, SMCG_NAME "_reset(0x%x)", macinfo);
#endif

	rc = SMCG_init_board(macinfo);
	return (rc == SUCCESS ? 0 : -1);
}

/*
 * SMCG_init_board() -- initialize the specified network board.
 */
static int
SMCG_init_board(gld_mac_info_t *macinfo)
{
	smcg_t		*smcg = (smcg_t *)macinfo->gldm_private;
	Adapter_Struc	*pAd = smcg->smcg_pAd;
	int		rc;

#ifdef	DEBUG
	if (SMCG_debug & SMCGTRACE)
		cmn_err(CE_CONT, SMCG_NAME "_init_board(0x%x)", macinfo);
#endif

	pAd->receive_mask = ACCEPT_BROADCAST;

	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_first->smcg_dual_port_lock);

	rc = LM_Initialize_Adapter(pAd);

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_first->smcg_dual_port_lock);

	/*
	 * The spec says we should wait for UM_Status_Change, but all LMs
	 * we currently support change the status prior to returning from
	 * LM_Initialize_Adapter().
	 */

	if (rc != SUCCESS)
		cmn_err(CE_WARN,
		    SMCG_NAME " LM_Initialize_Adapter failed %d", rc);

	return (rc);
}

/*
 * SMCG_start_board() -- start the board receiving and allow transmits.
 */
static int
SMCG_start_board(gld_mac_info_t *macinfo)
{
	smcg_t		*smcg = (smcg_t *)macinfo->gldm_private;
	Adapter_Struc	*pAd = smcg->smcg_pAd;
	int		rc;

#ifdef	DEBUG
	if (SMCG_debug & SMCGTRACE)
		cmn_err(CE_CONT, SMCG_NAME "_start_board(0x%x)", macinfo);
#endif

	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_first->smcg_dual_port_lock);

	rc = LM_Open_Adapter(pAd);

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_first->smcg_dual_port_lock);

	/*
	 * The spec says we should wait for UM_Status_Change, but all LMs
	 * we currently support change the status prior to returning from
	 * LM_Open_Adapter().
	 */

	if (rc != SUCCESS)
		cmn_err(CE_WARN,
		    SMCG_NAME " LM_Open_Adapter failed %d", rc);

	return (rc == SUCCESS ? 0 : -1);
}

/*
 * SMCG_stop_board() -- stop board receiving
 */
static int
SMCG_stop_board(gld_mac_info_t *macinfo)
{
	smcg_t		*smcg = (smcg_t *)macinfo->gldm_private;
	Adapter_Struc	*pAd = smcg->smcg_pAd;
	int		rc;

#ifdef	DEBUG
	if (SMCG_debug & SMCGTRACE)
		cmn_err(CE_CONT, SMCG_NAME "_stop_board(0x%x)", macinfo);
#endif

	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_first->smcg_dual_port_lock);

	rc = LM_Close_Adapter(pAd);

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_first->smcg_dual_port_lock);

	/*
	 * The spec says we should wait for UM_Status_Change, but all LMs
	 * we currently support change the status prior to returning from
	 * LM_Close_Adapter().
	 */

#ifdef	DEBUG
	if (rc != SUCCESS)
		cmn_err(CE_WARN,
		    SMCG_NAME " LM_Close_Adapter failed %d", rc);
#endif

	return (rc == SUCCESS ? 0 : -1);
}

/*
 * SMCG_saddr() -- set node MAC address
 */
static int
SMCG_saddr(gld_mac_info_t *macinfo)
{
	smcg_t		*smcg = (smcg_t *)macinfo->gldm_private;
	Adapter_Struc	*pAd = smcg->smcg_pAd;
	int		rc;

	bcopy((caddr_t)macinfo->gldm_macaddr, (caddr_t)pAd->node_address,
		macinfo->gldm_addrlen);

	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_first->smcg_dual_port_lock);

	rc = LM_Initialize_Adapter(pAd);
	if (rc == SUCCESS)
		rc = LM_Open_Adapter(pAd);

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_first->smcg_dual_port_lock);

	return (rc == SUCCESS ? 0 : -1);
}

/*
 * SMCG_dlsdmult() -- set (enable) or disable a multicast address
 *
 * Program the hardware to enable/disable the multicast address
 * in "mcast".  Enable if "op" is non-zero, disable if zero.
 */
static int
SMCG_dlsdmult(gld_mac_info_t *macinfo, struct ether_addr *mcast, int op)
{
	smcg_t		*smcg = (smcg_t *)macinfo->gldm_private;
	Adapter_Struc	*pAd = smcg->smcg_pAd;
	int		rc;
	int i;

#ifdef	DEBUG
	if (SMCG_debug & SMCGTRACE)
		cmn_err(CE_CONT, SMCG_NAME "_dlsdmult(0x%x, %s)", macinfo,
		    op ? "ON" : "OFF");
#endif

	for (i = 0; i < ETHERADDRL; i++)
		pAd->multi_address[i] = mcast->ether_addr_octet[i];

	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_first->smcg_dual_port_lock);

	if (op) {
		if ((rc = LM_Add_Multi_Address(pAd)) == SUCCESS)
			if (++smcg->smcg_multicount == 1) {
				pAd->receive_mask |= ACCEPT_MULTICAST;
				rc = LM_Change_Receive_Mask(pAd);
			}
	} else {
		if ((rc = LM_Delete_Multi_Address(pAd)) == SUCCESS)
			if (--smcg->smcg_multicount == 0) {
				pAd->receive_mask &= ~ACCEPT_MULTICAST;
				rc = LM_Change_Receive_Mask(pAd);
			}
	}

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_first->smcg_dual_port_lock);

#ifdef	DEBUG
	if (rc != SUCCESS)
		cmn_err(CE_WARN,
		    SMCG_NAME "_dlsdmult failed %d", rc);
#endif

	return (rc == SUCCESS ? 0 : -1);
}


/*
 * SMCG_prom() -- set or reset promiscuous mode on the board
 *
 * Program the hardware to enable/disable promiscuous mode.
 * Enable if "on" is non-zero, disable if zero.
 */
static int
SMCG_prom(gld_mac_info_t *macinfo, int on)
{
	smcg_t		*smcg = (smcg_t *)macinfo->gldm_private;
	Adapter_Struc	*pAd = smcg->smcg_pAd;
	int		rc;

#ifdef	DEBUG
	if (SMCG_debug & SMCGTRACE)
		cmn_err(CE_CONT, SMCG_NAME "_prom(0x%x, %s)", macinfo,
		    on ? "ON" : "OFF");
#endif
	if (on)
		pAd->receive_mask |= PROMISCUOUS_MODE;
	else
		pAd->receive_mask &= ~PROMISCUOUS_MODE;

	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_first->smcg_dual_port_lock);

	rc = LM_Change_Receive_Mask(pAd);

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_first->smcg_dual_port_lock);

#ifdef	DEBUG
	if (rc != SUCCESS)
		cmn_err(CE_WARN,
		    SMCG_NAME "_prom: LM_Change_Receive_Mask failed %d", rc);
#endif

	return (rc == SUCCESS ? 0 : -1);
}

/*
 * SMCG_gstat() -- update statistics
 *
 * GLD calls this routine just before it reads the driver's statistics
 * structure.  If your board maintains statistics, this is the time to
 * read them in and update the values in the structure.  If the driver
 * maintains statistics continuously, this routine need do nothing.
 */
static int
SMCG_gstat(gld_mac_info_t *macinfo)
{

#ifdef	DEBUG
	if (SMCG_debug & SMCGTRACE)
		cmn_err(CE_CONT, SMCG_NAME "_gstat(0x%x)", macinfo);
#endif

	/*
	 * We don't have a statistics interface to the LMAC.
	 */

	return (SUCCESS);
}

/*
 * SMCG_send() -- send a packet
 */
static int
SMCG_send(gld_mac_info_t *macinfo, mblk_t *mp)
{
	smcg_t		*smcg = (smcg_t *)macinfo->gldm_private;
	Adapter_Struc	*pAd = smcg->smcg_pAd;
	char    *buf = NULL;
	int	totalbytecnt = 0;
	Data_Buff_Structure mb;
	mblk_t  *tmp;
	int	i = 0;
	int	rc;
	int	max_frags =
		    sizeof (mb.fragment_list) / sizeof (mb.fragment_list[0]);

#ifdef	DEBUG
	if (SMCG_debug & SMCGSEND)
		cmn_err(CE_CONT, SMCG_NAME "_send(0x%x, 0x%x)", macinfo, mp);
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
		if (++mb.fragment_count == max_frags) {
			int index = 0, totfrag = 0, length;
			buf = kmem_alloc(SMMAXPKT, KM_NOSLEEP);
			if (buf == NULL) {
#ifdef	DEBUG
				cmn_err(CE_WARN, SMCG_NAME
				    "_send: kmem_alloc failed");
#endif
				return (1);
			}
			while (tmp) {
				length = tmp->b_wptr-tmp->b_rptr;
				totfrag += length;
				if ((totalbytecnt+totfrag) > SMMAXPKT) {
					cmn_err(CE_WARN, SMCG_NAME "_send: "
					    "dropping huge outgoing packet %d",
					    (totalbytecnt + totfrag));
					kmem_free(buf, SMMAXPKT);
					return (0);
				}
				bcopy((caddr_t)tmp->b_rptr, buf+index, length);
				index += length;
				tmp = tmp->b_cont;
			}
			mb.fragment_list[i].fragment_ptr = (ulong)buf;
			mb.fragment_list[i].fragment_length = (ushort)totfrag;
			totalbytecnt += totfrag;
			break;
		}
		mb.fragment_list[i].fragment_ptr = (ulong)tmp->b_rptr;
		mb.fragment_list[i].fragment_length =
			(ushort)(tmp->b_wptr- tmp->b_rptr);
		totalbytecnt += mb.fragment_list[i].fragment_length;
		tmp = tmp->b_cont;
		i++;
	}

	if (totalbytecnt > SMMAXPKT) {
		cmn_err(CE_WARN, SMCG_NAME
		    "_send: dropping huge outgoing packet %d", totalbytecnt);
		if (buf)
			kmem_free(buf, SMMAXPKT);
		return (0);
	}

	if (totalbytecnt < ETHERMIN)
		totalbytecnt = ETHERMIN;	/* pad if necessary */

	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_first->smcg_dual_port_lock);

	rc = LM_Send(&mb, pAd, totalbytecnt);

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_first->smcg_dual_port_lock);

	if (buf)
		kmem_free(buf, SMMAXPKT);

#ifdef	DEBUG
	if (rc != SUCCESS && rc != OUT_OF_RESOURCES)
		cmn_err(CE_WARN,
		    SMCG_NAME "_send: LM_Send failed %d", rc);
#endif
	if (rc == OUT_OF_RESOURCES)
		return (1);
	else
		return (0);
}

/*
 * SMCG_intr() -- interrupt from board to inform us that a receive or
 * transmit has completed.
 */
static u_int
SMCG_intr(gld_mac_info_t *macinfo)
{
	smcg_t		*smcg = (smcg_t *)macinfo->gldm_private;
	Adapter_Struc	*pAd = smcg->smcg_pAd;
	int		rc;

#ifdef	DEBUG
	if (SMCG_debug & SMCGINT)
		cmn_err(CE_CONT, SMCG_NAME "_intr(0x%x)", macinfo);
#endif

	if (!smcg->smcg_ready) {
#ifdef	DEBUG
		cmn_err(CE_WARN, SMCG_NAME "_intr: premature interrupt");
#endif
		return (DDI_INTR_UNCLAIMED);
	}

	if (smcg->smcg_numchannels > 1)
		mutex_enter(&smcg->smcg_first->smcg_dual_port_lock);

	LM_Disable_Adapter(pAd);
	rc = LM_Service_Events(pAd);
	LM_Enable_Adapter(pAd);

	if (smcg->smcg_numchannels > 1)
		mutex_exit(&smcg->smcg_first->smcg_dual_port_lock);

#ifdef	DEBUG
	if (rc != SUCCESS)
		cmn_err(CE_WARN,
		    SMCG_NAME "_intr: LM_Service_Events error %d", rc);
#endif

	if (rc == NOT_MY_INTERRUPT)
		return (DDI_INTR_UNCLAIMED);
	else
		return (DDI_INTR_CLAIMED);
}

/*
 * UMAC callback functions
 */

/*
 * UM_Receive_Packet() -- LM has received a packet
 */
int
UM_Receive_Packet(char *plkahead, unsigned short length,
    Adapter_Struc *pAd, int status)
{
	mblk_t	*bp;
	Data_Buff_Structure dbuf;
	int rc;

#ifdef	DEBUG
	if (SMCG_debug & SMCGRECV)
		cmn_err(CE_CONT, SMCG_NAME " UM_Receive_Packet len=%d", length);
#endif

	/* get buffer to put packet in & move it there */
	if ((bp = allocb(length+2, BPRI_MED)) != NULL) {
		bp->b_rptr += 2;	/* align IP header for performance */
		dbuf.fragment_count = 1;
		dbuf.fragment_list[0].fragment_ptr = (ulong)bp->b_rptr;
		dbuf.fragment_list[0].fragment_length = length;

		if ((rc = LM_Receive_Copy(length, 0, &dbuf, pAd, 1))
		    == SUCCESS) {
			bp->b_wptr = bp->b_rptr + length;
#ifdef DEBUG_PROMISC
			if (!(*(caddr_t)bp->b_rptr & 0x1) &&
			    !(pAd->receive_mask & PROMISCUOUS_MODE) &&
			    bcmp((char *)pAd->node_address, dptr, 6) == 0) {
				int i;
				cmn_err(CE_CONT, SMCG_NAME " Receive_Packet: "
				    "received unwanted packet <");
				for (i = 0; i < ETHERADDRL * 2; i++)
					cmn_err(CE_CONT, "%2X:",
					    *((caddr_t)bp->b_rptr + i));
				cmn_err(CE_CONT, ">\n");
			}
#endif
			gld_recv(
			    ((smcg_t *)pAd->sm_private)->smcg_macinfo, bp);
		} else {
#ifdef	DEBUG
			/* 8416 returns BUFFER_TOO_SMALL_ERROR */
			cmn_err(CE_CONT, SMCG_NAME
			    " LM_Receive_Copy failed (%d)", rc);
#endif
			freeb(bp);
		}
	}

	return (rc);
}

/*
 * UM_Status_Change -- LM has completed a driver state change
 */
int
UM_Status_Change(Adapter_Struc *pAd)
{
	/*
	 * This function is called by several LMACs but the completion
	 * mechanism is not used by the UMAC to determine if the event
	 * has completed, because all applicable functions complete
	 * prior to returning.
	 */
	return (SUCCESS);
}

/*
 * UM_Receive_Copy_Complete() -- LM has completed a receive copy
 */
int
UM_Receive_Copy_Complete(Adapter_Struc *pAd)
{
	/*
	 * This completion mechanism is not used by the UMAC to
	 * determine if the copy has completed, because all LMACs
	 * complete the copy prior to returning.
	 */
	return (SUCCESS);
}

/*
 * UM_Send_Complete() -- LM has completed sending a packet
 */
int
UM_Send_Complete(int sstatus, Adapter_Struc *pAd)
{
	/* We don't care whether or when the packet got sent */
	return (SUCCESS);
}

/*
 * UM_Interrupt() -- LM has generated an interrupt at our request
 */
int
UM_Interrupt(Adapter_Struc *pAd)
{
#ifdef	DEBUG
	cmn_err(CE_WARN, SMCG_NAME " UM_Interrupt called unexpectedly");
#endif
	return (SUCCESS);
}

static int
lm_stub()
{
	return (SUCCESS);
}
