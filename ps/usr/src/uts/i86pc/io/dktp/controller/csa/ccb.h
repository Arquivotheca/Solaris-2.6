/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CSA_CCB_H
#define	_CSA_CCB_H

#pragma	ident	"@(#)ccb.h	1.1	95/05/17 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * the common portion of the Command Control Block
 */
typedef struct ccb {
	Qel_t		 ccb_q;
	opaque_t	 ccb_private;	/* used by the controller's driver */
	struct cmpkt	*ccb_pktp;	/* dadk common packet */
	unchar		 ccb_pad;
	unchar		 ccb_cdb;	/* target driver command */
	unchar		 ccb_scb;
	unchar		 ccb_rawmode;
	daddr_t		 ccb_block;

	ddi_dma_handle_t ccb_dma_handle;
	ddi_dma_win_t	 ccb_dmawin;
	ddi_dma_seg_t	 ccb_dmaseg;
	int		(*ccb_sg_func)();
} ccb_t;

#ifdef	__cplusplus
}
#endif

#endif  /* _CSA_CCB_H */
