/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)devops.c	1.12	96/06/03 SMI"

#include <sys/dktp/ncrs/ncr.h>


/* The name of the device, for identify */
#define HBANAME	"ncrs"

extern kmutex_t ncr_global_mutex;
extern ddi_dma_lim_t ncr_dma_lim;

/*
 * Local static data
 */
static int ncr_global_init = 0;

/* Autoconfiguration routines */

/* 
 * identify(9E).  See if driver matches this dev_info node.
 * Return DDI_IDENTIFIED if ddi_get_name(devi) matches your
 * name, otherwise return DDI_NOT_IDENTIFIED.
 */

int
ncr_identify( dev_info_t *devi )
{
	char *dname = ddi_get_name(devi);

	NDBG3(("ncr_identify\n"));

	if (strcmp(dname, HBANAME) == 0) 
		return (DDI_IDENTIFIED);
	else if (strcmp(dname, "pci1000,1") == 0) 
		return (DDI_IDENTIFIED);
	else if (strcmp(dname, "pci1000,2") == 0) 
		return (DDI_IDENTIFIED);
	else if (strcmp(dname, "pci1000,3") == 0) 
		return (DDI_IDENTIFIED);
	else if (strcmp(dname, "pci1000,4") == 0) 
		return (DDI_IDENTIFIED);
	else 
		return (DDI_NOT_IDENTIFIED);
}


/* 
 * probe(9E).  Examine hardware to see if HBA device is actually present.  
 * Do no permanent allocations or permanent settings of device state,
 * as probe may be called more than once.
 * Return DDI_PROBE_SUCCESS if device is present and operable, 
 * else return DDI_PROBE_FAILURE.
 */

int
ncr_probe( dev_info_t *devi )
{
	NDBG4(("ncr_probe\n"));

	/* Check for a valid address and type of NCR HBA */
	if (ncr_hbatype(devi, NULL, NULL, NULL, TRUE) != NULL) {
		NDBG4(("ncr_probe: okay\n"));
		return (DDI_PROBE_SUCCESS);
	}

	NDBG4(("ncr_probe: failed\n"));
	return (DDI_PROBE_FAILURE);
}

/* 
 * attach(9E).  Set up all device state and allocate data structures, 
 * mutexes, condition variables, etc. for device operation.  Set mt-attr
 * property for driver to indicate MT-safety.  Add interrupts needed.
 * Return DDI_SUCCESS if device is ready, else return DDI_FAILURE.
 */

int
ncr_attach(	dev_info_t	*devi,
		ddi_attach_cmd_t cmd )
{
	np_t	*np;
	ncr_t		*ncrp;
	nops_t		**nopsp;
	int 			mt;
	scsi_hba_tran_t		*hba_tran;

	NDBG5(("ncr_attach\n"));

	switch (cmd) {

	case DDI_ATTACH:
		break;

	default:
		return (DDI_FAILURE);
	}

	/* Make a ncr_t instance for this HBA */
	np = (struct ncr *)kmem_zalloc(sizeof (*np) + sizeof (*ncrp),
		KM_NOSLEEP);
	if (!np)
		return (DDI_FAILURE);

	ncrp = (ncr_t *)(np + 1);
	NCR_BLKP(np) = ncrp;
	ncrp->n_dip = devi;		/* save the dev_info pointer */
	np->n_unitp = &ncrp->n_pt[0];


	/*
	 * Initialize scripts once only, for all instances of this driver.
	 */
	mutex_enter(&ncr_global_mutex);	/* protect multithreaded attach */
	if (ncr_global_init++ == 0) {
		for (nopsp = &ncr_conf[0]; *nopsp != NULL; nopsp++) {

			if ((*nopsp)->ncr_script_init() == FALSE) {
				cmn_err(CE_WARN,
				    "ncr_attach: script init failed");
				goto err_exit1;
			}
		}
	}
	mutex_exit(&ncr_global_mutex);

	/*
	 * check ncr.conf properties, and initialize all potential
	 * per-target structures.  Also, initialize chip's operating
	 * registers to a known state.
	 */
	if (!ncr_propinit(devi, ncrp) ||
	    !ncr_cfg_init(devi, ncrp) ||
	    !ncr_intr_init(devi, ncrp, (caddr_t)np)) {
		goto err_exit1;
	}
	if (!ncr_hba_init(ncrp)) {
		goto err_exit2;
	}

	/*
	 * No DMA setup required here - this device is a bus master.
	 */

	/*
	 * Allocate a transport structure
	 */
	if ((hba_tran = scsi_hba_tran_alloc(devi, 0)) == NULL) {
		cmn_err(CE_WARN, "ncr_attach: scsi_hba_tran_alloc failed\n");
		goto err_exit3;
	}

	/*
	 * Initialize the transport structure
	 */
	np->n_tran = hba_tran;

	hba_tran->tran_hba_private	= np;
	hba_tran->tran_tgt_private	= &ncrp->n_pt[0];

	hba_tran->tran_tgt_init		= ncr_tran_tgt_init;
	hba_tran->tran_tgt_probe	= ncr_tran_tgt_probe;
	hba_tran->tran_tgt_free		= ncr_tran_tgt_free;

	hba_tran->tran_start 		= ncr_transport;
	hba_tran->tran_reset		= ncr_reset;
	hba_tran->tran_abort		= ncr_abort;
	hba_tran->tran_getcap		= ncr_getcap;
	hba_tran->tran_setcap		= ncr_setcap;
	hba_tran->tran_init_pkt		= ncr_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= ncr_tran_destroy_pkt;

	hba_tran->tran_dmafree		= ncr_dmafree;
	hba_tran->tran_sync_pkt		= ncr_sync_pkt;
	hba_tran->tran_reset_notify	= NULL;

	if (scsi_hba_attach(devi, &ncr_dma_lim, hba_tran, SCSI_HBA_TRAN_CLONE,
	    NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "ncr_attach: scsi_hba_attach failed");
		goto err_exit4;
	}

#if 1
	ncrp->n_cbthdl = scsi_create_cbthread(ncrp->n_iblock, KM_NOSLEEP);
#endif

	/* Let the system know we're MT-safe */
	mt = D_MP;
	(void)ddi_prop_create(DDI_DEV_T_NONE, devi, DDI_PROP_CANSLEEP,
	    "mt-attr", (caddr_t)&mt, sizeof (mt));

	/* Print message about HBA being alive at address such-and-such */
	ddi_report_dev(devi);

	/* enable the interrupts and the interrupt handler */
	NCR_ENABLE_INTR(ncrp);

	NDBG5(("ncr_attach okay\n"));
	mutex_exit(&ncrp->n_mutex);
	return (DDI_SUCCESS);


err_exit4:
	/* free the transport structure */
	scsi_hba_tran_free(hba_tran);

err_exit3:
	/* free all the buffers */
	ncr_hba_uninit(ncrp);

err_exit2:
	/* remove the interrupt handler */
	ddi_remove_intr(devi, ncrp->n_inumber, ncrp->n_iblock);

	/* destroy the mutex */
	mutex_destroy(&ncrp->n_mutex);

err_exit1:
	/* free the SCRIPTS buffers */
	mutex_enter(&ncr_global_mutex);
	ncr_global_init--;
	if (ncr_global_init == 0) {
		for (nopsp = &ncr_conf[0]; *nopsp != NULL; nopsp++)
			(*nopsp)->ncr_script_fini();
	}
	mutex_exit(&ncr_global_mutex);

	kmem_free((caddr_t)np, sizeof (*np) + sizeof (*ncrp));
	return (DDI_FAILURE);
}


/* 
 * detach(9E).  Remove all device allocations and system resources; 
 * disable device interrupts.
 * Return DDI_SUCCESS if done; DDI_FAILURE if there's a problem.
 */

int 
ncr_detach(	dev_info_t	*devi,
		ddi_detach_cmd_t cmd )
{
	struct ncr	*np;
	ncr_t		*ncrp;
	nops_t		**nopsp;

	NDBG6(("ncr_detach\n"));
	switch (cmd) {
	case DDI_DETACH:
	{
		scsi_hba_tran_t *tran;

		tran = (scsi_hba_tran_t *)ddi_get_driver_private(devi);
		if (!tran) 
			return (DDI_SUCCESS);

		np = TRAN2NCR(tran);
		if (!np)
			return (DDI_SUCCESS);

		ncrp = NCR_BLKP(np);

		/* Disable HBA interrupts in hardware */
		NCR_DISABLE_INTR(ncrp);

		/* If driver creates any properties */
		ddi_prop_remove_all(devi);

		if (scsi_hba_detach(devi) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "ncr: scsi_hba_detach failed\n");
		}

#if 1
		/* remove the packet-complete callback thread */
		scsi_destroy_cbthread(ncrp->n_cbthdl);
#endif

		/* free the transport structure */
		scsi_hba_tran_free(np->n_tran);

		/* free all the buffers */
		ncr_hba_uninit(ncrp);

		/* remove the handler */
		ddi_remove_intr(devi, ncrp->n_inumber, ncrp->n_iblock);

		mutex_destroy(&ncrp->n_mutex);

		/* unmap the device */
		ddi_regs_map_free(&ncrp->n_handle);

		/* free the SCRIPTS buffers */
		mutex_enter(&ncr_global_mutex);
		ncr_global_init--;
		if (ncr_global_init == 0) {
			for (nopsp = &ncr_conf[0]; *nopsp != NULL; nopsp++)
				(*nopsp)->ncr_script_fini();
		}
		mutex_exit(&ncr_global_mutex);

		if (ncrp->n_regp)
			kmem_free((caddr_t)ncrp->n_regp, ncrp->n_reglen);

		kmem_free((caddr_t)np, sizeof(*np) + sizeof (*ncrp));

		NDBG6(("ncr_detach okay\n"));
		return (DDI_SUCCESS);
	}
	default:
		NDBG6(("ncr_detach not okay\n"));
		return (DDI_FAILURE);
	}
}


static bool_t
ncr_propinit(	dev_info_t	*dip,
		ncr_t		*ncrp )
{
	int	*discop;
	int	 val;
	int	 len;
	
	NDBG5(("ncr_propinit\n"));

	if (!ncr_prop_default(dip, "flow_control", "dsngl")
	||  !ncr_prop_default(dip, "queue", "qsort")
	||  !ncr_prop_default(dip, "disk", "scdk")
	||  !ncr_prop_default(dip, "tape", "sctp")) {   
		return (FALSE);
	}

	/* determine which target number the HBA should use */
	len = sizeof(val);
	if (HBA_INTPROP(dip, "scsi-initiator-id", &val, &len)
				== DDI_PROP_SUCCESS) {
		ncrp->n_initiatorid = val;

	} else {
		/* default the initiator id to 7 */
		ncrp->n_initiatorid = 7;
	}

	/* convert the target number to its corresponding bit position */
	ncrp->n_idmask = 1 << ncrp->n_initiatorid;


	/* check if disconnects are disabled on any targets */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS
				, "no-disconnect", (caddr_t)&discop, &len)
	== DDI_PROP_SUCCESS) {
		int	*tmpp = discop;
		int	 nvals;

		for (nvals = len / sizeof (int); nvals > 0; nvals--, tmpp++) {
			int	target = *tmpp;
			
			if (target >= NTARGETS)
				continue;
			ncrp->n_nodisconnect[target] = TRUE;
		}
		kmem_free(discop, len);
	}

	NDBG5(("ncr_propinit: okay\n"));
	return (TRUE);
}

static bool_t
ncr_prop_default(	dev_info_t	*dip,
			caddr_t		 propname,
			caddr_t		 propdefault )
{
	caddr_t	val;
	int	len;

	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS
		, propname, (caddr_t)&val, &len) == DDI_PROP_SUCCESS) {
		kmem_free(val, len);
		return (TRUE);
	}

	if (ddi_prop_create(DDI_DEV_T_NONE, dip, 0, propname, propdefault
		  , strlen(propdefault) + 1) == DDI_PROP_SUCCESS)
		return (TRUE);

	NDBG4(("ncr_prop_default: property create failed %s=%s\n"
		, propname, propdefault));
	return (FALSE);
}
