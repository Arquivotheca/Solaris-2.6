/*
 * Copyright (c) 1993-95, Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)mcis.c	1.44	96/07/29 SMI"

#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#include <sys/dktp/mcis.h>
#include <sys/debug.h>

/*
 * External references
 */

static int mcis_tran_tgt_init(dev_info_t *, dev_info_t *,
		scsi_hba_tran_t *, struct scsi_device *);
static int mcis_tran_tgt_probe(struct scsi_device *, int (*)());
static void mcis_tran_tgt_free(dev_info_t *, dev_info_t *,
		scsi_hba_tran_t *, struct scsi_device *);
static int mcis_transport(struct scsi_address *ap, struct scsi_pkt *pktp);
static int mcis_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static int mcis_reset(struct scsi_address *ap, int level);
static int mcis_capchk(char *cap, int tgtonly, int *cidxp);
static int mcis_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static int mcis_setcap(struct scsi_address *ap, char *cap,int value,int tgtonly);
static struct scsi_pkt *mcis_tran_init_pkt(struct scsi_address *ap,
		struct scsi_pkt *pkt, struct buf *bp, int cmdlen,
		int statuslen, int tgtlen, int flags, int (*callback)(),
		caddr_t arg);
static void mcis_tran_destroy_pkt(struct scsi_address *ap,
		struct scsi_pkt *pkt);
static struct scsi_pkt *mcis_pktalloc(struct scsi_address *ap, int cmdlen, 
	int statuslen, int tgtlen, int (*callback)(), caddr_t arg);
static void mcis_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt);
static struct scsi_pkt *mcis_dmaget(struct scsi_pkt *pkt, opaque_t dmatoken,  
	int (*callback)(), caddr_t arg);
static void mcis_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);
static void mcis_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);

/*
 * Local Function Prototypes
 */
static void mcis_poscmd(register struct mcis_ccb *ccbp,
    struct mcis_pos *datap);
static void mcis_initld(struct mcis_blk *mcis_blkp);
static int mcis_findhba(uint ioaddr);
static int mcis_propinit(register struct mcis_blk *mcis_blkp);
static int mcis_cfginit(struct  mcis_blk *mcis_blkp);
static int mcis_docmd( struct	mcis_blk *mcis_blkp, int cmd, unchar dev, unchar opcode);
static int mcis_wait(ushort ioaddr, ushort mask, ushort onbits, ushort offbits);
static int mcis_qwait(ushort ioaddr, ushort mask, ushort onbits, ushort offbits);
static int mcis_getedt(struct mcis_blk *mcis_blkp);
static int mcis_pollret(struct mcis_blk *mcis_blkp);
static void mcis_inqcmd(struct mcis_ccb *mcis_ccbp, char *inqp);
static u_int mcis_intr(caddr_t arg);
static u_int mcis_dummy_intr(caddr_t arg);
static struct mcis_ldevmap *mcis_map_target( struct mcis_blk *mcis_blkp, struct mcis *mcisp, int targ, int lun);
static void mcis_set_boottarg(register struct mcis_blk *mcis_blkp);
static int mcis_getld(struct mcis_blk *mcis_blkp, struct scsi_pkt *pktp, struct mcis *mcisp,  int nowait);
static void mcis_freeld(struct mcis_blk *mcis_blkp, struct mcis_ldevmap *ldp);
static struct scsi_pkt *mcis_chkstatus(struct mcis_blk *mcis_blkp, struct mcis_ldevmap *ldp, unchar intr_code);
static void mcis_seterr(struct scsi_pkt *pkt);
static int mcis_flush_cache(dev_info_t *dip, ddi_reset_cmd_t cmd);
static int mcis_feature(struct mcis_blk *mcis_blkp);

#ifdef MCIS_DEBUG
void mcis_dump_ccb(struct mcis_ccb *p);
void mcis_dump_mcisblk(struct mcis_blk *p);
void mcis_dump_ldp(struct mcis_ldevmap *l);
void mcis_dump_mcis(struct mcis *m);
#endif

/*
 * Local static data
 */
static int mcis_pgsz = 0;
static int mcis_pgmsk;
static int mcis_pgshf;

static int mcis_cb_id = 0;
static caddr_t mcisccb = (caddr_t) 0;
static kmutex_t mcis_rmutex;
static kmutex_t mcis_global_mutex;
static int mcis_global_init = 0;
static int mcis_indump;

static ddi_dma_lim_t mcis_dma_lim = {
	0,		/* address low				*/
	0x00ffffff,	/* address high				*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	(u_int) DMALIM_VER0, /* version			*/
	0x00ffffff,	/* address register			*/
	0x0000ffff,	/* counter register			*/
	512,		/* sector size				*/
	MCIS_MAX_DMA_SEGS,/* scatter/gather list length		*/
	(u_int) 0xffffffff /* request size			*/ 
};



#ifdef	MCIS_DEBUG
#define	DTST	0x0001
#define	DPKT	0x0002
#define DINIT   0x0004
#define DINTR   0x0008
#define DCMD    0x0010

static	int	mcis_debug = 0;

#endif	/* MCIS_DEBUG */


static int mcis_identify(dev_info_t *dev);
static int mcis_probe(dev_info_t *);
static int mcis_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int mcis_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

struct dev_ops	mcis_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	mcis_identify,		/* identify */
	mcis_probe,		/* probe */
	mcis_attach,		/* attach */
	mcis_detach,		/* detach */
	mcis_flush_cache,	/* reset */
	(struct cb_ops *)0,	/* driver operations */
	NULL			/* bus operations */
};

 
char _depends_on[] = "misc/scsi";

#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"IBM MicroChannel SCSI HBA Driver",	/* Name of the module. */
	&mcis_ops,	/* driver ops */
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

	mutex_init(&mcis_global_mutex, "MCIS global Mutex", 
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&mcis_global_mutex);
	}
	return (status);
}

int
_fini(void)
{
	int	status;

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&mcis_global_mutex);
	}
	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}


static int mcis_tran_tgt_init(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	int 	targ;
	int	lun;
	struct 	mcis *hba_mcisp;
	struct 	mcis *unit_mcisp;
	struct  scsi_inquiry *inqdp;
	struct  mcis_blk *mcis_blkp;

	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef	MCIS_DEBUG
	cmn_err(CE_CONT, "%s%d: %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif

	if (targ < 0 || targ > 7 || lun < 0 || lun > 7) {
		cmn_err(CE_CONT, "%s%d: %s%d bad address <%d,%d>\n",
			ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
			ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
			targ, lun);
		return (DDI_FAILURE);
	}

	hba_mcisp = SDEV2HBA(sd);
	inqdp = &(hba_mcisp->m_blkp->mb_inqp[targ<<3|lun]);

	if ((unit_mcisp = kmem_zalloc(
		sizeof(struct mcis)+sizeof(struct mcis_unit), 
			KM_NOSLEEP)) == NULL) {
		return (DDI_FAILURE);
	}

	bcopy((caddr_t)hba_mcisp, (caddr_t)unit_mcisp, sizeof(*hba_mcisp));
	unit_mcisp->m_unitp = (struct mcis_unit *)(unit_mcisp+1);
	unit_mcisp->m_unitp->mu_lim = mcis_dma_lim;
	unit_mcisp->m_ldp = (struct mcis_ldevmap *)0;

	sd->sd_inq = inqdp;

	hba_tran->tran_tgt_private = unit_mcisp;

	if (inqdp->inq_dtype == DTYPE_DIRECT) 
		unit_mcisp->m_unitp->mu_disk = 1;

/*	update	xfer request size max XXX??				*/

	mcis_blkp = MCIS_BLKP(hba_mcisp);
	mcis_blkp->mb_child++;

	if(mcis_blkp->mb_numdev < mcis_blkp->mb_ldcnt ||
		mcis_blkp->mb_boottarg == targ)
		(void) mcis_map_target(mcis_blkp, unit_mcisp, targ, lun);

#ifdef MCIS_DEBUG
	if (mcis_debug & DINIT) {
		PRF("mcis_tran_tgt_init: <%d,%d>\n", targ, lun);
	}
#endif
	return (DDI_SUCCESS);
}

static int
mcis_tran_tgt_probe(
	struct scsi_device	*sd,
	int			(*callback)())
{
	int	rval;

	rval = scsi_hba_probe(sd, callback);

#ifdef AHA_DEBUG
	{
		char		*s;
		struct 	mcis	*mcis = SDEV2MCIS(sd);

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
		cmn_err(CE_CONT, "mcis%d: %s target %d lun %d %s\n",
			ddi_get_instance(MCIS_DIP(mcis)),
			ddi_get_name(sd->sd_dev),
			sd->sd_address.a_target,
			sd->sd_address.a_lun, s);
	}
#endif	/* AHADEBUG */

	return (rval);
}



static void
mcis_tran_tgt_free(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	struct mcis	*mcis;
	struct mcis	*unit_mcisp;

#if defined(lint)
	hba_dip = hba_dip;
	tgt_dip = tgt_dip;
#endif
#ifdef	AHADEBUG
	cmn_err(CE_CONT, "mcis_tran_tgt_free: %s%d %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif	/* AHADEBUG */

	unit_mcisp = hba_tran->tran_tgt_private;
	kmem_free(unit_mcisp,
		sizeof(struct mcis)+sizeof(struct mcis_unit));

	sd->sd_inq = NULL;

	mcis = SDEV2HBA(sd);
	MCIS_BLKP(mcis)->mb_child--;
}


/*
 *	Autoconfiguration routines
 */
static int
mcis_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, "mcis") == 0) 
		return (DDI_IDENTIFIED);
	else 
		return (DDI_NOT_IDENTIFIED);
}


static int
mcis_probe(register dev_info_t *devi)
{
	int	ioaddr;
	int	len;

	len = sizeof(int);
	if ((HBA_INTPROP(devi, "ioaddr", &ioaddr, &len) != DDI_SUCCESS) ||
	    (mcis_findhba(ioaddr) != DDI_SUCCESS))
		return (DDI_PROBE_FAILURE);

	return (DDI_PROBE_SUCCESS);
}

static int 
mcis_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	register struct	mcis 	*mcis;
	register struct	mcis_blk *mcis_blkp;

	switch (cmd) {
	case DDI_DETACH:
	{
		scsi_hba_tran_t	*hba_tran;
		hba_tran = (scsi_hba_tran_t *)
			ddi_get_driver_private(devi);
		if (hba_tran == NULL)
			return (DDI_SUCCESS);
		mcis = TRAN2MCIS(hba_tran);
		if (!mcis) 
			return (DDI_SUCCESS);
		mcis_blkp = MCIS_BLKP(mcis);
		if (mcis_blkp->mb_child)
			return (DDI_FAILURE);

/*		enable on board	dma controller but turn interrupts off		*/
		outb(mcis_blkp->mb_ioaddr+MCIS_CTRL, ISCTRL_EDMA);
		ddi_iopb_free((caddr_t)mcis_blkp->mb_inqp);
		ddi_remove_intr(devi, 0, mcis_blkp->mb_iblock);

		scsi_destroy_cbthread(mcis_blkp->mb_cbthdl);
		mutex_destroy(&mcis_blkp->mb_mutex);

		mutex_enter(&mcis_global_mutex);
		mcis_global_init--; 
		if (mcis_global_init == 0)
			mutex_destroy(&mcis_rmutex);
		mutex_exit(&mcis_global_mutex);

		(void)kmem_free((caddr_t)mcis,(sizeof(*mcis)+sizeof(*mcis_blkp)));

		ddi_prop_remove_all(devi);
		if (scsi_hba_detach(devi) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "mcis: scsi_hba_detach failed\n");
		}
		return (DDI_SUCCESS);
	}
	default:
		return (DDI_FAILURE);
	}
}

static int
mcis_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	register struct mcis 	*mcis;
	register struct mcis_blk	*mcis_blkp;
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
	mcis = (struct mcis *) kmem_zalloc((unsigned) (sizeof(*mcis) +
		sizeof(*mcis_blkp)), KM_NOSLEEP);
	if (!mcis)
		return (DDI_FAILURE);
	mcis_blkp = (struct mcis_blk *)(mcis + 1);
	MCIS_BLKP(mcis) = mcis_blkp;
	mcis_blkp->mb_dip = devi;

	if ((mcis_propinit(mcis_blkp) == DDI_FAILURE) ||
	    (mcis_cfginit(mcis_blkp)  == DDI_FAILURE) ||
	    (mcis_getedt (mcis_blkp)  == DDI_FAILURE)) {
		(void)kmem_free((caddr_t)mcis,
			(sizeof(*mcis)+sizeof(*mcis_blkp)));
		return (DDI_FAILURE);
	}

	/*
	 * Allocate a transport structure
	 */
	hba_tran = scsi_hba_tran_alloc(devi, 0);
	if (hba_tran == NULL) {
		cmn_err(CE_WARN, "mcis_attach: scsi_hba_tran_alloc failed");
		kmem_free((caddr_t)mcis, (sizeof(*mcis)+sizeof(*mcis_blkp)));
		return (DDI_FAILURE);
	}

/*
 *	Establish initial dummy interrupt handler 
 *	get iblock cookie to initialize mutexes used in the 
 *	real interrupt handler
 */
	if (ddi_add_intr(devi, 0,
			(ddi_iblock_cookie_t *) &mcis_blkp->mb_iblock,
			(ddi_idevice_cookie_t *) 0, mcis_dummy_intr,
			(caddr_t) mcis)){
		cmn_err(CE_WARN, "mcis_attach: cannot add intr");
		(void)kmem_free((caddr_t)mcis,(sizeof(*mcis)+sizeof(*mcis_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	mutex_init(&mcis_blkp->mb_mutex, "mcis mutex", MUTEX_DRIVER,
		mcis_blkp->mb_iblock);

	ddi_remove_intr(devi, 0, mcis_blkp->mb_iblock);
/*	Establish real interrupt handler				*/
	if (ddi_add_intr(devi, 0,
		(ddi_iblock_cookie_t *) &mcis_blkp->mb_iblock,
		(ddi_idevice_cookie_t *) 0, mcis_intr, (caddr_t) mcis)) {
		cmn_err(CE_WARN, "mcis_attach: cannot add intr");
		(void)kmem_free((caddr_t)mcis,(sizeof(*mcis)+sizeof(*mcis_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}
	mcis_blkp->mb_cbthdl = scsi_create_cbthread(mcis_blkp->mb_iblock,
		KM_NOSLEEP);

	mcis->m_tran = hba_tran;

	hba_tran->tran_hba_private 	= mcis;
	hba_tran->tran_tgt_private 	= NULL;

	hba_tran->tran_tgt_init 	= mcis_tran_tgt_init;
	hba_tran->tran_tgt_probe 	= mcis_tran_tgt_probe;
	hba_tran->tran_tgt_free 	= mcis_tran_tgt_free;

	hba_tran->tran_start 		= mcis_transport;
	hba_tran->tran_abort		= mcis_abort;
	hba_tran->tran_reset		= mcis_reset;
	hba_tran->tran_getcap		= mcis_getcap;
	hba_tran->tran_setcap		= mcis_setcap;
	hba_tran->tran_init_pkt 	= mcis_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= mcis_tran_destroy_pkt;
	hba_tran->tran_dmafree		= mcis_dmafree;
	hba_tran->tran_sync_pkt		= mcis_sync_pkt;

	if (scsi_hba_attach(devi, &mcis_dma_lim, hba_tran,
			SCSI_HBA_TRAN_CLONE, NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "mcis_attach: scsi_hba_attach failed\n");
		ddi_remove_intr(devi, 0, mcis_blkp->mb_iblock);
		kmem_free((caddr_t)mcis,(sizeof(*mcis)+sizeof(*mcis_blkp)));
		scsi_hba_tran_free(hba_tran);
	}

	mutex_enter(&mcis_global_mutex);	/* protect multithreaded attach	*/
	if (!mcis_global_init) {
		mutex_init(&mcis_rmutex, "MCIS Resource Mutex",
			MUTEX_DRIVER, mcis_blkp->mb_iblock);
	}
	mcis_global_init++; 
	mutex_exit(&mcis_global_mutex);

	ddi_report_dev(devi);
	mcis_set_boottarg(mcis_blkp);

/*	enable both dma and interrupt 					*/
	outb(mcis_blkp->mb_ioaddr+MCIS_CTRL, ISCTRL_EDMA|ISCTRL_EINTR);

	return (DDI_SUCCESS);
}

static int
mcis_propinit(register struct mcis_blk *mcis_blkp)
{
	register dev_info_t *devi;
	int	i;
	int	val;
	int	len;
	char 	buf[12];
	
	/* Make sure this is a micro channel machine */
	len = sizeof(buf);
	if (ddi_prop_op(DDI_DEV_T_NONE, mcis_blkp->mb_dip, PROP_LEN_AND_VAL_BUF, 
		0, "bus-type", (caddr_t)buf, &len) != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);
	buf[len] = (char)0;
	if (strcmp(buf, "mc") != 0) {
#ifdef MCIS_DEBUG
		if (mcis_debug & DINIT) 
			PRF("mcis_propinit bus type %s\n",buf);
#endif
		return DDI_FAILURE;
	}

	devi = mcis_blkp->mb_dip;
	len = sizeof(int);
	if (HBA_INTPROP(devi, "ioaddr", &val, &len) != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);
	mcis_blkp->mb_ioaddr   = (ushort) val;

	len = sizeof(buf);
	if (ddi_prop_op(DDI_DEV_T_NONE, devi, PROP_LEN_AND_VAL_BUF, 
		0, "hwcache", (caddr_t)buf, &len) == DDI_PROP_SUCCESS) {
		buf[len] = (char)0;
		if (strcmp(buf,"on") == 0)   {
			mcis_blkp->mb_flag  |= MCIS_CACHE;
		}
	}

	mutex_enter(&mcis_global_mutex);	
	if (!mcis_pgsz) {
		mcis_pgsz = ddi_ptob(devi, 1L);
		mcis_pgmsk = mcis_pgsz - 1;
		for (i=mcis_pgsz, len=0; i > 1; len++)
			i >>=1;
		mcis_pgshf = len;
	}
	mutex_exit(&mcis_global_mutex);

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int 
mcis_transport(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct mcis_blk *mcis_blkp;
	register struct	mcis_ccb *ccbp;
	unchar opcode;
	struct   mcis *mcisp;
	int blkcnt;
	

	ccbp = (struct mcis_ccb *)SCMD_PKTP(pktp)->cmd_private;
	mcis_blkp = PKT2MCISBLKP(pktp);

	mcisp = PKT2MCIS(pktp);

/*	 Workaround: fail requests if mcis_wait() times out too often	*/
	if ((pktp->pkt_flags & FLAG_NOINTR) && panicstr) {
		if (!mcis_indump)
			mcis_indump = 1;
		else if (mcis_indump > 2)
			return (TRAN_ACCEPT);
	}

/*	if GOTLD, proceed
	   WAITLD, return - will be processed at interrupt time
	   OWNLD, go into mcis_getld()
	Note: review GOTLD case when testing XXX
*/
	if (mcisp->m_ldp->ld_status & MSCB_GOTLD)
		mcisp->m_ldp->ld_status &= ~MSCB_GOTLD;
	else if (mcisp->m_ldp->ld_status & MSCB_WAITLD) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DPKT)
			PRF("mcis_transport: WAITLD\n");
#endif
		return TRAN_ACCEPT;
	} else if (mcis_getld(mcis_blkp, pktp, mcisp, 
		((pktp->pkt_flags & FLAG_NOINTR) ? 0 : 1)) == MCIS_HBALD) {
			pktp->pkt_reason = CMD_INCOMPLETE;
/*			pkt_state is unchanged				*/
			return TRAN_BUSY;
	} 

	mutex_enter(&mcis_blkp->mb_mutex);
	if (mcisp->m_ldp->ld_busy > 0) {
		mutex_exit(&mcis_blkp->mb_mutex);
		return TRAN_BUSY;
	}
	mcisp->m_ldp->ld_busy++;
	mutex_exit(&mcis_blkp->mb_mutex);

/*	request run callback thread from mcis_chstatus			*/
	mcisp->m_ldp->ld_cmdp  = (struct scsi_cmd *)pktp;

	opcode = ISATTN_LSCB;

/*	xlate SCSI command to IBM vendor specific ccb */
	switch (*(pktp->pkt_cdbp)) {

		case SCMD_READ_G1:
		case SCMD_READ:
			if(mcisp->m_unitp->mu_disk && 
				ccbp->ccb_flag & MCIS_CACHE) {
				opcode = ISCMD_READ;
			}
			break;

		case SCMD_WRITE_G1:
		case SCMD_WRITE:
			if(mcisp->m_unitp->mu_disk && 
				ccbp->ccb_flag & MCIS_CACHE) {
				opcode = ISCMD_WRITE;
			}
			break;

		default:
			break;
	}

/*	enable read-write through the adapter cache if required		*/
	if(opcode == ISCMD_READ || opcode == ISCMD_WRITE) {
		ccbp->ccb_cmdop  = opcode;
		ccbp->ccb_cmdsup = ISCMD_SCB;
		ccbp->MSCB_opfld.ccb_nobuff = 0;

		/* save part of the command changed by vendor specific cmd */
		ccbp->ccb_scsi_op0 = *(uint *)ccbp->ccb_cdb;
		ccbp->ccb_scsi_op1 = *(uint *)&ccbp->ccb_cdb[4];

		if (ccbp->ccb_cdb[0] & 0x20) {
			ccbp->ccb_baddr=
				scsi_stoh_long(*((long *)(&ccbp->ccb_cdb[2])));
			blkcnt=(ccbp->ccb_cdb[7]<<8) | (ccbp->ccb_cdb[8]);
		} else {
			ccbp->ccb_baddr = ((ccbp->ccb_cdb[1]&0x1f)<<16) |
				(ccbp->ccb_cdb[2]<<8) |(ccbp->ccb_cdb[3]);
			blkcnt = ccbp->ccb_cdb[4];
		}
		set_blkcnt(ccbp, blkcnt);
		set_blklen(ccbp, mcisp->m_unitp->mu_lim.dlim_granular);
		opcode = ISATTN_SCB;
	} else if ((*(pktp->pkt_cdbp)) == SCMD_READ_CAPACITY) {
		opcode = ISATTN_SCB;
		ccbp->ccb_cmdop = ISCMD_READCAP;
		ccbp->ccb_cmdsup = ISCMD_SCB;
		ccbp->MSCB_op = 0xE6;
		ccbp->ccb_baddr  = 0;
	} else {
		ccbp->ccb_cmdop  = ISCMD_SNDSCSI;
		ccbp->ccb_cmdsup = ISCMD_LSCB;
		ccbp->MSCB_opfld.ccb_nobuff = 1;
		ccbp->ccb_baddr = ccbp->ccb_cdblen;
	}

	mutex_enter(&mcis_blkp->mb_mutex);
	if (mcis_docmd(mcis_blkp, (int) ccbp, mcisp->m_ldp->LD_CB.a_ld,
		opcode)) {
		mcisp->m_ldp->ld_busy--;
		mutex_exit(&mcis_blkp->mb_mutex);
		return TRAN_BUSY;
	}

	if (pktp->pkt_flags & FLAG_NOINTR) {
		if (mcis_pollret(mcis_blkp)) {
			pktp->pkt_reason = CMD_TRAN_ERR;
			mcisp->m_ldp->ld_busy--;
			mutex_exit(&mcis_blkp->mb_mutex);
			return TRAN_BUSY;
		}  else {
			opcode = mcis_blkp->mb_intr_code;
			mutex_exit(&mcis_blkp->mb_mutex);
			mcis_chkstatus(mcis_blkp, mcisp->m_ldp, opcode);
			return TRAN_ACCEPT;
		}
	}

	mutex_exit(&mcis_blkp->mb_mutex);
	return TRAN_ACCEPT;
}

static int
mcis_capchk(char *cap, int tgtonly, int *cidxp)
{
	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *) 0) 
		return (FALSE);

	*cidxp = scsi_hba_lookup_capstr(cap);
	return (TRUE);
}

static int 
mcis_getcap(struct scsi_address *ap, char *cap, int tgtonly) 
{
	int	ckey;
	int	status;

#if defined(lint)
	ap = ap;
#endif
	status = mcis_capchk(cap, tgtonly, &ckey);
	if (status != TRUE)
		return (UNDEFINED);

	switch (ckey) {

	case SCSI_CAP_GEOMETRY:
		return(HBA_SETGEOM(64,32));

	default:
		break;
	}

	return (UNDEFINED);
}

static int 
mcis_setcap(struct scsi_address *ap, char *cap, int value, int tgtonly) 
{
	int	ckey;
	int	status = FALSE;

	if ((status = mcis_capchk(cap, tgtonly, &ckey)) != TRUE)
		return (UNDEFINED);

	switch (ckey) {

		case SCSI_CAP_SECTOR_SIZE: 
			(ADDR2MCISUNITP(ap))->mu_lim.dlim_granular = 
						(u_int)value;
			status = TRUE;
			break;

		case SCSI_CAP_ARQ:
			status = UNDEFINED;
			break;

		default:
			break;
	}

	return (status);
}



static struct scsi_pkt *
mcis_tran_init_pkt(struct scsi_address *ap, struct scsi_pkt *pkt,
	struct buf *bp, int cmdlen, int statuslen, int tgtlen,
	int flags, int (*callback)(), caddr_t arg)
{
	struct scsi_pkt		*new_pkt = NULL;

	/*
	 * Allocate a pkt
	 */
	if (!pkt) {
		pkt = mcis_pktalloc(ap, cmdlen, statuslen,
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
		if (mcis_dmaget(pkt, (opaque_t) bp, callback, arg) == NULL) {
			if (new_pkt)
				mcis_pktfree(ap, new_pkt);
			return (NULL);
		}
	}

	return (pkt);
}


static void
mcis_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	mcis_dmafree(ap, pkt);
	mcis_pktfree(ap, pkt);
}


static struct scsi_pkt *
mcis_pktalloc(struct scsi_address *ap, int cmdlen, int statuslen,
    int tgtlen, int (*callback)(), caddr_t arg)
{
	register struct scsi_cmd *cmd;
	register struct mcis_ccb	*ccbp;
	struct	 mcis_blk	*mcis_blkp;
	caddr_t	 buf;
	int 	 kf;
	caddr_t	tgt;

#if defined(lint)
	statuslen = statuslen;
#endif
	mcis_blkp = ADDR2MCISBLKP(ap);
	kf = HBA_KMFLAG(callback);

	/*
	 * Allocate target-private data, if necessary
	 */
	if (tgtlen > PKT_PRIV_LEN) {
		tgt = kmem_zalloc(tgtlen, kf);
		if (!tgt) {
			ASSERT(callback != SLEEP_FUNC);
			if (callback != NULL_FUNC)
				ddi_set_callback(callback, arg, &mcis_cb_id);
			return ((struct scsi_pkt *)NULL);
		}
	} else {
		tgt = NULL;
	}

	cmd = kmem_zalloc(sizeof (*cmd), kf);
	mutex_enter(&mcis_rmutex);
	if (cmd) {
/* 		allocate ccb 						*/
		if (scsi_iopb_fast_zalloc(&mcisccb, mcis_blkp->mb_dip, 
			(ddi_dma_lim_t *)0, (u_int)(sizeof(*ccbp)), &buf)) {
			kmem_free(cmd, sizeof (*cmd));
			cmd = NULL;
		} 
	}
	mutex_exit(&mcis_rmutex);

	if (!cmd) {
		if (tgt)
			kmem_free(tgt, tgtlen);
		ASSERT(callback != SLEEP_FUNC);
		if (callback != DDI_DMA_DONTWAIT) 
			ddi_set_callback(callback, arg, &mcis_cb_id);
		return ((struct scsi_pkt *)NULL);
	}

/* 	initialize ccb 							*/
	ccbp = (struct mcis_ccb *)buf;

	ccbp->ccb_cdblen = (unchar)cmdlen;
	ccbp->ccb_ownerp = cmd;
	ccbp->ccb_paddr  = MCIS_KVTOP(ccbp);
	ccbp->ccb_tsbp  = MCIS_KVTOP(&ccbp->ccb_tsb);

	ccbp->MSCB_opfld.ccb_ss = 1;
	ccbp->MSCB_opfld.ccb_retry = 1;
	ccbp->MSCB_opfld.ccb_es = 1;
	ccbp->MSCB_opfld.ccb_nobuff = 1;
	/* note scatter gather is off by default */

	/* for non IBM read-write commands */
	ccbp->ccb_baddr  = cmdlen;

	if(mcis_blkp->mb_flag & MCIS_CACHE)
		ccbp->ccb_flag |= MCIS_CACHE;

/* 	prepare the packet for normal command 				*/
	cmd->cmd_private         = (opaque_t) ccbp;
	cmd->cmd_pkt.pkt_cdbp    = (opaque_t) ccbp->ccb_cdb;
	cmd->cmd_cdblen 	 = (u_char) cmdlen;

	cmd->cmd_pkt.pkt_scbp	 = &ccbp->ccb_status;
	cmd->cmd_scblen 	 = (u_char) 1;

	cmd->cmd_pkt.pkt_address = *ap;

	/*
	 * Set up target-private data
	 */
	cmd->cmd_privlen = (unchar)tgtlen;
	if (tgtlen > PKT_PRIV_LEN) {
		cmd->cmd_pkt.pkt_private = tgt;
	} else if (tgtlen > 0) {
		cmd->cmd_pkt.pkt_private = cmd->cmd_pkt_private;
	}

#ifdef MCIS_DEBUG
	if (mcis_debug & DPKT) {
		PRF("mcis_pktalloc:cmdpktp= 0x%x pkt_cdbp=0x%x pkt_scbp=0x%x\n", 
			cmd, cmd->cmd_pkt.pkt_cdbp, cmd->cmd_pkt.pkt_scbp);
		PRF("ccbp = 0x%x\n",ccbp);
	}
#endif
	return ((struct scsi_pkt *) cmd);
}

/*
 * packet free
 */
/*ARGSUSED*/
void
mcis_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	register struct scsi_cmd *cmd = (struct scsi_cmd *) pkt;

	if (cmd->cmd_privlen > PKT_PRIV_LEN) {
		kmem_free(pkt->pkt_private, cmd->cmd_privlen);
	}

	mutex_enter (&mcis_rmutex);
/*	deallocate the ccb						*/
	if (cmd->cmd_private)
		scsi_iopb_fast_free(&mcisccb, (caddr_t)cmd->cmd_private);
	mutex_exit (&mcis_rmutex);
/*	free the common packet						*/
	kmem_free(cmd, sizeof (*cmd));

	if (mcis_cb_id) 
		ddi_run_callback(&mcis_cb_id);
}

/*
 * Dma resource allocation
 */
/*ARGSUSED*/
static void
mcis_dmafree(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct	scsi_cmd *cmd = (struct scsi_cmd *) pktp;

/* 	Free the mapping.  						*/
	if (cmd->cmd_dmahandle) {
		ddi_dma_free(cmd->cmd_dmahandle);
		cmd->cmd_dmahandle = NULL;
	}
}

/*ARGSUSED*/
static void
mcis_sync_pkt(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register int i;
	register struct	scsi_cmd *cmd = (struct scsi_cmd *) pktp;

	if (cmd->cmd_dmahandle) {
		i = ddi_dma_sync(cmd->cmd_dmahandle, 0, 0,
			(cmd->cmd_cflags & CFLAG_DMASEND) ?
			DDI_DMA_SYNC_FORDEV : DDI_DMA_SYNC_FORCPU);
		if (i != DDI_SUCCESS) {
			cmn_err(CE_WARN, "mcis: sync pkt failed\n");
		}
	}
}



static struct scsi_pkt *
mcis_dmaget(struct scsi_pkt *pktp, opaque_t dmatoken, int (*callback)(), caddr_t arg)
{
	struct buf *bp = (struct buf *) dmatoken;
	register struct scsi_cmd *cmd = (struct scsi_cmd *) pktp;
	register struct mcis_ccb *ccbp;
	struct mcis_dma_seg *dmap;
	ddi_dma_cookie_t dmack;
	ddi_dma_cookie_t *dmackp = &dmack;
	int	cnt;
	int	bxfer;
	off_t	offset;
	off_t	len;

	ccbp = (struct mcis_ccb *)cmd->cmd_private;

	if (bp->b_flags & B_READ) {
		ccbp->MSCB_opfld.ccb_read = 1;
		cmd->cmd_cflags &= ~CFLAG_DMASEND;
	} else {
/* 		in case packets are reused				*/
		ccbp->MSCB_opfld.ccb_read = 0;
		cmd->cmd_cflags |= CFLAG_DMASEND;
	}

	if (!bp->b_bcount) {
		cmd->cmd_pkt.pkt_resid = 0;
		return (pktp);
	}

	/* setup vendor specific read/write commands in transport	*/
	ccbp->ccb_cmdop = ISCMD_SNDSCSI;
	ccbp->ccb_cmdsup = ISCMD_LSCB;

/*	setup dma memory and position to the next xfer segment		*/
	if (!scsi_impl_dmaget(pktp, (opaque_t)bp, callback, arg,
		&(PKT2MCISUNITP(pktp)->mu_lim)))
		return (NULL);
	ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len, dmackp);

/*	single block transfer						*/
	if (bp->b_bcount <= dmackp->dmac_size) {
		ccbp->MSCB_opfld.ccb_scatgath = 0; /* necessary for packet reuse XXX */
		ccbp->ccb_datap = dmackp->dmac_address;
		ccbp->ccb_datalen = bp->b_bcount;
		pktp->pkt_resid = 0;
		cmd->cmd_totxfer = bp->b_bcount;
	} else {
		ccbp->MSCB_opfld.ccb_scatgath = 1;
		dmap = ccbp->ccb_sg_list;
		for (bxfer=0, cnt=1; ; cnt++, dmap++) {
			bxfer += dmackp->dmac_size;

			dmap->data_len = (ulong) dmackp->dmac_size;
			dmap->data_ptr = (ulong) dmackp->dmac_address;

/*			check for end of list condition			*/
			if (bp->b_bcount == (bxfer + cmd->cmd_totxfer))
				break;
			ASSERT(bp->b_bcount > (bxfer + cmd->cmd_totxfer));
/*			check end of physical scatter-gather list limit	*/
			if (cnt >= MCIS_MAX_DMA_SEGS) 
				break;
/*			check for transfer count			*/
			if (bxfer >= (PKT2MCISUNITP(pktp)->mu_lim.dlim_reqsize)) 
				break;
			if (ddi_dma_nextseg(cmd->cmd_dmawin, cmd->cmd_dmaseg,
				&cmd->cmd_dmaseg) != DDI_SUCCESS) 
				break;
			ddi_dma_segtocookie(cmd->cmd_dmaseg,&offset,
				&len,dmackp);
		}
		ccbp->ccb_datalen =  (ulong)(cnt*sizeof(*dmap));
		ccbp->ccb_datap = (int)((paddr_t)ccbp->ccb_paddr + 
			((caddr_t)ccbp->ccb_sg_list - (caddr_t)ccbp));

		cmd->cmd_totxfer += bxfer;
		pktp->pkt_resid = bp->b_bcount - cmd->cmd_totxfer;
	}

	return (pktp);
}

/**************************** Adapter Dependent Layer *******************/

/*
 *	Adapter detection routine
 */
mcis_findhba(ioaddr)
register uint	ioaddr;
{
	register int	i;
	char	 status;
	char	 *sp = &status;

#ifdef MCIS_DEBUG
	if (mcis_debug & DINIT) 
		PRF("mcis_findhba looking at 0x%x\n",ioaddr);
#endif

/*	hardware reset							*/
	outb(ioaddr+MCIS_CTRL, ISCTRL_RESET);
/*	wait for at least 50ms and then turn off controller reset	*/
	for (i=6; i; i--)
		drv_usecwait(10);
	outb(ioaddr+MCIS_CTRL, 0);

/*	check busy in status reg 					*/
	if (MCIS_QBUSYWAIT(ioaddr)) {
#ifdef MCIS_DEBUG
		if (mcis_debug & DINIT) 
			PRF("mcis_findhba fail busywait\n");
#endif
		return (1);
	}

/*	wait for interrupt						*/
	if (MCIS_QINTRWAIT(ioaddr)) {
#ifdef MCIS_DEBUG
		if (mcis_debug & DINIT) 
			PRF("mcis_findhba fail intr wait\n");
#endif
		return (1);
	}
	
	status = inb(ioaddr+MCIS_INTR);
	MCIS_SENDEOI(ioaddr, MCIS_HBALD);
	if (MCIS_rintrp(sp)->ri_code)  {
#ifdef MCIS_DEBUG
		if (mcis_debug & DINIT) 
			PRF("mcis_findhba fail eoi code\n");
#endif
		return (1);
	}

/*	enable on board	dma controller but keep interrupts off		*/
	outb(ioaddr+MCIS_CTRL, ISCTRL_EDMA);

	status = inb(ioaddr+MCIS_CTRL);
	if (!MCIS_rctlp(sp)->rc_edma) {
		/*EMPTY*/
#ifdef MCIS_DEBUG
		if (mcis_debug & DINIT) 
			PRF("mcis_findhba fail dma enable\n");
#endif
		/* return (DDI_FAILURE); for model 95 */
	}

#ifdef MCIS_DEBUG
	if (mcis_debug & DINIT) 
		PRF("mcis_findhba good at 0x%x\n",ioaddr);
#endif
	return(DDI_SUCCESS);	
}

static int
mcis_cfginit(register struct  mcis_blk *mcis_blkp)
{
	struct mcis_ccb *ccbp;
	caddr_t		buf;
	int mem;
	int status = DDI_SUCCESS;
	struct	mcis_pos *posp;

/* 	allocate ccb and pos buffer 					*/
	mem = sizeof(struct mcis_ccb) + sizeof(*posp);
	if (ddi_iopb_alloc(mcis_blkp->mb_dip, (ddi_dma_lim_t *)0,
		mem, &buf)) {
#ifdef MCIS_DEBUG
	if(mcis_debug & DINIT)
		PRF("mcis_cfginit: unable to allocate memory.\n");
#endif
		return DDI_FAILURE;
	} 

	bzero((caddr_t)buf, mem);
	ccbp = (struct mcis_ccb *)buf;
	posp = (struct mcis_pos *)(ccbp+1);

/*	get POS information						*/
	mcis_poscmd(ccbp, posp);
	if ((mcis_docmd(mcis_blkp, (int)ccbp, MCIS_HBALD, ISATTN_SCB))
		|| (mcis_pollret(mcis_blkp))
		|| ((mcis_blkp->mb_intr_code!=ISINTR_SCB_OK) &&
		    (mcis_blkp->mb_intr_code!=ISINTR_SCB_OK2))) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DINIT)
		PRF("mcis_cfginit: fail on getpos retcod=0x%x\n",
			mcis_blkp->mb_intr_code);
#endif
		ddi_iopb_free((caddr_t)ccbp);
		ccbp = (struct mcis_ccb *)0;
		return DDI_FAILURE;
	}

	mcis_blkp->mb_intr     = posp->p_intr;
	mcis_blkp->mb_targetid = posp->p3_targid;

/*	calculate available logical device				*/
	if (posp->p_ldcnt < MCIS_MAXLD)
		mcis_blkp->mb_ldcnt = posp->p_ldcnt-1;
	else
		mcis_blkp->mb_ldcnt = MCIS_MAXLD-1;

#ifdef MCIS_DEBUG
	if(mcis_debug & DINIT) {
	PRF("mcis_cfginit: mcis_blkp= 0x%x ccbp= 0x%x \n", 
		mcis_blkp, ccbp);
	PRF("mcis_cfginit: hbaid= 0x%x targetid= %d\n", 
		posp->p_hbaid, mcis_blkp->mb_targetid);
	PRF("dma= 0x%x fair= %d ioaddr= 0x%x romseg= 0x%x\n",
		posp->p3_chan, posp->p3_fair, posp->p2_ioaddr, posp->p2_romseg);
	PRF("intr= 0x%x pos4= %d slotsz= 0x%x ldcnt= 0x%x\n",
		posp->p_intr, posp->p_pos4, posp->p_slotsz, posp->p_ldcnt);
	PRF("pacing= 0x%x tmeoi= 0x%x tmreset= 0x%x cache= 0x%x\n",
		posp->p_pacing, posp->p_tmeoi, posp->p_tmreset, posp->p_cache);
	PRF("mcis_bpkp->mb_ldcnt %d\n",mcis_blkp->mb_ldcnt);
	}
#endif

/*	initialize logical device nexus table				*/
	mcis_initld(mcis_blkp);

	if(mcis_feature(mcis_blkp))
		status =  DDI_FAILURE;

/*	free temp memory allocated here */
	ddi_iopb_free((caddr_t)ccbp);

	return (status);
}


/* turn off the automatic 45 second controller timeout
 * returns 0 on success, -1 on failure
 */
static int
mcis_feature(struct mcis_blk *mcis_blkp)
{

	if ((mcis_docmd(mcis_blkp, 0x400 | ISCMD_HBACTRL, MCIS_HBALD, ISATTN_ICMD))
		|| (mcis_pollret(mcis_blkp))
		|| (mcis_blkp->mb_intr_code != ISINTR_ICMD_OK)) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DINIT)
		PRF("mcis_feature: fail on set timeout: intr_code %x\n",
			mcis_blkp->mb_intr_code );
#endif
		return -1;
	}

#ifdef MCIS_DEBUG
		if(mcis_debug & DINIT)
		PRF("mcis_feature: success on set timeout: intr_code %x\n",
			mcis_blkp->mb_intr_code );
#endif

	return 0;
}

/* 	Flush hardware cache - only necessary if IBM r/w commands used.
 *	Called when the system is being halted to disable all hardware
 *	interrupts.  Note that we *can't* block at all, not even on mutexes.
 */
/*ARGSUSED*/
static int
mcis_flush_cache(dev_info_t *dip, ddi_reset_cmd_t cmd)
{
	struct mcis_blk *mcis_blkp;
	register struct	mcis_ldevmap *ldp;
	struct scsi_inquiry *inqp;
	int i;

	mcis_blkp = MCIS_BLKP((struct mcis *)ddi_get_driver_private(dip));
	if(!(mcis_blkp->mb_flag & MCIS_CACHE))
		return (DDI_SUCCESS);

	ldp = mcis_blkp->mb_ldm;

	for(i=0;i < MCIS_MAXLD;i++,ldp++) {

		inqp = &(mcis_blkp->mb_inqp[ldp->LD_CB.a_targ <<3|ldp->LD_CB.a_lunn ]); 

/* 		skip null devices or those not disks 			*/
		if (inqp->inq_dtype == DTYPE_NOTPRESENT ||
	    		inqp->inq_dtype == DTYPE_UNKNOWN ||
			inqp->inq_dtype != DTYPE_DIRECT)
			continue;

		/* skip hba */
		if( ldp->LD_CB.a_targ == mcis_blkp->mb_targetid)
			continue;

		ldp->LD_CB.a_rm = 0;		
/*		do not run callback thread from interrupt		*/
		ldp->ld_cmdp = (struct scsi_cmd *)0;
/* 		do an assign 						*/
		(void) mcis_docmd(mcis_blkp, (int)ldp->LD_CMD, MCIS_HBALD,
		    ISATTN_ICMD);
/* 				poll for assign command status 		*/
		mcis_pollret(mcis_blkp);

#ifdef MCIS_DEBUG
		if (mcis_blkp->mb_intr_code != ISINTR_ICMD_OK) {
			if(mcis_debug & DINIT)
			PRF("mcis_flush_cache: assign fail <targ %d lun %d>\n",
			ldp->LD_CB.a_targ,
			ldp->LD_CB.a_lunn);
		}
#endif
	}
	return (DDI_SUCCESS);
}

/*
 *	adapter command interface routine
 */
static int
mcis_docmd( struct mcis_blk *mcis_blkp, int cmd, unchar	dev, unchar opcode)
{
	register ushort	ioaddr;
	register int	i;
	register paddr_t outcmd;

	ioaddr = mcis_blkp->mb_ioaddr;
	outcmd = cmd;
	if (opcode !=ISATTN_ICMD)
		outcmd = MCIS_KVTOP(outcmd);

/*	check busy in status reg 					*/
	if (MCIS_CMDOUTWAIT(ioaddr)) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DCMD)
			PRF("mcis_docmd: board BUSY\n");
#endif
		return (1);
	}

	for (i=0; i<4 ; i++) {
		outb(ioaddr+MCIS_CMD+i,(outcmd&0xff));
		outcmd >>= 8;
	}

	outb(ioaddr+MCIS_ATTN,opcode|dev);

#ifdef MCIS_DEBUG
	if(mcis_debug & DCMD)
		PRF("mcis_docmd: cmd 0x%x opcode|dev 0x%x\n", 
		cmd, opcode|dev);
#endif
	return (0);
}

/*
 *	adapter ready quick wait routine
 */
mcis_qwait(ushort ioaddr,ushort mask,ushort onbits,ushort offbits)
{
	register ushort port;
	register int i;
	register ushort maskval; 

	port = ioaddr + MCIS_STAT; /* ask IBM about wait time - we hold a mutex */
	for (i=600000; i; i--) {
		maskval = inb(port) & mask;
		if (((maskval & onbits) == onbits) && ((maskval & offbits)== 0))
			return(0);
		drv_usecwait(10);
	}

	return(1);
}

/*
 *	adapter ready wait routine
 */
mcis_wait(ushort ioaddr,ushort mask,ushort onbits,ushort offbits)
{
	register ushort port;
	register int i;
	register ushort maskval; 

	port = ioaddr + MCIS_STAT; /* ask IBM about wait time - we hold a mutex */
	for (i=3000000; i; i--) {
		maskval = inb(port) & mask;
		if (((maskval & onbits) == onbits) && ((maskval & offbits)== 0))
			return(0);
		drv_usecwait(10);

/*	Workaround: fail after 10 seconds				*/
		if (mcis_indump && i < 2000000) {
			mcis_indump++;
			break;
		}
	}


	return(1);
}

static int
mcis_getedt(register struct mcis_blk *mcis_blkp)
{
	caddr_t		buf;
	int mem, targ, lun;
	struct	scsi_inquiry *inqp;
	struct mcis_ccb *ccbp;
	struct mcis_ldevmap *ldp;

/* 	allocate one ccb 						*/
	mem = sizeof(*ccbp) ;
	if (ddi_iopb_alloc(mcis_blkp->mb_dip, (ddi_dma_lim_t *)0,
		mem, &buf)) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DINIT)
			PRF("mcis_getedt: cannot allocate ccb memory.\n");
#endif
		return DDI_FAILURE;
	} 
	bzero((caddr_t)buf, mem);
	ccbp = (struct mcis_ccb *)buf;

/* 	allocate inquiry array 						*/
	mem = sizeof(struct scsi_inquiry) * (HBA_MAX_ATT_DEVICES + 8);
	if (ddi_iopb_alloc(mcis_blkp->mb_dip, (ddi_dma_lim_t *)0,
		mem, &buf)) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DINIT)
		PRF("mcis_getedt: cannot allocate inquiry memory.\n");
#endif
		return DDI_FAILURE;
	} 
	bzero((caddr_t)buf, mem);
	mcis_blkp->mb_inqp = (struct scsi_inquiry *)buf;

/* 	use head ldevmap in array for all non lun 0 combinations 	*/
	ldp = &mcis_blkp->mb_ldmhd; 
	ldp->LD_CMD  = 0;
	ldp->LD_CB.a_cmdop = 
		(ushort)((ISCMD_ICMD<<8)|ISCMD_ASSIGN);
	ldp->LD_CB.a_ld = MCIS_HBALD-1;

/* 	for each target and lun 					*/
	for (lun=0; lun < 8; lun++) {
		for (targ=0; targ < 8; targ++) {

/* 			set up inquiry command 				*/
			inqp = &(mcis_blkp->mb_inqp[targ<<3|lun]); 
			mcis_inqcmd(ccbp, (char *) inqp);
			inqp->inq_dtype = DTYPE_NOTPRESENT ;

			/* skip hba */
			if (targ==mcis_blkp->mb_targetid)
				continue;

			if (lun) {
/* 				request logical - physical assignment 	*/
				ldp->LD_CB.a_targ = targ;
				ldp->LD_CB.a_lunn  = lun;

				if (mcis_docmd(mcis_blkp, (int)ldp->LD_CMD, 
					MCIS_HBALD, ISATTN_ICMD)) {
					continue; 
				}

/* 				poll for assign command status 		*/
				mcis_pollret(mcis_blkp);

				if (mcis_blkp->mb_intr_code != ISINTR_ICMD_OK) {
#ifdef MCIS_DEBUG
					if(mcis_debug & DINIT)
					PRF("Assign fail <targ %d lun %d>\n",
						targ, lun);
#endif
					continue;
				}

/* 				send inquiry command 			*/
				if (mcis_docmd(mcis_blkp,(int)ccbp,MCIS_HBALD-1,
					ISATTN_SCB)) 
					continue;
			} else {
/* 				send inquiry command 			*/
				if (mcis_docmd(mcis_blkp, (int)ccbp, targ, 
					ISATTN_SCB)) 
					continue;
			}
	
			mcis_pollret(mcis_blkp);
			if ((mcis_blkp->mb_intr_code != ISINTR_SCB_OK) && 
				(mcis_blkp->mb_intr_code != ISINTR_SCB_OK2))
				inqp->inq_dtype = DTYPE_NOTPRESENT ;
			else if ((inqp->inq_dtype != DTYPE_NOTPRESENT) &&
				(inqp->inq_dtype != DTYPE_UNKNOWN)) 
				mcis_blkp->mb_numdev++;
		}
	}

#ifdef XXX
/* 	remove last assignment */
	ldp->LD_CB.a_rm = 1;		
	ldp->ld_cmdp = (struct scsi_cmd *)0;
	if (mcis_docmd(mcis_blkp, (int)ldp->LD_CMD, MCIS_HBALD, ISATTN_ICMD)) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DINIT)
		PRF("mcis_getedt: un-assign docmd fail\n");
#endif
	}
/* 			poll for assign command status 		*/
	mcis_pollret(mcis_blkp);

	if (mcis_blkp->mb_intr_code != ISINTR_ICMD_OK) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DINIT)
		PRF("mcis_getedt: un-assign fail\n");
#endif
	}
	ldp->LD_CB.a_rm = 0;		
#endif /* XXX end */

#ifdef MCIS_DEBUG
	if(mcis_debug & DINIT)
		PRF("Found %d scsi device(s)\n",mcis_blkp->mb_numdev);
#endif

/* 	free up ccb 							*/
	ddi_iopb_free((caddr_t)ccbp);

 	return DDI_SUCCESS;
}

/* returns 0 on success, 1 on failure */
static int
mcis_pollret(mcis_blkp)
register struct	mcis_blk *mcis_blkp;
{
	register ushort ioaddr;
	char	status;
	char	*sp = &status;

	ioaddr = mcis_blkp->mb_ioaddr;

/*	wait for interrupt						*/
	if (MCIS_INTRWAIT(ioaddr))
		return (1);
	
	status = inb(ioaddr+MCIS_INTR);
	mcis_blkp->mb_intr_code = MCIS_rintrp(sp)->ri_code;
	mcis_blkp->mb_intr_dev  = MCIS_rintrp(sp)->ri_ldevid;

	if (MCIS_BUSYWAIT(ioaddr))
		return (1);

	MCIS_SENDEOI(ioaddr, mcis_blkp->mb_intr_dev);

#ifdef MCIS_DEBUG
	if(mcis_debug & DINIT)
	PRF("mcis_pollret: icode= 0x%x idev= 0x%x\n",
		mcis_blkp->mb_intr_code, mcis_blkp->mb_intr_dev);
#endif
	return (0);
}

/*
 *	prepare an hba specific inquiry command
 */
static void
mcis_inqcmd(register struct mcis_ccb *ccbp, char *inqp)
{

	ccbp->ccb_cmdop  = ISCMD_DEVINQ;
	ccbp->ccb_cmdsup = ISCMD_SCB;
	ccbp->MSCB_opfld.ccb_nobuff = 1;
	ccbp->MSCB_opfld.ccb_ss = 1;
	ccbp->MSCB_opfld.ccb_retry = 1;
	ccbp->MSCB_opfld.ccb_es = 1;
	ccbp->MSCB_opfld.ccb_read = 1;
	ccbp->ccb_datap = MCIS_KVTOP(inqp);
	ccbp->ccb_datalen  = sizeof(struct scsi_inquiry);
	ccbp->ccb_tsbp = (paddr_t)MCIS_KVTOP(&ccbp->ccb_tsb);
}

/* Autovector Interrupt Entry Point */
/* Dummy return to be used before mutexes has been initialized		*/
/* guard against interrupts from drivers sharing the same irq line	*/
/*ARGSUSED*/
static u_int
mcis_dummy_intr(caddr_t arg)
{
	return (DDI_INTR_UNCLAIMED);
}

/*	Autovector Interrupt Entry Point				*/
static u_int
mcis_intr(caddr_t arg)
{
	register struct	mcis_blk *mcis_blkp= MCIS_BLKP(arg);
	register struct scsi_pkt *pktp;
	register ushort ioaddr;
	unchar status;

	ioaddr = mcis_blkp->mb_ioaddr;

	mutex_enter(&mcis_blkp->mb_mutex);
	if(!(inb(ioaddr+MCIS_STAT) & ISSTAT_INTRHERE)) {
		mutex_exit(&mcis_blkp->mb_mutex);
		return (DDI_INTR_UNCLAIMED);
	}

	status = inb(ioaddr+MCIS_INTR);

	if (MCIS_BUSYWAIT(ioaddr)) {
		mutex_exit(&mcis_blkp->mb_mutex);
		return (DDI_INTR_CLAIMED);
	}

	/* clear the interrupt */
	MCIS_SENDEOI(ioaddr, (status & 0xf));
	mutex_exit(&mcis_blkp->mb_mutex);

	pktp = mcis_chkstatus(mcis_blkp, &mcis_blkp->mb_ldm[status & 0xf],
			status >> 4);

	if(pktp) { 
		if((!(pktp->pkt_flags & FLAG_NOINTR)) && pktp->pkt_comp)
			scsi_run_cbthread(mcis_blkp->mb_cbthdl, 
			(struct scsi_cmd *)pktp);
	}

	return DDI_INTR_CLAIMED; 
}

/*
 * check packet status - returns NULL if not ready for callback 
 */
static struct scsi_pkt *
mcis_chkstatus(struct mcis_blk *mcis_blkp, register struct mcis_ldevmap *ldp, 
	unchar intr_code)
{
	register struct scsi_pkt *pktp;
	struct scsi_address *ap;

	pktp = (struct scsi_pkt *)ldp->ld_cmdp;

	switch(intr_code) {

		case ISINTR_SCB_OK:
		case ISINTR_SCB_OK2:

			if(pktp) {
				*pktp->pkt_scbp  = STATUS_GOOD;
				pktp->pkt_reason = CMD_CMPLT;
				pktp->pkt_resid  = 0;
				pktp->pkt_state = 
					(STATE_XFERRED_DATA|STATE_GOT_BUS|
					STATE_GOT_TARGET|STATE_SENT_CMD|
					STATE_GOT_STATUS);
			}
			break;

		case ISINTR_ICMD_OK:
/*			check for assign command 			*/
			if (ldp->ld_status & MSCB_WAITLD) {
				mcis_blkp->mb_ldm[MCIS_HBALD].ld_cmdp = 
					(struct scsi_cmd *)0;
				ldp->ld_status &= ~MSCB_WAITLD;
				ldp->ld_status |= (MSCB_GOTLD);
				ldp->ld_mcisp->m_ldp = ldp;
				ap = &pktp->pkt_address;
				if(mcis_transport(ap, pktp) == TRAN_ACCEPT)
					return NULL;
			}

/* 			immediate commands not accompanied 
			by a packet must have null in ldp->ld_cmdp	*/

			if(pktp) {
				*pktp->pkt_scbp  = STATUS_GOOD;
				pktp->pkt_reason = CMD_CMPLT;
				pktp->pkt_resid  = 0;
				pktp->pkt_state = 
					(STATE_XFERRED_DATA|STATE_GOT_BUS|
					STATE_GOT_TARGET|STATE_SENT_CMD|STATE_GOT_STATUS);
			}
			break;

		case ISINTR_CMD_FAIL:
			if(pktp)
				mcis_seterr(pktp);
			break;

		case ISINTR_HBA_FAIL:
			if(pktp)
				pktp->pkt_reason = CMD_ABORTED;
#ifdef MCIS_DEBUG
			if(mcis_debug & DINTR) {
				PRF("mcis_chkstatus: hba failure\n");
				PRF("pkt at 0x%x\n",pktp);
			}
#endif
			break; 

		case ISINTR_CMD_INV:
#ifdef MCIS_DEBUG
			if(mcis_debug & DINTR) {
				PRF("mcis_chkstatus: bad cmd at 0x%x to hba\n",
					pktp);
			}
#endif
			if(pktp) 
				pktp->pkt_reason = CMD_ABORTED;
			break;

		case ISINTR_SEQ_ERR:
#ifdef MCIS_DEBUG
			if(mcis_debug & DINTR) {
				PRF("mcis_chkstatus: sw sequencing error\n");
			}
#endif
			if(pktp)  {
				pktp->pkt_reason = CMD_CMPLT;
				*pktp->pkt_scbp  = STATUS_CHECK;
			}
			break;

		default:
#ifdef MCIS_DEBUG
			if(mcis_debug & DINTR) {
				PRF("mcis_chkstatus intr code 0x%x\n",intr_code);
			}
#endif
			if(pktp)  {
				pktp->pkt_reason = CMD_TRAN_ERR;
				*pktp->pkt_scbp  = STATUS_CHECK;
			}
			break;
	}

	mutex_enter(&mcis_blkp->mb_mutex);
	ASSERT(ldp->ld_busy > 0);
	ldp->ld_busy--;

	if (!(ldp->ld_status & MSCB_OWNLD)) {
		mcis_freeld(mcis_blkp,ldp);
	}
	mutex_exit(&mcis_blkp->mb_mutex);

	return pktp;
}

static void
mcis_seterr(register struct scsi_pkt *pktp)
{
	register struct	mcis_tsb *tsbp;
	struct scsi_cmd *cmd = (struct scsi_cmd *) pktp;
	struct mcis_ccb *ccbp;

	ccbp = (struct mcis_ccb *)cmd->cmd_private;
	tsbp = &ccbp->ccb_tsb;

	pktp->pkt_state = 0;

#ifdef MCIS_DEBUG
	if (mcis_debug & DINTR)
		PRF("mcis_seterr: tstat 0x%x terr 0x%x hbaerr 0x%x cmd 0x%x\n",
			tsbp->t_targstat << 1, tsbp->t_targerr, 
			tsbp->t_haerr, (*(pktp->pkt_cdbp)));

#endif

/*	record target device status				*/
	*pktp->pkt_scbp = tsbp->t_targstat << 1; 

	if (!tsbp->t_haerr && !tsbp->t_targerr && 
		!tsbp->t_targstat) {
		pktp->pkt_reason= CMD_CMPLT;
		pktp->pkt_state |= 
				(STATE_GOT_BUS| STATE_GOT_TARGET|
				STATE_SENT_CMD|STATE_GOT_STATUS);
	} else if (tsbp->t_haerr) {
		switch(tsbp->t_haerr) {
			case MCIS_HAERR_END:
			case MCIS_HAERR_END16:
				*pktp->pkt_scbp  = STATUS_CHECK;
				pktp->pkt_reason= CMD_CMPLT;
				break;

			case MCIS_HAERR_DMA:
				pktp->pkt_reason= CMD_DMA_DERR;
				break;

			case MCIS_HAERR_BADCCB:
				pktp->pkt_reason = CMD_TRAN_ERR;
#ifdef MCIS_DEBUG
				if(mcis_debug & DINTR) {
					PRF("ccb rejected by hba\n");
				}
#endif
				break;

			case MCIS_HAERR_BADDEV:
				pktp->pkt_reason = CMD_TRAN_ERR;
#ifdef MCIS_DEBUG
				if(mcis_debug & DINTR) {
					PRF("Invalid device for command\n");
				}
#endif
				break;

			case MCIS_HAERR_BADCMD:
				pktp->pkt_reason = CMD_TRAN_ERR;
#ifdef MCIS_DEBUG
				if(mcis_debug & DINTR) {
					PRF("Command not supported by hba\n");
				}
#endif
				break;

			case MCIS_HAERR_TIMEOUT:
				pktp->pkt_reason = CMD_TRAN_ERR;
				pktp->pkt_statistics |= STAT_TIMEOUT; 
#ifdef MCIS_DEBUG
				if(mcis_debug & DINTR) {
					PRF("Controller initiated timeout\n");
				}
#endif
				break;

			case MCIS_HAERR_SYSABORT:
			case MCIS_HAERR_ABORT:
				pktp->pkt_reason = CMD_ABORTED;
				pktp->pkt_statistics |= STAT_ABORTED;
				break;

			case MCIS_HAERR_BADLD:
				pktp->pkt_reason = CMD_TRAN_ERR;
#ifdef MCIS_DEBUG
				if(mcis_debug & DINTR) {
					PRF("Logical device not mapped\n");
				}
#endif
				break;

			default:
				pktp->pkt_reason = CMD_TRAN_ERR;
				break;
		}

	} else {
		switch (tsbp->t_targerr) {

			case MCIS_TAERR_BUSRESET:
				pktp->pkt_statistics |= STAT_BUS_RESET;
				break;

			case MCIS_TAERR_SELTO:
				pktp->pkt_statistics |= STAT_TIMEOUT;
				break;


			default:
				break;
		}
		pktp->pkt_reason = CMD_CMPLT;
		*pktp->pkt_scbp  = STATUS_CHECK;
	}

	/* restore part of the command changed by vendor specific cmd */
	if(ccbp->ccb_cmdop  == ISCMD_READ || ccbp->ccb_cmdop == ISCMD_WRITE) {
		*(uint *)ccbp->ccb_cdb = ccbp->ccb_scsi_op0;
		*(uint *)&ccbp->ccb_cdb[4] =  ccbp->ccb_scsi_op1;
	}
}

/*
 *	set up a pos command
 */
static void
mcis_poscmd(register struct mcis_ccb *ccbp, struct mcis_pos *datap)
{

	ccbp->ccb_cmdop  = ISCMD_GETPOS;
	ccbp->ccb_cmdsup = ISCMD_SCB;
	ccbp->MSCB_opfld.ccb_nobuff = 1;
	ccbp->MSCB_opfld.ccb_retry = 1;
	ccbp->MSCB_opfld.ccb_es = 1;
	ccbp->MSCB_opfld.ccb_read = 1;
	ccbp->ccb_datap = MCIS_KVTOP(datap);
	ccbp->ccb_datalen  = sizeof(struct mcis_pos);
	ccbp->ccb_tsbp = MCIS_KVTOP(&ccbp->ccb_tsb);
}

static void
mcis_set_boottarg(register struct mcis_blk *mcis_blkp)
{
	int targ;
	struct scsi_inquiry *inqp;

/*	look for first disk device, start counting down from target 7	*/
	for (targ=7; targ>0; targ--) {

/* 		skip hba 						*/
		if (targ==mcis_blkp->mb_targetid)
			continue;

		inqp = &(mcis_blkp->mb_inqp[targ<<3]); 
		if (inqp->inq_dtype == DTYPE_DIRECT) {
			mcis_blkp->mb_boottarg = (unchar)targ;
#ifdef MCIS_DEBUG
			if (mcis_debug & DINIT)  {
				PRF("mcis_set_boottarg: boottarg %d\n",targ);
				PRF("mb_boottarg set to %d\n", 
					mcis_blkp->mb_boottarg);
			}
#endif
			break;
		}
	}
}

/*
 *	initialize the logical device map
 */
static void
mcis_initld(struct mcis_blk *mcis_blkp)
{
	register struct	mcis_ldevmap *dp;
	register struct	mcis_ldevmap *lp;
	int	i;
	int	maxld;
	int	targ;

	dp = &(mcis_blkp->mb_ldmhd);
	dp->ld_avfw = dp->ld_avbk = dp;
	maxld = mcis_blkp->mb_ldcnt;

	lp = mcis_blkp->mb_ldm;
	for (targ=0,i=0; i<maxld; i++, lp++) {
		lp->ld_avbk = dp;
		lp->ld_avfw = dp->ld_avfw;
		dp->ld_avfw->ld_avbk = lp;
		dp->ld_avfw = lp;
		lp->LD_CMD  = 0;
		lp->LD_CB.a_cmdop = (ushort)((ISCMD_ICMD<<8)|ISCMD_ASSIGN);
		lp->ld_busy = 0;

/*		targets 0 to 6 have been assigned by the firmware	*/
		if (targ >= 8)
			continue;
		lp->LD_CB.a_targ = targ;
		lp->LD_CB.a_lunn  = 0;
		lp->LD_CB.a_ld = i;
		targ++;
	}
}

/*
 *	Get logical device mapping routine
 *	Normally called from mcis_transport with nowait == 1
 *	Can be called with nowait == 0 from mcis_reset()
 */
static int
mcis_getld(struct mcis_blk *mcis_blkp, struct scsi_pkt *pktp, struct mcis *mcisp, int nowait)
{
	register struct	mcis_ldevmap *ldp;
	int	 avld = -1;

	ldp = mcisp->m_ldp;

	mutex_enter(&mcis_blkp->mb_mutex);

	if (ldp) { 
/* 		we have logical device connection 			*/
		avld = ldp->LD_CB.a_ld;
		if(ldp->ld_status & MSCB_OWNLD) {
			ldp->ld_mcisp = mcisp;
			mutex_exit(&mcis_blkp->mb_mutex);
			return avld;
		}
	} else { 
/*		allocate a new logical device number			*/
		ldp = ldmhd_fw(mcis_blkp);

/*		check for empty list					*/
		if (ldp == &(mcis_blkp->mb_ldmhd)) {
#ifdef MCIS_DEBUG
			if(mcis_debug & DPKT)
				PRF("mcis_getld: empty ld list\n");
#endif
			mutex_exit(&mcis_blkp->mb_mutex);
			return (MCIS_HBALD);
		}
	}

	ldp->ld_avbk->ld_avfw = ldp->ld_avfw;
	ldp->ld_avfw->ld_avbk = ldp->ld_avbk;
	
	ldp->ld_avbk    = NULL;
	ldp->ld_avfw    = NULL;

/*	the WAITLD request will receive special handling in chkstatus	*/
	ldp->ld_cmdp	= (struct scsi_cmd *)pktp;

	ldp->ld_mcisp	= mcisp;
	if (avld != -1) {
		mutex_exit(&mcis_blkp->mb_mutex);
		return (avld);
	}

/*	release old ld - device nexus					*/
	ldp->ld_mcisp->m_ldp = (struct mcis_ldevmap *)0;
/*	insert new nexus						*/
	ldp->ld_mcisp = mcisp; 

/*	set up for assign command					*/
	ldp->LD_CB.a_targ = pktp->pkt_address.a_target;
	ldp->LD_CB.a_lunn  = pktp->pkt_address.a_lun;

	if (mcis_docmd(mcis_blkp, ldp->LD_CMD, MCIS_HBALD, ISATTN_ICMD)) {
		mcis_freeld(mcis_blkp, ldp);
#ifdef MCIS_DEBUG
			if(mcis_debug & DPKT)
			PRF("getld: assign fail\n");
#endif
		mutex_exit(&mcis_blkp->mb_mutex);
		return (MCIS_HBALD);
	}

	if (nowait) {
		ldp->ld_status |= MSCB_WAITLD; 
		mcis_blkp->mb_ldm[MCIS_HBALD].ld_cmdp = (struct scsi_cmd *)pktp;
		mutex_exit(&mcis_blkp->mb_mutex);
		return (avld);
	}

/*	wait for return status if specified - in device initialization	*/
	if ((mcis_pollret(mcis_blkp))
		|| (mcis_blkp->mb_intr_code!=ISINTR_ICMD_OK)) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DPKT)
			PRF("mcis_getld: fail = 0x%x retcod=0x%x\n",
			ldp->LD_CMD, mcis_blkp->mb_intr_code);
#endif
		mcis_freeld(mcis_blkp, ldp);
		mutex_exit(&mcis_blkp->mb_mutex);
		return(MCIS_HBALD);
	}

	mutex_exit(&mcis_blkp->mb_mutex);
	return (ldp->LD_CB.a_ld);
}

/*
 *	free logical device mapping
 *	this routine is protected by a mutex
 */
static void
mcis_freeld(mcis_blkp, ldp)
register struct	mcis_blk *mcis_blkp;
register struct	mcis_ldevmap *ldp;
{
	register struct	mcis_ldevmap *dp;

	dp = &(mcis_blkp->mb_ldmhd);
	dp->ld_avbk->ld_avfw = ldp;
	ldp->ld_avbk = dp->ld_avbk;
	dp->ld_avbk = ldp;
	ldp->ld_avfw = dp;

	/* Don't null out ldp->ld_mcisp->m_ldp so the ld can be 
	reused if not taken by another prior to entry into getld */

	ldp->ld_mcisp = (struct mcis *)0;

	return; 
}

/* set pointers between mcis structure and ldevmap if valid 
 *	and take ldevmap node off free list
 */
static struct mcis_ldevmap *
mcis_map_target( struct mcis_blk *mcis_blkp, struct mcis *mcisp, int targ, int lun)
{
	register struct	mcis_ldevmap *ldp;
	register int i;

	ldp = mcis_blkp->mb_ldm;

#ifdef MCIS_DEBUG
	if(mcis_debug & DINIT) {
		PRF("mcis_map_target for targ %d lun %d blkp 0x%x mcis 0x%x\n",
			targ, lun, mcis_blkp, mcisp);
		PRF("ldevmap at 0x%x\n",ldp);
	}
#endif
	mutex_enter(&mcis_blkp->mb_mutex);

/* 	is device still mapped ,i.e. (targ/lun <-> ld) intact? 		*/
	for(i=0;i < MCIS_MAX_PREMAP; i++,ldp++) {
		if( ldp->LD_CB.a_targ == targ && ldp->LD_CB.a_lunn == lun) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DINIT)
			PRF("ldev %d at 0x%x status 0x%x mcis 0x%x\n",i,
				ldp,ldp->ld_status,mcisp);
#endif

			mcisp->m_ldp = ldp;
			ldp->ld_mcisp = mcisp;

/* 			nail down the nexus 				*/
			ldp->ld_status = MSCB_OWNLD;
/* 			take it off the free list if on  		*/
			if(ldp->ld_avbk) {
				ldp->ld_avbk->ld_avfw = ldp->ld_avfw;
				ldp->ld_avfw->ld_avbk = ldp->ld_avbk;
				ldp->ld_avbk     = NULL;
				ldp->ld_avfw     = NULL;
			}
			break;
		}
	}
	mutex_exit(&mcis_blkp->mb_mutex);

	if(i >= mcis_blkp->mb_ldcnt)
		return ((struct mcis_ldevmap *)NULL);
	return ldp;
}

static int 
mcis_abort(struct scsi_address *ap, struct scsi_pkt *pktp) 
{
	struct	mcis_blk *mcis_blkp;
	struct   mcis *mcisp;
	struct mcis_ccb *ccbp;

	if(!pktp)
		return 1;

	ccbp = (struct mcis_ccb *)SCMD_PKTP(pktp)->cmd_private;
	mcis_blkp = ADDR2MCISBLKP(ap);
	mcisp = PKT2MCIS(pktp);
	
	if(!mcisp->m_ldp)
		return 1;
/*		the interrupt has already come in and nexus is lost 	*/

/* 	run callback thread from interrupt 				*/
	mcisp->m_ldp->ld_cmdp = (struct scsi_cmd *) pktp;

	ccbp->ccb_cmdop  = ISCMD_ABORT;
	ccbp->ccb_cmdsup = ISCMD_ICMD;
	ccbp->MSCB_opfld.ccb_nobuff = 1;
	ccbp->MSCB_opfld.ccb_ss = 1;
	ccbp->MSCB_opfld.ccb_retry = 0;
	ccbp->MSCB_opfld.ccb_es = 1;
	ccbp->MSCB_opfld.ccb_read = 0;

	mutex_enter(&mcis_blkp->mb_mutex);

	if (mcis_docmd(mcis_blkp, (int)ccbp, mcisp->m_ldp->LD_CB.a_ld,
		ISATTN_ICMD)) {

#ifdef MCIS_DEBUG
		if(mcis_debug & DINTR)
			PRF("Abort command failure\n");
#endif
		
		mutex_exit(&mcis_blkp->mb_mutex);
		return 0;
	}

	mutex_exit(&mcis_blkp->mb_mutex);
	return 1;
}

static int 
mcis_reset(struct scsi_address *ap, int level) 
{
	struct	mcis_blk *mcis_blkp;
	struct  scsi_pkt *pktp;
	struct  mcis_ccb *ccbp;
	struct   mcis *mcisp;
	int	ld;
	int	status = 1;

	mcis_blkp = ADDR2MCISBLKP(ap);

	switch (level) {

		case RESET_ALL:
			ld = MCIS_HBALD;
			/* XXX or use CTLR reset from findhba */
			pktp = mcis_pktalloc(ap, 6, 1, 0,
				DDI_DMA_DONTWAIT, 0);
			if (!pktp) {
				/*
				 * Need to handle this failure better,
				 * but this is better than a panic.
				 */
				cmn_err(CE_WARN, "mcis_reset: failed\n");
				return (0);
			}
			break;

		case RESET_TARGET:
			pktp = mcis_pktalloc(ap, 6, 1, 0,
				DDI_DMA_DONTWAIT, 0);
			if (!pktp) {
				/*
				 * Need to handle this failure better,
				 * but this is better than a panic.
				 */
				cmn_err(CE_WARN, "mcis_reset: failed\n");
				return (0);
			}

			mcisp = PKT2MCIS(pktp);
			if(mcisp->m_ldp)
				ld = mcisp->m_ldp->LD_CB.a_ld;
			else {
/*				get an ld without waiting		*/
				ld = mcis_getld(mcis_blkp, pktp, mcisp, 0);
/*				do not run callback thread from intr	*/
				mcis_blkp->mb_ldm[ld].ld_cmdp = 
					(struct scsi_cmd *)0;
			}
			break;

		default:
			return 0;
	}

	ccbp = (struct mcis_ccb *)SCMD_PKTP(pktp)->cmd_private;
	ccbp->ccb_cmdop  = ISCMD_RESET;
	ccbp->ccb_cmdsup = ISCMD_ICMD;
	ccbp->MSCB_opfld.ccb_nobuff = 1;
	ccbp->MSCB_opfld.ccb_ss = 1;
	ccbp->MSCB_opfld.ccb_retry = 0;
	ccbp->MSCB_opfld.ccb_es = 1;
	ccbp->MSCB_opfld.ccb_read = 0;

	mutex_enter(&mcis_blkp->mb_mutex);

	if (mcis_docmd(mcis_blkp, (int)ccbp, ld, ISATTN_ICMD)) {
#ifdef MCIS_DEBUG
		if(mcis_debug & DINTR)
			PRF("Reset command failure\n");
#endif
		status = 0;
	}

	mutex_exit(&mcis_blkp->mb_mutex);

	mcis_pktfree(ap, pktp);

	return status;
}

#ifdef MCIS_DEBUG

char *mcis_err_strings[] = {
	"No Error", 				/* 0x00 */
	"Invalid Error Code",			/* 0x01 */
	"Invalid Error Code",			/* 0x02 */
	"Invalid Error Code",			/* 0x03 */
	"Invalid Error Code",			/* 0x04 */
	"OK after retry",			/* 0x05 */
	"Invalid Error Code",			/* 0x06 */
	"HBA Harwdare Failure",			/* 0x07 */
	"Invalid Error Code",			/* 0x08 */
	"Invalid Error Code",			/* 0x09 */
	"Immediate Cmd OK",			/* 0x0a */
	"Invalid Error Code",			/* 0x0b */
	"Command Complete with Failure",	/* 0x0c */
	"Invalid Error Code",			/* 0x0d */
	"Invalid Command",			/* 0x0e */
	"SW Sequencing Error"			/* 0x0f */
};

void
mcis_dump_ccb(struct mcis_ccb *p)
{
	int index;
	struct scsi_cmd *cmdp;

	PRF("cmdop 0x%x ccb_ch 0x%x ccb_ops.ccb_op 0x%x addr ccb_status 0x%x\n",
		p->ccb_cmdop & 0xff, p->ccb_ch & 0xff, p->ccb_ops.ccb_op,
		&p->ccb_status);

	if(p->MSCB_opfld.ccb_nobuff)
		PRF(" Buffer off\n");
	else
		PRF(" Buffer on\n");

	if(p->MSCB_opfld.ccb_scatgath)
		PRF(" Scatter gather\n");
	else
		PRF(" No scatter gather\n");

	if(p->MSCB_opfld.ccb_retry)
		PRF(" Retry on\n");
	else
		PRF(" Retry off\n");

	if(p->MSCB_opfld.ccb_ss)
		PRF(" Short read OK\n");
	else
		PRF(" Short read not OKn\n");

	PRF("logical blk addr 0x%x datap 0x%x datalen 0x%x tsb 0x%x flag 0x%x cmdp 0x%x\n",
		p->ccb_baddr, p->ccb_datap, p->ccb_datalen, p->ccb_tsbp,
		p->ccb_flag, p->ccb_ownerp);

	cmdp = (struct scsi_cmd *) p->ccb_ownerp;
	if (p->ccb_datalen) {

		if(p->MSCB_opfld.ccb_read)
			PRF(" Read\n");
		else
			PRF(" Write\n");

		if(p->MSCB_opfld.ccb_scatgath) {
			for(index=0;index < MCIS_MAX_DMA_SEGS ;index++) {
			if(!((ulong )p->ccb_sg_list[index].data_ptr))
					break;
				PRF(" a:0x%x l:0x%x",
			p->ccb_sg_list[index].data_ptr,
			p->ccb_sg_list[index].data_len);
			}
		} else {
		PRF("logical block addr 0x%x datap 0x%x datalen 0x%x tsb 0x%x\n",
			p->ccb_baddr, p->ccb_datap, p->ccb_datalen, p->ccb_tsbp);
		}
	} else
		PRF("No data xfer\n");

}

void
mcis_dump_mcisblk(struct mcis_blk *p)
{
	int index;

	PRF("numdev %d flag 0x%x targetid 0x%x boottarg %d intr 0x%x\n",
		p->mb_numdev & 0xff, p->mb_flag & 0xff, 
		p->mb_targetid & 0xff, p->mb_boottarg & 0xff,
		p->mb_intr & 0xff);
	PRF("mb_intr_code 0x%x mb_intr_dev 0x%x\n",
		p->mb_intr_code, p->mb_intr_dev);
	PRF("dip 0x%x ioaddr 0x%x inqp 0x%x ldevmap hd 0x%x cache %s \n",
		p->mb_dip, p->mb_ioaddr, p->mb_inqp,&p->mb_ldmhd,
		p->mb_flag & MCIS_CACHE ? "on" : "off");

	if(p->mb_intr_code) {
		index = (int) p->mb_intr_code & 0xff;
		if (index < 0 || index > ISINTR_SEQ_ERR) 
			PRF("Bad intr code 0x%x\n",index);
		else
			PRF("\nintr_code %s \n", mcis_err_strings[index]);
	}
}

void 
mcis_dump_ldp(struct mcis_ldevmap *l)
{
	PRF("ldevmap at 0x%x avfw 0x%x avbk 0x%x cmd 0x%x mcisp 0x%x\n",
		l, l->ld_avfw, l->ld_avbk, l->ld_cmd, l->ld_mcisp);
	PRF("targ %d lun %d status 0x%x ",l->LD_CB.a_targ, l->LD_CB.a_lunn,
		l->ld_status);

	if(l->ld_status == 0)
		PRF("Null\n");
	if(l->ld_status & MSCB_OWNLD)
		PRF("MSCB_OWNLD\n");
	if(l->ld_status & MSCB_WAITLD)
		PRF("MSCB_WAITLD\n");
	if(l->ld_status & MSCB_GOTLD)
		PRF("MSCB_GOTLD\n");
	if(l->ld_mcisp)
		PRF("mcisp->mcis.m_ldp 0x%x\n",
			l->ld_mcisp->m_ldp);
}

void 
mcis_dump_mcis(struct mcis *m)
{
	PRF("mcis_blk 0x%x unit 0x%x ldevmap 0x%x ",
		m->m_blkp, m->m_unitp, m->m_ldp);
	if(m->m_unitp->mu_disk)
		PRF("A disk with 0x%x sectors\n",
		m->m_unitp->mu_tot_sects);
	else
		PRF("\n");
}
#endif
