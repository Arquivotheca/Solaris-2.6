/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
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
 * if Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#pragma	ident	"@(#)mlx_dac.c	1.3	96/07/28 SMI"

#include <sys/dktp/mlx/mlx.h>

/* Non-SCSI Ctlobj operations */
struct ctl_objops mlx_dac_objops = {
	mlx_dac_pktalloc,
	mlx_dac_pktfree,
	mlx_dac_memsetup,
	mlx_dac_memfree,
	mlx_dac_iosetup,
	mlx_dac_transport,
	mlx_dac_reset,
	mlx_dac_abort,
	nulldev,
	nulldev,
#if 0	/* XXX - Pending PSARC approval */
	mlx_dacioc,
#else /* XXX */
	mlx_dacioc_nopkt,
#endif /* XXX */
	0, 0
};

int
mlx_dac_tran_tgt_init(
    dev_info_t *const hba_dip,
    dev_info_t *const tgt_dip,
    scsi_hba_tran_t *const hba_tran,
    register struct scsi_device *const sd)
{
	int tgt;
	register mlx_hba_t *hba;
	register mlx_t *mlx;
	register mlx_unit_t *hba_unit;			/* self */
	register mlx_unit_t *child_unit;
	register struct scsi_inquiry *scsi_inq;
	register struct ctl_obj *ctlobjp;

	ASSERT(sd != NULL);
	tgt = sd->sd_address.a_target;

	if (sd->sd_address.a_lun)		/* no support for luns */
		return (DDI_NOT_WELL_FORMED);

	hba_unit = MLX_SDEV2HBA_UNIT(sd);
	/* nexus should already be initialized */
	ASSERT(hba_unit != NULL);
	hba = hba_unit->hba;
	ASSERT(hba != NULL);
	ASSERT(MLX_DAC(hba));
	mlx = hba->mlx;
	ASSERT(mlx != NULL);

	mutex_enter(&mlx->mutex);
	ASSERT(mlx->conf != NULL);
	if (tgt >= mlx->conf->nsd) {
		mutex_exit(&mlx->mutex);
		return (DDI_FAILURE);
	}
	mutex_exit(&mlx->mutex);

	scsi_inq = (struct scsi_inquiry *)kmem_zalloc(sizeof (*scsi_inq),
	    KM_NOSLEEP);
	if (scsi_inq == NULL)
		return (DDI_FAILURE);

	child_unit = (mlx_unit_t *)kmem_zalloc(
	    sizeof (mlx_unit_t) + sizeof (*ctlobjp), KM_NOSLEEP);
	if (child_unit == NULL) {
		kmem_free(scsi_inq, sizeof (*scsi_inq));
		return (DDI_FAILURE);
	}

	mlx_dac_fake_inquiry(hba, tgt, scsi_inq);

	bcopy((caddr_t)hba_unit, (caddr_t)child_unit, sizeof (*hba_unit));
	child_unit->dma_lim = mlx_dac_dma_lim;
	child_unit->dma_lim.dlim_sgllen = mlx->sgllen;
	/*
	 * After this point always use (mlx_unit_t *)->dma_lim and refrain
	 * from using mlx_dac_dma_lim as the former can be customized per
	 * unit target driver instance but the latter is the generic hba
	 * instance dma limits.
	 */

	ASSERT(hba_tran != NULL);
	hba_tran->tran_tgt_private = child_unit;

	child_unit->dac_unit.sd_num = (u_char)tgt;

	sd->sd_inq = scsi_inq;

	ctlobjp   = (struct ctl_obj *)(child_unit + 1);
	sd->sd_address.a_hba_tran = (scsi_hba_tran_t *)ctlobjp;

	ctlobjp->c_ops  = (struct ctl_objops *)&mlx_dac_objops;
	ctlobjp->c_data = (opaque_t)child_unit;
	ctlobjp->c_ext  = &(ctlobjp->c_extblk);
	ASSERT(hba_dip != NULL);
	ctlobjp->c_extblk.c_ctldip = hba_dip;
	ASSERT(tgt_dip != NULL);
	ctlobjp->c_extblk.c_devdip = tgt_dip;
	ctlobjp->c_extblk.c_targ   = tgt;
	ctlobjp->c_extblk.c_blksz  = NBPSCTR;


	mutex_enter(&hba->mutex);
	hba->refcount++;		/* increment active child refcount */
	mutex_exit(&hba->mutex);

	MDBG3(("mlx_dac_tran_tgt_init: S%xC%xd%x dip=0x%x sd=0x%x unit=0x%x",
		hba->mlx->reg/0x1000 /* Slot */, hba->chn, tgt,
		tgt_dip, sd, child_unit));

	return (DDI_SUCCESS);
}

void
mlx_dac_fake_inquiry(
    register mlx_hba_t *const hba,
    int tgt,
    struct scsi_inquiry *const scsi_inq)
{
	u_char status;
	char name[32];
	register mlx_dac_sd_t *sd;
	register mlx_t *mlx;

	ASSERT(hba != NULL);
	ASSERT(MLX_DAC(hba));
	mlx = hba->mlx;
	ASSERT(mlx != NULL);
	ASSERT(scsi_inq != NULL);

	mutex_enter(&mlx->mutex);
	ASSERT(mlx->conf != NULL);
	sd = mlx->conf->sd + tgt;
	ASSERT(sd != NULL);

	if (tgt >= mlx->conf->nsd) {
		scsi_inq->inq_dtype = DTYPE_NOTPRESENT;
		mutex_exit(&mlx->mutex);
		return;
	}

	status = sd->status;
	if (status == MLX_DAC_ONLINE || status == MLX_DAC_CRITICAL)
		scsi_inq->inq_dtype = DTYPE_DIRECT;
	else {
		ASSERT(status == MLX_DAC_OFFLINE);
		scsi_inq->inq_dtype = DTYPE_NOTPRESENT;
		mutex_exit(&mlx->mutex);
		return;
	}

	scsi_inq->inq_qual = DPQ_POSSIBLE;
	strncpy(scsi_inq->inq_vid, "Mylex ", 8);
	bzero((caddr_t)name, sizeof (name));
	sprintf(name, "Raid-%d", sd->raid);
	mutex_exit(&mlx->mutex);
	strncpy(scsi_inq->inq_pid, name, 16);
	strncpy(scsi_inq->inq_revision, "1234", 4);
}

struct cmpkt *
mlx_dac_iosetup(
    register mlx_unit_t *const unit,
    register struct cmpkt *const cmpkt)
{
	int 			stat, num_segs = 0;
	off_t 			off, len;
#ifdef	DADKIO_RWCMD_READ
	off_t			coff, clen, mlxcmd, lcmd;
#endif
	ulong 			bytes_xfer = 0;
	register mlx_dac_cmd_t	*cmd = (mlx_dac_cmd_t *)cmpkt;
	ddi_dma_cookie_t 	dmac;
	register mlx_ccb_t	*ccb = cmd->ccb;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	ASSERT(cmpkt != NULL);		/* and so is cmd != NULL */

#ifdef	DADKIO_RWCMD_READ
	if (cmpkt->cp_passthru) {
		switch (RWCMDP->cmd) {
		case DADKIO_RWCMD_READ:
		case DADKIO_RWCMD_WRITE:
			mlxcmd =
			(RWCMDP->cmd == DADKIO_RWCMD_READ) ? MLX_DAC_SREAD :
					MLX_DAC_SWRITE;
			lcmd =
			(RWCMDP->cmd == DADKIO_RWCMD_READ) ? DCMD_READ :
					DCMD_WRITE;
			/*
			 * Respect physio() boundaries on i/o
			 *
			 */
			coff = RWCMDP->blkaddr + cmpkt->cp_bp->b_lblkno +
				cmpkt->cp_srtsec;
			clen = cmpkt->cp_bp->b_bcount;
			break;
		default:
			cmn_err(CE_WARN, "mlx_dac_iosetup: unrecognised"
			    "command %x\n", RWCMDP->cmd);
			return (cmpkt);
		}
	} else {
		switch (lcmd = cmd->cdb) {
		case DCMD_READ:
			mlxcmd = MLX_DAC_SREAD;
			goto offsets;
		case DCMD_WRITE:
			mlxcmd = MLX_DAC_SWRITE;
		offsets:
			coff = cmpkt->cp_srtsec;
			clen = cmpkt->cp_bytexfer;
		}
	}

	switch (lcmd) {
#else
	switch (cmd->cdb) {
#endif
	case DCMD_READ:
	case DCMD_WRITE:
		do {
			stat = ddi_dma_nextseg(cmd->dmawin, cmd->dmaseg,
					&cmd->dmaseg);
			if (stat == DDI_DMA_DONE) {
			    ddi_dma_nextwin(cmd->handle, cmd->dmawin,
			    &cmd->dmawin);
				cmd->dmaseg = NULL;
				break;
			}
			if (stat != DDI_SUCCESS)
				return (NULL);
			ddi_dma_segtocookie(cmd->dmaseg, &off, &len, &dmac);
			ccb->ccb_sg_list[num_segs].data01_ptr32 =
			    dmac.dmac_address;
			ccb->ccb_sg_list[num_segs].data02_len32 =
			    dmac.dmac_size;
			bytes_xfer += dmac.dmac_size;
			num_segs++;
		} while ((num_segs < MLX_MAX_NSG) &&
		    (bytes_xfer < MLX_DAC_MAX_XFER) &&
#ifdef	DADKIO_RWCMD_READ
		    (bytes_xfer < clen));
#else
		    (bytes_xfer < cmpkt->cp_bytexfer));
#endif

		/*
		 * In case bytes_xfer exceeds MLX_DAC_MAX_XFER in
		 * the last iteration.
		 */
		bytes_xfer = MLX_MIN(bytes_xfer, MLX_DAC_MAX_XFER);

		cmpkt->cp_resid = cmpkt->cp_bytexfer = bytes_xfer;
		ccb->ccb_opcode =
#ifdef	DADKIO_RWCMD_READ
			(u_char)mlxcmd;
#else
			(cmd->cdb == DCMD_READ) ?
			    MLX_DAC_SREAD : MLX_DAC_SWRITE;
#endif
		ccb->type = 1;
		ccb->ccb_sg_type = (u_char)(num_segs | MLX_SGTYPE0);
		ccb->ccb_xferpaddr = ccb->paddr +
			MLX_OFFSET(ccb, ccb->ccb_sg_list);
		ccb->ccb_cnt = bytes_xfer / MLX_BLK_SIZE;
		if (bytes_xfer % MLX_BLK_SIZE)
			ccb->ccb_cnt++;
#ifdef	DADKIO_RWCMD_READ
		CCB_BLK(ccb, coff);
#else
		CCB_BLK(ccb, cmpkt->cp_srtsec);
#endif
		ccb->ccb_drv = unit->dac_unit.sd_num;

		break;
	case DCMD_SEEK:
	case DCMD_RECAL:
	    break;
	default:
		cmn_err(CE_WARN, "mlx_dac_iosetup: unrecognised command %x\n",
			cmd->cdb);
		break;
	}
	return (cmpkt);
}

struct cmpkt *
mlx_dac_memsetup(
    register mlx_unit_t *const unit,
    register struct cmpkt *const cmpkt,
    register buf_t *const bp,
    int (*const callback)(),
    const caddr_t arg)
{
	register int stat;
	mlx_dac_cmd_t *cmd = (mlx_dac_cmd_t *)cmpkt;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	ASSERT(cmd != NULL);		/* and so is cmpkt != NULL */

	if (cmd->handle == NULL) {
		stat = ddi_dma_buf_setup(unit->scsi_tran->tran_hba_dip,
		    bp, DDI_DMA_RDWR, callback, arg, &unit->dma_lim,
		    &cmd->handle);
		if (stat) {
			switch (stat) {
			case DDI_DMA_NORESOURCES:
				bp->b_error = 0;
				break;
			case DDI_DMA_TOOBIG:
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR;
				break;
			case DDI_DMA_NOMAPPING:
			default:
				bp->b_error = EFAULT;
				bp->b_flags |= B_ERROR;
				break;
			}
			return (NULL);
		}
	}

	/* Move to the next window */
	stat = ddi_dma_nextwin(cmd->handle, cmd->dmawin, &cmd->dmawin);

	if (stat == DDI_DMA_STALE)
		return (NULL);
	if (stat == DDI_DMA_DONE) {
		/* reset to the first window */
		if (ddi_dma_nextwin(cmd->handle, NULL, &cmd->dmawin) !=
		    DDI_SUCCESS)
			return (NULL);
		cmd->dmaseg = NULL;
	}

	return (cmpkt);
}

/*ARGSUSED*/
void
mlx_dac_memfree(
    const mlx_unit_t *const unit,
    register struct cmpkt *const cmpkt)
{
	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	ASSERT(cmpkt != NULL);

	ddi_dma_free(((mlx_dac_cmd_t *)cmpkt)->handle);
}

/* Abort specific command on target device */
/*ARGSUSED*/
int
mlx_dac_abort(const mlx_unit_t *const unit, const struct cmpkt *const cmpkt)
{
	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	/* Mylex DAC960 does not support recall of command in process */
	return (0);
}

/*
 * Reset all the System Drives or a single one.
 * Returns 1 on success and 0 on failure.
 */
/*ARGSUSED*/
int
mlx_dac_reset(register mlx_unit_t *const unit, int level)
{
#if 1	/* XXX - Re:  Comment on mlx_reset(). */
	return (0);
#else /* XXX */
	register int rval = 1;
	struct scsi_address sa;
	register mlx_t *mlx;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	mlx = unit->hba->mlx;
	ASSERT(mlx != NULL);

	switch (level) {
	case RESET_ALL: { /* Reset all SCSI disks even the standby ones */
		u_char chn;
		u_char nchn = mlx->nchn;

		/*
		 * It is best to reset all the channels.  The caveat is that
		 * all non-disk drive targets will be reset too, which may
		 * or may not be desirable.  The cost of distinguishing disks
		 * from non-disks (e.g. sending SCSI Inquiry commands to all
		 * targets) does not justify the benefits.  Furthermore,
		 * currently there is no way to find out the address of the
		 * mlx_unit_t of other System Drives in the system to invoke
		 * mlx_dac_reset(?, RESET_TARGET) on all the System Drives.
		 */

		for (chn = 0; chn < nchn; chn++) {
			sa.a_target = (ushort)(chn << 4);
			/*
			 * We fail even if one channel cannot be reset.
			 *
			 * We have an option to return immediately
			 * after the 1st failure.  But it is better
			 * to continue, so that ALL the failures are
			 * reported at once instead of having the
			 * the problems of one channel fixed then
			 * reporting that there are others to be
			 * fixed as well.
			 */
			if (!mlx_reset(&sa, RESET_ALL))
				rval = 0;
		}
		break;
	}
	case RESET_TARGET: { /* Reset all the SCSI disks in the System Drive */
		u_char nsd;
		u_char narm;
		register int s;
		register int a;
		register mlx_dac_sd_t *sd;
		register mlx_dac_conf_t *conf;

		mutex_enter(&mlx->mutex);
		conf = mlx->conf;
		ASSERT(conf != NULL);
		nsd = conf->nsd;
		ASSERT(nsd);
		ASSERT(nsd <= MLX_DAC_MAX_SD);

		s = unit->dac_unit.sd_num;
		ASSERT(s < nsd);
		sd = &conf->sd[s];
		ASSERT(sd != NULL);
		narm = sd->narm;
		ASSERT(narm <= MLX_DAC_MAX_ARM);

		for (a = 0; a < narm; a++) {
			u_char ndisk;
			register int d;
			register mlx_dac_arm_t *arm = &sd->arm[a];

			ASSERT(arm != NULL);
			ndisk = arm->ndisk;
			ASSERT(ndisk <= MLX_DAC_MAX_DISK);
			for (d = 0; d < ndisk; d++) {
				register mlx_dac_disk_t *disk = &arm->disk[d];

				ASSERT(disk != NULL);
				sa.a_target = (disk->chn << 4) | (disk->tgt);
				/*
				 * We fail even if one SCSI disk could not be
				 * reset and started.
				 *
				 * We have an option to return immediately
				 * after the 1st failure.  But it is better
				 * to continue, so that ALL the failures are
				 * reported at once instead of having one disk
				 * fixed then reporting that there are others
				 * to be fixed as well.
				 *
				 * mlx_dacioc() grabs mlx->mutex, so we have
				 * to release it here.
				 */
				mutex_exit(&mlx->mutex);
				if (!mlx_reset(&sa, RESET_TARGET))
					rval = 0;
				mutex_enter(&mlx->mutex);
			}
		}
		mutex_exit(&mlx->mutex);
		break;
	}
	default:
		MDBG3(("mlx_dac_reset: bad level %x", level));
		rval = 0;
		break;
	}
	return (rval);
#endif /* XXX */
}

struct cmpkt *
mlx_dac_pktalloc(
    register mlx_unit_t *const unit,
    int (*const callback)(),
    const caddr_t arg)
{
	register mlx_dac_cmd_t *cmd;
	register mlx_ccb_t *ccb;
	mlx_hba_t *hba;
	int kf = GDA_KMFLAG(callback);

	hba = unit->hba;
	ASSERT(hba != NULL);
	ASSERT(MLX_DAC(hba));

	cmd = kmem_zalloc(sizeof (mlx_dac_cmd_t) + sizeof (mlx_ccb_t), kf);

	if (cmd == NULL) {
		if (callback != DDI_DMA_DONTWAIT)
			ddi_set_callback(callback, arg, &hba->callback_id);
		return (NULL);
	}
	ccb = (mlx_ccb_t *)(cmd + 1);
	ccb->paddr = MLX_KVTOP(ccb);
	ccb->type = MLX_DAC_CTYPE1;

	cmd->ccb = ccb;
	cmd->cmpkt.cp_cdblen = 1;
	cmd->cmpkt.cp_cdbp = (opaque_t)&cmd->cdb;
	cmd->cmpkt.cp_scblen = 1;
	cmd->cmpkt.cp_scbp = (opaque_t)&cmd->scb;
	cmd->cmpkt.cp_ctl_private = (opaque_t)unit;

	return ((struct cmpkt *)cmd);
}

void
mlx_dac_pktfree(register mlx_unit_t *const unit, register struct cmpkt *cmpkt)
{
	register mlx_hba_t *hba;

	ASSERT(unit != NULL);
	hba = unit->hba;
	ASSERT(hba != NULL);
	ASSERT(MLX_DAC(hba));

	kmem_free(cmpkt, sizeof (mlx_dac_cmd_t) + sizeof (mlx_ccb_t));

	if (hba->callback_id)
		ddi_run_callback(&hba->callback_id);
}

int
mlx_dac_transport(
    register mlx_unit_t *const unit,
    register struct cmpkt *const cmpkt)
{
	register mlx_ccb_t *ccb;
	register mlx_t *mlx;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	ccb = ((mlx_dac_cmd_t *)cmpkt)->ccb;
	ASSERT(ccb != NULL);
	mlx = unit->hba->mlx;
	ASSERT(mlx != NULL);

	ccb->ccb_ownerp = (struct scsi_cmd *)cmpkt;
	mutex_enter(&mlx->mutex);
	if (mlx_sendcmd(mlx, ccb, 0) == DDI_FAILURE) {
		mutex_exit(&mlx->mutex);
		return (CTL_SEND_FAILURE);
	}
	if (cmpkt->cp_flags & CPF_NOINTR) {
		MLX_DISABLE_INTR(mlx);
		(void) mlx_pollstat(mlx, ccb, 0);
		/*
		 * Ignored DDI_FAILURE return possibility of
		 * mlx_pollstat() as cp_reason is set there already.
		 */
		MLX_ENABLE_INTR(mlx);
	}
	mutex_exit(&mlx->mutex);
	return (CTL_SEND_SUCCESS);
}

/*
 * At the end of successful completion of any command which changes the
 * configuration one way or the other, mlx->conf and mlx->enq will be
 * updated as these need to be reliably up-to-date for the other parts
 * of the driver.  The whole point is that the system need not be
 * rebooted after any command which changes the configuration.
 *
 * Type 3 (Direct CDB) commands should not end up here.
 */
int
mlx_dacioc(
    register mlx_unit_t *const unit,
    int cmd,
    int arg,
    int mode)
{
	int rval;
	register mlx_ccb_t *ccb;
	register mlx_dacioc_t *dacioc;
	register struct cmpkt *cmpkt;

	switch (cmd) {
	case DIOCTL_GETGEOM:
	case DIOCTL_GETPHYGEOM:
	case MLX_DACIOC_CARDINFO:
		/*
		 * These are not sent to the controller, hence
		 * require no packet setup.  And handled entirely
		 * in this driver.
		 */
		return (mlx_dacioc_nopkt(unit, cmd, arg, mode));
	}

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));

	MDBG3(("mlx_dacioc: unit=%x, cmd=%x, arg=%x, mode=%x",
		unit, cmd, arg, mode));

	if (arg == NULL)
		return (EINVAL);

	cmpkt = mlx_dacioc_pktalloc(unit, cmd, arg, mode, &rval);
	if (cmpkt == NULL) {
		ASSERT(rval);
		return (rval);
	}
	ccb = ((mlx_dac_cmd_t *)cmpkt)->ccb;
	ASSERT(ccb != NULL);
	dacioc = (mlx_dacioc_t *)cmpkt->cp_private;
	ASSERT(dacioc != NULL);

	if (mlx_dac_transport(unit, cmpkt) == CTL_SEND_SUCCESS) {
		sema_p(&ccb->ccb_da_sema);
		/*
		 * XXX - If mlx->conf and mlx->enq needed to be updated,
		 * there is a window between NOW that EEPROM is updated
		 * and when mlx->conf and mlx-enq get updated at the end
		 * of mlx_dacioc_done().
		 *
		 * Perhaps we should hold mlx->mutex here and don't hold it
		 * in mlx_dacioc_update_conf_enq().  But that is a serious
		 * performance hit.
		 */
		rval = mlx_dacioc_done(unit, cmpkt, ccb, dacioc, arg, mode);
	} else {
		rval = EIO;
		MDBG3(("mlx_dacioc: transport failure"));
	}

	mlx_dacioc_pktfree(unit, cmpkt, ccb, dacioc, mode);
	return (rval);
}

/*
 * It handles the ioctl's which are not sent to the adapter and require
 * no pkt setup.
 */
int
mlx_dacioc_nopkt(
    register mlx_unit_t *const unit,
    int cmd,
    int arg,
    int mode)
{
	register mlx_t  *mlx;
	ushort		heads   = 64;
	ushort		sectors = 32;
	int		geom;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	mlx = unit->hba->mlx;
	ASSERT(mlx != NULL);

	MDBG3(("mlx_dacioc_nopkt: unit=%x, cmd=%x, arg=%x, mode=%x",
		unit, cmd, arg, mode));

	switch (cmd) {
	case DIOCTL_GETGEOM:
	case DIOCTL_GETPHYGEOM: {
		register struct tgdk_geom *tg = (struct tgdk_geom *)arg;

		mutex_enter(&mlx->mutex);
		ASSERT(mlx->enq != NULL);
		tg->g_cap = mlx->enq->sd_sz[unit->dac_unit.sd_num];
		mutex_exit(&mlx->mutex);

		geom = MLX_GEOMETRY(mlx, NULL, tg->g_cap);
		heads = geom >> 16;
		sectors = geom & 0xffff;

		tg->g_acyl = 0;
		tg->g_secsiz = MLX_BLK_SIZE;
		/* Fill in some reasonable values for the rest */
		tg->g_cyl  = tg->g_cap / (sectors * heads);
		tg->g_head = heads;
		tg->g_sec  = sectors;

		return (DDI_SUCCESS);
	}
	case MLX_DACIOC_CARDINFO: {
		mlx_dacioc_t dacioc;
		mlx_dacioc_cardinfo_t cardinfo;

		if (mode == FKIOCTL) {
			MDBG3(("mlx_dacioc_nopkt: cmd %x should not be "
				"called from kernel", cmd));
			return (EINVAL);
		}
		if (arg == NULL)
			return (EINVAL);

		if (copyin((caddr_t)arg, (caddr_t)&dacioc, sizeof (dacioc))) {
			MDBG3(("mlx_dacioc_nopkt: failed to copyin arg"));
			return (EFAULT);
		}

		if (dacioc.ubuf_len != sizeof (mlx_dacioc_cardinfo_t)) {
			MDBG3(("mlx_dacioc_nopkt: invalid ubuf_len %x",
				dacioc.ubuf_len));
			return (EINVAL);
		}

		cardinfo.slot = mlx->reg >> 12;
		cardinfo.nchn = mlx->nchn;
		cardinfo.max_tgt = mlx->max_tgt;
		cardinfo.irq = mlx->irq;
		if (copyout((caddr_t)&cardinfo, dacioc.ubuf,
		    dacioc.ubuf_len)) {
			MDBG3(("mlx_dacioc_nopkt: failed to copyout to "
			    "user buffer, ubuf_len=%x", dacioc.ubuf_len));
			return (EFAULT);
		}

		dacioc.status = MLX_SUCCESS;
		if (copyout((caddr_t)&dacioc, (caddr_t)arg, sizeof (dacioc))) {
			MDBG3(("mlx_dacioc_nopkt: failed to copyout arg"));
			return (EFAULT);
		}
		return (DDI_SUCCESS);
	}
	default:
#if 0	/* XXX - Pending PSARC approval */
		ASSERT(0);	/* cmd requires packet set up */
#endif /* XXX */
		return (DDI_FAILURE);
	}
}

/*
 * Prepares a packet with all the necessary allocations and setups
 * to be transported to the adapter for the particular ioctl.
 */
struct cmpkt *
mlx_dacioc_pktalloc(
    register mlx_unit_t *const unit,
    int cmd,
    int arg,
    int mode,
    int *const err)
{
	int rval;
	register mlx_ccb_t *ccb;
	register mlx_dacioc_t *dacioc;
	register struct cmpkt *cmpkt;
	register mlx_t *mlx;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	mlx = unit->hba->mlx;
	ASSERT(mlx != NULL);
	ASSERT(arg != NULL);

	cmpkt = mlx_dac_pktalloc(unit, DDI_DMA_DONTWAIT, NULL);
	if (cmpkt == NULL) {
		MDBG3(("mlx_dacioc_pktalloc: failed to allocate cmpkt"));
		*err = ENOMEM;
		return (NULL);
	}
	ccb = ((mlx_dac_cmd_t *)cmpkt)->ccb;

	switch (cmd) {
	case MLX_DACIOC_FLUSH:
		ccb->type = MLX_DAC_CTYPE0;
		ccb->ccb_opcode = MLX_DAC_FLUSH;
		ccb->ccb_flags = MLX_CCB_DACIOC_NO_DATA_XFER;
		break;
	case MLX_DACIOC_SETDIAG:
		ccb->type = MLX_DAC_CTYPE0;
		ccb->ccb_opcode = MLX_DAC_SETDIAG;
		ccb->ccb_flags = MLX_CCB_DACIOC_NO_DATA_XFER;
		break;
	case MLX_DACIOC_SIZE:
		/* ccb->type aleady set to MLX_DAC_CTYPE1 in mlx_dac_pktalloc */
		ccb->ccb_opcode = MLX_DAC_SIZE;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_CHKCONS:
		/* ccb->type aleady set to MLX_DAC_CTYPE1 in mlx_dac_pktalloc */
		ccb->ccb_opcode = MLX_DAC_CHKCONS;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_RBLD:
		ccb->type = MLX_DAC_CTYPE2;
		ccb->ccb_opcode = MLX_DAC_RBLD;
		ccb->ccb_flags |=
		    MLX_CCB_UPDATE_CONF_ENQ | MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_START:
		ccb->type = MLX_DAC_CTYPE2;
		ccb->ccb_opcode = MLX_DAC_START;
		ccb->ccb_flags |=
		    MLX_CCB_UPDATE_CONF_ENQ | MLX_CCB_DACIOC_NO_DATA_XFER;
		break;
	case MLX_DACIOC_STOPC:
		ccb->type = MLX_DAC_CTYPE2;
		ccb->ccb_opcode = MLX_DAC_STOPC;
		ccb->ccb_flags = MLX_CCB_DACIOC_NO_DATA_XFER;
		break;
	case MLX_DACIOC_STARTC:
		ccb->type = MLX_DAC_CTYPE2;
		ccb->ccb_opcode = MLX_DAC_STARTC;
		ccb->ccb_flags = MLX_CCB_DACIOC_NO_DATA_XFER;
		break;
	case MLX_DACIOC_GSTAT:
		ccb->type = MLX_DAC_CTYPE2;
		ccb->ccb_opcode = MLX_DAC_GSTAT;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_RBLDA:
		ccb->type = MLX_DAC_CTYPE2;
		ccb->ccb_opcode = MLX_DAC_RBLDA;
		ccb->ccb_flags = MLX_CCB_DACIOC_NO_DATA_XFER;
		/*
		 * mlx->conf and mlx->enq need to be updated, but only
		 * when MLX_DACIOC_RBLDSTAT is called and it returns
		 * status indicating that no more rebuilds are in progress.
		 */
		break;
	case MLX_DACIOC_RESETC:
		ccb->type = MLX_DAC_CTYPE2;
		ccb->ccb_opcode = MLX_DAC_RESETC;
		ccb->ccb_flags |=
		    MLX_CCB_UPDATE_CONF_ENQ | MLX_CCB_DACIOC_NO_DATA_XFER;
		break;
	case MLX_DACIOC_RUNDIAG:
		ccb->type = MLX_DAC_CTYPE4;
		ccb->ccb_opcode = MLX_DAC_RUNDIAG;
		ccb->ccb_flags |=
		    MLX_CCB_DACIOC_DAC_TO_UBUF | MLX_CCB_DACIOC_UBUF_TO_DAC;
		break;
	case MLX_DACIOC_ENQUIRY:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_ENQUIRY;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		/*
		 * Normally, the user requests a MLX_DACIOC_ENQUIRY as the
		 * first thing in Mylex DAC960 diagnostics or monitoring.
		 * Returning mlx->enq is not prudent as the Enquiry Info
		 * on the card might have changed since driver initial-
		 * ization or the last update on mlx->enq.  It is best
		 * to go ahead with this Enquiry request through the
		 * card and then update mlx->enq and mlx->conf.
		 */
		if (mode != FKIOCTL)		/* called from user land */
			ccb->ccb_flags |= MLX_CCB_UPDATE_CONF_ENQ;
		break;
	case MLX_DACIOC_WRCONFIG:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_WRCONFIG;
		/*
		 * As there are commonality between ROM Configuration
		 * and Enquiry Info, we better update mlx->enq as well.
		 */
		ccb->ccb_flags |=
		    MLX_CCB_UPDATE_CONF_ENQ | MLX_CCB_DACIOC_UBUF_TO_DAC;
		break;
	case MLX_DACIOC_RDCONFIG:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_RDCONFIG;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		/*
		 * The configuration info might have changed since it
		 * was last updated.  So, it's best to go ahead with
		 * this Read Rom Configuration request through the card
		 * and then update mlx->conf and mlx->enq instead of
		 * passing what we have in mlx->conf to the user.
		 */
		if (mode != FKIOCTL) 		/* called from user land */
			ccb->ccb_flags |= MLX_CCB_UPDATE_CONF_ENQ;
		break;
	case MLX_DACIOC_RBADBLK:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_RBADBLK;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_RBLDSTAT:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_RBLDSTAT;
		/*
		 * Update mlx->conf and mlx->enq only if
		 * returns MLX_E_NORBLDCHK.
		 */
		ccb->ccb_flags |=
		    MLX_CCB_UPDATE_CONF_ENQ | MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_GREPTAB:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_GREPTAB;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_GEROR:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_GEROR;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_ADCONFIG:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_ADCONFIG;
		ccb->ccb_flags |=
		    MLX_CCB_UPDATE_CONF_ENQ | MLX_CCB_DACIOC_UBUF_TO_DAC;
		break;
	case MLX_DACIOC_SINFO:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_SINFO;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_RDNVRAM:
		ccb->type = MLX_DAC_CTYPE5;
		ccb->ccb_opcode = MLX_DAC_RDNVRAM;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_LOADIMG:
		ccb->type = MLX_DAC_CTYPE6;
		ccb->ccb_opcode = MLX_DAC_LOADIMG;
		ccb->ccb_flags = MLX_CCB_DACIOC_DAC_TO_UBUF;
		break;
	case MLX_DACIOC_STOREIMG:
		ccb->type = MLX_DAC_CTYPE6;
		ccb->ccb_opcode = MLX_DAC_STOREIMG;
		ccb->ccb_flags |=
		    MLX_CCB_UPDATE_CONF_ENQ | MLX_CCB_DACIOC_UBUF_TO_DAC;
		break;
	case MLX_DACIOC_PROGIMG:
		ccb->type = MLX_DAC_CTYPE6;
		ccb->ccb_opcode = MLX_DAC_PROGIMG;
		ccb->ccb_flags |=
		    MLX_CCB_UPDATE_CONF_ENQ | MLX_CCB_DACIOC_NO_DATA_XFER;
		break;
	case MLX_DACIOC_GENERIC:
		ccb->type = MLX_DACIOC_CTYPE_GEN;
		ccb->ccb_flags |= MLX_CCB_UPDATE_CONF_ENQ; /* conservative */
		break;
	default:
		MDBG3(("mlx_dacioc_pktalloc: bad ioctl cmd %x", cmd));
		mlx_dac_pktfree(unit, cmpkt);
		*err = ENOTTY;
		return (NULL);
	}
	ccb->ccb_ubuf_len = (cmd == MLX_DACIOC_GENERIC) ? 0 :
	    mlx_dacioc_ubuf_len(mlx, ccb->ccb_opcode);

	/* indicators, will use later */
	cmpkt->cp_private = NULL;
	cmpkt->cp_bp = NULL;

	rval = mlx_dacioc_getarg(mlx, unit, arg, cmpkt, ccb, mode);
	dacioc = (mlx_dacioc_t *)cmpkt->cp_private;
	if (rval != DDI_SUCCESS) {
		mlx_dacioc_pktfree(unit, cmpkt, ccb, dacioc, mode);
		*err = rval;
		return (NULL);
	}

	sema_init(&ccb->ccb_da_sema, 0, "Mylex DAC960 Ioctl Per Pkt Semaphore",
	    SEMA_DRIVER, NULL);
	ccb->ccb_flags |= MLX_CCB_GOT_DA_SEMA;
	cmpkt->cp_callback = mlx_dacioc_callback;
	*err = 0;
	return (cmpkt);
}

/* Based on the ccb_opcode returns the expected ubuf_len */
ushort
mlx_dacioc_ubuf_len(register mlx_t *const mlx, const u_char opcode)
{
	switch (opcode) {
	case MLX_DAC_SIZE:
		return (MLX_DACIOC_SIZE_UBUF_LEN);
	case MLX_DAC_CHKCONS:
		return (MLX_DACIOC_CHKCONS_UBUF_LEN);
	case MLX_DAC_RBLD:
		return (MLX_DACIOC_RBLD_UBUF_LEN);
	case MLX_DAC_GSTAT:
		return (MLX_DACIOC_GSTAT_UBUF_LEN);
	case MLX_DAC_RUNDIAG:
		return (MLX_DACIOC_RUNDIAG_UBUF_LEN);
	case MLX_DAC_ENQUIRY:
		return (MLX_DACIOC_ENQUIRY_UBUF_LEN);
	case MLX_DAC_WRCONFIG:
		return (MLX_DACIOC_WRCONFIG_UBUF_LEN +
		    (mlx->nchn * mlx->max_tgt * 12));
	case MLX_DAC_RDCONFIG:
		return (MLX_DACIOC_RDCONFIG_UBUF_LEN +
		    (mlx->nchn * mlx->max_tgt * 12));
	case MLX_DAC_RBADBLK:
		return (MLX_DACIOC_RBADBLK_UBUF_LEN);
	case MLX_DAC_RBLDSTAT:
		return (MLX_DACIOC_RBLDSTAT_UBUF_LEN);
	case MLX_DAC_GREPTAB:
		return (MLX_DACIOC_GREPTAB_UBUF_LEN);
	case MLX_DAC_GEROR:
		return (MLX_DACIOC_GEROR_UBUF_LEN +
		    (mlx->nchn * mlx->max_tgt * 4));
	case MLX_DAC_ADCONFIG:
		return (MLX_DACIOC_ADCONFIG_UBUF_LEN +
		    (mlx->nchn * mlx->max_tgt * 12));
	case MLX_DAC_SINFO:
		return (MLX_DACIOC_SINFO_UBUF_LEN);
	case MLX_DAC_RDNVRAM:
		return (MLX_DACIOC_RDNVRAM_UBUF_LEN +
		    (mlx->nchn * mlx->max_tgt * 12));
	case MLX_DAC_FLUSH:
	case MLX_DAC_SETDIAG:
	case MLX_DAC_START:
	case MLX_DAC_STOPC:
	case MLX_DAC_STARTC:
	case MLX_DAC_RBLDA:
	case MLX_DAC_RESETC:
	case MLX_DAC_PROGIMG:
	default:
		return (0);
	}
}

int
mlx_dacioc_getarg(
    mlx_t *const mlx,
    register mlx_unit_t *const unit,
    int arg,
    register struct cmpkt *const cmpkt,
    register mlx_ccb_t *const ccb,
    int mode)
{
	ushort ubuf_len;
	register mlx_dacioc_t *dacioc;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	ASSERT(arg != NULL);
	ASSERT(cmpkt != NULL);
	ASSERT(ccb != NULL);

	if (mode == FKIOCTL) {		/* called from kernel */
		dacioc = (mlx_dacioc_t *)arg;
		cmpkt->cp_private = (opaque_t)dacioc;
	} else {
		dacioc = (mlx_dacioc_t *)kmem_zalloc(sizeof (*dacioc),
		    KM_NOSLEEP);
		if (dacioc == NULL) {
			MDBG3(("mlx_dacioc_getarg: "
				"failed to allocate dacioc"));
			return (ENOMEM);
		}
		cmpkt->cp_private = (opaque_t)dacioc;

		if (copyin((caddr_t)arg, (caddr_t)dacioc, sizeof (*dacioc))) {
			MDBG3(("mlx_dacioc_getarg: failed to copyin arg"));
			return (EFAULT);
		}
	}

	ubuf_len = dacioc->ubuf_len;
	if (!mlx_dacioc_valid_args(mlx, ccb, ubuf_len, dacioc)) {
		MDBG3(("mlx_dacioc_getarg: invalid arg, "
		    "type=%x, ubuf_len=%x, flags=%x, dacioc=%x",
		    ccb->type, ubuf_len, dacioc->flags, dacioc));
		return (EINVAL);
	}

	switch (ccb->type) {
	case MLX_DAC_CTYPE1:
		ccb->ccb_drv = dacioc->dacioc_drv;
		CCB_BLK(ccb, dacioc->dacioc_blk);
		ccb->ccb_cnt = dacioc->dacioc_cnt;
		break;
	case MLX_DAC_CTYPE2:
		ccb->ccb_chn = dacioc->dacioc_chn;
		ccb->ccb_tgt = dacioc->dacioc_tgt;
		ccb->ccb_dev_state = dacioc->dacioc_dev_state;
		break;
	case MLX_DAC_CTYPE4:
		ccb->ccb_test = dacioc->dacioc_test;
		ccb->ccb_pass = dacioc->dacioc_pass;
		ccb->ccb_chan = dacioc->dacioc_chan;
		break;
	case MLX_DAC_CTYPE5:
		ccb->ccb_param = dacioc->dacioc_param;
		break;
	case MLX_DAC_CTYPE6:
		ccb->ccb_count = dacioc->dacioc_count;
		ccb->ccb_offset = dacioc->dacioc_offset;
		break;
	case MLX_DACIOC_CTYPE_GEN: {
		ulong len = ccb->ccb_gen_args_len = dacioc->dacioc_gen_args_len;
		register mlx_dacioc_generic_args_t *gen_args;

		ccb->ccb_xferaddr_reg = dacioc->dacioc_xferaddr_reg;
		ASSERT(len);
		ASSERT(!(len % sizeof (*gen_args)));
		gen_args = ccb->ccb_gen_args =
		    (mlx_dacioc_generic_args_t *)kmem_zalloc(len, KM_NOSLEEP);
		if (gen_args == NULL) {
			MDBG3(("mlx_dacioc_getarg: not enough memory for "
				"generic ioctl args"));
			return (ENOMEM);
		}

		if (ddi_copyin((caddr_t)dacioc->dacioc_gen_args,
		    (caddr_t)gen_args, len, mode)) {
			MDBG3(("mlx_dacioc_getarg: failed to copyin "
			    "gen_args"));
			return (EFAULT);
		}

		for (; len != 0; gen_args++, len -= sizeof (*gen_args)) {
			ushort reg_addr;

			ASSERT(gen_args != NULL);
			reg_addr = gen_args->reg_addr;

			if ((reg_addr & 0xff00) !=	/* slot# */
			    (mlx->reg & 0xff00)) {
				cmn_err(CE_WARN, "mlx_dacioc_getarg: "
				    "invalid reg_addr=%x destined for slot %d",
				    reg_addr, reg_addr & 0xf000);
				return (EINVAL);
			} else if (reg_addr & 0xF == MLX_MBXCMDID) {
				cmn_err(CE_WARN, "mlx_dacioc_getarg: "
				    "assignment to cmdid reg %x",
				    mlx->reg + MLX_MBXCMDID);
				return (EINVAL);
			} else if (reg_addr & 0xF == MLX_MBXCMD) {
				u_char opcode;
				ushort len;

				ccb->ccb_opcode = opcode = gen_args->val;
				ccb->ccb_ubuf_len = len =
				    mlx_dacioc_ubuf_len(mlx, ccb->ccb_opcode);
				if (opcode != MLX_DAC_LOADIMG &&
				    opcode != MLX_DAC_STOREIMG &&
				    ubuf_len != len) {
					cmn_err(CE_WARN, "mlx_dacioc_getarg: "
					    "unexpected ubuf_len=%x, "
					    "opcode=%x", ubuf_len,
					    opcode);
					/* can't tell if it's invalid or not! */
				}
			} else
				ccb->ccb_arr[(reg_addr & 0xF) - MLX_MBX2] =
					gen_args->val;
		}
		break;
	}
	defaut:
		ASSERT(0);		/* bad command type */
		break;
	}

	ccb->ccb_xferpaddr = (paddr_t)0;	/* indicator, will use later */
	if (ubuf_len) {
		caddr_t kv_ubuf;

		ASSERT(dacioc->flags &
		    (MLX_DACIOC_UBUF_TO_DAC | MLX_DACIOC_DAC_TO_UBUF));

		if (ddi_iopb_alloc(unit->hba->dip, &unit->dma_lim,
		    (u_int)ubuf_len, &kv_ubuf)) {
			MDBG3(("mlx_dacioc_getarg: failed to allocate dma "
				"buffer, ubuf_len=%x", ubuf_len));
			return (ENOMEM);
		}
		cmpkt->cp_bp = (buf_t *)kv_ubuf;
		ccb->ccb_xferpaddr = MLX_KVTOP(kv_ubuf);

		/*
		 * Both MLX_DACIOC_DAC_TO_UBUF and MLX_DACIOC_UBUF_TO_DAC
		 * could be set in dacioc->flags.
		 */
		if (dacioc->flags & MLX_DACIOC_UBUF_TO_DAC) {
			if (ddi_copyin(dacioc->ubuf, kv_ubuf,
			    ubuf_len, mode)) {
				MDBG3(("mlx_dacioc_getarg: failed to "
					"copyin from user buffer, ubuf_len=%x",
					ubuf_len));
				return (EFAULT);
			}
		} else {
			ASSERT(dacioc->flags & MLX_DACIOC_DAC_TO_UBUF);
			bzero(kv_ubuf, (size_t)ubuf_len);
		}

		if (ccb->ccb_opcode == MLX_DAC_SIZE &&
		    (ccb->ccb_xferpaddr & 3)) {
			MDBG3(("mlx_dacioc_getarg: ccb_opcode=%x, "
			    "ccb_xferpaddr=%x not on 4 byte boundary",
			    ccb->ccb_opcode, ccb->ccb_xferpaddr));
			return (EINVAL);
		}
	}
	return (DDI_SUCCESS);
}

/* Returns 1 if the args are valid, otherwise 0 */
int
mlx_dacioc_valid_args(
    mlx_t *const mlx,
    register mlx_ccb_t *const ccb,
    const ushort ubuf_len,
    register mlx_dacioc_t *const dacioc)
{
	ASSERT(mlx != NULL);
	ASSERT(ccb != NULL);
	ASSERT(dacioc != NULL);

	if (ubuf_len) {
		if (ubuf_len > MLX_DAC_MAX_XFER) {
			MDBG3(("mlx_dacioc_valid_args: ubuf_len > max (%x)",
			    MLX_DAC_MAX_XFER));
			return (0);
		}
		if (!(dacioc->flags &
		    (MLX_DACIOC_UBUF_TO_DAC | MLX_DACIOC_DAC_TO_UBUF))) {
			MDBG3(("mlx_dacioc_valid_args: data xfer direction "
			    "flag is incorrect"));
			return (0);
		}
	}

	if (ccb->type == MLX_DACIOC_CTYPE_GEN) {
		ulong len = dacioc->dacioc_gen_args_len;

		if (!len || len % sizeof (mlx_dacioc_generic_args_t)) {
			MDBG3(("mlx_dacioc_valid_args: invalid gen_args_len"
			    "(%x) in generic ioctl", len));
			return (0);
		}
		return (1);
	}

	if (!ubuf_len && !(ccb->ccb_flags & MLX_CCB_DACIOC_NO_DATA_XFER)) {
		MDBG3(("mlx_dacioc_valid_args: invalid ubuf_len, expected "
		    "non-zero"));
		return (0);
	}
	if (ubuf_len) {
		ushort len;

		if (ccb->ccb_flags & MLX_CCB_DACIOC_NO_DATA_XFER) {
			MDBG3(("mlx_dacioc_valid_args: invalid ubuf_len, "
			    "expected 0"));
			return (0);
		}
		if ((dacioc->flags & MLX_DACIOC_UBUF_TO_DAC) &&
		    !(ccb->ccb_flags & MLX_CCB_DACIOC_UBUF_TO_DAC)) {
			MDBG3(("mlx_dacioc_valid_args: invalid data xfer "
			    "direction flag"));
			return (0);
		}
		if ((dacioc->flags & MLX_DACIOC_DAC_TO_UBUF) &&
		    !(ccb->ccb_flags & MLX_CCB_DACIOC_DAC_TO_UBUF)) {
			MDBG3(("mlx_dacioc_valid_args: bad data xfer "
			    "direction flag"));
			return (0);
		}

		if (ccb->ccb_opcode == MLX_DAC_LOADIMG ||
		    ccb->ccb_opcode == MLX_DAC_STOREIMG)
			len = dacioc->dacioc_count;
		else
			len = ccb->ccb_ubuf_len;
		if (ubuf_len != len) {
			MDBG3(("mlx_dacioc_valid_args: bad ubuf_len, "
			    "expected %x", len));
			return (0);
		}
	}

	/*
	 * There are special ioctl's which have to be performed on System
	 * Drives with data redundancy, i.e. RAID levels 1, 3, 4, 5 and 6.
	 * The adapter performs this check for some (e.g. MLX_DACIOC_CHKCONS)
	 * and ignores this check for the others for performance reasons.
	 * Therefore, here we have to perform the check for the latter ones.
	 * XXX - This is a bug in the f/w that we have to compensate for.
	 */
	switch (ccb->ccb_opcode) {
	case MLX_DAC_RBLD:
	case MLX_DAC_RBLDA: {
		register mlx_dac_sd_t *sd;

		mutex_enter(&mlx->mutex);
		sd = mlx_in_any_sd(mlx, dacioc->dacioc_chn, dacioc->dacioc_tgt);
		if (sd == NULL) {
			MDBG3(("mlx_dacioc_valid_args: not in any sd"));
			mutex_exit(&mlx->mutex);
			return (0);
		}
		switch (sd->raid) {
		case 1:
		case 3:
		case 4:
		case 5:
		case 6:
			break;
		default:
			MDBG3(("mlx_dacioc_valid_args: RAID-%d doesn't "
			    "contain data redundancy", sd->raid));
			mutex_exit(&mlx->mutex);
			return (0);
		}
		mutex_exit(&mlx->mutex);
		break;
	}
	default:
		break;
	}

	return (1);
}

/*
 * This routine will be called at interrupt time for all the
 * packets created and successfully transferred by mlx_dacioc().
 */
void
mlx_dacioc_callback(register struct cmpkt *const cmpkt)
{
	ASSERT(cmpkt != NULL);

	sema_v(&((mlx_dac_cmd_t *)cmpkt)->ccb->ccb_da_sema);
}

/*
 * Post processing the ioctl packet received from the adapter and
 * delivered by the interrupt routine.
 */
int
mlx_dacioc_done(
    register mlx_unit_t *const unit,
    register struct cmpkt *const cmpkt,
    register mlx_ccb_t *const ccb,
    register mlx_dacioc_t *const dacioc,
    int arg,
    int mode)
{
	ushort ubuf_len;
	int scb;
	caddr_t kv_ubuf;

	ASSERT(unit != NULL);
	ASSERT(cmpkt != NULL);
	ASSERT(unit == (mlx_unit_t *)cmpkt->cp_ctl_private);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	ASSERT(ccb != NULL);
	ASSERT(ccb == ((mlx_dac_cmd_t *)cmpkt)->ccb);
	ASSERT(dacioc != NULL);
	ASSERT(dacioc == cmpkt->cp_private);
	ASSERT(arg != NULL);

	scb = (int)(*(char *)cmpkt->cp_scbp);
	if (cmpkt->cp_reason == CPS_CHKERR &&
	    (scb == DERR_ABORT || scb == DERR_SUCCESS))
		cmpkt->cp_reason = CPS_SUCCESS;

	ubuf_len = dacioc->ubuf_len;
	if (ubuf_len) {
		kv_ubuf = (caddr_t)cmpkt->cp_bp;
		ASSERT(kv_ubuf != NULL);
	}

	switch (cmpkt->cp_reason) {
	case CPS_SUCCESS: {
		dacioc->status = ccb->ccb_status;
		if (ubuf_len) {
			ASSERT(ccb->type != MLX_DAC_CTYPE0);

			if ((dacioc->flags & MLX_DACIOC_DAC_TO_UBUF) &&
			    ddi_copyout(kv_ubuf, dacioc->ubuf,
					ubuf_len, mode)) {
				MDBG3(("mlx_dacioc_done: failed to "
				    "copyout  to ubuf, ubuf_len=%x",
				    ubuf_len));
				return (EFAULT);
			}
		}

		if (mode != FKIOCTL && 		/* called from user land */
		    copyout((caddr_t)dacioc, (caddr_t)arg, sizeof (*dacioc))) {
			MDBG3(("mlx_dacioc_done: failed to copyout arg"));
			return (EFAULT);
		}

		return ((ccb->ccb_flags & MLX_CCB_UPDATE_CONF_ENQ) ?
		    mlx_dacioc_update_conf_enq(unit, ccb, dacioc->status) :
		    DDI_SUCCESS);
	}
	case CPS_CHKERR:
		return ((scb == DERR_BUSY) ? EAGAIN : EIO);
	case CPS_ABORTED:
	case CPS_FAILURE:
	default:
		cmn_err(CE_WARN, "mlx_dacioc_callback: bad reason code %x",
			(int)cmpkt->cp_reason);
		return (EIO);
	}
}

/*
 * This function is called only when mlx->conf and mlx->enq need
 * to be updated because of some configuration change through a
 * mlx_dacioc() call.
 */
int
mlx_dacioc_update_conf_enq(
    register mlx_unit_t *const unit,
    register mlx_ccb_t *const ccb,
    const ushort status)
{
	size_t mem;
	register mlx_t *mlx;
	register mlx_dac_conf_t *conf;
	register mlx_dac_enquiry_t *enq;
	mlx_dacioc_t dacioc;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));

	ccb->ccb_flags &= ~MLX_CCB_UPDATE_CONF_ENQ;

	if (ccb->ccb_opcode == MLX_DAC_RBLDSTAT && status != MLX_E_NORBLDCHK) {
		MDBG3(("mlx_dacioc_update_conf_enq: async rebuild is not "
		    "done yet"));
		return (EINPROGRESS);
	}

	ASSERT(unit->hba != NULL);
	mlx = unit->hba->mlx;
	ASSERT(mlx != NULL);

	mem = sizeof (*conf) - sizeof (mlx_dac_tgt_info_t) +
		((mlx->nchn * mlx->max_tgt) * sizeof (mlx_dac_tgt_info_t));
	conf = (mlx_dac_conf_t *)kmem_zalloc(mem, KM_NOSLEEP);
	if (conf == NULL) {
		MDBG3(("mlx_dacioc_update_conf_enq: not enough memory to "
		    "update configuration info."));
		return (ENOMEM);
	}

	bzero((caddr_t)&dacioc, sizeof (mlx_dacioc_t));
	dacioc.ubuf = (caddr_t)conf;
	dacioc.ubuf_len = (ushort)mem;
	ASSERT(dacioc.ubuf_len <= MLX_DAC_MAX_XFER);
	dacioc.flags = MLX_DACIOC_DAC_TO_UBUF;
	if (mlx_dacioc(unit, MLX_DACIOC_RDCONFIG, (int)&dacioc, FKIOCTL) ==
	    DDI_SUCCESS && dacioc.status == MLX_SUCCESS) {
		mutex_enter(&mlx->mutex);
		ASSERT(mlx->conf != NULL);
		kmem_free((caddr_t)mlx->conf, mem);
		mlx->conf = conf;
		mutex_exit(&mlx->mutex);
	} else
		cmn_err(CE_WARN, "mlx_dacioc_update_conf_enq: unable to "
		    "update driver's configuration info.");

	mem = sizeof (mlx_dac_enquiry_t);
	enq = (mlx_dac_enquiry_t *)kmem_zalloc(mem, KM_NOSLEEP);
	if (enq == NULL) {
		MDBG3(("mlx_dacioc_update_conf_enq: not enough memory to "
		    "update the enquiry info."));
		return (ENOMEM);
	}

	bzero((caddr_t)&dacioc, sizeof (mlx_dacioc_t));
	dacioc.ubuf = (caddr_t)enq;
	dacioc.ubuf_len = (ushort)mem;
	ASSERT(dacioc.ubuf_len <= MLX_DAC_MAX_XFER);
	dacioc.flags = MLX_DACIOC_DAC_TO_UBUF;
	if (mlx_dacioc(unit, MLX_DACIOC_ENQUIRY, (int)&dacioc, FKIOCTL) ==
	    DDI_SUCCESS && dacioc.status == MLX_SUCCESS) {
		mutex_enter(&mlx->mutex);
		ASSERT(mlx->enq != NULL);
		kmem_free((caddr_t)mlx->enq, mem);
		mlx->enq = enq;
		mlx_getenq_info(mlx);
		mutex_exit(&mlx->mutex);
	} else
		cmn_err(CE_WARN, "mlx_dacioc_update_conf_enq: unable to "
		    "update driver's enquiry info.");

	return (DDI_SUCCESS);
}

/* Free the ioctl packet */
void
mlx_dacioc_pktfree(
    mlx_unit_t *const unit,
    register struct cmpkt *cmpkt,
    register mlx_ccb_t *const ccb,
    register mlx_dacioc_t *dacioc,
    int mode)
{
	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(MLX_DAC(unit->hba));
	ASSERT(cmpkt != NULL);
	ASSERT(ccb != NULL);
	ASSERT(ccb == ((mlx_dac_cmd_t *)cmpkt)->ccb);
	ASSERT(dacioc != NULL);
	ASSERT(dacioc == cmpkt->cp_private);

	if (cmpkt->cp_bp != NULL)
		ddi_iopb_free((caddr_t)cmpkt->cp_bp);

	if (ccb->type == MLX_DACIOC_CTYPE_GEN && dacioc != NULL &&
	    ccb->ccb_gen_args != NULL)
		kmem_free((caddr_t)ccb->ccb_gen_args,
			    (size_t)ccb->ccb_gen_args_len);

	if (mode != FKIOCTL && dacioc != NULL)
		kmem_free((caddr_t)dacioc, sizeof (*dacioc));

	if (ccb->ccb_flags & MLX_CCB_GOT_DA_SEMA)
		sema_destroy(&ccb->ccb_da_sema);

	mlx_dac_pktfree(unit, cmpkt);
}
