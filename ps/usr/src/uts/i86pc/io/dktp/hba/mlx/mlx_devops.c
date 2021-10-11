/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)mlx_devops.c	1.2	96/05/27 SMI"

#include <sys/dktp/mlx/mlx.h>

/*
 * Local static data
 */

/* Autoconfiguration routines */


/*
 * probe(9E).  Examine hardware to see if HBA device is actually present.
 * Do no permanent allocations or permanent settings of device state,
 * as probe may be called more than once.
 * Return DDI_PROBE_SUCCESS if device is present and operable,
 * else return DDI_PROBE_FAILURE.
 */

int
mlx_probe(dev_info_t *devi)
{
	MDBG4(("mlx_probe\n"));
	/* Check for override */
	if (mlx_forceload < 0)
		return (DDI_PROBE_FAILURE);

	/* Check for a valid address and type of MLX HBA */
	if (mlx_hbatype(devi, NULL, NULL, TRUE) == NULL) {
		MDBG4(("mlx_probe: reg failed\n"));
		return (DDI_PROBE_FAILURE);
	}

	MDBG4(("mlx_probe: okay\n"));
	return (DDI_PROBE_SUCCESS);
}

/*
 * attach(9E).  Set up all device state and allocate data structures,
 * mutexes, condition variables, etc. for device operation.  Set mt-attr
 * property for driver to indicate MT-safety.  Add interrupts needed.
 * Return DDI_SUCCESS if device is ready,
 * else return DDI_FAILURE.
 */

/*
 * _attach() is serialized and single-threaded for all the channels, simply
 * because interrupts are shared by all the channels and we should disable
 * the interrupts at the beginning of _attach() and enable them at the end
 * of it.
 */
int
mlx_attach(dev_info_t	*dip,
		ddi_attach_cmd_t cmd)
{
	mlx_unit_t 	*unit;
	mlx_hba_t 	*hba;
	scsi_hba_tran_t	*scsi_tran;

	ASSERT(dip != NULL);
	switch (cmd) {
	case DDI_ATTACH:
		break;
	default:
		return (DDI_FAILURE);
	}

	/* Make a mlx_unit_t and mlx_hba_t instance for this HBA */
	unit = (mlx_unit_t *)kmem_zalloc(sizeof (*unit) + sizeof (*hba),
	    KM_NOSLEEP);
	if (unit == NULL)
		return (DDI_FAILURE);

	hba = (mlx_hba_t *)(unit + 1);
	unit->hba = hba;
	hba->dip = dip;

	/* find card structure */
	mutex_enter(&mlx_global_mutex);
	hba->mlx = mlx_cardfind(dip, unit);

	if (!hba->mlx) {
		kmem_free(unit, sizeof (*unit) + sizeof (*hba));
		mutex_exit(&mlx_global_mutex);
		return (DDI_FAILURE);
	}

	/* initialize card structure if needed */
	if (!(hba->mlx->flags & MLX_CARD_CREATED)) {
		if (!mlx_cardinit(dip, unit)) {
			mlx_carduninit(dip, unit);
			mutex_exit(&mlx_global_mutex);
			return (DDI_FAILURE);
		}
	} else {
		/* enter card mutex */
		mutex_enter(&hba->mlx->mutex);
	}

	/* fail attach of nonexistent nodes */
	if (hba->chn == MLX_DAC_CHN_NUM) {
#ifndef MSCSI_FEATURE
		/*
		 * When the mscsi bus child nexus driver is used,
		 * system drive instance must persist until all scsi
		 * channels are probed.
		 */
		if (!hba->mlx->conf->nsd) {
			MDBG3(("mlx_attach: causing attach failure to unload "
				"virtual channel instance"));
			mutex_exit(&hba->mlx->mutex);
			mlx_carduninit(dip, unit);
			mutex_exit(&mlx_global_mutex);
			return (DDI_FAILURE);
		}
#endif
		hba->flags = MLX_HBA_DAC;
	} else if (hba->chn >= hba->mlx->nchn) {
		mutex_exit(&hba->mlx->mutex);
		mlx_carduninit(dip, unit);
		mutex_exit(&mlx_global_mutex);
		return (DDI_FAILURE);
	}

	/*
	 * check mlx.conf properties, set defaults.
	 */
	if (!mlx_propinit(dip, unit)) {
		mutex_exit(&hba->mlx->mutex);
		mlx_carduninit(dip, unit);
		mutex_exit(&mlx_global_mutex);
		return (DDI_FAILURE);
	}

	/* allocate transport */
	scsi_tran = scsi_hba_tran_alloc(dip, 0);
	unit->scsi_tran = scsi_tran;
	if (scsi_tran == NULL) {
		mutex_exit(&hba->mlx->mutex);
		mlx_carduninit(dip, unit);
		mutex_exit(&mlx_global_mutex);
		cmn_err(CE_WARN, "mlx_attach: failed to alloc scsi tran");
		return (DDI_FAILURE);
	}

	scsi_tran->tran_hba_private = unit;
	scsi_tran->tran_tgt_private = NULL;
	scsi_tran->tran_tgt_init = mlx_tgt_init;
	scsi_tran->tran_tgt_free = mlx_tgt_free;

	if (MLX_DAC(hba)) {
		unit->dac_unit.lkarg = (void *)hba->mlx->iblock_cookie;
	} else {
		scsi_tran->tran_tgt_probe = scsi_hba_probe;
		scsi_tran->tran_start = mlx_transport;
		scsi_tran->tran_reset = mlx_reset;
		scsi_tran->tran_abort = mlx_abort;
		scsi_tran->tran_getcap = mlx_getcap;
		scsi_tran->tran_setcap = mlx_setcap;
		scsi_tran->tran_init_pkt = mlx_init_pkt;
		scsi_tran->tran_destroy_pkt = mlx_destroy_pkt;
		scsi_tran->tran_dmafree = mlx_dmafree;
		scsi_tran->tran_sync_pkt = mlx_sync_pkt;
	}

	if (scsi_hba_attach(dip, MLX_DAC(hba) ? &mlx_dac_dma_lim : &mlx_dma_lim,
	    scsi_tran, SCSI_HBA_TRAN_CLONE, NULL) != DDI_SUCCESS) {
		mutex_exit(&hba->mlx->mutex);
		mlx_carduninit(dip, unit);
		mutex_exit(&mlx_global_mutex);
		cmn_err(CE_WARN, "mlx_attach: failed to scsi_hba_attach(%x)",
		    (int)dip);
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);

	/*
	 * Although we can reset the channel here it is not safe to do so,
	 * in case the System-Drive hba is operational.
	 */
	hba->pkt_pool = NULL;
	hba->ccb_pool = NULL;
	hba->callback_id = 0;
	hba->flags |= MLX_HBA_ATTACHED;

	MLX_ENABLE_INTR(hba->mlx);
	mutex_exit(&hba->mlx->mutex);
	mutex_exit(&mlx_global_mutex);

	return (DDI_SUCCESS);
}

/*
 * detach(9E).  Remove all device allocations and system resources;
 * disable device interrupts.
 * Return DDI_SUCCESS if done; DDI_FAILURE if there's a problem.
 */
int
mlx_detach(dev_info_t	*dip,
		ddi_detach_cmd_t cmd)
{
	register mlx_unit_t *unit;
	register mlx_hba_t *hba;
	register scsi_hba_tran_t *scsi_tran;

	ASSERT(dip != NULL);

	if (cmd != DDI_DETACH)
		return (DDI_FAILURE);

	scsi_tran = (scsi_hba_tran_t *)ddi_get_driver_private(dip);
	if (scsi_tran == NULL)
		return (DDI_SUCCESS);

	unit = MLX_SCSI_TRAN2UNIT(scsi_tran);
	if (unit == NULL)
		return (DDI_SUCCESS);

	MDBG1(("mlx_detach: dip=%x, unit=%x", dip, unit));
	hba = unit->hba;
	ASSERT(hba != NULL);

	mutex_enter(&mlx_global_mutex);
	if ((hba->refcount-1) > 0) {
		mutex_exit(&mlx_global_mutex);
		return (DDI_FAILURE);
	}

	mutex_destroy(&hba->mutex);
	ddi_prop_remove_all(dip);
	if (scsi_hba_detach(dip) != DDI_SUCCESS)
		cmn_err(CE_WARN, "mlx_detach: failed to scsi_hba_detach(%x)",
		    (int)dip);

	mlx_carduninit(dip, unit);
	mutex_exit(&mlx_global_mutex);
	return (DDI_SUCCESS);
}

/*ARGSUSED*/
int
mlx_flush_cache(register dev_info_t *const dip, const ddi_reset_cmd_t cmd)
{
	register mlx_unit_t *unit;
	register scsi_hba_tran_t *scsi_tran;
	register mlx_hba_t *hba;


	ASSERT(dip != NULL);

	if (cmd != (ddi_reset_cmd_t)DDI_DETACH)
		return (DDI_FAILURE);

	scsi_tran = (scsi_hba_tran_t *)ddi_get_driver_private(dip);
	if (scsi_tran == NULL)
		return (DDI_SUCCESS);
	unit = MLX_SCSI_TRAN2UNIT(scsi_tran);
	if (unit == NULL)
		return (DDI_SUCCESS);

	MDBG1(("mlx_flush_cache: dip=%x, unit=%x", dip, unit));

	hba = unit->hba;
	ASSERT(hba != NULL);

	return (MLX_SCSI(hba) ? 0 :
	    mlx_dacioc(unit, MLX_DACIOC_FLUSH, NULL, FKIOCTL));
}

/*ARGSUSED*/
bool_t
mlx_propinit(dev_info_t	*dip,
		mlx_unit_t	*unitp)
{
/*	set up default customary properties 				*/
	if (unitp->hba->chn == MLX_DAC_CHN_NUM) {
		if (mlx_prop_default(dip, "flow_control", "dmult") == 0 ||
		    mlx_prop_default(dip, "queue", (caddr_t)"qfifo") == 0 ||
		    mlx_prop_default(dip, "disk", (caddr_t)"dadk") == 0 ||
#ifdef MSCSI_FEATURE
	/*
	 * when the mscsi bus nexus child driver is used, the MSCSI_CALLPROP
	 * property must be set indicating the mscsi child should callback
	 * the mlx parent devops for channel init and uninit.
	 */
		    mlx_prop_default(dip, MSCSI_CALLPROP, (caddr_t)"y") == 0 ||
#endif
		    mlx_prop_default(dip, "disk", (caddr_t)"dadk") == 0)
			return (FALSE);
	} else if (mlx_prop_default(dip, "flow_control", "dsngl") == 0 ||
		mlx_prop_default(dip, "queue", (caddr_t)"qsort") == 0 ||
		mlx_prop_default(dip, "tape", (caddr_t)"sctp") == 0 ||
		mlx_prop_default(dip, "tag_fctrl", (caddr_t)"adapt") == 0 ||
		mlx_prop_default(dip, "tag_queue", (caddr_t)"qtag") == 0 ||
		mlx_prop_default(dip, "disk", (caddr_t)"scdk") == 0) {
			return (FALSE);
	}

	MDBG5(("mlx_propinit: okay\n"));
	return (TRUE);
}

/*
 * mlx_prop_default: set default property if unset.
 * if plen > 0, search globally, and add non-string property.
 */
bool_t
mlx_prop_default(dev_info_t	*dip,
			caddr_t		 propname,
			caddr_t		 propdefault)
{
	caddr_t	val;
	int len;

	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS,
		propname, (caddr_t)&val, &len) == DDI_PROP_SUCCESS) {
		kmem_free(val, len);
		return (TRUE);
	}

	if (ddi_prop_create(DDI_DEV_T_NONE, dip, 0, propname, propdefault,
		strlen(propdefault) + 1) != DDI_PROP_SUCCESS) {
		MDBG4(("mlx_prop_default: property create failed %s=%s\n",
			propname, propdefault));
		return (FALSE);
	}
	return (TRUE);
}

mlx_t *
mlx_cardfind(dev_info_t	*dip,
		mlx_unit_t	*unit)
{
	register mlx_t *mlx;
	register mlx_t *mlx_prev;
	int chn;
	int len;
	int *regp;
	int reglen;
	dev_info_t *pdip = dip;
	mlx_t tmlx;

#ifdef MSCSI_FEATURE
	/*
	 * when the mscsi bus nexus child driver is used, the parent
	 * dip must be used for hardware probe functions.
	 */
	if (strcmp(MSCSI_NAME, ddi_get_name(dip)) == 0)
		pdip = ddi_get_parent(dip);
#endif

	/* Determine type of mlx card */
	if ((tmlx.ops = mlx_hbatype(pdip, &regp, &reglen, FALSE)) == NULL) {
		MDBG4(("mlx_cardfind: no hbatype match\n"));
		return (NULL);
	}

	/* Determine physical ioaddr from reg of appropriate dip */
	tmlx.dip = pdip;
	if (MLX_RNUMBER(&tmlx, regp, reglen) < 0) {
		MDBG4(("mlx_cardfind: no reg match\n"));
		return (NULL);
	}

	/* Determine which channel the HBA should use */
	len = sizeof (chn);
	if (HBA_INTPROP(dip, MSCSI_BUSPROP, &chn, &len) != DDI_PROP_SUCCESS) {
		len = sizeof (chn);
		if (ddi_getlongprop_buf(DDI_DEV_T_NONE, ddi_get_parent(dip),
		    DDI_PROP_DONTPASS, MSCSI_BUSPROP, (caddr_t)&chn, &len) !=
		    DDI_PROP_SUCCESS) {
			/*
			 * If no MSCSI_BUSPROP property exists,
			 * set default channel based on ops and reg property
			 */
			chn = MLX_CHN(&tmlx, *regp);
		}
		len = sizeof (chn);
		if (ddi_prop_create(DDI_DEV_T_NONE, dip, 0, "mcsci-bus",
			(caddr_t)&chn, len) != DDI_PROP_SUCCESS) {
			MDBG4(("mlx_cardfind: property create failed %s=%s\n",
			MSCSI_BUSPROP, chn));
			kmem_free(regp, reglen);
			return (NULL);
		}
	}

	/* find or add card struct of matching type and reg address  */
	for (mlx_prev = mlx = mlx_cards; ; mlx_prev = mlx, mlx = mlx->next)
		if (mlx == NULL || (mlx->ops == tmlx.ops &&
		    mlx->reg == tmlx.reg))
			break;
	if (mlx == NULL) {
		mlx = (mlx_t *)kmem_zalloc(sizeof (*mlx), KM_NOSLEEP);
		if (mlx == NULL) {
			MDBG4(("mlx_cardfind: no mlx\n"));
			kmem_free(regp, reglen);
			return (NULL);
		}
		mlx->ops = tmlx.ops;
		mlx->reg = tmlx.reg;
		mlx->regp = regp;
		mlx->reglen = reglen;
		mlx->dip = mlx->idip = dip;
		mlx->attach_calls = -1;

		if (mlx_prev != NULL)
			mlx_prev->next = mlx;
		else
			mlx_cards = mlx;
	} else
		kmem_free(regp, reglen);

	mlx->refcount++;

	unit->hba->mlx = mlx;
	unit->hba->chn = (u_char)chn;

	return (mlx);
}

/*
 * Creates and initializes an mlx structure representing a DAC960 card
 * and its software state.
 */
bool_t
mlx_cardinit(dev_info_t	*dip,
		mlx_unit_t	*unitp)
{
	int val;
	int len;
	mlx_t *mlxp = unitp->hba->mlx;

	/*
	 * check mlx.conf properties, and initialize all potential
	 * per-target structures.  Also, initialize chip's operating
	 * registers to a known state.
	 */
	if (!mlx_cfg_init(dip, mlxp))
		return (FALSE);
	mlxp->flags = MLX_CARD_CREATED;

	/* disable interrupts */
	MLX_DISABLE_INTR(mlxp);

	/* setup interrupts */
	if (!mlx_intr_init(mlxp->idip, mlxp, (caddr_t)mlxp))
		return (FALSE);
	mlxp->flags |= (MLX_INTR_SET|MLX_INTR_IDX_SET);

	/* enter card mutex */
	mutex_enter(&mlxp->mutex);

	/* setup cv and sema */
	cv_init(&mlxp->ccb_stk_cv, "MLX Per Card Condvar", CV_DRIVER, NULL);
	sema_init(&mlxp->scsi_ncdb_sema, MLX_SCSI_MAX_NCDB,
			"MLX SCSI Direct CDB Semaphore", SEMA_DRIVER, NULL);

	/* determine which target number the HBA should use */
	len = sizeof (val);
	if (HBA_INTPROP(dip, "scsi-initiator-id", &val, &len)
				== DDI_PROP_SUCCESS) {
		mlxp->initiatorid = (u_char)val;

	} else {
		/* default the initiator id to 7 */
		mlxp->initiatorid = 7;
	}

	/* pre-allocate ccb */
	if (mlx_ccbinit(unitp->hba) == DDI_FAILURE) {
		mutex_exit(&mlxp->mutex);
		return (FALSE);
	}

	/* determine the maximum channel and target */
	if (!mlx_getnchn_maxtgt(mlxp, unitp->hba->ccb)) {
		mutex_exit(&mlxp->mutex);
		return (FALSE);
	}

	if (mlx_getconf(mlxp, unitp->hba->ccb) == DDI_FAILURE) {
		mutex_exit(&mlxp->mutex);
		return (FALSE);
	}
	mlxp->flags |= MLX_GOT_ROM_CONF;

	if (mlx_getenquiry(mlxp, unitp->hba->ccb) == DDI_FAILURE) {
		mutex_exit(&mlxp->mutex);
		return (FALSE);
	}
	mlxp->flags |= MLX_GOT_ENQUIRY;

	if (mlx_ccb_stkinit(mlxp) == DDI_FAILURE) {
		mutex_exit(&mlxp->mutex);
		return (FALSE);
	}
	mlxp->flags |= MLX_CCB_STK_CREATED;

	mlxp->scsi_cbthdl = scsi_create_cbthread(mlxp->iblock_cookie,
		KM_NOSLEEP);
	if (mlxp->scsi_cbthdl == NULL) {
		mutex_exit(&mlxp->mutex);
		return (FALSE);
	}
	mlxp->flags |= MLX_CBTHD_CREATED;

	mlxp->attach_calls++;

	return (TRUE);
}

/*
 * Frees all the non-shared resources of an mlx_unit_t and enables interrupts
 * only if enable_intr is set.
 */
void
mlx_carduninit(dev_info_t	*dip,
		mlx_unit_t	*unitp)
{
	register mlx_t *mlx = unitp->hba->mlx;
	register mlx_hba_t *hba = unitp->hba;

	if (unitp->scsi_tran != NULL)
		scsi_hba_tran_free(unitp->scsi_tran);
	if (hba->ccb != NULL)
		ddi_iopb_free((caddr_t)hba->ccb);

	if (mlx != NULL && !--mlx->refcount &&
	    (!(mlx->flags & MLX_CCB_STK_CREATED) ||	/* not useful info */
		mlx->attach_calls >= mlx->nchn)) {	/* very last one */
		ASSERT(mlx_cards != NULL);
		if (mlx_cards == mlx) {
			mlx_cards = mlx->next;
		} else {
			register mlx_t *mlx_tmp;
			register mlx_t *mlx_prev;

			for (mlx_prev = mlx_cards, mlx_tmp = mlx_cards->next;
			    mlx_tmp != NULL;
			    mlx_prev = mlx_tmp, mlx_tmp = mlx_tmp->next)
				if (mlx_tmp == mlx)
					break;
			if (mlx_tmp == NULL)
				cmn_err(CE_PANIC, "mlx_unsetup: %x not in "
				    "cards list %x", (int)mlx, (int)mlx_cards);
			mlx_prev->next = mlx->next;
		}

		if (mlx->flags & MLX_GOT_ROM_CONF) {
			size_t mem =	sizeof (mlx_dac_conf_t) -
					sizeof (mlx_dac_tgt_info_t) +
					    ((mlx->nchn * mlx->max_tgt) *
						sizeof (mlx_dac_tgt_info_t));

			kmem_free((caddr_t)mlx->conf, mem);
		}

		if (mlx->flags & MLX_GOT_ENQUIRY) {
			kmem_free((caddr_t)mlx->enq,
			    sizeof (mlx_dac_enquiry_t));
		}

		if (mlx->flags & MLX_CCB_STK_CREATED) {
			kmem_free((caddr_t)mlx->ccb_stk, (mlx->max_cmd + 1) *
				sizeof (*mlx->ccb_stk));
		}

		if (mlx->flags & MLX_INTR_SET) {
			ddi_remove_intr(mlx->idip, mlx->intr_idx,
					mlx->iblock_cookie);
			cv_destroy(&mlx->ccb_stk_cv);
			mutex_destroy(&mlx->mutex);
			sema_destroy(&mlx->scsi_ncdb_sema);
		}

		if (mlx->flags & MLX_CBTHD_CREATED)
			scsi_destroy_cbthread(mlx->scsi_cbthdl);

		if (mlx->regp)
			kmem_free(mlx->regp, mlx->reglen);

		if (mlx->flags & MLX_CARD_CREATED)
			MLX_UNINIT(mlx, dip);

		/*
		 * No need to reset all the channels the F/W
		 * takes care of that.
		 */
		kmem_free((caddr_t)mlx, sizeof (*mlx));

		/*
		 * No need to set unitp->hba->mlx to NULL,
		 * the whole hba and unit is going to be freed.
		 */
	} else {
		/* re-enable intrs */
		MLX_ENABLE_INTR(mlx);
	}
	kmem_free((caddr_t)unitp, sizeof (*unitp) + sizeof (*hba));
}
