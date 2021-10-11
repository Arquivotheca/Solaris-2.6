/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)ghd_scsa.c	1.1	96/07/30 SMI"

#include "ghd.h"

/*
 * Local Function Prototypes
 */

static	struct scsi_pkt	*ghd_pktalloc(ccc_t *cccp, struct scsi_address *ap,
				int cmdlen, int statuslen, int tgtlen,
				int (*callback)(), caddr_t arg, int ccblen);



/*ARGSUSED*/
void
ghd_tran_sync_pkt( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	gcmd_t *gcmdp = PKTP2GCMDP(pktp);
	int	status;

	if (gcmdp->cmd_dma_handle) {
		status = ddi_dma_sync(gcmdp->cmd_dma_handle, 0, 0,
			(gcmdp->cmd_dma_flags & DDI_DMA_READ) ?
			DDI_DMA_SYNC_FORCPU : DDI_DMA_SYNC_FORDEV);
		if (status != DDI_SUCCESS) {
			cmn_err(CE_WARN, "ghd_tran_sync_pkt() fail\n");
		}
	}
	return;
}




struct scsi_pkt *
ghd_tran_init_pkt(	ccc_t	*cccp,
			struct scsi_address *ap,
			struct scsi_pkt	*pktp,
			struct buf	*bp,
			int	 cmdlen,
			int	 statuslen,
			int	 tgtlen,
			int	 flags,
			int	(*callback)(),
			caddr_t	 arg,
			int	 ccblen,
			ddi_dma_lim_t	*sg_limitp )
{
	gcmd_t	*gcmdp;
	int	 new_pkt;

	ASSERT(callback == NULL_FUNC || callback == SLEEP_FUNC);

	/*
	 * Allocate a pkt
	 */
	if (pktp == NULL) {
		pktp = ghd_pktalloc(cccp, ap, cmdlen, statuslen, tgtlen,
					callback, arg, ccblen);
		if (pktp == NULL)
			return (NULL);
		new_pkt = TRUE;

	} else {
		new_pkt = FALSE;

	}

	gcmdp = PKTP2GCMDP(pktp);

	GDBG_PKT(("ghd_tran_init_pkt: gcmdp 0x%x dma_handle 0x%x\n",
		  gcmdp, gcmdp->cmd_dma_handle));

	/* 
	 * free stale DMA window if necessary.
	 */

	if (cmdlen && gcmdp->cmd_dma_handle) {
		/* release the old DMA resources */
		ghd_dmafree(gcmdp);
	}

	/*
	 * Set up dma info if there's any data and
	 * if the device supports DMA.
	 */

	GDBG_PKT(("ghd_tran_init_pkt: gcmdp 0x%x bp 0x%x limp 0x%x\n",
		  gcmdp, bp, sg_limitp));

	if (bp && bp->b_bcount && sg_limitp) {
		int	dma_flags;

		/* check direction for data transfer */
		if (bp->b_flags & B_READ)
			dma_flags = DDI_DMA_READ;
		else
			dma_flags = DDI_DMA_WRITE;

		/* check dma option flags */
		if (flags & PKT_CONSISTENT)
			dma_flags |= DDI_DMA_CONSISTENT;
		if (flags & PKT_DMA_PARTIAL)
			dma_flags |= DDI_DMA_PARTIAL;

		/* map the buffer and/or create the scatter/gather list */
		if (ghd_dmaget(cccp->ccc_hba_dip, gcmdp, bp, dma_flags,
			       callback, arg, sg_limitp,
			       cccp->ccc_sg_func) == NULL) {
			if (new_pkt)
				ghd_pktfree(cccp, ap, pktp);
			return (NULL);
		}
		pktp->pkt_resid = gcmdp->cmd_resid;
	} else {
		pktp->pkt_resid = 0;
	}

	return (pktp);
}

static struct scsi_pkt *
ghd_pktalloc(	ccc_t	*cccp,
		struct scsi_address *ap,
		int	 cmdlen,
		int	 statuslen,
		int	 tgtlen,
		int	(*callback)(),
		caddr_t	 arg,
		int  	 ccblen )
{
	opaque_t	 ha_private;
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

	if (!(gcmdp = (*cccp->ccc_ccballoc)(PKTP2TARGET(pktp), ha_private,
					    cmdlen, statuslen, tgtlen,
					    ccblen))) {
		scsi_hba_pkt_free(ap, pktp);
		return (NULL);
	}

	/*
	 * Save the gcmd_t ptr in the scsi_pkt,
	 * save the the scsi_pkt ptr in gcmd_t.
	 */
	pktp->pkt_ha_private = gcmdp;
	gcmdp->cmd_pktp = pktp;

	return (pktp);
}



/*
 * packet free
 */
/*ARGSUSED*/
void
ghd_pktfree(	ccc_t	*cccp,
		struct scsi_address	*ap,
		struct scsi_pkt		*pktp )
{
	(*cccp->ccc_ccbfree)(PKTP2GCMDP(pktp)->cmd_private);
	scsi_hba_pkt_free(ap, pktp);
	return;
}
