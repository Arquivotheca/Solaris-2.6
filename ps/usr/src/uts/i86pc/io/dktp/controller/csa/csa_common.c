/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)csa_common.c	1.3	95/06/05 SMI"

#include "csa.h"

/*
 * Local Function Prototypes
 */

/*ARGSUSED*/
void
common_uninitchild(
	dev_info_t	*mdip,
	dev_info_t	*cdip
)
{
	struct scsi_device *sdp;

	sdp = (struct scsi_device *)ddi_get_driver_private(cdip);

	kmem_free((caddr_t)sdp, sizeof (*sdp) + sizeof (struct ctl_obj));

	ddi_set_driver_private(cdip, NULL);
	ddi_set_name_addr(cdip, NULL);
	return;
}



/*
 * create a scsi_devicee structure for this logical drive's instance.
 * attach the ctlobj in place of the scsi_hba_tran struct.
 *
 *
 */

int
common_initchild(
	dev_info_t	*mdip,
	dev_info_t	*cdip,
	opaque_t	 ctl_data,
	struct ctl_objops *ctl_ops,
	struct ctl_obj	**ctlobjpp
)
{
	struct scsi_device	*sdp;
	struct ctl_obj		*ctlobjp;
	int			 len;
	int			 ldrive;
	int			 lun;
	char			 namebuf[MAXNAMELEN];

	/*
	 * If lun isn't specified, assume 0.
	 * If a lun other than 0 is specified, fail it now.
	 */
	len = sizeof (int);
	if (HBA_INTPROP(cdip, "lun", &lun, &len) == DDI_SUCCESS && lun != 0)
		return (DDI_NOT_WELL_FORMED);

	/*
	 * use the target property as the logical drive number
	 */
	len = sizeof (int);
	if (HBA_INTPROP(cdip, "target", &ldrive, &len) != DDI_SUCCESS)
		return (DDI_NOT_WELL_FORMED);

	sdp = (struct scsi_device *)kmem_zalloc(sizeof (*sdp) +
				sizeof (*ctlobjp), KM_NOSLEEP);
	if (!sdp)
		return (DDI_NOT_WELL_FORMED);

	ctlobjp = (struct ctl_obj *)(sdp+1);

	sdp->sd_dev = cdip;
	sdp->sd_address.a_hba_tran = (struct scsi_hba_tran *)ctlobjp;
	sdp->sd_address.a_target = (u_short)ldrive;
	sdp->sd_address.a_lun = 0;	/* no support for lun's */

	ctlobjp->c_ops = ctl_ops;
	ctlobjp->c_data = ctl_data;
	ctlobjp->c_ext = &(ctlobjp->c_extblk);
	CTL_DIP_CTL(ctlobjp) = mdip;
	CTL_DIP_DEV(ctlobjp) = cdip;
	CTL_GET_TARG(ctlobjp) = ldrive;
	CTL_GET_BLKSZ(ctlobjp) = NBPSCTR;

	sprintf(namebuf, "sd mutex %s:%d:%d", ddi_get_name(mdip),
		ddi_get_instance(mdip), ldrive);
	mutex_init(&sdp->sd_mutex, namebuf, MUTEX_DRIVER, NULL);

	sprintf(namebuf, "%d,0", ldrive);
	ddi_set_name_addr(cdip, namebuf);
	ddi_set_driver_private(cdip, (caddr_t)sdp);

	if (ctlobjpp)
		*ctlobjpp = ctlobjp;
	return (DDI_SUCCESS);
}



struct cmpkt *
common_pktalloc(
	ccb_t		*(*ccballoc_func)(opaque_t),
	void		 (*ccbfree_func)(opaque_t, ccb_t *),
	opaque_t	 ccb_arg
)
{
	struct cmpkt	*pktp;
	ccb_t		*ccbp;

	if (!(ccbp = (*ccballoc_func)(ccb_arg))) {
		CDBG_WARN(("common_pktalloc(): failed\n"));
		return (NULL);
	}

	if ((pktp = CCBP2PKTP(ccbp)) == NULL) {
		/* no cmpkt attached, allocate and attach it now */
		if (!(pktp = kmem_zalloc(sizeof (struct cmpkt), KM_NOSLEEP))) {
			(*ccbfree_func)(ccb_arg, ccbp);
			return (NULL);
		}
		ccbp->ccb_pktp = pktp;
	}
	/* must reinitialize the command packet each time */
	bzero((caddr_t)pktp, sizeof (struct cmpkt));

	pktp->cp_ctl_private = (opaque_t)ccbp;
	pktp->cp_cdbp = (opaque_t)&ccbp->ccb_cdb;
	pktp->cp_cdblen = 1;
	pktp->cp_scbp = (opaque_t)&ccbp->ccb_scb;
	pktp->cp_scblen = 1;
	return (pktp);
}



Bool_t
common_iosetup(
	struct cmpkt	*pktp,
	ccb_t		*ccbp,
	int		 max_sgllen,
	void		(*sg_func)(struct cmpkt *, ccb_t *, ddi_dma_cookie_t *,
				   int, opaque_t),
	opaque_t	sg_func_arg
)
{
	ddi_dma_cookie_t cookie;
	int		 status;
	off_t		 off;
	off_t		 len;
	ulong		 bytes_xfer = 0;
	int		 num_segs = 0;


nextwin:
	if (ccbp->ccb_dmawin == (ddi_dma_win_t)0) {
		status = ddi_dma_nextwin(ccbp->ccb_dma_handle, ccbp->ccb_dmawin,
					 &ccbp->ccb_dmawin);
		if (status != DDI_SUCCESS) {
			CDBG_WARN(("?csa_common_iosetup(0x%x): status=%d\n",
				   status, ccbp));
			return (FALSE);
		}
		ccbp->ccb_dmaseg = (ddi_dma_seg_t)0;
	}

	do {
		status = ddi_dma_nextseg(ccbp->ccb_dmawin, ccbp->ccb_dmaseg,
					 &ccbp->ccb_dmaseg);
		if (status == DDI_DMA_STALE) {
			CDBG_WARN(("?csa_common_iosetup(0x%x): stale\n", ccbp));
			return (FALSE);
		}

		if (status == DDI_DMA_DONE) {
			ccbp->ccb_dmawin = (ddi_dma_win_t)0;
			if (num_segs == 0)
				goto nextwin;
			return (TRUE);
		}

		ddi_dma_segtocookie(ccbp->ccb_dmaseg, &off, &len, &cookie);

		/* call the controller specific S/G function */
		(*sg_func)(pktp, ccbp, &cookie, num_segs, sg_func_arg);

		bytes_xfer += cookie.dmac_size;
		num_segs++;

	} while (num_segs < max_sgllen && bytes_xfer < pktp->cp_bytexfer);

	pktp->cp_resid = pktp->cp_bytexfer = bytes_xfer;
	return (TRUE);
}



/*
 * Autovector Interrupt Entry Point
 *
 *	Dummy return to be used before mutexes has been initialized
 *	guard against interrupts from drivers sharing the same irq line
 */

/*ARGSUSED*/
static u_int
common_dummy_intr(caddr_t arg)
{
	return (DDI_INTR_UNCLAIMED);
}

/*
 * Convert an IRQ into the index number of the matching tuple
 * in the interrupts property from the driver.conf file.
 */

Bool_t
common_xlate_irq(
	dev_info_t	*dip,
	unchar		 irq,
	u_int		*inumber
)
{
	struct regspec {
		int	level;
		int	irq;
	} *intrp;
	int	 len;
	int	 nintrs;
	int	 indx;

	/* get the "interrupts" property specified by the driver.conf file */
	if (ddi_getlongprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
			    "interrupts", (caddr_t)&intrp, &len)
		!= DDI_PROP_SUCCESS) {
		return (FALSE);
	}
	/*
	 * Check the "interrupts" property tuples (level, irq) for a
	 * match on the irq. The irq value is the second int of
	 * the tuple.
	 */
	nintrs = len / sizeof (*intrp);
	for (indx = 0; indx < nintrs; indx++, intrp) {
		if (intrp[indx].irq == irq) {
			*inumber = indx;
			kmem_free(intrp, len);
			return (TRUE);
		}
	}
	kmem_free(intrp, len);
	return (FALSE);
}



Bool_t
common_intr_init(
	dev_info_t	*dip,
	int		inumber,
	ddi_iblock_cookie_t *iblock_cookiep,
	kmutex_t	*mutexp,
	char		*mutex_name,
	u_int		(*int_handler)(caddr_t),
	caddr_t		int_handler_arg
)
{
	/*
	 *	Establish initial dummy interrupt handler
	 *	get iblock cookie to initialize mutexes used in the
	 *	real interrupt handler
	 */
	if (ddi_add_intr(dip, inumber, iblock_cookiep, NULL,
			 common_dummy_intr, int_handler_arg) != DDI_SUCCESS) {
		return (FALSE);
	}
	mutex_init(mutexp, mutex_name, MUTEX_DRIVER, *iblock_cookiep);
	ddi_remove_intr(dip, inumber, *iblock_cookiep);

	/* Establish real interrupt handler */
	if (ddi_add_intr(dip, inumber, iblock_cookiep, NULL,
			 int_handler, int_handler_arg) != DDI_SUCCESS) {
		return (FALSE);
	}
	return (TRUE);
}


void
common_intr(
	opaque_t	 arg,
	opaque_t	 status,
	void		(*process_intr)(opaque_t arg, opaque_t status),
	int		(*get_status)(opaque_t arg, opaque_t *status),
	kmutex_t	*mutexp,
	Que_t		*doneq
)
{
	ccb_t		*ccbp;
	struct cmpkt	*pktp;

	for (;;) {
		/* process the interrupt status */
		(*process_intr)(arg, status);

		/* run the completion callback of anything that completed */
		while (ccbp = (ccb_t *)QueueRemove(doneq)) {
			mutex_exit(mutexp);
			pktp = CCBP2PKTP(ccbp);
			if (pktp != NULL && pktp->cp_callback != NULL)
				(*pktp->cp_callback)(pktp);
#if CSA_DEBUG
			else
				CDBG_ERROR(("common_intr: null 0x%x\n", ccbp));
#endif

			mutex_enter(mutexp);
		}

		/* loop if new interrupt status was stacked by hardware */
		if (!(*get_status)(arg, &status))
			return;
	}
}
