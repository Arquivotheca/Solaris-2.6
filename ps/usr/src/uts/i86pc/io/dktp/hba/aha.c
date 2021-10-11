/*
 * Copyright (c) 1992-96, Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)aha.c	1.68	96/08/29 SMI"

#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#include <sys/dktp/aha.h>
#include <sys/debug.h>

/*
 * External references
 */

static int aha_tran_tgt_init(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
static int aha_tran_tgt_probe(struct scsi_device *, int (*)());
static void aha_tran_tgt_free(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);

static int aha_transport(struct scsi_address *ap, struct scsi_pkt *pktp);
static int aha_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static int aha_reset(struct scsi_address *ap, int level);
static int aha_capchk(char *cap, int tgtonly, int *cidxp);
static int aha_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static int aha_setcap(struct scsi_address *ap, char *cap, int value,
		int tgtonly);
static struct scsi_pkt *aha_tran_init_pkt(struct scsi_address *ap,
	struct scsi_pkt *pkt, struct buf *bp, int cmdlen, int statuslen,
	int tgtlen, int flags, int (*callback)(), caddr_t arg);
static void aha_tran_destroy_pkt(struct scsi_address *ap,
		struct scsi_pkt *pkt);
static struct scsi_pkt *aha_pktalloc(struct scsi_address *ap, int cmdlen,
	int statuslen, int tgtlen, int (*callback)(), caddr_t arg);
static void aha_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt);
static struct scsi_pkt *aha_dmaget(struct scsi_pkt *pkt, opaque_t dmatoken,
	int (*callback)(), caddr_t arg);
static void aha_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);
static void aha_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);

/*
 * Local Function Prototypes
 */
static int aha_findhba(uint ioaddr);
static int aha_propinit(register struct aha_blk *aha_blkp);
static int aha_cfginit(struct  aha_blk *aha_blkp);
static int aha_mboxunlock(register struct aha_blk *aha_blkp);
static void aha_mboxinit(struct aha_blk *aha_blkp);
static void aha_scsicmd(struct aha_blk *aha_blkp);
static int aha_docmd(struct aha_blk *aha_blkp, int opcode, unchar *sbp,
		unchar *rbp);
static int aha_wait(ushort port, ushort mask, ushort onbits, ushort offbits);
static int aha_setup_inq(register struct aha_blk *aha_blkp);
static int aha_getedt(struct aha_blk *aha_blkp);
static int aha_poll(struct aha_blk *aha_blkp);
static void aha_inmbx(struct aha_blk *aha_blkp, int stop_1stmiss);
static int aha_outmbx(struct aha_blk *aha_blkp, struct aha_ccb *ccbp);
static struct scsi_pkt *aha_chkerr(struct aha_blk *aha_blkp);
static void aha_pollret(struct aha_blk *aha_blkp, struct scsi_pkt *pktp);
static void aha_saveccb(struct aha_blk *aha_blkp, struct aha_ccb *ccbp);
static struct aha_ccb *aha_retccb(struct aha_blk *aha_blkp, ulong ai);
static void aha_inqcmd(struct aha_ccb *ccbp, caddr_t bp, int targ, int lun);
static u_int aha_xlate_vec(struct aha_blk *aha_blkp);
static u_int aha_intr(caddr_t arg);
static void aha_seterr(struct scsi_pkt *pktp, struct aha_ccb *ccbp);
static int aha_find_1640irq(struct aha_blk *aha_blkp);
static int aha_find_ioaddridx(ushort ioaddr);

/*
 * Local static data
 */
static int aha_pgsz = 0;
static int aha_pgmsk;
static int aha_pgshf;

static int aha_cb_id = 0;
static kmutex_t aha_rmutex;
static kmutex_t aha_global_mutex;
static int aha_global_init = 0;

static ddi_dma_lim_t aha_dma_lim17 = {
	0,			/* address low				*/
	0x00ffffff,		/* address high				*/
	0,			/* counter max				*/
	1,			/* burstsize 				*/
	DMA_UNIT_8,		/* minimum xfer				*/
	0,			/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version				*/
	0x00ffffff,		/* address register			*/
	0x00007fff,		/* counter register			*/
	512,			/* sector size				*/
	AHA_17_DMA_SEGS,	/* scatter/gather list length		*/
	(u_int)0xffffffff	/* request size				*/
};

static ddi_dma_lim_t aha_dma_lim255 = {
	0,			/* address low				*/
	0x00ffffff,		/* address high				*/
	0,			/* counter max				*/
	1,			/* burstsize 				*/
	DMA_UNIT_8,		/* minimum xfer				*/
	0,			/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version				*/
	0x00ffffff,		/* address register			*/
	0x00007fff,		/* counter register			*/
	512,			/* sector size				*/
	AHA_255_DMA_SEGS,	/* scatter/gather list length		*/
	(u_int)0xffffffff	/* request size				*/
};

/*
 * ac_flags bits:
 */
#define	ACF_WAITIDLE    0x01    /* Wait for STAT_IDLE before issuing */
#define	ACF_WAITCPLTE   0x02    /* Wait for INT_HACC before returning */
#define	ACF_INVDCMD	0x04    /* INVALID COMMAND CODE */

struct aha_cmd aha_cmds[] = {
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 0}, /* 00 - CMD_NOP */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 4, 0}, /* 01 - CMD_MBXINIT */
	{0, 0, 0},				/* 02 - CMD_DOSCSI */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 10, 1},	/* 03 - CMD_ATBIOS */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 4}, /* 04 - CMD_ADINQ */
	{ACF_WAITCPLTE, 1, 0},			/* 05 - CMD_MBOE_CTL */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 4, 0}, /* 06 - CMD_SELTO_CTL */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 0}, /* 07 - CMD_BONTIME */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 0}, /* 08 - CMD_BOFFTIME */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 0}, /* 09 - CMD_XFERSPEED */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 8}, /* 0a - CMD_INSTDEV */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 3}, /* 0b - CMD_CONFIG */
	{ACF_INVDCMD, 0, 0},			/* 0c -- INVALID */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 17},	/* 0d - CMD_GETCFG */
	{ACF_INVDCMD, 0, 0},			/* 0e -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 0f -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 10 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 11 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 12 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 13 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 14 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 15 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 16 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 17 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 18 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 19 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 1a -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 1b -- INVALID */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 3, 0}, /* 1c - CMD_WTFIFO */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 3, 0}, /* 1d -- CMD_RDFIFO */
	{ACF_INVDCMD, 0, 0},			/* 1e -- INVALID */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 1}, /* 1f - CMD_ECHO */
	{ACF_INVDCMD, 0, 0},			/* 20 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 21 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 22 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 23 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 24 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 25 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 26 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 27 -- INVALID */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 2},   /* 28 -- CMD_EXTBIOS */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 2, 0},   /* 29 -- CMD_MBXUNLK */
};


struct aha_addr_code aha_codes[] = {	/* 1640 codes vs io port addresses */
	{ 0x330, AHA_IOADDR_330 },
	{ 0x334, AHA_IOADDR_334 },
	{ 0x130, AHA_IOADDR_130	},
	{ 0x134, AHA_IOADDR_134 },
	{ 0x230, AHA_IOADDR_230 },
	{ 0x234, AHA_IOADDR_234 }

};

#ifdef	AHA_DEBUG
#define	DENT	0x0001
#define	DPKT	0x0002
#define	DINM	0x0004
#define	DOUTM	0x0008
#define	DINIT	0x0010
#define	DRESID  0x0020
static	int	aha_debug = DOUTM;

#endif	/* AHA_DEBUG */



static int aha_identify(dev_info_t *dev);
static int aha_probe(dev_info_t *);
static int aha_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int aha_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

struct dev_ops	aha_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	aha_identify,		/* identify */
	aha_probe,		/* probe */
	aha_attach,		/* attach */
	aha_detach,		/* detach */
	nulldev,		/* reset */
	(struct cb_ops *)0,	/* driver operations */
	NULL			/* bus operations */
};


#include <sys/modctl.h>

char _depends_on[] = "misc/scsi drv/rootnex";

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"AHA SCSI Host Adapter Driver",	/* Name of the module. */
	&aha_ops,	/* driver ops */
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

	mutex_init(&aha_global_mutex, "AHA global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&aha_global_mutex);
	}
	return (status);
}

int
_fini(void)
{
	int	status;

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&aha_global_mutex);
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
aha_tran_tgt_init(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	int 	targ;
	int	lun;
	struct 	aha *hba_ahap;
	struct 	aha *unit_ahap;
	struct	aha_blk *aha_blkp;
	struct  scsi_inquiry *inqdp;

	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef AHA_DEBUG
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

	hba_ahap = SDEV2HBA(sd);
	aha_blkp = AHA_BLKP(hba_ahap);
	inqdp = &(hba_ahap->a_blkp->ab_inqp[targ << 3|lun]);

	if ((unit_ahap = kmem_zalloc(
		sizeof (struct aha) + sizeof (struct aha_unit),
			KM_NOSLEEP)) == NULL) {
		return (DDI_FAILURE);
	}

	bcopy((caddr_t)hba_ahap, (caddr_t)unit_ahap, sizeof (*hba_ahap));
	unit_ahap->a_unitp = (struct aha_unit *)(unit_ahap+1);
	unit_ahap->a_unitp->au_lim = *(aha_blkp->ab_dma_lim);

	sd->sd_inq = inqdp;

	hba_tran->tran_tgt_private = unit_ahap;

	/*
	 * update xfer request size max for sequential (tape) devices
	 */
	if ((hba_ahap->a_blkp->ab_dma_reqsize) &&
	    (inqdp->inq_dtype != DTYPE_SEQUENTIAL)) {
		unit_ahap->a_unitp->au_lim.dlim_reqsize =
			hba_ahap->a_blkp->ab_dma_reqsize;
	}

	AHA_BLKP(hba_ahap)->ab_child++;   /* increment active child cnt   */

#ifdef AHA_DEBUG
	if (aha_debug & DENT) {
		PRF("aha_tran_tgt_init: <%d,%d>\n", targ, lun);
	}
#endif
	return (DDI_SUCCESS);
}


/*ARGSUSED*/
static int
aha_tran_tgt_probe(
	struct scsi_device	*sd,
	int			(*callback)())
{
	int	rval;

	rval = scsi_hba_probe(sd, callback);

#ifdef AHA_DEBUG
	{
		char		*s;
		struct aha	*aha = SDEV2AHA(sd);

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
		cmn_err(CE_CONT, "aha%d: %s target %d lun %d %s\n",
			ddi_get_instance(AHA_DIP(aha)),
			ddi_get_name(sd->sd_dev),
			sd->sd_address.a_target,
			sd->sd_address.a_lun, s);
	}
#endif	/* AHADEBUG */

	return (rval);
}


/*ARGSUSED*/
static void
aha_tran_tgt_free(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	struct aha		*aha;
	struct aha		*unit_ahap;

#ifdef	AHADEBUG
	cmn_err(CE_CONT, "aha_tran_tgt_free: %s%d %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif	/* AHADEBUG */

	unit_ahap = hba_tran->tran_tgt_private;
	kmem_free(unit_ahap,
		sizeof (struct aha) + sizeof (struct aha_unit));

	sd->sd_inq = NULL;

	aha = SDEV2HBA(sd);
	AHA_BLKP(aha)->ab_child--;
}



/*
 *	Autoconfiguration routines
 */
static int
aha_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, "aha") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}


static int
aha_probe(register dev_info_t *devi)
{
	int	ioaddr;
	int	len;

#ifdef AHA_DEBUG
	if (aha_debug & DENT)
		PRF("ahaprobe: aha devi= 0x%x\n", devi);
#endif
	len = sizeof (int);
	if ((HBA_INTPROP(devi, "ioaddr", &ioaddr, &len) != DDI_SUCCESS) ||
	    (aha_findhba(ioaddr) != DDI_SUCCESS))
		return (DDI_PROBE_FAILURE);

	return (DDI_PROBE_SUCCESS);
}

static int
aha_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	register struct	aha 	*aha;
	register struct	aha_blk *aha_blkp;
	unchar savepos, saveint;

	switch (cmd) {
	case DDI_DETACH:
	{
		scsi_hba_tran_t	*tran;

		tran = (scsi_hba_tran_t *) ddi_get_driver_private(devi);
		if (!tran)
			return (DDI_SUCCESS);
		aha = TRAN2AHA(tran);
		if (!aha)
			return (DDI_SUCCESS);
		aha_blkp = AHA_BLKP(aha);
		if (aha_blkp->ab_child)
			return (DDI_FAILURE);

		/* disable interrupts if 1640 */
		if (aha_blkp->ab_bdid == 'B') {

			/* save contents of POS reg 96 */
			savepos = inb(MC_SLOT_SELECT);

			/* enable slot adjust */
			outb(MC_SLOT_SELECT,
				savepos | (aha_blkp->ab_slot|SLOT_ENABLE));

			saveint = inb(POS4);
			outb(POS4, saveint & ~INTR_DISABLE);

			/* disable slot adjust */
			outb(MC_SLOT_SELECT, savepos |
				(aha_blkp->ab_slot & ~SLOT_ENABLE));
		}

		ddi_iopb_free((caddr_t)aha_blkp->ab_inqp);
		ddi_remove_intr(devi, aha_xlate_vec(aha_blkp),
			aha_blkp->ab_iblock);

		ddi_dmae_release(devi, aha_blkp->ab_dmachan);

		scsi_destroy_cbthread(aha_blkp->ab_cbthdl);
		mutex_destroy(&aha_blkp->ab_mutex);

		mutex_enter(&aha_global_mutex);
		aha_global_init--;
		if (aha_global_init == 0)
			mutex_destroy(&aha_rmutex);
		mutex_exit(&aha_global_mutex);

		if (aha_blkp->ab_mboutp) {
			kmem_free((caddr_t)aha_blkp->ab_mboutp,
					aha_blkp->ab_mbx_allocmem);
		}
		if (scsi_hba_detach(devi) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "aha: scsi_hba_detach failed\n");
		}
		scsi_hba_tran_free(aha->a_tran);
		kmem_free((caddr_t)aha, (sizeof (*aha)+sizeof (*aha_blkp)));

		ddi_prop_remove_all(devi);

		return (DDI_SUCCESS);

	}
	default:
		return (DDI_FAILURE);
	}
}

static int
aha_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	register struct aha 	*aha;
	register struct aha_blk	*aha_blkp;
	u_int			intr_idx;
	unchar			savepos;
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
	aha = (struct aha *)kmem_zalloc((unsigned)(sizeof (*aha) +
		sizeof (*aha_blkp)), KM_NOSLEEP);
	if (!aha)
		return (DDI_FAILURE);
	aha_blkp = (struct aha_blk *)(aha + 1);
	AHA_BLKP(aha) = aha_blkp;
	aha_blkp->ab_dip = devi;

	if ((aha_propinit(aha_blkp) == DDI_FAILURE) ||
	    (aha_cfginit(aha_blkp)  == DDI_FAILURE) ||
	    (aha_getedt(aha_blkp)   == DDI_FAILURE)) {
		if (aha_blkp->ab_mboutp) {
			kmem_free((caddr_t)aha_blkp->ab_mboutp,
					aha_blkp->ab_mbx_allocmem);
		}
		kmem_free((caddr_t)aha, (sizeof (*aha)+sizeof (*aha_blkp)));
		return (DDI_FAILURE);
	}

	/*
	 * Allocate a transport structure
	 */
	hba_tran = scsi_hba_tran_alloc(devi, 0);
	if (hba_tran == NULL) {
		cmn_err(CE_WARN, "aha_attach: scsi_hba_tran_alloc failed\n");
		if (aha_blkp->ab_mboutp) {
			kmem_free((caddr_t)aha_blkp->ab_mboutp,
				aha_blkp->ab_mbx_allocmem);
		}
		kmem_free((caddr_t)aha, (sizeof (*aha)+sizeof (*aha_blkp)));
		return (DDI_FAILURE);
	}

	if (ddi_dmae_alloc(devi, aha_blkp->ab_dmachan,
		DDI_DMA_DONTWAIT, NULL) != DDI_SUCCESS) {
		kmem_free((caddr_t)aha_blkp->ab_mboutp,
				aha_blkp->ab_mbx_allocmem);
		kmem_free((caddr_t)aha, (sizeof (*aha)+sizeof (*aha_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	intr_idx = aha_xlate_vec(aha_blkp);
/*
 *	get iblock cookie to initialize mutexes used in the
 *	interrupt handler
 */
	if (ddi_get_iblock_cookie(devi, intr_idx,
	    &aha_blkp->ab_iblock) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "aha_attach: cannot get iblock cookie");
		kmem_free(aha, (sizeof (*aha)+sizeof (*aha_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	mutex_init(&aha_blkp->ab_mutex, "aha mutex", MUTEX_DRIVER,
		aha_blkp->ab_iblock);

	mutex_enter(&aha_global_mutex);	/* protect multithreaded attach	*/
	if (aha_global_init == 0) {
		mutex_init(&aha_rmutex, "AHA Resource Mutex", MUTEX_DRIVER,
			aha_blkp->ab_iblock);
	}
	aha_global_init++;
	mutex_exit(&aha_global_mutex);


	if (ddi_add_intr(devi, intr_idx,
		&aha_blkp->ab_iblock,
		(ddi_idevice_cookie_t *)0, aha_intr, (caddr_t)aha)) {
		cmn_err(CE_WARN, "aha_attach: cannot add intr\n");
		mutex_destroy(&aha_blkp->ab_mutex);
		mutex_enter(&aha_global_mutex);
		aha_global_init--;
		if (aha_global_init == 0)
			mutex_destroy(&aha_rmutex);
		mutex_exit(&aha_global_mutex);
		kmem_free((caddr_t)aha, (sizeof (*aha)+sizeof (*aha_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}
	aha_blkp->ab_cbthdl = scsi_create_cbthread(aha_blkp->ab_iblock,
		KM_NOSLEEP);

	aha->a_tran = hba_tran;

	hba_tran->tran_hba_private	= aha;
	hba_tran->tran_tgt_private	= NULL;

	hba_tran->tran_tgt_init		= aha_tran_tgt_init;
	hba_tran->tran_tgt_probe	= aha_tran_tgt_probe;
	hba_tran->tran_tgt_free		= aha_tran_tgt_free;

	hba_tran->tran_start 		= aha_transport;
	hba_tran->tran_abort		= aha_abort;
	hba_tran->tran_reset		= aha_reset;
	hba_tran->tran_getcap		= aha_getcap;
	hba_tran->tran_setcap		= aha_setcap;
	hba_tran->tran_init_pkt 	= aha_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= aha_tran_destroy_pkt;
	hba_tran->tran_dmafree		= aha_dmafree;
	hba_tran->tran_sync_pkt		= aha_sync_pkt;

	if (scsi_hba_attach(devi, aha_blkp->ab_dma_lim, hba_tran,
			SCSI_HBA_TRAN_CLONE, NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "aha_attach: scsi_hba_attach failed\n");
		scsi_destroy_cbthread(aha_blkp->ab_cbthdl);
		ddi_dmae_release(devi, aha_blkp->ab_dmachan);
		ddi_remove_intr(devi, intr_idx, aha_blkp->ab_iblock);
		mutex_destroy(&aha_blkp->ab_mutex);
		mutex_enter(&aha_global_mutex);
		aha_global_init--;
		if (aha_global_init == 0)
			mutex_destroy(&aha_rmutex);
		mutex_exit(&aha_global_mutex);
		kmem_free((caddr_t)aha_blkp->ab_mboutp,
				aha_blkp->ab_mbx_allocmem);
		kmem_free((caddr_t)aha, (sizeof (*aha)+sizeof (*aha_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	ddi_report_dev(devi);

	/* enable interrupts which the real mode driver turned off if 1640 */
	if (aha_blkp->ab_bdid == 'B') {
		/* save contents of POS reg 96 */
		savepos = inb(MC_SLOT_SELECT);

		/* enable slot adjust */
		outb(MC_SLOT_SELECT, savepos
			| (aha_blkp->ab_slot | SLOT_ENABLE));

		outb(POS4, (inb(POS4) & ~AHA_POS4IRQ_MASK) +
			aha_blkp->ab_intr - AHA_CODE_TOIRQ);

		/* disable slot adjust */
		outb(MC_SLOT_SELECT, savepos
			| (aha_blkp->ab_slot & ~SLOT_ENABLE));
	}

	return (DDI_SUCCESS);
}


static int
aha_propinit(register struct aha_blk *aha_blkp)
{
	register dev_info_t *devi;
	int	i;
	int	val;
	int	len;

	devi = aha_blkp->ab_dip;
	len = sizeof (int);
	if (HBA_INTPROP(devi, "ioaddr", &val, &len) != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);
	aha_blkp->ab_ioaddr   = (ushort) val;

	if (HBA_INTPROP(devi, "dmachan", &val, &len) == DDI_PROP_SUCCESS)
		aha_blkp->ab_dmachan  = (unchar) val;

	if (HBA_INTPROP(devi, "dmaspeed", &val, &len) == DDI_PROP_SUCCESS)
		aha_blkp->ab_dmaspeed = (unchar) val;

	if (HBA_INTPROP(devi, "buson", &val, &len) == DDI_PROP_SUCCESS)
		aha_blkp->ab_buson    = (unchar) val;

	if (HBA_INTPROP(devi, "busoff", &val, &len) == DDI_PROP_SUCCESS)
		aha_blkp->ab_busoff   = (unchar) val;

	aha_blkp->ab_num_mbox = 56;

	mutex_enter(&aha_global_mutex);
	if (!aha_pgsz) {
		aha_pgsz = ddi_ptob(devi, 1L);
		aha_pgmsk = aha_pgsz - 1;
		for (i = aha_pgsz, len = 0; i > 1; len++)
			i >>= 1;
		aha_pgshf = len;
	}
	mutex_exit(&aha_global_mutex);

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
aha_transport(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct aha_blk *aha_blkp;
	register struct	aha_ccb *ccbp;

	ccbp = (struct aha_ccb *)SCMD_PKTP(pktp)->cmd_private;

	aha_blkp = PKT2AHABLKP(pktp);
	mutex_enter(&aha_blkp->ab_mutex);
	if (aha_outmbx(aha_blkp, ccbp)) {
		mutex_exit(&aha_blkp->ab_mutex);
		return (TRAN_BUSY);
	}

	if (pktp->pkt_flags & FLAG_NOINTR)
		aha_pollret(aha_blkp, pktp);

	mutex_exit(&aha_blkp->ab_mutex);
	return (TRAN_ACCEPT);
}

/*ARGSUSED*/
static int
aha_abort(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	/* hardware does not support recall of command in process */
	return (FALSE);
}

/*ARGSUSED*/
static int
aha_reset(struct scsi_address *ap, int level)
{
	struct	aha_blk *aha_blkp;

	aha_blkp = ADDR2AHABLKP(ap);
	mutex_enter(&aha_blkp->ab_mutex);
	outb(aha_blkp->ab_ioaddr+AHACTL, CTL_SCRST);
	mutex_exit(&aha_blkp->ab_mutex);

	return (TRUE);
}

static int
aha_capchk(char *cap, int tgtonly, int *cidxp)
{
	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *)0)
		return (FALSE);

	*cidxp = scsi_hba_lookup_capstr(cap);
	return (TRUE);
}

/*ARGSUSED*/
static int
aha_getcap(struct scsi_address *ap, char *cap, int tgtonly)
{
	int	ckey;
	struct aha_blk	*aha_blkp;

	aha_blkp = ADDR2AHABLKP(ap);

	if (aha_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {

	case SCSI_CAP_GEOMETRY:
	    {
		int	total_sectors, h, s;

		total_sectors = (ADDR2AHAUNITP(ap))->au_total_sectors;

		if ((aha_blkp->ab_flag & AHA_CFG_EXTBIOS) &&
		    total_sectors > 0x200000)  {
		    h = 255;
		    s = 63;
		} else {
		    h = 64;
		    s = 32;
		}
#ifdef AHA_DEBUG
		if (aha_debug & DINIT) {
		    PRF("aha_getcap: heads = %d, sectors = %d, "
			"total_sectors = 0x%x\n", h, s, total_sectors);
		}
#endif
		return (HBA_SETGEOM(h, s));
	}


	case SCSI_CAP_ARQ:
		return (TRUE);
	default:
		break;
	}

	return (UNDEFINED);
}

static int
aha_setcap(struct scsi_address *ap, char *cap, int value, int tgtonly)
{
	int	ckey, status = FALSE;

	if (aha_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {
		case SCSI_CAP_SECTOR_SIZE:
			(ADDR2AHAUNITP(ap))->au_lim.dlim_granular =
						(u_int)value;
			status = TRUE;
			break;

		case SCSI_CAP_ARQ:
			if (tgtonly) {
				(ADDR2AHAUNITP(ap))->au_arq = (u_int)value;
				status = TRUE;
			}
			break;

		case SCSI_CAP_TOTAL_SECTORS:
			(ADDR2AHAUNITP(ap))->au_total_sectors = value;
			status = TRUE;
#ifdef AHA_DEBUG
			if (aha_debug & DINIT) {
			    PRF("aha_setcap: value=0x%x, au_total_sectors="
				"0x%x\n",
				value, (ADDR2AHAUNITP(ap))->au_total_sectors);
			}
#endif
			break;


		default:
			break;
	}

	return (status);
}


static struct scsi_pkt *
aha_tran_init_pkt(struct scsi_address *ap, struct scsi_pkt *pkt,
	struct buf *bp, int cmdlen, int statuslen, int tgtlen,
	int flags, int (*callback)(), caddr_t arg)
{
	struct scsi_pkt		*new_pkt = NULL;

	/*
	 * Allocate a pkt
	 */
	if (!pkt) {
		pkt = aha_pktalloc(ap, cmdlen, statuslen,
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
		if (aha_dmaget(pkt, (opaque_t)bp, callback, arg) == NULL) {
			if (new_pkt)
				aha_pktfree(ap, new_pkt);
			return (NULL);
		}
	}

	return (pkt);
}


static void
aha_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	aha_dmafree(ap, pkt);
	aha_pktfree(ap, pkt);
}


static struct scsi_pkt *
aha_pktalloc(
	struct scsi_address	*ap,
	int			cmdlen,
	int			statuslen,
	int			tgtlen,
	int			(*callback)(),
	caddr_t			arg)
{
	register struct scsi_cmd	*cmd;
	register struct aha_ccb		*ccbp;
	struct aha_blk			*aha_blkp;
	caddr_t				buf0, buf1;
	int				kf;
	caddr_t				tgt;

	aha_blkp = ADDR2AHABLKP(ap);
	kf = HBA_KMFLAG(callback);

	/*
	 * Allocate target-private data, if necessary
	 */
	if (tgtlen > PKT_PRIV_LEN) {
		tgt = kmem_zalloc(tgtlen, kf);
		if (!tgt) {
			ASSERT(callback != SLEEP_FUNC);
			if (callback != NULL_FUNC)
				ddi_set_callback(callback, arg, &aha_cb_id);
			return ((struct scsi_pkt *)NULL);
		}
	} else {
		tgt = NULL;
	}

	mutex_enter(&aha_rmutex);
	cmd = (struct scsi_cmd *)
		kmem_zalloc(sizeof (*cmd), kf);
	if (cmd) {
/* 		allocate ccb 						*/
		if (ddi_iopb_alloc(aha_blkp->ab_dip, (ddi_dma_lim_t *)0,
			(u_int)(sizeof (*ccbp)), &buf0)) {
			kmem_free((void *)cmd, sizeof (*cmd));
			cmd = NULL;
		}
/*		allocate sg list					*/
/*		cmd can be set to NULL by the previous alloc failing 	*/
/*		if so skip this alloc.					*/
		if (cmd) {
		    if (ddi_iopb_alloc(aha_blkp->ab_dip, (ddi_dma_lim_t *)0,
			(u_int) (sizeof (struct aha_dma_seg)*
				aha_blkp->ab_dma_lim->dlim_sgllen), &buf1)) {
			kmem_free((void *)cmd, sizeof (*cmd));
			ddi_iopb_free(buf0);
			cmd = NULL;
		    }
		}

	}
	mutex_exit(&aha_rmutex);

	if (!cmd) {
		if (tgt)
			kmem_free(tgt, tgtlen);
		ASSERT(callback != SLEEP_FUNC);
		if (callback != NULL_FUNC)
			ddi_set_callback(callback, arg, &aha_cb_id);
		return ((struct scsi_pkt *)NULL);
	}

	bzero(buf0, sizeof (*ccbp));
	bzero(buf1, sizeof (struct aha_dma_seg)*
			(aha_blkp->ab_dma_lim->dlim_sgllen));
/* 	initialize ccb 							*/
	ccbp		 = (struct aha_ccb *)buf0;
	ccbp->ccb_tf_tid = ap->a_target;
	ccbp->ccb_tf_lun = ap->a_lun;
	ccbp->ccb_cdblen = (u_char)cmdlen;
	ccbp->ccb_ownerp = cmd;
	ccbp->ccb_paddr  = AHA_KVTOP(ccbp);
	ccbp->ccb_sg_list = (struct aha_dma_seg *)buf1;
	if (!(ADDR2AHAUNITP(ap))->au_arq)
		ccbp->ccb_senselen = 1;

/* 	prepare the packet for normal command 				*/
	cmd->cmd_private	= (opaque_t)ccbp;
	cmd->cmd_pkt.pkt_cdbp   = (opaque_t)ccbp->ccb_cdb;
	cmd->cmd_cdblen 	= (u_char)cmdlen;

	cmd->cmd_pkt.pkt_scbp   = (u_char *)&ccbp->ccb_sense;
	cmd->cmd_scblen		= (u_char)statuslen;
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

#ifdef AHA_DEBUG
	if (aha_debug & DPKT) {
		PRF("aha_pktalloc:cmdpktp= 0x%x pkt_cdbp=0x%x pkt_scbp=0x%x\n",
			cmd, cmd->cmd_pkt.pkt_cdbp, cmd->cmd_pkt.pkt_scbp);
		PRF("ccbp = 0x%x\n", ccbp);
	}
#endif
	return ((struct scsi_pkt *) cmd);
}

/*
 * packet free
 */
/*ARGSUSED*/
void
aha_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	register struct scsi_cmd *cmd = (struct scsi_cmd *) pkt;
	struct aha_ccb *ccbp;


	if (cmd->cmd_privlen > PKT_PRIV_LEN) {
		kmem_free(pkt->pkt_private, cmd->cmd_privlen);
	}

	mutex_enter(&aha_rmutex);
/*	deallocate the ccb						*/
	ccbp = cmd->cmd_private;
	if (ccbp) {
		/* deallocate the sg list				*/
		if (ccbp->ccb_sg_list)
		    ddi_iopb_free((caddr_t)ccbp->ccb_sg_list);
		ddi_iopb_free((caddr_t)cmd->cmd_private);
	}
/*	free the common packet						*/
	kmem_free((void *)cmd, sizeof (*cmd));
	mutex_exit(&aha_rmutex);

	if (aha_cb_id)
		ddi_run_callback(&aha_cb_id);
}

/*
 * Dma resource deallocation
 */
/*ARGSUSED*/
void
aha_dmafree(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct	scsi_cmd *cmd = (struct scsi_cmd *) pktp;

/* 	Free the mapping.  						*/
	if (cmd->cmd_dmahandle) {
		ddi_dma_free(cmd->cmd_dmahandle);
		cmd->cmd_dmahandle = NULL;
	}
}


/*
 * Dma sync
 */
/*ARGSUSED*/
static void
aha_sync_pkt(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register int i;
	register struct	scsi_cmd *cmd = (struct scsi_cmd *) pktp;

	if (cmd->cmd_dmahandle) {
		i = ddi_dma_sync(cmd->cmd_dmahandle, 0, 0,
			(cmd->cmd_cflags & CFLAG_DMASEND) ?
			DDI_DMA_SYNC_FORDEV : DDI_DMA_SYNC_FORCPU);
		if (i != DDI_SUCCESS) {
			cmn_err(CE_WARN, "aha: sync pkt failed\n");
		}
	}
}


static struct scsi_pkt *
aha_dmaget(
	struct scsi_pkt	*pktp,
	opaque_t	dmatoken,
	int		(*callback)(),
	caddr_t		arg)
{
	struct buf *bp = (struct buf *) dmatoken;
	register struct scsi_cmd *cmd = (struct scsi_cmd *) pktp;
	register struct aha_ccb *ccbp;
	struct aha_dma_seg *dmap;
	ddi_dma_cookie_t dmack;
	ddi_dma_cookie_t *dmackp = &dmack;
	int	cnt;
	int	bxfer;
	off_t	offset;
	off_t	len;

	ccbp = (struct aha_ccb *)cmd->cmd_private;

	if (!bp->b_bcount) {
		cmd->cmd_pkt.pkt_resid = 0;
		ccbp->ccb_tf_in  = 1;
		ccbp->ccb_tf_out = 1;
		return (pktp);
	}

	if (bp->b_flags & B_READ) {
		ccbp->ccb_tf_in  = 1;
		ccbp->ccb_tf_out = 0;
		cmd->cmd_cflags &= ~CFLAG_DMASEND;
	} else {
		ccbp->ccb_tf_in  = 0;
		ccbp->ccb_tf_out = 1;
		cmd->cmd_cflags |= CFLAG_DMASEND;
	}

/*	setup dma memory and position to the next xfer segment		*/
	if (!scsi_impl_dmaget(pktp, (opaque_t)bp, callback, arg,
		&(PKT2AHAUNITP(pktp)->au_lim)))
		return (NULL);
	ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len, dmackp);

/*	check for one single block transfer				*/
	if (bp->b_bcount <= dmackp->dmac_size) {
		ccbp->ccb_op = COP_CMD_RESID;
		scsi_htos_3byte(ccbp->ccb_datalen, bp->b_bcount);
		scsi_htos_3byte(ccbp->ccb_datap, dmackp->dmac_address);
		pktp->pkt_resid = 0;
		cmd->cmd_totxfer = bp->b_bcount;
	} else {
/*		use scatter-gather transfer				*/
		ccbp->ccb_op = COP_SG_RESID;
		dmap = ccbp->ccb_sg_list;
		for (bxfer = 0, cnt = 1; ; cnt++, dmap++) {
			bxfer += dmackp->dmac_size;
			scsi_htos_3byte(dmap->data_len, dmackp->dmac_size);
			scsi_htos_3byte(dmap->data_ptr, dmackp->dmac_address);

/*			check for end of list condition			*/
			if (bp->b_bcount == (bxfer + cmd->cmd_totxfer))
				break;
			ASSERT(bp->b_bcount > (bxfer + cmd->cmd_totxfer));
/*			check end of physical scatter-gather list limit	*/
			if (cnt >= AHA_MAX_DMA_SEGS)
				break;
/*			check for transfer count			*/
			if (bxfer >= (PKT2AHAUNITP(pktp)->au_lim.dlim_reqsize))
				break;
			if (ddi_dma_nextseg(cmd->cmd_dmawin, cmd->cmd_dmaseg,
				&cmd->cmd_dmaseg) != DDI_SUCCESS)
				break;
			ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset,
				&len, dmackp);
		}
		scsi_htos_3byte(ccbp->ccb_datalen,
				(ulong)(cnt*sizeof (struct aha_dma_seg)));
		scsi_htos_3byte(ccbp->ccb_datap, AHA_KVTOP(ccbp->ccb_sg_list));

		cmd->cmd_totxfer += bxfer;
		pktp->pkt_resid = bp->b_bcount - cmd->cmd_totxfer;
	}
	return (pktp);
}

/*
 *	Adapter Dependent Layer
 */

static int
aha_findhba(register uint ioaddr)
{
	register int	i;

/*	probe for the hba						*/
/*	test for self echoing						*/
	outb(ioaddr+AHADATACMD, CMD_ECHO);
	for (i = 10; i; i--) {
		if (!((inb(ioaddr+AHASTAT) & STAT_MASK) &
		    (STAT_STST | STAT_CDF | STAT_DF | STAT_INVDCMD))) {
			break;
		}
		drv_usecwait(10);
	}
	if (i <= 0)
		return (DDI_FAILURE);

	outb(ioaddr+AHADATACMD, 0x5a);
	if (aha_wait(ioaddr+AHASTAT, STAT_MASK, (STAT_DF),
	    (STAT_STST | STAT_CDF | STAT_INVDCMD)))
		return (DDI_FAILURE);

	if (inb(ioaddr+AHADATACMD) != 0x5a || inb(ioaddr+AHASTAT) & STAT_DF)
		return (DDI_FAILURE);

	if (aha_wait(ioaddr+AHAINTFLGS, INT_HACC, INT_HACC, 0))
		return (DDI_FAILURE);

	if (aha_wait(ioaddr+AHASTAT, STAT_MASK, STAT_IDLE,
	    (STAT_STST | STAT_CDF | STAT_DF | STAT_INVDCMD)))
		return (DDI_FAILURE);

/*	Hard reset and scsi bus reset					*/
	outb(ioaddr+AHACTL, (CTL_HRST | CTL_IRST | CTL_SCRST));
	if (aha_wait(ioaddr+AHASTAT, STAT_MASK, (STAT_INIT | STAT_IDLE),
	    (STAT_STST | STAT_CDF | STAT_DF | STAT_INVDCMD)))
		return (DDI_FAILURE);

	drv_usecwait(SEC_INUSEC);

	if (inb(ioaddr+AHAINTFLGS))
		return (DDI_FAILURE);

	/* Note that the real mode driver will leave interrupts off here */

	return (DDI_SUCCESS);
}

static u_int
aha_xlate_vec(register struct  aha_blk *aha_blkp)
{
	static u_char aha_vec[] = {9, 10, 11, 12, 13, 14, 15};
	register int i;
	register u_char vec;

	vec = aha_blkp->ab_intr;
	for (i = 0; i < (sizeof (aha_vec)/sizeof (u_char)); i++) {
		if (aha_vec[i] == vec) {
			return ((u_int)i);
		}
	}
	return ((u_int)-1);
}

static int
aha_cfginit(register struct  aha_blk *aha_blkp)
{
	register int	i;
	register unchar	*ahabuf;
	caddr_t		buf;
	int		initmem;

/*	allocate memory for mailbox initialization			*/
	i = (sizeof (struct mbox_entry)*aha_blkp->ab_num_mbox) << 1;
	initmem = i + 32;
	if (ddi_iopb_alloc(aha_blkp->ab_dip, (ddi_dma_lim_t *)0,
			(u_int) initmem, &buf)) {
		cmn_err(CE_CONT,
			"aha_cfginit: unable to allocate memory.\n");
		return (DDI_FAILURE);
	}
	bzero(buf, initmem);

	aha_blkp->ab_mbx_allocmem = initmem;
	aha_blkp->ab_mboutp = (struct mbox_entry *)buf;
	ahabuf = (unchar *)(buf + i);
	aha_blkp->ab_bufp = ahabuf;

#ifdef AHA_DEBUG
	PRF("aha_cfginit: aha_blkp= 0x%x mboutp= 0x%x\n", aha_blkp,
		aha_blkp->ab_mboutp);
#endif


	(void) aha_docmd(aha_blkp, CMD_ADINQ, NULL, ahabuf);
	aha_blkp->ab_bdid = ahabuf[0];
#ifdef AHA_DEBUG
	if (aha_debug & DENT)
		PRF("aha_cfginit:board type = %c,0x%x\n", ahabuf[0], ahabuf[0]);
#endif
	if (aha_blkp->ab_bdid >= 0x30) {
		aha_blkp->ab_flag |= AHA_CFG_64HD;
		if (aha_blkp->ab_bdid >= 'A') {
			aha_blkp->ab_flag |= AHA_CFG_SCATGATH;

/*			check for AHA-1740 and set hba max xfer count	*/
			if (aha_blkp->ab_bdid == 'C') {
				cmn_err(CE_WARN, "aha: the 174x controller "
					"must run in enhanced mode\n");
				return (DDI_FAILURE);
			}
		}
		/*							*/
		/* see if the board is a 1540C[F|P]/1542C[F|P] 		*/
		/* controller the id string will be D|E|F for any of	*/
		/* these otherwise consider it to be a 1540B/1542B type	*/
		/* of board						*/
		if (aha_blkp->ab_bdid == 'D' || aha_blkp->ab_bdid == 'E' ||
		    aha_blkp->ab_bdid == 'F') {
			aha_blkp->ab_dma_lim = &aha_dma_lim255;
			aha_blkp->ab_flag |= AHA_CFG_C;
		} else
			aha_blkp->ab_dma_lim = &aha_dma_lim17;

	}

	aha_mboxinit(aha_blkp);

	(void) aha_docmd(aha_blkp, CMD_CONFIG, NULL, ahabuf);
	aha_blkp->ab_targetid = ahabuf[2];
	if (ahabuf[0] & CFG_DMA_MASK) {
		for (i = 0; (i < 8 && ahabuf[0] != 1); i++)
			ahabuf[0] >>= 1;
		if (aha_blkp->ab_bdid != 'B')
			aha_blkp->ab_dmachan = (unchar)i;
			/*
			 * note user must set arbritration level (dmachan)
			 * as in the configuration file (6)
			 */
	}

/*	save and check interrupt vector number				*/
	if (aha_blkp->ab_bdid != 'B') {

		if (ahabuf[1] & CFG_INT_MASK) {
			for (i = 0; (i < 8 && ahabuf[1] != 1); i++)
				ahabuf[1] >>= 1;
			aha_blkp->ab_intr = (unchar)(i + 9);

			if (aha_xlate_vec(aha_blkp) == (u_int)-1)
				return (DDI_FAILURE);
		} else
			return (DDI_FAILURE);

	} else {

		/* must read pos registers for IRQ on 1640 */
		if (aha_find_1640irq(aha_blkp) == -1)
			return (DDI_FAILURE);
	}

/*
 * 	set up Bus Master DMA
 */
	if (ddi_dmae_1stparty(aha_blkp->ab_dip, aha_blkp->ab_dmachan)
		!= DDI_SUCCESS)
		return (DDI_FAILURE);

	if (aha_blkp->ab_bdid != 'B') {
		ahabuf[0] = aha_blkp->ab_dmaspeed;
		(void) aha_docmd(aha_blkp, CMD_XFERSPEED, ahabuf, NULL);

		ahabuf[0] = aha_blkp->ab_buson;
		(void) aha_docmd(aha_blkp, CMD_BONTIME, ahabuf, NULL);
		ahabuf[0] = aha_blkp->ab_busoff;
		(void) aha_docmd(aha_blkp, CMD_BOFFTIME, ahabuf, NULL);
	}
	drv_usecwait(2*SEC_INUSEC);

	return (DDI_SUCCESS);
}

/*
 *	save and check interrupt vector number on 1640
 * 	returns 0 on success, -1 on failure
 */
static int
aha_find_1640irq(struct aha_blk *aha_blkp)
{
	int slot;
	unchar savepos;
	int ioaddr_idx;

	savepos = inb(MC_SLOT_SELECT);

	for (slot = 0; slot < AHA_MAX_MC_SLOTS; slot++) {

		/* enable slot */
		outb(MC_SLOT_SELECT, savepos | (slot | SLOT_ENABLE));

		/* check 1640 signature */
		if (inb(POS0) != 0x1f || inb(POS1) != 0xf) {
			outb(MC_SLOT_SELECT,
				savepos | (slot & ~SLOT_ENABLE));
			continue;
		}

		/* make sure this is the right 1640 card */
		if ((ioaddr_idx =
			aha_find_ioaddridx((ushort)(inb(POS3)
			& AHA_POSADDR3_MASK)))
			== AHA_MAX_NUMIOADDR) {
			outb(MC_SLOT_SELECT,
				savepos | (slot & ~SLOT_ENABLE));
			return (-1);
		}

		if (aha_codes[ioaddr_idx].ac_ioaddr != aha_blkp->ab_ioaddr) {
			outb(MC_SLOT_SELECT,
				savepos | (slot & ~SLOT_ENABLE));
			continue;
		} else {
			aha_blkp->ab_intr = (ushort)
				(((inb(POS4) & AHA_POS4IRQ_MASK)) +
					AHA_CODE_TOIRQ);
		}

		/* save slot for detach */
		aha_blkp->ab_slot = (ushort)slot;

		/* If we don't know, set to IRQ 11 */
		if (aha_blkp->ab_intr == AHA_CODE_TOIRQ) {
			aha_blkp->ab_intr = AHA_1640DEFAULT_IRQ +
				AHA_CODE_TOIRQ;
		}

		/* disable it */
		outb(MC_SLOT_SELECT, savepos | (slot & ~SLOT_ENABLE));
		break;

	}

	if (slot == AHA_MAX_MC_SLOTS)
		return (-1);

	return (0);

}


/*
 *	find index into array of io port addresses and POS reg 3 codes
 * 	returns 0 on success, -1 on failure
 */
static int
aha_find_ioaddridx(ushort iocode)
{
	int index;

	for (index = 0; index < AHA_MAX_NUMIOADDR; index++) {
		if (iocode == aha_codes[index].ac_code)
			break;
	}

	return (index);
}

static void
aha_mboxinit(register struct aha_blk *aha_blkp)
{
	register int	num_mbox;
	unchar	*ahabuf;


	/* 								*/
	/* We only can unlock mailboxes for the 154xC[F|P] controllers.	*/
	/* The command will fail on 154XA or 154XB controlers.  	*/
	/* This isn't clear from the Adaptec documentation  yet I have 	*/
	/* confirmed this fact with Adaptec technical folks. Attempting	*/
	/* to get the EXTENDED BIOS information (which is what		*/
	/* aha_mboxunlock does) will cause the driver to fail on the	*/
	/* 154XA or 154XB cards. Thus the check is necessary.		*/
	/*								*/
	if (aha_blkp->ab_flag & AHA_CFG_C) {
	    if (aha_mboxunlock(aha_blkp)) {
		cmn_err(CE_PANIC, "aha_mboxinit: Failed to unlock mailboxes\n");
		return;
	    }
	}


	aha_blkp->ab_mbout_miss = aha_blkp->ab_mbin_miss = 0;
	num_mbox = aha_blkp->ab_num_mbox;
	aha_blkp->ab_mbinp  = &aha_blkp->ab_mboutp[num_mbox];
	ahabuf   = aha_blkp->ab_bufp;
	ahabuf[0] = (unchar)num_mbox;
	scsi_htos_3byte(&ahabuf[1], AHA_KVTOP(aha_blkp->ab_mboutp));
	(void) aha_docmd(aha_blkp, CMD_MBXINIT, ahabuf, (unchar *)NULL);

#ifdef AHA_DEBUG
	ahabuf[0] = 17;
	(void) aha_docmd(aha_blkp, 0xd, ahabuf, ahabuf);
	PRF("aha_mboxinit: VERIFY num_mbox= %d mboutp= 0x%x\n",
		ahabuf[4], scsi_stoh_3byte(&ahabuf[5]));
#endif
}

static void
aha_scsicmd(struct aha_blk *aha_blkp)
{
	register int ioaddr;

	ioaddr = aha_blkp->ab_ioaddr;
	if (!(inb(ioaddr+AHASTAT) & STAT_CDF)) {
		outb(ioaddr+AHADATACMD, CMD_DOSCSI);
		return;
	}
#ifdef CDFWAIT
	for (;;) {
		if (!(inb(ioaddr+AHASTAT) & STAT_CDF)) {
			outb(ioaddr+AHADATACMD, CMD_DOSCSI);
			return;
		}
		if (aha_blkp->ab_iblock) {
			mutex_exit(&aha_blkp->ab_mutex);
			drv_usecwait(10);
			mutex_enter(&aha_blkp->ab_mutex);
		} else
			drv_usecwait(10);
	}
#endif
}

static int
aha_mboxunlock(register struct aha_blk *aha_blkp)
{
	unchar  *ahabuf;
	int 	ret;

	ahabuf = aha_blkp->ab_bufp;

	if ((ret = aha_docmd(aha_blkp, CMD_EXTBIOS, (unchar *) NULL, ahabuf))
		!= 0) {
		cmn_err(CE_CONT,
		    "aha_mboxunlock: aha_docmd for CMD_EXTBIOS failed!\n");
		return (ret);
	}

#ifdef AHA_DEBUG
	if (aha_debug & DINIT)
	    PRF("aha_mboxunlock: return from CMD_EXTBIOS byte0=%d,byte1=%d\n",
		ahabuf[0], ahabuf[1]);
#endif

	/* indicate to the driver that Extended BIOS translation is enabled */
	if (ahabuf[0] & AHA_EXTBIOS_ENABLED_FLAG) {
	    aha_blkp->ab_flag |= AHA_CFG_EXTBIOS;
#ifdef AHA_DEBUG
	    if (aha_debug & DINIT)
		PRF("aha_mboxunlock: EXTENDED BIOS set for drivers > 1gig\n");
#endif
	}
	/*
	 * ahabuf[1] contains the key value for unlocking the mailboxes.
	 * The key value is 0 if the mailboxes are not locked;
	 * otherwise the maiboxes need to be unlocked.
	 */
	if (ahabuf[1] == 0)
	    return (0);

	/*							*/
	/* Unlock the mailboxes					*/
	/* The first data byte must be zero and the second 	*/
	/* (aready in ahabuf[1] must be the lock key).		*/
	/*							*/
	ahabuf[0] = 0;
#ifdef AHA_DEBUG
	if (aha_debug & DINIT)
	    PRF("aha_mboxunlock: calling to unlock mailbox\n");
#endif

	return (aha_docmd(aha_blkp, CMD_MBXUNLK, ahabuf, (unchar *) NULL));
}



static int
aha_docmd(struct aha_blk *aha_blkp, int opcode, unchar *sbp, unchar *rbp)
{
	register struct aha_cmd *cp = &aha_cmds[opcode];
	register int i;
	register int ioaddr;

	if (cp->ac_flags & ACF_INVDCMD) {
#ifdef AHA_DEBUG
		PRF("aha_docmd: ACF_INVCMD\n");
#endif
		return (1);	/* invalid command */
	}
	ioaddr = aha_blkp->ab_ioaddr;
	if (cp->ac_flags & ACF_WAITIDLE) { /* wait for the adapter to go idle */
		if (aha_wait(ioaddr+AHASTAT, STAT_IDLE, STAT_IDLE, 0))
			cmn_err(CE_PANIC, "aha_docmd: adapter won't go IDLE");
	}

	if (aha_wait(ioaddr+AHASTAT, STAT_CDF, 0, STAT_CDF))
		cmn_err(CE_PANIC, "aha_docmd: CDF won't go off for command");
	outb(ioaddr+AHADATACMD, opcode);

/* 	If any output data, write it out */
	for (i = cp->ac_args; i > 0; i--, sbp++) {
		/* wait for STAT_CDF to turn off */
		if (aha_wait(ioaddr+AHASTAT, STAT_CDF, 0, STAT_CDF))
			cmn_err(CE_PANIC,
				"aha_docmd: CDF won't go off for data");
		outb(ioaddr+AHADATACMD, *sbp);
		if (inb(ioaddr+AHASTAT) & STAT_INVDCMD) {
#ifdef AHA_DEBUG
			PRF("aha_docmd: STAT_INVDCMD\n");
#endif
			return (1);
		}
	}

/* 	if any return data, get it */
	for (i = cp->ac_vals; i > 0; i--, rbp++) {
		/* wait for STAT_DF to turn on */
		if (aha_wait(ioaddr+AHASTAT, STAT_DF, STAT_DF, 0))
			cmn_err(CE_PANIC,
				"aha_docmd: DF won't go on for value");
		*rbp = inb(ioaddr+AHADATACMD);
	}

/* 	Wait for completion if necessary */
	if ((cp->ac_flags & ACF_WAITCPLTE) == 0)
		return (0);
	if (aha_wait(ioaddr+AHAINTFLGS, INT_HACC, INT_HACC, 0))
		cmn_err(CE_PANIC, "aha_docmd: adapter won't indicate COMPLETE");

/* 	Reset the interrupts */
	outb(ioaddr+AHACTL, CTL_IRST);

/* 	Check for error */
	if (inb(ioaddr+AHASTAT) & STAT_INVDCMD) {
#ifdef AHA_DEBUG
		PRF("aha_docmd: STAT_INVDCMD\n");
#endif
		return (1);
	}

	return (0);
}

/*
 * aha_wait --  wait for a register of a controller to achieve a
 *              specific state.  Arguments are a mask of bits we care about,
 *              and two sub-masks.  To return normally, all the bits in the
 *              first sub-mask must be ON, all the bits in the second sub-
 *              mask must be OFF.  If 5 seconds pass without the controller
 *              achieving the desired bit configuration, we return 1, else
 *              0.
 */
static int
aha_wait(register ushort port, ushort mask, ushort onbits, ushort offbits)
{
	register int i;
	register ushort maskval;

	for (i = 500000; i; i--) {
		maskval = inb(port) & mask;
		if (((maskval & onbits) == onbits) &&
				((maskval & offbits) == 0))
			return (0);
		drv_usecwait(10);
	}
	return (1);
}

static int
aha_setup_inq(register struct aha_blk *aha_blkp)
{
	register int	mem;
	caddr_t 	buf;

/* 	set up memory for the inquiry array				*/
	mem = sizeof (struct scsi_inquiry) * (HBA_MAX_ATT_DEVICES + 8);

	if (ddi_iopb_alloc(aha_blkp->ab_dip, (ddi_dma_lim_t *)0, (u_int) mem,
			&buf)) {
		return (DDI_FAILURE);
	}
	bzero(buf, mem);
	aha_blkp->ab_inqp = (struct scsi_inquiry *)buf;

/* 	allocate ccb for hba specific command 				*/
	mem = sizeof (struct aha_ccb);

	if (ddi_iopb_alloc(aha_blkp->ab_dip, (ddi_dma_lim_t *)0,
		mem, &buf)) {
/* 		free array of inquiry data 				*/
		ddi_iopb_free((caddr_t)aha_blkp->ab_inqp);
		return (DDI_FAILURE);
	}

	bzero((caddr_t)buf, mem);
	aha_blkp->ab_ccbp = (struct aha_ccb *)buf;
	aha_blkp->ab_ccbp->ccb_paddr   = AHA_KVTOP(aha_blkp->ab_ccbp);

	return (DDI_SUCCESS);
}

static int
aha_getedt(register struct aha_blk *aha_blkp)
{
	register int	lun, i;
	int	targ;
	struct aha_ccb *ccbp;
	struct scsi_inquiry *inqdp;

	if (aha_setup_inq(aha_blkp) == DDI_FAILURE)
		return (DDI_FAILURE);
	ccbp = aha_blkp->ab_ccbp;

	for (targ = 0; targ < 8; targ++) {
		if (targ == aha_blkp->ab_targetid) {
			for (lun = 0; lun < 8; lun++) {
				inqdp = (struct scsi_inquiry *)
					(&aha_blkp->ab_inqp[targ<<3|lun]);
				inqdp->inq_dtype = DTYPE_NOTPRESENT;
			}
			continue;
		}
		for (lun = 0; lun < 8; lun++) {
			inqdp = (struct scsi_inquiry *)(&aha_blkp->ab_inqp[
				targ<<3|lun]);
			for (i = 0; i < 2; i++) {
				aha_inqcmd(ccbp, (caddr_t)inqdp, targ, lun);
				if (aha_outmbx(aha_blkp, ccbp)) {
#ifdef AHA_DEBUG
					if (aha_debug & DINIT)
					    PRF("aha_getedt: outmbx fail\n");
#endif
					inqdp->inq_dtype = DTYPE_NOTPRESENT;
					break;
				}
				if (aha_poll(aha_blkp)) {
#ifdef AHA_DEBUG
					if (aha_debug & DINIT)
						PRF("aha_getedt: poll fail\n");
#endif
					inqdp->inq_dtype = DTYPE_NOTPRESENT;
					break;
				}
				aha_inmbx(aha_blkp, 0);
				if (aha_blkp->ab_actm_stat == MBX_STAT_DONE) {
					break;
				} else {
					inqdp->inq_dtype = DTYPE_NOTPRESENT;
					if (ccbp->ccb_hastat == HS_SELTO)
						break;
					else
						continue;
				}
			}

			if (inqdp->inq_dtype != DTYPE_NOTPRESENT &&
				inqdp->inq_dtype != DTYPE_UNKNOWN) {
				aha_blkp->ab_numdev++;
#ifdef AHA_DEBUG
				PRF("aha_getedt: <targ,lun>= (%d,%d)"
					" devtyp= 0x%x\n",
					ccbp->ccb_tf_tid, ccbp->ccb_tf_lun,
					inqdp->inq_dtype);
#endif
					}

		}
	}

#ifdef AHA_DEBUG
	PRF("aha_getedt:  numdev= %d\n", aha_blkp->ab_numdev);
#endif
	ddi_iopb_free((caddr_t)aha_blkp->ab_ccbp);
	if (aha_blkp->ab_numdev &&
	    (aha_blkp->ab_numdev < aha_blkp->ab_num_mbox-2)) {
		aha_blkp->ab_num_mbox = aha_blkp->ab_numdev+2;
		aha_mboxinit(aha_blkp);
	}

	return (DDI_SUCCESS);
}

static int
aha_poll(register struct aha_blk *aha_blkp)
{
	register ushort	ioaddr;

	ioaddr = aha_blkp->ab_ioaddr;

/* 	wait for INT_MBIF 						*/
	if (aha_wait(ioaddr+AHAINTFLGS, INT_MBIF, INT_MBIF, 0)) {
#ifdef AHA_DEBUG
		PRF("aha_poll: command failed with no ack.\n");
		PRF("stat= 0x%x intr= 0x%x\n",
			inb(ioaddr+AHASTAT), inb(ioaddr+AHAINTFLGS));
#endif
		return (1);
	}

	outb(ioaddr+AHACTL, CTL_IRST);
	return (0);
}

static void
aha_inmbx(register struct aha_blk *aha_blkp, int stop_1stmiss)
{
	register struct mbox_entry *mbep;
	register int i;

	aha_blkp->ab_actm_addrp = NULL;
	aha_blkp->ab_actm_stat  = 0;
	i = aha_blkp->ab_num_mbox;
	for (; i > 0; i--) {
		if (aha_blkp->ab_mbin_idx >= aha_blkp->ab_num_mbox)
			aha_blkp->ab_mbin_idx = 0;
		mbep = (struct mbox_entry *)&aha_blkp->ab_mbinp[
				aha_blkp->ab_mbin_idx];
		aha_blkp->ab_mbin_idx++;
		if (mbep->mbx_cmdstat != MBX_FREE) {
			aha_blkp->ab_actm_stat = (uint)mbep->mbx_cmdstat;
			aha_blkp->ab_actm_addrp = aha_retccb(aha_blkp,
					scsi_stoh_3byte(mbep->mbx_ccb_addr));
			mbep->mbx_cmdstat = MBX_FREE;
			return;
		}
		if (stop_1stmiss) {
			aha_blkp->ab_mbin_idx--;
			return;
		}
		aha_blkp->ab_mbin_miss++;
	}
#ifdef AHA_DEBUG
	if (aha_debug & DINM)
		PRF("aha_inmbx: NO in-coming mailbox\n");
#endif
}

static int
aha_outmbx(register struct aha_blk *aha_blkp, struct aha_ccb *ccbp)
{
	register struct mbox_entry *mbep;
	register int i;

/*	search round robin for 2 times					*/
	i = aha_blkp->ab_num_mbox << 1;
	for (; i > 0; i--) {
		if (aha_blkp->ab_mbout_idx >= aha_blkp->ab_num_mbox)
			aha_blkp->ab_mbout_idx = 0;
		mbep = (struct mbox_entry *) &aha_blkp->ab_mboutp
				[aha_blkp->ab_mbout_idx];
		aha_blkp->ab_mbout_idx++;
		if (mbep->mbx_cmdstat == MBX_FREE) {
			aha_saveccb(aha_blkp, ccbp);
			scsi_htos_3byte(mbep->mbx_ccb_addr, ccbp->ccb_paddr);
			mbep->mbx_cmdstat = MBX_CMD_START;
			aha_scsicmd(aha_blkp);
			return (0);
		}
		aha_blkp->ab_mbout_miss++;
	}

	cmn_err(CE_CONT, "aha_outmbx: no empty outgoing mailboxes found\n");
	return (1);
}

static struct aha_ccb *
aha_retccb(struct aha_blk *aha_blkp, ulong ai)
{
	register struct	aha_ccb *cp;

	for (cp = aha_blkp->ab_ccboutp; cp; ) {
		if (cp->ccb_paddr != ai) {
			cp = cp->ccb_forw;
			ASSERT(cp != aha_blkp->ab_ccboutp);
			continue;
		}
/*		check for single entry on the list			*/
		if (cp == cp->ccb_forw) {
			aha_blkp->ab_ccboutp = NULL;
			return (cp);
		}

/*		check for first entry on the list			*/
		if (aha_blkp->ab_ccboutp == cp)
			aha_blkp->ab_ccboutp = cp->ccb_forw;
		cp->ccb_back->ccb_forw = cp->ccb_forw;
		cp->ccb_forw->ccb_back = cp->ccb_back;
		return (cp);

	}
#ifdef AHA_DEBUG
	if (aha_debug & DINM)
		PRF("aha_retccb: NO match\n");
#endif
	return (cp);
}

static void
aha_saveccb(register struct aha_blk *aha_blkp, register struct aha_ccb *ccbp)
{
	register struct	aha_ccb *cp;

	cp = aha_blkp->ab_ccboutp;
	if (!cp) {
		aha_blkp->ab_ccboutp = ccbp;
		ccbp->ccb_forw = ccbp;
		ccbp->ccb_back = ccbp;
		return;
	}

	cp->ccb_back->ccb_forw = ccbp;
	ccbp->ccb_back = cp->ccb_back;
	ccbp->ccb_forw = cp;
	cp->ccb_back = ccbp;
}

static void
aha_inqcmd(register struct aha_ccb *ccbp, caddr_t bp, int targ, int lun)
{
	ccbp->ccb_tf_lun = (u_char)lun;
	ccbp->ccb_tf_tid = (u_char)targ;
	ccbp->ccb_tf_in = 1;
	ccbp->ccb_tf_out = 0;
	ccbp->ccb_op = COP_COMMAND;
	scsi_htos_3byte(ccbp->ccb_datalen, sizeof (struct scsi_inquiry));
	scsi_htos_3byte(ccbp->ccb_datap, AHA_KVTOP(bp));
	ccbp->ccb_cdblen = 6;
	bzero((caddr_t)ccbp->ccb_cdb, ccbp->ccb_cdblen);
	ccbp->ccb_cdb[0] = SCMD_INQUIRY;
	ccbp->ccb_cdb[1] = lun << 5;
	ccbp->ccb_cdb[4] = sizeof (struct scsi_inquiry);
}

static u_int
aha_intr(caddr_t arg)
{
	register struct	aha_blk *aha_blkp;
	register struct	scsi_pkt *pktp;
	unchar	 intflgs;

	aha_blkp = AHA_BLKP(arg);
	mutex_enter(&aha_blkp->ab_mutex);
	intflgs = inb(aha_blkp->ab_ioaddr+AHAINTFLGS);
	if (!(intflgs & INT_ANY)) {
#ifdef AHA_DEBUG
		if (aha_debug & DINM)
			PRF("aha_intr: intr flag=0x%x\n", intflgs);
#endif
		mutex_exit(&aha_blkp->ab_mutex);
		return (DDI_INTR_UNCLAIMED);
	}

	outb(aha_blkp->ab_ioaddr+AHACTL, CTL_IRST);

	if (intflgs & INT_MBIF) {
		aha_inmbx(aha_blkp, 0);
		for (;;) {
			pktp = aha_chkerr(aha_blkp);
			if (!pktp)
				break;
			mutex_exit(&aha_blkp->ab_mutex);
			scsi_run_cbthread(aha_blkp->ab_cbthdl,
				(struct scsi_cmd *)pktp);
			mutex_enter(&aha_blkp->ab_mutex);
			aha_inmbx(aha_blkp, 1);
		}
	}

	mutex_exit(&aha_blkp->ab_mutex);
	return (DDI_INTR_CLAIMED);
}

static void
aha_pollret(struct aha_blk *aha_blkp, struct scsi_pkt *poll_pktp)
{
	register struct	scsi_pkt *pktp;
	register struct	scsi_cmd *cmd;
	struct	scsi_cmd *cmd_hdp = (struct  scsi_cmd *)NULL;

	for (;;) {
/* 		Wait for Command Complete "Interrupt"			*/
		if (aha_poll(aha_blkp)) {
			poll_pktp->pkt_reason = CMD_INCOMPLETE;
			poll_pktp->pkt_state  = 0;
			break;
		}

		aha_inmbx(aha_blkp, 0);
		pktp = aha_chkerr(aha_blkp);
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
			for (cmd = cmd_hdp;
				cmd->cmd_cblinkp;
					cmd = cmd->cmd_cblinkp)
				;
			cmd->cmd_cblinkp = (struct scsi_cmd *)pktp;
		}
	}

/*	check for other completed packets that have been queued		*/
	if (cmd_hdp) {
		mutex_exit(&aha_blkp->ab_mutex);
		for (; (cmd = cmd_hdp) != NULL; ) {
			cmd_hdp = cmd->cmd_cblinkp;
			scsi_run_cbthread(aha_blkp->ab_cbthdl, cmd);
		}
		mutex_enter(&aha_blkp->ab_mutex);
	}
}

/*
 *	check any possible error from the returned packet
 */
static struct scsi_pkt *
aha_chkerr(register struct aha_blk *aha_blkp)
{
	register struct	scsi_pkt *pktp;
	register struct	aha_ccb *ccbp;

	if (aha_blkp->ab_actm_stat == MBX_STAT_CCBNF)
		return (NULL);

	ccbp = aha_blkp->ab_actm_addrp;
	if (!ccbp)
		return (NULL);

/* 	check for OPCODE 0x81 - target reset 				*/
	pktp = (struct scsi_pkt *)ccbp->ccb_ownerp;

	/* 								*/
	/* use resid from ccb (ccb_datalen) because the transfers keep	*/
	/* track of residual counts.  The resid will be the difference 	*/
	/* between the original datalength from the host and the 	*/
	/* actuall xfer.  If there is a dataoverrun the resid will be	*/
	/* zero.							*/
	pktp->pkt_resid = scsi_stoh_3byte(ccbp->ccb_datalen);
/*	clear request sense cache flag					*/
	switch (aha_blkp->ab_actm_stat) {
	case MBX_STAT_DONE:
		pktp->pkt_reason = CMD_CMPLT;
		*pktp->pkt_scbp = STATUS_GOOD;
		pktp->pkt_state =
			(STATE_XFERRED_DATA|STATE_GOT_BUS|
			STATE_GOT_TARGET|STATE_SENT_CMD|STATE_GOT_STATUS);
		break;
	case MBX_STAT_ERROR:
		*pktp->pkt_scbp = ccbp->ccb_tarstat;
		if (ccbp->ccb_hastat == HS_OK) {
			pktp->pkt_reason = CMD_CMPLT;
			if (ccbp->ccb_tarstat == STATUS_CHECK)
				aha_seterr(pktp, ccbp);
/*						1540 behavior		*/
		} else if (ccbp->ccb_hastat == HS_SELTO) {
			pktp->pkt_reason = CMD_TIMEOUT;
			pktp->pkt_statistics |= STAT_TIMEOUT;
		} else if (ccbp->ccb_hastat == HS_DATARUN) {

			if (ccbp->ccb_tarstat == STATUS_CHECK)
/*						1740 behavior		*/
				aha_seterr(pktp, ccbp);
			else {
			    pktp->pkt_reason = CMD_CMPLT;
			    *pktp->pkt_scbp = STATUS_GOOD;
			    pktp->pkt_state =
				(STATE_GOT_BUS | STATE_GOT_TARGET |
				STATE_SENT_CMD | STATE_XFERRED_DATA |
				STATE_GOT_STATUS);
#ifdef AHA_DEBUG
			if (aha_debug & DRESID)
			    PRF(" aha_chkerr: HS_DATARUN, resid=0x%x, "
				"ccb_tarstat=0x%x\n", pktp->pkt_resid,
				ccbp->ccb_tarstat);
#endif
			}
		} else if (ccbp->ccb_hastat == HS_BADFREE)
			pktp->pkt_reason = CMD_UNX_BUS_FREE;
		else
			pktp->pkt_reason = CMD_INCOMPLETE;
		break;
	case MBX_STAT_ABORT:
		pktp->pkt_reason = CMD_ABORTED;
		pktp->pkt_statistics |= STAT_ABORTED;
		break;
	default:
		break;
	}
	return (pktp);
}

static void
aha_seterr(register struct scsi_pkt *pktp, register struct aha_ccb *ccbp)
{
	register struct	 scsi_arq_status *arqp;

	pktp->pkt_reason = CMD_CMPLT;
	pktp->pkt_state  = (STATE_GOT_BUS|STATE_GOT_TARGET|
		STATE_SENT_CMD|STATE_GOT_STATUS);
	if (!(PKT2AHAUNITP(pktp))->au_arq)
		return;

	pktp->pkt_state  |= STATE_ARQ_DONE;
	bcopy((caddr_t)&(ccbp->ccb_cdb[ccbp->ccb_cdblen]),
		(caddr_t)&ccbp->ccb_sense.sts_sensedata, AHA_SENSE_LEN);
	arqp = (struct scsi_arq_status *)pktp->pkt_scbp;
	arqp->sts_rqpkt_reason = CMD_CMPLT;

	arqp->sts_rqpkt_resid  = sizeof (struct scsi_extended_sense) -
		AHA_SENSE_LEN;
	arqp->sts_rqpkt_state |= STATE_XFERRED_DATA;
}

#ifdef SCSI_SYS_DEBUG
void
aha_dump_ahablk(struct aha_blk *p)
{
	PRF("numdev %d flag 0x%x targetid 0x%x bdid 0x%x intr 0x%x\n",
		p->ab_numdev & 0xff, p->ab_flag & 0xff,
		p->ab_targetid & 0xff, p->ab_bdid,
		p->ab_intr & 0xff);
	PRF("dmachan %d dmaspeed 0x%x buson 0x%x busoff 0x%x dmasiz 0x%x\n",
		p->ab_dmachan & 0xff, p->ab_dmaspeed & 0xff,
		p->ab_buson & 0xff, p->ab_busoff & 0xff, p->ab_dma_reqsize);
	PRF("dip 0x%x ioaddr 0x%x inqp 0x%x ccb_cnt %d ccbp 0x%x\n",
		p->ab_dip, p->ab_ioaddr, p->ab_inqp, p->ab_ccb_cnt,
		p->ab_ccbp);
}

char *aha_err_strings[] = {
	"No Error", 				/* 0x00 */
	"Invalid Error Code",			/* 0x01 */
	"Invalid Error Code",			/* 0x02 */
	"Invalid Error Code",			/* 0x03 */
	"Invalid Error Code",			/* 0x04 */
	"Invalid Error Code",			/* 0x05 */
	"Invalid Error Code",			/* 0x06 */
	"Invalid Error Code",			/* 0x07 */
	"Invalid Error Code",			/* 0x08 */
	"Invalid Error Code",			/* 0x09 */
	"Invalid Error Code",			/* 0x0a */
	"Invalid Error Code",			/* 0x0b */
	"Invalid Error Code",			/* 0x0c */
	"Invalid Error Code",			/* 0x0d */
	"Invalid Error Code",			/* 0x0e */
	"Invalid Error Code",			/* 0x0f */
	"Invalid Error Code",			/* 0x10 */
	"Selection Time Out",			/* 0x11 */
	"Data Over-Under Run",			/* 0x12 */
	"Unexpected Bus Free",			/* 0x13 */
	"Target Bus Phase Sequence Error", 	/* 0x14 */
	"Invalid Error Code",			/* 0x15 */
	"Invalid CCB Op",			/* 0x16 */
	"Link CCB with Bad Lun",		/* 0x17 */
	"Invalid Target Direction from Host",	/* 0x18 */
	"Duplicate CCB",			/* 0x19 */
	"Invalid CCB or Segment List Parm"	/* 0x1a */
};
void
aha_dump_ccb(struct aha_ccb *p)
{
	int index;

	PRF("op code 0x%x targ %d cdblen 0x%x scsi_cmd 0x%x\n",
		p->ccb_op & 0xff, p->ccb_tf_tid & 0xff,
		p->ccb_cdblen & 0xff, p->ccb_ownerp);

	if (!(PKT2AHAUNITP((struct scsi_pkt *)p->ccb_ownerp))->au_arq)
		PRF("Auto RS not on in scsi pkt->au_arq\n");
	else
		PRF("ARS on in scsi pkt->au_arq\n");

	PRF("lun %d", p->ccb_tf_lun & 0x0f);

	if (p->ccb_tf_out)
		PRF(" xfer out ");
	if (p->ccb_tf_in)
		PRF(" xfer in ");

	if (p->ccb_op == COP_SCATGATH)
		PRF(" scatter gather\n");
	if (p->ccb_op == COP_COMMAND)
		PRF(" No scatter gather\n");
	if (p->ccb_op == COP_CMD_RESID)
		PRF(" Command with residual\n");
	if (p->ccb_op == COP_SG_RESID)
		PRF(" Scatter gather with resid\n");
	if (p->ccb_op == COP_RESET)
		PRF(" Bus Device Reset\n");

	if (p->ccb_op == COP_SCATGATH || p->ccb_op == COP_SG_RESID) {
		for (index = 0; index < AHA_MAX_DMA_SEGS; index++) {
		if (!(scsi_stoh_3byte(p->ccb_sg_list[index].data_ptr)))
				break;
			PRF(" a:0x%x l:0x%x",
		scsi_stoh_3byte(p->ccb_sg_list[index].data_ptr),
		scsi_stoh_3byte(p->ccb_sg_list[index].data_len));
		}
	} else {
		PRF(" Data ptr 0x%x data len 0x%x",
		scsi_stoh_3byte(p->ccb_datap),
		scsi_stoh_3byte(p->ccb_datalen));
	}

	index = p->ccb_hastat & 0xff;
	if (index < 0 || index > HS_BADSEG)
		index = HS_UNKNOWN_ERR;
	PRF("\nctlr stat %s \n", aha_err_strings[index]);
}
#endif
