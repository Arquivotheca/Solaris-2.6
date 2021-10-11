/*
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident	"@(#)pcscsi.c	95/06/05 SMI"

/* ========================================================================== */
/*
 *	This is the driver for the AMD PCscsi chip.  It supports the following
 *	AMD chips:
 *
 *		PCscsi		(Am53C974)
 *		PCscsi II	(Am53C974A)
 *		PCnet-SCSI	(Am79C974)
 *
 *	This chip has embedded PCI support (connects directly to the PCI
 *	bus).  Thus the driver supports the chip only on PCI.
 *
 *	This driver is utilizes the AMD-provided 'portable core module',
 *	rev 2.01 (date 2/15/95).  (The files are ggmini.c and ggmini.h).
 *
 *	NOTE that the modifications have been made to the AMD core module,
 *	and certain include files.  In all cases the modifications have been
 *	marked in some way with the string SOLARIS.
 *
 *	The driver also provides a number of OS-specific routines which the
 *	core calls.  These are contained in the file portability.c.
 *
 *	Eric Theis 5/1/95
 *
 */


/* ========================================================================== */
/*
 * General driver includes.
 */
#include <sys/modctl.h>
#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#include <sys/types.h>
#include <sys/pci.h>


/*
 * Includes specifically for the AMD core code.
 * Order is significant.
 */
#include "miniport.h"	/* N.B.: Includes "srb.h"! */
#include "scsi.h"
#include "ggmini.h"


/*
 * Driver-specific.
 * Order is significant.
 */
#include <sys/dktp/pcscsi/pcscsi_dma_impl.h>
#include <sys/dktp/pcscsi/pcscsi.h>



/* ========================================================================== */
/*
 * Force modules we depend on to be loaded.
 */
#if defined PCI_DDI_EMULATION
char _depends_on[] = "misc/scsi misc/xpci";	/* 2.4 compatibility */
#else
char _depends_on[] = "misc/scsi";		/* 2.5 compatibility */
#endif	/* PCI_DDI_EMULATION */



/* ========================================================================== */
/*
 * External references (Driver entry points).
 */

static int
pcscsi_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
		void **result);

static int
pcscsi_identify(dev_info_t *dev);


static int
pcscsi_probe(dev_info_t *);

static int
pcscsi_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);

static int
pcscsi_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);


static int
pcscsi_tran_tgt_init(dev_info_t *, dev_info_t *, scsi_hba_tran_t *,
		struct scsi_device *);

static int
pcscsi_tran_tgt_probe(struct scsi_device *, int (*)());

static void
pcscsi_tran_tgt_free(dev_info_t *, dev_info_t *, scsi_hba_tran_t *,
		struct scsi_device *);



static int
pcscsi_transport(struct scsi_address *ap, struct scsi_pkt *scsi_pkt_p);


static int
pcscsi_abort(struct scsi_address *ap, struct scsi_pkt *pkt);

static int
pcscsi_reset(struct scsi_address *ap, int level);

static int
pcscsi_getcap(struct scsi_address *ap, char *cap, int tgtonly);

static int
pcscsi_setcap(struct scsi_address *ap, char *cap, int value, int tgtonly);



static struct scsi_pkt *
pcscsi_tran_init_pkt(struct scsi_address *ap, struct scsi_pkt *pkt,
		struct buf *bp, int cmdlen, int statuslen,
		int tgtlen, int flags, int (*callback)(), caddr_t arg);

static void
pcscsi_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);

static void
pcscsi_tran_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);

static void
pcscsi_tran_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);



static int
pcscsi_system_halt(dev_info_t *dip, ddi_reset_cmd_t cmd);



/* -------------------------------------------------------------------------- */
/*
 * Internal functions
 */

static int
pcscsi_init_properties(
			dev_info_t		 *dip,
			struct pcscsi_blk	 *pcscsi_blk_p);

static boolean_t
pcscsi_get_property(
			dev_info_t	 *dip,
			caddr_t		propname,
			caddr_t		propdefault);

static int
pcscsi_device_setup(struct pcscsi_blk *pcscsi_blk_p);

static int
pcscsi_device_teardown(struct pcscsi_blk *pcscsi_blk_p);


static int
pcscsi_xfer_setup(
	struct pcscsi_ccb	 *ccb_p,
	struct buf		 *bp,
	int			pkt_flags,
	int			(*callback)(),
	boolean_t		new_transfer);

static int
pcscsi_capchk(char *cap, int tgtonly, int *cidxp);


#if defined PCI_DDI_EMULATION
static int
pcscsi_xlate_vec(struct pcscsi_blk *pcscsi_blk_p, u_int *intr_idx);
#endif	/* PCI_DDI_EMULATION */


static u_int
pcscsi_dummy_intr(caddr_t arg);

static u_int
pcscsi_intr(caddr_t arg);



static int
pcscsi_send(struct pcscsi_blk *pcscsi_blk_p, struct pcscsi_ccb *ccb_p,
		long retry_timeout);

static void
pcscsi_pollret(struct pcscsi_blk *pcscsi_blk_p, struct scsi_pkt *scsi_pkt_p);

static u_int
pcscsi_chkerr(struct pcscsi_blk *pcscsi_blk_p, struct scsi_pkt *scsi_pkt_p);

static u_int
pcscsi_run_tgt_compl(caddr_t arg);


static int
pcscsi_request_alloc(
	struct scsi_address	 *ap,
	int			cmdlen,
	int			statuslen,
	int			tgtlen,
	int			flags,
	int			(*callback)(),
	caddr_t			arg,
	struct pcscsi_ccb	 **ccb_p_p);

static int
pcscsi_request_init(struct pcscsi_ccb *ccb_p);


static int
pcscsi_hba_request_alloc(
	struct scsi_address	 *ap,
	int			cmdlen,
	int			statuslen,
	struct pcscsi_ccb	 **ccb_p_p,
	int			(*callback)());

static void
pcscsi_hba_request_free(struct pcscsi_ccb *ccb_p);


static int
pcscsi_device_structs_alloc(
	struct scsi_address	 *ap,
	struct pcscsi_ccb	 *ccb_p,
	int			cmdlen,
	int			statuslen,
	int			(*callback)());

static void
pcscsi_device_structs_free(struct pcscsi_ccb *ccb_p);


static int
pcscsi_dma_structs_alloc(struct pcscsi_ccb *ccb_p, int (*callback)());

static void
pcscsi_dma_structs_free(struct pcscsi_ccb *ccb_p);


static int
pcscsi_dma_device_structs_alloc(
	struct pcscsi_ccb	 *ccb_p,
	int			(*callback)());

static int
pcscsi_dma_device_structs_init(
	struct pcscsi_ccb	 *ccb_p,
	boolean_t		new_transfer);

static void
pcscsi_dma_device_structs_free(struct pcscsi_ccb *ccb_p);


static int
pcscsi_dma_driver_structs_alloc(
	struct pcscsi_ccb	 *ccb_p,
	int			(*callback)());

static void
pcscsi_dma_driver_structs_free(struct pcscsi_ccb *ccb_p);

#ifdef	UNIT_QUEUE_SIZE
static int
pcscsi_queue_request(struct pcscsi_ccb *ccb_p);

/*
 * Defined in pcscsi.h:
 * struct pcscsi_ccb *
 * pcscsi_dequeue_request(int target, int lun, struct pcscsi_blk *blk_p);
 */

static struct pcscsi_ccb *
pcscsi_dequeue_next_ccb(struct pcscsi_blk *pcscsi_blk_p);

#endif	/* UNIT_QUEUE_SIZE	*/


#ifdef PCSCSI_DEBUG

void
pcscsi_debug(uint funcs, uint granularity, char *message);

static void
pcscsi_dump_blk(struct pcscsi_blk *pcscsi_blk_p);

static void
pcscsi_dump_ccb(struct pcscsi_ccb *pcscsi_ccb_p);

static void
pcscsi_dump_srb(SCSI_REQUEST_BLOCK *srb_p);

#endif	/* PCSCSI_DEBUG */



/* -------------------------------------------------------------------------- */
/*
 * Function prototypes for 'Portability Layer' functions (those called by
 * the AMD 'core module' code)
 *
 * (These are prototyped in srb.h)
 */



/* -------------------------------------------------------------------------- */
/*
 * Function prototypes for those AMD core routines lacking them.
 */

ULONG
DriverEntry(IN PVOID DriverObject, IN PVOID Argument2);



/* ========================================================================== */
/*
 * Local static data
 *
 * (None)
 */



/* -------------------------------------------------------------------------- */
/*
 * Globals
 *
 * (Globals are named pcscsig_whatever).
 */

/* Used by the PCSCSI_KVTOP macro.  */
int pcscsig_pgsz = 0;
int pcscsig_pgmsk;
int pcscsig_pgshf;


/* Driver code global mutex */
static kmutex_t pcscsig_global_mutex;


/*
 * DMA limits for data transfer
 */
ddi_dma_lim_t pcscsig_dma_lim = {
	0,			/* address reg lower bound		 */
	0xffffffffU,		/* address reg upper bound		 */
	0,			/* counter max (0 for x86)		 */
	1,			/* burstsize (1 for x86)		 */
	DMA_UNIT_8,		/* minimum xfer size (8, 16, 32)	 */
	0,			/* dma speed (must be zero)		 */
	(u_int)DMALIM_VER0,	/* DMA struct version			 */
	0xffffffffU,		/* address register max-1 (32 bits)	 */
	0x003fffff,		/* counter register max-1 (24 bits)	 */
	512,			/* transfer granularity (sector size)	 */
	MAX_SG_LIST_ENTRIES,	/* max scatter/gather list length	 */
	0xffffffffU		/* max I/O request size			 */
};


struct dev_ops	pcscsi_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	pcscsi_getinfo,		/* info */
	pcscsi_identify,	/* identify */
	pcscsi_probe,		/* probe */
	pcscsi_attach,		/* attach */
	pcscsi_detach,		/* detach */
	pcscsi_system_halt,	/* system reset - flush the cache */
	(struct cb_ops *)0,	/* driver operations */
	NULL			/* bus operations */
};



#ifdef PCSCSI_DEBUG

/* The routines/execution paths for which debug reporting can be turned on:  */
uint	pcscsig_debug_funcs = (

/* DBG_PROBE		|   */
/* DBG_ATTACH		|   */
/* DBG_TGT_INIT		|   */
/* DBG_TGT_PROBE		|   */
/* DBG_TRAN_INIT_PKT	|   */
/* DBG_PKTALLOC		|   */
/* DBG_DMAGET		|   */
/* DBG_TRANSPORT		|   */
/* DBG_INTR		|   */
/* DBG_PKT_COMPLETION	|   */
/* DBG_PKT_CHK_ERRS	|   */
/* DBG_SOFTINT		|   */
/* DBG_ABORT		|   */
/* DBG_RESET		|   */

/* Turns on debug reporting in portability.c (portability layer) */
/* DBG_PORTABILITY	|   */
/* Turns on *all* of AMD's debug reporting AND debug in ggmini_solaris.c. */
/* DBG_CORE		|   */

0x0			/* Just to make changes easy */
);


/*
 * The routines/execution paths for which debug reporting can be turned on:
 * (for pcscsig_debug_gran)
 * For all of the above, granularity of debug reporting.
 */
uint	pcscsig_debug_gran = (

/* DBG_ENTRY		|   */
DBG_RESULTS		|
/* DBG_VERBOSE		|   */

0x0			/* Just to make changes easy */
);

char	pcscsig_dbgmsg[100];

#endif /* PCSCSI_DEBUG */



/* ========================================================================== */
/*
 * This is the loadable module wrapper.
 */

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module. This one is a driver */
	"PCSCSI SCSI Host Adapter Driver",	/* Name of the module. */
	&pcscsi_ops,				/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};


/* -------------------------------------------------------------------------- */
/*
 * Framework-required _init routine.
 */
int
_init(void)
{
	int	status;

	if ((status = scsi_hba_init(&modlinkage)) != 0) {
		return (status);
	}

	mutex_init(&pcscsig_global_mutex, "PCSCSI global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&pcscsig_global_mutex);
	}
	return (status);
}


/* -------------------------------------------------------------------------- */
/*
 * Framework-required _fini routine.
 */
int
_fini(void)
{
	int	status;

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&pcscsig_global_mutex);
	}
	return (status);
}


/* -------------------------------------------------------------------------- */
/*
 * Framework-required _info routine.
 */
int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}



/* ========================================================================== */
/*
 * Autoconfiguration routines.
 */

/* -------------------------------------------------------------------------- */
/*
 * Framework-required identify routine.
 */
static int
pcscsi_identify(dev_info_t *dip)
{
	char *dname = ddi_get_name(dip);

	if (
		(strcmp(dname, "pcscsi") == 0)		||
		(strcmp(dname, "pci1022,2020") == 0))	{

		return (DDI_IDENTIFIED);

	} else {

		return (DDI_NOT_IDENTIFIED);

	}
}


/* -------------------------------------------------------------------------- */
/*
 * Probe routine.
 * See if the device corrsponding to the dev_info node we've been passed
 * has our vendor and device id.
 */
static int
pcscsi_probe(register dev_info_t *dip)
{
	ddi_acc_handle_t	pcscsi_pci_config_handle;
	ushort_t		vendorid, deviceid;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PROBE, DBG_ENTRY, "pcscsi_probe: Entry\n");
#endif /* PCSCSI_DEBUG */


	/*
	 * Set up access to PCI configuration space, for the dev_info
	 * node passed to us.
	 */
	if (pci_config_setup(dip, &pcscsi_pci_config_handle) != DDI_SUCCESS) {

		/*
		 * If this fails, we're either not a PCI bus,
		 * or something's seriously wrong.
		 */
		return (DDI_PROBE_FAILURE);
	}


	/*
	 * The above has set up everything to allow us to access the
	 * PCI config space for the dev_info node passed to us.
	 * Now let's see if it's our device.
	 */
	vendorid = pci_config_getw(pcscsi_pci_config_handle, PCI_CONF_VENID);
	deviceid = pci_config_getw(pcscsi_pci_config_handle, PCI_CONF_DEVID);

#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PROBE, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_probe: vendor %x device %x\n", vendorid, deviceid));
#endif /* PCSCSI_DEBUG */


	/*
	 * Deallocate resources set up for PCI config space access.
	 * (We can't retain state; so we must throw away the config
	 * space access set up above before we leave this function).
	 */
	pci_config_teardown(&pcscsi_pci_config_handle);


	/* If it's our device, SUCCESS; else failure.	 */
	switch (vendorid)  {

	case AMD_VENDOR_ID:
		/*
		 * Check for PCnet-SCSI, PCscsi, and PCscsi-II chips.
		 * Note all have the same deviceid.
		 * The PCnet-SCSI looks like two single-function chips,
		 * hence the SCSI device has the same deviceid as PCscsi.
		 */
		switch (deviceid)  {

		case PCSCSI_DEVICE_ID:
			return (DDI_PROBE_SUCCESS);
		}
	}

	return (DDI_PROBE_FAILURE);

}


/* -------------------------------------------------------------------------- */
/*ARGSUSED*/
/*
 * Set up and initialize everything necessary to make this driver instance
 * usable by the system.
 */
static int
pcscsi_attach(
	dev_info_t		 *dip,
	ddi_attach_cmd_t	cmd)
{
	register struct pcscsi_glue	 *pcscsi_glue_p;
	register struct pcscsi_blk	 *pcscsi_blk_p;
	int			i, len;
	int			mp_safe;
	u_int			intr_idx;


	intr_idx = 0;


	/*
	 * We can only handle DDI_ATTACH requests.
	 */
	switch (cmd) {
	case DDI_ATTACH:
		break;

	default:
		return (DDI_FAILURE);
	}


	/*
	 * Set up parameters for the KVTOP (kernel-virtual-address to
	 * physical-address) macro.  (Once only; these are driver-global).
	 */
	if (!pcscsig_pgsz) {
		pcscsig_pgsz = ddi_ptob(dip, 1L);
		pcscsig_pgmsk = pcscsig_pgsz - 1;
		for (i = pcscsig_pgsz, len = 0; i > 1; len++)
			i >>= 1;
		pcscsig_pgshf = len;
	}


	/*
	 * Allocate space for the glue struct and the blk struct.
	 * They're contiguous, but just to save a second call to
	 * kmem_zalloc.
	 */
	pcscsi_glue_p = (struct pcscsi_glue *)
			kmem_zalloc(
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk),
			KM_SLEEP);
	if (pcscsi_glue_p == NULL)
		return (DDI_FAILURE);


	/*
	 * (Partially) initialize the blk struct.
	 */
	pcscsi_glue_p->pg_blk_p =
				(struct pcscsi_blk *) (pcscsi_glue_p + 1);
	pcscsi_blk_p = pcscsi_glue_p->pg_blk_p;

	pcscsi_blk_p->pb_dip = dip;	/* Save dev_info ptr */
	pcscsi_blk_p->pb_dma_lim_p = &pcscsig_dma_lim;


	/*
	 * Set up access to PCI configuration space, for the dev_info
	 * node passed to us.
	 * We are required to provide a routine for the AMD core code
	 * which acceses PCI config space.
	 */
	if (pci_config_setup(dip, &(pcscsi_blk_p->pb_pci_config_handle))
			!= DDI_SUCCESS) {
		/*
		 * If this fails, we're either not a PCI bus,
		 * or something's seriously wrong.
		 */

		cmn_err(CE_WARN, "pcscsi_attach: "
				"Cannot set up PCI config space access");
		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		return (DDI_FAILURE);
	}


	/*
	 * Get the IRQ and i/o address from PCI config space.
	 */
	pcscsi_blk_p->pb_intr = (uchar_t)
			pci_config_getb(pcscsi_blk_p->pb_pci_config_handle,
					PCI_CONF_ILINE);

	pcscsi_blk_p->pb_ioaddr = (ushort_t)
			pci_config_getl(pcscsi_blk_p->pb_pci_config_handle,
					PCI_CONF_BASE0);
	pcscsi_blk_p->pb_ioaddr &= ~1;	/* Get rid of "in I/O space" bit */


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_ATTACH, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_attach: IRQ: %x   I/O Address: %x\n",
		pcscsi_blk_p->pb_intr, pcscsi_blk_p->pb_ioaddr));
#endif  /* PCSCSI_DEBUG */


	/*
	 * Set the driver properties.
	 */
	if (pcscsi_init_properties(dip, pcscsi_blk_p) != DDI_SUCCESS) {

		cmn_err(CE_WARN, "pcscsi_attach: Cannot init properties.");
		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		return (DDI_FAILURE);
	}


	/*
	 * Set up and initialize the AMD core code.
	 *
	 * As attach is single-threaded (per device), we don't need
	 * to worry about setting up a mutex to touch the core code
	 * or hardware.  We'll set that up later.
	 * The core code initialization calls a portability layer
	 * routine (ScsiPortInitialize), which sets up *all* the
	 * memory the core requires.  Pointers to these areas are
	 * kept in the blk struct.  Thus any core state is kept in
	 * the blk struct, making the -code- stateless.
	 */
	if (pcscsi_device_setup(pcscsi_blk_p) == DDI_FAILURE) {
		cmn_err(CE_WARN, "pcscsi_attach: "
				"core code init failed");
		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		pci_config_teardown(
			&pcscsi_blk_p->pb_pci_config_handle);
		return (DDI_FAILURE);
	}


	/* Save the SCSI ID of the adapter.	 */
	pcscsi_blk_p->pb_targetid =
		pcscsi_blk_p->pb_core_PortConfigInfo_p->InitiatorBusId[0];


	/*
	 * Establish initial dummy interrupt handler, to
	 * get the iblock cookie to initialize mutexes used in the
	 * real interrupt handler.
	 */
#if defined PCI_DDI_EMULATION
	/* Only needed in 2.4, as the 2.5 framework takes care of this. */
	if (pcscsi_xlate_vec(pcscsi_blk_p, &intr_idx) != DDI_SUCCESS)  {
		cmn_err(CE_WARN, "pcscsi_attach: xlate vec failed");
		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		pci_config_teardown(
			&pcscsi_blk_p->pb_pci_config_handle);
		return (DDI_FAILURE);
	}
#endif	/* PCI_DDI_EMULATION */

	if (ddi_add_intr(dip,
			intr_idx,
			(ddi_iblock_cookie_t *) &pcscsi_blk_p->pb_iblock_cookie,
			(ddi_idevice_cookie_t *) 0,
			pcscsi_dummy_intr,
			(caddr_t) pcscsi_glue_p))	{

		cmn_err(CE_WARN, "pcscsi_attach: cannot add intr");
		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		pci_config_teardown(
			&pcscsi_blk_p->pb_pci_config_handle);
		return (DDI_FAILURE);
	}

	ddi_remove_intr(dip, intr_idx, pcscsi_blk_p->pb_iblock_cookie);


	/*
	 * Establish real interrupt handler
	 */
	if (ddi_add_intr(dip,
			intr_idx,
			(ddi_iblock_cookie_t *) &pcscsi_blk_p->pb_iblock_cookie,
			(ddi_idevice_cookie_t *) 0,
			pcscsi_intr,
			(caddr_t) pcscsi_glue_p))	{

		cmn_err(CE_WARN, "pcscsi_attach: cannot add intr");
		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		pci_config_teardown(
			&pcscsi_blk_p->pb_pci_config_handle);
		return (DDI_FAILURE);
	}


	/*
	 * Initialize the mutex that protects the blk struct,
	 * AMD core code, and hardware.
	 */
	mutex_init(&pcscsi_glue_p->pg_blk_p->pb_core_mutex,
			"PCSCSI Core Mutex",
			MUTEX_DRIVER,
			pcscsi_blk_p->pb_iblock_cookie);

	/*
	 * Initialize the "wating for core available to send the
	 * next request" condition variable.
	 */
	cv_init(&(pcscsi_blk_p->pb_wait_for_core_ready),
		"pcscsi_wait_for_core", CV_DRIVER, (void *)NULL);


	/*
	 * Allocate the (master, not clone) scsi_transport struct.
	 */
	if ((pcscsi_glue_p->pg_hbatran_p = scsi_hba_tran_alloc(dip, 0))
		== (scsi_hba_tran_t *)0)	{

		cmn_err(CE_WARN, "pcscsi_attach: scsi_hba_tran_alloc failed\n");

		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		pci_config_teardown(
			&pcscsi_blk_p->pb_pci_config_handle);
		return (DDI_FAILURE);
	}


	/*
	 * Initialize the scsi_transport struct.
	 */
	pcscsi_glue_p->pg_hbatran_p->tran_hba_private	= pcscsi_glue_p;
	pcscsi_glue_p->pg_hbatran_p->tran_tgt_private	= NULL;

	pcscsi_glue_p->pg_hbatran_p->tran_tgt_init	= pcscsi_tran_tgt_init;
	pcscsi_glue_p->pg_hbatran_p->tran_tgt_probe	= pcscsi_tran_tgt_probe;
	pcscsi_glue_p->pg_hbatran_p->tran_tgt_free	= pcscsi_tran_tgt_free;

	pcscsi_glue_p->pg_hbatran_p->tran_start	= pcscsi_transport;
	pcscsi_glue_p->pg_hbatran_p->tran_abort		= pcscsi_abort;
	pcscsi_glue_p->pg_hbatran_p->tran_reset		= pcscsi_reset;
	pcscsi_glue_p->pg_hbatran_p->tran_getcap	= pcscsi_getcap;
	pcscsi_glue_p->pg_hbatran_p->tran_setcap	= pcscsi_setcap;
	pcscsi_glue_p->pg_hbatran_p->tran_init_pkt	= pcscsi_tran_init_pkt;
	pcscsi_glue_p->pg_hbatran_p->tran_destroy_pkt = pcscsi_tran_destroy_pkt;
	pcscsi_glue_p->pg_hbatran_p->tran_dmafree	= pcscsi_tran_dmafree;
	pcscsi_glue_p->pg_hbatran_p->tran_sync_pkt	= pcscsi_tran_sync_pkt;


	/*
	 * Try to attach to the framework.
	 */
	if (scsi_hba_attach(
				dip,
				pcscsi_blk_p->pb_dma_lim_p,
				pcscsi_glue_p->pg_hbatran_p,
				SCSI_HBA_TRAN_CLONE,
				NULL)
					!= DDI_SUCCESS) {

		cmn_err(CE_WARN, "pcscsi_attach: scsi_hba_attach failed");

		ddi_remove_intr(dip, intr_idx, pcscsi_blk_p->pb_iblock_cookie);
		mutex_destroy(&pcscsi_glue_p->pg_blk_p->pb_core_mutex);
		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		scsi_hba_tran_free(pcscsi_glue_p->pg_hbatran_p);
		pci_config_teardown(
			&pcscsi_blk_p->pb_pci_config_handle);
		return (DDI_FAILURE);
	}


	/*
	 * Initiailize the softint used to handle target completion
	 * routines.
	 */
	if (ddi_add_softintr(dip,
			DDI_SOFTINT_MED,
			&pcscsi_blk_p->pb_soft_int_id,
			0,
			0,
			pcscsi_run_tgt_compl,
			(caddr_t)pcscsi_blk_p)
				!= DDI_SUCCESS) {

		cmn_err(CE_WARN, "pcscsi_attach: Cannot add softintr\n");

		ddi_remove_intr(dip, intr_idx, pcscsi_blk_p->pb_iblock_cookie);
		mutex_destroy(&pcscsi_glue_p->pg_blk_p->pb_core_mutex);
		kmem_free((caddr_t)pcscsi_glue_p,
			sizeof (struct pcscsi_glue) +
				sizeof (struct pcscsi_blk));
		scsi_hba_tran_free(pcscsi_glue_p->pg_hbatran_p);
		pci_config_teardown(
			&pcscsi_blk_p->pb_pci_config_handle);

		return (DDI_FAILURE);
	}


	ddi_report_dev(dip);	/*  Announce that we're attached. */


	return (DDI_SUCCESS);
}


/* -------------------------------------------------------------------------- */
/*
 * Tear down and deallocate everything necessary to make this driver instance
 * unloadable by the system.  This driver instance cannot be accessed after
 * this succeeds.
 */
static int
pcscsi_detach(
	dev_info_t			 *dip,
	ddi_detach_cmd_t		cmd)
{
	register struct pcscsi_glue	 *pcscsi_glue_p;
	register struct	pcscsi_blk	 *pcscsi_blk_p;
	scsi_hba_tran_t			 *tran;
	u_int				intr_idx = 0;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_ATTACH, DBG_ENTRY, "pcscsi_detach: Entry\n");
#endif  /* PCSCSI_DEBUG */


	/*
	 * We can only handle DETACH.
	 * (Not actually called in 2.4).
	 */
	switch (cmd) {
	case DDI_DETACH:
		break;

	default:
		return (DDI_FAILURE);
	}	/* End switch (cmd) */



	tran = (scsi_hba_tran_t *) ddi_get_driver_private(dip);
	if (!tran)
		return (DDI_SUCCESS);

	pcscsi_glue_p = TRAN2MASTERGLUE(tran);
	if (!pcscsi_glue_p)	/* Already detached */
		return (DDI_SUCCESS);

	pcscsi_blk_p = PCSCSI_BLKP(pcscsi_glue_p);


	/*
	 * If there's devices still unfreed, fail.
	 */
	if (pcscsi_glue_p->pg_blk_p->pb_active_units != 0)	{
		cmn_err(CE_WARN,
			"pcscsi_detach: "
			"Units still unfreed - detach failed\n");
		return (DDI_FAILURE);
	};


	/*
	 * Release any resources allocted for this device instance.
	 */

	/*
	 * Free any resources the core code is using.
	 */
	if (pcscsi_device_teardown(pcscsi_blk_p) != DDI_SUCCESS)  {
		cmn_err(CE_WARN,
			"pcscsi_attach: pcscsi_device_teardown failed\n");
		return (DDI_FAILURE);
	}


	pci_config_teardown(&pcscsi_blk_p->pb_pci_config_handle);


	/*
	 * Remove the interrupt
	 */
#if defined PCI_DDI_EMULATION
	if (pcscsi_xlate_vec(pcscsi_blk_p, &intr_idx) != DDI_SUCCESS) {
		cmn_err(CE_WARN,
			"pcscsi_attach: scsi_hba_detach (xlate) failed\n");
		return (DDI_FAILURE);
	}
#endif	/* PCI_DDI_EMULATION */

	ddi_remove_intr(dip, intr_idx, pcscsi_blk_p->pb_iblock_cookie);


	/*
	 * Remove the condition variable.
	 */
	cv_destroy(&(pcscsi_blk_p->pb_wait_for_core_ready));


	/*
	 * Remove the softint.
	 */
	ddi_remove_softintr(pcscsi_blk_p->pb_soft_int_id);


	/*
	 * Free the 'master' glue and blk struct
	 */
	kmem_free((caddr_t)pcscsi_glue_p,
		sizeof (struct pcscsi_glue) + sizeof (struct pcscsi_blk));


	/*
	 * Remove all properties set up for this nexus
	 */
	ddi_prop_remove_all(dip);


	/*
	 * Detach this instance from the framework.
	 */
	if (scsi_hba_detach(dip) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "pcscsi_detach: scsi_hba_detach failed\n");
		return (DDI_FAILURE);
	}


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_ATTACH, DBG_ENTRY,
		"pcscsi_detach: Successful exit.\n");
#endif  /* PCSCSI_DEBUG */


	return (DDI_SUCCESS);		/* Completed successfully.  */
}


/* -------------------------------------------------------------------------- */
/*ARGSUSED*/
/*
 * Framework-required getinfo routine.
 */
static int
pcscsi_getinfo(
	dev_info_t	 *dip,
	ddi_info_cmd_t	infocmd,
	void		 *arg,
	void		 **result)
{
	/* getinfo not supported for HBA drivers */
	return (DDI_FAILURE);
}



/* ========================================================================== */
/*
 * Target driver installation entry points.
 */

/* -------------------------------------------------------------------------- */
/*
 * Target initilization routine.
 * Called by EACH target driver (i.e., cmdk, st, ...)
 * for EACH entry in the target driver's .conf file.
 * Performs any per-target/lun initialization for the driver.
 *
 * Note that the clone_hba_tran passed in here is a CLONE of the
 * 'master' hba_transport struct.
 */
/*ARGSUSED*/
static int
pcscsi_tran_tgt_init(
	dev_info_t		 *hba_dip,
	dev_info_t		 *tgt_dip,
	scsi_hba_tran_t		 *clone_hba_tran,
	struct scsi_device	 *sd)
{
	int			targ;
	int			lun;
	struct	pcscsi_glue	 *hba_pcscsi_glue_p;
	struct	pcscsi_glue	 *unit_pcscsi_glue_p;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_TGT_INIT, DBG_ENTRY, "pcscsi_tran_tgt_init: Entry.\n");
#endif /* PCSCSI_DEBUG */


	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_TGT_INIT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_tran_tgt_init: %s%d: Tgt: %s%d Targ/Lun: <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun));
#endif /* PCSCSI_DEBUG */


	if (targ < 0 || targ > 7 || lun < 0 || lun > 7) {

		cmn_err(CE_CONT, "%s%d: %s%d Bad target/lun address: <%d,%d>\n",
			ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
			ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
			targ, lun);

		return (DDI_FAILURE);
	}


	/*
	 * Allocate (clone) glue struct.
	 */
	if ((unit_pcscsi_glue_p =
		kmem_zalloc(sizeof (struct pcscsi_glue), KM_SLEEP)) == NULL) {

		cmn_err(CE_WARN,
			"pcscsi_tran_tgt_init: glue kmem_zalloc failed\n");

		return (DDI_FAILURE);
	}


	/* Get ptr to 'master' glue struct */
	hba_pcscsi_glue_p = clone_hba_tran->tran_hba_private;

	/* Copy contents of the master glue struct to the clone.  */
	bcopy((caddr_t)hba_pcscsi_glue_p, (caddr_t)unit_pcscsi_glue_p,
		sizeof (*hba_pcscsi_glue_p));


	/*
	 * IFF we haven't already done so (by already being called from one
	 * of the various target drivers),
	 * Allocate and initialize the per-unit struct for this targ/lun, and
	 * save a pointer to it in the master blk struct.
	 */
	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

	mutex_enter(&hba_pcscsi_glue_p->pg_blk_p->pb_core_mutex);

	if (hba_pcscsi_glue_p->pg_blk_p->pb_unit_structs[targ][lun] == NULL) {

		/* Allocate the unit struct.  Tack on SpecificLuExtension */
		if ((hba_pcscsi_glue_p->pg_blk_p->pb_unit_structs[targ][lun]
			= (struct pcscsi_unit *)
			kmem_zalloc(
				sizeof (struct pcscsi_unit) +
		hba_pcscsi_glue_p->pg_blk_p->pb_core_SpecificLuExtensionSize,
				KM_SLEEP)) == (struct pcscsi_unit *)NULL) {

			cmn_err(CE_WARN,
			"pcscsi_tran_tgt_init: unit kmem_zalloc failed\n");

			kmem_free(unit_pcscsi_glue_p,
				sizeof (struct pcscsi_glue));

			mutex_exit(&hba_pcscsi_glue_p->pg_blk_p->pb_core_mutex);

			return (DDI_FAILURE);
		}


		/* Copy the master DMA lims struct into the unit struct. */
		hba_pcscsi_glue_p->pg_blk_p->pb_unit_structs[targ][lun]->pu_lim
			= *(hba_pcscsi_glue_p->pg_blk_p->pb_dma_lim_p);


		/*
		 * Set unit struct pointer to per-logical-unit-extension
		 * for the AMD core code.
		 */
		hba_pcscsi_glue_p->pg_blk_p->pb_unit_structs[targ][lun]
			->pu_SpecificLuExtension =
				(PSPECIFIC_LOGICAL_UNIT_EXTENSION)
		(hba_pcscsi_glue_p->pg_blk_p->pb_unit_structs[targ][lun] +1);


		/* Note that we have set up another unit struct.	 */
		hba_pcscsi_glue_p->pg_blk_p->pb_active_units++;

	}	/* End if (unit struct not created for this node) */


	/* Point this instance of the glue struct to the unit struct. */
	unit_pcscsi_glue_p->pg_unit_p =
			hba_pcscsi_glue_p->pg_blk_p->pb_unit_structs[targ][lun];


	/* Note an additional node is now accessing this 'unit' */
	unit_pcscsi_glue_p->pg_unit_p->pu_refcnt++;


	/* Point clone_hba_transport struct to corresponding unit struct */
	clone_hba_tran->tran_tgt_private = unit_pcscsi_glue_p;

	mutex_exit(&hba_pcscsi_glue_p->pg_blk_p->pb_core_mutex);


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_TGT_INIT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"pcscsi_tran_tgt_init: Success! <%d,%d>\n", targ, lun));
#endif /* PCSCSI_DEBUG */

	return (DDI_SUCCESS);
}


/* -------------------------------------------------------------------------- */
/*ARGSUSED*/
/*
 * Target probe routine.
 * Called by EACH target driver (i.e., cmdk, st, ...)
 * for EACH entry in the target driver's .conf file.
 * Determines if the target/lun actually exists.
 *
 * Note that the clone_hba_tran passed in here is a CLONE of the
 * 'master' hba_transport struct.
 */
static int
pcscsi_tran_tgt_probe(
	struct scsi_device	 *sd,
	int			(*callback)())
{
	int			rval;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_TGT_PROBE, DBG_ENTRY,
		"pcscsi_tran_tgt_probe: Entry\n");
	pcscsi_debug(DBG_TGT_PROBE, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_tran_tgt_probe: sd: %x callback %x\n", sd, callback));
#endif /* PCSCSI_DEBUG */


	/* Call scsi_hba_probe to do the work.	 */
	rval = scsi_hba_probe(sd, callback);


#ifdef PCSCSI_DEBUG
	{
		char			 *s;
		struct pcscsi_glue	 *pcscsi_glue_p = SDEV2CLONEGLUE(sd);
		int			targ;
		int			lun;

		switch (rval) {
		case SCSIPROBE_NOMEM:
			s = "scsi_probe_nomem";
			break;
		case SCSIPROBE_EXISTS:
			s = "scsi_probe_exists";
			break;
		case SCSIPROBE_NONCCS:
			s = "scsi_probe_nonccs";
			break;
		case SCSIPROBE_FAILURE:
			s = "scsi_probe_failure";
			break;
		case SCSIPROBE_BUSY:
			s = "scsi_probe_busy";
			break;
		case SCSIPROBE_NORESP:
			s = "scsi_probe_noresp";
			break;
		default:
			s = "UNKNOWN STATUS???";
			break;
		}
		pcscsi_debug(DBG_TGT_PROBE, DBG_RESULTS,
			"pcscsi_tran_tgt_probe: scsi_hba_probe complete:\n");
		pcscsi_debug(DBG_TGT_PROBE, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"Status:(%d) %s Inst:%d: %s target:%d lun:%d\n",
			rval, s,
			ddi_get_instance(PCSCSI_DIP(pcscsi_glue_p)),
			ddi_get_name(sd->sd_dev),
			sd->sd_address.a_target,
			sd->sd_address.a_lun, s));

	}
#endif	/* PCSCSI_DEBUG */

	return (rval);
}


/* -------------------------------------------------------------------------- */
/*
 * Per-Target/Lun structs free routine.
 * Called by EACH target driver (i.e., cmdk, st, ...)
 * for EACH entry in the target driver's .conf file
 * for each of the target/luns which failed the target probe (above).
 *
 * Note that the hba_tran passed in here is a CLONE of the
 * 'master' hba_transport struct.
 */
/*ARGSUSED*/
static void
pcscsi_tran_tgt_free(
	dev_info_t		 *hba_dip,
	dev_info_t		 *tgt_dip,
	scsi_hba_tran_t		 *clone_hba_tran,
	struct scsi_device	 *sd)
{
	int			targ;
	int			lun;
	struct	pcscsi_glue	 *hba_pcscsi_glue_p;
	struct	pcscsi_glue	 *unit_pcscsi_glue_p;


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TGT_INIT, DBG_ENTRY, "pcscsi_tran_tgt_free: Entry\n");
	pcscsi_debug(DBG_TGT_INIT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_tran_tgt_free: %s%d %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		sd->sd_address.a_target,
		sd->sd_address.a_lun));
#endif	/* PCSCSI_DEBUG */


	/*
	 * Get ptr to 'master' glue struct.
	 */
	hba_pcscsi_glue_p  = clone_hba_tran->tran_hba_private;
	unit_pcscsi_glue_p = clone_hba_tran->tran_tgt_private;


	mutex_enter(&hba_pcscsi_glue_p->pg_blk_p->pb_core_mutex);


	/*
	 * Indicate that one less node is accessng this 'unit'.
	 */
	unit_pcscsi_glue_p->pg_unit_p->pu_refcnt--;

	ASSERT(unit_pcscsi_glue_p->pg_unit_p->pu_refcnt >= 0);


	/*
	 * If no one is now using this unit,
	 *	Free the per-target/lun structs.
	 */
	if (unit_pcscsi_glue_p->pg_unit_p->pu_refcnt == 0)	{

		ASSERT(unit_pcscsi_glue_p->pg_unit_p->pu_active_ccb_cnt == 0);

		/*
		 * Deallocate unit struct (and the SpecificDeviceExtension)
		 */
		kmem_free(unit_pcscsi_glue_p->pg_unit_p,
				sizeof (struct pcscsi_unit) +
				hba_pcscsi_glue_p->pg_blk_p->
					pb_core_SpecificLuExtensionSize);

		/*
		 * Clear ptr to the unit struct in the 'master' blk struct
		 * (used in portability.c)
		 */
		targ = sd->sd_address.a_target;
		lun = sd->sd_address.a_lun;
		hba_pcscsi_glue_p->pg_blk_p->pb_unit_structs[targ][lun] = NULL;

		/*
		 * Note that we have removed a unit struct.
		 */
		hba_pcscsi_glue_p->pg_blk_p->pb_active_units--;

		ASSERT(hba_pcscsi_glue_p->pg_blk_p->pb_active_units >= 0);

	}

	/*
	 * Deallocate the glue struct for this node instance.
	 *
	 * (Note there may well NOT be a one-to-one correspondence of
	 * clone hba/glue structs to unit structs at
	 * tran_tgt_/init/probe/free time.
	 * There will be exectly one unit struct per target/lun, but
	 * may (temporarily) be more than one clone_hba/clone_glue struct
	 * pointing to this unit struct.
	 */
	kmem_free(unit_pcscsi_glue_p, sizeof (struct pcscsi_glue));

	mutex_exit(&hba_pcscsi_glue_p->pg_blk_p->pb_core_mutex);

}


/* ========================================================================== */
/*
 * Resource Allocation Entry Points.
 */

/* -------------------------------------------------------------------------- */
/*
 * (Optionally) Allocate a SCSI packet, and
 * initilaize the SCSI packet (passed in or just allocated) and
 * any device-specific data structs, and
 * (optionally) allocate/initalize DMA resources for the transfer described
 * by the bp.
 *
 * There is some trickiness subsumed in this design.
 * Suffice it to say that the upper layer (target drivers) can reuse SCSI
 * packets without calling this routine again at all, or pass in an old
 * packet to be reused.
 */
static struct scsi_pkt *
pcscsi_tran_init_pkt(
	struct scsi_address	 *ap,
	struct scsi_pkt		 *scsi_pkt_p,
	struct buf		 *bp,
	int			cmdlen,
	int			statuslen,
	int			tgtlen,
	int			flags,
	int			(*callback)(),
	caddr_t			arg)
{
	struct pcscsi_blk	 *pcscsi_blk_p;
	struct pcscsi_ccb	 *ccb_p;
	boolean_t		request_allocated;
	boolean_t		new_transfer;


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRAN_INIT_PKT, DBG_ENTRY,
				"pcscsi_tran_init_pkt: Entry\n");
#endif	/* PCSCSI_DEBUG */


	request_allocated = FALSE;


	/*
	 * (If asked) Allocate the data structs for a SCSI request.
	 */
	if (scsi_pkt_p == (struct scsi_pkt *)NULL) {

		if (pcscsi_request_alloc(ap, cmdlen, statuslen,
			tgtlen, flags, callback, arg, &ccb_p)
			!= DDI_SUCCESS)  {


#ifdef PCSCSI_DEBUG
			pcscsi_debug(DBG_TRAN_INIT_PKT, DBG_RESULTS,
				"pcscsi_tran_init_pkt: request_alloc failed\n");
#endif /* PCSCSI_DEBUG */


			return ((struct scsi_pkt *)NULL);	/* Failure */
		}

		/*
		 *The ccb can hereafter be accessed thusly:
		 * PKT2CCB(scsi_pkt_p)
		 */

		scsi_pkt_p = ccb_p->ccb_pkt_p;

		request_allocated = TRUE;

	} else {
		ccb_p = PKT2CCB(scsi_pkt_p);	/* Init; needed below. */
	}



	/*
	 * (Re)Initialize the device-specific data structs for this transaction.
	 * Note that this must be done EVERY time, as the upper layer can
	 * reuse packets without telling us, and we must be sure
	 * any changes made to the packet (like flags) get reflected in
	 * the device-specific structs..
	 */
	if (pcscsi_request_init(ccb_p) != DDI_SUCCESS)	{

		/*
		 * If we've just allocated the resources, free them.
		 * (The upper layer may pass a pointer (scsi_pkt_p) to
		 * previously allocated resources - in which case we
		 * cannot free them here).
		 */
		if (request_allocated)  {
			pcscsi_tran_destroy_pkt(
					&(CCB2TRAN(ccb_p)->tran_sd->sd_address),
					CCB2PKT(ccb_p));
		}

		return ((struct scsi_pkt *)NULL);	/* Failure */
	}



	/*
	 * Set up and allocate data transfer (DMA) resources (if requested).
	 */
	if (bp != NULL) {	/* then we're to set up DMA resources */


		/*
		 * There are four cases here.
		 *	1) Unused pkt, new bp.
		 *	2) Reused pkt, new bp
		 *	3) Reused pkt, old bp (as a retry, or continuance of a
		 *		transfer which couldn't be completed last time
		 *		(possibly due to a DDI_DMA_PARTIAL situation)).
		 *	4) Reused pkt, old bp BUT a different operation has
		 *		been hacked into the cdb.  This happens
		 *		in scdk_open and sctp_open.
		 *
		 * For cases 1), 2),  and 3), we need to reinitialze DMA
		 * resources.  Otherwise we just
		 *
		 * Currently, the only safe way to test for this is to test
		 * for case 4, and if not case 4 initialize the resources.
		 */
		if (scsi_pkt_p != 0 && cmdlen == 0) {
			new_transfer = FALSE;
		} else {
			new_transfer = TRUE;
		}


		/*
		 * Set up everything necessary for a data transfer.
		 */
		if (pcscsi_xfer_setup(ccb_p, bp, flags, callback, new_transfer)
						!= DDI_SUCCESS) {

			if (request_allocated)  {
				pcscsi_tran_destroy_pkt(
					&(CCB2TRAN(ccb_p)->tran_sd->sd_address),
					CCB2PKT(ccb_p));
			}

			return ((struct scsi_pkt *)NULL);	/* Failure */
		}

	} /* if (dma resources requested) */


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRAN_INIT_PKT, DBG_RESULTS,
		"pcscsi_tran_init_pkt: Success!\n");
#endif	/* PCSCSI_DEBUG */


	return (scsi_pkt_p);	/* Success */
}


/* -------------------------------------------------------------------------- */
/*ARGSUSED*/
/*
 * Free the DMA resources associated with a SCSI packet (if any).
 */
void
pcscsi_tran_dmafree(
	struct scsi_address		 *ap,
	register struct scsi_pkt	 *scsi_pkt_p)
{
	register struct	pcscsi_ccb	 *ccb_p;


	/*
	 * This entry point will always have a scsi_pkt, so this will work.
	 */
	ccb_p = PKT2CCB(scsi_pkt_p);

	/*
	 * If there's any data transfer resources to free, free them.
	 */
	pcscsi_dma_structs_free(ccb_p);

}


/* -------------------------------------------------------------------------- */
/*ARGSUSED*/
/*
 * Force the device's and memory's view of the data to be consistent.
 */
static void
pcscsi_tran_sync_pkt(
	struct scsi_address		 *ap,
	register struct scsi_pkt	 *scsi_pkt_p)
{
	register int			i;
	register struct	pcscsi_ccb	 *ccb_p;


	ccb_p = PKT2CCB(scsi_pkt_p);

	if (ccb_p->ccb_dma_p->dma_handle) {	/* Sync only if data transfer */

		i = ddi_dma_sync(ccb_p->ccb_dma_p->dma_handle, 0, 0,
			(ccb_p->ccb_dma_p->dma_buf_p->b_flags & B_READ) ?
			DDI_DMA_SYNC_FORCPU : DDI_DMA_SYNC_FORDEV);

		if (i != DDI_SUCCESS) {
			cmn_err(CE_WARN, "pcscsi_tran_sync_pkt: "
					"Sync pkt failed\n");
		}
	}

#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_PKT_COMPLETION, DBG_RESULTS,
		"pcscsi_tran_sync_pkt: Success!\n");
#endif	/* PCSCSI_DEBUG */

}


/* -------------------------------------------------------------------------- */
/*
 * Dellocate a SCSI packet, and any driver- or device-specific structs that
 * may be connected with it.
 */
static void
pcscsi_tran_destroy_pkt(
	struct scsi_address	 *ap,
	struct scsi_pkt		 *scsi_pkt_p)
{
	struct pcscsi_ccb	 *ccb_p;


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_PKT_COMPLETION, DBG_ENTRY,
		"pcscsi_tran_destroy_pkt: Entry\n");
#endif	/* PCSCSI_DEBUG */


	/* Order is significant here. */


	/* Free all the other (internal) structs associated with a request. */
	ccb_p = PKT2CCB(scsi_pkt_p);
	pcscsi_hba_request_free(ccb_p);


	/* Free the scsi_pkt.  */
	if (scsi_pkt_p != (struct scsi_pkt *)NULL)	{
		scsi_hba_pkt_free(ap, scsi_pkt_p);
	}

}



/* ========================================================================== */
/*
 * Command-Transport entry point.
 */

/* -------------------------------------------------------------------------- */
/*
 * Start the transfer described by the SCSI packet.
 */
static int
pcscsi_transport(
	struct scsi_address		 *ap,
	register struct scsi_pkt	 *scsi_pkt_p)
{
	register struct pcscsi_blk	 *pcscsi_blk_p;
	register struct	pcscsi_ccb	 *ccb_p;
	int				status;


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_ENTRY, "pcscsi_transport: Entry\n");
	pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_transport: ap:%x  scsi_pkt:%x\n", ap, scsi_pkt_p));
#endif	/* PCSCSI_DEBUG */


	pcscsi_blk_p = PKT2BLK(scsi_pkt_p);
	ccb_p = PKT2CCB(scsi_pkt_p);


	/*
	 * Make sure we can safely access driver data structs and the h/w.
	 * Note that if we're polling, the mutex is held, effectively
	 * blocking any further transfers until polling finishes.
	 */
	mutex_enter(&pcscsi_blk_p->pb_core_mutex);


	/*
	 * Send the request to the HBA.  (Device-specific).
	 */
	if (!(scsi_pkt_p->pkt_flags & FLAG_NOINTR))	{

		/*
		 * Not a polled request.
		 * Try to send it once only.
		 */
		status = pcscsi_send(pcscsi_blk_p, ccb_p, NORMAL_RETRY);

		if (status != DDI_SUCCESS) {

#ifdef UNIT_QUEUE_SIZE
			/*
			 * Try to queue the request for later execution.
			 */
			if (pcscsi_queue_request(ccb_p) != TRUE)  {

				/* Queue full; return busy.	 */
				mutex_exit(&pcscsi_blk_p->pb_core_mutex);
				return (TRAN_BUSY);   /* Can't start it now. */
			}

#ifdef	PCSCSI_DEBUG
			pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
				"pcscsi_transport: ccb queued.\n");
#endif	/* PCSCSI_DEBUG */




#else	/* UNIT_QUEUE_SIZE	 */

			mutex_exit(&pcscsi_blk_p->pb_core_mutex);
			return (TRAN_BUSY);	/* Can't start it now. */

#endif	/* UNIT_QUEUE_SIZE	 */


		}	/* End if pcscsi_send failed */

	} else {

		/*
		 * Polled request.
		 * Keep trying to start it until we've exceeded our retry
		 * timeout.
		 * We can't return till it's done, one way or the other.
		 */
		status = pcscsi_send(pcscsi_blk_p, ccb_p, POLLED_RETRY_TIMEOUT);

		if (status != DDI_SUCCESS) {

			mutex_exit(&pcscsi_blk_p->pb_core_mutex);
			return (TRAN_BUSY);	/* Can't start it now. */
		}


		/*
		 * Go poll for completion.
		 * Note we don't return until the command completes.
		 */
		if (scsi_pkt_p->pkt_flags & FLAG_NOINTR)	{
			pcscsi_blk_p->pb_polling_ccb = ccb_p;
			pcscsi_pollret(pcscsi_blk_p, scsi_pkt_p);
		}

	}


	/*
	 * Done with the data structs and h/w - let others use them.
	 */
	mutex_exit(&pcscsi_blk_p->pb_core_mutex);


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
		"pcscsi_transport: Transport accepted.\n");
#endif	/* PCSCSI_DEBUG */


	return (TRAN_ACCEPT);	/* Transport initiated.	 */
}


/* ========================================================================== */
/*
 * Capability Management Entry Points.
 */

/* -------------------------------------------------------------------------- */
/*
 * Get the current value of a driver property.
 */
static int
pcscsi_getcap(
	struct scsi_address	 *ap,
	char			 *cap,
	int			tgtonly)
{
	int			ckey;
	int			total_sectors, h, s;


	/*
	 * Go see if the requested capability is even defined for this driver.
	 */
	if (pcscsi_capchk(cap, tgtonly, &ckey) != TRUE)  {
		return (UNDEFINED);
	}



	switch (ckey) {

	case SCSI_CAP_GEOMETRY:

		total_sectors =
			(ADDR2UNIT(ap))->pu_total_sectors;
		if (total_sectors <= 0)
			break;

		/*
		 * Copy the parameter banding scheme used in the SCSI BIOS.
		 * All drives will be parameterized the same via the ROM
		 * and via UNIX.
		 *   Upto 1G (200000 blocks) use   64 x 32
		 *   Over 1G (800001 blocks) use  255 x 63
		 */
		if (total_sectors > 0x200000) {
			h = 255;
			s = 63;
		} else {
			h = 64;
			s = 32;
		}
		return (HBA_SETGEOM(h, s));


	case SCSI_CAP_ARQ:
#ifdef ARQ
		if (tgtonly) {
			return ((ADDR2UNIT(ap))->pu_arq);
		} else {
			return ((ADDR2BLK(ap))->pb_arq_enabled);
		}

#else	/* ARQ	 */

		return (FALSE);	/* ARQ not supported */

#endif	/* ARQ	 */


	default:
		break;
	}


	return (UNDEFINED);
}


/* -------------------------------------------------------------------------- */
/*
 * Set the value of a driver property.
 */
static int
pcscsi_setcap(
	struct scsi_address	 *ap,
	char			 *cap,
	int			value,
	int			tgtonly)
{
	int			ckey;
	int			status;


	/*
	 * Go see if the requested capability is even defined for this driver.
	 */
	if (pcscsi_capchk(cap, tgtonly, &ckey) != TRUE) {
		return (UNDEFINED);
	}


	status = FALSE;

	switch (ckey) {

	case SCSI_CAP_SECTOR_SIZE:
		(ADDR2UNIT(ap))->pu_lim.dlim_granular
			= (u_int)value;
		status = TRUE;
		break;

	case SCSI_CAP_TOTAL_SECTORS:
		(ADDR2UNIT(ap))->pu_total_sectors = value;
		status = TRUE;
		break;

	case SCSI_CAP_ARQ:
#ifdef ARQ
		if (tgtonly) {
			(ADDR2UNIT(ap))->pu_arq = (u_int)value;
			status = TRUE;
		} else {
			(ADDR2BLK(ap))->pb_arq_enabled = (u_int)value;
			status = TRUE;
		}

#else   /* ARQ  */

		status = FALSE;		/* Don't support ARQ.	 */

#endif  /* ARQ  */

		break;

	case SCSI_CAP_GEOMETRY:

	default:
		break;

	}  /* end switch (ckey)	 */


	return (status);
}


/* ========================================================================== */
/*
 * Target Device Management Entry Points
 */

/* -------------------------------------------------------------------------- */
/*
 * Abort a specific command on a target device.
 * NOTE: The ccb and srb generated here may or may not have a scsi_pkt
 * associated with them.
 * Be careful in the request completion stuff to NOT indiscriminitely access
 * the scsi_pkt OR scsi_cmd.
 */
static int
pcscsi_abort(
	struct scsi_address	 *ap,
	struct scsi_pkt		 *scsi_pkt_p)
{
	struct pcscsi_blk	 *pcscsi_blk_p;
	struct pcscsi_ccb	 *ccb_p;
	SCSI_REQUEST_BLOCK	 *srb_p;
	int			status;

#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_ABORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_abort: Entry: pkt_p:%x scsi_address:%x\n",
				scsi_pkt_p, ap));
#endif

	pcscsi_blk_p = ADDR2BLK(ap);


	/*
	 * The only way to communicate with the core is through an SRB struct.
	 * The only way to abort anything is to make an SRB, set
	 * srb->Function = SRB_FUNCTION_ABORT_COMMAND,
	 * set the bus/target/lun/queue-tag,
	 * and send it to the core (next time the core's ready).
	 */

	/*
	 * Allocate a new ccb for the abort.
	 */
	status = pcscsi_hba_request_alloc(ap, HBA_MAX_CDB_LEN, MAX_SCB_LEN,
					&ccb_p, NULL_FUNC);
	if (status != DDI_SUCCESS)  {

		cmn_err(CE_WARN, "pcscsi_abort: ccb alloc failed\n");
		return (0);	/* Failure */
	}
	srb_p = (SCSI_REQUEST_BLOCK *)ccb_p->ccb_hw_request_p;


	/*
	 * Fill in the relvant parts of the SRB (there aren't many).
	 */
	srb_p->Function = SRB_FUNCTION_ABORT_COMMAND;
	srb_p->SrbFlags =	SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
				SRB_FLAGS_DISABLE_AUTOSENSE;
	srb_p->PathId = (UCHAR)0; /* never changes; mult. buses not supported */


	/*
	 * Abort a specific request, or all outstanding requests on this
	 * LUN.
	 * Note that as this driver is currently dsngl, AND tag queuing is not
	 * yet implemented, there will only ever be one request outstanding
	 * on any LUN.
	 */
	if (scsi_pkt_p != NULL) {

		/* Just abort the specific packet *scsi_pkt_p.	 */


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_ABORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"pcscsi_abort: Abort scsi_pkt:%x\n", scsi_pkt_p));
#endif


		srb_p->TargetId	= (UCHAR) (scsi_pkt_p->pkt_address.a_target);
		srb_p->Lun	= (UCHAR) (scsi_pkt_p->pkt_address.a_lun);

	} else {


		/*
		 * Abort all outstanding reqeusts to the target indicated
		 * by *ap.
		 */


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_ABORT, DBG_RESULTS,
			"pcscsi_abort: Abort all\n");
#endif


		srb_p->TargetId = ap->a_target;
		srb_p->Lun	= ap->a_lun;
	}


	srb_p->QueueTag = 0;	/* Tag Queuing not supported yet */

	/* Point OriginalRequest to the ccb (back ptr)	 */
	srb_p->OriginalRequest		= (PVOID)ccb_p;


	/*
	 * Make sure we can send a request now.
	 */
	mutex_enter(&pcscsi_blk_p->pb_core_mutex);


	/*
	 * Send the request to the HBA.
	 */
	if (pcscsi_send(pcscsi_blk_p, ccb_p, ABORT_RETRY_TIMEOUT)
		!= DDI_SUCCESS) {

		cmn_err(CE_WARN, "pcscsi_abort: Core busy, abort rejected\n");
		mutex_exit(&pcscsi_blk_p->pb_core_mutex);
		return (0);	/* Failure */


		/*
		 * Note that this could be better.
		 * We could queue up unsubmitted abort requests in the unit
		 * struct, and modify pcscsi_start_next_request to submit any
		 * stacked abort (or reset) requests before sending any
		 * 'normal' requests.
		 */
	}

	mutex_exit(&pcscsi_blk_p->pb_core_mutex);

	return (1);		/* Success */

}


/* -------------------------------------------------------------------------- */
/*
 * Reset the scsi bus, or just one target device.
 * returns 0 on failure, 1 on success.
 *
 * Note that as this driver is currently dsngl, AND tag queuing is not
 * yet implemented, there will only ever be one request outstanding
 * on any LUN.
 */
static int
pcscsi_reset(
	struct scsi_address	 *ap,
	int			level)
{
	struct pcscsi_blk	 *pcscsi_blk_p;
	struct pcscsi_ccb	 *ccb_p;
	SCSI_REQUEST_BLOCK	 *srb_p;
	int			status;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_RESET, DBG_ENTRY, sprintf(pcscsig_dbgmsg,
		"pcscsi_reset: Entry: scsi_address:%x\n", ap));
#endif


	pcscsi_blk_p = ADDR2BLK(ap);

	/*
	 * The only way to communicate with the core is through an SRB struct.
	 * The only way to reset anything is to make an SRB, set
	 * srb->Function appropriately,
	 * set the bus/target/lun/queue-tag,
	 * and send it to the core (next time the core's ready).
	 */

	/* Allocate a new ccb for the reset.  */
	status = pcscsi_hba_request_alloc(ap, HBA_MAX_CDB_LEN, MAX_SCB_LEN,
					&ccb_p, NULL_FUNC);
	if (status != DDI_SUCCESS)  {

		cmn_err(CE_WARN, "pcscsi_reset: ccb alloc failed\n");
		return (0);	/* Failure */
	}
	srb_p = (SCSI_REQUEST_BLOCK *)ccb_p->ccb_hw_request_p;


	/*
	 * Fill in the relvant parts of the SRB (there aren't many).
	 */
	srb_p->SrbFlags =	SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
				SRB_FLAGS_DISABLE_AUTOSENSE;
	srb_p->PathId = (UCHAR)0; /* never changes; mult. buses not supported */

	srb_p->TargetId = ap->a_target;
	srb_p->Lun	= ap->a_lun;
	srb_p->QueueTag = 0;	/* Tag Queuing not supported yet */

	/* Point OriginalRequest to the ccb (back ptr)	 */
	srb_p->OriginalRequest		= (PVOID)ccb_p;


	/*
	 * Set the function (Reset device/Reset bus).
	 */
	switch (level) {

	case RESET_ALL:


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_ABORT, DBG_RESULTS,
			"pcscsi_reset: Reset all.\n");
#endif


		srb_p->Function = SRB_FUNCTION_RESET_BUS;
		/* Note: The bus settling delay is handled by the core code.  */
		break;


	case RESET_TARGET:


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_ABORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"pcscsi_reset: Reset target/lun: %d/%d\n",
			srb_p->TargetId, srb_p->Lun));
#endif


		srb_p->Function = SRB_FUNCTION_RESET_DEVICE;
		break;


	default:
		cmn_err(CE_PANIC, "pcscsi_reset: bad level %d\n", level);
		break;

	}  /* end switch (level) */


	/*
	 * Make sure we can send a request now.
	 */
	mutex_enter(&pcscsi_blk_p->pb_core_mutex);


	/*
	 * Send the request to the HBA.
	 */
	if (pcscsi_send(pcscsi_blk_p, ccb_p, RESET_RETRY_TIMEOUT)
		!= DDI_SUCCESS) {

		cmn_err(CE_WARN, "pcscsi_reset: Core busy, reset rejected\n");
		mutex_exit(&pcscsi_blk_p->pb_core_mutex);
		return (0);	/* Failure */

		/*
		 * Note that this could be better.
		 * We could queue up unsubmitted abort requests in the unit
		 * struct, and modify pcscsi_start_next_request to submit any
		 * stacked abort (or reset) requests before sending any
		 * 'normal' requests.
		 */
	}

	mutex_exit(&pcscsi_blk_p->pb_core_mutex);

	return (1);		/* Success */

}


/* ========================================================================== */
/*
 * Interrupt service routines
 */

/* -------------------------------------------------------------------------- */
/*
 * Dummy Autovector Interrupt Entry Point.
 * Dummy return to be used before mutexes have been initialized, to
 * deal with interrupts from drivers sharing our IRQ line.
 */
static u_int
pcscsi_dummy_intr(
	caddr_t		arg)
{
	return (DDI_INTR_UNCLAIMED);	/* Do nothing. */
}


/* -------------------------------------------------------------------------- */
/*
 * Interrupt service routine.
 *
 * Loop, calling the AMD core ISR until it says it didn't claim an interrupt.
 */
static u_int
pcscsi_intr(
	caddr_t		arg)
{
	register struct	pcscsi_blk *pcscsi_blk_p;
	boolean_t	AMDReturnStatus;
	int		return_status;
	int		rundown_status;


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_INTR, DBG_ENTRY, "pcscsi_intr: Entry.\n");
#endif	/* PCSCSI_DEBUG */


	pcscsi_blk_p = PCSCSI_BLKP(arg);
	return_status = DDI_INTR_UNCLAIMED;


	/*
	 * Grab the core mutex before going into any AMD core routine.
	 */
	mutex_enter(&pcscsi_blk_p->pb_core_mutex);

	do {

		/*
		 * Call AMD's Interrupt Service routine.
		 *
		 * NOTE this can ultimately call ScsiPortNotification, hence
		 * pcscsi_request_complete and/or pcscsi_start_next_request.
		 * All will be called with the mutex HELD from here.
		 */
		AMDReturnStatus =
			(*pcscsi_blk_p->pb_AMDInterruptServiceRoutine)(
				pcscsi_blk_p->pb_core_DeviceExtension_p);

		if (AMDReturnStatus == TRUE)	{
			return_status = DDI_INTR_CLAIMED;
		}


#ifdef	PCSCSI_DEBUG
		if (AMDReturnStatus == TRUE)	{
			pcscsi_debug(DBG_INTR, DBG_RESULTS,
				"pcscsi_intr: Interrupt claimed.\n");
		} else {
			pcscsi_debug(DBG_INTR, DBG_RESULTS,
				"pcscsi_intr: Interrupt unclaimed.\n");
		}
#endif	/* PCSCSI_DEBUG */


	} while (AMDReturnStatus == TRUE);	/* TRUE = DDI_INTR_CLAIMED */


	mutex_exit(&pcscsi_blk_p->pb_core_mutex);


#ifdef	SYNCHRONOUS_TGT_COMPLETIONS
	/*
	 * Now run down any queued target completion routines.
	 * (Mutex will be acquired and released multiple times).
	 *
	 * This is currently commented out, as target completion routines
	 * are run via the softintr mechanism.
	 * Comment out the call to ddi_trigger_softintr (elsewhere),
	 * and uncomment this line to switch to synchronous rundown of the
	 * target completion routines.
	 *
	 */
	 rundown_status = pcscsi_run_tgt_compl((caddr_t)pcscsi_blk_p);

#endif	/* SYNCHRONOUS_TGT_COMPLETIONS	*/


	return (return_status);
}


/* ========================================================================== */
/*
 * Called when the system is being halted to perform device-specific
 * actions (such as flushing onboard caches, disabling interrupts,
 * resetting the device).
 *
 * Note that in this routine we *can't* block at all, not even on mutexes.
 */
static int
pcscsi_system_halt(dev_info_t *dip, ddi_reset_cmd_t cmd)
{

	/*
	 * Not called for 2.4; Noop for 2.5.
	 * (May need to reset the chip for Compaq systems...)
	 */


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(0xffff, DBG_ENTRY, /* Display if *any* debug defined */
		"pcscsi_system_halt: Entry.\n");
#endif	/* PCSCSI_DEBUG */


	return (DDI_SUCCESS);
}


/* ========================================================================== */
/*
 * Routines used internally.
 */


#if defined PCI_DDI_EMULATION
/* -------------------------------------------------------------------------- */
/*
 * Do the necessary magic to tell the framework which IRQ we're using.
 */
static int
pcscsi_xlate_vec(
	struct pcscsi_blk	 *pcscsi_blk_p,
	u_int			 *intr_idx)
{
	register int		irq;
	register int		i;
	int			intrspec[3];


	irq = pcscsi_blk_p->pb_intr;
	if (irq < 3 || irq > 15) {

		cmn_err(CE_WARN,
			"pcscsi_xlate_vec: Illegal IRQ for device:%d", irq);

		return (DDI_FAILURE);
	}


	/*
	 * Create an interrupt spec using default interrupt priority level.
	 * Call the upper layer (via ddi_ctlops) to actually build it.
	 */
	intrspec[0] = 2;
	intrspec[1] = 5;
	intrspec[2] = irq;

	if (ddi_ctlops(pcscsi_blk_p->pb_dip,
				pcscsi_blk_p->pb_dip,
				DDI_CTLOPS_XLATE_INTRS,
				(caddr_t)intrspec,
				ddi_get_parent_data(pcscsi_blk_p->pb_dip))
		!= DDI_SUCCESS) {

		cmn_err(CE_WARN,
			"pcscsi_xlate_vec: interrupt prop create failed");

		return (DDI_FAILURE);
	}


	 *intr_idx = 0;	/* 'First' interrupt spec (and only one for PCI).    */

	return (DDI_SUCCESS);
}


#endif	/* PCI_DDI_EMULATION */


/* -------------------------------------------------------------------------- */
/*
 * Get the value of a driver property.
 */
static int
pcscsi_capchk(
	char		 *cap,
	int		tgtonly,
	int		 *cidxp)
{
	register int	cidx;


	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *) 0)	{
		return (FALSE);
	}

	 *cidxp = scsi_hba_lookup_capstr(cap);

	return (TRUE);
}


/* -------------------------------------------------------------------------- */
/*
 * Send an SRB down to the AMD core code to start execution.
 * This routine mainly handles any necessary waits or retries necessary to
 * get the request started - if requested and necessary.
 *
 * Mutex is *held* when this function is called.
 */
static int
pcscsi_send(
	struct pcscsi_blk	 *pcscsi_blk_p,
	struct pcscsi_ccb	 *ccb_p,
	long			retry_timeout)
{
	int			status;
	clock_t			current_ticks;
	clock_t			timeout_time;


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_ENTRY, "pcscsi_send: Entry.\n");
#endif	/* PCSCSI_DEBUG */


	status = pcscsi_send_to_core(pcscsi_blk_p, ccb_p);
	if (status == DDI_SUCCESS)  {
		return (status);
	}


	if (retry_timeout == NO_RETRY)  {
		return (status);
	}


	/*
	 * The following mechanism is to ensure that 'priority' requests
	 * (those that either cannot be queued (as polled requests) or must
	 * be sent 'next' (as abort and reset requests) can get through to
	 * the core while the driver is under heavy I/O load.
	 *
	 * Essentially we do a busy-wait for the specified time, trying to
	 * send down the request whenever the core is available.
	 *
	 * without this, such requests would probably get kicked back with
	 * TRAN_BUSY under heavy load, thus 'not working'.
	 */

	status = drv_getparm(LBOLT, (unsigned long *)&current_ticks);
	if (status != 0)  {
		cmn_err(CE_WARN, "pcscsi: drv_getparm failed.\n");
		return (DDI_FAILURE);
	}
	timeout_time = current_ticks + drv_usectohz(retry_timeout);

	for (;;)  {

		/*
		 * Indicate that there is (another) request waiting to start.
		 * (This tells pcscsi_start_next_request not to dequeue
		 * and start the next command, but to just mark the core as
		 * available and signal us, and return.)
		 */
		pcscsi_blk_p->pb_send_requests_waiting++;


#ifdef	PCSCSI_DEBUG
		pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"pcscsi_send: (Re)starting cv_timedwait:  "
			"ccb_p:%x  requests_waiting:%x\n",
			ccb_p,
			pcscsi_blk_p->pb_send_requests_waiting));
#endif	/* PCSCSI_DEBUG */


		status = cv_timedwait((&pcscsi_blk_p->pb_wait_for_core_ready),
					&(pcscsi_blk_p->pb_core_mutex),
					timeout_time);


#ifdef	PCSCSI_DEBUG
		pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
			"pcscsi_send: cv_timedwait returned:  "
			"ccb_p:%x  requests_waiting:%x\n",
			ccb_p,
			pcscsi_blk_p->pb_send_requests_waiting));
#endif	/* PCSCSI_DEBUG */


		if (status <= 0)  {	/* Timer expired; give up.  */


#ifdef	PCSCSI_DEBUG
			pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
				sprintf(pcscsig_dbgmsg,
				"pcscsi_send: cv_timedwait timed out:  "
				"ccb_p:%x  requests_waiting:%x\n",
				ccb_p,
				pcscsi_blk_p->pb_send_requests_waiting));
			ASSERT(status != 0);
#endif	/* PCSCSI_DEBUG */


#ifdef UNIT_QUEUE_SIZE
			/*
			 * Resume the 'normal' processing stream.
			 * If the core is now available,
			 * dequeue the next queued request and start it.
			 * (If the core isn't available right now, it's
			 * starting a request and so will eventually call
			 * pcscsi_start_next_request).
			 */
			if (pcscsi_blk_p->pb_core_ready_for_next == TRUE)  {
				pcscsi_start_next_request(pcscsi_blk_p,
					NO_TARGET, NO_LUN);
			}
#endif	/* UNIT_QUEUE_SIZE	*/


			return (DDI_FAILURE);
		}


		/*
		 * Otherwise, cv_timedwait was signalled by cv_signal;
		 * core is (or *was*) available.
		 * Note that we cannot guarantee that the core is *now*
		 * available; on an MP system, somebody else may have
		 * sneaked in and grabbed the core before this thread got
		 * reactivated.
		 *
		 * Try to start the request again now.
		 */
		status = pcscsi_send_to_core(pcscsi_blk_p, ccb_p);
		if (status == DDI_SUCCESS)  {
			return (status);
		}


		/*
		 * Couldn't start it.
		 * If we have time left to try again, do so.  Else give up.
		 */
		status = drv_getparm(LBOLT, (unsigned long *)&current_ticks);
		if (status != 0)  {
			cmn_err(CE_WARN, "pcscsi: drv_getparm failed.\n");
			return (DDI_FAILURE);
		}

		if (current_ticks >= timeout_time)  {


#ifdef	PCSCSI_DEBUG
			pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
				sprintf(pcscsig_dbgmsg,
				"pcscsi_send: No time left to retry:  "
				"ccb_p:%x  requests_waiting:%x\n",
				ccb_p,
				pcscsi_blk_p->pb_send_requests_waiting));
#endif	/* PCSCSI_DEBUG */


#ifdef UNIT_QUEUE_SIZE
			/*
			 * Resume the 'normal' processing stream.
			 * If the core is now available,
			 * dequeue the next queued request and start it.
			 * (If the core isn't available right now, it's
			 * starting a request and so will eventually call
			 * pcscsi_start_next_request).
			 */
			if (pcscsi_blk_p->pb_core_ready_for_next == TRUE)  {
				pcscsi_start_next_request(pcscsi_blk_p,
					NO_TARGET, NO_LUN);
			}
#endif	/* UNIT_QUEUE_SIZE	*/

			return (DDI_FAILURE);
		}
	}
}


/* -------------------------------------------------------------------------- */
/*
 * Send an SRB down to the AMD core code to start execution.
 * This routine does NO waits or retries.
 *
 * Mutex is *held* when this function is called.
 */
static int
pcscsi_send_to_core(
	struct pcscsi_blk	 *pcscsi_blk_p,
	struct pcscsi_ccb	 *ccb_p)
{
	boolean_t		status;
	int			targ;
	int			lun;
	int			q_tag = 0;   /* Tag queing not implemnented */


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_ENTRY, "pcscsi_send_to_core: Entry.\n");
#endif	/* PCSCSI_DEBUG */


	/*
	 * See if the AMD core code is ready to start a new request.
	 */
	if (pcscsi_blk_p->pb_core_ready_for_next != TRUE)  {


#ifdef	PCSCSI_DEBUG
		pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
			"pcscsi_send_to_core: Core busy.\n");
#endif	/* PCSCSI_DEBUG */


		return (DDI_FAILURE);
	}


	targ = ((SCSI_REQUEST_BLOCK *)(ccb_p->ccb_hw_request_p))->TargetId;
	lun = ((SCSI_REQUEST_BLOCK *)(ccb_p->ccb_hw_request_p))->Lun;


	/*
	 * If there's already a command active for this target/lun, don't
	 * start a new one.
	 *
	 * If it's not a command (and hence a message), pass it through.
	 * As the only possible messages come from the abort and reset
	 * entry points, we know we should pass these through even if there
	 * is a command active on the target/lun.
	 *
	 * As we haven't implemented tagged queueing, there can be exactly
	 * one command outstanding on a given target/lun at a given time.
	 * The core does not guarantee that; we must.
	 * DSNGL should guarantee this for disks, but we have to guarantee
	 * it for -all- targets.
	 */
	/* Implement a for loop here, to find an available queue_tag. */
	if (pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccbs[q_tag]
		!= NULL)  {

#ifdef	PCSCSI_DEBUG
		pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
			"pcscsi_send_to_core: Target/lun currently busy.\n");
#endif	/* PCSCSI_DEBUG */


		return (DDI_FAILURE);
	}


	/*
	 * Send the request (SRB) down to the core.
	 *
	 * Wnen this returns, we're done with the core.
	 * NOTE this does *NOT* mean that the core can now accept another
	 * request!
	 * The core can only accept another request when pb_core_ready_for_next
	 * is TRUE.  This is a 'feature' of the core code.  Don't call us,
	 * we'll call you (via pcscsi_start_next_request).
	 */
	pcscsi_blk_p->pb_core_ready_for_next = FALSE;

	status = (*pcscsi_blk_p->pb_AMDStartIo)(
			pcscsi_blk_p->pb_core_DeviceExtension_p,
			ccb_p->ccb_hw_request_p); /* SRB */

	/*
	 * The status from AMDStartIo is -currently- hardwired to TRUE,
	 * but what the heck.
	 */
	if (status != TRUE) {

		/* *Assume* Core is still available.  */
		pcscsi_blk_p->pb_core_ready_for_next = TRUE;

		return (DDI_FAILURE);	/* Can't start it now */
	}


#ifdef	PCSCSI_DEBUG
		pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
			"pcscsi_send_to_core: Request started in core.\n");
#endif	/* PCSCSI_DEBUG */


	/*
	 * Save the ccb in the per-unit array of active requests.
	 */
	pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccbs[q_tag] =
		ccb_p;

	pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccb_cnt ++;


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsisendtocor: "
	"targ:%d lun:%d qtag:%d unit_ptr:%x ccb_p:%x actvSRBs:%d\n",
			targ, lun, q_tag,
			pcscsi_blk_p->pb_unit_structs[targ][lun],
			ccb_p,
		pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccb_cnt));

	if ((pcscsig_debug_funcs & DBG_TRANSPORT) &&
	(pcscsig_debug_gran  & DBG_VERBOSE))
		pcscsi_dump_srb(ccb_p->ccb_hw_request_p);
#endif	/* PCSCSI_DEBUG */


	return (DDI_SUCCESS);
}


/* -------------------------------------------------------------------------- */
/*
 * Poll for status of a command sent to hba without interrupts.
 * Called with the core mutex held.
 */
static void
pcscsi_pollret(
	register struct pcscsi_blk	 *pcscsi_blk_p,
	struct scsi_pkt			 *poll_scsi_pkt_p)
{


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_ENTRY, "pcscsi_pollret: Entry.\n");
#endif	/* PCSCSI_DEBUG */


	/*
	 * Loop, calling AMD's ISR, until our commamd completes.
	 */
	do {

		/*
		 * Call AMD's ISR, and let it figure out what's happened
		 * (if anything)
		 *
		 * If no interrupt flag (in h/w) was detected (FALSE returned),
		 * just loop.
		 *
		 * If a request completes, the ISR calls ScsiPortNotification
		 * directly, which will ultimately complete the request
		 * (and clear pb_polling_ccb) before returning (to here).
		 *
		 * As we're holding the mutex, nobody else can do anything
		 * till we're done here.
		 */
		for (;;)	{

			if (
			(*pcscsi_blk_p->pb_AMDInterruptServiceRoutine)
				(pcscsi_blk_p->pb_core_DeviceExtension_p))  {
					break; /* TRUE = Interrupt handled */
			}

			/*
			 * Really should implement some kind of polled-request
			 * timeout here.
			 */

		}

	} while (pcscsi_blk_p->pb_polling_ccb != NULL);


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS, "pcscsi_pollret: Exit.\n");
#endif	/* PCSCSI_DEBUG */


}


/* -------------------------------------------------------------------------- */
/*
 * Check for/handle any errors conditions for the completed transport.
 * Set pkt_resid.
 */
static u_int
pcscsi_chkerr(
	struct pcscsi_blk	 *pcscsi_blk_p,
	register struct scsi_pkt *scsi_pkt_p)
{
	struct pcscsi_ccb	 *ccb_p;
	SCSI_REQUEST_BLOCK	 *srb_p;
	char	 *msgstr;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PKT_COMPLETION, DBG_ENTRY, "pcscsi_chkerr: Entry\n");
#endif	/* PCSCSI_DEBUG */


	ccb_p = PKT2CCB(scsi_pkt_p);
	srb_p = (SCSI_REQUEST_BLOCK *)ccb_p->ccb_hw_request_p;


	/*
	 * Set pkt_resid.
	 */
	if (ccb_p->ccb_dma_p != NULL) {

		/* This is a data-transfer cmd */

		scsi_pkt_p->pkt_resid =
			ccb_p->ccb_dma_p->dma_totxfer -		/* Requested */
			srb_p->DataTransferLength;		/* Actual */
	} else {
		/* Not a data transfer cmd */
		scsi_pkt_p->pkt_resid = 0;
	}



	/*
	 * Handle the most common case (success) immediately.
	 */
	if (
		(srb_p->SrbStatus == SRB_STATUS_SUCCESS)	&&
		(srb_p->ScsiStatus == STATUS_GOOD /* SCSISTAT_GOOD */))  {


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_PKT_COMPLETION, DBG_RESULTS,
			sprintf(pcscsig_dbgmsg,
			"pcscsi_chkerr: Ok :srb_p:%x Xfred:%dd",
			srb_p,
			srb_p->DataTransferLength));		/* Actual */

		if (PKT2CCB(scsi_pkt_p)->ccb_dma_p != NULL)  {

			pcscsi_debug(DBG_PKT_COMPLETION, DBG_RESULTS,
				sprintf(pcscsig_dbgmsg,
				"/Wanted:%dd &cdb:%x opcode:%x\n",
			PKT2CCB(scsi_pkt_p)->ccb_dma_p->dma_seg_xfer_cnt,
								/* Expected */
				&srb_p->Cdb,
				srb_p->Cdb[0]));
		} else {
			pcscsi_debug(DBG_PKT_COMPLETION, DBG_RESULTS, "\n");
		}

#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_CMPLT;
		/* scsi_pkt_p->scbp points to srb_p->ScsiStatus already.  */

		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_TARGET	|
					STATE_SENT_CMD		|
					STATE_XFERRED_DATA	|
					STATE_GOT_STATUS);
		return ((u_int)(srb_p->ScsiStatus));
	}



	/*
	 * (Otherwise...)  We have some sort of error.
	 */


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PKT_CHK_ERRS, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_chkerr: Err :srb_p:%x Xfred:%dd",
		srb_p,
		srb_p->DataTransferLength));		/* Actual */

	if (PKT2CCB(scsi_pkt_p)->ccb_dma_p != NULL)  {

		pcscsi_debug(DBG_PKT_CHK_ERRS, DBG_RESULTS,
			sprintf(pcscsig_dbgmsg,
			"/Wanted:%dd &cdb:%x opcode:%x\n",
		PKT2CCB(scsi_pkt_p)->ccb_dma_p->dma_seg_xfer_cnt,
							/* Expected */
			&srb_p->Cdb,
			srb_p->Cdb[0]));
	} else {
		pcscsi_debug(DBG_PKT_CHK_ERRS, DBG_RESULTS, "\n");
	}
	pcscsi_debug(DBG_PKT_CHK_ERRS, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"target:%x lun:%x",
		srb_p->TargetId, srb_p->Lun));
#endif	/* PCSCSI_DEBUG */



	/*
	 * Handle other SRB (controller) return statuses.
	 */
	switch (srb_p->SrbStatus)	{


	case  SRB_STATUS_DATA_OVERRUN:


#ifdef PCSCSI_DEBUG
		msgstr = "SRB_STATUS_DATA_OVERRUN";
#endif	/* PCSCSI_DEBUG */


		/*
		 * NOTE: The core code returns SRB_STATUS_DATA_OVERRUN
		 * whenever the requested and received transfer counts
		 * -differ-.
		 * This is the ONLY situation where 'overrun' is returned.
		 */
		if (scsi_pkt_p->pkt_resid < 0)	{	/* Overrun */

			cmn_err(CE_WARN, "pcscsi: "
				"Overrun: More data received than requested.");
			/* Just return it for now, see what happens */
		}

		if (scsi_pkt_p->pkt_resid > 0)	{	/* Underrun */

			/*
			 *Special case per Jeffco (Frits):
			 * A device can complete a transfer normally but NOT
			 * transfer the expected number of bytes on a SCSI
			 * Inquiry.
			 * Thus controller-status 'overrun' is returned fairly
			 * often to the upper layer.
			 *
			 * The AMD core code tries to deal with this:
			 * whenever they see
			 * amount-tranferred != amount-requested, they set
			 * SrbStatus=SRB_STATUS_DATA_OVERRUN, and set
			 * srb_p->DataTransferLength = (amount actually
			 * transferred).
			 *
			 * We need to accept this as a valid completion, but set
			 * pkt->resid properly.
			 * So we need to look into the cdb to see if this is an
			 * Inquiry.
			 */
			/*
			 * Per Bruce Adler, as used by AMD,this is not an error.
			 * *sigh* Need to look into this...
			 */

			/*
			 * Force success.
			 */
			scsi_pkt_p->pkt_reason = CMD_CMPLT;


#ifdef PCSCSI_DEBUG
/*		pcscsi_debug(DBG_PKT_CHK_ERRS, DBG_RESULTS,    */
/*		"pcscsi_chkerr: Overrun converted to success\n");   */
#endif	/* PCSCSI_DEBUG */


		}

		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_TARGET	|
					STATE_SENT_CMD		|
					STATE_XFERRED_DATA	|
					STATE_GOT_STATUS);
		break;



	case  SRB_STATUS_ABORTED:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_ABORTED";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_ABORTED;
		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_TARGET	|
					STATE_SENT_CMD		|
					STATE_GOT_STATUS);
		scsi_pkt_p->pkt_statistics |= STAT_ABORTED;
		break;



	case  SRB_STATUS_ABORT_FAILED:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_ABORT_FAILED";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_ABORT_FAIL;
		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_TARGET	|
					STATE_SENT_CMD);
		break;



	case  SRB_STATUS_MESSAGE_REJECTED:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_MESSAGE_REJECTED";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_TRAN_ERR;
		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_TARGET	|
					STATE_SENT_CMD		|
					STATE_GOT_STATUS);
		break;



	case  SRB_STATUS_BUS_RESET:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_BUS_RESET";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_RESET;
		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_TARGET	|
					STATE_SENT_CMD		|
					STATE_GOT_STATUS);
		scsi_pkt_p->pkt_statistics |= STAT_BUS_RESET;
		break;



	case  SRB_STATUS_UNEXPECTED_BUS_FREE:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_UNEXPECTED_BUS_FREE";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_UNX_BUS_FREE;
		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_TARGET	|
					STATE_SENT_CMD);
		break;



	case  SRB_STATUS_PHASE_SEQUENCE_FAILURE:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_PHASE_SEQUENCE_ERROR";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_TRAN_ERR;
		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS);
		break;



	case  SRB_STATUS_ERROR:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_ERROR";
#endif	/* PCSCSI_DEBUG */


		/*
		 * This srb status code is completely bogus.
		 * It is generated by the following code in the core:
		 *
		 * if (srb->ScsiStatus != SCSISTAT_GOOD) {
		 *	//
		 *	// Indicate an abnormal status code.
		 *	//
		 *	srb->SrbStatus = SRB_STATUS_ERROR;
		 *
		 * This is the only case where this error code is generated.
		 * Obviously this is not really an HBA or core layer error.
		 * Therefore we can safely ignore it.
		 *
		 * Set the HBA completion status to 'ok'.
		 */
		scsi_pkt_p->pkt_reason = CMD_CMPLT;

		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_TARGET	|
					STATE_SENT_CMD		|
					STATE_GOT_STATUS);

		/*
		 * The AMD core code uses this status as an unspecified
		 * catch-all status.
		 * It returns this when (no other specific failure occured and)
		 * ScsiStatus is anything other than SCSI_GOOD.
		 *
		 * Note that this includes SCSI_CHECK_CONDITION.
		 * In this case we need to return CMD_COMPLETE, as it's
		 * not an error.
		 *
		 * (Removed, as the above is the more general fix. Left here
		 * for documentation of the problem.)
		 *
		 * if (srb_p->ScsiStatus == STATUS_CHECK)	{
		 * 	scsi_pkt_p->pkt_reason = CMD_CMPLT;  * Force success *
		 * }
		 */

		break;



	case  SRB_STATUS_SELECTION_TIMEOUT:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_SELECTION_TIMEOUT";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_TRAN_ERR;
		scsi_pkt_p->pkt_state = (
					STATE_GOT_BUS		|
					STATE_GOT_STATUS);
		scsi_pkt_p->pkt_statistics |= STAT_TIMEOUT;
		break;



	case  SRB_STATUS_INVALID_REQUEST:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_INVALID_REQUEST";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_TRAN_ERR;
		scsi_pkt_p->pkt_state = (NULL);
		break;



	case  SRB_STATUS_BAD_FUNCTION:


#ifdef PCSCSI_DEBUG
			msgstr = "SRB_STATUS_BAD_FUNCTION";
#endif	/* PCSCSI_DEBUG */


		scsi_pkt_p->pkt_reason = CMD_TRAN_ERR;
		scsi_pkt_p->pkt_state = (NULL);
		break;

	/*
	 * Defined but not currently used in the core code...
	 * case  SRB_STATUS_PENDING:
	 * case  SRB_STATUS_BUSY:
	 * case  SRB_STATUS_INVALID_PATH_ID:
	 * case  SRB_STATUS_NO_DEVICE:
	 * case  SRB_STATUS_TIMEOUT:
	 * case  SRB_STATUS_COMMAND_TIMEOUT:
	 * case  SRB_STATUS_PARITY_ERROR:
	 * case  SRB_STATUS_REQUEST_SENSE_FAILED:
	 * case  SRB_STATUS_NO_HBA:
	 * case  SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
	 * case  SRB_STATUS_REQUEST_FLUSHED:
	 * case  SRB_STATUS_INVALID_LUN:
	 * case  SRB_STATUS_INVALID_TARGET_ID:
	 * case  SRB_STATUS_ERROR_RECOVERY:
	 */



	default:
		cmn_err(CE_PANIC, "pcscsi_chkerr: Unrecognized SRB status\n");
		break;

	}	/* End case SrbStatus */


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PKT_CHK_ERRS, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"    SRB status: %s  ", msgstr));
#endif	/* PCSCSI_DEBUG */



	/*
	 * Handle the SCSI return status.
	 */
#ifdef PCSCSI_DEBUG

	switch (srb_p->ScsiStatus)	{

	case  STATUS_GOOD		/* SCSISTAT_GOOD */:
		msgstr = "SCSISTAT_GOOD";
		break;
	case  STATUS_CHECK		/* SCSISTAT_CHECK_CONDITION */:
		msgstr = "SCSISTAT_CHECK_CONDITION";
		break;
	case  STATUS_MET		/* SCSISTAT_CONDITION_MET */:
		msgstr = "SCSISTAT_CONDITION_MET";
		break;
	case  STATUS_BUSY		/* SCSISTAT_BUSY */:
		msgstr = "SCSISTAT_BUSY";
		break;
	case  STATUS_INTERMEDIATE	/* SCSISTAT_INTERMEDIATE */:
		msgstr = "SCSISTAT_INTERMEDIATE";
		break;
	case  STATUS_INTERMEDIATE_MET	/* SCSISTAT_INTERMEDIATE_COND_MET */:
		msgstr = "SCSISTAT_INTERMEDIATE_COND_MET";
		break;
	case  STATUS_RESERVATION_CONFLICT /* SCSISTAT_RESERVATION_CONFLICT */:
		msgstr = "SCSISTAT_RESERVATION_CONFLICT";
		break;
	case  STATUS_TERMINATED		/* SCSISTAT_COMMAND_TERMINATED */:
		msgstr = "SCSISTAT_COMMAND_TERMINATED";
		break;
	case  STATUS_QFULL		/* SCSISTAT_QUEUE_FULL */:
		msgstr = "SCSISTAT_QUEUE_FULL";
		break;
	default:
		msgstr = "Unknown SCSISTAT";
		pcscsi_debug(DBG_PKT_CHK_ERRS, DBG_RESULTS,
			sprintf(pcscsig_dbgmsg,
			"SCSISTAT:%x\n", srb_p->ScsiStatus));
		break;

	}	/* End case ScsiStatus */


	pcscsi_debug(DBG_PKT_CHK_ERRS, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"SRB ScsiStatus: %s\n", msgstr));

#endif	/* PCSCSI_DEBUG */


	return ((u_int)(srb_p->ScsiStatus));
}


/* -------------------------------------------------------------------------- */
/*
 * Ensure that all the necessary and optional properties are set up.
 */
static int
pcscsi_init_properties(
	dev_info_t		 *dip,
	struct pcscsi_blk	 *pcscsi_blk_p)
{
	int	*discop;
	int	val;
	int	len;


	/*
	 * Ensure essential properties are defined.
	 */
	if (
		!pcscsi_get_property(dip, "flow_control",	"dsngl") ||
		!pcscsi_get_property(dip, "queue",		"qsort") ||
		!pcscsi_get_property(dip, "disk",		"scdk")	||
		!pcscsi_get_property(dip, "tape",		"sctp")) {
		return (DDI_FAILURE);
	}


	/*
	 * Determine which target ID the HBA should use.
	 */
	len = sizeof (val);
	if (HBA_INTPROP(dip, "scsi-initiator-id", &val, &len)
		== DDI_PROP_SUCCESS) {

		pcscsi_blk_p->pb_initiator_id = val;

	} else {

		/* default the initiator id to DEFAULT_INITIATOR_ID */
		pcscsi_blk_p->pb_initiator_id = DEFAULT_INITIATOR_ID;
	}


	/*
	 * See if the user defined "disable_compaq_specific" in the .conf file.
	 * If so turn off the Compaq-specific behavior in the core code.
	 * This is an override, in case the Compaq-specific stuff in the
	 * current or future core causes problems.
	 */
	pcscsi_blk_p->pb_disable_compaq_specific = FALSE;
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS,
		"disable_compaq_specific", (caddr_t)&val, &len)
			== DDI_PROP_SUCCESS) {

		kmem_free((caddr_t)val, len);   /* Free memory just allocated */

		pcscsi_blk_p->pb_disable_compaq_specific = TRUE;
	}


	return (DDI_SUCCESS);
}


/* -------------------------------------------------------------------------- */
/*
 * Try to get the value for a property from the framework.
 * If there isn't one, create it with the default value passed in.
 */
static boolean_t
pcscsi_get_property(
	dev_info_t	 *dip,
	caddr_t		propname,
	caddr_t		propdefault)
{
	caddr_t	val;
	int	len;


	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS, propname,
		(caddr_t)&val, &len) == DDI_PROP_SUCCESS) {

		kmem_free(val, len);
		return (TRUE);		/* Ok -Property already defined. */
	}

	if (ddi_prop_create(DDI_DEV_T_NONE, dip, 0, propname, propdefault,
				strlen(propdefault) + 1) == DDI_PROP_SUCCESS)  {
		return (TRUE);
	}


	cmn_err(CE_WARN, "pcscsi_get_property: property create failed %s=%s\n",
			propname, propdefault);

	return (FALSE);
}


/* -------------------------------------------------------------------------- */
/*
 * Set up everything necessary to make the core code ready to handle tranfers.
 * Called only from attach.
 * This function is called with NO mutex held.
 */
static int
pcscsi_device_setup(
	struct pcscsi_blk	 *pcscsi_blk_p)
{


	/*
	 * Call AMD's core code initialization routine.
	 *
	 * NOTE this ultimately calls the portability layer
	 * routine ScsiPortInitialize (in portability.c).
	 *
	 * Neither arg is used in any way by the core code;
	 * they are simply passed through the to ScsiPortInitialize
	 * routine (in portability.c).
	 */
	if (DriverEntry((PVOID)pcscsi_blk_p, (PVOID)0) != SP_RETURN_FOUND)  {
		return (DDI_FAILURE);
	}


	/*
	 * Initialize the "core can now accept the next request" flag.
	 */
	pcscsi_blk_p->pb_core_ready_for_next = TRUE;


	return (DDI_SUCCESS);
}


/* -------------------------------------------------------------------------- */
/*
 * This is where we release any Driver and device-specific resources,
 * set up as a result of a call to pcscsi_device_setup.
 */
static int
pcscsi_device_teardown(
	struct pcscsi_blk	 *pcscsi_blk_p)
{

	/*
	 * Destroy the core code/data structs mutex
	 */
	mutex_destroy(&pcscsi_blk_p->pb_core_mutex);


	/*
	 * Free the (physically contiguous) temp buf
	 * the core uses.
	 */
	ddi_iopb_free(pcscsi_blk_p->pb_tempbuf_p);


	/*
	 * Free the data structs the core uses.
	 * (Allocated in ScsiPortInitialize, in portability.c).
	 */
	kmem_free(pcscsi_blk_p->pb_core_DeviceExtension_p,
		pcscsi_blk_p->pb_core_HwInitializationData.DeviceExtensionSize +
		sizeof (PORT_CONFIGURATION_INFORMATION)		+
		(pcscsi_blk_p->pb_core_HwInitializationData.NumberOfAccessRanges
				 * sizeof (ACCESS_RANGE)));


	return (DDI_SUCCESS);
}


/* -------------------------------------------------------------------------- */
/*
 * This routine is device-specific.
 * Initialze the contents of an SRB.
 * This sets up everything -except- specifics of a data transfer.
 * (Those are set up in pcscsi_xfer_init).
 */
static int
pcscsi_request_init(
	struct pcscsi_ccb	 *ccb_p)
{
	struct scsi_pkt		 *scsi_pkt_p;
	SCSI_REQUEST_BLOCK	 *srb_p;	/* SRB ptr; pcscsi-specific */


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PKTALLOC, DBG_ENTRY, "pcscsi_request_init: Entry\n");
#endif	/* PCSCSI_DEBUG */


	scsi_pkt_p = CCB2PKT(ccb_p);
	srb_p = (SCSI_REQUEST_BLOCK *)(ccb_p->ccb_hw_request_p);


	/*
	 * Initialize the SRB.
	 */
	srb_p->Length			= (USHORT)SCSI_REQUEST_BLOCK_SIZE;

	/* Of the various possible values, we only ever use this one: */
	srb_p->Function			= (UCHAR)SRB_FUNCTION_EXECUTE_SCSI;

	srb_p->SrbStatus		= (UCHAR)0;	/* Init to null */
	srb_p->ScsiStatus		= (UCHAR)0;	/* Init to null */

	/* PCI bus number - never changes; multiple buses not supported  */
	srb_p->PathId			= (UCHAR)0;
	srb_p->TargetId			= (UCHAR)
					(scsi_pkt_p->pkt_address.a_target);
	srb_p->Lun			= (UCHAR)
					(scsi_pkt_p->pkt_address.a_lun);

	/* Tag queing not implemented yet - always use 0 */
	srb_p->QueueTag			= (UCHAR)0;

	/*
	 * Should do something like this when TQ is implemented:
	 * switch (something)
	 * {
	 * case SCSI_TAG_HEAD:
	 *	NewSrbPtr->QueueAction= SCSIMESS_HEAD_OF_QUEUE_TAG;
	 *	NewSrbPtr->SrbFlags|= SRB_FLAGS_QUEUE_ACTION_ENABLE;
	 *	break;
	 * case SCSI_TAG_ORDER:
	 *	NewSrbPtr->QueueAction= SCSIMESS_ORDERED_QUEUE_TAG;
	 *	NewSrbPtr->SrbFlags|= SRB_FLAGS_QUEUE_ACTION_ENABLE;
	 *	break;
	 * case SCSI_TAG_SIMPLE:
	 *	NewSrbPtr->QueueAction= SCSIMESS_SIMPLE_QUEUE_TAG;
	 *	NewSrbPtr->SrbFlags|= SRB_FLAGS_QUEUE_ACTION_ENABLE;
	 *	break;
	 * default:
	 *	This is not a tagged request *
	 *	break;
	 * }
	 * Meanwhile...:
	 */
	srb_p->QueueAction		= (UCHAR)0;	/* Disable Tag Q'ing */

	srb_p->CdbLength		= (UCHAR)ccb_p->ccb_cdb_len;

	/*
	 * NOT USED ANYWHERE.
	 * srb_p->SenseInfoBufferLength	= (UCHAR)0;
	 */

	/*
	 * Set SRB flags
	 *	Only three of the many defined SrbFlags are ever used in core:
	 *		SRB_FLAGS_QUEUE_ACTION_ENABLE (aka enable Tag Q'ing)
	 *		SRB_FLAGS_DISABLE_DISCONNECT
	 *		SRB_FLAGS_DISABLE_SYNCH_TRANSFER
	 */
	if (scsi_pkt_p->pkt_flags & FLAG_NODISCON)  {	/* Disable disconnect */
		srb_p->SrbFlags		|= (ULONG)SRB_FLAGS_DISABLE_DISCONNECT;
	}

	/*
	 * Uncommnent this (and other stuff) to enable Tagged Queuing...
	 * srb_p->SrbFlags		|= (ULONG)SRB_FLAGS_QUEUE_ACTION_ENABLE;
	 */


	/*
	 * Disable synchronous negotiation
	 * srb_p->SrbFlags			|= (ULONG)
	 *				SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
	 */


	/*
	 * Init to 0;
	 * pcscsi_ccb_xfer_init will set this to the actual xfer count.
	 */
	srb_p->DataTransferLength	= (ULONG)0;


	/*
	 * NOT USED ANYWHERE.
	 * srb_p->TimeOutValue		= (ULONG)0;
	 */


	/*
	 * Pointer to S/G list (Filled in by dma_impl).
	 */
	srb_p->DataBuffer		= (PVOID)NULL;

	/*
	 * NOT USED ANYWHERE.
	 * srb_p->SenseInfoBuffer	= (PVOID)0;
	 */

	/*
	 * Not used.
	 * srb_p->NextSrb		= (struct _SCSI_REQUEST_BLOCK*)NULL;
	 * ...but tested in ggmini.c! We used kmem_zalloc, so we're okay anyway.
	 */

	/*
	 * Point OriginalRequest to the ccb (back ptr)
	 */
	srb_p->OriginalRequest		= (PVOID)ccb_p;

	/*
	 * This is done in pcscsi_device_structs_alloc - here just for
	 * completeness.
	 * srb_p->SrbExtension		= (PVOID)
	 *				ccb_p->ccb_hw_request_p->SrbExtension;
	 */

	/*
	 * NOT USED ANYWHERE...
	 * srb_p->QueueSortKey		= (ULONG)0;
	 */

	/*
	 * The SRB -contains- the (16-byte) CDB we'll be using.
	 * srb_p->Cdb
	 * Above we point pkt->cdbp to this structure.
	 * Thus when the target driver fills in the cdb via pkt->cdbp,
	 * it's actually filling in this array.
	 */


	/*	Don't need to initialize the SRBE - the AMD core does. */


	return (DDI_SUCCESS);
}



/* ========================================================================= */
/*
 * Routines called only from the portability layer.
 * These will *usually* be called in interrupt context.
 */

/* -------------------------------------------------------------------------- */
/*
 * Called only from ScsiPortNotification in portability.c.
 * WE ARE IN INTERRUPT CONTEXT WHEN THIS RUNS
 * Mutex is *held* when this function is called
 * We cannot release it, as the trail leads all the way back to the ISR.
 * BE CAREFUL accessing the scsi_pkt from here -
 * if this was an abort or reset request THERE IS NO SCSI_PKT.
 */
void
pcscsi_request_completion(
	struct pcscsi_blk	*pcscsi_blk_p,
	SCSI_REQUEST_BLOCK	*srb_p)
{
	struct scsi_pkt		*scsi_pkt_p;
	struct pcscsi_ccb	*ccb_p;
	int			targ;
	int			lun;
	int			q_tag = 0;	/* Tag queuing not impl. */
	u_int			scsi_status;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_PKT_COMPLETION, DBG_ENTRY,
		"pcscsi_request_completion: Entry\n");
#endif	/* PCSCSI_DEBUG */


	targ = srb_p->TargetId;
	lun  = srb_p->Lun;
	ccb_p = (struct pcscsi_ccb *)(srb_p->OriginalRequest);


	/*
	 * Make sure the request that's completing is the one we expect.
	 */
	ASSERT(
	pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccbs[q_tag]
		!= NULL);
	ASSERT(
	pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccbs[q_tag]
		== ccb_p);


	/*
	 * Remove the ccb from the per-unit array of active ccbs.
	 */
	pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccbs[q_tag] = NULL;

	pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccb_cnt --;
	ASSERT(pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccb_cnt
		>=0);


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_PKT_COMPLETION, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsireqcomp: "
	"targ:%d lun:%d qtag:%d unitptr:%x srbp:%x actvSRBs:%d\n",
			targ, lun, q_tag,
			pcscsi_blk_p->pb_unit_structs[targ][lun],
			srb_p,
		pcscsi_blk_p->pb_unit_structs[targ][lun]->pu_active_ccb_cnt));
#endif	/* PCSCSI_DEBUG */


	/*
	 * OriginalRequest points to the ccb for this transaction.
	 *
	 * See if this srb was generated by an abort or reset.
	 * If so ccb->ccb_pkt_p may be NULL.
	 * (For a reset request it always will be (as there are no scsi_pkts
	 * generated within the driver for resets or general aborts)).
	 * In this case there's nothing for us to do.
	 */
	scsi_pkt_p = ccb_p->ccb_pkt_p;
	if (scsi_pkt_p == NULL) {


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_PKT_COMPLETION, DBG_RESULTS,
			"pcscsi_request_completion: Abort or Reset pkt.\n");
#endif	/* PCSCSI_DEBUG */


		/*
		 * Free the ccb, and all other structs for the request.
		 */
		pcscsi_hba_request_free(ccb_p);
		return;		/* Done with this abort or reset request. */
	}



	/*
	 * Handle the completion status of the request.
	 */
	scsi_status = pcscsi_chkerr(pcscsi_blk_p, scsi_pkt_p);


#ifdef ARQ
	/*
	 * See if we need to run a Request Sense for this req.
	 */

	if (scsi_status == STATUS_CHECK)  {
		pcscsi_start_arq();
		return;
	}
#endif ARQ


	/*
	 * If polling is in progress, see if the packet we're polling on has
	 * completed.
	 */
	if (pcscsi_blk_p->pb_polling_ccb != NULL)	{
		if (pcscsi_blk_p->pb_polling_ccb == ccb_p)	{


#ifdef PCSCSI_DEBUG
			pcscsi_debug(DBG_PKT_COMPLETION, DBG_RESULTS,
			"pcscsi_request_completion: Polled packet completed\n");
#endif	/* PCSCSI_DEBUG */


			/* Note that we're done polling */
			pcscsi_blk_p->pb_polling_ccb = NULL;

			return;		/* This transaction is completed.  */
		}
	}


	/*
	 * Put the packet on the list of packets awaiting target completion.
	 * Put it at the head of the queue, and pcscsi_run_tgt_compl will
	 * take from the tail of the queue.
	 */
	ccb_p->ccb_next_ccb = pcscsi_blk_p->pb_tgt_compl_q_head;
	pcscsi_blk_p->pb_tgt_compl_q_head = ccb_p;


#ifndef	SYNCHRONOUS_TGT_COMPLETIONS
	/*
	 * Trigger a softint to run them down.
	 * This ultimately invokes pcscsi_run_tgt_compl.
	 * (Commnent out this line, and uncomment the appropriate line
	 * in pcscsi_intr to switch to running down the target completion
	 * routines synchronously.)
	 */
#ifdef PCSCSI_DEBUG
	pcscsi_blk_p->pb_pending_comps++;
#endif	/* PCSCSI_DEBUG */

	ddi_trigger_softintr(pcscsi_blk_p->pb_soft_int_id);

#endif	/* SYNCHRONOUS_TGT_COMPLETIONS	*/


}


/* -------------------------------------------------------------------------- */
/*
 * Called only from ScsiPortNotification in portability.c.
 * Mutex is *held* when this function is called.
 * We cannot release it, as the trail leads all the way back to the ISR.
 */
void
pcscsi_start_next_request(
	struct pcscsi_blk	*pcscsi_blk_p,
	uchar_t			target,
	uchar_t			lun)
{
	struct pcscsi_ccb	*ccb_p;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT | DBG_PKT_COMPLETION, DBG_ENTRY,
		"pcscsi_start_next_request: Entry\n");
#endif	/* PCSCSI_DEBUG */


	ccb_p = NULL;


	/*
	 * Indicate that the core is now ready to have another
	 * request submitted to it.
	 */
	pcscsi_blk_p->pb_core_ready_for_next = TRUE;


	/*
	 * See if we have any priority requests pending.
	 * If so, deal with them first.
	 */
	if (pcscsi_blk_p->pb_send_requests_waiting > 0)  {

		/*
		 * Indicate that there is now one less request waiting to start.
		 */
		pcscsi_blk_p->pb_send_requests_waiting--;
		ASSERT(pcscsi_blk_p->pb_send_requests_waiting >= 0);

		/*
		 * Somebody's wating to send a request to the core, so
		 * signal their thread that they can now try again.
		 * (Note this gets around the problem that the core code
		 * can do further processing which touches data structures
		 * *after* calling this routine.
		 * Since the call stack from this routine leads back to the
		 * ISR, and the last thing the (Solaris) ISR does is
		 * release the mutex, we don't have to worry about the
		 * thread waiting on the condition variable starting up
		 * before the core has cleaned up its act.  The waiting thread
		 * cannot continue until the mutex can be re-acquired.)
		 */
		cv_signal(&(pcscsi_blk_p->pb_wait_for_core_ready));

		/*
		 * Don't try to dequeue a request here, we have priority
		 * requests waiting to be started (as soon as the mutex
		 * gets released and the thread waiting on the condition
		 * variable gets signaled).
		 */
		return;
	}


#ifdef UNIT_QUEUE_SIZE
	/*
	 * If we've been asked for the next request
	 * - for a given target/lun -  ...
	 */
	if (target != (UCHAR)NO_TARGET && lun != (UCHAR)NO_LUN)  {

		/*
		 * Attempt to get the next queued request for
		 * the specified target/lun.
		 */
		ccb_p = pcscsi_dequeue_request((int)target, (int)lun,
						pcscsi_blk_p);

	}


	/*
	 * If the pcscsi_dequeue_request either wasn't executed,
	 * or failed to find a queued request (ccb_p=NULL),
	 * find the 'next' target/lun with a queued request.
	 * (Note that as tag queueing is not currently implemented,
	 * this path will always be taken).
	 */
	if (ccb_p == NULL)  {

		/*
		 * If there are no queued requests, this returns NULL,
		 * so we can fall through cleanly.
		 */
		ccb_p = pcscsi_dequeue_next_ccb(pcscsi_blk_p);
	}


	/*
	 * If we found a request to start, do it.
	 */
	if (ccb_p != (struct pcscsi_ccb *)NULL)	{


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_TRANSPORT | DBG_PKT_COMPLETION, DBG_RESULTS,
			"pcscsi_start_next_request: Starting next...\n");
#endif	/* PCSCSI_DEBUG */


		/*
		 * Note that pcscsi_send will set
		 * pcscsi_blk_p->pb_core_ready_for_next = FALSE once it
		 * starts the request.
		 */
		if (pcscsi_send(pcscsi_blk_p, ccb_p, NO_RETRY) != DDI_SUCCESS) {

			/*
			 * Nothing we can do if it fails, so
			 * stick it back on the queue.
			 */
			if (pcscsi_queue_request(ccb_p) != TRUE)  {
				/*
				 * This should *really* never happen, as
				 * we just dequeued this ccb.
				 */
				cmn_err(CE_WARN,
					"pcscsi_start_next_request: "
					"Couldn't re-queue request - "
					"request lost.\n");
			}
		}
	}

#endif	/*  UNIT_QUEUE_SIZE	 */

}



/* ========================================================================= */
/*
 * Functions to handle callback of the target's I/O completion routine.
 */


/* -------------------------------------------------------------------------- */
/*
 * Run off all the pending target request-completion routines.
 * Called only via pcscsi_request_completion.
 */
u_int
pcscsi_run_tgt_compl(
	caddr_t			arg)
{
	struct pcscsi_blk	 *pcscsi_blk_p;
	struct scsi_pkt		 *scsi_pkt_p;
	struct scsi_pkt		 *previous_pkt_p;
	int			return_status;


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_SOFTINT, DBG_ENTRY, "pcscsi_run_tgt_compl: Entry\n");
#endif	/* PCSCSI_DEBUG */


	return_status = DDI_INTR_UNCLAIMED;	/* Assume nothing to do */

	pcscsi_blk_p = (struct pcscsi_blk *)arg;


	mutex_enter(&pcscsi_blk_p->pb_core_mutex);


#ifdef PCSCSI_DEBUG
	pcscsi_debug(DBG_SOFTINT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_run_compl: Pending completion count:%d\n",
			pcscsi_blk_p->pb_pending_comps));
#endif	/* PCSCSI_DEBUG */


	/*
	 * Run the completion routines for -all- the pkts in the linked list.
	 */
	while (pcscsi_blk_p->pb_tgt_compl_q_head != NULL)	{


		/*
		 * There's at least one in the list, or we wouldn't be in
		 * the while loop.
		 */
		return_status = DDI_INTR_CLAIMED;


		/*
		 * Find the last (oldest) packet in the linked list.
		 * NOTE we can't retain any state here;
		 * once we release the mutex we don't know what may have
		 * happened.
		 */
		scsi_pkt_p = pcscsi_blk_p->pb_tgt_compl_q_head->ccb_pkt_p;
		previous_pkt_p = NULL;
		for (;;)	{
			if (PKT2CCB(scsi_pkt_p)->ccb_next_ccb == NULL)	{
				break;	/* Got it. */
			}
			previous_pkt_p = scsi_pkt_p;
			scsi_pkt_p =
				PKT2CCB(scsi_pkt_p)->ccb_next_ccb->ccb_pkt_p;
		}


		/*
		 * Remove it from the linked list.
		 */
		if (scsi_pkt_p ==
			pcscsi_blk_p->pb_tgt_compl_q_head->ccb_pkt_p) {

			/* None left */
			pcscsi_blk_p->pb_tgt_compl_q_head = NULL;
		} else	{
			/* Set new end-of-list */
			PKT2CCB(previous_pkt_p)->ccb_next_ccb = NULL;
		}


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_SOFTINT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_run_compl: Running target completion for pkt:%x...\n",
		scsi_pkt_p));

		pcscsi_blk_p->pb_pending_comps--;
#endif /* PCSCSI_DEBUG */


		/*
		 * Release the mutex, as we're going back to the upper layer.
		 */
		mutex_exit(&pcscsi_blk_p->pb_core_mutex);


		/*
		 * Run the target driver's completion routine for this pkt.
		 */
		(*scsi_pkt_p->pkt_comp)(scsi_pkt_p);


		/*
		 * Grab it again, as we have more diddling to do with driver
		 * data structs.
		 */
		mutex_enter(&pcscsi_blk_p->pb_core_mutex);


#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_SOFTINT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_run_compl: Finished target completion for pkt:%x.\n",
			scsi_pkt_p));
#endif /* PCSCSI_DEBUG */


	}	/* End while (completion routines to run) */


	/*
	 * Done - be sure to turn the mutex loose.
	 */
	mutex_exit(&pcscsi_blk_p->pb_core_mutex);


#ifdef PCSCSI_DEBUG
	if (return_status = DDI_INTR_UNCLAIMED)  {
		pcscsi_debug(DBG_SOFTINT, DBG_RESULTS,
			"pcscsi_run_tgt_compl: Softint discarded.\n");
	}
#endif /* PCSCSI_DEBUG */


	return (return_status);
}


/* ========================================================================== */
/*
 * Allocate all the data structures necessary to send a request to the h/w,
 * including a scsi_pkt.
 */
static int
pcscsi_request_alloc(
	struct scsi_address	 *ap,
	int			cmdlen,
	int			statuslen,
	int			tgtlen,
	int			flags,
	int			(*callback)(),
	caddr_t			arg,
	struct pcscsi_ccb	 **ccb_p_p)
{
	struct pcscsi_ccb	 *ccb_p;
	struct pcscsi_blk	 *pcscsi_blk_p;
	int			status;


	/*
	 * Allocate the ccb,
	 * -and-
	 * any device-dependent structures needed for this transaction.
	 */
	status = pcscsi_hba_request_alloc(ap, cmdlen, statuslen, ccb_p_p,
					callback);
	if (status != DDI_SUCCESS)  {
		return (status);
	}
	ccb_p = *ccb_p_p;


	/*
	 * Allocate the scsi_pkt.
	 * scsi_hba_pkt_alloc puts addr of target_private in pkt_private.
	 * It also sets scsi_pkt_p->pkt_address.
	 *
	 * Must pass 0 to scsi_hba_pkt_alloc for sizeof scb and cdb
	 * fields. As these structs may be DMAed, they'll need to be
	 * allocated with iopb_alloc (which provides DMA-able memory).
	 */

	/* Request minimum target_privte area from scsi_hba_pkt_alloc.	 */
	if (tgtlen <= PKT_PRIV_LEN)  {
		tgtlen = PKT_PRIV_LEN;
	}

	pcscsi_blk_p = ADDR2BLK(ap);
	ccb_p->ccb_pkt_p = scsi_hba_pkt_alloc(
			pcscsi_blk_p->pb_dip,	/* HBA devinfo node */
			ap,			/* SCSI address struct */
			0,			/* CDB length */
			0,			/* SCB length */
			tgtlen,			/* target-private area */
			0,			/* HBA-private data length */
			callback,
			arg);

	if (ccb_p->ccb_pkt_p == (struct scsi_pkt *)NULL) {
		pcscsi_hba_request_free(ccb_p);
		return (DDI_FAILURE);
	}



	/*
	 * Initialize static info for the scsi_pkt.
	 */

	/* Point pkt back to ccb. */
	ccb_p->ccb_pkt_p->pkt_ha_private = (opaque_t)ccb_p;

	/* Set pkt pointer to the SCSI CDB (which is embedded in the SRB) */
	ccb_p->ccb_pkt_p->pkt_cdbp	= (u_char *)ccb_p->ccb_cdb_p;

	/* Set pkt pointer to the SCSI request status.  */
	ccb_p->ccb_pkt_p->pkt_scbp	= (u_char *)ccb_p->ccb_scb_p;


	return (DDI_SUCCESS);
}


/* ========================================================================== */
/*
 * Allocate all the data structures necessary to send a request to the h/w,
 * NOT including a scsi_pkt.
 * This routine can be called directly within the driver for setting up
 * requests for which there may not be a scsi_pkt (such as resets and aborts).
 */
static int
pcscsi_hba_request_alloc(
	struct scsi_address	 *ap,
	int			cmdlen,
	int			statuslen,
	struct pcscsi_ccb	 **ccb_p_p,
	int			(*callback)())
{
	struct pcscsi_ccb	 *ccb_p;
	int			status;


	/*
	 * Allocate the ccb struct.
	 */
	 *ccb_p_p = (struct pcscsi_ccb *)
		kmem_zalloc(sizeof (struct pcscsi_ccb), HBA_KMFLAG(callback));
	if (*ccb_p_p == NULL)	{
		return (DDI_FAILURE);
	}
	ccb_p = *ccb_p_p;


	/*
	 * Initialize static info for the ccb.
	 * (Just that which needs to be passed down to
	 * pcscsi_device_structs_alloc).
	 */
	ccb_p->ccb_blk_p = ADDR2BLK(ap);
	ccb_p->ccb_ap = ap;
	ccb_p->ccb_cdb_len = cmdlen;
	ccb_p->ccb_scb_len = statuslen;


	/*
	 * Assume this request is to send a normal SCSI cmd.
	 * If it's to be an abort or reset the caller will take care
	 * of setting this to the proper value.
	 */
	ccb_p->ccb_cmd_type = SCSI_CMD;


	/*
	 * Allocate the CDB, SCB, and any h/w-specific structs for this request.
	 * (Fills in ccb_cdb_p, ccb_scb_p, ccb_hw_request_p).
	 */
	status = pcscsi_device_structs_alloc(ap, ccb_p, cmdlen, statuslen,
						callback);
	if (status != DDI_SUCCESS)	{
		kmem_free(ccb_p, sizeof (struct pcscsi_ccb));
	}

	return (status);
}


/* ========================================================================== */
/*
 * ccb is allocated when we get here.
 * This routine is used to allocate memory
 * for per-request device-specific data structs.
 * This routine is device-specific.
 */
static int
pcscsi_device_structs_alloc(
	struct scsi_address	 *ap,
	struct pcscsi_ccb	 *ccb_p,
	int			cmdlen,
	int			statuslen,
	int			(*callback)())
{
	SCSI_REQUEST_BLOCK	 *srb_p;


	/*
	 * Allocate the SRB (SCSI Request Block - an AMD core code
	 * struct which is passed to the core).
	 * The SRB *contains* the SCSI CDB.
	 * NOTE currently no part of the SRB (including the CDB) is DMAed.
	 * Allocate the SRB and SRBE back-to-back, just for convenience.
	 */
	ccb_p->ccb_hw_request_p = (opaque_t)
		kmem_zalloc(sizeof (SCSI_REQUEST_BLOCK)    +
				sizeof (SRB_EXTENSION),
				HBA_KMFLAG(callback));

	if (ccb_p->ccb_hw_request_p == (opaque_t)NULL)  {
		return (DDI_FAILURE);
	}
	srb_p = (SCSI_REQUEST_BLOCK *)(ccb_p->ccb_hw_request_p);


	/*
	 * Set ptr to memory for SRBE.
	 */
	srb_p->SrbExtension = (SRB_EXTENSION *) (srb_p + 1);



	/*
	 * Set ptr to memory for CDB.
	 */
	ccb_p->ccb_cdb_p = (uchar_t *) &(srb_p->Cdb);


	/*
	 * Set ptr to memory for SCB.
	 */
	ccb_p->ccb_scb_p = (uchar_t *) &(srb_p->ScsiStatus);


	return (DDI_SUCCESS);
}


/* ========================================================================== */
/*
 * Does everything necessary to set up
 * (an alreaady-allocated-and-initialized request)
 * for data transfer.
 */
static int
pcscsi_xfer_setup(
	struct pcscsi_ccb	 *ccb_p,
	struct buf		 *bp,
	int			pkt_flags,
	int			(*callback)(),
	boolean_t		new_transfer)
{
	int			status;


	/*
	 * If this is a new transfer -
	 * (not a continuance of the next portion of a broken-up (DMA_PARTIAL)
	 * request),
	 * allocate the DMA transfer resources (if necessary).
	 */
	if (new_transfer == TRUE)  {

		/*
		 * If DMA resources haven't been allocated, do so.
		 * (If they have already been allocated, this packet/ccb/etc
		 * are being reused by the upper layer).
		 */
		if (ccb_p->ccb_dma_p == NULL)  {

			status = pcscsi_dma_structs_alloc(ccb_p, callback);
			if (status != DDI_SUCCESS)  {
				return (status);
			}

		} else {

			/*
			 * This is a new transfer, *but*
			 * this packet is being reused (or we wouldn't
			 * already have dma structs allocated).
			 *
			 * Reinitialize key fields, so things work right.
			 */

			/* Set amount-already-transferred to zero.	 */
			ccb_p->ccb_dma_p->dma_totxfer = 0;

			/*
			 * Tell dma_impl_setup logic to start over at the first
			 * seg of the first window.
			 */
			ccb_p->ccb_dma_p->dma_seg = NULL;
		}
	}


	/*
	 * Initialize and/or Set up the DMA resources for -this- transfer.
	 * (Serves as pcscsi_dma_driver_structs_init, for those that have
	 * noticed the naming convention).
	 */
	status = dma_impl_setup(CCB2UNITDIP(ccb_p), ccb_p->ccb_dma_p, bp,
				pkt_flags, (opaque_t)ccb_p,
				&(CCB2UNIT(ccb_p)->pu_lim),
				callback, (caddr_t)NULL, new_transfer);
	if (status != DDI_SUCCESS) {


		/*
		 * Warning messages are output directly from dma_impl_setup
		 * and children, so no need to do it here.
		 */
#ifdef PCSCSI_DEBUG
		pcscsi_debug(DBG_PKT_COMPLETION, DBG_RESULTS,
			"pcscsi: dma_impl_setup failed.\n");
#endif /* PCSCSI_DEBUG */


		pcscsi_dma_structs_free(ccb_p);
		return (status);
	}


	/*
	 * Set up the device-specific data structs for data transfer.
	 * (These need to be set up for every transfer).
	 */
	status = pcscsi_dma_device_structs_init(ccb_p, new_transfer);
	if (status != DDI_SUCCESS) {
		pcscsi_dma_structs_free(ccb_p);
		return (status);
	}


	/*
	 * Save how much remains to be transferred *after* this
	 * DMA is done.
	 */
	ccb_p->ccb_pkt_p->pkt_resid = bp->b_bcount -
				ccb_p->ccb_dma_p->dma_totxfer;


	return (status);
}


/* ========================================================================== */
/*
 * Frees any DMA resources associated with a request.
 */
void
pcscsi_dma_structs_free(
	struct pcscsi_ccb	 *ccb_p)
{


	/*
	 * If there's any data transfer resources to set up, free them.
	 */
	if (ccb_p->ccb_dma_p != NULL)  {


#ifdef SAVE_DMA_SEG_VIRT_ADDRS
/* Can this code go away?? */
		/*
		 * If this is a page-io buffer, then we've mapped a virtual
		 * address to the buffer (for use in physical->virtual
		 * address translation (in ScsiPortGetVirtualAddress
		 * in portabiliity.c)j.
		 * We need to undo this mapping.
		 */
		if (ccb_p->ccb_dma_p->dma_buf_p->b_flags &
				(B_PAGEIO | B_PHYS)) {
			/*
			 * Release the virtual mapping for the pages.
			 */
			/*
			 * Don't do this -
			 * per Bruce/Dan 4/5/95
			 * Potential conflict with ODS...?
			 */
			/* bp_mapout(ccb_p->ccb_dma_p->dma_buf_p);   */
		}
#endif /* SAVE_DMA_SEG_VIRT_ADDRS */


		/* Free Solaris DMA resources.  */
		if (ccb_p->ccb_dma_p->dma_handle != NULL)  {
			ddi_dma_free(ccb_p->ccb_dma_p->dma_handle);
			ccb_p->ccb_dma_p->dma_handle = NULL;
		}


		/* Free device-specific DMA structs. */
		pcscsi_dma_device_structs_free(ccb_p);


		/* Free driver-specific DMA structs. */
		pcscsi_dma_driver_structs_free(ccb_p);


		ccb_p->ccb_dma_p = NULL;	/* Indicate none allocated. */

	}
}


/* ========================================================================== */
/*
 * Allocates any necessary data structs to perform DMA.
 */
static int
pcscsi_dma_structs_alloc(struct pcscsi_ccb *ccb_p, int (*callback)())
{
	int	status;


	/*
	 * Allocate driver-specific per-request DMA data structs.
	 */
	status = pcscsi_dma_driver_structs_alloc(ccb_p, callback);
	if (status != DDI_SUCCESS)  {
		return (status);
	}


	/*
	 * Allocate device-specific per-request DMA data structs.
	 */
	status = pcscsi_dma_device_structs_alloc(ccb_p, callback);
	if (status != DDI_SUCCESS)  {
		pcscsi_dma_driver_structs_free(ccb_p);
		return (status);
	}


	return (DDI_SUCCESS);
}


/* ========================================================================== */
/*
 * ccb is allocated when we get here.
 * This routine is used to allocate memory
 * for per-request driver-specific data structs for DMA.
 * This routine is device-specific.
 */
static int
pcscsi_dma_driver_structs_alloc(
	struct pcscsi_ccb	 *ccb_p,
	int			(*callback)())
{
	dma_blk_t		 *scsi_dma_blk;


	/*
	 * Allocate the scsi_dma_blk.
	 * (This data structure contains everything you need to know about
	 * the current state of a DMA transfer).
	 */
	ccb_p->ccb_dma_p = kmem_zalloc(sizeof (dma_blk_t),
					HBA_KMFLAG(callback));
	if (ccb_p->ccb_dma_p == (dma_blk_t *)NULL)  {
		return (DDI_FAILURE);
	}


	/*
	 * Allocate the sg list. (DMAable memory).
	 */
	if (ddi_iopb_alloc(
				CCB2BLK(ccb_p)->pb_dip,
				&(CCB2UNIT(ccb_p)->pu_lim),
				sizeof (sg_list_t),
				(caddr_t *)&(ccb_p->ccb_dma_p->dma_sg_list_p))
					== DDI_FAILURE) {

		kmem_free(ccb_p->ccb_dma_p, sizeof (dma_blk_t));
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);

}


/* ========================================================================== */
/*
 * ccb is allocated when we get here.
 * This routine is used to allocate memory
 * for per-request device-specific data structs for DMA.
 * This routine is device-specific.
 */
/* This function is device-dependent.	 */
static int
pcscsi_dma_device_structs_alloc(
	struct pcscsi_ccb	 *ccb_p,
	int			(*callback)())
{

	/*
	 * No additional per-device data structs to allocate for DMA
	 * in this driver.
	 */

	return (DDI_SUCCESS);

}


/* ========================================================================== */
/*
 * This function is device-dependent.
 * Set up the device-specific data structs for -this- DMA transfer.
 */
static int
pcscsi_dma_device_structs_init(
	struct pcscsi_ccb	 *ccb_p,
	boolean_t		new_transfer)
{
	SCSI_REQUEST_BLOCK	 *srb_p;


	srb_p = (SCSI_REQUEST_BLOCK *)ccb_p->ccb_hw_request_p;


	/*
	 * Set up the requested transfer length.
	 * Transfer count is NOT (bp->b_bcount); this may be a partial xfr.
	 */
	srb_p->DataTransferLength = (ULONG)ccb_p->ccb_dma_p->dma_seg_xfer_cnt;


	/* Hook the sg list to the SRB (where the core expects it).	 */
	srb_p->DataBuffer	= (PVOID)ccb_p->ccb_dma_p->dma_sg_list_p;


	return (DDI_SUCCESS);
}


/* ========================================================================== */
/*
 * Free the request, and any other data structs associated with it.
 */
static void
pcscsi_hba_request_free(struct pcscsi_ccb *ccb_p)
{
	struct scsi_pkt		 *scsi_pkt_p;


	/*
	 * If there's any data transfer resources to free, free them.
	 */
	pcscsi_dma_structs_free(ccb_p);


	/*
	 * Free the CDB, SCB, and any device-specific structs.
	 */
	pcscsi_device_structs_free(ccb_p);


	/*
	 * Free the ccb.
	 */
	kmem_free(ccb_p, sizeof (struct pcscsi_ccb));


}


/* ========================================================================== */
/*
 * Free any device-specific DMA structs.
 */
static void
pcscsi_dma_device_structs_free(struct pcscsi_ccb *ccb_p)
{

	/*
	 * No additional DMA-specific structs in this driver.
	 */

}


/* ========================================================================== */
/*
 * Free any driver-specific DMA structs.
 */
static void
pcscsi_dma_driver_structs_free(struct pcscsi_ccb *ccb_p)
{

	/*
	 * Deallocate the sg list. (DMAable memory).
	 */
	if (ccb_p->ccb_dma_p->dma_sg_list_p != NULL)  {
		ddi_iopb_free((caddr_t)(ccb_p->ccb_dma_p->dma_sg_list_p));
	}


	/* Free the scsi_dma_blk struct. */
	if (ccb_p->ccb_dma_p != NULL)  {
		kmem_free(ccb_p->ccb_dma_p, sizeof (dma_blk_t));
	}

}


/* ========================================================================== */
/*
 * Device-specific.
 * Free the CDB, SCB, and any device-specific structs.
 */
static void
pcscsi_device_structs_free(struct pcscsi_ccb *ccb_p)
{


	/* Free the pcscsi device-specific per-request struct.	 */
	kmem_free(ccb_p->ccb_hw_request_p, sizeof (SCSI_REQUEST_BLOCK)    +
						sizeof (SRB_EXTENSION));

}


#ifdef UNIT_QUEUE_SIZE
/* ========================================================================== */
/*
 * Puts the request at the end of the linked list of queued requests for this
 * target/lun.
 * Called with the mutex *held*.
 */
static int
pcscsi_queue_request(struct pcscsi_ccb *ccb_p)
{
	struct pcscsi_unit	 *unit_p;


	unit_p = CCB2UNIT(ccb_p);
	ASSERT(unit_p != NULL);


	if (unit_p->pu_ccb_queued_cnt >= UNIT_QUEUE_SIZE)  {
		return (FALSE);	/* Request *not* queued - queue full.  */
	}


	/*
	 * Queue it (at the end of the linked list).
	 */
	unit_p->pu_ccb_queued_cnt++;
	ccb_p->ccb_next_ccb = NULL;
	if (unit_p->pu_ccb_queue_tail != NULL)  {
		unit_p->pu_ccb_queue_tail->ccb_next_ccb = ccb_p;
	} else {
		/* Nothing in queue - set head and taile to this ccb.	 */
		unit_p->pu_ccb_queue_head = ccb_p;
		unit_p->pu_ccb_queue_tail = ccb_p;
	}
	unit_p->pu_ccb_queue_tail = ccb_p;


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_queue_request: ccb %x queued to unit_p %x..\n",
		ccb_p, unit_p));
#endif	/* PCSCSI_DEBUG */


	return (TRUE);	/* Request queued.  */
}


/* ========================================================================== */
/*
 * Dequeues a request from the front of the linked list of queued requests
 * for this target/lun.
 * Called with the mutex *held*.
 */
struct pcscsi_ccb *
pcscsi_dequeue_request(int target, int lun, struct pcscsi_blk *blk_p)
{
	struct pcscsi_unit	 *unit_p;
	struct pcscsi_ccb	 *ccb_p;


	ASSERT(target >= 0 && target <= MAX_TARGETS);
	ASSERT(lun >= 0 && lun <= MAX_LUNS_PER_TARGET);

	unit_p = blk_p->pb_unit_structs[target][lun];
	ASSERT(unit_p != NULL);


	if (unit_p->pu_ccb_queued_cnt == 0)  {
		ASSERT(unit_p->pu_ccb_queue_head == NULL);
		ASSERT(unit_p->pu_ccb_queue_tail == NULL);
		return ((struct pcscsi_ccb *)NULL);
	}


	/*
	 * DeQueue it (from the front of the linked list).
	 */
	unit_p->pu_ccb_queued_cnt--;
	ccb_p = unit_p->pu_ccb_queue_head;
	unit_p->pu_ccb_queue_head = ccb_p->ccb_next_ccb;

	if (unit_p->pu_ccb_queue_head == NULL)  {
		/* Queue empty. */
		unit_p->pu_ccb_queue_tail = NULL;
	}


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
		"pcscsi_dequeue_request: ccb %x dequeued from unit_p %x\n",
		ccb_p, unit_p));
#endif	/* PCSCSI_DEBUG */


	return (ccb_p);	/* Request dequeued.  */
}


/* ========================================================================== */
/*
 * Called with the mutex *held*.
 * This routine is perhaps not well-named.
 * This routine is called to get the "next" queued request, so it can be
 * started.
 * It implements a simple round-robin, among all the target/luns instances.
 * pb_last_rr_target and pb_last_rr_lun retain state (for this driver
 * instance) as to which instance we last pulled a request from.
 * If in this routine we cycle through all the target/luns back to the ones
 * we started with, drop out and return NULL.
 */
static struct pcscsi_ccb *
pcscsi_dequeue_next_ccb(struct pcscsi_blk *blk_p)
{
	struct pcscsi_ccb	 *ccb_p;
	int			i, j;


	ccb_p = NULL;


	/* Loop through all luns no more than once.	 */
	for (i = 0; i < MAX_LUNS_PER_TARGET; i++)  {

		/* Move to the next lun. */
		blk_p->pb_last_rr_lun =
			(blk_p->pb_last_rr_lun+1)
				% MAX_LUNS_PER_TARGET;


		/* Loop through all targets no more than once.	 */
		for (j = 0; j < MAX_TARGETS; j++)  {

			/* Move to the next target. */
			blk_p->pb_last_rr_target =
				(blk_p->pb_last_rr_target+1)
					% MAX_TARGETS;

			/* If this an existing unit... */
			if (blk_p->pb_unit_structs[blk_p->pb_last_rr_target]
						[blk_p->pb_last_rr_lun]
				!= NULL)  {


				/*
				 * Attempt to dequeue a ccb for this unit.
				 * Returns NULL if none.
				 */
				ccb_p = pcscsi_dequeue_request(
						blk_p->pb_last_rr_target,
						blk_p->pb_last_rr_lun,
						blk_p);

				if (ccb_p != NULL)  {


#ifdef	PCSCSI_DEBUG
					pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
						sprintf(pcscsig_dbgmsg,
					"pcscsi_dequeue_next_ccb: "
					"dequeued ccb %x target %x/lun %x.\n",
					ccb_p, blk_p->pb_last_rr_target,
					blk_p->pb_last_rr_lun));
#endif	/* PCSCSI_DEBUG */


					return (ccb_p);	/* Got one. */
				}

			}	/* If unit exists.	 */

		}	/* For all targets... */

	}	/* For all luns. */


#ifdef	PCSCSI_DEBUG
	pcscsi_debug(DBG_TRANSPORT, DBG_RESULTS,
		"pcscsi_dequeue_next_ccb: No queued ccbs found.\n");
#endif	/* PCSCSI_DEBUG */


	return (NULL);
}


/* ========================================================================== */
#endif	/* UNIT_QUEUE_SIZE	 */




#ifdef PCSCSI_DEBUG
/* ========================================================================== */
/*
 * Debug functions.
 *
 * These can be invoked from kadb with the following syntax:
 *	kadb[0]: pcscsi_dump_blk::call <blk-address>
 */


/* -------------------------------------------------------------------------- */
void
pcscsi_debug(uint funcs, uint granularity, char *message)
{

	if (funcs & pcscsig_debug_funcs) { /* Then we're debugging this func */

		if (pcscsig_debug_gran & DBG_VERBOSE)	{
			/* Then print everything */
			prom_printf(message);
			return;
		}

		/* If this granularity level is set, */
		if (granularity & pcscsig_debug_gran)	{
			prom_printf(message);
			return;
		}
	}
}


/* -------------------------------------------------------------------------- */
static void
pcscsi_dump_blk(struct pcscsi_blk *pcscsi_blk_p)
{
	int	targ;
	int	lun;


	prom_printf("core-ready-for-next: %d\n",
		pcscsi_blk_p->pb_core_ready_for_next);
	prom_printf("Active units  : %d\n", pcscsi_blk_p->pb_active_units);

	prom_printf(
"       |     0		1	2	3        4        5        6\n");

	for (targ = 0; targ < 7; targ++)	{
		prom_printf("targ:%d | ", targ);
		for (lun = 0; lun < 7; lun++)	{
			prom_printf("%8x ",
				pcscsi_blk_p->pb_unit_structs[targ][lun]);
		}
		prom_printf("\n");
	}
	prom_printf("pb_tempbuf_p	: %x\n", pcscsi_blk_p->pb_tempbuf_p);
	prom_printf("pb_tempbuf_physaddr: %x\n",
		pcscsi_blk_p->pb_tempbuf_physaddr);
	prom_printf("pb_tempbuf_length	: %x\n",
		pcscsi_blk_p->pb_tempbuf_length);

	prom_printf("pb_polling_ccb	: %x\n", pcscsi_blk_p->pb_polling_ccb);

	prom_printf("pb_last_rr_target	: %x\n",
		pcscsi_blk_p->pb_last_rr_target);
	prom_printf("pb_last_rr_lun	: %x\n", pcscsi_blk_p->pb_last_rr_lun);


	prom_printf("pb_tgt_compl_q_head: %x\n",
		pcscsi_blk_p->pb_tgt_compl_q_head);


	prom_printf("pb_core_DeviceExtension_p	: %x\n",
		pcscsi_blk_p->pb_core_DeviceExtension_p);

}


/* -------------------------------------------------------------------------- */
static void
pcscsi_dump_pkt(struct scsi_pkt *pkt_p)
{
	prom_printf("pkt_ha_private	:%x\n", pkt_p->pkt_ha_private);
	prom_printf("pkt_addr.a_hba_tran:%x\n", pkt_p->pkt_address.a_hba_tran);
	prom_printf("pkt_addr.a_target	:%d\n", pkt_p->pkt_address.a_target);
	prom_printf("pkt_addr.a_lun	:%d\n", pkt_p->pkt_address.a_lun);
	prom_printf("pkt_private	:%x\n", pkt_p->pkt_private);
	prom_printf("pkt_comp		:%x\n", pkt_p->pkt_comp);
	prom_printf("pkt_flags		:%d\n", pkt_p->pkt_flags);
	prom_printf("pkt_time		:%d\n", pkt_p->pkt_time);
	prom_printf("pkt_scbp		:%d\n", pkt_p->pkt_scbp);
	prom_printf("pkt_cdbp		:%d\n", pkt_p->pkt_cdbp);
	prom_printf("pkt_resid		:%d\n", pkt_p->pkt_resid);
	prom_printf("pkt_state		:%d\n", pkt_p->pkt_state);
	prom_printf("pkt_statistics	:%d\n", pkt_p->pkt_statistics);
	prom_printf("pkt_reason		:%d\n", pkt_p->pkt_reason);

}


/* -------------------------------------------------------------------------- */
static void
pcscsi_dump_unit(struct pcscsi_unit *unit_p)
{

	prom_printf("pu_refcnt:		:%x\n", unit_p->pu_refcnt);
	prom_printf("pu_active_ccb_cnt	:%x\n", unit_p->pu_active_ccb_cnt);
/*	prom_printf("&pu_lim		:%x\n", &unit_p->pu_lim);   */
	prom_printf("pu_total_sectors	:%x\n", unit_p->pu_total_sectors);
	prom_printf("&pu_active_ccbs	:%x\n", &unit_p->pu_active_ccbs);
#ifdef UNIT_QUEUE_SIZE
	prom_printf("pu_ccb_queue_head	:%x\n", unit_p->pu_ccb_queue_head);
	prom_printf("pu_ccb_queue_tail	:%x\n", unit_p->pu_ccb_queue_tail);
	prom_printf("pu_ccb_queued_cnt	:%x\n", unit_p->pu_ccb_queued_cnt);
#endif	/* UNIT_QUEUE_SIZE */
	prom_printf("pu_SpecificLuExtens:%x\n", unit_p->pu_SpecificLuExtension);

}


/* -------------------------------------------------------------------------- */
static void
pcscsi_dump_ccb(struct pcscsi_ccb *pcscsi_ccb_p)
{

	prom_printf("pktp	:%x\n", pcscsi_ccb_p->ccb_pkt_p);
	prom_printf("srbp	:%x\n", pcscsi_ccb_p->ccb_hw_request_p);
	prom_printf("ccb_dma_p	:%x\n", pcscsi_ccb_p->ccb_dma_p);
#ifdef SAVE_DMA_SEG_VIRT_ADDRS
/*
 *	prom_printf("virt_addrs :%x\n", pcscsi_ccb_p->ccb_sg_list_virt_addrs);
 */
#endif /* SAVE_DMA_SEG_VIRT_ADDRS */

}


/* -------------------------------------------------------------------------- */
static void
pcscsi_dump_srb(SCSI_REQUEST_BLOCK *srb_p)
{
	char	 *srb_status_str;
	char	 *scsi_status_str;


	/*
	 * Here's the SRB struct def, copied from srb.h:
	 *
	 * typedef struct _SCSI_REQUEST_BLOCK {
	 *	USHORT Length;			/ offset 0
	 *	UCHAR Function;			/ offset 2
	 *	UCHAR SrbStatus;		/ offset 3
	 *	UCHAR ScsiStatus;		/ offset 4
	 *	UCHAR PathId;			/ offset 5
	 *	UCHAR TargetId;			/ offset 6
	 *	UCHAR Lun;			/ offset 7
	 *	UCHAR QueueTag;			/ offset 8
	 *	UCHAR QueueAction;		/ offset 9
	 *	UCHAR CdbLength;		/ offset 10
	 *	UCHAR SenseInfoBufferLength;	/ offset 11
	 *	ULONG SrbFlags;			/ offset 12
	 *	ULONG DataTransferLength;	/ offset 16
	 *	ULONG TimeOutValue;		/ offset 20
	 *	PVOID DataBuffer;		/ offset 24
	 *	PVOID SenseInfoBuffer;		/ offset 28
	 *	struct _SCSI_REQUEST_BLOCK *NextSrb;	/ offset 32
	 *	PVOID OriginalRequest;		/ offset 36
	 *	PVOID SrbExtension;		/ offset 40
	 *	ULONG QueueSortKey;		/ offset 44
	 *	UCHAR Cdb[16];			/ offset 48
	 * } SCSI_REQUEST_BLOCK, *PSCSI_REQUEST_BLOCK;
	 */

	/*
	 * Handle SRB (controller) return statuses.
	 */
	switch (srb_p->SrbStatus)	{

	case  SRB_STATUS_SUCCESS:
		srb_status_str = "SRB_STATUS_SUCCESS";
		break;
	case  SRB_STATUS_DATA_OVERRUN:
		srb_status_str = "SRB_STATUS_DATA_OVERRUN";
		break;
	case  SRB_STATUS_ABORTED:
		srb_status_str = "SRB_STATUS_ABORTED";
		break;
	case  SRB_STATUS_ABORT_FAILED:
		srb_status_str = "SRB_STATUS_ABORT_FAILED";
		break;
	case  SRB_STATUS_MESSAGE_REJECTED:
		srb_status_str = "SRB_STATUS_MESSAGE_REJECTED";
		break;
	case  SRB_STATUS_BUS_RESET:
		srb_status_str = "SRB_STATUS_BUS_RESET";
		break;
	case  SRB_STATUS_UNEXPECTED_BUS_FREE:
		srb_status_str = "SRB_STATUS_UNEXPECTED_BUS_FREE";
		break;
	case  SRB_STATUS_PHASE_SEQUENCE_FAILURE:
		srb_status_str = "SRB_STATUS_PHASE_SEQUENCE_ERROR";
		break;
	case  SRB_STATUS_ERROR:
		srb_status_str = "SRB_STATUS_ERROR";
		break;
	case  SRB_STATUS_SELECTION_TIMEOUT:
		srb_status_str = "SRB_STATUS_SELECTION_TIMEOUT";
		break;
	case  SRB_STATUS_INVALID_REQUEST:
		srb_status_str = "SRB_STATUS_INVALID_REQUEST";
		break;
	case  SRB_STATUS_BAD_FUNCTION:
		srb_status_str = "SRB_STATUS_BAD_FUNCTION";
		break;
	/*
	 * Defined but not currently used in the core code...
	 * case  SRB_STATUS_PENDING:
	 * case  SRB_STATUS_BUSY:
	 * case  SRB_STATUS_INVALID_PATH_ID:
	 * case  SRB_STATUS_NO_DEVICE:
	 * case  SRB_STATUS_TIMEOUT:
	 * case  SRB_STATUS_COMMAND_TIMEOUT:
	 * case  SRB_STATUS_PARITY_ERROR:
	 * case  SRB_STATUS_REQUEST_SENSE_FAILED:
	 * case  SRB_STATUS_NO_HBA:
	 * case  SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
	 * case  SRB_STATUS_REQUEST_FLUSHED:
	 * case  SRB_STATUS_INVALID_LUN:
	 * case  SRB_STATUS_INVALID_TARGET_ID:
	 * case  SRB_STATUS_ERROR_RECOVERY:
	 */

	default:
		srb_status_str = "Unknown SRB_STATUS";
		break;
	}	/* End case SrbStatus */


	/* Handle SCSI return status */
	switch (srb_p->ScsiStatus)	{

	case  STATUS_GOOD		/* SCSISTAT_GOOD */:
		scsi_status_str = "SCSISTAT_GOOD";
		break;
	case  STATUS_CHECK		/* SCSISTAT_CHECK_CONDITION */:
		scsi_status_str = "SCSISTAT_CHECK_CONDITION";
		break;
	case  STATUS_MET		/* SCSISTAT_CONDITION_MET */:
		scsi_status_str = "SCSISTAT_CONDITION_MET";
		break;
	case  STATUS_BUSY		/* SCSISTAT_BUSY */:
		scsi_status_str = "SCSISTAT_BUSY";
		break;
	case  STATUS_INTERMEDIATE	/* SCSISTAT_INTERMEDIATE */:
		scsi_status_str = "SCSISTAT_INTERMEDIATE";
		break;
	case  STATUS_INTERMEDIATE_MET	/* SCSISTAT_INTERMEDIATE_COND_MET */:
		scsi_status_str = "SCSISTAT_INTERMEDIATE_COND_MET";
		break;
	case  STATUS_RESERVATION_CONFLICT /* SCSISTAT_RESERVATION_CONFLICT */:
		scsi_status_str = "SCSISTAT_RESERVATION_CONFLICT";
		break;
	case  STATUS_TERMINATED		/* SCSISTAT_COMMAND_TERMINATED */:
		scsi_status_str = "SCSISTAT_COMMAND_TERMINATED";
		break;
	case  STATUS_QFULL		/* SCSISTAT_QUEUE_FULL */:
		scsi_status_str = "SCSISTAT_QUEUE_FULL";
		break;
	default:
		scsi_status_str = "Unknown SCSISTAT";
		break;
	}	/* End case ScsiStatus */


	prom_printf("    Length		: %x\n", srb_p->Length);
	prom_printf("    Function	: %x\n", srb_p->Function);
	prom_printf("    SrbStatus	: %x (%s)\n",
					srb_p->SrbStatus, srb_status_str);
	prom_printf("    ScsiStatus	: %x (%s)\n",
					srb_p->ScsiStatus, scsi_status_str);
	prom_printf("    TargetId	: %x",   srb_p->TargetId);
	prom_printf("    Lun		: %x\n", srb_p->Lun);
	prom_printf("    QueueTag	: %x\n", srb_p->QueueTag);
	prom_printf("    QueueAction	: %x\n", srb_p->QueueAction);
	prom_printf("    CdbLength	: %x\n", srb_p->CdbLength);
	prom_printf("    SenseInfoBufferLength: %x\n",
						srb_p->SenseInfoBufferLength);
	prom_printf("    SrbFlags	: %x\n", srb_p->SrbFlags);
	prom_printf("    DataTransferLength: %x\n", srb_p->DataTransferLength);
	prom_printf("    TimeOutValue	: %x\n", srb_p->TimeOutValue);
	prom_printf("    DataBuffer	: %x\n", srb_p->DataBuffer);
	prom_printf("    TimeOutValue	: %x\n", srb_p->TimeOutValue);
	prom_printf("    SenseInfoBuffer: %x\n", srb_p->SenseInfoBuffer);
	prom_printf("    NextSrb	: %x\n", srb_p->NextSrb);
	prom_printf("    OriginalRequest: %x\n", srb_p->OriginalRequest);
	prom_printf("    SrbExtension	: %x\n", srb_p->SrbExtension);
	prom_printf("    QueueSortKey	: %x\n", srb_p->QueueSortKey);
	prom_printf("    &Cdb		: %x\n", &srb_p->Cdb);

}


/* -------------------------------------------------------------------------- */
static void
pcscsi_dump_DevExt(SPECIFIC_DEVICE_EXTENSION *DevExt)
{

	/*
	 * The DeviceExtension struct is defined in ggmini.h.
	 */
	prom_printf("AdapterFlags:	%x\n", DevExt->AdapterFlags);
	prom_printf("AdapterState:	%x\n", DevExt->AdapterState);
	prom_printf("Adapter:		%x\n", DevExt->Adapter);
	prom_printf("AdapterStatus:	%x\n", DevExt->AdapterStatus);
	prom_printf("SequenceStep:	%x\n", DevExt->SequenceStep);
	prom_printf("AdapterInterrupt:	%x\n", DevExt->AdapterInterrupt);
	prom_printf("AdapterBusId:	%x\n", DevExt->AdapterBusId);
	prom_printf("AdapterBusIdMask:	%x\n", DevExt->AdapterBusIdMask);
	prom_printf("&MessageBuffer:	%x\n", &(DevExt->MessageBuffer));
	prom_printf("MessageCount:	%x\n", DevExt->MessageCount);
	prom_printf("MessageSent:	%x\n", DevExt->MessageSent);
	prom_printf("ClockSpeed:	%x\n", DevExt->ClockSpeed);
	prom_printf("TargetId:		%x\n", DevExt->TargetId);
	prom_printf("Lun:		%x\n", DevExt->Lun);
/*
 *	prom_printf("Configuration3:	%x\n", DevExt->Configuration3);
 */
/*
 *	prom_printf("Configuration4:	%x\n", DevExt->Configuration4);
 */
	prom_printf("ActiveSGEntry:	%x\n", DevExt->ActiveSGEntry);
	prom_printf("ActiveSGTEntryOffset:%x\n", DevExt->ActiveSGTEntryOffset);
	prom_printf("ActiveDataPointer:	%x\n", DevExt->ActiveDataPointer);
	prom_printf("ActiveDataLength:	%x\n", DevExt->ActiveDataLength);
	prom_printf("ActiveDMAPointer:	%x\n", DevExt->ActiveDMAPointer);
	prom_printf("ActiveDMALength:	%x\n", DevExt->ActiveDMALength);
	prom_printf("TempPhysicalBuf:	%x\n", DevExt->TempPhysicalBuf);
	prom_printf("TempLinearBuf:	%x\n", DevExt->TempLinearBuf);
	prom_printf("dataPhase:		%x\n", DevExt->dataPhase);
/*
 *	prom_printf("powerDownPost:	%x\n", DevExt->powerDownPost);
 */
/*
 *	prom_printf("cpuModePost:	%x\n", DevExt->cpuModePost);
 */
	prom_printf("ActiveLuRequest:	%x\n", DevExt->ActiveLuRequest);
	prom_printf("ActiveLogicalUnit:	%x\n",
						DevExt->ActiveLogicalUnit);
	prom_printf("NextSrbRequest:	%x\n", DevExt->NextSrbRequest);


/*
 *	prom_printf("PciConfigBase:	%x\n", DevExt->PciConfigBase);
 */
/*
 *	prom_printf("PciMappedBase:	%x\n", DevExt->PciMappedBase);   */
/*
 *	prom_printf("PciConfigInfo:	%x\n", DevExt->PciConfigInfo);
 */
	prom_printf("TargetInitFlags:	%x\n", DevExt->TargetInitFlags);
	prom_printf("SysFlags:		%x\n", DevExt->SysFlags);

#ifdef DISABLE_SREG
/*
 *	prom_printf("&SCSIState[8]:	%x\n", &(DevExt->SCSIState));
 */
#endif

/*
 *	prom_printf("pcscsi_blk_p:	%x\n", DevExt->pcscsi_blk_p);
 */


}


/* ========================================================================== */
#endif	/* PCSCSI_DEBUG  */
