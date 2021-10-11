/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)dptghd_scsa.c	1.1	96/06/13 SMI"

#include "dptghd.h"

/*
 * Local Function Prototypes
 */

static	struct scsi_pkt	*dptghd_pktalloc(ccc_t *cccp, struct scsi_address *ap,
				int cmdlen, int statuslen, int tgtlen,
				int (*callback)(), caddr_t arg, int ccblen);

static	int		 dptghd_dmaget(ccc_t *cccp, gcmd_t *gcmdp,
				struct scsi_pkt *pktp, struct buf *bp,
				int flags, int (*callback)(), caddr_t arg,
				ddi_dma_lim_t *sg_limitp, void *sg_func_arg);



/*
 * free dma handle and controller handle allocated in memsetup
 */

/*ARGSUSED*/
void
dptghd_tran_dmafree( struct scsi_address *ap, struct scsi_pkt  *pktp )
{
	gcmd_t	*gcmdp = PKTP2GCMDP(pktp);

	if (gcmdp->cmd_dma_handle) {
		ddi_dma_free(gcmdp->cmd_dma_handle);
		gcmdp->cmd_dma_handle= NULL;
		gcmdp->cmd_dmawin = NULL;
		gcmdp->cmd_totxfer = 0;
	}
	return;
}

/*ARGSUSED*/
void
dptghd_tran_sync_pkt( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	gcmd_t *gcmdp = PKTP2GCMDP(pktp);
	int	status;

	if (gcmdp->cmd_dma_handle) {
		status = ddi_dma_sync(gcmdp->cmd_dma_handle, 0, 0,
			(gcmdp->cmd_dma_flags & DDI_DMA_READ) ?
			DDI_DMA_SYNC_FORCPU : DDI_DMA_SYNC_FORDEV);
		if (status != DDI_SUCCESS) {
			cmn_err(CE_WARN, "dptghd_tran_sync_pkt() fail\n");
		}
	}
	return;
}




static int
dptghd_dmaget(	ccc_t		*cccp,
		gcmd_t		*gcmdp,
		struct scsi_pkt	*pktp,
		struct buf	*bp,
		int		 flags,
		int		(*callback)(),
		caddr_t		 arg,

		ddi_dma_lim_t	*sg_limitp,
		void		*sg_func_arg )
{
	int	 sg_size = sg_limitp->dlim_sgllen;
	ulong	 bcount = bp->b_bcount;
	ulong	 xferred = gcmdp->cmd_totxfer;
	int	 status;
	off_t	 off;
	off_t	 len;
	int	 num_segs = 0;
	int	 dma_flags;
	ddi_dma_cookie_t cookie;

	if (!gcmdp->cmd_dma_handle)
		goto new_handle;

	if (gcmdp->cmd_dmawin == NULL)
		goto nextwin;

nextseg:
	do {
		status = ddi_dma_nextseg(gcmdp->cmd_dmawin, gcmdp->cmd_dmaseg,
					 &gcmdp->cmd_dmaseg);
		switch (status) {
		case DDI_SUCCESS:
			break;

		case DDI_DMA_DONE:
			if (num_segs == 0) {
				/* start the next window */
				goto nextwin;
			}
			gcmdp->cmd_totxfer = xferred;
			pktp->pkt_resid = bcount - gcmdp->cmd_totxfer;
			return (TRUE);

		default:
			return (FALSE);
		}

		ddi_dma_segtocookie(gcmdp->cmd_dmaseg, &off, &len, &cookie);

		/* call the controller specific S/G function */
		if (num_segs == 0 && len >= bcount) {
			/* transfer whole request in a single segment */
			(*cccp->ccc_sg_func)(pktp, gcmdp, &cookie, TRUE,
					num_segs, sg_func_arg);
		} else {
			/* do multi-segment Scatter/Gather transfer */
			(*cccp->ccc_sg_func)(pktp, gcmdp, &cookie, FALSE,
					num_segs, sg_func_arg);
		}

		xferred += cookie.dmac_size;
		num_segs++;

	} while (xferred < bcount && num_segs < sg_size);

	gcmdp->cmd_totxfer = xferred;
	pktp->pkt_resid = bcount - gcmdp->cmd_totxfer;
	return (TRUE);


	/*
	 * First time, need to establish the handle.
	 */

new_handle:
	gcmdp->cmd_dmawin = NULL;

	/* check direction for data transfer */
	if (bp->b_flags & B_READ)
		dma_flags = DDI_DMA_READ;
	else
		dma_flags = DDI_DMA_WRITE;

	if (flags & PKT_CONSISTENT)
		dma_flags |= DDI_DMA_CONSISTENT;
	if (flags & PKT_DMA_PARTIAL)
		dma_flags |= DDI_DMA_PARTIAL;


	status = ddi_dma_buf_setup(PKT2TRAN(pktp)->tran_hba_dip, bp, dma_flags,
					callback, arg, sg_limitp,
					&gcmdp->cmd_dma_handle);


	switch (status) {
	case DDI_DMA_MAPOK:
	case DDI_DMA_PARTIAL_MAP:
		/* enable first call to ddi_dma_nextwin */
		pktp->pkt_resid  = 0;
		gcmdp->cmd_dma_flags = dma_flags;
		break;

	case DDI_DMA_NORESOURCES:
		bp->b_error = 0;
		return (FALSE);

	case DDI_DMA_TOOBIG:
		bioerror(bp, EINVAL);
		return (FALSE);

	case DDI_DMA_NOMAPPING:
	default:
		bioerror(bp, EFAULT);
		return (FALSE);
	}


	/*
	 * get the next window
	 */

nextwin:
	gcmdp->cmd_dmaseg = NULL;

	status = ddi_dma_nextwin(gcmdp->cmd_dma_handle, gcmdp->cmd_dmawin,
				 &gcmdp->cmd_dmawin);

	switch (status) {
	case DDI_SUCCESS:
		break;

	case DDI_DMA_DONE:
		return (FALSE);

	default:
		return (FALSE);
	}
	goto nextseg;

}

struct scsi_pkt *
dptghd_tran_init_pkt(	ccc_t		*cccp,
			struct scsi_address	*ap,
			struct scsi_pkt	*pktp,
			struct buf	*bp,
			int		 cmdlen,
			int		 statuslen,
			int		 tgtlen,
			int		 flags,
			int		(*callback)(),
			caddr_t		 arg,
			int		 ccblen,
			ddi_dma_lim_t	*sg_limitp,
			void		*sg_func_arg )
{
	gcmd_t	*gcmdp;
	int	 new_pkt;

	ASSERT(callback == NULL_FUNC || callback == SLEEP_FUNC);

	/*
	 * Allocate a pkt
	 */
	if (pktp == NULL) {
		pktp = dptghd_pktalloc(cccp, ap, cmdlen, statuslen, tgtlen,
					callback, arg, ccblen);
		if (pktp == NULL)
			return (NULL);
		new_pkt = TRUE;

	} else {
		new_pkt = FALSE;

	}

	gcmdp = PKTP2GCMDP(pktp);

	/* 
	 * free stale DMA window if necessary.
	 */
	if (cmdlen && gcmdp->cmd_dma_handle) {
		/* release the old DMA resources */
		dptghd_tran_dmafree(ap, pktp);
	}

	/*
	 * Set up dma info
	 */
	if (bp && bp->b_bcount) {
		if (dptghd_dmaget(cccp, gcmdp, pktp, bp, flags, callback, arg,
				sg_limitp, sg_func_arg) == NULL) {
			if (new_pkt)
				dptghd_pktfree(cccp, ap, pktp);
			return (NULL);
		}
	} else {
		pktp->pkt_resid = 0;
	}

	return (pktp);
}

static struct scsi_pkt *
dptghd_pktalloc(	ccc_t	*cccp,
		struct scsi_address	*ap,
		int	 cmdlen,
		int	 statuslen,
		int	 tgtlen,
		int	(*callback)(),
		caddr_t	 arg,
		int  	 ccblen )
{
	opaque_t	ha_private;
	struct scsi_pkt	*pktp;
	gcmd_t		*gcmdp;

	/* allocate everything else from kmem pool */
	pktp = scsi_hba_pkt_alloc(cccp->ccc_hba_dip, ap, cmdlen, statuslen,
				tgtlen, ccblen, callback, arg);
	if (pktp == NULL) {
		return (NULL);
	}

	/* get the ptr to the HBA specific buffer */
	ha_private = pktp->pkt_ha_private;

	/*
	 * callback to the HBA driver so it can initalize its
	 * buffer and return the ptr to my cmd_t structure which is
	 * probably embedded in its buffer.
	 */

	if (!(gcmdp = (*cccp->ccc_ccballoc)(ap, pktp, ha_private, cmdlen,
					    statuslen, tgtlen, ccblen))) {
		scsi_hba_pkt_free(ap, pktp);
		return (NULL);
	}

	/*
	 * Initialize the cmd_t structure
	 */
	QEL_INIT(&gcmdp->cmd_q);
	L2_INIT(&gcmdp->cmd_timer_link);

	/*
	 * Save the gcmd_t ptr in the scsi_pkt and save the ptr from
	 * the scsi_pkt in gcmd_t
	 */
	gcmdp->cmd_pktp = pktp;
	gcmdp->cmd_private = ha_private;
	pktp->pkt_ha_private = gcmdp;

	return (pktp);
}



/*
 * packet free
 */
/*ARGSUSED*/
void
dptghd_pktfree(	ccc_t	*cccp,
		struct scsi_address	*ap,
		struct scsi_pkt		*pktp )
{
	(*cccp->ccc_ccbfree)(ap, pktp);
	scsi_hba_pkt_free(ap, pktp);
	return;
}
