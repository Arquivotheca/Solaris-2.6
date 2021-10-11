/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)mlx_transport.c	1.5	96/07/28 SMI"

#include <sys/dktp/mlx/mlx.h>

int
mlx_tgt_init(
    dev_info_t *const hba_dip,
    dev_info_t *const tgt_dip,
    register scsi_hba_tran_t *const hba_tran,
    struct scsi_device *const sd)
{
	register mlx_hba_t *hba;

	ASSERT(hba_tran != NULL);
	hba = MLX_SCSI_TRAN2HBA(hba_tran);
	ASSERT(hba != NULL);

	return ((MLX_DAC(hba)) ?
		mlx_dac_tran_tgt_init(hba_dip, tgt_dip, hba_tran, sd) :
		mlx_tran_tgt_init(hba_dip, tgt_dip, hba_tran, sd));
}

/*ARGSUSED*/
void
mlx_tgt_free(
    register dev_info_t *const hba_dip,
    register dev_info_t *const tgt_dip,
    register scsi_hba_tran_t *const hba_tran,
    register struct scsi_device *const sd)
{
	size_t size;
	mlx_hba_t *hba;
	mlx_unit_t *child_unit;

	ASSERT(hba_dip != NULL);
	ASSERT(tgt_dip != NULL);
	ASSERT(hba_tran != NULL);
	hba = MLX_SCSI_TRAN2HBA(hba_tran);
	ASSERT(hba != NULL);

	MDBG2(("mlx_tgt_free: %s%d %s%d\n",
	    ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
	    ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip)));

	size = sizeof (mlx_unit_t);
	child_unit = hba_tran->tran_tgt_private;
	ASSERT(child_unit != NULL);
	if (MLX_DAC(hba)) {
		size += sizeof (struct ctl_obj);
		if (sd->sd_inq != NULL) {
			kmem_free((caddr_t)sd->sd_inq,
			    sizeof (struct scsi_inquiry));
		}
		sd->sd_inq = NULL;
	}
	kmem_free(child_unit, size);

	mutex_enter(&hba->mutex);
	hba->refcount--;		/* decrement active children */
	mutex_exit(&hba->mutex);
}

int
mlx_tgt_probe(
    register struct scsi_device *const sd,
    int (*const callback)())
{
	int rval = SCSIPROBE_FAILURE;
	char *s;
	register mlx_hba_t *hba;
	int tgt;

	ASSERT(sd != NULL);
	hba = MLX_SDEV2HBA(sd);
	ASSERT(hba != NULL);
	ASSERT(MLX_SCSI(hba));

	tgt = sd->sd_address.a_target;

	if (sd->sd_address.a_lun) {
		MDBG2(("mlx%d: target-zero %d lun!=0 %d\n",
			ddi_get_instance(hba->dip),
			tgt, sd->sd_address.a_lun));
		return (rval);
	}

	if (mlx_dont_access(hba->mlx, hba->chn, (u_char)tgt))
		return (rval);

	rval = scsi_hba_probe(sd, callback);

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
		s = "???";
		break;
	}

	MDBG2(("mlx%d: %s target %d lun %d %s\n",
	    ddi_get_instance(hba->dip), ddi_get_name(sd->sd_dev),
	    sd->sd_address.a_target, sd->sd_address.a_lun, s));

	return (rval);
}

/*
 * Execute a SCSI command during init time using no interrupts or
 * command overlapping.
 */
int
mlx_init_cmd(register mlx_t *const mlx, register mlx_ccb_t *const ccb)
{
	ASSERT(mlx != NULL);
	ASSERT(ccb != NULL);
	ASSERT(mutex_owned(&mlx_global_mutex));

	if (mlx_sendcmd(mlx, ccb, 1) == DDI_FAILURE ||
	    mlx_pollstat(mlx, ccb, 1) == DDI_FAILURE)
		return (DDI_FAILURE);

	if (ccb->ccb_status == MLX_BAD_OPCODE) {
		cmn_err(CE_WARN, "mlx_init_cmd: bad opcode %x for cmdid %d",
			ccb->ccb_opcode, ccb->ccb_cmdid);
		return (DDI_FAILURE);
	}
	return (DDI_SUCCESS);
}

/*
 * init = 1	=> called during initialization from mlx_init_cmd()
 * init = 0	=> called from mlx_{scsi,dac}_transport()
 */
int
mlx_sendcmd(
    register mlx_t *const mlx,
    register mlx_ccb_t *const ccb,
    int init)
{
	int cntr;

	ASSERT(ccb != NULL);
	ASSERT(mlx != NULL);

	if (init) {
		ASSERT(mutex_owned(&mlx_global_mutex));
		/*
		 * Need not change cmdid during init, as mlx_attach()
		 * is serialized and polls on the status of cmd's 1 by 1.
		 */
		ccb->ccb_cmdid = MLX_INVALID_CMDID;
	} else {
		ASSERT(mutex_owned(&mlx->mutex));
		if (mlx->free_ccb->next == MLX_INVALID_CMDID)
			cv_wait_sig(&mlx->ccb_stk_cv, &mlx->mutex);
		ASSERT(mlx->free_ccb->ccb == NULL);
		ccb->ccb_cmdid = mlx->free_ccb - mlx->ccb_stk;
	}
	ccb->ccb_stat_id = ccb->ccb_cmdid;

	/* Check if command mail box is free, if not retry. */
	for (cntr = MLX_MAX_RETRY; cntr > 0; cntr--) {
		if (MLX_CREADY(mlx))
			break;
		if (cntr & 1)
			drv_usecwait(1);
	}
	if (!cntr) {
		cmn_err(CE_WARN, "mlx_sendcmd: not ready to accept "
		    "commands, retried %u times", MLX_MAX_RETRY);
		return (DDI_FAILURE);
	}

	/* Issue the command. */
	if (!MLX_CSEND(mlx, (void *)ccb))
		return (DDI_FAILURE);

	if (init)
		return (DDI_SUCCESS);

	/* Push the ccb onto the ccb_stk stack */
	ASSERT(mutex_owned(&mlx->mutex));
	ASSERT(mlx->free_ccb->ccb == NULL);
	mlx->free_ccb->ccb = ccb;
	mlx->free_ccb = mlx->ccb_stk + mlx->free_ccb->next;
	return (DDI_SUCCESS);
}

/*
 * init = 1	=> called during initialization from mlx_init_cmd()
 * init = 0	=> called from mlx_{scsi,dac}_transport()
 */
int
mlx_pollstat(
    register mlx_t *const mlx,
    register mlx_ccb_t *const ccb,
    int init)
{
	int cntr;
	int retry;
	register mlx_ccb_stk_t *ccb_stk;

	ASSERT(ccb != NULL);
	ASSERT(mlx != NULL);

	ASSERT(mutex_owned((init) ? &mlx_global_mutex : &mlx->mutex));

	/* check if intr handled this */
	ccb->intr_wanted = 1;

	for (retry = MLX_MAX_RETRY; retry > 0; drv_usecwait(10), retry--) {
		for (cntr = MLX_MAX_RETRY; cntr > 0; drv_usecwait(10), cntr--) {
			if (MLX_IREADY(mlx))
				break;
		}
		if (!cntr) {
			cmn_err(CE_WARN, "mlx_pollstat: status not ready, "
			    "retried %u times", MLX_MAX_RETRY);
			return (DDI_FAILURE);
		}

		MLX_GET_ISTAT(mlx, (void *)&ccb->ccb_stat_id, 0);

		if (ccb->ccb_stat_id == ccb->ccb_cmdid) {

			MLX_GET_ISTAT(mlx, NULL, 1);

			if (!init) {

				ccb_stk = mlx->ccb_stk + ccb->ccb_stat_id;
				ASSERT(ccb_stk->ccb == ccb);

				ccb_stk->next = mlx->free_ccb - mlx->ccb_stk;
				mlx->free_ccb = ccb_stk;
				ccb_stk->ccb = NULL;
				cv_signal(&mlx->ccb_stk_cv);

				if (ccb->type == MLX_SCSI_CTYPE) {
					sema_v(&mlx->scsi_ncdb_sema);
					ASSERT(ccb->ccb_ownerp != NULL);
					mlx_chkerr(mlx, ccb,
					    (struct scsi_pkt *)ccb->ccb_ownerp,
					    ccb->ccb_status);
				} else {
					register struct cmpkt *cmpkt =
					    (struct cmpkt *)ccb->ccb_ownerp;

					ASSERT(cmpkt != NULL);
					if (ccb->ccb_status) {
						cmn_err(CE_CONT,
						    "?mlx_pollstat: DAC960 "
						    "opcode=0x%x, error "
						    "status=0x%x",
						    ccb->ccb_opcode,
						    ccb->ccb_status);
						cmpkt->cp_reason = CPS_CHKERR;
						((mlx_dac_cmd_t *)cmpkt)->scb =
							DERR_ABORT;
					} else
						cmpkt->cp_reason = CPS_SUCCESS;
				}
			}
			break;
		} else if (!init) {
			/*
			 * Although intr's are disabled, some have
			 * leaked in, we handle them immediately.
			 */
			mutex_exit(&mlx->mutex);
			(void) mlx_intr((caddr_t)mlx);
			mutex_enter(&mlx->mutex);

			if (!ccb->intr_wanted)
				break;
		} else {
			MLX_GET_ISTAT(mlx, NULL, 1);
			MDBG2(("mlx_pollstat: bad stat_id %d, cmdid %d",
				ccb->ccb_stat_id, ccb->ccb_cmdid));
		}
	}
	if (!retry) {
		cmn_err(CE_WARN, "mlx_pollstat: failed to get the status of "
			"cmdid %d, retried %d times", ccb->ccb_cmdid,
			MLX_MAX_RETRY);
		return (DDI_FAILURE);
	}
	return (DDI_SUCCESS);
}

int
mlx_tran_tgt_init(
    dev_info_t *const hba_dip,
    dev_info_t *const tgt_dip,
    scsi_hba_tran_t *const hba_tran,
    register struct scsi_device *const sd)
{
	int tgt;
	register mlx_t *mlx;
	register mlx_hba_t *hba;
	register mlx_unit_t *hba_unit;			/* self */
	register mlx_unit_t *child_unit;
	register struct scsi_inquiry *scsi_inq;

	ASSERT(sd != NULL);
	hba_unit = MLX_SDEV2HBA_UNIT(sd);
	/* nexus should already be initialized */
	ASSERT(hba_unit != NULL);
	hba = hba_unit->hba;
	ASSERT(hba != NULL);
	ASSERT(MLX_SCSI(hba));
	mlx = hba->mlx;
	ASSERT(mlx != NULL);

	tgt = sd->sd_address.a_target;
	if (tgt >= mlx->max_tgt)
		return (DDI_FAILURE);
	if (sd->sd_address.a_lun) {
		cmn_err(CE_WARN, "mlx_tran_tgt_init: non-zero lun on chn %d "
		    "tgt %d cannot be supported", hba->chn, tgt);
		return (DDI_FAILURE);
	}
	scsi_inq = sd->sd_inq;

	mutex_enter(&mlx->mutex);
	if (scsi_inq && (mlx->flags & MLX_NO_HOT_PLUGGING)) {
		/*
		 * mlx_getinq() set inq_dtype for the device
		 * based on its accessiblility.
		 */
		if (scsi_inq->inq_dtype == DTYPE_NOTPRESENT ||
		    scsi_inq->inq_dtype == DTYPE_UNKNOWN) {
			mutex_exit(&mlx->mutex);
			return (DDI_FAILURE);
		}
	}

	if (mlx_dont_access(mlx, hba->chn, tgt)) {
		mutex_exit(&mlx->mutex);
		return (DDI_FAILURE);
	}
	mutex_exit(&mlx->mutex);

	if (scsi_inq && (scsi_inq->inq_rdf == RDF_SCSI2) &&
	    (scsi_inq->inq_cmdque)) {
		ASSERT(hba_dip != NULL);
		ASSERT(tgt_dip != NULL);
		mlx_childprop(hba_dip, tgt_dip);
	}

	child_unit = (mlx_unit_t *)kmem_zalloc(sizeof (mlx_unit_t),
		KM_NOSLEEP);
	if (child_unit == NULL)
		return (DDI_FAILURE);

	bcopy((caddr_t)hba_unit, (caddr_t)child_unit, sizeof (*hba_unit));
	child_unit->dma_lim = mlx_dma_lim;
	/*
	 * After this point always use (mlx_unit_t *)->dma_lim and refrain
	 * from using mlx_dac_dma_lim as the former can be customized per
	 * unit target driver instance but the latter is the generic hba
	 * instance dma limits.
	 */

	ASSERT(hba_tran != NULL);
	hba_tran->tran_tgt_private = child_unit;

	mutex_enter(&hba->mutex);
	hba->refcount++;   		/* increment active child refcount */
	mutex_exit(&hba->mutex);

	MDBG2(("mlx_tran_tgt_init: S%xC%xt%x dip=0x%x sd=0x%x unit=0x%x",
	    MLX_SLOT(hba->mlx->reg) /* Slot */, hba->chn, tgt,
	    tgt_dip, sd, child_unit));

	return (DDI_SUCCESS);
}

struct scsi_pkt *
mlx_init_pkt(
    struct scsi_address *const sa,
    register struct scsi_pkt *pkt,
    register buf_t *const bp,
    int cmdlen,
    int statuslen,
    int tgt_priv_len,
    int flags, int (*const callback)(),
    const caddr_t arg)
{
	struct scsi_pkt	*new_pkt = NULL;

	ASSERT(sa != NULL);
	ASSERT(MLX_SCSI(MLX_SA2HBA(sa)));

	/* Allocate a pkt only if NULL */
	if (pkt == NULL) {
		pkt = mlx_pktalloc(sa, cmdlen, statuslen,
		    tgt_priv_len, callback, arg);
		if (pkt == NULL)
			return (NULL);
		((struct scsi_cmd *)pkt)->cmd_flags = flags;
		new_pkt = pkt;
	} else
		new_pkt = NULL;

	/* Set up dma info only if bp is non-NULL */
	if (bp != NULL &&
	    mlx_dmaget(pkt, (opaque_t)bp, callback, arg) == NULL) {
		if (new_pkt != NULL)
			mlx_pktfree(new_pkt);
		return (NULL);
	}

	return (pkt);
}

void
mlx_destroy_pkt(struct scsi_address *const sa, struct scsi_pkt *const pkt)
{
	ASSERT(sa != NULL);
	ASSERT(MLX_SCSI(MLX_SA2HBA(sa)));

	mlx_dmafree(sa, pkt);
	mlx_pktfree(pkt);
}

struct scsi_pkt *
mlx_pktalloc(
    register struct scsi_address *const sa,
    int cmdlen,
    int statuslen,
    int tgt_priv_len,
    int (*const callback)(),
    const caddr_t arg)
{
	register struct scsi_cmd *cmd;
	mlx_ccb_t *ccb;
	register mlx_cdbt_t *cdbt;
	register mlx_hba_t *hba;
	register mlx_unit_t *unit;
	caddr_t	tgt_priv;
	int kf = HBA_KMFLAG(callback);

	ASSERT(sa != NULL);
	unit = MLX_SA2UNIT(sa);
	ASSERT(unit != NULL);
	hba = unit->hba;
	ASSERT(hba != NULL);
	ASSERT(MLX_SCSI(hba));

	/* Allocate target private data, if necessary */
	if (tgt_priv_len > PKT_PRIV_LEN) {
		tgt_priv = kmem_zalloc(tgt_priv_len, kf);
		if (tgt_priv == NULL) {
			ASSERT(callback != SLEEP_FUNC);
			if (callback != NULL_FUNC)
				ddi_set_callback(callback, arg,
				    &hba->callback_id);
			return (NULL);
		}
	} else		/* not necessary to allocate target private data */
		tgt_priv = NULL;

	/* Allocate common packet */
	cmd = kmem_zalloc(sizeof (*cmd), kf);
	mutex_enter(&hba->mutex);
	if (cmd != NULL) {
		size_t mem = sizeof (*ccb) + sizeof (*cdbt);

		ASSERT(hba->dip != NULL);
		/*
		 * Allocate a ccb.
		 *
		 * We should not use &unit->dma_lim here as we are
		 * getting a ccb from the hba's pool and the ccb's
		 * in this pool should not be bound/specific to a unit.
		 */
		if (scsi_iopb_fast_zalloc(&hba->ccb_pool, hba->dip,
		    &mlx_dma_lim, mem, (caddr_t *)&ccb)) {
			kmem_free(cmd, sizeof (*cmd));
			cmd = NULL;
		}
	}
	mutex_exit(&hba->mutex);

	if (cmd == NULL) {
		if (tgt_priv != NULL)
			kmem_free(tgt_priv, tgt_priv_len);
		if (callback != DDI_DMA_DONTWAIT)
			ddi_set_callback(callback, arg, &hba->callback_id);
		return (NULL);
	}

	ccb->paddr = MLX_KVTOP(ccb);
	ccb->type = MLX_SCSI_CTYPE;
	ccb->ccb_opcode = MLX_SCSI_DCDB;

	ccb->ccb_cdbt = cdbt = (mlx_cdbt_t *)(ccb + 1);
	ccb->ccb_xferpaddr = ccb->paddr + sizeof (*ccb);

	cdbt->unit = (hba->chn << 4) | (u_char)sa->a_target;

	/* Set up target private data */
	cmd->cmd_privlen = (u_char)tgt_priv_len;
	if (tgt_priv_len > PKT_PRIV_LEN)
		cmd->cmd_pkt.pkt_private = (opaque_t)tgt_priv;
	else if (tgt_priv_len > 0)
		cmd->cmd_pkt.pkt_private = cmd->cmd_pkt_private;

	/* XXX - How about early status? */
	MLX_SCSI_CDB_NORMAL_STAT(cdbt->cmd_ctrl);
	/*
	 * If we don't set the timeout and leave it as it is
	 * it will be 1 hr., so we take the min possible to
	 * be on the safe side.
	 */
	MLX_SCSI_CDB_1HR_TIMEOUT(cdbt->cmd_ctrl);
	if (unit->scsi_auto_req) {
		MLX_SCSI_CDB_AUTO_REQ_SENSE(cdbt->cmd_ctrl);
		cmd->cmd_pkt.pkt_scbp = (u_char *)&ccb->ccb_arq_stat;
	} else {
		MLX_SCSI_CDB_NO_AUTO_REQ_SENSE(cdbt->cmd_ctrl);
		cmd->cmd_pkt.pkt_scbp = (u_char *)&cdbt->status;
	}
	/*
	 * Disconnects will be enabled, if appropriate,
	 * after the pkt_flags bit is set
	 */
	cdbt->cdblen = (u_char)cmdlen;
	cdbt->senselen = MLX_SCSI_MAX_SENSE;

	cmd->cmd_pkt.pkt_cdbp = (opaque_t)&cdbt->cdb;

	ccb->ccb_ownerp = cmd;

	cmd->cmd_private = (opaque_t)ccb;
	cmd->cmd_cdblen = (u_char)cmdlen;
	cmd->cmd_scblen = (u_char)statuslen;
	cmd->cmd_pkt.pkt_address = *sa;
	return ((struct scsi_pkt *)cmd);
}

void
mlx_pktfree(register struct scsi_pkt *const pkt)
{
	register mlx_ccb_t *ccb;
	register mlx_hba_t *hba;
	register struct scsi_cmd *cmd;

	ASSERT(pkt != NULL);
	cmd = (struct scsi_cmd *)pkt;

	hba = MLX_SCSI_PKT2HBA(pkt);
	ASSERT(hba != NULL);
	ASSERT(MLX_SCSI(hba));

	if (cmd->cmd_privlen > PKT_PRIV_LEN) {
		ASSERT(pkt->pkt_private != NULL);
		kmem_free(pkt->pkt_private, cmd->cmd_privlen);
	}

	mutex_enter(&hba->mutex);
	ccb = (mlx_ccb_t *)cmd->cmd_private;
	if (ccb != NULL)
		scsi_iopb_fast_free(&hba->ccb_pool, (caddr_t)ccb);

	mutex_exit(&hba->mutex);
	kmem_free(cmd, sizeof (*cmd));

	if (hba->callback_id)
		ddi_run_callback(&hba->callback_id);
}

struct scsi_pkt *
mlx_dmaget(
    register struct scsi_pkt *const pkt,
    const opaque_t dmatoken,
    int (*const callback)(),
    const caddr_t arg)
{
	ushort max_xfer;	/* max that can be transferred w/ or w/o SG */
	int fw_supports_sg;
	off_t offset;
	off_t len;
	register int cnt;
	register u_int pkt_totxfereq;		/* total xfer request */
	register buf_t *bp = (buf_t *)dmatoken;
	register struct scsi_cmd *cmd = (struct scsi_cmd *)pkt;
	register mlx_t *mlx;
	register mlx_hba_t *hba;
	register mlx_ccb_t *ccb;
	register mlx_cdbt_t *cdbt;
	ddi_dma_cookie_t dmac;
	register ddi_dma_cookie_t *dmacp = &dmac;

	ASSERT(pkt != NULL);		/* also cmd != NULL */
	hba = MLX_SCSI_PKT2HBA(pkt);
	ASSERT(hba != NULL);
	ASSERT(MLX_SCSI(hba));
	mlx = hba->mlx;
	ASSERT(mlx != NULL);
	ASSERT(bp != NULL);
	pkt_totxfereq = bp->b_bcount;
	ccb = (mlx_ccb_t *)cmd->cmd_private;
	ASSERT(ccb != NULL);
	cdbt = ccb->ccb_cdbt;
	ASSERT(cdbt != NULL);

	if (!pkt_totxfereq) {
		pkt->pkt_resid = 0;
		MLX_SCSI_CDB_NODATA(cdbt->cmd_ctrl);
		return (pkt);
	}

	/* Check direction for data transfer */
	if (bp->b_flags & B_READ) {
		MLX_SCSI_CDB_DATAIN(cdbt->cmd_ctrl);
		cmd->cmd_cflags &= ~CFLAG_DMASEND;
	} else {
		MLX_SCSI_CDB_DATAOUT(cdbt->cmd_ctrl);
		cmd->cmd_cflags |= CFLAG_DMASEND;
	}

	/* Setup dma memory and position to the next xfer segment */
	if (scsi_impl_dmaget(pkt, (opaque_t)bp, callback, arg,
	    &(MLX_SCSI_PKT2UNIT(pkt)->dma_lim)) == NULL)
		return (NULL);
	if (ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len, dmacp) ==
	    DDI_FAILURE)
		return (NULL);

	/* Establish how many bytes can be transferred w/o SG */
	max_xfer = (ushort)MLX_MIN(MLX_MIN(pkt_totxfereq, MLX_SCSI_MAX_XFER),
	    dmacp->dmac_size);
	if (MLX_SCSI_AUTO_REQ_OFF(cdbt) &&
	    (*(pkt->pkt_cdbp) == SCMD_REQUEST_SENSE))
		max_xfer = MLX_MIN(MLX_SCSI_MAX_SENSE, max_xfer);

	mutex_enter(&mlx->mutex);
	fw_supports_sg = mlx->flags & MLX_SUPPORTS_SG;
	mutex_exit(&mlx->mutex);

	/* Check for one single block transfer */
	if (pkt_totxfereq <= max_xfer || !fw_supports_sg) {
		/* need not or cannot do SG transfer */
		cdbt->xfersz = (ushort)MLX_MIN(pkt_totxfereq, max_xfer);
		cdbt->databuf = (paddr_t)dmacp->dmac_address;
		cmd->cmd_totxfer = cdbt->xfersz;
	} else if (fw_supports_sg) {	/* attempt multi-block SG transfer */
		register int bxfer;
		register mlx_sg_element_t *sge;

		/* Request Sense shouldn't need scatter-gather io */
		ASSERT(*(pkt->pkt_cdbp) != SCMD_REQUEST_SENSE);

		/* ccb->type is set to MLX_SCSI_CTYPE, in _pktalloc() */
		ccb->ccb_opcode = MLX_SCSI_SG_DCDB;

		/* max_xfer is no longer limited to the 1st dmacp->dmac_size */
		max_xfer = (ushort)MLX_MIN(pkt_totxfereq, MLX_SCSI_MAX_XFER);

		/* Set address of scatter-gather segs */
		sge = ccb->ccb_sg_list;
		for (bxfer = 0, cnt = 1; ; cnt++, sge++) {
			bxfer += dmacp->dmac_size;

			sge->data01_ptr32 = (ulong)dmacp->dmac_address;
			sge->data02_len32 = (ulong)dmacp->dmac_size;

			/* Check for end of list condition */
			if (pkt_totxfereq == (bxfer + cmd->cmd_totxfer))
				break;
			ASSERT(pkt_totxfereq > (bxfer + cmd->cmd_totxfer));

			/*
			 * Check for end of list condition and check
			 * end of physical scatter-gather list limit,
			 * then attempt to get the next dma segment
			 * and cookie.
			 */
			if (bxfer >= max_xfer || cnt >= MLX_MAX_NSG ||
			    ddi_dma_nextseg(cmd->cmd_dmawin,
					    cmd->cmd_dmaseg,
					    &cmd->cmd_dmaseg) != DDI_SUCCESS ||
			    ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset,
						&len, dmacp) == DDI_FAILURE)
				break;
		}
		ASSERT(cnt <= MLX_MAX_NSG);
		ccb->ccb_sg_type = (u_char)cnt | MLX_SGTYPE0;

		/* In case bxfer exceeded max_xfer in the last iteration */
		bxfer = MLX_MIN(bxfer, max_xfer);
		cdbt->xfersz = (ushort)bxfer;
		cdbt->databuf = ccb->paddr + MLX_OFFSET(ccb, ccb->ccb_sg_list);
		cmd->cmd_totxfer += bxfer;
	}

	/* f/w updates cdbt->xfersz, so we have to preserve it */
	ccb->bytexfer = cdbt->xfersz;

	/*
	 * We have to calculate the "tentative" value of pkt_resid which
	 * is the left over of data *if* transport is successful.  This
	 * value needs to be communicated back to the target layer to
	 * update the contents of the SCSI cdb which the hba layer is
	 * not allowed to change.
	 *
	 * Based on this tentative value of pkt_resid the target layer
	 * updates the SCSI cdb and then attempts the transport of the
	 * (same) packet.
	 */
	pkt->pkt_resid = pkt_totxfereq - cmd->cmd_totxfer;	/* tentative */
	ASSERT(pkt->pkt_resid >= 0);

	return (pkt);
}

/* Dma resource deallocation */
/*ARGSUSED*/
void
mlx_dmafree(
    struct scsi_address *const sa,
    register struct scsi_pkt *const pkt)
{
	register struct	scsi_cmd *cmd = (struct scsi_cmd *)pkt;

	ASSERT(sa != NULL);
	ASSERT(MLX_SCSI(MLX_SA2HBA(sa)));
	ASSERT(cmd != NULL);

	if (cmd->cmd_dmahandle) {		/* Free the mapping. */
		ddi_dma_free(cmd->cmd_dmahandle);
		cmd->cmd_dmahandle = NULL;
	}
}

/*ARGSUSED*/
void
mlx_sync_pkt(
    struct scsi_address *const sa,
    register struct scsi_pkt *const pkt)
{
	register int rval;
	register struct	scsi_cmd *cmd = (struct scsi_cmd *)pkt;

	ASSERT(sa != NULL);
	ASSERT(MLX_SCSI(MLX_SA2HBA(sa)));
	ASSERT(cmd != NULL);

	if (cmd->cmd_dmahandle) {
		rval = ddi_dma_sync(cmd->cmd_dmahandle, 0, 0,
		    (cmd->cmd_cflags & CFLAG_DMASEND) ?
			DDI_DMA_SYNC_FORDEV : DDI_DMA_SYNC_FORCPU);
		if (rval != DDI_SUCCESS)
			cmn_err(CE_WARN, "mlx_sync_pkt: dma sync failed");
	}
}

/*ARGSUSED*/
int
mlx_transport(
    struct scsi_address *const sa,
    register struct scsi_pkt *const pkt)
{
	register mlx_hba_t *hba;
	register mlx_ccb_t *ccb;
	register mlx_t *mlx;

	ASSERT(pkt != NULL);
	ccb = (mlx_ccb_t *)SCMD_PKTP(pkt)->cmd_private;
	ASSERT(ccb != NULL);

	hba = MLX_SCSI_PKT2HBA(pkt);
	ASSERT(hba != NULL);
	ASSERT(MLX_SCSI(hba));
	mlx = hba->mlx;
	ASSERT(mlx != NULL);

	ASSERT(ccb->ccb_cdbt != NULL);

	/* initialize some pkt vars that might need it on re-transport */
	if (pkt->pkt_flags & FLAG_NODISCON)
		MLX_SCSI_CDB_NO_DISCON(ccb->ccb_cdbt->cmd_ctrl);
	else
		MLX_SCSI_CDB_DISCON(ccb->ccb_cdbt->cmd_ctrl);

	pkt->pkt_statistics = 0;
	pkt->pkt_resid = 0;
	pkt->pkt_state = 0;

	if (ccb->type != MLX_SCSI_CTYPE) {
		cmn_err(CE_WARN, "mlx_transport: bad SCSI ccb cmd, "
		    "type %d, pkt %x", ccb->type, (int)pkt);
		return (TRAN_FATAL_ERROR);
	}
	switch (ccb->type) {
	case MLX_DAC_CTYPE0:
	case MLX_DAC_CTYPE1:
	case MLX_DAC_CTYPE2:
	case MLX_DAC_CTYPE4:
	case MLX_DAC_CTYPE5:
	case MLX_DAC_CTYPE6:
		break;
	case MLX_SCSI_CTYPE:
		if (!sema_tryp(&mlx->scsi_ncdb_sema)) {
			MDBG2(("mlx_transport: refused to xport "
				"Direct CDB cmd, hit the max"));
			return (TRAN_BUSY);
		}
		break;
	default:
		cmn_err(CE_WARN, "mlx_transport: bad dac cmd type %d, "
		    "pkt %x", ccb->type, (int)pkt);
		return (TRAN_BADPKT);
	}

	/* XXX - ddi_dma_sync() for device, not required for EISA cards. */

	mutex_enter(&mlx->mutex);
	if (mlx_sendcmd(mlx, ccb, 0) == DDI_FAILURE) {
		mutex_exit(&mlx->mutex);
		return (TRAN_BUSY);
	}
	if (pkt->pkt_flags & FLAG_NOINTR) {
		MLX_DISABLE_INTR(mlx);
		if (mlx_pollstat(mlx, ccb, 0) == DDI_FAILURE) {
			pkt->pkt_reason = CMD_TRAN_ERR;
			pkt->pkt_state = 0;
		}
		MLX_ENABLE_INTR(mlx);
	}
	mutex_exit(&mlx->mutex);
	return (TRAN_ACCEPT);
}

void
mlx_chkerr(
    mlx_t *const mlx,
    register mlx_ccb_t *const ccb,
    register struct scsi_pkt *const pkt,
    const ushort status)
{
	register struct scsi_cmd *cmd;
	register struct scsi_arq_status *arq_stat;
	register mlx_cdbt_t *cdbt;

	ASSERT(ccb != NULL);
	cdbt = ccb->ccb_cdbt;

	ASSERT(cdbt != NULL);
	ASSERT(pkt != NULL);
	cmd = SCMD_PKTP(pkt);
	ASSERT(cmd != NULL);
	ASSERT(MLX_SCSI(MLX_SCSI_PKT2HBA(pkt)));

	pkt->pkt_scbp  = (u_char *)&cdbt->status;
	pkt->pkt_resid = ccb->bytexfer - cdbt->xfersz;		/* final */

	switch (status) {
	case 0:
		pkt->pkt_resid  = ccb->bytexfer - cdbt->xfersz;
		pkt->pkt_reason = CMD_CMPLT;
		pkt->pkt_state |= STATE_GOT_BUS  | STATE_GOT_TARGET |
		    STATE_SENT_CMD | STATE_GOT_STATUS;
		if (cdbt->xfersz)
			pkt->pkt_state |= STATE_XFERRED_DATA;
		return;
	case 2:			/* check-condition */
		pkt->pkt_reason = CMD_CMPLT;
		pkt->pkt_state |= STATE_GOT_BUS  | STATE_GOT_TARGET |
		    STATE_SENT_CMD | STATE_GOT_STATUS;
		if (cdbt->xfersz)
			pkt->pkt_state |= STATE_XFERRED_DATA;

		if (!(MLX_SCSI_PKT2UNIT(pkt))->scsi_auto_req ||
		    MLX_SCSI_AUTO_REQ_OFF(cdbt))
			return;

		/*
		 * ARQ was not issued, indicate check condition
		 */
		if (!cdbt->senselen) {
			*pkt->pkt_scbp = (u_char)status;
			MDBG2(("mlx_chkerr: no sense data available"));
			return;
		}

		pkt->pkt_scbp = (u_char *)&ccb->ccb_arq_stat;
		arq_stat = &ccb->ccb_arq_stat;

		arq_stat->sts_status = *(struct scsi_status *)&status;

		arq_stat->sts_rqpkt_status = cdbt->status;

		arq_stat->sts_rqpkt_reason = CMD_CMPLT;

		arq_stat->sts_rqpkt_resid  = 0;

		arq_stat->sts_rqpkt_state |= STATE_XFERRED_DATA;

		bcopy((caddr_t)&cdbt->sensedata,
		    (caddr_t)&arq_stat->sts_sensedata,
			MLX_MIN(sizeof (struct scsi_extended_sense),
			    cdbt->senselen));

		MDBG2(("mlx_chkerr: check-condition, arq_stat=%x",
		    arq_stat));

		pkt->pkt_state |= STATE_ARQ_DONE;
		return;
	case 8:						/* device busy */
		pkt->pkt_reason = CMD_CMPLT;
		pkt->pkt_state |= STATE_GOT_BUS | STATE_GOT_TARGET |
		    STATE_SENT_CMD | STATE_GOT_STATUS;
		return;
	case 0xe:
	case 0xf:
		/*
		 * Selection timeout or abnormal cmd termination due to
		 * to a device or bus reset.  Unfortunately, we have no
		 * way to find out which one of the above conditions
		 * caused the error.
		 */
		pkt->pkt_reason = CMD_RESET;
		pkt->pkt_statistics |= STAT_TIMEOUT | STAT_BUS_RESET |
		    STAT_DEV_RESET | STAT_ABORTED;
		return;
	case MLX_BAD_OPCODE:
		MDBG2(("mlx_chkerr: bad opcode"));
		pkt->pkt_reason = CMD_TRAN_ERR;
		return;
	case MLX_E_LIMIT:
		/*
		 * Invalid chn# &/or tgt id or bad xfer length.
		 *
		 * NB. xfer length of zero and io beyond the
		 * device limits are the most common causes
		 * of this error.
		 */
		if ((mlx->flags & MLX_NO_HOT_PLUGGING) &&
		    !(pkt->pkt_flags & FLAG_SILENT)) {
			MDBG2(("mlx_chkerr: invalid device "
			    "address or bad xfer length(0x%x)\n\t or io "
			    "beyond device limits: chn %d, tgt %d, cdbt=%x,"
			    "pkt=%x",
			    cdbt->xfersz, cdbt->unit >> 4, cdbt->unit & 0xf,
			    (int)cdbt, (int)pkt));
		}

		pkt->pkt_reason = CMD_TRAN_ERR;
		return;
	default:
		cmn_err(CE_PANIC, "mlx_chkerr: invalid status 0x%x",
		    status);
	}
}

/* Abort specific command on target device */
/*ARGSUSED*/
int
mlx_abort(struct scsi_address *const sa, struct scsi_pkt *const pkt)
{
	ASSERT(MLX_SCSI(MLX_SA2HBA(sa)));
	/* Mylex DAC960 does not support recall of command in process */
	return (0);
}

/*
 * Reset the bus, or just one target device on the bus.
 * Returns 1 on success and 0 on failure.
 *
 * Currently no packet queueing is done prior to transport.  Therefore,
 * after a successful reset operation, we can only wait until the aborted
 * commands are returned with the appropriate error status from the
 * adapter and then set the pkt_reason and pkt_statistics accordingly.
 */
/*ARGSUSED*/
int
mlx_reset(register struct scsi_address *const sa, int level)
{
#if 1	/* XXX */
	/*
	 * Because of performance reasons the adapter cannot tolerate
	 * long timeouts on the drives.  The SCSI disk drives which do
	 * not have a quick enough response to Device Ready after a
	 * reset are set to DEAD by the f/w which causes a serious
	 * inconvenience to the user.  Unfortunately, the majority of
	 * the SCSI disk drives in the market fall into this catagory
	 * and except a few which are approved by Mylex do not behave
	 * properly at reset.  Hence, it is more appropriate to return
	 * failure for this entry point than cause grieviance to users.
	 *
	 * Taq queueing on the disks not certified by Mylex cause resets
	 * at the f/w level which display the above symptom.  Hence, it is
	 * strongly recommended to turn taq queueing off unless the dirve
	 * is explicitly listed in the tested and approved drives by Mylex.
	 */
	return (0);
#else /* XXX */
	u_char chn;
	register int errno;
	register mlx_unit_t *unit;
	register mlx_hba_t *hba;
	mlx_dacioc_t dacioc;

	ASSERT(sa != NULL);
	unit = MLX_SA2UNIT(sa);
	ASSERT(unit != NULL);
	hba = unit->hba;
	ASSERT(hba != NULL);

	if (MLX_DAC(hba))		/* called via mlx_dac_reset() */
		chn = (u_char)(sa->a_target >> 4);
	else
		chn = hba->chn;

	ASSERT(chn < hba->mlx->nchn);
	dacioc.dacioc_chn = chn;
	dacioc.ubuf_len = 0;

	switch (level) {
	case RESET_ALL:
		dacioc.dacioc_dev_state = MLX_DAC_RESETC_HARD;
		errno = mlx_dacioc(unit, MLX_DACIOC_RESETC, (int)&dacioc,
		    FKIOCTL);
		if (errno != DDI_SUCCESS) {
			cmn_err(CE_WARN, "mlx_reset: failed to issue reset on "
				"chn %x", chn);
			MDBG2(("mlx_reset: errno=%x", errno));
			return (0);
		} else if (dacioc.status != MLX_SUCCESS) {
			cmn_err(CE_WARN, "mlx_reset: failed to reset chn %x",
				chn);
			MDBG2(("mlx_reset: status=%x", dacioc.status));
			return (0);
		} else {
			MDBG2(("mlx_reset: reset chn %x", chn));
			return (1);
		}
	case RESET_TARGET: {
		u_char tgt;

		tgt = (u_char)sa->a_target;
		if (MLX_DAC(hba))
			tgt &= 0x0F;	/* throw away the channel number */
		ASSERT(tgt < hba->mlx->max_tgt);
		dacioc.dacioc_tgt = tgt;
		dacioc.dacioc_dev_state = MLX_DAC_TGT_ONLINE;
		errno = mlx_dacioc(unit, MLX_DACIOC_START, (int)&dacioc,
		    FKIOCTL);
		if (errno != DDI_SUCCESS) {
			cmn_err(CE_WARN, "mlx_reset: failed to issue a reset "
				"to chn %x tgt %x", chn, tgt);
			MDBG2(("mlx_reset: errno=%x", errno));
			return (0);
		} else if (dacioc.status != MLX_SUCCESS) {
			cmn_err(CE_WARN, "mlx_reset: failed to reset "
				"chn %x tgt %x", chn, tgt);
			MDBG2(("mlx_reset: status=%x", dacioc.status));
			return (0);
		} else {
			MDBG2(("mlx_reset: reset chn %x tgt %x", chn, tgt));
			return (1);
		}
	}
	default:
		MDBG2(("mlx_reset: bad level %x", level));
		return (0);
	}
#endif /* XXX */
}

int
mlx_capchk(
    register char *cap,
    int tgtonly,
    int *cap_idxp)
{
	if ((tgtonly && tgtonly != 1) || cap == NULL)
		return (0);

	*cap_idxp = scsi_hba_lookup_capstr(cap);
	return (1);
}

int
mlx_getcap(
    register struct scsi_address *const sa,
    char *cap,
    int tgtonly)
{
	int 			cap_idx;
	int			heads = 64, sectors = 32;
	register mlx_unit_t 	*unit;

	ASSERT(sa != NULL);
	unit = MLX_SA2UNIT(sa);
	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_SCSI(unit->hba));

	if (!mlx_capchk(cap, tgtonly, &cap_idx))
		return (UNDEFINED);

	switch (cap_idx) {
		case SCSI_CAP_GEOMETRY:
			if (unit->hba->mlx)
				return (MLX_GEOMETRY(unit->hba->mlx, sa,
					unit->capacity));

			return (HBA_SETGEOM(heads, sectors));
		case SCSI_CAP_ARQ:
			return (unit->scsi_auto_req);
		case SCSI_CAP_TAGGED_QING:
			return (unit->scsi_tagq);
		default:
			return (UNDEFINED);
	}
}

int
mlx_setcap(
    register struct scsi_address *const sa,
    char *cap,
    int value,
    int tgtonly)
{
	int cap_idx;
	register mlx_unit_t *unit;

	ASSERT(sa != NULL);
	unit = MLX_SA2UNIT(sa);
	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_SCSI(unit->hba));

	if (!mlx_capchk(cap, tgtonly, &cap_idx))
		return (UNDEFINED);

	switch (cap_idx) {
	case SCSI_CAP_TAGGED_QING:
		if (tgtonly) {
			unit->scsi_tagq = (u_int)value;
			return (1);
		}
		break;
	case SCSI_CAP_ARQ:
		if (tgtonly) {
			unit->scsi_auto_req = (u_int)value;
			return (1);
		}
		break;
	case SCSI_CAP_SECTOR_SIZE:
		unit->dma_lim.dlim_granular = (u_int)value;
		return (1);
	case SCSI_CAP_TOTAL_SECTORS:
		unit->capacity = (u_int)value;
		return (1);
	case SCSI_CAP_GEOMETRY:
		/*FALLTHROUGH*/
	default:
		break;
	}
	return (0);
}
