/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)csa.c	1.7	95/07/12 SMI"

#define	HBANAME	"csa"

#include "csa.h"

/*
 * External references
 */

/*
 * Local Function Prototypes
 */
static	int	csa_initchild(dev_info_t *mdip, dev_info_t *cdip);
static	int	csa_uninitchild(dev_info_t *mdip, dev_info_t *cdip);
static	int	csa_propinit(dev_info_t	*dip, csa_blk_t	*csa_blkp);
static	uint	csa_intr(caddr_t arg);

/*
 * Interface to dadk.
 */
struct cmpkt	*csa_pktalloc(csa_t *csap, int (*callback)(),
			caddr_t arg);
void		 csa_pktfree(csa_t *csap, struct cmpkt *pktp);
struct cmpkt	*csa_memsetup(csa_t *csap, struct cmpkt *pktp,
			struct buf *bp, int (*callback)(), caddr_t arg);
void		 csa_memfree(csa_t *csap, struct cmpkt *pktp);
struct cmpkt	*csa_iosetup(csa_t *csap, struct cmpkt *pktp);
static	int	 csa_transport(csa_t *csap, struct cmpkt *pktp);
static	int	 csa_reset(csa_t *csap, int level);
static	int	 csa_abort(csa_t *csap, struct cmpkt *pktp);
static	int	 csa_ioctl(csa_t *csap, int cmd, int arg, int flag);


static	int	 csa_bus_ctl(dev_info_t *d, dev_info_t *r, ddi_ctl_enum_t o,
				void *a, void *v);


#ifndef	CSA_DEBUG
ulong	csa_debug_flags = 0;
#else
ulong	csa_debug_flags = CDBG_FLAG_ERROR
		/*	| CDBG_FLAG_INTR	*/
		/*	| CDBG_FLAG_PEND_INTR	*/
		/*	| CDBG_FLAG_RW		*/
		/*	| CDBG_FLAG_SEND	*/
		/*	| CDBG_FLAG_START	*/
			| CDBG_FLAG_WARN
			;
#endif

/*
 * These are the entry points that dadk expects to be present.
 */
static struct ctl_objops csa_objops = {
	csa_pktalloc,
	csa_pktfree,
	csa_memsetup,
	csa_memfree,
	csa_iosetup,
	csa_transport,
	csa_reset,
	csa_abort,
	nulldev,
	nulldev,
	csa_ioctl,
	0, 0
};

/*
 * Local static data
 */


ddi_dma_lim_t csa_dma_lim = {
	0,		/* address low				*/
	(ulong)0xffffffff,	/* address high			*/
	0,		/* counter max				*/
	1,		/* burstsize				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version			*/
	(ulong)0xffffffff,	/* address register		*/
	/*
	 * NOTE: in order to avoid a bug in rootnex_io_brkup the following
	 *	 is set to 2^32 - 2 rather than 2^32 - 1.
	 */
	(ulong)0xfffffffe,	/* counter register: ctreg_max	*/

	NBPSCTR,	/* sector size: granular		*/
	CSA_MAX_SG,	/* scatter/gather list length: sgllen	*/
	(NBPSCTR * ((1 << 16) - 1)) /* request size: regsize	*/
};

/*
 *	bus nexus operations.
 */

static struct bus_ops csa_bus_ops = {
#if _SOLARIS_PS_RELEASE >= 250
	BUSO_REV,
#endif
	nullbusmap,
	0,		/* ddi_intrspec_t (*bus_get_intrspec)(); */
	0,		/* int		(*bus_add_intrspec)(); */
	0,		/* void		(*bus_remove_intrspec)(); */
	i_ddi_map_fault,
	ddi_dma_map,
#if _SOLARIS_PS_RELEASE >= 250
	ddi_dma_allochdl,
	ddi_dma_freehdl,
	ddi_dma_bindhdl,
	ddi_dma_unbindhdl,
	ddi_dma_flush,
	ddi_dma_win,
#endif
	ddi_dma_mctl,
	csa_bus_ctl,
	ddi_bus_prop_op,
};

static	int	csa_identify(dev_info_t *dev);
static	int	csa_probe(dev_info_t *);
static	int	csa_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static	int	csa_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

static struct dev_ops	csa_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt */
	ddi_no_info,		/* info */
	csa_identify,		/* identify */
	csa_probe,		/* probe */
	csa_attach,		/* attach */
	csa_detach,		/* detach */
	csa_flush_cache,	/* reset - flush the cache */
	(struct cb_ops *)0,	/* driver operations */
	&csa_bus_ops		/* bus operations */
};

/*
 * This is the driver loadable module wrapper.
 */
char _depends_on[] = "misc/dadk misc/strategy";

#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"Compaq SMART SCSI Array Controller Driver", /* Name of the module. */
	&csa_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, &modldrv, NULL
};


int
_init(void)
{
	return (mod_install(&modlinkage));
}

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}


/*ARGSUSED*/
static int
csa_bus_ctl(
	dev_info_t	*d,
	dev_info_t	*r,
	ddi_ctl_enum_t	 o,
	void		*a,
	void		*v
)
{

	switch (o) {
	case DDI_CTLOPS_REPORTDEV:
	{
		struct scsi_device	*sdp;
		struct ctl_obj		*ctlobjp;

		sdp = (struct scsi_device *)ddi_get_driver_private(r);
		ctlobjp = (struct ctl_obj *)sdp->sd_address.a_hba_tran;
		cmn_err(CE_CONT, "?%s%d at %s%d target %d lun %d\n",
			ddi_get_name(r), ddi_get_instance(r),
			ddi_get_name(d), ddi_get_instance(d),
			CTL_GET_TARG(ctlobjp), 0);
		return (DDI_SUCCESS);
	}
	case DDI_CTLOPS_INITCHILD:
		return (csa_initchild(d, a));

	case DDI_CTLOPS_UNINITCHILD:
		return (csa_uninitchild(d, a));

	default:
		CDBG_BUSCTL(("%s%d: received op (%d) from %s%d\n",
			     ddi_get_name(d), ddi_get_instance(d),
			     o, ddi_get_name(r), ddi_get_instance(r)));
		return (ddi_ctlops(d, r, o, a, v));
	}
}


static int
csa_initchild(
	dev_info_t	*mdip,
	dev_info_t	*cdip
)
{
	struct ctl_obj		*ctlobjp;
	csa_t			*csap;
	csa_unit_t		*csa_unitp;
	csa_blk_t		*csa_blkp;


	csa_blkp = (csa_blk_t *)ddi_get_driver_private(mdip);
	mutex_enter(&csa_blkp->cb_mutex);

	if (!(csap = (csa_t *)kmem_zalloc(sizeof (csa_t) + sizeof (csa_unit_t),
					  KM_NOSLEEP))) {
		goto invalid_dev1;
	}

	if (common_initchild(mdip, cdip, (opaque_t)csap, &csa_objops,
				   &ctlobjp) != DDI_SUCCESS) {
		goto invalid_dev2;
	}

	csa_unitp = (csa_unit_t *)(csap + 1);
	csap->c_blkp = csa_blkp;
	csap->c_unitp = csa_unitp;
	csap->c_ctlobjp = ctlobjp;

	/* save the target number as the logical drive number */
	csap->c_drive = CTL_GET_TARG(ctlobjp);

	/* the hardware only supports 8 drives, may as well check here */
	if (csap->c_drive >= CSA_MAX_LDRIVES) {
		goto invalid_dev3;
	}

	/* copy in template static dma_lim_t, then modify to suit */
	csa_unitp->cu_dmalim = csa_dma_lim;

	/* these values may be reset by csa_inquiry() */
	csa_unitp->cu_dmalim.dlim_granular = NBPSCTR;
	csa_unitp->cu_dmalim.dlim_reqsize = NBPSCTR * ((1 << 16) - 1);

	if (!csa_inquiry(mdip, cdip, csap)) {
		goto invalid_dev3;
	}

	mutex_exit(&csa_blkp->cb_mutex);
	return (DDI_SUCCESS);

invalid_dev3:
	common_uninitchild(mdip, cdip);
invalid_dev2:
	kmem_free(csap, sizeof (csa_t) + sizeof (csa_unit_t));
invalid_dev1:
	mutex_exit(&csa_blkp->cb_mutex);
	return (DDI_NOT_WELL_FORMED);
}


static int
csa_uninitchild(
	dev_info_t	*mdip,
	dev_info_t	*cdip
)
{
	csa_blk_t		*csa_blkp;
	struct scsi_device	*sdp;
	struct ctl_obj		*ctlobjp;
	csa_t			*csap;

	csa_blkp = (csa_blk_t *)ddi_get_driver_private(mdip);
	mutex_enter(&csa_blkp->cb_mutex);

/* ??? turn this into a macro ??? */
	sdp = (struct scsi_device *)ddi_get_driver_private(cdip);
	ctlobjp = (struct ctl_obj *)sdp->sd_address.a_hba_tran;
	csap = (csa_t *)ctlobjp->c_data;

	common_uninitchild(mdip, cdip);
	kmem_free(csap, sizeof (csa_t) + sizeof (csa_unit_t));

	mutex_exit(&csa_blkp->cb_mutex);
	return (DDI_SUCCESS);
}


static int
csa_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, HBANAME) == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}


static int
csa_probe(dev_info_t *devi)
{
	int	ioaddr;
	int	len;
	unchar	irq;

#ifdef CSA_DEBUGX
	debug_enter("\n\ncsa PROBE\n\n");
#endif

	len = sizeof (int);
	if (HBA_INTPROP(devi, "ioaddr", &ioaddr, &len) != DDI_SUCCESS) {
		return (DDI_PROBE_FAILURE);
	}
	if (!eisa_probe(devi, (ushort)ioaddr)) {
		return (DDI_PROBE_FAILURE);
	}
	if (!csa_bmic_mode((ushort)ioaddr)) {
		return (DDI_PROBE_FAILURE);
	}
	if (!csa_get_irq((ushort)ioaddr, &irq)) {
		return (DDI_PROBE_FAILURE);
	}

	return (DDI_PROBE_SUCCESS);
}


static int
csa_detach(
	dev_info_t		*devi,
	ddi_detach_cmd_t	cmd
)
{
	csa_blk_t	*csa_blkp;


#ifdef CSA_DEBUGX
	debug_enter("\n\ncsa DETACH\n\n");
#endif

	if (cmd != DDI_DETACH)
		return (DDI_FAILURE);

	if (!(csa_blkp = (csa_blk_t *)ddi_get_driver_private(devi))) {
		return (DDI_SUCCESS);
	}

	/* clear any pending interrupts */
	csa_dev_fini(csa_blkp, csa_blkp->cb_ioaddr);

	ddi_remove_intr(devi, csa_blkp->cb_inumber, csa_blkp->cb_iblock);
	mutex_destroy(&csa_blkp->cb_mutex);
	mutex_destroy(&csa_blkp->cb_rmutex);

	kmem_free((caddr_t)csa_blkp, sizeof (csa_blk_t));

	ddi_prop_remove_all(devi);
	ddi_set_driver_private(devi, (caddr_t)NULL);
	return (DDI_SUCCESS);
}


/*ARGSUSED*/
static int
csa_attach(
	dev_info_t	*devi,
	ddi_attach_cmd_t cmd
)
{
	csa_blk_t	*csa_blkp;
	uint		 inumber;
	ushort		 ioaddr;
	unchar		 irq;
	char		*errormsg;


#ifdef CSA_DEBUGX
	debug_enter("\n\ncsa ATTACH\n\n");
#endif

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	csa_blkp = (csa_blk_t *)kmem_zalloc(sizeof (csa_blk_t), KM_NOSLEEP);
	if (!csa_blkp) {
		return (DDI_FAILURE);
	}

	csa_blkp->cb_dip = devi;

	if (!csa_propinit(devi, csa_blkp)) {
		errormsg = "invalid property";
		goto bailout;
	}

	ioaddr = csa_blkp->cb_ioaddr;

	if (!eisa_probe(devi, ioaddr)) {
		errormsg = "adapter not found";
		goto bailout;
	}
	if (!csa_bmic_mode(ioaddr)) {
		errormsg = "BMIC mode not configured";
		goto bailout;
	}
	if (!csa_get_irq(ioaddr, &irq)) {
		errormsg = "IRQ not configured";
		goto bailout;
	}
	if (!common_xlate_irq(devi, irq, &inumber)) {
		errormsg = "invalid IRQ";
		goto bailout;
	}

	csa_blkp->cb_inumber = inumber;
	csa_blkp->cb_irq = irq;

	if (!common_intr_init(devi, inumber, &csa_blkp->cb_iblock,
			      &csa_blkp->cb_mutex, "csa Mutex", csa_intr,
			      (caddr_t)csa_blkp)) {
		errormsg = "cannot add intr";
		goto bailout;
	}

	/* initialize a mutex for the pool of free ccbs */
	mutex_init(&csa_blkp->cb_rmutex, "csa Resource Mutex", MUTEX_DRIVER,
		   (void *)csa_blkp->cb_iblock);

	mutex_enter(&csa_blkp->cb_rmutex);
	if (!csa_ccbinit(csa_blkp)) {
		mutex_exit(&csa_blkp->cb_rmutex);
		errormsg = "ccb init failed";
		goto bailout;
	}
	mutex_exit(&csa_blkp->cb_rmutex);

	ddi_set_driver_private(devi, (caddr_t)csa_blkp);
	ddi_report_dev(devi);

	/* set the interface to a known state */
	csa_dev_init(csa_blkp, ioaddr);

	return (DDI_SUCCESS);


bailout1:
	mutex_destroy(&csa_blkp->cb_rmutex);
	ddi_remove_intr(devi, csa_blkp->cb_inumber, csa_blkp->cb_iblock);
	mutex_destroy(&csa_blkp->cb_mutex);
bailout:
	cmn_err(CE_WARN, "csa_attach: %s", errormsg);
	kmem_free((caddr_t)csa_blkp, sizeof (csa_blk_t));
	return (DDI_FAILURE);
}


/*
 *	Common controller object interface
 */
struct cmpkt *
csa_pktalloc(
	csa_t	*csap,
	int	(*callback)(),
	caddr_t	 callback_arg
)
{
	csa_blk_t	*csa_blkp = CSAP2CSABLKP(csap);
	struct cmpkt	*pktp;

	mutex_enter(&csa_blkp->cb_rmutex);
	if (pktp = common_pktalloc(csa_ccballoc, csa_ccbfree,
				   (opaque_t)csa_blkp)) {
		mutex_exit(&csa_blkp->cb_rmutex);
		return (pktp);
	}
	CDBG_WARN(("?csa_pktalloc(0x%x,0x%x): failed\n", csa_blkp->cb_ioaddr,
		   csap->c_drive));
	if (callback) {
		ddi_set_callback(callback, callback_arg, &csa_blkp->cb_ccb_id);
	}
	mutex_exit(&csa_blkp->cb_rmutex);
	return (NULL);
}


void
csa_pktfree(
	csa_t		*csap,
	struct cmpkt	*pktp
)
{
	csa_blk_t	*csa_blkp = CSAP2CSABLKP(csap);

	mutex_enter(&csa_blkp->cb_rmutex);
	csa_ccbfree((opaque_t)CSAP2CSABLKP(csap), PKTP2CCBP(pktp));

	/* wake up any waiters */
	if (csa_blkp->cb_ccb_id) {
		CDBG_WARN(("?csa_pktfree(0x%x,0x%x): wakeup\n",
			   csa_blkp->cb_ioaddr, csap->c_drive));
		ddi_run_callback(&csa_blkp->cb_ccb_id);
	}
	mutex_exit(&csa_blkp->cb_rmutex);
	return;
}


/*
 * free dma handle and controller handle allocated in memsetup
 */

/*ARGSUSED*/
void
csa_memfree(
	csa_t		*csap,
	struct cmpkt	*pktp
)
{

#ifdef CSA_DEBUG
	if (CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status != CCB_CBUSY
	&&  CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status != CCB_CDONE) {
		CDBG_ERROR(("csa_memfree(): 0x%x 0x%x\n", csap, pktp));
debug_enter("\n\ncsa_memfree\n");
	}
#endif

	if (PKTP2CCBP(pktp)->ccb_dma_handle)
		ddi_dma_free(PKTP2CCBP(pktp)->ccb_dma_handle);
}


/*
 * this function assumes it's called just once per bp
 */

struct cmpkt *
csa_memsetup(
	csa_t		 *csap,
	struct cmpkt	 *pktp,
	struct buf	 *bp,
	int		(*callback)(),
	caddr_t		  arg
)
{
	ccb_t		*ccbp = PKTP2CCBP(pktp);
	int		 status;
	int		 flags;

#ifdef CSA_DEBUG
	if (CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status != CCB_CBUSY) {
		CDBG_ERROR(("csa_memsetup(): 0x%x 0x%x 0x%x\n",
			    csap, pktp, bp));
debug_enter("\n\ncsa_memsetup\n");
	}
#endif

	/* check direction for data transfer */
	if (bp->b_flags & B_READ)
		flags = DDI_DMA_READ;
	else
		flags = DDI_DMA_WRITE;

	status = ddi_dma_buf_setup(CTL_DIP_CTL(csap->c_ctlobjp), bp, flags,
			callback, arg, &csap->c_unitp->cu_dmalim,
			&ccbp->ccb_dma_handle);

	if (status == DDI_DMA_MAPOK) {
		/* enable first call to ddi_dma_nextwin */
		ccbp->ccb_dmawin = (ddi_dma_win_t)0;
		return (pktp);
	}

	switch (status) {
	case DDI_DMA_NORESOURCES:
		bp->b_error = 0;
		break;
	case DDI_DMA_TOOBIG:
		bp->b_error = EFBIG;
		break;
	case DDI_DMA_NOMAPPING:
	default:
		bp->b_error = EFAULT;
		break;
	}
	CDBG_WARN(("?csa_memsetup(0x%x,0x%x): status=%d\n",
		   CSAP2CSABLKP(csap)->cb_ioaddr, csap->c_drive, status));
	return (NULL);
}


/*ARGSUSED*/
struct cmpkt *
csa_iosetup(
	csa_t		*csap,
	struct cmpkt	*pktp
)
{
	ccb_t	*ccbp = PKTP2CCBP(pktp);
	rblk_t	*rbp = &CCBP2CSACCBP(ccbp)->csa_ccb_clp->cl_req;

	if (CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status == CCB_CBUSY)
		;
	else if (CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status == CCB_CDONE)
		CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status = CCB_CBUSY;
#ifdef CSA_DEBUG
	else {
		CDBG_ERROR(("csa_iosetup(): 0x%x 0x%x\n", csap, pktp));
debug_enter("\n\ncsa_iosetup\n");
	}
#endif

	/* zero the number of scatter/gather descriptors */
	rbp->rb_hdr.rh_cnt1 = 0;


#if _SOLARIS_PS_RELEASE >= 250
	if (pktp->cp_passthru) {
		struct dadkio_rwcmd *rwcmdp;
		rwcmdp = (struct dadkio_rwcmd *)(pktp->cp_bp->b_back);

		switch (rwcmdp->cmd) {
		case DADKIO_RWCMD_READ:
			break;
		case DADKIO_RWCMD_WRITE:
			break;
		default:
			cmn_err(CE_WARN, "csa_iosetup: bad command %x\n",
				rwcmdp->cmd);
			return (NULL);
		}
		pktp->cp_srtsec = rwcmdp->blkaddr;
		pktp->cp_bytexfer = rwcmdp->buflen;
	} else
#endif
	{
		switch (ccbp->ccb_cdb) {
		case DCMD_READ:
		case DCMD_WRITE:
			break;
		case DCMD_SEEK:
		case DCMD_RECAL:
			return (pktp);
		default:
			cmn_err(CE_WARN, "csa_iosetup: bad command %x\n",
				ccbp->ccb_cdb);
			return (NULL);
		}
	}


	if (!common_iosetup(pktp, ccbp, CSA_MAX_SG, csa_sg_func,
			    (opaque_t)rbp)) {
#ifdef CSA_DEBUG
		CDBG_ERROR(("csa_iosetup(0x%x,0x%x): pktp=0x%x failed\n",
			    CSAP2CSABLKP(csap)->cb_ioaddr,
			    csap->c_drive, pktp));
#endif
		return (NULL);
	}

/* ??? fix this, sector size should come from logical drive */
	rbp->rb_hdr.rh_blk_cnt = pktp->cp_bytexfer / 512;
	return (pktp);
}


static int
csa_transport(
	csa_t		*csap,
	struct cmpkt	*pktp
)
{
	csa_blk_t	*csa_blkp = CSAP2CSABLKP(csap);
	ccb_t		*ccbp = PKTP2CCBP(pktp);


	mutex_enter(&csa_blkp->cb_mutex);

	if (CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status == CCB_CBUSY)
		;
	else if (CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status == CCB_CDONE)
		CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_status = CCB_CBUSY;
#ifdef CSA_DEBUG
	else {
		CDBG_ERROR(("csa_iosetup(): 0x%x 0x%x\n", csap, pktp));
debug_enter("\n\ncsa_iosetup\n");
	}
#endif

	if (!csa_send_readwrite(csap, pktp, csa_blkp, ccbp)) {
#ifdef CSA_DEBUG
		CDBG_ERROR(("csa_iosetup(0x%x,0x%x): pktp=0x%x failed\n",
			    CSAP2CSABLKP(csap)->cb_ioaddr,
			    csap->c_drive, pktp));
#endif
		mutex_exit(&csa_blkp->cb_mutex);
		return (CTL_SEND_FAILURE);
	}


	if (!(pktp->cp_flags & CPF_NOINTR)) {
		mutex_exit(&csa_blkp->cb_mutex);
		return (CTL_SEND_SUCCESS);
	}

	/* pollret releases the mutex */
	if (!csa_pollret(csa_blkp, ccbp)) {
		CDBG_WARN(("?csa_transport(0x%x,0x%x): pollret failed, "
			   "pktp=0x%x\n", csa_blkp->cb_ioaddr,
			   csap->c_drive, pktp));
		pktp->cp_reason = CPS_CHKERR;
		mutex_exit(&csa_blkp->cb_mutex);
		return (CTL_SEND_SUCCESS);
	}
	mutex_exit(&csa_blkp->cb_mutex);

	/* run callback on the packet */
	if (pktp->cp_callback != NULL)
		(pktp->cp_callback)(pktp);

#ifdef CSA_DEBUG
	else
		CDBG_WARN(("?csa_transport(0x%x,0x%x): null callback, "
			   "pktp=0x%x\n", csa_blkp->cb_ioaddr,
			   csap->c_drive, pktp));
#endif CSA_DEBUG

	return (CTL_SEND_SUCCESS);
}


/* abort not implemented in the hardware */
/*ARGSUSED*/
static int
csa_abort(
	csa_t		*csap,
	struct cmpkt	*pktp)
{

#ifdef CSA_DEBUG
	debug_enter("\n\ncsa ABORT\n\n");
#endif

	return (TRUE);
}


/* reset not implemented in the hardware */
/*ARGSUSED*/
static int
csa_reset(
	csa_t	*csap,
	int	 level
)
{

#ifdef CSA_DEBUG
	debug_enter("\n\ncsa RESET\n\n");
#endif

	return (TRUE);
}


/*ARGSUSED*/
static int
csa_ioctl(
	csa_t	*csap,
	int	 cmd,
	int	 arg,
	int	 flag
)
{
	switch (cmd) {
	case DIOCTL_GETGEOM:
	case DIOCTL_GETPHYGEOM:
		if (!csa_get_ldgeom(csap, (struct tgdk_geom *)arg)) {
			return (ENXIO);
		}
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}


/*
 *	controller dependent functions
 */

static int
csa_propinit(
	dev_info_t	*dip,
	csa_blk_t	*csa_blkp
)
{
	int		val;
	int		len;

	len = sizeof (int);
	if (HBA_INTPROP(dip, "ioaddr", &val, &len) != DDI_PROP_SUCCESS)
		return (FALSE);
	csa_blkp->cb_ioaddr = (ushort)val;

	len = sizeof (int);
	if (HBA_INTPROP(dip, "nccbs", &val, &len) == DDI_PROP_SUCCESS)
		csa_blkp->cb_nccbs = val;
	else
		csa_blkp->cb_nccbs = CSA_MAX_CMDS;

	return (TRUE);
}


/*
 * This function is called with mutex held and releases it.
 * It returns TRUE on success, FALSE on failure
 */

Bool_t
csa_pollret(
	csa_blk_t	*csa_blkp,
	ccb_t		*poll_ccbp
)
{
	ccb_t		*ccbp;
	Que_t		 ccb_hold_queue = { NULL, NULL };
	struct cmpkt	*pktp;
	int		 status;
	int		 cntr;
	int		 got_it = FALSE;

	/* unqueue and save all ccbs until I find the right one */
	while (!got_it) {
		for (cntr = 0; cntr < 100000; cntr++) {
			if (csa_intr_status((opaque_t)csa_blkp,
					    (opaque_t *)&status))
				goto process_intr;
			drv_usecwait(1000);
		}
#ifdef CSA_DEBUG
debug_enter("\n\ncsa pollret failed\n");
#endif
		break;

	process_intr:
		csa_process_intr((opaque_t)csa_blkp, (opaque_t)status);

		/* unqueue all the completed requests, look for mine */
		while (ccbp = (ccb_t *)QueueRemove(&csa_blkp->cb_doneq)) {
			/* if it's my ccb, requeue the rest then return */
			if (ccbp == poll_ccbp) {
				got_it = TRUE;
				continue;
			}
			/* fifo queue the other ccbs on my local list */
			QueueAdd(&ccb_hold_queue, &ccbp->ccb_q, ccbp);
		}
	}

	if (QEMPTY(&ccb_hold_queue)) {
		return (got_it);
	}

	mutex_exit(&csa_blkp->cb_mutex);
	/* check for other completed packets that have been queued */
	while (ccbp = (ccb_t *)QueueRemove(&ccb_hold_queue)) {
		pktp = CCBP2PKTP(ccbp);
		if (pktp != NULL && pktp->cp_callback != NULL)
			/* and run the packet callback */
			(*pktp->cp_callback)(pktp);
	}
	mutex_enter(&csa_blkp->cb_mutex);
	return (got_it);
}


static uint
csa_intr(caddr_t arg)
{
	csa_blk_t	*csa_blkp;
	ushort		 ioaddr;
	ulong		 status;

	csa_blkp = (csa_blk_t *)arg;
	ioaddr = csa_blkp->cb_ioaddr;

	mutex_enter(&csa_blkp->cb_mutex);

	if (!csa_intr_status((opaque_t)csa_blkp, (opaque_t *)&status)) {
		mutex_exit(&csa_blkp->cb_mutex);
		return (DDI_INTR_UNCLAIMED);
	}

	CSA_BMIC_DISABLE(csa_blkp, ioaddr);
	common_intr((opaque_t)csa_blkp, (opaque_t)status, csa_process_intr,
		    csa_intr_status, &csa_blkp->cb_mutex, &csa_blkp->cb_doneq);
	CSA_BMIC_ENABLE(csa_blkp, ioaddr);

	mutex_exit(&csa_blkp->cb_mutex);
	return (DDI_INTR_CLAIMED);

}

void
csa_err(char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vcmn_err(CE_CONT, fmt, ap);
	va_end(ap);
}
