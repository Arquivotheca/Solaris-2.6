/*
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)sg.c	1.4	94/07/01 SMI"

#include <sys/dktp/ncrs/ncr.h>


/*
 * set up the target's Scatter/Gather DMA list
 */

void
ncr_sg_setup(	ncr_t	*ncrp,
		npt_t	*nptp,
		nccb_t	*nccbp )
{
	int	numsg;

	/* number of entries in the DMA Scatter/Gather list */
	numsg = nccbp->nc_num;

	nptp->nt_savedp.nd_num = numsg;	/* total # of S/G list entries */
	nptp->nt_savedp.nd_left = numsg; /* # of entries left to do */

	/* Right justify the list. I.e., copy from the front
	 * of the caller's array to the end of the array in the
	 * target structure.
	 */
	if (numsg != 0) 
	    bcopy((caddr_t)&nccbp->nc_sg[0]
		, (caddr_t)&nptp->nt_savedp.nd_data[NCR_MAX_DMA_SEGS - numsg]
		, sizeof nccbp->nc_sg[0] * numsg);

	nptp->nt_curdp = nptp->nt_savedp;	
	return;
}

/*
 * Save the scatter/gather current-index and number-completed
 * values so when the target reconnects we can restart the
 * data in/out move instruction at the proper point. Also, if the
 * disconnect happened within a segment there's a fixup value
 * for the partially completed data in/out move instruction.
 */
void
ncr_sg_update(	ncr_t	*ncrp,
			npt_t	*nptp,
			unchar	 index,
			ulong	 remain )
{
	ncrti_t	*sgp;


	/* record the number of segments left to do */
	nptp->nt_curdp.nd_left = NCR_MAX_DMA_SEGS - index;

	/* if interrupted between segments then don't adjust S/G table */
	if (remain == 0) {
		/* Must have just completed the current segment when
		 * the interrupt occurred,  restart at the next segment.
		 */
		nptp->nt_curdp.nd_left--;
		return;
	}

	/* fixup the Table Indirect entry for this segment */
	sgp = &nptp->nt_curdp.nd_data[index];

	NDBG26(("ncr_sg_update: ioaddr=0x%x remain=%d\n"
			, ncrp->n_ioaddr, remain));
	NDBG26(("    Total number of bytes to transfer was %d,", sgp->count));
	NDBG26(("at physical address=0x%x\n", sgp->address));

	sgp->address += sgp->count - remain;
	sgp->count = remain;

	return;
}

/*
 * Determine if the command completed with any bytes leftover 
 * in the Scatter/Gather DMA list.
 */
ulong
ncr_sg_residual(	ncr_t	*ncrp,
			npt_t	*nptp )
{
	ncrti_t	*sgp;
	ulong	 residual = 0;
	int	 index;

	index = NCR_MAX_DMA_SEGS - nptp->nt_savedp.nd_left;

	NDBG26(("ncr_sg_residual: ioaddr=0x%x index=%d\n"
			, ncrp->n_ioaddr, index));

	sgp = &nptp->nt_curdp.nd_data[index];

	for (; index < NCR_MAX_DMA_SEGS; index++, sgp++) 
		residual += sgp->count;

	NDBG26(("ncr_sg_residual: ioaddr=0x%x residual=%d\n"
			, ncrp->n_ioaddr, residual));
	return (residual);
}
