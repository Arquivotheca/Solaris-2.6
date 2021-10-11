/*
 * Copyright (c) 1992-1994, Sun Microsystems, Inc.
 */

#pragma ident	"@(#)eha.c	1.62	96/07/29 SMI"

#include <sys/scsi/scsi.h>
#include <sys/dktp/objmgr.h>
#include <sys/dktp/hba.h>
#include <sys/dktp/eha.h>
#include <sys/debug.h>

#include <sys/scsi/impl/pkt_wrapper.h>

/*
 * External SCSA Interface
 */

static int eha_tran_tgt_init(dev_info_t *, dev_info_t *, scsi_hba_tran_t *,
    struct scsi_device *);
static int eha_tran_tgt_probe(struct scsi_device *, int (*)());
static void eha_tran_tgt_free(dev_info_t *, dev_info_t *, scsi_hba_tran_t *,
    struct scsi_device *);
static int eha_transport(struct scsi_address *ap, struct scsi_pkt *pktp);
static int eha_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static int eha_reset(struct scsi_address *ap, int level);
static int eha_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static int eha_setcap(struct scsi_address *ap, char *cap, int value,
    int tgtonly);
static struct scsi_pkt *eha_tran_init_pkt(struct scsi_address *ap,
    struct scsi_pkt *pkt, struct buf *bp, int cmdlen, int statuslen,
    int tgtlen, int flags, int (*callback)(), caddr_t arg);
static void eha_tran_destroy_pkt(struct scsi_address *ap,
    struct scsi_pkt *pkt);
static struct scsi_pkt *eha_pktalloc(struct scsi_address *ap, int cmdlen,
    int statuslen, int tgtlen, int (*callback)(), caddr_t arg);
static void eha_pktfree(struct scsi_pkt *pkt);
static struct scsi_pkt *eha_dmaget(struct scsi_pkt *pkt, opaque_t dmatoken,
    int (*callback)(), caddr_t arg);
static void eha_tran_dmafree(struct scsi_address *ap, struct scsi_pkt *pktp);
static void eha_tran_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pktp);

/*
 * Local Function Prototypes
 */
static int eha_ccbinit(register struct  eha_blk *eha_blkp);
static int eha_capchk(char *cap, int tgtonly, int *cidxp);
static int eha_propinit(struct eha_blk *eha_blkp);
static int eha_cfginit(struct  eha_blk *eha_blkp);
static int eha_findhba(register uint ioaddr);
static int eha_setup_inq(struct eha_blk *eha_blkp);
static int eha_getedt(struct eha_blk *eha_blkp);
static int eha_init_cmd(register struct eha_blk *eha_blkp);
static int eha_reset_hba(unsigned int ioaddr, int target);
static u_int eha_intr(caddr_t arg);
static u_int eha_dummy_intr(caddr_t arg);
static u_int eha_xlate_vec(struct eha_blk *eha_blkp);
static void eha_send_cmd(struct eha_blk *eha_blkp, struct eha_ccb *ccbp);
static void eha_pollret(struct eha_blk *eha_blkp, struct scsi_pkt *pktp);
static void eha_chkerr(struct scsi_pkt *pktp, struct eha_ccb *ccbp);
static void eha_childprop(dev_info_t *mdip, dev_info_t *cdip);
static void eha_saveccb(struct eha_blk *eha_blkp, struct eha_ccb *ccbp);
static struct eha_ccb *eha_retccb(struct eha_blk *eha_blkp, unsigned long ai);
static struct scsi_pkt *eha_chkstatus(struct eha_blk *eha_blkp);

/*
 * Local static data
 */
static int eha_pgsz = 0;
static int eha_pgmsk;
static int eha_pgshf;

static int eha_cb_id = 0;
static caddr_t ehaccb = NULL;
static kmutex_t eha_rmutex;
static kmutex_t eha_global_mutex;
static int eha_global_init = 0;

static ddi_dma_lim_t eha_dma_lim = {
	0,		/* address low				*/
	(u_long)0xffffffff, /* address high			*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	(u_int)DMALIM_VER0, /* version				*/
	(u_long)0xffffffff, /* address register		*/
	0x003fffff,	/* counter max 				*/
	512,		/* sector size				*/
	EHA_MAX_DMA_SEGS, /* scatter/gather list length		*/
	(u_int)0xffffffff /* request size			*/
};


#ifdef EHA_DEBUG
/*
 *      eha_debug is used to turn on/off specific debug statements, it is a
 *              bit mapped value as follows:
 */
#define		DENTRY  0x01    /* data entry			*/
#define		DIO	0x02	/* io processing		*/

int eha_debug = 0;
#endif

static int eha_identify(dev_info_t *dev);
static int eha_probe(dev_info_t *);
static int eha_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int eha_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

struct dev_ops	eha_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	eha_identify,		/* identify */
	eha_probe,		/* probe */
	eha_attach,		/* attach */
	eha_detach,		/* detach */
	nulldev,		/* reset */
	(struct cb_ops *)0,	/* driver operations */
	NULL			/* bus operations */
};

/*
 * This is the loadable module wrapper.
 */
#include <sys/modctl.h>

char _depends_on[] = "misc/scsi drv/rootnex";

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module. This one is a driver */
	"Adaptec 174x SCSI Host Adapter Driver", /* Name of the module. */
	&eha_ops,		/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

int
_init(void)
{
	int	status;

	if ((status = scsi_hba_init(&modlinkage)) != 0) {
		return (status);
	}

	mutex_init(&eha_global_mutex, "EHA global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&eha_global_mutex);
	}
	return (status);
}

int
_fini(void)
{
	int	status;

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&eha_global_mutex);
	}
	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}




/*ARGSUSED*/
static int
eha_tran_tgt_init(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	int 	targ;
	int	lun;
	struct 	eha *hba_ehap;
	struct 	eha *unit_ehap;
	struct  scsi_inquiry *inqp;

	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef EHA_DEBUG
	cmn_err(CE_CONT, "%s%d: %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif

	if (targ < 0 || targ > 7 || lun < 0 || lun > 7) {
		cmn_err(CE_WARN, "%s%d: %s%d bad address <%d,%d>\n",
			ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
			ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
			targ, lun);
		return (DDI_FAILURE);
	}

	hba_ehap = SDEV2HBA(sd);
	inqp = &(hba_ehap->e_blkp->eb_inqp[targ<<3|lun]);

	if ((inqp->inq_rdf == RDF_SCSI2) && (inqp->inq_cmdque))
		eha_childprop(hba_dip, tgt_dip);

	unit_ehap = kmem_zalloc(
		sizeof (struct eha) + sizeof (struct eha_unit), KM_NOSLEEP);

	bcopy((caddr_t)hba_ehap, (caddr_t)unit_ehap, sizeof (*hba_ehap));
	unit_ehap->e_unitp = (struct eha_unit *)(unit_ehap+1);
	unit_ehap->e_unitp->eu_lim = eha_dma_lim;

/*	update xfer request size max for non-seq (disk) devices		*/

	if (inqp->inq_dtype == DTYPE_DIRECT)
		unit_ehap->e_unitp->eu_lim.dlim_reqsize = 0x8000;

	sd->sd_inq = inqp;

	hba_tran->tran_tgt_private = unit_ehap;

	EHA_BLKP(hba_ehap)->eb_child++;   /* increment active child cnt   */

#ifdef EHA_DEBUG
	if (eha_debug & DIO) {
		PRF("eha_tran_tgt_init: <%d,%d>\n", targ, lun);
	}
#endif
	return (DDI_SUCCESS);
}


/*ARGSUSED*/
static int
eha_tran_tgt_probe(
	struct scsi_device	*sd,
	int			(*callback)())
{
	int	rval;

	rval = scsi_hba_probe(sd, callback);

#ifdef EHA_DEBUG
	{
		char		*s;
		struct eha	*eha = SDEV2EHA(sd);

		switch (rval) {
		case SCSIPROBE_NOMEM:
			s = "scsi_probe_nomem";
			break;
		case SCSIPROBE_EXISTS:
			s = "scsi_probe_exists";
			break;
		case SCSIPROBE_NONCCS:
			s = "scsi_probe_nonccs";
			break;
		case SCSIPROBE_FAILURE:
			s = "scsi_probe_failure";
			break;
		case SCSIPROBE_BUSY:
			s = "scsi_probe_busy";
			break;
		case SCSIPROBE_NORESP:
			s = "scsi_probe_noresp";
			break;
		default:
			s = "???";
			break;
		}
		cmn_err(CE_CONT, "eha%d: %s target %d lun %d %s\n",
			ddi_get_instance(EHA_DIP(eha)),
			ddi_get_name(sd->sd_dev),
			sd->sd_address.a_target,
			sd->sd_address.a_lun, s);
	}
#endif	/* EHADEBUG */

	return (rval);
}


/*ARGSUSED*/
static void
eha_tran_tgt_free(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	struct eha		*eha;
	struct eha		*unit_ehap;

#ifdef	EHADEBUG
	cmn_err(CE_CONT, "eha_tran_tgt_free: %s%d %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif	/* EHADEBUG */

	unit_ehap = hba_tran->tran_tgt_private;
	kmem_free(unit_ehap,
		sizeof (struct eha) + sizeof (struct eha_unit));

	sd->sd_inq = NULL;

	eha = SDEV2HBA(sd);
	EHA_BLKP(eha)->eb_child--;
}

static void
eha_childprop(dev_info_t *mdip, dev_info_t *cdip)
{
	char	 que_keyvalp[OBJNAMELEN];
	int	 que_keylen;
	char	 flc_keyvalp[OBJNAMELEN];
	int	 flc_keylen;

	que_keylen = sizeof (que_keyvalp);
	if (ddi_prop_op(DDI_DEV_T_NONE, mdip, PROP_LEN_AND_VAL_BUF,
		DDI_PROP_CANSLEEP|DDI_PROP_DONTPASS, "tag_queue",
		(caddr_t)que_keyvalp, &que_keylen) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
			"eha_childprop: tagged queue property undefined");
		return;
	}
	que_keyvalp[que_keylen] = (char)0;

	flc_keylen = sizeof (flc_keyvalp);
	if (ddi_prop_op(DDI_DEV_T_NONE, mdip, PROP_LEN_AND_VAL_BUF,
		DDI_PROP_CANSLEEP|DDI_PROP_DONTPASS, "tag_fctrl",
		(caddr_t)flc_keyvalp, &flc_keylen) != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN,
		    "eha_childprop: tagged flow-control property undefined");
		return;
	}
	flc_keyvalp[flc_keylen] = (char)0;

	(void) ddi_prop_create(DDI_DEV_T_NONE, cdip, DDI_PROP_CANSLEEP,
		"queue", (caddr_t)que_keyvalp, que_keylen);
	(void) ddi_prop_create(DDI_DEV_T_NONE, cdip, DDI_PROP_CANSLEEP,
		"flow_control", (caddr_t)flc_keyvalp, flc_keylen);
}


/*
 *	Autoconfiguration routines
 */
static int
eha_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, "eha") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}


static int
eha_probe(register dev_info_t *devi)
{
	unsigned int	ioaddr;
	int	len;

	len = sizeof (int);
	if (HBA_INTPROP(devi, "ioaddr", &ioaddr, &len) != DDI_SUCCESS)
		return (DDI_PROBE_FAILURE);

	return (eha_findhba(ioaddr));
}

static int
eha_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	struct 	eha *eha;
	struct	eha_blk *eha_blkp;

	switch (cmd) {
	case DDI_DETACH:
	{
		scsi_hba_tran_t	*tran;

		tran = (scsi_hba_tran_t *)ddi_get_driver_private(devi);
		if (!tran)
			return (DDI_SUCCESS);
		eha = TRAN2EHA(tran);
		if (!eha)
			return (DDI_SUCCESS);
		eha_blkp = EHA_BLKP(eha);
		if (eha_blkp->eb_child)
			return (DDI_FAILURE);

		EHA_DISABLE_INTR(eha_blkp->eb_ioaddr);
		ddi_iopb_free((caddr_t)eha_blkp->eb_inqp);
		ddi_remove_intr(devi, eha_xlate_vec(eha_blkp),
			eha_blkp->eb_iblock);

		scsi_destroy_cbthread(eha_blkp->eb_cbthdl);
		mutex_destroy(&eha_blkp->eb_mutex);

		mutex_enter(&eha_global_mutex);
		eha_global_init--;
		if (eha_global_init == 0)
			mutex_destroy(&eha_rmutex);
		mutex_exit(&eha_global_mutex);

		(void) kmem_free((caddr_t)eha,
		    (sizeof (*eha) + sizeof (*eha_blkp)));

		ddi_prop_remove_all(devi);
		return (DDI_SUCCESS);
	}
	default:
		return (DDI_FAILURE);
	}
}

static int
eha_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	register struct eha 	*eha;
	register struct eha_blk	*eha_blkp;
	u_int			intr_idx;
	scsi_hba_tran_t		*hba_tran;

	switch (cmd) {
	case DDI_ATTACH:
		break;

	default:
		return (DDI_FAILURE);
	}

	/*
	 * Attach handling
	 */
	eha = kmem_zalloc((unsigned)(sizeof (*eha) +
		sizeof (*eha_blkp)), KM_NOSLEEP);
	if (!eha)
		return (DDI_FAILURE);
	eha_blkp = (struct eha_blk *)(eha + 1);
	EHA_BLKP(eha) = eha_blkp;
	eha_blkp->eb_dip = devi;

	if ((eha_propinit(eha_blkp) == DDI_FAILURE) ||
	    (eha_ccbinit(eha_blkp)  == DDI_FAILURE) ||
	    (eha_cfginit(eha_blkp)  == DDI_FAILURE) ||
	    (eha_getedt(eha_blkp)   == DDI_FAILURE)) {
		kmem_free(eha, (sizeof (*eha) + sizeof (*eha_blkp)));
		return (DDI_FAILURE);
	}

	/*
	 * Allocate a transport structure
	 */
	hba_tran = scsi_hba_tran_alloc(devi, 0);
	if (hba_tran == NULL) {
		cmn_err(CE_WARN, "eha_attach: scsi_hba_tran_alloc failed");
		kmem_free(eha, (sizeof (*eha) + sizeof (*eha_blkp)));
		return (DDI_FAILURE);
	}

	intr_idx = eha_xlate_vec(eha_blkp);
/*
 *	Establish initial dummy interrupt handler
 *	get iblock cookie to initialize mutexes used in the
 *	real interrupt handler
 */
	if (ddi_add_intr(devi, intr_idx,
	    (ddi_iblock_cookie_t *)&eha_blkp->eb_iblock,
	    (ddi_idevice_cookie_t *)0, eha_dummy_intr, (caddr_t)eha)) {
		cmn_err(CE_WARN, "eha_attach: cannot add intr");
		kmem_free(eha, (sizeof (*eha) + sizeof (*eha_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	mutex_init(&eha_blkp->eb_mutex, "eha mutex", MUTEX_DRIVER,
		eha_blkp->eb_iblock);

	ddi_remove_intr(devi, intr_idx, eha_blkp->eb_iblock);
/*	Establish real interrupt handler				*/
	if (ddi_add_intr(devi, intr_idx,
			(ddi_iblock_cookie_t *)&eha_blkp->eb_iblock,
			(ddi_idevice_cookie_t *)0, eha_intr,
			(caddr_t)eha)) {
		cmn_err(CE_WARN, "eha_attach: cannot add intr");
		mutex_destroy(&eha_blkp->eb_mutex);
		kmem_free(eha, (sizeof (*eha) + sizeof (*eha_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	eha_blkp->eb_cbthdl = scsi_create_cbthread(eha_blkp->eb_iblock,
		KM_NOSLEEP);
	if (eha_blkp->eb_cbthdl == NULL) {
		cmn_err(CE_WARN, "eha_attach: cannot create cbthread");
		ddi_remove_intr(devi, intr_idx, eha_blkp->eb_iblock);
		mutex_destroy(&eha_blkp->eb_mutex);
		kmem_free(eha, (sizeof (*eha) + sizeof (*eha_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	hba_tran->tran_hba_private	= eha;
	hba_tran->tran_tgt_private	= NULL;

	hba_tran->tran_tgt_init		= eha_tran_tgt_init;
	hba_tran->tran_tgt_probe	= eha_tran_tgt_probe;
	hba_tran->tran_tgt_free		= eha_tran_tgt_free;

	hba_tran->tran_start 		= eha_transport;
	hba_tran->tran_abort		= eha_abort;
	hba_tran->tran_reset		= eha_reset;
	hba_tran->tran_getcap		= eha_getcap;
	hba_tran->tran_setcap		= eha_setcap;
	hba_tran->tran_init_pkt 	= eha_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= eha_tran_destroy_pkt;
	hba_tran->tran_dmafree		= eha_tran_dmafree;
	hba_tran->tran_sync_pkt		= eha_tran_sync_pkt;

	if (scsi_hba_attach(devi, &eha_dma_lim, hba_tran,
			SCSI_HBA_TRAN_CLONE, NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "eha_attach: scsi_hba_attach failed");
		scsi_destroy_cbthread(eha_blkp->eb_cbthdl);
		ddi_remove_intr(devi, intr_idx, eha_blkp->eb_iblock);
		mutex_destroy(&eha_blkp->eb_mutex);
		kmem_free(eha, (sizeof (*eha) + sizeof (*eha_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	mutex_enter(&eha_global_mutex);	/* protect multithreaded attach	*/
	if (!eha_global_init) {
		mutex_init(&eha_rmutex, "EHA Resource Mutex", MUTEX_DRIVER,
			eha_blkp->eb_iblock);
	}
	eha_global_init++;
	mutex_exit(&eha_global_mutex);

	ddi_report_dev(devi);

	EHA_ENABLE_INTR(eha_blkp->eb_ioaddr);
	return (DDI_SUCCESS);
}


static int
eha_propinit(register struct eha_blk *eha_blkp)
{
	register dev_info_t *devi;
	int	i;
	int	val;
	int	len;
	ushort	ioaddr;

	devi = eha_blkp->eb_dip;
	len = sizeof (int);
	if (HBA_INTPROP(devi, "ioaddr", &val, &len) != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);
	ioaddr   = (ushort)val;

	eha_blkp->eb_ioaddr    = ioaddr;
	eha_blkp->eb_mbi_lsb   = ioaddr + EHA_MBI_LSB;
	eha_blkp->eb_mbo_lsb   = ioaddr + EHA_MBO_LSB;
	eha_blkp->eb_attention = ioaddr + EHA_ATTENTION;
	eha_blkp->eb_scsidef   = ioaddr + EHA_SCSIDEF;
	eha_blkp->eb_status1   = ioaddr + EHA_STATUS1;
	eha_blkp->eb_intrport  = ioaddr + EHA_INTERRUPT;
	eha_blkp->eb_control   = ioaddr + EHA_CONTROL;

	mutex_enter(&eha_global_mutex);
	if (!eha_pgsz) {
		eha_pgsz = ddi_ptob(devi, 1L);
		eha_pgmsk = eha_pgsz - 1;
		for (i = eha_pgsz, len = 0; i > 1; len++)
			i >>= 1;
		eha_pgshf = len;
	}
	mutex_exit(&eha_global_mutex);

	return (DDI_SUCCESS);
}

static int
eha_transport(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct eha_blk *eha_blkp;
	register struct	eha_ccb *ccbp;

	ccbp = (struct eha_ccb *)SCMD_PKTP(pktp)->cmd_private;
	eha_blkp = ADDR2EHABLKP(ap);

/*	handle generic request sense if ARQ is off			*/
	if (*(pktp->pkt_cdbp) == SCMD_REQUEST_SENSE &&
			(!(ccbp->ccb_flag1 & ARS))) {
				ccbp->ccb_cmdword = READ_SENSE_CMD;
				ccbp->ccb_cdblen = 0;
	}

	mutex_enter(&eha_blkp->eb_mutex);
/* 	put this ccb on linked list of outstanding ccbs 		*/
	eha_saveccb(eha_blkp, ccbp);

	eha_send_cmd(eha_blkp, ccbp);

	if (pktp->pkt_flags & FLAG_NOINTR)
		eha_pollret(eha_blkp, pktp);

	mutex_exit(&eha_blkp->eb_mutex);
	return (TRAN_ACCEPT);
}

/* Abort specific command on target device */
/*ARGSUSED*/
static int
eha_abort(struct scsi_address *ap, struct scsi_pkt *pktp)
{
	register struct	eha_blk *eha_blkp;
	register struct eha_ccb *ccbp;
	struct  eha_ccb *rccbp;

	if (!pktp)
		return (1);

	eha_blkp = PKT2EHABLKP(pktp);
	ccbp = (struct eha_ccb *)SCMD_PKTP(pktp)->cmd_private;

	mutex_enter(&eha_blkp->eb_mutex);
	rccbp = eha_retccb(eha_blkp, ccbp->ccb_paddr);
	mutex_exit(&eha_blkp->eb_mutex);


	if (!rccbp) {
#ifdef EHA_DEBUG
		PRF("eha_abort: called for command not outstanding\n");
#endif
		return (TRUE);
	}

	if (ccbp != rccbp) {
#ifdef EHA_DEBUG
		PRF("eha_abort: bad ccbp 0x%x rccbp 0x%x\n", ccbp, rccbp);
#endif
		return (FALSE);
	}

	mutex_enter(&eha_blkp->eb_mutex);

/* 	put address of ccb in mbxout register 				*/
	outl(eha_blkp->eb_mbo_lsb, ccbp->ccb_paddr);

/* 	put target id + CCB_ABORT into ATN register 			*/
	outb(eha_blkp->eb_attention, CCB_ABORT | ccbp->ccb_target);

	mutex_exit(&eha_blkp->eb_mutex);

	return (TRUE);
}

/* reset the scsi bus, or just one target device */
static int
eha_reset(struct scsi_address *ap, int level)
{
	register struct	eha_blk *eha_blkp;

	eha_blkp = ADDR2EHABLKP(ap);
	mutex_enter(&eha_blkp->eb_mutex);

	switch (level) {
		case RESET_ALL:
			outl(eha_blkp->eb_mbo_lsb, IMMED_RESET);
			outb(eha_blkp->eb_attention,
				IMMEDIATE_CMD | eha_blkp->eb_targetid);
			break;

		case RESET_TARGET:
			outl(eha_blkp->eb_mbo_lsb, IMMED_RESET);
			outb(eha_blkp->eb_attention,
				IMMEDIATE_CMD | ap->a_target);
			break;

		default:
#ifdef EHA_DEBUG
			if (eha_debug & DIO)
				PRF("eha_reset: bad level %d\n", level);
#endif
			return (FALSE);
	}

	mutex_exit(&eha_blkp->eb_mutex);

	return (TRUE);
}

static int
eha_capchk(char *cap, int tgtonly, int *cidxp)
{
	if ((tgtonly != 0 && tgtonly != 1) || cap == NULL)
		return (FALSE);

	*cidxp = scsi_hba_lookup_capstr(cap);
	return (TRUE);
}

static int
eha_getcap(struct scsi_address *ap, char *cap, int tgtonly)
{
	int	ckey;

	if (eha_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {
		case SCSI_CAP_GEOMETRY:
			return (HBA_SETGEOM(64, 32));
		case SCSI_CAP_ARQ:
			return ((ADDR2EHAUNITP(ap))->eu_arq);
		case SCSI_CAP_TAGGED_QING:
			return ((ADDR2EHAUNITP(ap))->eu_tagque);
		default:
			return (UNDEFINED);
	}
}

static int
eha_setcap(struct scsi_address *ap, char *cap, int value, int tgtonly)
{
	int	ckey, status = FALSE;

	if (eha_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {
		case SCSI_CAP_TAGGED_QING:
			if (tgtonly) {
				(ADDR2EHAUNITP(ap))->eu_tagque = (u_int)value;
				status =  TRUE;
			}
			break;

		case SCSI_CAP_ARQ:
			if (tgtonly) {
				(ADDR2EHAUNITP(ap))->eu_arq = (u_int)value;
				status =  TRUE;
			}
			break;

		case SCSI_CAP_SECTOR_SIZE:
			(ADDR2EHAUNITP(ap))->eu_lim.dlim_granular =
			    (u_int)value;
			status =  TRUE;
			break;

		default:
			break;
	}

	return (status);
}

static struct scsi_pkt *
eha_tran_init_pkt(struct scsi_address *ap, struct scsi_pkt *pkt,
	struct buf *bp, int cmdlen, int statuslen, int tgtlen,
	int flags, int (*callback)(), caddr_t arg)
{
	struct scsi_pkt		*new_pkt = NULL;

	/*
	 * Allocate a pkt
	 */
	if (!pkt) {
		pkt = eha_pktalloc(ap, cmdlen, statuslen,
			tgtlen, callback, arg);
		if (pkt == NULL)
			return (NULL);
		((struct scsi_cmd *)pkt)->cmd_flags = flags;
		new_pkt = pkt;
	} else {
		new_pkt = NULL;
	}

	/*
	 * Set up dma info
	 */
	if (bp) {
		if (eha_dmaget(pkt, (opaque_t)bp, callback, arg) == NULL) {
			if (new_pkt)
				eha_pktfree(new_pkt);
			return (NULL);
		}
	}

	return (pkt);
}

static void
eha_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	eha_tran_dmafree(ap, pkt);
	eha_pktfree(pkt);
}


static struct scsi_pkt *
eha_pktalloc(struct scsi_address *ap, int cmdlen, int statuslen,
    int tgtlen, int (*callback)(), caddr_t arg)
{
	register struct scsi_cmd *cmd;
	register struct eha_ccb	*ccbp;
	struct	 eha_blk	*eha_blkp;
	caddr_t	 buf;
	paddr_t	 ccb_paddr;
	caddr_t	 ccb_caddr;
	int	 kf;
	caddr_t	tgt;

	eha_blkp = ADDR2EHABLKP(ap);
	kf = HBA_KMFLAG(callback);

	/*
	 * Allocate target-private data, if necessary
	 */
	if (tgtlen > PKT_PRIV_LEN) {
		tgt = kmem_zalloc(tgtlen, kf);
		if (!tgt) {
			ASSERT(callback != SLEEP_FUNC);
			if (callback != NULL_FUNC)
				ddi_set_callback(callback, arg, &eha_cb_id);
			return ((struct scsi_pkt *)NULL);
		}
	} else {
		tgt = NULL;
	}

/*	allocate common packet						*/
	cmd = kmem_zalloc(sizeof (*cmd), kf);
	mutex_enter(&eha_rmutex);
	if (cmd) {
/* 		allocate ccb 						*/
		if (scsi_iopb_fast_zalloc(&ehaccb, eha_blkp->eb_dip,
			(ddi_dma_lim_t *)0, (u_int)(sizeof (*ccbp)), &buf)) {
			kmem_free(cmd, sizeof (*cmd));
			cmd = NULL;
		}
	}
	mutex_exit(&eha_rmutex);

	if (!cmd) {
		if (tgt)
			kmem_free(tgt, tgtlen);
		if (callback != DDI_DMA_DONTWAIT)
			ddi_set_callback(callback, arg, &eha_cb_id);
		return ((struct scsi_pkt *)NULL);
	}

/* 	initialize ccb 							*/
	ccbp = (struct eha_ccb *)buf;
	ccbp->ccb_cdblen = (unchar)cmdlen;
	ccbp->ccb_ownerp = cmd;

	ccbp->ccb_cmdword = DO_SCSI_CMD;
	if ((ADDR2EHAUNITP(ap))->eu_arq)
		ccbp->ccb_flag1  = DSBLK+ARS;
	else
		ccbp->ccb_flag1  = DSBLK;
	ccbp->ccb_flag2  = ap->a_lun;
	ccbp->ccb_target = ap->a_target;

	ccb_caddr	= (caddr_t)ccbp;
	ccb_paddr	= EHA_KVTOP(ccbp);
	ccbp->ccb_paddr	= ccb_paddr;

/* 	pointer to hardware completion status area 			*/
	ccbp->ccb_statp  =  (paddr_t)(ccb_paddr + ((caddr_t)&ccbp->ccb_stat -
						ccb_caddr));

/* 	auto request sense data 					*/
	ccbp->ccb_sensep   = (paddr_t)(ccb_paddr + ((caddr_t)
			(&ccbp->ccb_sense.sts_sensedata) - ccb_caddr));
	ccbp->ccb_senselen = EHA_SENSE_LEN;

	/*
	 * disconnects will be enabled, if appropriate,
	 * after the pkt_flags bit is set
	 */

/* 	prepare the packet for normal command 				*/
	cmd->cmd_private	= (opaque_t)ccbp;
	cmd->cmd_pkt.pkt_cdbp	= (opaque_t)ccbp->ccb_cdb;
	cmd->cmd_cdblen		= (u_char)cmdlen;

	cmd->cmd_pkt.pkt_scbp	= (u_char *)&ccbp->ccb_sense;
	cmd->cmd_scblen 	= (u_char)statuslen;
	cmd->cmd_pkt.pkt_address = *ap;

	/*
	 * Set up target-private data
	 */
	cmd->cmd_privlen = (u_char)tgtlen;
	if (tgtlen > PKT_PRIV_LEN) {
		cmd->cmd_pkt.pkt_private = tgt;
	} else if (tgtlen > 0) {
		cmd->cmd_pkt.pkt_private = cmd->cmd_pkt_private;
	}

#ifdef EHA_DEBUG2
	if (eha_debug & DIO) {
		PRF("eha_pktalloc:cmdpktp= 0x%x pkt_cdbp=0x%x pkt_sdbp=0x%x\n",
			cmd, cmd->cmd_pkt.pkt_cdbp, cmd->cmd_pkt.pkt_scbp);
		PRF("ccbp= 0x%x\n", ccbp);
	}
#endif
	return ((struct scsi_pkt *)cmd);
}

/*
 * packet free
 */
void
eha_pktfree(register struct scsi_pkt *pkt)
{
	register struct scsi_cmd *cmd = (struct scsi_cmd *)pkt;

	if (cmd->cmd_privlen > PKT_PRIV_LEN) {
		kmem_free(pkt->pkt_private, cmd->cmd_privlen);
	}

	mutex_enter(&eha_rmutex);
/*	deallocate the ccb						*/
	if (cmd->cmd_private)
		scsi_iopb_fast_free(&ehaccb, (caddr_t)cmd->cmd_private);
	mutex_exit(&eha_rmutex);
/*	free the common packet						*/
	kmem_free(cmd, sizeof (*cmd));

	if (eha_cb_id)
		ddi_run_callback(&eha_cb_id);
}

/*
 * Dma resource allocation
 */

static struct scsi_pkt *
eha_dmaget(struct scsi_pkt *pktp, opaque_t dmatoken, int (*callback)(),
    caddr_t arg)
{
	struct buf *bp = (struct buf *)dmatoken;
	register struct scsi_cmd *cmd = (struct scsi_cmd *)pktp;
	register struct eha_ccb *ccbp;
	register struct sg_element *dmap;
	ddi_dma_cookie_t dmack;
	ddi_dma_cookie_t *dmackp = &dmack;
	int		cnt;
	int		bxfer;
	off_t		offset;
	off_t		len;

	ccbp = (struct eha_ccb *)cmd->cmd_private;
/*	check for scsi bus disconnect (Adaptec man page 6-7)		*/
	if (pktp->pkt_flags & FLAG_NODISCON)
		ccbp->ccb_flag2 |= FLAG2ND;

	if (!bp->b_bcount) {
		cmd->cmd_pkt.pkt_resid = 0;

/* 		turn off direction flags 				*/
		ccbp->ccb_flag2 &= ~(DIR|DAT);
/* 		do not report data underrun as error 			*/
		ccbp->ccb_flag1 |= SES;

		return (pktp);
	}

/* 	check direction for data transfer 				*/
	if (bp->b_flags & B_READ) {
		ccbp->ccb_flag2 |= (DIR | DAT);
		cmd->cmd_cflags &= ~CFLAG_DMASEND;
	} else {
		ccbp->ccb_flag2 |= DAT;
		cmd->cmd_cflags |= CFLAG_DMASEND;
	}

/*	setup dma memory and position to the next xfer segment		*/
	if (!scsi_impl_dmaget(pktp, (opaque_t)bp, callback, arg,
		&(PKT2EHAUNITP(pktp)->eu_lim)))
		return (NULL);

	ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len, dmackp);

/* 	check for one single block transfer 				*/
	if (bp->b_bcount <= dmackp->dmac_size) {
/* 		use non-scatter gather 					*/

		ccbp->ccb_datap    = dmackp->dmac_address;
		ccbp->ccb_datalen  = bp->b_bcount;
		pktp->pkt_resid = 0;
		cmd->cmd_totxfer = bp->b_bcount;

	} else {
/* 		use scatter-gather transfer 				*/
		ccbp->ccb_flag1 |= SG;


/* 		set address of scatter gather segs 			*/
		dmap = ccbp->ccb_sg_list;

		for (bxfer = 0, cnt = 1; ; cnt++, dmap++) {
			bxfer += dmackp->dmac_size;

			dmap->data_len = (ulong) dmackp->dmac_size;
			dmap->data_ptr = (ulong) dmackp->dmac_address;

/*			check for end of list condition			*/
			if (bp->b_bcount == (bxfer + cmd->cmd_totxfer))
				break;
			ASSERT(bp->b_bcount > (bxfer + cmd->cmd_totxfer));
/*			check end of physical scatter-gather list limit	*/
			if (cnt >= EHA_MAX_DMA_SEGS)
				break;
			if (bxfer >= (PKT2EHAUNITP(pktp)->eu_lim.dlim_reqsize))
				break;
			if (ddi_dma_nextseg(cmd->cmd_dmawin, cmd->cmd_dmaseg,
				&cmd->cmd_dmaseg) != DDI_SUCCESS)
				break;
			ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len,
			    dmackp);
		}
		ccbp->ccb_datalen = (ulong) (cnt*sizeof (*dmap));

/* 		physical address of scatter gather list 		*/
		ccbp->ccb_datap = (paddr_t)(ccbp->ccb_paddr +
			((caddr_t)ccbp->ccb_sg_list - (caddr_t)ccbp));

		cmd->cmd_totxfer += bxfer;
		pktp->pkt_resid = bp->b_bcount - cmd->cmd_totxfer;
	}
	return (pktp);
}




/*
 * Dma resource deallocation
 */
/*ARGSUSED*/
static void
eha_tran_dmafree(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct	scsi_cmd *cmd = (struct scsi_cmd *)pktp;

/* 	Free the mapping.  						*/
	if (cmd->cmd_dmahandle) {
		ddi_dma_free(cmd->cmd_dmahandle);
		cmd->cmd_dmahandle = NULL;
	}
}


/*ARGSUSED*/
static void
eha_tran_sync_pkt(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register int i;
	register struct	scsi_cmd *cmd = (struct scsi_cmd *)pktp;

	if (cmd->cmd_dmahandle) {
		i = ddi_dma_sync(cmd->cmd_dmahandle, 0, 0,
			(cmd->cmd_cflags & CFLAG_DMASEND) ?
			DDI_DMA_SYNC_FORDEV : DDI_DMA_SYNC_FORCPU);
		if (i != DDI_SUCCESS) {
			cmn_err(CE_WARN, "eha: sync pkt failed\n");
		}
	}
}


static int
eha_ccbinit(register struct  eha_blk *eha_blkp)
{
	register struct eha_ccb *ccbp;
	register paddr_t	ccb_paddr;
	int		mem;
	caddr_t 	buffer;

	mem = sizeof (*ccbp) + EHA_INQUIRY_DATA_LEN;

/* 	allocate ccb for hba specific command 				*/
	if (ddi_iopb_alloc(eha_blkp->eb_dip, (ddi_dma_lim_t *)0,
		mem, &buffer)) {
		return (DDI_FAILURE);
	}

	bzero((caddr_t)buffer, mem);
	ccbp = (struct eha_ccb *)buffer;

	ccb_paddr	  = (paddr_t)EHA_KVTOP(ccbp);
	ccbp->ccb_paddr	  = ccb_paddr;
	ccbp->ccb_cmdword = HA_INQ_CMD;	/* Read hba inquiry data	*/
	ccbp->ccb_flag1   = SES;
	ccbp->ccb_datalen = EHA_INQUIRY_DATA_LEN;
	ccbp->ccb_datap   = ccb_paddr + sizeof (*ccbp);
	ccbp->ccb_statp   = (paddr_t)(ccb_paddr + ((caddr_t)&ccbp->ccb_stat -
					(caddr_t)ccbp));
/* 	auto request sense data 					*/
	ccbp->ccb_sensep   = (paddr_t)(ccb_paddr + ((caddr_t)
				&ccbp->ccb_sense.sts_sensedata-(caddr_t)ccbp));
	ccbp->ccb_senselen = EHA_SENSE_LEN;

/* 	save size so ccb can be freed at end of eha_getedt 		*/
	eha_blkp->eb_ccb_cnt = mem;
	eha_blkp->eb_ccbp    = ccbp;

#ifdef EHA_DEBUG
	if (eha_debug & DIO) {
		PRF("eha_ccbinit: ccbp=0x%x\n", ccbp);
	}
#endif
	return (DDI_SUCCESS);
}

static u_int
eha_xlate_vec(register struct  eha_blk *eha_blkp)
{
	static u_char eha_vec[] = {9, 10, 11, 12, 13, 14, 15};
	register int i;
	register u_char vec;

	vec = eha_blkp->eb_intr;
	for (i = 0; i < (sizeof (eha_vec)/sizeof (u_char)); i++) {
		if (eha_vec[i] == vec)
			return ((u_int)i);
	}
	return ((u_int)-1);
}

/*
 *	Configuration initialization
 */
static int
eha_cfginit(register struct  eha_blk *eha_blkp)
{
	char 	*info;

/* 	save the SCSI ID of the adapter 				*/
	eha_blkp->eb_targetid = inb(eha_blkp->eb_scsidef) & SCSI_ID_BITS;

/*	save and check interrupt vector number				*/
	eha_blkp->eb_intr = (inb(eha_blkp->eb_ioaddr + EHA_INTDEF)
	    & EHA_INTBITS) + EHA_INT2IRQ;
	if (eha_xlate_vec(eha_blkp) == (u_int)-1) {
		return (DDI_FAILURE);
	}

	if (eha_init_cmd(eha_blkp)) {
		eha_blkp->eb_q_siz = EHA_MAX_CCBS_OUT;	/* error case	*/
	} else {
/*		get queue size						*/
		info = (char *)(eha_blkp->eb_ccbp + 1);
		eha_blkp->eb_q_siz = (short)info[6];
	}

#ifdef EHA_DEBUG
	if (eha_debug & DIO) {
		PRF("eha_cfginit: ccb que size %d\n", eha_blkp->eb_q_siz);
	}
#endif
	return (DDI_SUCCESS);
}

/*
 * Returns DDI_SUCCESS if found a controller, else DDI_FAILURE
 */
static int
eha_findhba(register uint ioaddr)
{
	union 	{
		unsigned long prodid;
		unchar product_id[4];
	} a174x_id;
	unsigned int ha_id;

	/* Try to find 174x at user supplied address */
	a174x_id.prodid = inl(ioaddr + HID0);

	/*
	 * "ADP" encrypted in first 2 bytes of product ID is:
	 *  04h (byte 0), 90h (byte 1)
	 */

	if (!((a174x_id.product_id[0] == 0x4) &&
	    ((unchar)a174x_id.product_id[1] == 0x90) &&
	    (((unchar)a174x_id.product_id[2] & 0xf0) == 0))) {
#ifdef EHA_DEBUG
		if (eha_debug & DIO)
			PRF("eha_findhba fail: not our ID\n");
#endif
		return (DDI_PROBE_FAILURE);
	}

	/* If we're not in enhanced mode, let aha driver handle it */
	if (!(inb(ioaddr + EHA_MODE) & INTERFACE_1740)) {
#ifdef EHA_DEBUG
		if (eha_debug & DIO)
			PRF("eha_findhba failed - not in 1740 mode\n");
#endif
		return (DDI_PROBE_FAILURE);
	}

	EHA_DISABLE_INTR(ioaddr);
	ha_id = inb(ioaddr | EHA_SCSIDEF) & SCSI_ID_BITS;
	if (!eha_reset_hba(ioaddr, ha_id))
		return (DDI_PROBE_FAILURE);

	return (DDI_PROBE_SUCCESS);
}

/*
 * Reset the adapter
 * Returns 1 on success, 0 on failure
 */

static int
eha_reset_hba(unsigned int ioaddr, int target)
{
	register int 	i;

#ifdef HARD_RESET_PATH
/*	do hard reset							*/
	outb(ioaddr | EHA_CONTROL, HARD_RESET);
	drv_usecwait(20);
	outb(ioaddr | EHA_CONTROL, 0);
	drv_usecwait(2*SEC_INUSEC);
#else
/*	do soft reset							*/
	outl(ioaddr + EHA_MBO_LSB, IMMED_RESET);
	outb(ioaddr + EHA_ATTENTION, IMMEDIATE_CMD | target);
	drv_usecwait(2*SEC_INUSEC);

	i = 500000;
	while (!((inb(ioaddr | EHA_STATUS1)) & INT_PENDING)) {
		drv_usecwait(10);
		if (--i <= 0)
			break;
	}

	if ((i <= 0) ||
	    ((inb(ioaddr + EHA_INTERRUPT) & INTSTAT_BITS) != IMMEDIATE_DONE)) {
		inb(ioaddr + EHA_MBI_LSB);
		return (0);
	}

	/* clear adapter interrupt 					*/
	outb(ioaddr | EHA_CONTROL, CLEAR_INT);
#endif

	return (1);
}


/*
 *	prepare for device inquiry
 */
static int
eha_setup_inq(register struct eha_blk *eha_blkp)
{
	register int	mem;
	caddr_t 	buf;
	struct	eha_ccb *ccbp;

	mem = sizeof (struct scsi_inquiry) * (HBA_MAX_ATT_DEVICES + 8);

	if (ddi_iopb_alloc(eha_blkp->eb_dip, (ddi_dma_lim_t *)0, (u_int)mem,
	    &buf)) {
		return (DDI_FAILURE);
	}
	bzero(buf, mem);
	eha_blkp->eb_inqp = (struct scsi_inquiry *)buf;

	ccbp = eha_blkp->eb_ccbp;
	ccbp->ccb_cmdword = DO_SCSI_CMD;
	ccbp->ccb_flag1   = (SES | DSBLK | ARS);
	ccbp->ccb_datalen = sizeof (struct scsi_inquiry);
	ccbp->ccb_datap   = 0;

/* 	set up scsi cdb 						*/
	ccbp->ccb_cdblen  = 6;
	bzero((caddr_t)ccbp->ccb_cdb, 6);
	ccbp->ccb_cdb[0] = SCMD_INQUIRY;
	ccbp->ccb_cdb[4] = sizeof (struct scsi_inquiry);

	return (DDI_SUCCESS);
}

/*
 *	Get equipment description table
 */
static int
eha_getedt(struct eha_blk *eha_blkp)
{
	register struct	eha_ccb *ccbp;
	register int	lun;
	register int	targ;
	struct 	scsi_inquiry *inqp;

	if (eha_setup_inq(eha_blkp) == DDI_FAILURE)
		return (DDI_FAILURE);

	eha_blkp->eb_numdev = 0;
	inqp = eha_blkp->eb_inqp;
	ccbp = eha_blkp->eb_ccbp;

	for (targ = 0; targ < 8; targ++) {
/*		check for hba id (myself)				*/
		if (targ == eha_blkp->eb_targetid) {
			for (lun = 0; lun < 8; lun++) {
				inqp = (struct scsi_inquiry *)
				    (&eha_blkp->eb_inqp[targ<<3|lun]);
				inqp->inq_dtype = DTYPE_NOTPRESENT;
			}
			continue;
		}

		ccbp->ccb_target = (unsigned char) targ;
		for (lun = 0; lun < 8; lun++) {
			inqp = (struct scsi_inquiry *)
			    (&eha_blkp->eb_inqp[targ<<3|lun]);

			ccbp->ccb_datap = EHA_KVTOP(inqp);
			ccbp->ccb_flag2 = DIR | DAT | lun;
			ccbp->ccb_cdb[1] = lun << 5;

			if (eha_init_cmd(eha_blkp)) {
				/* Inquiry Command failed */
				inqp->inq_dtype = DTYPE_NOTPRESENT;
			} else {
				if ((inqp->inq_dtype != DTYPE_NOTPRESENT) &&
				(inqp->inq_dtype != DTYPE_UNKNOWN)) {
					eha_blkp->eb_numdev++;
				}
			}

		}
	}

	/* free ccb */
	ddi_iopb_free((caddr_t)ccbp);

#ifdef EHA_DEBUG
	if (eha_debug & DENTRY)
		PRF("eha_getedt:  numdev= %d\n", eha_blkp->eb_numdev);
#endif

	return (DDI_SUCCESS);
}

static struct scsi_pkt *
eha_chkstatus(register struct eha_blk *eha_blkp)
{
	register struct eha_ccb *ccbp;
	register struct scsi_pkt *pktp;
	paddr_t  ccb_paddr;
	u_char	 status;

/* 	read command status 						*/
	status = inb(eha_blkp->eb_intrport) & INTSTAT_BITS;
	switch (status) {

		case CCB_DONE:
		case CCB_RETRIED:
			/* extract physical address of ccb */
			ccb_paddr = (paddr_t)inl(eha_blkp->eb_mbi_lsb);
			ccbp = eha_retccb(eha_blkp, ccb_paddr);
			if (!ccbp)
				return (NULL);
			pktp = (struct scsi_pkt *)ccbp->ccb_ownerp;
			*pktp->pkt_scbp  = STATUS_GOOD;
			pktp->pkt_reason = CMD_CMPLT;
			pktp->pkt_resid  = 0;
			pktp->pkt_state = (STATE_XFERRED_DATA | STATE_GOT_BUS |
			    STATE_GOT_TARGET | STATE_SENT_CMD |
			    STATE_GOT_STATUS);
			return (pktp);

		case HBA_HW_FAILED:
		case CCB_DONE_ERROR:
			ccb_paddr = (paddr_t)inl(eha_blkp->eb_mbi_lsb);
			ccbp = eha_retccb(eha_blkp, ccb_paddr);
			if (!ccbp)
				return (NULL);

			pktp = (struct scsi_pkt *)ccbp->ccb_ownerp;
			eha_chkerr(pktp, ccbp);
			return (pktp);

		case ASYNC_EVENT:
		case IMMEDIATE_DONE_ERR:
		default:
			return ((struct scsi_pkt *)NULL);
	}
}

/* poll for status of a command sent to hba without interrupts 		*/
static void
eha_pollret(register struct eha_blk *eha_blkp, struct scsi_pkt *poll_pktp)
{
	register struct	scsi_pkt *pktp;
	register struct	scsi_cmd *cmd;
	struct	scsi_cmd *cmd_hdp = (struct  scsi_cmd *)NULL;
	int    i;

	for (;;) {
/* 		Wait for Command Complete "Interrupt"			*/
		for (i = 500000;
		    !(inb(eha_blkp->eb_status1) & INT_PENDING);
		    /* null loop-end statement */) {
			drv_usecwait(10);
			if (--i == 0) {
				poll_pktp->pkt_reason = CMD_INCOMPLETE;
				poll_pktp->pkt_state  = 0;
				break;
			}
		}

		pktp = eha_chkstatus(eha_blkp);
		outb(eha_blkp->eb_control, CLEAR_INT);
		if (!pktp)
			continue;

/*		check for polled packet returned			*/
		if (pktp == poll_pktp)
			break;
/*
 *		chained up all completed packets
 *		until the polled packet returns
 */
		((struct scsi_cmd *)pktp)->cmd_cblinkp = NULL;
		if (!cmd_hdp)
			cmd_hdp = (struct scsi_cmd *)pktp;
		else {
			for (cmd = cmd_hdp; cmd->cmd_cblinkp;
			    cmd = cmd->cmd_cblinkp)
				;
			cmd->cmd_cblinkp = (struct scsi_cmd *)pktp;
		}
	}

/*	check for other completed packets that have been queued		*/
	if (cmd_hdp) {
		mutex_exit(&eha_blkp->eb_mutex);
		while ((cmd = cmd_hdp) != NULL) {
			cmd_hdp = cmd->cmd_cblinkp;
			scsi_run_cbthread(eha_blkp->eb_cbthdl, cmd);
		}
		mutex_enter(&eha_blkp->eb_mutex);
	}
}

/* Autovector Interrupt Entry Point */
/* Dummy return to be used before mutexes has been initialized		*/
/* guard against interrupts from drivers sharing the same irq line	*/
/*ARGSUSED*/
static u_int
eha_dummy_intr(caddr_t arg)
{
	return (DDI_INTR_UNCLAIMED);
}

static u_int
eha_intr(caddr_t arg)
{
	register struct	eha_blk *eha_blkp;
	register struct scsi_pkt *pktp;

	eha_blkp = EHA_BLKP(arg);

	mutex_enter(&eha_blkp->eb_mutex);

	if (!(inb(eha_blkp->eb_status1) & INT_PENDING)) {
		mutex_exit(&eha_blkp->eb_mutex);
		return (DDI_INTR_UNCLAIMED);
	}
	for (;;) {
		pktp = eha_chkstatus(eha_blkp);

		/* clear the interrupt */
		outb(eha_blkp->eb_control, CLEAR_INT);
		if (!pktp) {
			mutex_exit(&eha_blkp->eb_mutex);
			return (DDI_INTR_CLAIMED);
		}

		mutex_exit(&eha_blkp->eb_mutex);
		scsi_run_cbthread(eha_blkp->eb_cbthdl, (struct scsi_cmd *)pktp);
		mutex_enter(&eha_blkp->eb_mutex);

		if (!(inb(eha_blkp->eb_status1) & INT_PENDING))
			break;
	}
	mutex_exit(&eha_blkp->eb_mutex);
	return (DDI_INTR_CLAIMED);
}

static void
eha_chkerr(register struct scsi_pkt *pktp, struct eha_ccb *ccbp)
{
	register struct eha_stat *sp;
	register unsigned short status_word;
	struct	 scsi_arq_status *arqp;

	sp = &ccbp->ccb_stat;
	status_word = sp->status_word;


	pktp->pkt_state = 0;

	if (sp->target_status == STATUS_CHECK) {
		*pktp->pkt_scbp = sp->target_status;
		pktp->pkt_reason = CMD_CMPLT;
		pktp->pkt_state  |=
			(STATE_GOT_BUS|STATE_GOT_TARGET|STATE_SENT_CMD);

		if (!(status_word & SNS))
			return;

		if (!(ccbp->ccb_flag1 & ARS))
			return;

		pktp->pkt_state  |= STATE_GOT_STATUS;
		pktp->pkt_state  |= STATE_ARQ_DONE;

		arqp = (struct scsi_arq_status *)pktp->pkt_scbp;
		arqp->sts_rqpkt_reason = CMD_CMPLT;
		arqp->sts_rqpkt_resid  = sizeof (struct scsi_extended_sense) -
						EHA_SENSE_LEN;
		arqp->sts_rqpkt_state |= STATE_XFERRED_DATA;
#ifdef EHA_DEBUG
		if (eha_debug & DIO)
			PRF("eha_chkerr:target status 0x%x\n", *pktp->pkt_scbp);
#endif
		return;
	}

	if (status_word & SNS)
		pktp->pkt_state |= (STATE_GOT_BUS | STATE_GOT_TARGET |
		    STATE_SENT_CMD | STATE_GOT_STATUS);

	if (status_word & (INI | SC | ME)) {
		/* we have more status data */
		switch (sp->ctlr_status) {

		case HOST_CMD_ABORTED: 	/* command aborted by host */
			pktp->pkt_reason = CMD_ABORTED;
			pktp->pkt_statistics |= STAT_ABORTED;
			break;

		case H_A_CMD_ABORTED: 	/* command aborted by host adapter */
			pktp->pkt_reason = CMD_INCOMPLETE;
			pktp->pkt_statistics |= STAT_ABORTED;
			break;

		case HOST_NO_FW: /* firmware not downloaded */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_NOT_INITTED: /* SCSI subsytem not initialized */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_BAD_TARGET: /* target not assigned to subsytem */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_SEL_TO: /* selection timeout */
			pktp->pkt_reason = CMD_TIMEOUT;
			pktp->pkt_statistics |= STAT_TIMEOUT;
			break;

		case HOST_DU_DO: /* Data overrun or underrun */

			/*
			 * Make sure this logic is correct; in
			 * particular, does the test for resid
			 * really have to come before the setting
			 * of pkt_state to XFERRED_DATA?
			 */
			if (sp->status_word & DU) {
				pktp->pkt_resid = sp->resid_count;
				pktp->pkt_reason = CMD_CMPLT;
				if (pktp->pkt_resid)
					pktp->pkt_state |= STATE_XFERRED_DATA;
			} else
				pktp->pkt_reason = CMD_DATA_OVR;

			/*
			 * The code below is wrong, but is kept for 2.1
			 * compatibility.  There is a nasty kluge in
			 * scdk_pktcb() to allow format to work.
			 */
			if (*(pktp->pkt_cdbp) == SCMD_MODE_SENSE)
				pktp->pkt_reason = CMD_DATA_OVR;

			break;

		case HOST_BUS_FREE: /* unexpected bus free */
			pktp->pkt_reason = CMD_UNX_BUS_FREE;
			break;

		case HOST_PHASE_ERR: /* target bus phase seq error */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_BAD_OPCODE: /* invalid operation code */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_BAD_PARAM: /* invalid control blk parameter */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_DUPLICATE: /* duplicate targ ctl blk received */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_BAD_SG_LIST: /* invalid scatter/gather list */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_BAD_REQSENSE: /* request sense command failed */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_HW_ERROR: /* host adapter hardware error */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_ATTN_FAILED: /* target didn't respond to attn */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_SCSI_RST1: /* SCSI bus reset by host adapter */
			pktp->pkt_reason = CMD_RESET;
			pktp->pkt_statistics |= STAT_BUS_RESET;
			break;

		case HOST_SCSI_RST2: /* SCSI bus reset by other device */
			pktp->pkt_reason = CMD_RESET;
			pktp->pkt_statistics |= STAT_DEV_RESET;
			break;

		case HOST_BAD_CHKSUM: /* program checksum failure */
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		default:
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;
		}
		return;
	}

	if (status_word & DU) {
		/* data underrun */
		pktp->pkt_reason = CMD_INCOMPLETE;
		return;
	}
	if (status_word & QF) {
		/* host adapter queue full */
		pktp->pkt_reason = CMD_TRAN_ERR;
		return;
	}
	if (status_word & DO) {
		/* host adapter dat overrun */
		pktp->pkt_reason = CMD_DATA_OVR;
		return;
	}
	if (status_word & STATUSCH) {
		/* chaining halted */
		pktp->pkt_reason = CMD_TRAN_ERR;
		return;
	}
	if (status_word & INI) {
		/* initialization required */
		cmn_err(CE_CONT, "HBA initialization required\n");
		pktp->pkt_reason = CMD_TRAN_ERR;
		return;
	}
	if (status_word & ECA) {
		/* extended contingent allegiance */
		pktp->pkt_reason = CMD_TRAN_ERR;
	}
}

static void
eha_send_cmd(register struct eha_blk *eha_blkp, register struct eha_ccb *ccbp)
{
	register unchar status;

	/* Adaptec manual promises this will not last long */
	do {
		status = inb(eha_blkp->eb_status1);
	} while ((status & EHA_BUSY) || !(status & MBO_EMPTY));

	outl(eha_blkp->eb_mbo_lsb, ccbp->ccb_paddr);
	outb(eha_blkp->eb_attention, CCB_START | ccbp->ccb_target);

}

/*
 * eha_init_cmd -- Execute a SCSI command during init time
 * using no interrupts or command overlapping
 * returns 0 on success,
 * 1 for command terminated with error
 * 2 command could not be sent to controller
 */
static int
eha_init_cmd(register struct eha_blk *eha_blkp)
{
	register struct eha_ccb *ccbp;
	unchar status;
	int ret, i;
	paddr_t ccb_adr;

	ccbp = eha_blkp->eb_ccbp;
	ccb_adr = ccbp->ccb_paddr;

	do {
		status = inb(eha_blkp->eb_status1);
	} while ((status & EHA_BUSY) || !(status & MBO_EMPTY));

	outl(eha_blkp->eb_mbo_lsb, ccb_adr);
	outb(eha_blkp->eb_attention, CCB_START | ccbp->ccb_target);

	for (i = 500000; i > 0; i--) {
		if ((status = inb(eha_blkp->eb_status1)) & INT_PENDING)
			break;
	}

	if (!status & INT_PENDING) {
#ifdef EHA_DEBUG
if (eha_debug & DIO)
		PRF("eha_init_cmd: no int pending = status %x\n", status);
#endif
		return (2);
	}

	/* read command status */
	status = inb(eha_blkp->eb_intrport) & INTSTAT_BITS;
	switch (status) {
		case CCB_DONE:
		case CCB_RETRIED:
			ret = 0;
			break;

		case CCB_DONE_ERROR:
			ret = 1;
			break;

		case HBA_HW_FAILED:
		case IMMEDIATE_DONE: /* we issued a START command */
		case IMMEDIATE_DONE_ERR:
		case ASYNC_EVENT:
		default:
			ret = 2;
			break;
	}

	/* clear adapter interrupt */
	outb(eha_blkp->eb_control, CLEAR_INT);

	return (ret);
}

struct eha_ccb *
eha_retccb(struct eha_blk *eha_blkp, register unsigned long ai)
{
	register struct	eha_ccb *cp;
	register struct eha_ccb *prev = NULL;

	for (cp = eha_blkp->eb_ccboutp; cp; prev = cp, cp = cp->ccb_forw) {
		if (cp->ccb_paddr != ai)
			continue;

		/* if first one on list */
		if (eha_blkp->eb_ccboutp == cp)
			eha_blkp->eb_ccboutp = cp->ccb_forw;
		else
			/* intermediate or last */
			prev->ccb_forw = cp->ccb_forw;

		/* if last one on list */
		if (eha_blkp->eb_last == cp)
			eha_blkp->eb_last = prev;

		return (cp);
	}

	return (NULL);
}

void
eha_saveccb(register struct eha_blk *eha_blkp, register struct eha_ccb *ccbp)
{
	if (!eha_blkp->eb_ccboutp)
		eha_blkp->eb_ccboutp = ccbp;
	else
		eha_blkp->eb_last->ccb_forw = ccbp;

	ccbp->ccb_forw = NULL;
	eha_blkp->eb_last = ccbp;

}

#ifdef SCSI_SYS_DEBUG
void
eha_dump_ehablk(struct eha_blk *p)
{
	PRF("numdev %d flag 0x%x targetid 0x%x intr 0x%x\n",
		p->eb_numdev & 0xff, p->eb_flag & 0xff,
		p->eb_targetid & 0xff, p->eb_intr & 0xff);

	PRF("dip 0x%x ioaddr 0x%x inqp 0x%x ccb_cnt %d ccbp 0x%x\n",
		p->eb_dip, p->eb_ioaddr, p->eb_inqp, p->eb_ccb_cnt,
		p->eb_ccbp);
}

char *eha_err_strings[] = {
	"No error", 				/* 0x00 */
	"Invalid error", 			/* 0x01 */
	"Invalid error", 			/* 0x02 */
	"Invalid error", 			/* 0x03 */
	"Command aborted by host", 		/* 0x04 */
	"Command aborted by hba", 		/* 0x05 */
	"Invalid error", 			/* 0x06 */
	"Invalid error", 			/* 0x07 */
	"Firmware not downloaded", 		/* 0x08 */
	"SCSI subsytem not initialized",	/* 0x09 */
	"Target not assigned", 			/* 0x0a */
	"Invalid error", 			/* 0x0b */
	"Invalid error", 			/* 0x0c */
	"Invalid error", 			/* 0x0d */
	"Invalid error", 			/* 0x0e */
	"Invalid error", 			/* 0x0f */
	"Invalid error", 			/* 0x10 */
	"Selection timeout",			/* 0x11 */
	"Data overrun or underrun",		/* 0x12 */
	"Unexpected bus free",			/* 0x13 */
	"Target bus phase seq error",		/* 0x14	*/
	"Invalid error", 			/* 0x15 */
	"Invalid operation code",		/* 0x16 */
	"Invalid error", 			/* 0x17 */
	"Invalid control blk parameter",	/* 0x18	*/
	"Duplicate target ctl blk rec",		/* 0x19 */
	"Invalid Scatter-Gather list",		/* 0x1a */
	"Request sense command failed",		/* 0x1b */
	"Invalid error", 			/* 0x1c */
	"Invalid error", 			/* 0x1d */
	"Invalid error", 			/* 0x1e */
	"Invalid error", 			/* 0x1f */
	"Host Adapter hardware error",		/* 0x20 */
	"target didn't respond to attn",	/* 0x21 */
	"SCSI bus reset by host adapter",	/* 0x22 */
	"SCSI bus reset by other device"	/* 0x23 */
};

void
eha_dump_ccb(struct eha_ccb *p)
{
	int index;
	register struct eha_stat *sp;

	PRF("cmdword 0x%x flag1 %d flag2 0x%x cdblen 0x%x scsi_cmd 0x%x\n",
		p->ccb_cmdword & 0xff,
		p->ccb_flag1 & 0xff, p->ccb_flag2 & 0xff,
		p->ccb_cdblen & 0xff, p->ccb_ownerp);

	if (!(PKT2EHAUNITP((struct scsi_pkt *)p->ccb_ownerp))->eu_arq)
		PRF("Auto RS not on in scsi pkt->au_arq\n");
	else
		PRF("ARS on in scsi pkt->au_arq\n");

	PRF("lun %d target %d", p->ccb_flag2 & 0x0f, p->ccb_target & 0xff);

	if (p->ccb_flag1 & ARS)
		PRF(" ARS CACHE");
	else
		PRF(" no ARS CACHE");

	if (p->ccb_flag2 & DAT)
		PRF(" data xfer");
	else
		PRF(" no data xfer");

	if (p->ccb_flag2 & DAT) {
		if (p->ccb_flag2 & DIR)
			PRF(" xfer in[read]");
		else
			PRF(" xfer out[write]");
	}

	if (p->ccb_flag1 & SG)
		PRF(" scatter gather\n");
	else
		PRF(" No scatter gather\n");

	if (p->ccb_flag1 & DSBLK)
		PRF("Disable status blk ");
	else
		PRF("No Disable status blk ");

	if (p->ccb_flag2 & FLAG2ND)
		PRF(" No disconnect");
	else
		PRF(" Disconnect");

	if (p->ccb_flag2 & TAG)
		PRF(" Tagged Q\n");
	else
		PRF(" No Tagged Q\n");

	if (p->ccb_flag1 & SG) {
		for (index = 0; index < EHA_MAX_DMA_SEGS; index++) {
			if (!((ulong)p->ccb_sg_list[index].data_ptr))
				break;
			PRF(" a:0x%x l:0x%x",
			    ((ulong)p->ccb_sg_list[index].data_ptr),
			    ((ulong)p->ccb_sg_list[index].data_len));
		}
	} else {
		PRF(" Data ptr 0x%x data len 0x%x", (ulong)p->ccb_datap,
		    p->ccb_datalen);
	}

	sp = &p->ccb_stat;
	index = sp->ctlr_status & 0xff;
	if (index < HOST_NO_STATUS || index > HOST_SCSI_RST2)
		index = HOST_UNKNOWN_ERR;
	PRF("\nctlr stat %s \n", eha_err_strings[index]);
}
#endif
