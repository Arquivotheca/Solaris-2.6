/*
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident	"@(#)pcscsi_dma_impl.c 95/05/17 SMI"


/* ---------------------------------------------------------------------- */
/*
 * Includes
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



#include <sys/dktp/pcscsi/pcscsi_dma_impl.h>
/* Needed only for PCSCSI_DEBUG symbols.	 */
#include <sys/dktp/pcscsi/pcscsi.h>


/* ========================================================================== */
/*
 * Globals
 */

#ifdef PCSCSI_DEBUG
extern uint	pcscsig_debug_funcs;	/* Defined in pcscsi.c  */
extern uint	pcscsig_debug_gran;	/* Defined in pcscsi.c  */
extern char	pcscsig_dbgmsg[];	/* Defined in pcscsi.c  */
#endif /* PCSCSI_DEBUG */


/* -------------------------------------------------------------------------- */
/*
 * Dma resource allocation
 */
int
dma_impl_setup(
	dev_info_t		 *dip,
	dma_blk_t		 *dma_state_p,
	struct buf		 *bp,
	int			pkt_flags,
	opaque_t		driver_private,
	ddi_dma_lim_t		 *dma_lim_p,
	int			(*callback)(),
	caddr_t			arg,
	boolean_t		new_transfer)
{
	off_t			offset;
	off_t			len;
	ddi_dma_cookie_t	dma_cookie;
	ddi_dma_cookie_t	 *dma_cookie_p;
	sg_list_entry_t		 *sg_list_entry_p;


	dma_cookie_p = &dma_cookie;


	/*
	 * It is possible to get a buf with a 0 bcount.
	 * In this case, there's nothing else to do here.
	 */
	if (bp->b_bcount == 0) {
		dma_state_p->dma_seg_xfer_cnt = 0;
		dma_state_p->dma_totxfer = 0;
		dma_state_p->dma_sg_nbr_entries = 0;

		return (DDI_SUCCESS);
	}


	/*
	 * Initialize static info in the dma_blk.
	 */
	if (new_transfer)  {

		dma_state_p->dma_driver_priv = driver_private;
		dma_state_p->dma_buf_p = bp;


		/*
		 * Translate pkt_flags to dma_flags for call to
		 * dma_impl_init (below; which ultimately calls
		 * ddi_dma_buf_setup).
		 */
		if (bp->b_flags & B_READ)
			dma_state_p->dma_flags = DDI_DMA_READ;
		else
			dma_state_p->dma_flags = DDI_DMA_WRITE;

		if (pkt_flags & PKT_CONSISTENT)
			dma_state_p->dma_flags |= DDI_DMA_CONSISTENT;

		if (pkt_flags & PKT_DMA_PARTIAL)
			dma_state_p->dma_flags |= DDI_DMA_PARTIAL;


#ifdef SAVE_DMA_SEG_VIRT_ADDRS
		/*
		 * If this is a page-io or physio buffer,
		 * the b_un.b_addr is *not* the
		 * virtual address of the buffer, it's the offset into the
		 * page being requested.
		 *
		 * We need to have a virtual address to associate with the
		 * buffer, for physical-to-virtual translation necessitated
		 * by the hw.
		 *
		 * If it's a physio buffer, the virtual address mapping for the
		 * buffer may *not* be valid at interrupt time - which is
		 * when the virtual addresses are (probably) needed.
		 */
		if (bp->b_flags & (B_PAGEIO | B_PHYS)) {

			/*
			 * Create a virtual mapping for the pages;
			 * set b_un.b_addr to this value.
			 */
			bp_mapin(bp);
		}
		ASSERT(bp->b_un.b_addr != 0);	/* Make sure it didn't fail. */

#endif /* SAVE_DMA_SEG_VIRT_ADDRS */


	} /* End if new_transfer */


	/*
	 * Setup dma memory and position to the first xfer segment
	 * -OR-
	 * Update dmawin and dmaseg for the next transfer.
	 *
	 * This initializes (OR updates)
	 *	dma_state_p->dma_handle, dma_state_p->dma_win,
	 *	dma_state_p->dma_seg.
	 */
	if (dma_impl_init(dip, dma_state_p, callback, arg, dma_lim_p)
					!= DDI_SUCCESS)  {
		cmn_err(CE_WARN, "dma_impl_setup:dma_impl_init failed.\n");
		return (DDI_FAILURE);
	}


#ifdef  PCSCSI_DEBUG
	pcscsi_debug(DBG_DMAGET, DBG_RESULTS, sprintf(pcscsig_dbgmsg,
	"pcscsi_dmaget: "
	"After pcscsi_impl_dmaget: dmahandle:%x dmawin:%x dmaseg:%x\n",
		dma_state_p->dma_handle,
		dma_state_p->dma_dmawin,
		dma_state_p->dma_seg));
#endif  /* PCSCSI_DEBUG */



	/*
	 * Get the DMA cookie for the first segment.
	 */
	if (ddi_dma_segtocookie(dma_state_p->dma_seg, &offset, &len,
				dma_cookie_p) == DDI_FAILURE) {
		cmn_err(CE_PANIC, "dma_impl_setup: ddi_dma_segtocookie error");
	}


#ifndef SG_ALWAYS
	if (bp->b_bcount <= dma_cookie_p->dmac_size) { /* Single-block xfer */

		/* SINGLE-BLOCK TRANSFER NOT DEBUGGED */

		/*
		 * scsi_htos_long means 'host-to-scsi'
		 * which means 'convert from a host-format value
		 * (e.g. big-endian) to a "scsi-based" (i.e., hardware
		 * acceptable) value.
		 */
		scsi_htos_long(dma_state_p->ccb_datap,
			dma_cookie_p->dmac_address);
		scsi_htos_long(dma_state_p->ccb_datalen, bp->b_bcount);

	} else	/* Use scatter-gather transfer	 */

#endif /* SG_ALWAYS */


	{	/* Start a block here so above #ifdefs will work right. */


		/*
		 * set address of the first sg_entry_t in the list of
		 * scatter/gather entries.
		 */
		sg_list_entry_p = (sg_list_entry_t *)
					(dma_state_p->dma_sg_list_p);


		/*
		 * Set up entries in the s/g list, until done or we've
		 * built as big a s/g list as the h/w can handle.
		 */
		for (dma_state_p->dma_seg_xfer_cnt = 0,
				dma_state_p->dma_sg_nbr_entries = 1;
				/* Loop till breakout */;
				dma_state_p->dma_sg_nbr_entries++,
				sg_list_entry_p++)	{


			/* Set the DMA address and byte count for this seg */


#ifdef BIG_ENDIAN_HW
			scsi_htos_long((unchar *)&sg_list_entry_p->data_len,
					dma_cookie_p->dmac_size);
			scsi_htos_long((unchar *)&sg_list_entry_p->data_addr,
					dma_cookie_p->dmac_address);
#else
			sg_list_entry_p->data_len  = (ulong)
						dma_cookie_p->dmac_size;
			sg_list_entry_p->data_addr = (ulong)
						dma_cookie_p->dmac_address;
#endif /* BIG_ENDIAN_HW */




#ifdef SAVE_DMA_SEG_VIRT_ADDRS
			/*
			 * Save the virtual address of the start of this seg.
			 * (Needed for Physical->Virtual addresss translation).
			 */
			dma_state_p->dma_sg_list_virtaddrs
					[dma_state_p->dma_sg_nbr_entries - 1] =
				(caddr_t) (bp->b_un.b_addr
					+ dma_state_p->dma_totxfer
					+ dma_state_p->dma_seg_xfer_cnt);
#endif /* SAVE_DMA_SEG_VIRT_ADDRS */



			/*
			 * Accumulate total size of *this* DMA transfer
			 * (which may -not- encompass all of b_bcount).
			 */
			dma_state_p->dma_seg_xfer_cnt +=
				dma_cookie_p->dmac_size;


			/*
			 * Check for end of S/G list
			 * (Don't exceed driver/hw scatter-gather list limit)
			 */
			if (dma_state_p->dma_sg_nbr_entries >=
							MAX_SG_LIST_ENTRIES)  {
				break;
			}


			/*
			 * Check for end of transfer (redundant?).
			 */
			if (dma_state_p->dma_totxfer == bp->b_bcount)  {
				break;
			}


			/*
			 * Move to next segment (if any left).
			 */
			if (ddi_dma_nextseg(dma_state_p->dma_dmawin,
					dma_state_p->dma_seg,
					&dma_state_p->dma_seg)
						!= DDI_SUCCESS) {

				break;	/* No more segments */
			}


			if (ddi_dma_segtocookie(dma_state_p->dma_seg,
						&offset, &len, dma_cookie_p)
							== DDI_FAILURE)  {
				cmn_err(CE_PANIC, "scsi_dmaget: "
						"ddi_dma_segtocookie error");
			}

		}	/* End for (all DMA segments) */


		/*
		 * Update the total nbr bytes we have allocated DMA resources
		 * for.
		 */
		dma_state_p->dma_totxfer += dma_state_p->dma_seg_xfer_cnt;
		ASSERT(bp->b_bcount >= dma_state_p->dma_totxfer);

	}

	return (DDI_SUCCESS);
}


/* -------------------------------------------------------------------------- */
static int
dma_impl_init(
	dev_info_t		*dip,
	dma_blk_t		*dma_state_p,
	int			(*callback)(),
	caddr_t			callback_arg,
	ddi_dma_lim_t		*dma_lim_p)
{
	int			status;


	/*
	 * If no resources have been set up yet, set them up
	 */
	if (!dma_state_p->dma_handle) {

		/* Set up the DNA object	 */
		status = ddi_dma_buf_setup(dip, dma_state_p->dma_buf_p,
					dma_state_p->dma_flags,
					callback, callback_arg, dma_lim_p,
		&dma_state_p->dma_handle);

		if (status != DDI_SUCCESS) {

			cmn_err(CE_WARN,
				"dma_impl_init: ddi_dma_buf_setup failed.\n");

			switch (status) {
			case DDI_DMA_NORESOURCES:
				cmn_err(CE_CONT, "dma_impl_init:  "
						"DDI_DMA_NORESOURCES\n");
				dma_state_p->dma_buf_p->b_error = 0;
				break;
			case DDI_DMA_TOOBIG:
				cmn_err(CE_CONT, "dma_impl_init:  "
						"DDI_DMA_TOOBIG\n");
				dma_state_p->dma_buf_p->b_error = EINVAL;
				break;
			case DDI_DMA_NOMAPPING:
				cmn_err(CE_CONT, "dma_impl_init:  "
						"DDI_DMA_NOMAPPING\n");
			default:
				cmn_err(CE_CONT, "dma_impl_init:  "
						"Unknown error\n");
				dma_state_p->dma_buf_p->b_error = EFAULT;
				break;
			}
			return (status);
		}

	} else {


		/*
		 * Resources already set up; get next segment.
		 */
		status = ddi_dma_nextseg(dma_state_p->dma_dmawin,
					dma_state_p->dma_seg,
					&dma_state_p->dma_seg);
		if (status != DDI_DMA_DONE)  {
			return (status);
		}
		/* DDI_DMA_DONE = end of window.  Fall through and continue. */

	}


	/*
	 * Get the first (or next) window.
	 */
	status = ddi_dma_nextwin(dma_state_p->dma_handle,
					dma_state_p->dma_dmawin,
					&dma_state_p->dma_dmawin);
	if (status == DDI_DMA_DONE) {	/* Everything complete? */

		/*
		 * Reset things to wrap around to the first window.
		 */
		status = ddi_dma_nextwin(dma_state_p->dma_handle,
					NULL,
					&dma_state_p->dma_dmawin);
		if (status != DDI_SUCCESS)  {	/* Shouldn't happen */
			cmn_err(CE_WARN, "ddi_dma_nextwin reset failed.\n");
			return (status);
		}

		return (DDI_DMA_DONE);

	} else if (status != DDI_SUCCESS)  {	/* Something broke */
		cmn_err(CE_WARN, "ddi_dma_nextwin failed.\n");
		return (status);
	}


	/*
	 * Get first segment in the (just gotten) window.
	 */
	status = ddi_dma_nextseg(dma_state_p->dma_dmawin, NULL,
				&dma_state_p->dma_seg);

	if (status != DDI_SUCCESS)  {
		cmn_err(CE_WARN, "ddi_dma_nextseg (init) failed.\n");
	}

	return (status);
}
