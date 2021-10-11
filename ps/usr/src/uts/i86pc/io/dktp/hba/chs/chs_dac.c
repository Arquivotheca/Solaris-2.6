/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
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

#pragma	ident	"@(#)chs_dac.c	1.3	96/06/18 SMI"

#include "chs.h"

/* Non-SCSI Ctlobj operations */
struct ctl_objops chs_dac_objops = {
	chs_dac_pktalloc,
	chs_dac_pktfree,
	chs_dac_memsetup,
	chs_dac_memfree,
	chs_dac_iosetup,
	chs_dac_transport,
	chs_dac_reset,
	chs_dac_abort,
	nulldev,
	nulldev,
	chs_dacioc_nopkt,
	0, 0
};

int
chs_dac_tran_tgt_init(
    dev_info_t *const hba_dip,
    dev_info_t *const tgt_dip,
    scsi_hba_tran_t *const hba_tran,
    register struct scsi_device *const sd)
{
	int tgt;
	register chs_hba_t *hba;
	register chs_t *chs;
	register chs_unit_t *hba_unit;			/* self */
	register chs_unit_t *child_unit;
	register struct scsi_inquiry *scsi_inq;
	register struct ctl_obj *ctlobjp;

	ASSERT(sd != NULL);
	tgt = sd->sd_address.a_target;

	if (sd->sd_address.a_lun)		/* no support for luns */
		return (DDI_NOT_WELL_FORMED);

	hba_unit = CHS_SDEV2HBA_UNIT(sd);
	/* nexus should already be initialized */
	ASSERT(hba_unit != NULL);
	hba = hba_unit->hba;
	ASSERT(hba != NULL);
	ASSERT(CHS_DAC(hba));
	chs = hba->chs;
	ASSERT(chs != NULL);

	mutex_enter(&chs->mutex);
	ASSERT(chs->conf != NULL);
	if (tgt >=  chs_get_nsd(chs)) {
		mutex_exit(&chs->mutex);
		return (DDI_FAILURE);
	}
	mutex_exit(&chs->mutex);

	scsi_inq = (struct scsi_inquiry *)kmem_zalloc(sizeof (*scsi_inq),
	    KM_NOSLEEP);
	if (scsi_inq == NULL)
		return (DDI_FAILURE);

	child_unit = (chs_unit_t *)kmem_zalloc(
	    sizeof (chs_unit_t) + sizeof (*ctlobjp), KM_NOSLEEP);
	if (child_unit == NULL) {
		kmem_free(scsi_inq, sizeof (*scsi_inq));
		return (DDI_FAILURE);
	}

	chs_dac_fake_inquiry(hba, tgt, scsi_inq);

	bcopy((caddr_t)hba_unit, (caddr_t)child_unit, sizeof (*hba_unit));
	child_unit->dma_lim = chs_dac_dma_lim;
	child_unit->dma_lim.dlim_sgllen = chs->sgllen;
	/*
	 * After this point always use (chs_unit_t *)->dma_lim and refrain
	 * from using chs_dac_dma_lim as the former can be customized per
	 * unit target driver instance but the latter is the generic hba
	 * instance dma limits.
	 */

	ASSERT(hba_tran != NULL);
	hba_tran->tran_tgt_private = child_unit;

	child_unit->dac_unit.sd_num = (u_char)tgt;

	sd->sd_inq = scsi_inq;

	ctlobjp   = (struct ctl_obj *)(child_unit + 1);
	sd->sd_address.a_hba_tran = (scsi_hba_tran_t *)ctlobjp;

	ctlobjp->c_ops  = (struct ctl_objops *)&chs_dac_objops;
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

	MDBG3(("chs_dac_tran_tgt_init: "
		"S%xC%xd%x dip=0x%x sd=0x%x unit=0x%x",
		hba->chs->reg/0x1000 /* Slot */, hba->chn, tgt,
		tgt_dip, sd, child_unit));

	return (DDI_SUCCESS);
}

void
chs_dac_fake_inquiry(
    register chs_hba_t *const hba,
    int tgt,
    struct scsi_inquiry *const scsi_inq)
{
	char name[32];
	register chs_t *chs;
	chs_ld_t	logdrv_info;

	ASSERT(hba != NULL);
	ASSERT(CHS_DAC(hba));
	chs = hba->chs;
	ASSERT(chs != NULL);
	ASSERT(scsi_inq != NULL);

	mutex_enter(&chs->mutex);

	if (CHS_GET_LOGDRV_INFO(chs, tgt, &logdrv_info) == FALSE) {
		/* illegal tgt number or tgt not configured */
		scsi_inq->inq_dtype = DTYPE_NOTPRESENT;
		mutex_exit(&chs->mutex);
		return;
	}

	if (logdrv_info.state == CHS_DAC_ONLINE ||
			logdrv_info.state == CHS_DAC_CRITICAL)
		scsi_inq->inq_dtype = DTYPE_DIRECT;
	else {
		scsi_inq->inq_dtype = DTYPE_NOTPRESENT;
		mutex_exit(&chs->mutex);
		return;
	}

	scsi_inq->inq_qual = DPQ_POSSIBLE;
	strncpy(scsi_inq->inq_vid, logdrv_info.vendorname, 8);
	bzero((caddr_t)name, sizeof (name));
	sprintf(name, "Raid-%d", logdrv_info.raid);
	mutex_exit(&chs->mutex);
	strncpy(scsi_inq->inq_pid, name, 16);
	strncpy(scsi_inq->inq_revision, "1234", 4);
}

struct cmpkt *
chs_dac_iosetup(
    register chs_unit_t *const unit,
    register struct cmpkt *const cmpkt)
{
	int 			stat, num_segs = 0;
	off_t 			off, len, tmpoff;
#ifdef	DADKIO_RWCMD_READ
	off_t			coff, clen, chscmd, lcmd;
#endif
	ulong 			bytes_xfer = 0;
	register chs_dac_cmd_t	*cmd = (chs_dac_cmd_t *)cmpkt;
	ddi_dma_cookie_t 	dmac;
	register chs_ccb_t	*ccb = cmd->ccb;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
	ASSERT(cmpkt != NULL);		/* and so is cmd != NULL */

	bzero((caddr_t)&(ccb->cmd), sizeof (ccb->cmd));
#ifdef	DADKIO_RWCMD_READ
	if (cmpkt->cp_passthru) {
		switch (RWCMDP->cmd) {
		case DADKIO_RWCMD_READ:
		case DADKIO_RWCMD_WRITE:
			chscmd =
			(RWCMDP->cmd == DADKIO_RWCMD_READ) ? CHS_DAC_SREAD :
					CHS_DAC_SWRITE;
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
			cmn_err(CE_WARN, "chs_dac_iosetup: unrecognised"
			    "command %x\n", RWCMDP->cmd);
			return (cmpkt);
		}
	} else {
		switch (lcmd = cmd->cdb) {
		case DCMD_READ:
			chscmd = CHS_DAC_SREAD;
			goto offsets;
		case DCMD_WRITE:
			chscmd = CHS_DAC_SWRITE;
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
		} while ((num_segs < CHS_MAX_NSG) &&
		    (bytes_xfer < CHS_DAC_MAX_XFER) &&
#ifdef	DADKIO_RWCMD_READ
		    (bytes_xfer < clen));
#else
		    (bytes_xfer < cmpkt->cp_bytexfer));
#endif

		/*
		 * In case bytes_xfer exceeds CHS_DAC_MAX_XFER in
		 * the last iteration.
		 */
		bytes_xfer = CHS_MIN(bytes_xfer, CHS_DAC_MAX_XFER);
		cmpkt->cp_resid = cmpkt->cp_bytexfer = bytes_xfer;
		ccb->ccb_opcode =
#ifdef	DADKIO_RWCMD_READ
			(u_char)chscmd;
#else
			(cmd->cdb == DCMD_READ) ?
			    CHS_DAC_SREAD : CHS_DAC_SWRITE;
#endif



#ifdef	DADKIO_RWCMD_READ
		tmpoff =	coff;
#else
		tmpoff = 	cmpkt->cp_srtsec,
#endif

		CHS_IOSETUP(unit->hba->chs, ccb,  num_segs,  bytes_xfer,
			tmpoff, unit->dac_unit.sd_num, 0);

		break;
	case DCMD_SEEK:
	case DCMD_RECAL:
	    break;
	default:
		cmn_err(CE_WARN, "chs_dac_iosetup: "
				"unrecognized command %x\n",
			cmd->cdb);
		break;
	}
	return (cmpkt);
}


struct cmpkt *
chs_dac_memsetup(
    register chs_unit_t *const unit,
    register struct cmpkt *const cmpkt,
    register buf_t *const bp,
    int (*const callback)(),
    const caddr_t arg)
{
	register int stat;
	chs_dac_cmd_t *cmd = (chs_dac_cmd_t *)cmpkt;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
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
chs_dac_memfree(
    const chs_unit_t *const unit,
    register struct cmpkt *const cmpkt)
{
	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
	ASSERT(cmpkt != NULL);

	ddi_dma_free(((chs_dac_cmd_t *)cmpkt)->handle);
}

/* Abort specific command on target device */
/*ARGSUSED*/
int
chs_dac_abort(const chs_unit_t *const unit,
		const struct cmpkt *const cmpkt)
{
	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
	/* IBM PCI RAID does not support recall of command in process */
	return (0);
}

/*
 * Reset all the System Drives or a single one.
 * Returns 1 on success and 0 on failure.
 */
/*ARGSUSED*/
int
chs_dac_reset(register chs_unit_t *const unit, int level)
{
	/* See:  Comment on chs_reset(). */
	return (0);
}

struct cmpkt *
chs_dac_pktalloc(
    register chs_unit_t *const unit,
    int (*const callback)(),
    const caddr_t arg)
{
	register chs_dac_cmd_t *cmd;
	register chs_ccb_t *ccb;
	chs_hba_t *hba;
	int kf = GDA_KMFLAG(callback);

	hba = unit->hba;
	ASSERT(hba != NULL);
	ASSERT(CHS_DAC(hba));

	cmd = (chs_dac_cmd_t *)kmem_zalloc(sizeof (*cmd) + sizeof (*ccb),
									kf);

	if (cmd == NULL) {
		if (callback != DDI_DMA_DONTWAIT)
			ddi_set_callback(callback, arg, &hba->callback_id);
		return (NULL);
	}
	ccb = (chs_ccb_t *)(cmd + 1);
	ccb->paddr = CHS_KVTOP(ccb);
	ccb->type = CHS_DAC_CTYPE1;

	cmd->ccb = ccb;
	cmd->cmpkt.cp_cdblen = 1;
	cmd->cmpkt.cp_cdbp = (opaque_t)&cmd->cdb;
	cmd->cmpkt.cp_scblen = 1;
	cmd->cmpkt.cp_scbp = (opaque_t)&cmd->scb;
	cmd->cmpkt.cp_ctl_private = (opaque_t)unit;

	return ((struct cmpkt *)cmd);
}

void
chs_dac_pktfree(register chs_unit_t *const unit,
		register struct cmpkt *cmpkt)
{
	register chs_hba_t *hba;

	ASSERT(unit != NULL);
	hba = unit->hba;
	ASSERT(hba != NULL);
	ASSERT(CHS_DAC(hba));

	kmem_free((caddr_t)cmpkt,
			sizeof (chs_dac_cmd_t) + sizeof (chs_ccb_t));

	if (hba->callback_id)
		ddi_run_callback(&hba->callback_id);
}

int
chs_dac_transport(
    register chs_unit_t *const unit,
    register struct cmpkt *const cmpkt)
{
	register chs_ccb_t *ccb;
	register chs_t *chs;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
	ccb = ((chs_dac_cmd_t *)cmpkt)->ccb;
	ASSERT(ccb != NULL);
	chs = unit->hba->chs;
	ASSERT(chs != NULL);

	ccb->ccb_ownerp = (struct scsi_cmd *)cmpkt;
	mutex_enter(&chs->mutex);
	if (chs_sendcmd(chs, ccb, 0) == DDI_FAILURE) {
		mutex_exit(&chs->mutex);
		return (CTL_SEND_FAILURE);
	}
	if (cmpkt->cp_flags & CPF_NOINTR) {
		CHS_DISABLE_INTR(chs);
		(void) chs_pollstat(chs, ccb, 0);
		/*
		 * Ignored DDI_FAILURE return possibility of
		 * chs_pollstat() as cp_reason is set there already.
		 */
		CHS_ENABLE_INTR(chs);
	}
	mutex_exit(&chs->mutex);
	return (CTL_SEND_SUCCESS);
}

/*
 * At the end of successful completion of any command which changes the
 * configuration one way or the other, chs->conf and chs->enq will be
 * updated as these need to be reliably up-to-date for the other parts
 * of the driver.  The whole point is that the system need not be
 * rebooted after any command which changes the configuration.
 *
 * Type 3 (Direct CDB) commands should not end up here.
 */
int
chs_dacioc(
    register chs_unit_t *const unit,
    int cmd,
    int arg,
    int mode)
{
	int rval;
	register chs_ccb_t *ccb;
	register chs_dacioc_t *dacioc;
	register struct cmpkt *cmpkt;

	switch (cmd) {
	case DIOCTL_GETGEOM:
	case DIOCTL_GETPHYGEOM:
		/*
		 * These are not sent to the controller, hence
		 * require no packet setup.  And handled entirely
		 * in this driver.
		 */
		return (chs_dacioc_nopkt(unit, cmd, arg, mode));
	}

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));

	MDBG3(("chs_dacioc: unit=%x, cmd=%x, arg=%x, mode=%x",
		unit, cmd, arg, mode));

	if (arg == NULL)
		return (EINVAL);

	cmpkt = chs_dacioc_pktalloc(unit, cmd, arg, mode, &rval);
	if (cmpkt == NULL) {
		ASSERT(rval);
		return (rval);
	}
	ccb = ((chs_dac_cmd_t *)cmpkt)->ccb;
	ASSERT(ccb != NULL);
	dacioc = (chs_dacioc_t *)cmpkt->cp_private;
	ASSERT(dacioc != NULL);

	if (chs_dac_transport(unit, cmpkt) == CTL_SEND_SUCCESS) {
		sema_p(&ccb->ccb_da_sema);
		/*
		 * XXX - If chs->conf and chs->enq needed to be updated,
		 * there is a window between NOW that EEPROM is updated
		 * and when chs->conf and chs-enq get updated at the end
		 * of chs_dacioc_done().
		 *
		 * Perhaps we should hold chs->mutex here and don't hold it
		 * in chs_dacioc_update_conf_enq().  But that is a serious
		 * performance hit.
		 */
		rval = chs_dacioc_done(cmpkt, ccb, dacioc, mode);
	} else {
		rval = EIO;
		MDBG3(("chs_dacioc: transport failure"));
	}

	chs_dacioc_pktfree(unit, cmpkt, ccb, dacioc, mode);
	return (rval);
}

/*
 * It handles the ioctl's which are not sent to the adapter and require
 * no pkt setup.
 */
int
chs_dacioc_nopkt(
    register chs_unit_t *const unit,
    int cmd,
    int arg,
    int mode)
{
	register chs_t  *chs;
	ushort		heads   = 64;
	ushort		sectors = 32;
	int		geom;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
	chs = unit->hba->chs;
	ASSERT(chs != NULL);

	MDBG3(("chs_dacioc_nopkt: unit=%x, cmd=%x, arg=%x, mode=%x",
		unit, cmd, arg, mode));

	switch (cmd) {
	case DIOCTL_GETGEOM:
	case DIOCTL_GETPHYGEOM: {
		register struct tgdk_geom *tg = (struct tgdk_geom *)arg;
		chs_ld_t	ld_info;

		mutex_enter(&chs->mutex);
		ASSERT(chs->enq != NULL);
		if (CHS_GET_LOGDRV_INFO(chs, unit->dac_unit.sd_num,
							&ld_info) == FALSE) {
			mutex_exit(&chs->mutex);
			return (EINVAL);
		}
		tg->g_cap = ld_info.size;

		mutex_exit(&chs->mutex);

		geom = CHS_GEOMETRY(chs, NULL, tg->g_cap);
		heads = geom >> 16;
		sectors = geom & 0xffff;

		tg->g_acyl = 0;
		tg->g_secsiz = CHS_BLK_SIZE;
		/* Fill in some reasonable values for the rest */
		tg->g_cyl  = tg->g_cap / (sectors * heads);
		tg->g_head = heads;
		tg->g_sec  = sectors;

		return (DDI_SUCCESS);
	}
	default:
		if (mode == FKIOCTL) {
			/* not supported */
			cmn_err(CE_CONT, "chs_dacioc_nopkt, "
					"unknown IOCTL from kernel %x", cmd);
		}
		return (DDI_FAILURE);
	}
}

/*
 * Prepares a packet with all the necessary allocations and setups
 * to be transported to the adapter for the particular ioctl.
 */
struct cmpkt *
chs_dacioc_pktalloc(
    register chs_unit_t *const unit,
    int cmd,
    int arg,
    int mode,
    int *const err)
{
	int rval;
	register chs_ccb_t *ccb;
	register chs_dacioc_t *dacioc;
	register struct cmpkt *cmpkt;
	register chs_t *chs;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
	chs = unit->hba->chs;
	ASSERT(chs != NULL);
	ASSERT(arg != NULL);

	cmpkt = chs_dac_pktalloc(unit, DDI_DMA_DONTWAIT, NULL);
	if (cmpkt == NULL) {
		MDBG3(("chs_dacioc_pktalloc: failed to allocate cmpkt"));
		*err = ENOMEM;
		return (NULL);
	}
	ccb = ((chs_dac_cmd_t *)cmpkt)->ccb;
	/* in case ccb was recycled */
	ccb->ccb_flags = 0;

	switch (cmd) {
	case CHS_DACIOC_FLUSH:
		ccb->type = CHS_DAC_CTYPE0;
		ccb->ccb_opcode = CHS_DAC_FLUSH;
		ccb->ccb_flags = CHS_CCB_DACIOC_NO_DATA_XFER;
		break;
	default:
		MDBG3(("chs_dacioc_pktalloc: bad ioctl cmd %x", cmd));
		chs_dac_pktfree(unit, cmpkt);
		*err = ENOTTY;
		return (NULL);
	}
	ccb->ccb_ubuf_len = chs_dacioc_ubuf_len(chs, ccb->ccb_opcode);

	/* indicators, will use later */
	cmpkt->cp_private = NULL;
	cmpkt->cp_bp = NULL;

	rval = chs_dacioc_getarg(chs, unit, arg, cmpkt, ccb, mode);
	dacioc = (chs_dacioc_t *)cmpkt->cp_private;
	if (rval != DDI_SUCCESS) {
		chs_dacioc_pktfree(unit, cmpkt, ccb, dacioc, mode);
		*err = rval;
		return (NULL);
	}

	sema_init(&ccb->ccb_da_sema, 0, "IBM PCI RAID Ioctl Per Pkt Semaphore",
	    SEMA_DRIVER, NULL);
	ccb->ccb_flags |= CHS_CCB_GOT_DA_SEMA;
	cmpkt->cp_callback = chs_dacioc_callback;
	*err = 0;
	return (cmpkt);
}

/* Based on the ccb_opcode returns the expected ubuf_len */
ushort
chs_dacioc_ubuf_len(register chs_t *const chs, const u_char opcode)
{
	switch (opcode) {
	case CHS_DAC_FLUSH:
		return (0);
	default:
		cmn_err(CE_CONT, "?chs_dacioc_ubuf_len: "
				" does not supporte opcode %x instance %x\n",
				opcode,(unsigned long) chs);
		return (0);
	}
}



/*
 * This routine will be called at interrupt time for all the
 * packets created and successfully transferred by chs_dacioc().
 */
void
chs_dacioc_callback(register struct cmpkt *const cmpkt)
{
	ASSERT(cmpkt != NULL);

	sema_v(&((chs_dac_cmd_t *)cmpkt)->ccb->ccb_da_sema);
}

/*
 * Post processing the ioctl packet received from the adapter and
 * delivered by the interrupt routine.
 */
int
chs_dacioc_done(
    register struct cmpkt *const cmpkt,
    register chs_ccb_t *const ccb,
    register chs_dacioc_t *const dacioc,
    int mode)
{
	ushort ubuf_len;
	int scb;
	caddr_t kv_ubuf;

	ASSERT(cmpkt != NULL);
	ASSERT(ccb != NULL);
	ASSERT(ccb == ((chs_dac_cmd_t *)cmpkt)->ccb);
	ASSERT(dacioc != NULL);
	ASSERT(dacioc == cmpkt->cp_private);

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
		dacioc->status = (ccb->ccb_status);
		if (ubuf_len) {
			ASSERT(ccb->type != CHS_DAC_CTYPE0);

			if ((dacioc->flags & CHS_DACIOC_DAC_TO_UBUF) &&
			    ddi_copyout(kv_ubuf, dacioc->ubuf,
					ubuf_len, mode)) {
				MDBG3(("chs_dacioc_done: failed to "
				    "copyout  to ubuf, ubuf_len=%x",
				    ubuf_len));
				return (EFAULT);
			}
		}

		if (mode != FKIOCTL) {
			/* not supported */
			return (EINVAL);
		}

		return (DDI_SUCCESS);
	}
	case CPS_CHKERR:
		return ((scb == DERR_BUSY) ? EAGAIN : EIO);
	case CPS_ABORTED:
	case CPS_FAILURE:
	default:
		cmn_err(CE_WARN, "chs_dacioc_callback: bad reason code %x",
			(int)cmpkt->cp_reason);
		return (EIO);
	}
}


/* Free the ioctl packet */
void
chs_dacioc_pktfree(
    chs_unit_t *const unit,
    register struct cmpkt *cmpkt,
    register chs_ccb_t *const ccb,
    register chs_dacioc_t *dacioc,
    int mode)
{
	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
	ASSERT(cmpkt != NULL);
	ASSERT(ccb != NULL);
	ASSERT(ccb == ((chs_dac_cmd_t *)cmpkt)->ccb);
	ASSERT(dacioc != NULL);
	ASSERT(dacioc == cmpkt->cp_private);

	if (cmpkt->cp_bp != NULL)
		ddi_iopb_free((caddr_t)cmpkt->cp_bp);

	if (mode != FKIOCTL && dacioc != NULL)
		kmem_free((caddr_t)dacioc, sizeof (*dacioc));

	if (ccb->ccb_flags & CHS_CCB_GOT_DA_SEMA)
		sema_destroy(&ccb->ccb_da_sema);

	chs_dac_pktfree(unit, cmpkt);
}
int
chs_dacioc_getarg(
    chs_t *const chs,
    register chs_unit_t *const unit,
    int arg,
    register struct cmpkt *const cmpkt,
    register chs_ccb_t *const ccb,
    int mode)
{
	ushort ubuf_len;
	register chs_dacioc_t *dacioc;

	ASSERT(unit != NULL);
	ASSERT(unit->hba != NULL);
	ASSERT(CHS_DAC(unit->hba));
	ASSERT(arg != NULL);
	ASSERT(cmpkt != NULL);
	ASSERT(ccb != NULL);

	if (mode == FKIOCTL) {		/* called from kernel */
		dacioc = (chs_dacioc_t *)arg;
		cmpkt->cp_private = (opaque_t)dacioc;
	} else {
		dacioc = (chs_dacioc_t *)kmem_zalloc(sizeof (*dacioc),
		    KM_NOSLEEP);
		if (dacioc == NULL) {
			MDBG3(("chs_dacioc_getarg: "
				"failed to allocate dacioc"));
			return (ENOMEM);
		}
		cmpkt->cp_private = (opaque_t)dacioc;

		if (copyin((caddr_t)arg, (caddr_t)dacioc, sizeof (*dacioc))) {
			MDBG3(("chs_dacioc_getarg: failed to copyin arg"));
			return (EFAULT);
		}
	}

	ubuf_len = dacioc->ubuf_len;
	if (!chs_dacioc_valid_args(chs, ccb, ubuf_len, dacioc)) {
		MDBG3(("chs_dacioc_getarg: invalid arg, "
		    "type=%x, ubuf_len=%x, flags=%x, dacioc=%x",
		    ccb->type, ubuf_len, dacioc->flags, dacioc));
		return (EINVAL);
	}

	switch (ccb->type) {
	case CHS_DAC_CTYPE1:
		ccb->ccb_drv = dacioc->dacioc_drv;
		CCB_CBLK(ccb, dacioc->dacioc_blk);
		ccb->ccb_cnt = dacioc->dacioc_cnt;
		break;
	case CHS_DAC_CTYPE2:
		ccb->ccb_chn = dacioc->dacioc_chn;
		ccb->ccb_tgt = dacioc->dacioc_tgt;
		ccb->ccb_dev_state = dacioc->dacioc_dev_state;
		break;
	case CHS_DAC_CTYPE4:
		ccb->ccb_test = dacioc->dacioc_test;
		ccb->ccb_pass = dacioc->dacioc_pass;
		ccb->ccb_chan = dacioc->dacioc_chan;
		break;
	case CHS_DAC_CTYPE5:
		ccb->ccb_param = dacioc->dacioc_param;
		break;
	case CHS_DAC_CTYPE6:
		ccb->ccb_count = dacioc->dacioc_count;
		ccb->ccb_offset = dacioc->dacioc_offset;
		break;
	case CHS_DACIOC_CTYPE_GEN: {
		ulong len = ccb->ccb_gen_args_len = dacioc->dacioc_gen_args_len;
		register chs_dacioc_generic_args_t *gen_args;

		ccb->ccb_xferaddr_reg = dacioc->dacioc_xferaddr_reg;
		ASSERT(len);
		ASSERT(!(len % sizeof (*gen_args)));
		gen_args = ccb->ccb_gen_args =
		    (chs_dacioc_generic_args_t *)
				kmem_zalloc(len, KM_NOSLEEP);
		if (gen_args == NULL) {
			MDBG3(("chs_dacioc_getarg: not enough memory for "
				"generic ioctl args"));
			return (ENOMEM);
		}

		if (ddi_copyin((caddr_t)dacioc->dacioc_gen_args,
		    (caddr_t)gen_args, len, mode)) {
			MDBG3(("chs_dacioc_getarg: failed to copyin "
			    "gen_args"));
			return (EFAULT);
		}

		for (; len != 0; gen_args++, len -= sizeof (*gen_args)) {
			ushort reg_addr;

			ASSERT(gen_args != NULL);
			reg_addr = gen_args->reg_addr;

			if ((reg_addr & 0xff00) !=	/* slot# */
			    (chs->reg & 0xff00)) {
				cmn_err(CE_WARN, "chs_dacioc_getarg: "
				    "invalid reg_addr=%x destined for slot %d",
				    reg_addr, reg_addr & 0xf000);
				return (EINVAL);
			} else if (reg_addr & 0xF == CHS_MBXCMDID) {
				cmn_err(CE_WARN, "chs_dacioc_getarg: "
				    "assignment to cmdid reg %x",
				    chs->reg + CHS_MBXCMDID);
				return (EINVAL);
			} else if (reg_addr & 0xF == CHS_MBXCMD) {
				u_char opcode;
				ushort len;

				ccb->ccb_opcode = opcode = gen_args->val;
				ccb->ccb_ubuf_len = len =
				    chs_dacioc_ubuf_len(chs,
							ccb->ccb_opcode);
				if (opcode != CHS_DAC_LOADIMG &&
				    opcode != CHS_DAC_STOREIMG &&
				    ubuf_len != len) {
					cmn_err(CE_WARN,
					    "chs_dacioc_getarg: "
					    "unexpected ubuf_len=%x, "
					    "opcode=%x", ubuf_len,
					    opcode);
					/* can't tell if it's invalid or not! */
				}
			} else
				ccb->ccb_arr[(reg_addr & 0xF) - CHS_MBX2] =
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
		    (CHS_DACIOC_UBUF_TO_DAC | CHS_DACIOC_DAC_TO_UBUF));

		if (ddi_iopb_alloc(unit->hba->dip, &unit->dma_lim,
		    (u_int)ubuf_len, &kv_ubuf)) {
			MDBG3(("chs_dacioc_getarg: failed to allocate dma "
				"buffer, ubuf_len=%x", ubuf_len));
			return (ENOMEM);
		}
		cmpkt->cp_bp = (buf_t *)kv_ubuf;
		ccb->ccb_xferpaddr = CHS_KVTOP(kv_ubuf);

		/*
		 * Both CHS_DACIOC_DAC_TO_UBUF and
		 * CHS_DACIOC_UBUF_TO_DAC could be set in dacioc->flags.
		 */
		if (dacioc->flags & CHS_DACIOC_UBUF_TO_DAC) {
			if (ddi_copyin(dacioc->ubuf, kv_ubuf,
			    ubuf_len, mode)) {
				MDBG3(("chs_dacioc_getarg: failed to "
					"copyin from user buffer, ubuf_len=%x",
					ubuf_len));
				return (EFAULT);
			}
		} else {
			ASSERT(dacioc->flags & CHS_DACIOC_DAC_TO_UBUF);
			bzero(kv_ubuf, (size_t)ubuf_len);
		}

		if (ccb->ccb_opcode == CHS_DAC_SIZE &&
		    (ccb->ccb_xferpaddr & 3)) {
			MDBG3(("chs_dacioc_getarg: ccb_opcode=%x, "
			    "ccb_xferpaddr=%x not on 4 byte boundary",
			    ccb->ccb_opcode, ccb->ccb_xferpaddr));
			return (EINVAL);
		}
	}
	return (DDI_SUCCESS);
}

/* Returns 1 if the args are valid, otherwise 0 */
int
chs_dacioc_valid_args(
    chs_t *const chs,
    register chs_ccb_t *const ccb,
    const ushort ubuf_len,
    register chs_dacioc_t *const dacioc)
{
	ASSERT(chs != NULL);
	ASSERT(ccb != NULL);
	ASSERT(dacioc != NULL);

	if (ubuf_len) {
		if (ubuf_len > CHS_DAC_MAX_XFER) {
			MDBG3(("chs_dacioc_valid_args: ubuf_len > max (%x)",
			    CHS_DAC_MAX_XFER));
			return (0);
		}
		if (!(dacioc->flags &
		    (CHS_DACIOC_UBUF_TO_DAC |
					CHS_DACIOC_DAC_TO_UBUF))) {
			MDBG3(("chs_dacioc_valid_args: data xfer direction "
			    "flag is incorrect"));
			return (0);
		}
	}

	if (ccb->type == CHS_DACIOC_CTYPE_GEN) {
		ulong len = dacioc->dacioc_gen_args_len;

		if (!len || len % sizeof (chs_dacioc_generic_args_t)) {
			MDBG3(("chs_dacioc_valid_args: invalid gen_args_len"
			    "(%x) in generic ioctl", len));
			return (0);
		}
		return (1);
	}

	if (!ubuf_len && !(ccb->ccb_flags & CHS_CCB_DACIOC_NO_DATA_XFER)) {
		MDBG3(("chs_dacioc_valid_args: invalid ubuf_len, expected "
		    "non-zero"));
		return (0);
	}
	if (ubuf_len) {
		ushort len;

		if (ccb->ccb_flags & CHS_CCB_DACIOC_NO_DATA_XFER) {
			MDBG3(("chs_dacioc_valid_args: invalid ubuf_len, "
			    "expected 0"));
			return (0);
		}
		if ((dacioc->flags & CHS_DACIOC_UBUF_TO_DAC) &&
		    !(ccb->ccb_flags & CHS_CCB_DACIOC_UBUF_TO_DAC)) {
			MDBG3(("chs_dacioc_valid_args: invalid data xfer "
			    "direction flag"));
			return (0);
		}
		if ((dacioc->flags & CHS_DACIOC_DAC_TO_UBUF) &&
		    !(ccb->ccb_flags & CHS_CCB_DACIOC_DAC_TO_UBUF)) {
			MDBG3(("chs_dacioc_valid_args: bad data xfer "
			    "direction flag"));
			return (0);
		}

		if (ccb->ccb_opcode == CHS_DAC_LOADIMG ||
		    ccb->ccb_opcode == CHS_DAC_STOREIMG)
			len = dacioc->dacioc_count;
		else
			len = ccb->ccb_ubuf_len;
		if (ubuf_len != len) {
			MDBG3(("chs_dacioc_valid_args: bad ubuf_len, "
			    "expected %x", len));
			return (0);
		}
	}

	/*
	 * There are special ioctl's which have to be performed on System
	 * Drives with data redundancy, i.e. RAID levels 1, 3, 4, 5 and 6.
	 * The adapter performs this check for some (e.g.
	 * CHS_DACIOC_CHKCONS) and ignores this check for the others for
	 * performance reasons.
	 * Therefore, here we have to perform the check for the latter ones.
	 * XXX - This is a bug in the f/w that we have to compensate for.
	 */
	switch (ccb->ccb_opcode) {
	case CHS_DAC_RBLD:
	case CHS_DAC_RBLDA: {
		unchar raidlevel;

		mutex_enter(&chs->mutex);
		if (CHS_IN_ANY_SD(chs, dacioc->dacioc_chn,
				dacioc->dacioc_tgt, &raidlevel) == FALSE) {
			MDBG3(("chs_dacioc_valid_args: not in any sd"));
			mutex_exit(&chs->mutex);
			return (0);
		}
		switch (raidlevel) {
		case 1:
		case 3:
		case 4:
		case 5:
		case 6:
			break;
		default:
			MDBG3(("chs_dacioc_valid_args: RAID-%d doesn't "
			    "contain data redundancy", raidlevel));
			mutex_exit(&chs->mutex);
			return (0);
		}
		mutex_exit(&chs->mutex);
		break;
	}
	default:
		break;
	}

	return (1);
}
