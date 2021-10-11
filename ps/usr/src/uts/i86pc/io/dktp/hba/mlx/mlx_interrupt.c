/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)mlx_interrupt.c	1.2	95/10/30 SMI"

#include <sys/dktp/mlx/mlx.h>

#ifdef DADKIO_RWCMD_READ
#if defined(MLX_DEBUG)

/*
 * Debug Interrupt Routine: this debug routine removes the overhead
 * of setting up dma requests, minimizing the amount of time spent
 * between commands. For performance profiling.
 */
void
mlx_dintr(register mlx_ccb_t *ccb,
	register mlx_t *mlx,
	register struct cmpkt *cmpkt)
{
	long	seccnt;
	off_t	coff;
	int	i;
	int	num_segs = 0;

	/* subtract previous i/o operation */
	cmpkt->cp_byteleft -= cmpkt->cp_bytexfer;
	cmpkt->cp_retry = 0;
	seccnt = cmpkt->cp_bytexfer >> SCTRSHFT;
	cmpkt->cp_secleft -= seccnt;
	cmpkt->cp_srtsec += seccnt;

	/* calculate next i/o operation */
	if (cmpkt->cp_secleft > 0) {
		/* adjust scatter-gather length */
		if (cmpkt->cp_bytexfer > cmpkt->cp_byteleft) {
			cmpkt->cp_bytexfer = cmpkt->cp_byteleft;
			for (i = cmpkt->cp_byteleft; i > 0;
				i -= ccb->ccb_sg_list[num_segs++].data02_len32)
				;
			if (i < 0)
				ccb->ccb_sg_list[num_segs-1].data02_len32 += i;
			ccb->ccb_sg_type = (u_char)(num_segs | MLX_SGTYPE0);
		}
		seccnt = cmpkt->cp_bytexfer >> SCTRSHFT;
		coff = RWCMDP->blkaddr + cmpkt->cp_srtsec;
		CCB_BLK(ccb, coff);
		ccb->ccb_cnt = (u_char)seccnt;

		if (mlx_sendcmd(mlx, ccb, 0) == DDI_FAILURE) {
			/* complete packet */
			cmpkt->cp_byteleft = cmpkt->cp_bytexfer;
			mutex_exit(&mlx->mutex);
			(*cmpkt->cp_callback)(cmpkt);
			mutex_enter(&mlx->mutex);
		}
	} else {
		/* complete packet */
		cmpkt->cp_byteleft = cmpkt->cp_bytexfer;
		mutex_exit(&mlx->mutex);
		(*cmpkt->cp_callback)(cmpkt);
		mutex_enter(&mlx->mutex);
	}
}
#endif
#endif

/*
 * Autovector interrupt entry point.  Passed to ddi_add_intr() in
 * mlx_attach().
 */

/*
 * This interrupt handler is not only called at interrupt time by the
 * cpu interrupt thread, it is also called from mlx_{scsi,dac}_transport()
 * through mlx_pollstat().  In the latter case, although the interrupts are
 * disabled, some might have come in because operations initiated earlier
 * could have completed and raised the interrupt.
 */
u_int
mlx_intr(caddr_t arg)
{
	register mlx_ccb_t *ccb;
	register mlx_ccb_stk_t *ccb_stk;
	register mlx_t *mlx = (mlx_t *)arg;
	register struct scsi_pkt *pkt;
	register struct cmpkt *cmpkt;
	mlx_stat_t hw_stat;
	mlx_stat_t *hw_statp = &hw_stat;
	int ret = DDI_INTR_UNCLAIMED;

	ASSERT(mlx != NULL);
	mutex_enter(&mlx->mutex);

	for (; MLX_IREADY(mlx); ) {
		/*
		 * The following inb()/inw() seems wasteful if the above
		 * stat_id does not correspond to an outstanding cmdid
		 * (statistically minority of cases).  However, in
		 * the absolute majority of the cases it does match
		 * some outstanding cmdid and by reading the status
		 * earlier and setting the BMIC registers ASAP the
		 * chances of iterating this loop is increased.
		 */
		MLX_GET_ISTAT(mlx, hw_statp, 1);

		if (hw_statp->stat_id == MLX_INVALID_CMDID) {
			/*
			 * This is coming as a result of some target's
			 * _probe() where stat_id's are set to
			 * MLX_INVALID_CMDID
			 */
			mutex_exit(&mlx->mutex);
			return (ret);
		}
		if (hw_statp->stat_id >= mlx->max_cmd)
			cmn_err(CE_PANIC, "mlx_intr: stat_id=%d, max_cmd=%d",
				hw_statp->stat_id, mlx->max_cmd);

		ASSERT(mlx->ccb_stk != NULL);
		ccb_stk = mlx->ccb_stk + hw_statp->stat_id;
		ccb = ccb_stk->ccb;
		if (ccb == NULL) {
			mutex_exit(&mlx->mutex);
			MDBG1(("mlx_intr: bad stat_id %d, spurious intr",
				hw_statp->stat_id));
			return (ret);
		}
		ASSERT(hw_statp->stat_id == ccb->ccb_cmdid);
		ASSERT(hw_statp->stat_id == ccb->ccb_stat_id);
		ccb->cmd.hw_stat = *hw_statp;

		ccb_stk->next = mlx->free_ccb - mlx->ccb_stk;
		mlx->free_ccb = ccb_stk;
		ccb_stk->ccb = NULL;
		cv_signal(&mlx->ccb_stk_cv);

		if (ccb->type == MLX_SCSI_CTYPE) {
			pkt = (struct scsi_pkt *)ccb->ccb_ownerp;
			ASSERT(pkt != NULL);
			sema_v(&mlx->scsi_ncdb_sema);
			mlx_chkerr(mlx, ccb, pkt, hw_statp->status);

			ASSERT(mutex_owned(&mlx->mutex));

			if (!(pkt->pkt_flags & FLAG_NOINTR) &&
			    pkt->pkt_comp != NULL) {
				/*
				 * ddi_dma_sync() for CPU,
				 * not required for EISA cards.
				 */
				mutex_exit(&mlx->mutex);
				scsi_run_cbthread(mlx->scsi_cbthdl,
				    (struct scsi_cmd *)pkt);
				mutex_enter(&mlx->mutex);
			}
		} else {
			cmpkt = (struct cmpkt *)ccb->ccb_ownerp;
			ASSERT(cmpkt != NULL);

			if (hw_statp->status) {
				cmn_err(CE_CONT, "?mlx_intr: DAC960 opcode="
				    "0x%x, error status=0x%x",
				    ccb->ccb_opcode, hw_statp->status);
				cmpkt->cp_reason = CPS_CHKERR;
				((mlx_dac_cmd_t *)cmpkt)->scb = DERR_ABORT;
			} else
				cmpkt->cp_reason = CPS_SUCCESS;

#ifdef	DADKIO_RWCMD_READ
#if defined(MLX_DEBUG)
			if (cmpkt->cp_passthru && cmpkt->cp_bp &&
			    cmpkt->cp_bp->b_back && (RWCMDP->flags & 0x8000))
				mlx_dintr(ccb, mlx, cmpkt);
			else
#endif
#endif
			{
				mutex_exit(&mlx->mutex);
				(*cmpkt->cp_callback)(cmpkt);
				mutex_enter(&mlx->mutex);
			}
		}
		ret = DDI_INTR_CLAIMED;
	}
	mutex_exit(&mlx->mutex);
	return (ret);
}
