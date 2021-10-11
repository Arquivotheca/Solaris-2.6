/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)mlx_busops.c	1.1	95/10/16 SMI"

#include <sys/dktp/mlx/mlx.h>

/*
 * Define this if you have your own DMA engine on board.  The example
 * given is for a typical EISA bus-mastering board (much like the
 * Adaptec 174x).  This is a starting point for the dma limits structure;
 * a target may modify dlim_granular based on its knowledge of the
 * sector size.
 *
 * The MLX 53c710 supports a full 32 bit address register but only
 * a 24 bit counter per segment.
 */

ddi_dma_lim_t mlx_dma_lim = {
	0,			/* address low - should be 0		*/
	0xffffffffUL,		/* address high - all 32 bits on EISA	*/
	0,			/* counter max - not used, set to 0	*/
	1,			/* burstsize - not used, set to 1 	*/
	DMA_UNIT_8,		/* dlim_minxfer - gran. of DMA engine 	*/
	0,			/* dma speed - unused, set to 0		*/
	(u_int)DMALIM_VER0,	/* version		*/
	0xFFFFFFFFU,		/* address register		*/
	MLX_SCSI_MAX_XFER-1,	/* ctreg_max - how many count steps 	*/
	512,			/* dlim_granular - min device req size	*/
	MLX_MAX_NSG,		/* scatter/gather list length		*/
	MLX_SCSI_MAX_XFER	/* reqsize - max device I/O length	*/
};

ddi_dma_lim_t mlx_dac_dma_lim = {
	0,			/* address low - should be 0		*/
	0xffffffffUL,		/* address high - all 32 bits on EISA	*/
	0,			/* counter max - not used, set to 0	*/
	1,			/* burstsize - not used, set to 1 	*/
	DMA_UNIT_8,		/* dlim_minxfer - gran. of DMA engine 	*/
	0,			/* DMa speed - unused, set to 0		*/
	(u_int)DMALIM_VER0,	/* version		*/
	0xFFFFFFFFU,		/* address register		*/
	MLX_DAC_MAX_XFER-1,	/* ctreg_max - how many count steps 	*/
	512,			/* dlim_granular - min device req size	*/
	MLX_MAX_NSG,		/* scatter/gather list length		*/
	MLX_DAC_MAX_XFER 	/* reqsize - max device I/O length	*/
};

/*
 * Add overriding properties for "queue" and "flow_control" for child,
 * because we know the supports tagged queueing and may benefit from his
 * own queueing and flow_control strategies.
 * pdip is parent's; cdip is child's.
 *
 * This function is not currently used by the driver; it's here for
 * demonstration purposes, to instruct you in how to create a property for
 * a child, should you want to override the HBA's properties.
 */

void
mlx_childprop(dev_info_t *pdip, dev_info_t *cdip)
{
	char	que_keyvalp[OBJNAMELEN];
	int	que_keylen;
	char	flc_keyvalp[OBJNAMELEN];
	int	flc_keylen;

	MDBG7(("mlx_childprop \n"));

	que_keylen = sizeof (que_keyvalp);
	if (ddi_prop_op(DDI_DEV_T_NONE, pdip, PROP_LEN_AND_VAL_BUF,
		DDI_PROP_CANSLEEP|DDI_PROP_DONTPASS, "tag_queue",
		(caddr_t)que_keyvalp, &que_keylen) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
			"mlx_childprop: tagged queue property undefined");
		return;
	}
	que_keyvalp[que_keylen] = (char)0;

	flc_keylen = sizeof (flc_keyvalp);
	if (ddi_prop_op(DDI_DEV_T_NONE, pdip, PROP_LEN_AND_VAL_BUF,
		DDI_PROP_CANSLEEP|DDI_PROP_DONTPASS, "tag_fctrl",
		(caddr_t)flc_keyvalp, &flc_keylen) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
		    "mlx_childprop: tagged flow-control property undefined");
		return;
	}
	flc_keyvalp[flc_keylen] = (char)0;

	(void) ddi_prop_create(DDI_DEV_T_NONE, cdip, DDI_PROP_CANSLEEP,
		"queue", (caddr_t)que_keyvalp, que_keylen);
	(void) ddi_prop_create(DDI_DEV_T_NONE, cdip, DDI_PROP_CANSLEEP,
		"flow_control", (caddr_t)flc_keyvalp, flc_keylen);
}
