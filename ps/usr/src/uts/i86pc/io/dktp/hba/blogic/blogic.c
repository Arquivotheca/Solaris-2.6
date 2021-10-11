/*
 * blogic - Solaris device driver for BusLogic SCSI Host Adapter
 *
 * Copyright (c) 1995, BusLogic, Inc.
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)blogic.c	1.10	96/08/07 SMI"

#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#include <sys/dktp/blogic/blogic.h>
#include <sys/condvar.h>
#include <sys/debug.h>
#include <sys/pci.h>

#ifdef PCI_DDI_EMULATION
#define	OLD_PCI
#else
#undef OLD_PCI
#endif

/*
 * External references
 */

static int blogic_tran_tgt_init(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
static int blogic_tran_tgt_probe(struct scsi_device *, int (*)());
static void blogic_tran_tgt_free(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);

static int blogic_transport(struct scsi_address *ap, struct scsi_pkt *pktp);
static int blogic_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static int blogic_reset(struct scsi_address *ap, int level);
static int blogic_capchk(char *cap, int tgtonly, int *cidxp);
static int blogic_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static int blogic_setcap(struct scsi_address *ap, char *cap, int value,
		int tgtonly);
static struct scsi_pkt *blogic_tran_init_pkt(struct scsi_address *ap,
	struct scsi_pkt *pkt, struct buf *bp, int cmdlen, int statuslen,
	int tgtlen, int flags, int (*callback)(), caddr_t arg);
static void blogic_tran_destroy_pkt(struct scsi_address *ap,
		struct scsi_pkt *pkt);
static struct scsi_pkt *blogic_pktalloc(struct scsi_address *ap, int cmdlen,
	int statuslen, int tgtlen, int (*callback)(), caddr_t arg);
static void blogic_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt);
static struct scsi_pkt *blogic_dmaget(struct scsi_pkt *pkt, opaque_t dmatoken,
	int (*callback)(), caddr_t arg);
static void blogic_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);
static void blogic_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);

/*
 * Local Function Prototypes
 */
static int blogic_findhba(uint ioaddr);
static int blogic_propinit(struct blogic_blk *blogic_blkp);
static int blogic_cfginit(struct  blogic_blk *blogic_blkp);
static void blogic_mbox_init(struct blogic_blk *blogic_blkp);
static void blogic_rndrbn_init(struct blogic_blk *blogic_blkp);
static int blogic_docmd(struct blogic_blk *blogic_blkp, int opcode, unchar *sbp,
    unchar *rbp);
static int blogic_wait(ushort port, ushort mask, ushort onbits, ushort offbits);
static int blogic_poll(struct blogic_blk *blogic_blkp);
static int blogic_outmbx(struct blogic_blk *blogic_blkp,
    struct blogic_ccb *ccbp, unchar cmd);
static struct scsi_pkt *blogic_chkerr(struct blogic_blk *blogic_blkp,
    struct blogic_ccb *ccbp, char cmdstat);
static u_int blogic_dummy_intr(caddr_t arg);
static u_int blogic_xlate_vec(struct blogic_blk *blogic_blkp);
static u_int blogic_intr(caddr_t arg);
static void blogic_seterr(struct scsi_pkt *pktp, struct blogic_ccb *ccbp);
static void blogic_reset_ctlr(struct blogic_blk *blogic_blkp);
static void blogic_deallocate(struct blogic *blogic);
static int blogic_search_pci(uint *ioaddr, dev_info_t *devi);
static int blogic_check_device_id(dev_info_t *devi, int device_id);
struct blogic_ccb *blogic_getccb(struct blogic_blk *blogic_blkp);
int	blogic_init_lists(struct blogic_blk *blogic_blkp);
void	blogic_freeccb(struct blogic_blk *blogic_blkp,
	struct blogic_ccb *ccbp);
static void blogic_pollret(struct blogic_blk *blogic_blkp,
	struct scsi_pkt *poll_pktp);
void	blogic_flush_ccbs(struct blogic_blk *blogic_blkp);
void	blogic_timer(caddr_t arg);
void	blogic_sendback_reset(caddr_t arg);
static	void blogic_run_err(struct scsi_pkt *pktp, struct blogic_ccb *ccbp);


/*
 * Local static data
 */
static int blogic_pgsz = 0;
static int blogic_pgmsk;
static int blogic_pgshf;

static kmutex_t blogic_rmutex;
static kmutex_t blogic_global_mutex;
static int blogic_global_init = 0;

static ddi_dma_lim_t blogic_24bit_dma_lim = {
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
	BLOGIC_MAX_DMA_SEGS,	/* scatter/gather list length		*/
	(u_int)0xffffffff	/* request size				*/
};

static ddi_dma_lim_t blogic_32bit_dma_lim = {
	0,			/* address low				*/
	(ulong) 0xffffffff,	/* address high				*/
	0,			/* counter max				*/
	1,			/* burstsize 				*/
	DMA_UNIT_8,		/* minimum xfer				*/
	0,			/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version				*/
	(ulong) 0xffffffff,	/* address register			*/
	0x7fffffff,		/* counter register			*/
	512,			/* sector size				*/
	BLOGIC_MAX_DMA_SEGS,	/* scatter/gather list length		*/
	(ulong) 0xffffffff	/* request size				*/
};


/*
 * ac_flags bits:
 */
#define	ACF_WAITIDLE    0x01    /* Wait for STAT_IDLE before issuing */
#define	ACF_WAITCPLTE   0x02    /* Wait for INT_HACC before returning */
#define	ACF_INVDCMD	0x04    /* INVALID COMMAND CODE */

struct blogic_cmd blogic_cmds[] = {
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 0},	/* 00 - CMD_NOP */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 4, 0},	/* 01 - CMD_MBXINIT */
	{0, 0, 0},				/* 02 - CMD_DOSCSI */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 10, 1},	/* 03 - CMD_ATBIOS */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 4},	/* 04 - CMD_ADINQ */
	{ACF_WAITCPLTE, 1, 0},			/* 05 - CMD_MBOE_CTL */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 4, 0},	/* 06 - CMD_SELTO_CTL */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 0},	/* 07 - CMD_BONTIME */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 0},	/* 08 - CMD_BOFFTIME */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 0},	/* 09 - CMD_XFERSPEED */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 8},	/* 0a - CMD_INSTDEV */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 0, 3},	/* 0b - CMD_CONFIG */
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
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 3, 0},	/* 1c - CMD_WTFIFO */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 3, 0},	/* 1d - CMD_RDFIFO */
	{ACF_INVDCMD, 0, 0},			/* 1e -- INVALID */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 1},	/* 1f - CMD_ECHO */
};

struct blogic_cmd blogic_xcmds[] = {
	{ACF_INVDCMD, 0, 0},			/* 80 -- INVALID */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 5, 0},	/* 81 -- CMD_XMBXINIT */
	{ACF_INVDCMD, 0, 0},			/* 82 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 83 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 84 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 85 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 86 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 87 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 88 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 89 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 8a -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 8b -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 8c -- INVALID */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 15},	/* 8d -- CMD_XINQSETUP */
	{ACF_INVDCMD, 0, 0},			/* 8e -- INVALID */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 0},	/* 8f -- CMD_XSTRICT_RND_RBN */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 3, 8},	/* 90 -- CMD_WRITE_LOC_RAM */
	{0, 2, 8},				/* 91 -- CMD_READ_LOC_RAM */
	{ACF_INVDCMD, 0, 0},			/* 92 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 93 -- INVALID */
	{ACF_INVDCMD, 0, 0},			/* 94 -- INVALID */
	{ACF_WAITIDLE, 1, 0},			/* 95 -- CMD_DISAB_ISA_PORT */
	{(ACF_WAITIDLE|ACF_WAITCPLTE), 1, 0},	/* 96 -- CMD_ENAB_64_LUN */
};

#ifdef	BLOGIC_DEBUG

#define	DOFF	0x0000
#define	DENT	0x0001
#define	DPKT	0x0002
#define	DINM	0x0004
#define	DOUTM	0x0008
#define	DMACROS	0x0010
#define	DINIT	0x0020
int	blogic_debug = DOFF;

struct blogic_dbg {
	ushort	active;
	ushort	idx;
	caddr_t ccb;
	caddr_t	pktp;
	ulong	time;
};
struct blogic_dbg blogic_to[256];
struct blogic_dbg blogic_from[256];
unchar blogic_to_idx = 0;
unchar blogic_from_idx = 0;

unchar blogic_ccb_to1[256][16];
unchar blogic_ccb_to2[256][16];

#endif	/* BLOGIC_DEBUG */

ulong	blogic_dbg_blkp;

static int blogic_identify(dev_info_t *dev);
static int blogic_probe(dev_info_t *);
static int blogic_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int blogic_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

struct dev_ops	blogic_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	blogic_identify,		/* identify */
	blogic_probe,		/* probe */
	blogic_attach,		/* attach */
	blogic_detach,		/* detach */
	nulldev,		/* reset */
	(struct cb_ops *)0,	/* driver operations */
	NULL			/* bus operations */
};


#include <sys/modctl.h>

#ifdef OLD_PCI
char _depends_on[] = "misc/xpci misc/scsi";
#else
char _depends_on[] = "misc/scsi";
#endif

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"BusLogic SCSI Host Adapter Driver",	/* Name of the module. */
	&blogic_ops,	/* driver ops */
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

	mutex_init(&blogic_global_mutex, "BLOGIC global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&blogic_global_mutex);
	}
	return (status);
}

int
_fini(void)
{
	int	status;

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&blogic_global_mutex);
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
blogic_tran_tgt_init(dev_info_t *hba_dip, dev_info_t *tgt_dip,
    scsi_hba_tran_t *hba_tran, struct scsi_device *sd)
{
	int 			targ;
	int			lun;
	struct blogic		*hba_blogicp;
	struct blogic		*unit_blogicp;
	struct blogic_blk	*blogic_blkp;

	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef BLOGIC_DEBUG
	if (blogic_debug & DINIT)
	cmn_err(CE_CONT, "%s%d: %s%d <%d,%d>\n",
	    ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
	    ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip), targ, lun);
#endif

	hba_blogicp = SDEV2HBA(sd);
	blogic_blkp = BLOGIC_BLKP(hba_blogicp);

	if ((targ < 0) || (targ > blogic_blkp->bb_max_targs) ||
	    (lun < 0) || (lun > blogic_blkp->bb_max_luns)) {
		cmn_err(CE_WARN, "%s%d: %s%d bad address <%d,%d>\n",
		    ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		    ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		    targ, lun);
		return (DDI_FAILURE);
	}

	if ((unit_blogicp =
	    kmem_zalloc(sizeof (struct blogic) + sizeof (struct blogic_unit),
	    KM_NOSLEEP)) == NULL) {
		return (DDI_FAILURE);
	}

	bcopy((caddr_t)hba_blogicp, (caddr_t)unit_blogicp,
	    sizeof (*hba_blogicp));

	unit_blogicp->a_unitp = (struct blogic_unit *)(unit_blogicp+1);

	bcopy((caddr_t)blogic_blkp->bb_dmalim_p,
	    (caddr_t)&unit_blogicp->a_unitp->au_lim, sizeof (ddi_dma_lim_t));

	hba_tran->tran_tgt_private = unit_blogicp;

	BLOGIC_BLKP(hba_blogicp)->bb_child++;

#ifdef BLOGIC_DEBUG
	if (blogic_debug & DINIT) {
		PRF("blogic_tran_tgt_init: <%d,%d>\n", targ, lun);
	}
#endif

	return (DDI_SUCCESS);
}


/*ARGSUSED*/
static int
blogic_tran_tgt_probe(
	struct scsi_device	*sd,
	int			(*callback)())
{
	int	rval;

	rval = scsi_hba_probe(sd, callback);

#ifdef BLOGIC_DEBUG
	if (blogic_debug & DINIT) {
		char		*s;
		struct blogic	*blogic = SDEV2BLOGIC(sd);

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
		cmn_err(CE_CONT, "blogic%d: %s target %d lun %d %s\n",
			ddi_get_instance(BLOGIC_DIP(blogic)),
			ddi_get_name(sd->sd_dev),
			sd->sd_address.a_target,
			sd->sd_address.a_lun, s);
	}
#endif	/* BLOGICDEBUG */

	/*
	* Set up device type table.  This will be used mainly
	* to determine if we have a tape device so that we can
	* avoid retrying or timing out on tape requests.
	*/
	if (sd->sd_address.a_lun == 0) {
		struct blogic	*blogic = SDEV2BLOGIC(sd);

		switch (sd->sd_inq->inq_dtype & 0x1F) {
		case DTYPE_SEQUENTIAL:
		    BLOGIC_BLKP(blogic)->bb_dev_type[sd->sd_address.a_target]
			|= TAPE_DEVICE;

		/* FALLTHROUGH */
		case DTYPE_SCANNER:
		case DTYPE_CHANGER:
		case DTYPE_PRINTER:
		    BLOGIC_BLKP(blogic)->bb_dev_type[sd->sd_address.a_target]
			|= SLOW_DEVICE;
		}
	}

	return (rval);
}


/*ARGSUSED*/
static void
blogic_tran_tgt_free(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	struct blogic		*blogic;
	struct blogic		*unit_blogicp;

#ifdef	BLOGICDEBUG
	cmn_err(CE_CONT, "blogic_tran_tgt_free: %s%d %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif	/* BLOGICDEBUG */

	unit_blogicp = hba_tran->tran_tgt_private;
	kmem_free(unit_blogicp,
		sizeof (struct blogic) + sizeof (struct blogic_unit));

	sd->sd_inq = NULL;

	blogic = SDEV2HBA(sd);
	BLOGIC_BLKP(blogic)->bb_child--;
}



/*
 *	Autoconfiguration routines
 */
static int
blogic_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, "blogic") == 0)
		return (DDI_IDENTIFIED);

	if (strcmp(dname, "pci104b,1040") == 0)
		return (DDI_IDENTIFIED);

	if (strcmp(dname, "pci104B,1040") == 0)
		return (DDI_IDENTIFIED);

	return (DDI_NOT_IDENTIFIED);
}

static int
blogic_probe(register dev_info_t *devi)
{
	uint	ioaddr, blogic_pci;
	int	len;
	int *regp;
	int reglen;

#ifdef BLOGIC_DEBUG
	if (blogic_debug & DENT)
		PRF("blogic_probe: blogic devi= 0x%x\n", devi);
#endif

	len = sizeof (int);

	if (HBA_INTPROP(devi, "blogic_pci", &blogic_pci, &len) != DDI_SUCCESS)
		blogic_pci = 1;

	if (blogic_pci) {
		if (blogic_search_pci(&ioaddr, devi))
			return (DDI_PROBE_FAILURE);
	} else {
		if (ddi_getlongprop(DDI_DEV_T_NONE, devi,
			DDI_PROP_DONTPASS, "reg", (caddr_t)&regp,
			&reglen) != DDI_PROP_SUCCESS) {
#ifdef ADP_DEBUG
			if (blogic_debug & DINIT)
				cmn_err(CE_WARN,
				"blogic_probe: reg property not found\n");
#endif
			return (DDI_PROBE_FAILURE);
		}

		ioaddr = *regp;
		kmem_free(regp, reglen);
	}

	if (blogic_findhba(ioaddr) != DDI_SUCCESS)
		return (DDI_PROBE_FAILURE);

	return (DDI_PROBE_SUCCESS);
}

/* returns 0 on success, 1 on failure */
static int
blogic_search_pci(uint *ioaddr, dev_info_t *devi)
{
	ushort			vendor_id;
	ushort			device_id;
	ulong			base;
	ddi_acc_handle_t	cfg_handle;

	if (pci_config_setup(devi, &cfg_handle) != DDI_SUCCESS) {
#ifdef BLOGIC_DEBUG
		if (blogic_debug & DINIT)
			cmn_err(CE_CONT,
			"blogic_search_pci: pci_config_setup failed");
#endif
		return (1);
	}

	vendor_id = pci_config_getw(cfg_handle, PCI_CONF_VENID);

	if (vendor_id != BLOGIC_VENDOR_ID) {
		pci_config_teardown(&cfg_handle);
		return (1);
	}

	device_id = pci_config_getw(cfg_handle, PCI_CONF_DEVID);

	if (blogic_check_device_id(devi, (int)device_id)) {
#ifdef BLOGIC_DEBUG
		if (blogic_debug & DINIT)
		cmn_err(CE_CONT, "blogic_search_pci: bad device %x\n",
			device_id);
#endif
		pci_config_teardown(&cfg_handle);
		return (1);
	}

	base = pci_config_getl(cfg_handle, PCI_CONF_BASE0);
	*ioaddr = (uint) base & 0xfffffe;

	pci_config_teardown(&cfg_handle);

#ifdef BLOGIC_DEBUG
	if (blogic_debug & DINIT)
		cmn_err(CE_CONT,
		"blogic_search_pci: vendor %x device %x ioaddr %x\n",
		vendor_id, device_id, *ioaddr);
#endif
	return (0);
}

/* returns 0 on success, 1 on failure */
static int
blogic_check_device_id(dev_info_t *devi, int device_id)
{
	int		len, count, num_devices;
	int		*p, *dev_ids;

	if (ddi_getlongprop(DDI_DEV_T_ANY, devi, DDI_PROP_DONTPASS,
		"device_ids", (caddr_t)&dev_ids,
		&len) != DDI_PROP_SUCCESS) {
			cmn_err(CE_WARN, "blogic_check_device_id: no property");
		return (1);
	}

	num_devices = len / sizeof (int);

	if (num_devices < 1)
		return (1);

	for (count = 0, p = dev_ids; count < num_devices; p++) {
		if (device_id == *p) {
			(void) kmem_free(dev_ids, len);
			return (0);
		}
	}
	(void) kmem_free(dev_ids, len);
	return (1);
}

static int
blogic_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	register struct	blogic 	*blogic;
	register struct	blogic_blk *blogic_blkp;

	switch (cmd) {
	case DDI_DETACH: {
		scsi_hba_tran_t	*tran;

		tran = (scsi_hba_tran_t *) ddi_get_driver_private(devi);
		if (!tran)
			return (DDI_SUCCESS);
		blogic = TRAN2BLOGIC(tran);
		if (!blogic)
			return (DDI_SUCCESS);
		blogic_blkp = BLOGIC_BLKP(blogic);
		if (blogic_blkp->bb_child)
			return (DDI_FAILURE);

		/*
		* Cancel call to blogic_timer().
		*/
		if (blogic_blkp->bb_timeout_ticks)
			untimeout(blogic_blkp->bb_timeout_id);

		ddi_remove_intr(devi, blogic_blkp->bb_intr_idx,
			blogic_blkp->bb_iblock);

		if (blogic_blkp->bb_boardtype == ISA_HBA)
			ddi_dmae_release(devi, blogic_blkp->bb_dmachan);

		scsi_destroy_cbthread(blogic_blkp->bb_cbthdl);
		mutex_destroy(&blogic_blkp->bb_mutex);

		mutex_enter(&blogic_global_mutex);
		blogic_global_init--;
		if (blogic_global_init == 0)
			mutex_destroy(&blogic_rmutex);
		mutex_exit(&blogic_global_mutex);

		scsi_hba_tran_free(blogic->a_tran);
		blogic_deallocate(blogic);

		ddi_prop_remove_all(devi);
		if (scsi_hba_detach(devi) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "blogic: scsi_hba_detach failed\n");
		}
		return (DDI_SUCCESS);

	}
	default:
		return (DDI_FAILURE);
	}
}

static int
blogic_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	register struct blogic 	*blogic;
	register struct blogic_blk	*blogic_blkp;
	int 			len;
	u_int			intr_idx;
	scsi_hba_tran_t		*hba_tran;
	ulong blogic_pci;

	switch (cmd) {
	case DDI_ATTACH:
		break;

	default:
		return (DDI_FAILURE);
	}

	/*
	 * Attach handling
	 */
	blogic = (struct blogic *)kmem_zalloc((unsigned)(sizeof (*blogic) +
		sizeof (*blogic_blkp)), KM_NOSLEEP);
	if (!blogic)
		return (DDI_FAILURE);
	blogic_blkp = (struct blogic_blk *)(blogic + 1);
	BLOGIC_BLKP(blogic) = blogic_blkp;
	blogic_blkp->bb_dip = devi;

	blogic_dbg_blkp = (ulong) blogic_blkp;

	if ((blogic_propinit(blogic_blkp) == DDI_FAILURE) ||
	    (blogic_cfginit(blogic_blkp)  == DDI_FAILURE) ||
	    (blogic_init_lists(blogic_blkp) == DDI_FAILURE)) {
		blogic_deallocate(blogic);
		return (DDI_FAILURE);
	}

	/*
	 * Allocate a transport structure
	 */
	hba_tran = scsi_hba_tran_alloc(devi, 0);
	if (hba_tran == NULL) {
		cmn_err(CE_WARN, "blogic_attach: scsi_hba_tran_alloc failed\n");
		blogic_deallocate(blogic);
		return (DDI_FAILURE);
	}

	if (blogic_blkp->bb_boardtype == ISA_HBA) {
		if (ddi_dmae_alloc(devi, blogic_blkp->bb_dmachan,
		    DDI_DMA_DONTWAIT, NULL) != DDI_SUCCESS) {
			blogic_deallocate(blogic);
			scsi_hba_tran_free(hba_tran);
			return (DDI_FAILURE);
		}
	}

/*
 *	If the adapter is PCI, there is no individual conf file entry,
 *	and "bustype" is not defined, and the framework sets up the
 *	"interrupt" property for us, otherwise (for ISA and EISA) we must
 *	do it in blogic_xlate_vec.
 */
	len = sizeof(int);
	if (HBA_INTPROP(devi, "blogic_pci", &blogic_pci, &len) != DDI_SUCCESS) {
		blogic_pci = 1;
		intr_idx = 0;
	} 

	if (blogic_pci == 0) {
		intr_idx = blogic_xlate_vec(blogic_blkp);

		if (intr_idx ==  UINT_MAX) {
			blogic_deallocate(blogic);
			scsi_hba_tran_free(hba_tran);
			return (DDI_FAILURE);
		}
	}
/*
 *	Establish initial dummy interrupt handler
 *	get iblock cookie to initialize mutexes used in the
 *	real interrupt handler
 */
	if (ddi_add_intr(devi, intr_idx,
			(ddi_iblock_cookie_t *) &blogic_blkp->bb_iblock,
			(ddi_idevice_cookie_t *) 0, blogic_dummy_intr,
			(caddr_t)blogic)) {
		cmn_err(CE_WARN, "blogic_attach: cannot add intr\n");
		blogic_deallocate(blogic);
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	mutex_init(&blogic_blkp->bb_mutex, "blogic mutex", MUTEX_DRIVER,
		blogic_blkp->bb_iblock);

	ddi_remove_intr(devi, intr_idx, blogic_blkp->bb_iblock);
/*	Establish real interrupt handler				*/
	if (ddi_add_intr(devi, intr_idx,
		(ddi_iblock_cookie_t *) &blogic_blkp->bb_iblock,
		(ddi_idevice_cookie_t *)0, blogic_intr, (caddr_t)blogic)) {
		cmn_err(CE_WARN, "blogic_attach: cannot add intr\n");
		blogic_deallocate(blogic);
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}
	blogic_blkp->bb_intr_idx = intr_idx;
	blogic_blkp->bb_cbthdl = scsi_create_cbthread(blogic_blkp->bb_iblock,
		KM_NOSLEEP);

	blogic->a_tran = hba_tran;

	hba_tran->tran_hba_private	= blogic;
	hba_tran->tran_tgt_private	= NULL;

	hba_tran->tran_tgt_init		= blogic_tran_tgt_init;
	hba_tran->tran_tgt_probe	= blogic_tran_tgt_probe;
	hba_tran->tran_tgt_free		= blogic_tran_tgt_free;

	hba_tran->tran_start 		= blogic_transport;
	hba_tran->tran_abort		= blogic_abort;
	hba_tran->tran_reset		= blogic_reset;
	hba_tran->tran_getcap		= blogic_getcap;
	hba_tran->tran_setcap		= blogic_setcap;
	hba_tran->tran_init_pkt 	= blogic_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= blogic_tran_destroy_pkt;
	hba_tran->tran_dmafree		= blogic_dmafree;
	hba_tran->tran_sync_pkt		= blogic_sync_pkt;

	if (scsi_hba_attach(devi, blogic_blkp->bb_dmalim_p,
			hba_tran, SCSI_HBA_TRAN_CLONE, NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "blogic_attach: scsi_hba_attach failed\n");
		scsi_destroy_cbthread(blogic_blkp->bb_cbthdl);
		if (blogic_blkp->bb_boardtype == ISA_HBA)
			ddi_dmae_release(devi, blogic_blkp->bb_dmachan);
		ddi_remove_intr(devi, intr_idx, blogic_blkp->bb_iblock);
		blogic_deallocate(blogic);
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	mutex_enter(&blogic_global_mutex);
	if (!blogic_global_init) {
		mutex_init(&blogic_rmutex, "BLOGIC Resource Mutex",
			MUTEX_DRIVER, blogic_blkp->bb_iblock);
	}
	blogic_global_init++;
	mutex_exit(&blogic_global_mutex);

	ddi_report_dev(devi);

	blogic_blkp->bb_flag |= INSTALLED;

	if (blogic_blkp->bb_timeout_ticks)
		blogic_timer((caddr_t) blogic_blkp);

	return (DDI_SUCCESS);
}


/*
* blogic_deallocate()
*
* Description
*	blogic_deallocate frees up space allocated for the
*	corresponding controller.
*/
static void
blogic_deallocate(struct blogic *blogic)
{
	register struct blogic_blk *blogic_blkp;

	blogic_blkp = BLOGIC_BLKP(blogic);

	/*
	* Free mailboxes
	*/
	if (blogic_blkp->bb_FirstMboxOut)
		ddi_iopb_free((caddr_t)blogic_blkp->bb_FirstMboxOut);

	/*
	* Free ccb's
	*/
	if (blogic_blkp->bb_ccblist)
		ddi_iopb_free(blogic_blkp->bb_ccblist);

	/*
	* Free scatter gather lists
	*/
	if (blogic_blkp->bb_dmalist)
		ddi_iopb_free(blogic_blkp->bb_dmalist);

	/*
	* Free blogic adapter table
	*/
	kmem_free((caddr_t)blogic, (sizeof (*blogic) + sizeof (*blogic_blkp)));
}


/*
* blogic_propinit()
*
* Description
*	blogic_propinit gets tuneable parameters from the
*	blogic.conf file and sets up the controller
*	environment accordingly.
*/
static int
blogic_propinit(register struct blogic_blk *blogic_blkp)
{
	register dev_info_t	*devi = blogic_blkp->bb_dip;
	int			i, len;
	ulong			blogic_pci;
	uint			val;
	int 			*regp;
	int			reglen;
	ddi_acc_handle_t	cfg_handle;

#ifdef BLOGIC_DEBUG
	if (blogic_debug & DENT)
		PRF("blogic_propinit: blogic devi= 0x%x\n", devi);
#endif

	len = sizeof (int);

	if (HBA_INTPROP(devi, "blogic_pci", &blogic_pci, &len) != DDI_SUCCESS)
		blogic_pci = 1;

	if (blogic_pci) {
		if (blogic_search_pci(&val, devi))
			return (DDI_PROBE_FAILURE);

		blogic_blkp->bb_base_ioaddr	= (ushort) val;
		blogic_blkp->bb_datacmd_ioaddr	= (ushort) val + BLOGICDATACMD;
		blogic_blkp->bb_intr_ioaddr	= (ushort) val + BLOGICINTFLGS;

		/*
		* Get PCI IRQ vector.
		*/
		if (pci_config_setup(devi, &cfg_handle) != DDI_SUCCESS) {
#ifdef BLOGIC_DEBUG
			if (blogic_debug & DINIT)
				cmn_err(CE_CONT,
				"blogic_propinit: pci_config_setup fail");
#endif
			return (DDI_PROBE_FAILURE);
		}
		val = (uint) pci_config_getl(cfg_handle, PCI_CONF_ILINE);
		pci_config_teardown(&cfg_handle);

		blogic_blkp->bb_intr = (unchar) val & 0xf;

		blogic_blkp->bb_boardtype	= PCI_HBA;
	} else {
		if (ddi_getlongprop(DDI_DEV_T_NONE, devi,
			DDI_PROP_DONTPASS, "reg", (caddr_t)&regp,
			&reglen) != DDI_PROP_SUCCESS) {
#ifdef ADP_DEBUG
			if (blogic_debug & DINIT)
				cmn_err(CE_WARN,
				"blogic_propinit: no reg property\n");
#endif
			return (DDI_PROBE_FAILURE);
		}

		val = *regp;
		kmem_free(regp, reglen);
		blogic_blkp->bb_base_ioaddr	= (ushort) val;
		blogic_blkp->bb_datacmd_ioaddr	= (ushort) val + BLOGICDATACMD;
		blogic_blkp->bb_intr_ioaddr	= (ushort) val + BLOGICINTFLGS;
	}

	if (HBA_INTPROP(devi, "dmachan", &val, &len) == DDI_PROP_SUCCESS)
		blogic_blkp->bb_dmachan  = (unchar) val;

	if (HBA_INTPROP(devi, "dmaspeed", &val, &len) == DDI_PROP_SUCCESS)
		blogic_blkp->bb_dmaspeed = (unchar) val;

	if (HBA_INTPROP(devi, "buson", &val, &len) == DDI_PROP_SUCCESS)
		blogic_blkp->bb_buson    = (unchar) val;

	if (HBA_INTPROP(devi, "busoff", &val, &len) == DDI_PROP_SUCCESS)
		blogic_blkp->bb_busoff   = (unchar) val;

	/*
	* "retry_max" is the number of times a command is retried
	* because of timeout.
	*/
	if (HBA_INTPROP(devi, "retry_max", &val, &len) == DDI_PROP_SUCCESS)
		blogic_blkp->bb_retry_max = val;
	else
		blogic_blkp->bb_retry_max = BLOGIC_RETRY_MAX;

	/*
	* "reset_max" is the number of times a controller is reset
	* before it is disabled.
	*/
	if (HBA_INTPROP(devi, "reset_max", &val, &len) == DDI_PROP_SUCCESS)
		blogic_blkp->bb_reset_max = val;
	else
		blogic_blkp->bb_reset_max = BLOGIC_RESET_MAX;

	/*
	* "timeout_delay" is the number of secs the controller is
	* given to process a command before the command is aborted
	* and retried.
	*/
	if (HBA_INTPROP(devi, "timeout_delay", &val, &len) == DDI_PROP_SUCCESS)
		blogic_blkp->bb_timeout_ticks = val * HZ;
	else
		blogic_blkp->bb_timeout_ticks = BLOGIC_TIMEOUT_SECS * HZ;

	/*
	* "tagged_queue" value of 1 enables command tag queueing to
	* the hard disks.
	*/
	if (HBA_INTPROP(devi, "tagged_queue", &val, &len) == DDI_PROP_SUCCESS) {
		if (val == 0)
			blogic_blkp->bb_flag |= TAG_Q_OFF;
	}
	else
		blogic_blkp->bb_flag |= TAG_Q_OFF;

	/*
	* "nmbox" specifies the number of mailboxes used to pass
	* commands to the controller.
	*/
	if (HBA_INTPROP(devi, "nmbox", &val, &len) == DDI_PROP_SUCCESS)
		if (val > BLOGIC_MAX_NUM_MBOX)
			blogic_blkp->bb_num_mbox = (unchar) BLOGIC_MAX_NUM_MBOX;
		else
			blogic_blkp->bb_num_mbox = (unchar) val;
	else
		blogic_blkp->bb_num_mbox = 32;

	if (blogic_blkp->bb_num_mbox < BLOGIC_MIN_NUM_MBOX)
		blogic_blkp->bb_num_mbox = BLOGIC_MIN_NUM_MBOX;

	mutex_enter(&blogic_global_mutex);

	if (!blogic_pgsz) {
		blogic_pgsz = ddi_ptob(devi, 1L);
		blogic_pgmsk = blogic_pgsz - 1;

		for (i = blogic_pgsz, len = 0; i > 1; len++) {
			i >>= 1;
		}

		blogic_pgshf = len;
#ifdef BLOGIC_DEBUG
		if (blogic_debug & DMACROS) {
			PRF("blogic_propinit(): pgsz %x pgmsk %x pgshf %x\n",
			    blogic_pgsz, blogic_pgmsk, blogic_pgshf);
		}
#endif
	}
	mutex_exit(&blogic_global_mutex);

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
blogic_transport(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct blogic_blk *blogic_blkp;
	register struct	blogic_ccb *ccbp;

	ccbp = (struct blogic_ccb *)SCMD_PKTP(pktp)->cmd_private;

#ifdef BLOGIC_DEBUG
	if (ccbp->ccb_flag & ACTIVE) {
		cmn_err(CE_WARN,
			"blogic_transport: want to use active ccb %x\n", ccbp);
		return (TRAN_ACCEPT);
	}
#endif

	blogic_blkp = PKT2BLOGICBLKP(pktp);

	/*
	* If a SCSI Bus Reset occurred, send the reset status back
	* to the tape device driver.  This driver transparently
	* retries the request for other devices.
	*/
	if ((blogic_blkp->bb_dev_type[ccbp->ccb_tf_tid] & RESET_CONDITION) &&
		!(pktp->pkt_flags & FLAG_NOINTR)) {
		/*
		* Use timeout() because we need to wakeup the
		* tape driver sleeping on this pktp but the
		* driver does not sleep until blogic_transport()
		* returns.  Two seconds ought to be enough.
		*/
		ccbp->ccb_timeout_id =
		    timeout(blogic_sendback_reset, (caddr_t) pktp, 2 * HZ);
		return (TRAN_ACCEPT);
	}

	if (blogic_blkp->bb_flag & WIDE_SCSI) {
		if ((blogic_blkp->bb_flag & TAG_Q_SUPPORTED) &&
		    (pktp->pkt_flags & FLAG_TAGMASK)) {
			switch (pktp->pkt_flags & FLAG_TAGMASK) {
			case FLAG_OTAG:
				ccbp->ccb_datadir = ORDERED;
				break;

			case FLAG_STAG:
				ccbp->ccb_datadir = SIMPLE;
				break;

			case FLAG_HTAG:
				ccbp->ccb_datadir = QUEUEHEAD;
				break;
			} /* end switch */
		}
	} else {
		if ((blogic_blkp->bb_flag & TAG_Q_SUPPORTED) &&
		    (pktp->pkt_flags & FLAG_TAGMASK)) {
			switch (pktp->pkt_flags & FLAG_TAGMASK) {
			case FLAG_OTAG:
				ccbp->ccb_tf_lun |= ORDERED;
				break;

			case FLAG_STAG:
				ccbp->ccb_tf_lun |= SIMPLE;
				break;

			case FLAG_HTAG:
				ccbp->ccb_tf_lun |= QUEUEHEAD;
				break;
			} /* end switch */
		}
	}

	mutex_enter(&blogic_blkp->bb_mutex);

#ifdef BLOGIC_DEBUG
	blogic_to[blogic_to_idx].idx = blogic_to_idx;
	blogic_to[blogic_to_idx].active = 1;
	blogic_to[blogic_to_idx].ccb = (caddr_t) ccbp;
	blogic_to[blogic_to_idx].pktp = (caddr_t) pktp;
	blogic_to[blogic_to_idx].time = lbolt;
	bcopy((caddr_t) ccbp, (caddr_t) &blogic_ccb_to1[blogic_to_idx], 16);
	bcopy((caddr_t) ((caddr_t)ccbp + 16),
		(caddr_t) &blogic_ccb_to2[blogic_to_idx], 16);
	ccbp->ccb_dbg_idx = blogic_to_idx;
	if ((caddr_t) pktp != (caddr_t) ccbp->ccb_ownerp)
		cmn_err(CE_WARN,
			"blogic_transport: pktp != ccbp->ccb_ownerp!");
	blogic_to_idx++;
#endif

	ccbp->ccb_retrycnt = 0;
	ccbp->ccb_abortcnt = 0;

	if (pktp->pkt_flags & FLAG_NOINTR) {
		if (blogic_outmbx(blogic_blkp, ccbp, MBX_CMD_START)) {
			/*
			* We will get here if the adapter is hung up
			* for some reason.  We can try to hard reset the
			* adapter to bring it back to life.
			*/
			blogic_blkp->bb_reset_cnt++;
			blogic_reset_ctlr(blogic_blkp);
			mutex_exit(&blogic_blkp->bb_mutex);
			return (TRAN_BUSY);
		}

		blogic_pollret(blogic_blkp, pktp);

		/*
		* If we timeout on the request, abort it.
		*/
		if (pktp->pkt_reason == CMD_INCOMPLETE) {
			if (blogic_outmbx(blogic_blkp, ccbp, MBX_CMD_ABORT)) {
				/*
				* We'll get here if the adapter is hung
				* up for some reason.  We can try to
				* hard reset the adapter to bring it back
				* to life.
				*/
				blogic_blkp->bb_reset_cnt++;
				blogic_reset_ctlr(blogic_blkp);
				mutex_exit(&blogic_blkp->bb_mutex);
				return (TRAN_BUSY);
			}

			/*
			* Call blogic_pollret() to process ABORT
			* acknowledgement intr.
			*/
			blogic_pollret(blogic_blkp, pktp);
		}
	} else {
		if (blogic_outmbx(blogic_blkp, ccbp, MBX_CMD_START)) {
			/*
			* We'll get here if the adapter is hung up
			* for some reason.  We can try to hard reset the
			* adapter to bring it back to life.
			*/
			blogic_blkp->bb_reset_cnt++;
			blogic_reset_ctlr(blogic_blkp);
			mutex_exit(&blogic_blkp->bb_mutex);
			return (TRAN_BUSY);
		}
	}

	mutex_exit(&blogic_blkp->bb_mutex);
	return (TRAN_ACCEPT);
}

/*ARGSUSED*/
static int
blogic_abort_cmd(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	struct blogic_blk *blogic_blkp;
	struct blogic_ccb *ccbp;

	blogic_blkp = ADDR2BLOGICBLKP(ap);
	if (((ccbp = ((struct scsi_cmd *)pkt)->cmd_private)) ==
		(struct blogic_ccb *)0)
		return (FALSE);

	if (blogic_outmbx(blogic_blkp, ccbp, MBX_CMD_ABORT)) {
		/*
		* We'll get here if the adapter is hung up
		* for some reason.  We can try to hard reset the
		* adapter to bring it back to life.
		*/
		blogic_blkp->bb_reset_cnt++;
		blogic_reset_ctlr(blogic_blkp);
	}

	return (TRUE);
}

/*ARGSUSED*/
static int
blogic_abort(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	struct blogic_blk *blogic_blkp;
	struct blogic_ccb *ccbp;

	blogic_blkp = ADDR2BLOGICBLKP(ap);
	if (((ccbp = ((struct scsi_cmd *)pkt)->cmd_private)) ==
		(struct blogic_ccb *)0)
		return (FALSE);

	mutex_enter(&blogic_blkp->bb_mutex);
	if (blogic_outmbx(blogic_blkp, ccbp, MBX_CMD_ABORT)) {
		/*
		* We'll get here if the adapter is hung up
		* for some reason.  We can try to hard reset the
		* adapter to bring it back to life.
		*/
		blogic_blkp->bb_reset_cnt++;
		blogic_reset_ctlr(blogic_blkp);
	}
	mutex_exit(&blogic_blkp->bb_mutex);

	return (TRUE);
}

/*ARGSUSED*/
static int
blogic_reset(struct scsi_address *ap, int level)
{
	struct	blogic_blk *blogic_blkp;

	blogic_blkp = ADDR2BLOGICBLKP(ap);
	mutex_enter(&blogic_blkp->bb_mutex);
	blogic_reset_ctlr(blogic_blkp);
	mutex_exit(&blogic_blkp->bb_mutex);

	return (TRUE);
}

static int
blogic_capchk(char *cap, int tgtonly, int *cidxp)
{
	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *)0)
		return (FALSE);

	*cidxp = scsi_hba_lookup_capstr(cap);
	return (TRUE);
}

/*ARGSUSED*/
static int
blogic_getcap(struct scsi_address *ap, char *cap, int tgtonly)
{
	int		ckey;
	uint		heads = 64;
	uint		spt = 32;
	ulong		capacity;
	struct blogic_blk	*blogic_blkp;

	if (blogic_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {

	case SCSI_CAP_GEOMETRY:
		blogic_blkp = ADDR2BLOGICBLKP(ap);
		if (blogic_blkp->bb_flag & VAR_XLAT_SCHEME) {
			capacity = (ADDR2BLOGICUNITP(ap))->au_capacity;
			if ((capacity <= 0x1fffff) &&
			    (capacity <= 0x3fffff))
				heads = 128;	/* 1G-2G uses 128 hds */
			else if (capacity > 0x3fffff) {
			heads = 255;	/* +2G uses 255 hds */
				spt = 63;	/* and 63 SPT */
			}
		}
		return (HBA_SETGEOM(heads, spt));
	case SCSI_CAP_ARQ:
		return (TRUE);
	default:
		break;
	}

	return (UNDEFINED);
}

static int
blogic_setcap(struct scsi_address *ap, char *cap, int value, int tgtonly)
{
	int	ckey, status = FALSE;

	if (blogic_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {
		case SCSI_CAP_SECTOR_SIZE:
			(ADDR2BLOGICUNITP(ap))->au_lim.dlim_granular =
						(u_int)value;
			status = TRUE;
			break;

		case SCSI_CAP_ARQ:
			if (tgtonly) {
				(ADDR2BLOGICUNITP(ap))->au_arq = (u_int)value;
				status = TRUE;
			}
			break;

		default:
			break;
	}

	return (status);
}


static struct scsi_pkt *
blogic_tran_init_pkt(struct scsi_address *ap, struct scsi_pkt *pkt,
	struct buf *bp, int cmdlen, int statuslen, int tgtlen,
	int flags, int (*callback)(), caddr_t arg)
{
	struct scsi_pkt		*new_pkt = (struct scsi_pkt *)0;

	/*
	 * Allocate a pkt
	 */
	if (!pkt) {
		pkt = blogic_pktalloc(ap, cmdlen, statuslen,
			tgtlen, callback, arg);
		if (pkt == (struct scsi_pkt *)0)
			return ((struct scsi_pkt *)0);
		((struct scsi_cmd *)pkt)->cmd_flags = flags;
		new_pkt = pkt;
	} else {
		new_pkt = NULL;
	}

	/*
	 * Set up dma info
	 */
	if (bp) {
		if (blogic_dmaget(pkt, (opaque_t)bp, callback, arg) == NULL) {
			if (new_pkt)
				blogic_pktfree(ap, new_pkt);
			return (NULL);
		}
	}

	return (pkt);
}


static void
blogic_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	blogic_dmafree(ap, pkt);
	blogic_pktfree(ap, pkt);
}


static struct scsi_pkt *
blogic_pktalloc(
	struct scsi_address	*ap,
	int			cmdlen,
	int			statuslen,
	int			tgtlen,
	int			(*callback)(),
	caddr_t			arg)
{
	register struct scsi_cmd	*cmd;
	register struct blogic_ccb	*ccbp;
	struct blogic_blk		*blogic_blkp;
	int				kf;
	caddr_t				tgt;

	blogic_blkp = ADDR2BLOGICBLKP(ap);
	kf = HBA_KMFLAG(callback);

	/*
	 * Allocate target-private data, if necessary
	 */
	if (tgtlen > PKT_PRIV_LEN) {
		tgt = kmem_zalloc(tgtlen, kf);
		if (!tgt) {
			ASSERT(callback != SLEEP_FUNC);
			if (callback != NULL_FUNC)
				ddi_set_callback(callback, arg,
				    &blogic_blkp->blogic_cb_id);
			return ((struct scsi_pkt *)NULL);
		}
	} else {
		tgt = NULL;
	}

	mutex_enter(&blogic_blkp->bb_mutex);
	cmd = (struct scsi_cmd *) kmem_zalloc(sizeof (*cmd), kf);
	if (cmd) {
/* 		allocate ccb 						*/
		if (!(ccbp = blogic_getccb(blogic_blkp))) {
			kmem_free((caddr_t)cmd, sizeof (*cmd));
			cmd = NULL;
		}
	}
	mutex_exit(&blogic_blkp->bb_mutex);

	if (!cmd) {
		if (tgt)
			kmem_free(tgt, tgtlen);
		ASSERT(callback != SLEEP_FUNC);
		if (callback != NULL_FUNC)
			ddi_set_callback(callback, arg,
			    &blogic_blkp->blogic_cb_id);
		return ((struct scsi_pkt *)NULL);
	}

/* 	initialize ccb 							*/
	ccbp->ccb_tf_tid = ap->a_target;
	ccbp->ccb_tf_lun = ap->a_lun;
	ccbp->ccb_cdblen = (u_char)cmdlen;
	ccbp->ccb_ownerp = cmd;
	if (!(ADDR2BLOGICUNITP(ap))->au_arq)
		ccbp->ccb_senselen = 1;
	else
		ccbp->ccb_senselen = BLOGIC_SENSE_LEN;

	bzero((caddr_t)ccbp->ccb_cdb, HBA_MAX_CDB_LEN);

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

#ifdef BLOGIC_DEBUG
	if (blogic_debug & DPKT) {
		PRF("blogic_pktalloc:cmdpktp %x pkt_cdbp %x pkt_scbp %x\n",
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
blogic_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	register struct scsi_cmd *cmd = (struct scsi_cmd *) pkt;
	register struct blogic_blk *blogic_blkp;

	if (cmd->cmd_privlen > PKT_PRIV_LEN) {
		kmem_free(pkt->pkt_private, cmd->cmd_privlen);
	}

	blogic_blkp = ADDR2BLOGICBLKP(ap);

	mutex_enter(&blogic_blkp->bb_mutex);
/*	deallocate the ccb						*/
	if (cmd->cmd_private)
		blogic_freeccb(blogic_blkp,
		    (struct blogic_ccb *)cmd->cmd_private);
/*	free the common packet						*/
	kmem_free((caddr_t)cmd, sizeof (*cmd));
	mutex_exit(&blogic_blkp->bb_mutex);

	if (blogic_blkp->blogic_cb_id)
		ddi_run_callback(&blogic_blkp->blogic_cb_id);
}

/*
 * Dma resource deallocation
 */
/*ARGSUSED*/
void
blogic_dmafree(struct scsi_address *ap, register struct scsi_pkt *pktp)
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
blogic_sync_pkt(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register int i;
	register struct	scsi_cmd *cmd = (struct scsi_cmd *) pktp;

	if (cmd->cmd_dmahandle) {
		i = ddi_dma_sync(cmd->cmd_dmahandle, 0, 0,
			(cmd->cmd_cflags & CFLAG_DMASEND) ?
			DDI_DMA_SYNC_FORDEV : DDI_DMA_SYNC_FORCPU);
		if (i != DDI_SUCCESS) {
			cmn_err(CE_WARN, "blogic: sync pkt failed\n");
		}
	}
}


static struct scsi_pkt *
blogic_dmaget(
	struct scsi_pkt	*pktp,
	opaque_t	dmatoken,
	int		(*callback)(),
	caddr_t		arg)
{
	struct buf *bp = (struct buf *) dmatoken;
	register struct scsi_cmd *cmd = (struct scsi_cmd *) pktp;
	register struct blogic_ccb *ccbp;
	struct blogic_dma_seg *dmap;
	ddi_dma_cookie_t dmack;
	ddi_dma_cookie_t *dmackp = &dmack;
	int	cnt;
	int	bxfer;
	off_t	offset;
	off_t	len;

	ccbp = (struct blogic_ccb *)cmd->cmd_private;
	ccbp->ccb_datadir &= ~CTD_MASK;

	if (!bp->b_bcount) {
		cmd->cmd_pkt.pkt_resid = 0;
		return (pktp);
	}

	if (bp->b_flags & B_READ) {
		ccbp->ccb_datadir |= CTD_IN;
		cmd->cmd_cflags &= ~CFLAG_DMASEND;
	} else {
		ccbp->ccb_datadir |= CTD_OUT;
		cmd->cmd_cflags |= CFLAG_DMASEND;
	}

/*	setup dma memory and position to the next xfer segment		*/
	if (!scsi_impl_dmaget(pktp, (opaque_t)bp, callback, arg,
		&(PKT2BLOGICUNITP(pktp)->au_lim)))
		return (NULL);
	ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len, dmackp);

/*	check for one single block transfer				*/
	if (bp->b_bcount <= dmackp->dmac_size) {
		ccbp->ccb_op = COP_CMD_RESID;
		ccbp->ccb_datalen = bp->b_bcount;
		ccbp->ccb_datap = dmackp->dmac_address;
		pktp->pkt_resid = 0;
		cmd->cmd_totxfer = bp->b_bcount;
	} else {
/*		use scatter-gather transfer				*/
		ccbp->ccb_op = COP_SG_RESID;
		dmap = ccbp->ccb_sg_list;
		for (bxfer = 0, cnt = 1; ; cnt++, dmap++) {
			bxfer += dmackp->dmac_size;
			dmap->data_len = dmackp->dmac_size;
			dmap->data_ptr = dmackp->dmac_address;

/*			check for end of list condition			*/
			if (bp->b_bcount == (bxfer + cmd->cmd_totxfer))
				break;
			ASSERT(bp->b_bcount > (bxfer + cmd->cmd_totxfer));
/*			check end of physical scatter-gather list limit	*/
			if (cnt >= BLOGIC_MAX_DMA_SEGS)
				break;
/*			check for transfer count			*/
			if (bxfer >=
				(PKT2BLOGICUNITP(pktp)->au_lim.dlim_reqsize))
				break;
			if (ddi_dma_nextseg(cmd->cmd_dmawin, cmd->cmd_dmaseg,
				&cmd->cmd_dmaseg) != DDI_SUCCESS)
				break;
			ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset,
				&len, dmackp);
		}
		ccbp->ccb_datalen = (ulong)(cnt*sizeof (struct blogic_dma_seg));
		ccbp->ccb_datap = BLOGIC_KVTOP(ccbp->ccb_sg_list);

		cmd->cmd_totxfer += bxfer;
		pktp->pkt_resid = bp->b_bcount - cmd->cmd_totxfer;
	}
	return (pktp);
}

/*
 *	Adapter Dependent Layer
 */

static int
blogic_findhba(register uint ioaddr)
{
	/*
	* Check to see if it looks like it might be a BusLogic controller.
	*/
	if ((inb(ioaddr+BLOGICSTAT) & (0xF0 & ~STAT_INIT)) != STAT_IDLE) {
		return (DDI_FAILURE);
	}

	/*
	* Clear any pending status.
	*/
	outb(ioaddr+BLOGICSTAT, CTL_IRST);

	/*
	* Make sure the status register did not get changed as a result
	* of writing to the control register.
	*/
	if ((inb(ioaddr+BLOGICSTAT) & (0xF0 & ~STAT_INIT)) != STAT_IDLE) {
		return (DDI_FAILURE);
	}

/*	probe for the hba						*/
/*	test for self echoing						*/
	outb(ioaddr+BLOGICDATACMD, CMD_ECHO);
	if (blogic_wait(ioaddr+BLOGICSTAT, STAT_MASK, 0,
	    (STAT_STST | STAT_CDF | STAT_DF | STAT_INVDCMD)))
		return (DDI_FAILURE);

	outb(ioaddr+BLOGICDATACMD, 0x5a);
	if (blogic_wait(ioaddr+BLOGICSTAT, STAT_MASK, (STAT_DF),
	    (STAT_STST | STAT_CDF | STAT_INVDCMD)))
		return (DDI_FAILURE);

	if (inb(ioaddr+BLOGICDATACMD) != 0x5a ||
		inb(ioaddr+BLOGICSTAT) & STAT_DF)
		return (DDI_FAILURE);

	if (blogic_wait(ioaddr+BLOGICINTFLGS, INT_HACC, INT_HACC, 0))
		return (DDI_FAILURE);

	if (blogic_wait(ioaddr+BLOGICSTAT, STAT_MASK, STAT_IDLE,
	    (STAT_STST | STAT_CDF | STAT_DF | STAT_INVDCMD)))
		return (DDI_FAILURE);

/*	Soft reset							*/
	outb(ioaddr+BLOGICCTL, CTL_SRST);
	drv_usecwait(2 * SEC_INUSEC);

	/*
	* Clear interrupt issued due to Soft Reset
	*/
	outb(ioaddr+BLOGICSTAT, CTL_IRST);

	if (blogic_wait(ioaddr+BLOGICSTAT, STAT_MASK, (STAT_INIT | STAT_IDLE),
	    (STAT_STST | STAT_CDF | STAT_DF | STAT_INVDCMD)))
		return (DDI_FAILURE);

	if (inb(ioaddr+BLOGICINTFLGS))
		return (DDI_FAILURE);

	/* Note that the real mode driver will leave interrupts off here */

	return (DDI_SUCCESS);
}

static u_int
blogic_xlate_vec(struct blogic_blk *blogic_blkp)
{
	register int vec = blogic_blkp->bb_intr;
	int	 intrspec[3];

	if (vec < 5 || vec > 15) {
#ifdef BLOGIC_DEBUG
		if (blogic_debug & DINIT)
			cmn_err(CE_WARN, "blogic_xlate_irq: bad IRQ %d", vec);
#endif
		return ((u_int)UINT_MAX);
	}

	/* create an interrupt spec using default interrupt priority level */
	intrspec[0] = 2;
	intrspec[1] = 5;
	intrspec[2] = vec; /* set irq */

	if (ddi_ctlops(blogic_blkp->bb_dip, blogic_blkp->bb_dip,
	    DDI_CTLOPS_XLATE_INTRS, (caddr_t)intrspec,
	    ddi_get_parent_data(blogic_blkp->bb_dip)) != DDI_SUCCESS) {
#ifdef BLOGIC_DEBUG
		if (blogic_debug & DINIT)
			cmn_err(CE_WARN,
			    "blogic_xlate_vec: interrupt create failed");
#endif
		return ((u_int)UINT_MAX);
	}

	return (0);
}

static int
blogic_cfginit(register struct  blogic_blk *blogic_blkp)
{
	register int	i;
	register unchar *blogicbuf;
	int		initmem;
	caddr_t		buf;
	ushort		port, data, data2;

	/*
	* Test for clone boards
	*/
	port = blogic_blkp->bb_intr_ioaddr;
	data = inb(port);
	outb(port, 0x55);
	data2 = inb(port);
	if (data2 != 0x55)
		return (DDI_FAILURE);
	else {
		outb(port, 0xaa);
		data2 = inb(port);
		if (data2 !=  0xaa)
		return (DDI_FAILURE);
	}
	outb(port, data);		/* restore original port value */

/*	allocate memory for mailbox initialization			*/
	i = (sizeof (struct mbox_entry)*blogic_blkp->bb_num_mbox) << 1;
	initmem = i + 32;	/* buffer allocation for misc commands */

	if (ddi_iopb_alloc(blogic_blkp->bb_dip, blogic_blkp->bb_dmalim_p,
	    (u_int) initmem, &buf)) {
		cmn_err(CE_CONT,
			"blogic_cfginit: unable to allocate memory.\n");
		return (DDI_FAILURE);
	}
	bzero(buf, initmem);

	blogic_blkp->bb_FirstMboxOut = (struct mbox_entry *)buf;
	blogic_blkp->bb_CurrMboxOut = (struct mbox_entry *)buf;
	blogic_blkp->bb_LastMboxOut = blogic_blkp->bb_FirstMboxOut +
	    (blogic_blkp->bb_num_mbox - 1);
	blogic_blkp->bb_buf = (unchar *)(buf + i);
	blogicbuf = blogic_blkp->bb_buf;

	blogic_mbox_init(blogic_blkp);

	(void) blogic_docmd(blogic_blkp, CMD_ADINQ, NULL, blogicbuf);
	blogic_blkp->bb_bdid = blogicbuf[0];
	blogic_blkp->bb_fwrev = blogicbuf[2] - '0';
	blogic_blkp->bb_fwver = blogicbuf[3] - '0';

	blogic_rndrbn_init(blogic_blkp);

	(void) blogic_docmd(blogic_blkp, CMD_CONFIG, NULL, blogicbuf);
	blogic_blkp->bb_targetid = blogicbuf[2];
	if (blogicbuf[0] & CFG_DMA_MASK) {
		for (i = 0; (i < 8 && blogicbuf[0] != 1); i++)
			blogicbuf[0] >>= 1;
		blogic_blkp->bb_dmachan = (unchar)i;
		/*
		 * note user must set arbritration level (dmachan)
		 * as in the configuration file (6)
		 */
	}

/*	save and check interrupt vector number				*/
	if (blogicbuf[1] & CFG_INT_MASK) {
		for (i = 0; (i < 8 && blogicbuf[1] != 1); i++)
			blogicbuf[1] >>= 1;
		if (blogic_blkp->bb_boardtype != PCI_HBA)
			blogic_blkp->bb_intr = (unchar)(i + 9);

	} else
		return (DDI_FAILURE);

/*	send Inquire Extended Setup Info command */
	blogicbuf[0] = 15;
	if (blogic_docmd(blogic_blkp, CMD_XINQSETUP, blogicbuf, blogicbuf))
		return (DDI_FAILURE);

	/*
	* Check if we're talking to a wide board.
	*/
	if (blogicbuf[13] & 0x01)
		blogic_blkp->bb_flag |= WIDE_SCSI;

	if (blogic_blkp->bb_boardtype == PCI_HBA) {
		blogic_blkp->bb_dmalim_p = &blogic_32bit_dma_lim;

		/*
		* Disable backdoor ISA-compatible I/O port range
		* on this card otherwise, the card will be reported
		* twice to the driver.
		*/
		blogicbuf[0] = 0x6;
		(void) blogic_docmd(blogic_blkp, CMD_DISAB_ISA_PORT,
		    blogicbuf, NULL);
	} else {
		switch (blogicbuf[0]) {
		case 'A':
			blogic_blkp->bb_boardtype = ISA_HBA;
			blogic_blkp->bb_dmalim_p  = &blogic_24bit_dma_lim;
			break;
		case 'E':
			blogic_blkp->bb_boardtype = EISA_HBA;
			blogic_blkp->bb_dmalim_p  = &blogic_32bit_dma_lim;
			break;
		case 'M':
			blogic_blkp->bb_boardtype = MCA_HBA;
			blogic_blkp->bb_dmalim_p  = &blogic_32bit_dma_lim;
			break;
		default:
			blogic_blkp->bb_dmalim_p  = &blogic_24bit_dma_lim;
			break;
		}
	}

	if (blogic_blkp->bb_flag & WIDE_SCSI) {
		/*
		* Enable 64 LUN support.
		*/
		blogicbuf[0] = 1;
		if (blogic_docmd(blogic_blkp, CMD_ENAB_64_LUN,
		    blogicbuf, NULL)) {
			cmn_err(CE_WARN,
			"blogic_cfginit: cant enable extended tar/lun support");
			return (DDI_FAILURE);
		}
		blogic_blkp->bb_max_targs = BLOGIC_MAX_TARG_WIDE;
		blogic_blkp->bb_max_luns = BLOGIC_MAX_LUN_WIDE;
	} else {
		blogic_blkp->bb_max_targs = BLOGIC_MAX_TARG_NARROW;
		blogic_blkp->bb_max_luns = BLOGIC_MAX_LUN_NARROW;
	}

	/*
	* Turn on Command tag queueing for FW 4.20 and above
	* for C boards, and F/W 3.30 and above
	* for "S" boards.
	*/
	if ((blogic_blkp->bb_fwrev > 4) ||
	    ((blogic_blkp->bb_fwrev == 4) && (blogic_blkp->bb_fwver >= 2)) ||
	    ((blogic_blkp->bb_fwrev == 3) && (blogic_blkp->bb_fwver >= 3))) {
		if (!(blogic_blkp->bb_flag & TAG_Q_OFF))
			blogic_blkp->bb_flag |= TAG_Q_SUPPORTED;
	}

	/* use variable scheme */
	if (inb(blogic_blkp->bb_base_ioaddr + 3) & 0x80)
		blogic_blkp->bb_flag |= VAR_XLAT_SCHEME;

/*
 * 	set up Bus Master DMA
 */
	if (blogic_blkp->bb_boardtype == ISA_HBA) {
		if ((ddi_dmae_1stparty(blogic_blkp->bb_dip,
			blogic_blkp->bb_dmachan))
			!= DDI_SUCCESS)
			return (DDI_FAILURE);

		blogicbuf[0] = blogic_blkp->bb_dmaspeed;
		(void) blogic_docmd(blogic_blkp, CMD_XFERSPEED,
			blogicbuf, NULL);

		blogicbuf[0] = blogic_blkp->bb_buson;
		(void) blogic_docmd(blogic_blkp, CMD_BONTIME, blogicbuf, NULL);
		blogicbuf[0] = blogic_blkp->bb_busoff;
		(void) blogic_docmd(blogic_blkp, CMD_BOFFTIME, blogicbuf, NULL);
	}

	return (DDI_SUCCESS);
}

static void
blogic_mbox_init(register struct blogic_blk *blogic_blkp)
{
	register int	num_mbox;
#pragma pack(1)
	struct blogic_mbox_init {
		unchar	mbxi_num_mbox;
		paddr_t	mbxi_paddr;
	};
#pragma pack()
	struct blogic_mbox_init	*mboxi;
	int n;

	num_mbox = blogic_blkp->bb_num_mbox;

	blogic_blkp->bb_FirstMboxIn  = blogic_blkp->bb_FirstMboxOut + num_mbox;
	blogic_blkp->bb_CurrMboxIn = blogic_blkp->bb_FirstMboxIn;
	blogic_blkp->bb_LastMboxIn = blogic_blkp->bb_FirstMboxIn
		+ (num_mbox - 1);
	blogic_blkp->bb_CurrMboxOut = blogic_blkp->bb_FirstMboxOut;
	blogic_blkp->bb_LastMboxOut = blogic_blkp->bb_FirstMboxOut
		+ (num_mbox - 1);

	mboxi = (struct blogic_mbox_init *) blogic_blkp->bb_buf;
	mboxi->mbxi_num_mbox = (unchar) num_mbox;
	mboxi->mbxi_paddr = BLOGIC_KVTOP(blogic_blkp->bb_FirstMboxOut);
	if (blogic_docmd(blogic_blkp, CMD_XMBXINIT, (unchar *) mboxi, NULL)) {
		cmn_err(CE_WARN,
		    "blogic_mbox_init: CMD_XMBXINIT command failed.");
		return;
	}

	/*
	* Clear mailboxes.
	*/
	n = 2 * num_mbox * sizeof (struct mbox_entry);
	bzero((caddr_t) blogic_blkp->bb_FirstMboxOut, n);
}

static void
blogic_rndrbn_init(register struct blogic_blk *blogic_blkp)
{
	unchar *blogicbuf = blogic_blkp->bb_buf;

	if ((blogic_blkp->bb_fwrev > 3) ||
	    ((blogic_blkp->bb_fwrev == 3) && (blogic_blkp->bb_fwver >= 3))) {
		/*
		* Implement strict round robin for performance.
		*/
		blogicbuf[0] = 1;	/* Enable Strict Round Robin */
		if (blogic_docmd(blogic_blkp, CMD_XSTRICT_RND_RBN,
		    blogicbuf, NULL) != 0) {
			cmn_err(CE_WARN,
			"blogic_cfginit: Init err CMD_XSTRICT_RND_RBN fail");
			return;
		}
	}
}

static int
blogic_docmd(struct blogic_blk *blogic_blkp, int opcode, unchar *sbp,
    unchar *rbp)
{
	register struct blogic_cmd *cp;
	register int i;

	if (opcode & CMD_XMASK)
		cp = &blogic_xcmds[opcode - 0x80];
	else
		cp = &blogic_cmds[opcode];

	if (cp->ac_flags & ACF_INVDCMD) {
#ifdef BLOGIC_DEBUG
		PRF("blogic_docmd: ACF_INVCMD\n");
#endif
		return (1);	/* invalid command */
	}
	if (cp->ac_flags & ACF_WAITIDLE) { /* wait for the adapter to go idle */
		if (blogic_wait(blogic_blkp->bb_stat_ioaddr,
			STAT_IDLE, STAT_IDLE, 0))
			cmn_err(CE_WARN, "blogic_docmd: adapter won't go IDLE");
	}

	if (blogic_wait(blogic_blkp->bb_stat_ioaddr, STAT_CDF, 0, STAT_CDF))
		cmn_err(CE_WARN, "blogic_docmd: CDF won't go off for command");
	outb(blogic_blkp->bb_datacmd_ioaddr, opcode);

/* 	If any output data, write it out */
	for (i = cp->ac_args; i > 0; i--, sbp++) {
		/* wait for STAT_CDF to turn off */
		if (blogic_wait(blogic_blkp->bb_stat_ioaddr,
			STAT_CDF, 0, STAT_CDF))
			cmn_err(CE_WARN,
				"blogic_docmd: CDF won't go off for data");
		outb(blogic_blkp->bb_datacmd_ioaddr, *sbp);
		if (inb(blogic_blkp->bb_stat_ioaddr) & STAT_INVDCMD) {
#ifdef BLOGIC_DEBUG
			PRF("blogic_docmd: STAT_INVDCMD\n");
#endif
			return (1);
		}
	}

/* 	if any return data, get it */
	for (i = cp->ac_vals; i > 0; i--, rbp++) {
		/* wait for STAT_DF to turn on */
		if (blogic_wait(blogic_blkp->bb_stat_ioaddr, STAT_DF,
			STAT_DF, 0))
			cmn_err(CE_WARN,
				"blogic_docmd: DF won't go on for value");
		*rbp = inb(blogic_blkp->bb_datacmd_ioaddr);
	}

/* 	Wait for completion if necessary */
	if ((cp->ac_flags & ACF_WAITCPLTE) == 0)
		return (0);
	if (blogic_wait(blogic_blkp->bb_intr_ioaddr, INT_HACC, INT_HACC, 0))
		cmn_err(CE_WARN,
		"blogic_docmd: adapter won't indicate COMPLETE");

/* 	Reset the interrupts */
	outb(blogic_blkp->bb_ctrl_ioaddr, CTL_IRST);

/* 	Check for error */
	if (inb(blogic_blkp->bb_stat_ioaddr) & STAT_INVDCMD) {
#ifdef BLOGIC_DEBUG
		PRF("blogic_docmd: STAT_INVDCMD\n");
#endif
		return (1);
	}

	return (0);
}

/*
 * blogic_wait --  wait for a register of a controller to achieve a
 * specific state.  Arguments are a mask of bits we care about,
 * and two sub-masks.  To return normally, all the bits in the
 * first sub-mask must be ON, all the bits in the second sub-
 * mask must be OFF.  If 5 seconds pass without the controller
 * achieving the desired bit configuration, we return 1, else
 * 0.
 */
static int
blogic_wait(register ushort port, ushort mask, ushort onbits, ushort offbits)
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

/*
* blogic_poll()
*
* Description
* Poll controller until an incoming mailbox is ready.
*/
static int
blogic_poll(register struct blogic_blk *blogic_blkp)
{
	register ushort	ioaddr;

	ioaddr = blogic_blkp->bb_base_ioaddr;

	/*
	* Wait for INT_MBIF
	*/
	if (blogic_wait(ioaddr+BLOGICINTFLGS, INT_MBIF, INT_MBIF, 0)) {
#ifdef BLOGIC_DEBUG
		PRF("blogic_poll: command failed with no ack.\n");
		PRF("stat= 0x%x intr= 0x%x\n", inb(ioaddr+BLOGICSTAT),
		    inb(ioaddr+BLOGICINTFLGS));
#endif
		return (1);
	}

	outb(ioaddr + BLOGICCTL, CTL_IRST);
	return (0);
}

/*
* blogic_outmbx()
*
* Description
* Place the ccb in the mailbox queue and poke the host
*	adapter to retrieve the request.  Returns 0 on success,
*	1 on failure.  Must acquire mutex before calling.
*/
static int
blogic_outmbx(register struct blogic_blk *blogic_blkp,
    struct blogic_ccb *ccbp, unchar cmd)
{
	register struct mbox_entry *mbep;

	mbep = blogic_blkp->bb_CurrMboxOut;

	if (mbep->mbx_cmdstat != MBX_FREE) {
		cmn_err(CE_CONT,
		    "blogic_outmbx: all outgoing mailboxes used\n");
		return (1);
	}

	if (++blogic_blkp->bb_CurrMboxOut > blogic_blkp->bb_LastMboxOut)
		blogic_blkp->bb_CurrMboxOut = blogic_blkp->bb_FirstMboxOut;

	mbep->mbx_ccb_addr = ccbp->ccb_paddr;
	mbep->mbx_cmdstat = cmd;

	/*
	* Make sure the host adapter is not busy.
	*/
	if (blogic_wait(blogic_blkp->bb_stat_ioaddr, STAT_CDF, 0, STAT_CDF)) {
		cmn_err(CE_WARN, "blogic_outmbx: adapter busy (%x)",
		    inl(blogic_blkp->bb_base_ioaddr));
		return (1);
	}

	ccbp->ccb_hastat	= 0;
	ccbp->ccb_tarstat	= 0;
	ccbp->ccb_starttime	= lbolt;

	ccbp->ccb_flag |= ACTIVE;
	blogic_blkp->bb_active_cnt++;

	outb(blogic_blkp->bb_datacmd_ioaddr, CMD_DOSCSI);

	return (0);
}

/*	Autovector Interrupt Entry Point				*/
/*ARGSUSED*/
static u_int
blogic_dummy_intr(caddr_t arg)
{
	return (DDI_INTR_UNCLAIMED);
}

static u_int
blogic_intr(caddr_t arg)
{
	register struct	blogic_blk	*blogic_blkp;
	unchar				intflgs;
	register struct mbox_entry	*mbep;
	register int			i;
	char				cmdstat;
	struct scsi_pkt			*pktp;
	struct blogic_ccb		*ccbp;

	blogic_blkp = BLOGIC_BLKP(arg);

	mutex_enter(&blogic_blkp->bb_mutex);

	/*
	* Check if any interrupt is pending
	*/
	intflgs = inb(blogic_blkp->bb_intr_ioaddr);
	if (!(intflgs & INT_ANY)) {
		/*
		* Nope.  Notify the calling routine that
		* we did not claim the interrupt.
		*/
		mutex_exit(&blogic_blkp->bb_mutex);
		return (DDI_INTR_UNCLAIMED);
	}

	/*
	* Clear interrupt status before processing any
	* mailbox so that if we get another mailbox
	* right before we exit, we're still guaranteed
	* to get our doorbell rang again.
	*/
	outb(blogic_blkp->bb_ctrl_ioaddr, CTL_IRST);

	/*
	* Check if we have a SCSI Reset condition.
	*/
	if (intflgs & INT_SCRD) {
		blogic_reset_ctlr(blogic_blkp);
		mutex_exit(&blogic_blkp->bb_mutex);
		return (DDI_INTR_CLAIMED);
	}

	for (i = 0; i < blogic_blkp->bb_num_mbox; i++) {
		mbep = blogic_blkp->bb_CurrMboxIn;

		/*
		* Check if we need to wrap around the
		* end of the round-robin mailbox queue.
		*/
		if (mbep > blogic_blkp->bb_LastMboxIn)
			mbep = blogic_blkp->bb_FirstMboxIn;

		/*
		* If there is no more serviced mailbox
		* requests queued up, we're done.
		*/
		if ((cmdstat = mbep->mbx_cmdstat) == MBX_FREE)
			break;

		if ((ccbp = (struct blogic_ccb *)
		    BLOGIC_CCB_PHYSTOKV(blogic_blkp, mbep->mbx_ccb_addr))
			== (struct blogic_ccb *)0) {
			cmn_err(CE_WARN,
			    "blogic_intr: spurious intr (ccb=NULL)");
			break;
		}

		switch (cmdstat) {
		case MBX_STAT_ABORT:
			/*
			* ccb successfully aborted by host.
			* If abort was sent because the ccb was
			* "stuck", set ccb_hastat to force retry.
			*/
			if (ccbp->ccb_flag & TIMEDOUT) {
				ccbp->ccb_flag &= ~TIMEDOUT;
				ccbp->ccb_hastat = HS_UNKNOWN_ERR;
				cmdstat = MBX_STAT_ERROR;
			}
			break;

		case MBX_STAT_CCBNF:
			mbep->mbx_cmdstat = MBX_FREE;
			blogic_blkp->bb_CurrMboxIn = ++mbep;
			if (ccbp->ccb_flag & ACTIVE) {
				ccbp->ccb_flag &= ~TIMEDOUT;
				blogic_freeccb(blogic_blkp, ccbp);
			}
			continue;
		}

		/*
		* Set this mailbox free!
		*/
		mbep->mbx_cmdstat = MBX_FREE;

		blogic_blkp->bb_CurrMboxIn = ++mbep;

		/*
		* Check for any errors.
		* Save the pkt to send back to the target driver
		* once we have finished parsing the mailbox list.
		*/
		if (!(pktp = blogic_chkerr(blogic_blkp, ccbp, cmdstat)))
			continue;

#ifdef BLOGIC_DEBUG
		blogic_from[blogic_from_idx].idx = blogic_from_idx;
		blogic_from[blogic_from_idx].ccb = (caddr_t) ccbp;
		blogic_from[blogic_from_idx].pktp = (caddr_t) pktp;
		blogic_from[blogic_from_idx].time = lbolt;
		blogic_to[ccbp->ccb_dbg_idx].active = 0;
		blogic_from_idx++;
#endif

		mutex_exit(&blogic_blkp->bb_mutex);
		scsi_run_cbthread(blogic_blkp->bb_cbthdl,
		    (struct scsi_cmd *) pktp);
		mutex_enter(&blogic_blkp->bb_mutex);
	}

	mutex_exit(&blogic_blkp->bb_mutex);

	return (DDI_INTR_CLAIMED);
}

static void
blogic_pollret(struct blogic_blk *blogic_blkp, struct scsi_pkt *poll_pktp)
{
	register struct mbox_entry	*mbep;
	register int			i;
	char				cmdstat;
	struct blogic_ccb		*ccbp;
	register struct scsi_pkt	*pktp;
	register struct scsi_cmd	*cmd;
	struct scsi_cmd			*cmd_hdp = (struct scsi_cmd *) NULL;

	for (;;) {
		if (blogic_poll(blogic_blkp)) {
			poll_pktp->pkt_reason = CMD_INCOMPLETE;
			poll_pktp->pkt_state = 0;
			(void) blogic_abort_cmd(&poll_pktp->pkt_address,
				poll_pktp);
			break;
		}

		for (i = 0; i < blogic_blkp->bb_num_mbox; i++) {
			mbep = blogic_blkp->bb_CurrMboxIn;

			/*
			* Check if we need to wrap around the
			* end of the round-robin mailbox queue.
			*/
			if (mbep > blogic_blkp->bb_LastMboxIn)
				mbep = blogic_blkp->bb_FirstMboxIn;

			/*
			* If there is no more serviced mailbox
			* requests queued up, we're done.
			*/
			if ((cmdstat = mbep->mbx_cmdstat) == MBX_FREE)
				break;

			if ((ccbp = (struct blogic_ccb *)
			    BLOGIC_CCB_PHYSTOKV(blogic_blkp,
			    mbep->mbx_ccb_addr)) ==
				(struct blogic_ccb *)0) {
				cmn_err(CE_WARN,
				    "blogic_intr: spurious intr (ccb=NULL)");
				break;
			}

			switch (cmdstat) {
			case MBX_STAT_ABORT:
				/*
				* ccb successfully aborted by host.
				* If abort was sent because the ccb was
				* "stuck", set ccb_hastat to force retry.
				*/
				if (ccbp->ccb_flag & TIMEDOUT) {
					ccbp->ccb_flag &= ~TIMEDOUT;
					ccbp->ccb_hastat = HS_UNKNOWN_ERR;
					cmdstat = MBX_STAT_ERROR;
				}
				break;

			case MBX_STAT_CCBNF:
				mbep->mbx_cmdstat = MBX_FREE;
				blogic_blkp->bb_CurrMboxIn = ++mbep;
				if (ccbp->ccb_flag & ACTIVE) {
					ccbp->ccb_flag &= ~TIMEDOUT;
					blogic_freeccb(blogic_blkp, ccbp);
				}
				continue;
			}

			/*
			* Set this mailbox free!
			*/
			mbep->mbx_cmdstat = MBX_FREE;

			blogic_blkp->bb_CurrMboxIn = ++mbep;

			/*
			* Check for any errors.
			* Save the pkt to send back to the target driver
			* once we have finished parsing the mailbox list.
			*/
			if (!(pktp =
			    blogic_chkerr(blogic_blkp, ccbp, cmdstat)))
				continue;

#ifdef BLOGIC_DEBUG
			blogic_from[blogic_from_idx].idx = blogic_from_idx;
			blogic_from[blogic_from_idx].ccb = (caddr_t) ccbp;
			blogic_from[blogic_from_idx].pktp = (caddr_t) pktp;
			blogic_from[blogic_from_idx].time = lbolt;
			blogic_from_idx++;
#endif

			if (pktp == poll_pktp)
				goto out_of_loop;

			((struct scsi_cmd *) pktp)->cmd_cblinkp = NULL;

			if (!cmd_hdp)
				cmd_hdp = (struct scsi_cmd *) pktp;
			else {
				for (cmd = cmd_hdp; cmd->cmd_cblinkp;
					cmd = cmd->cmd_cblinkp)
					;

				cmd->cmd_cblinkp = (struct scsi_cmd *)pktp;
			}
		}
	}

out_of_loop:

	if (cmd_hdp) {
		for (; (cmd = cmd_hdp) != NULL; ) {
			cmd_hdp = cmd->cmd_cblinkp;
			mutex_exit(&blogic_blkp->bb_mutex);
			scsi_run_cbthread(blogic_blkp->bb_cbthdl, cmd);
			mutex_enter(&blogic_blkp->bb_mutex);
		}
	}
}

/*
 *	check any possible error from the returned packet
 */
static struct scsi_pkt *
blogic_chkerr(register struct blogic_blk *blogic_blkp,
    register struct blogic_ccb *ccbp, char cmdstat)
{
	register struct	scsi_pkt *pktp;

	ccbp->ccb_flag &= ~ACTIVE;
	blogic_blkp->bb_active_cnt--;

	if ((pktp = (struct scsi_pkt *)ccbp->ccb_ownerp) ==
		(struct scsi_pkt *)0) {
		/*
		* We shouldn't get here, but in case we do,
		* just return NULL.
		*/
		return ((struct scsi_pkt *)0);
	}

/*	clear request sense cache flag					*/
	switch (cmdstat) {
	case MBX_STAT_DONE:
		pktp->pkt_reason = CMD_CMPLT;
		*pktp->pkt_scbp = STATUS_GOOD;
		pktp->pkt_resid = 0;
		pktp->pkt_state =
			(STATE_XFERRED_DATA|STATE_GOT_BUS|
			STATE_GOT_TARGET|STATE_SENT_CMD|STATE_GOT_STATUS);
		break;

	case MBX_STAT_ERROR:
		*pktp->pkt_scbp = ccbp->ccb_tarstat;
		if (ccbp->ccb_hastat == HS_OK) {
			pktp->pkt_reason = CMD_CMPLT;
			if (ccbp->ccb_tarstat == STATUS_CHECK)
				blogic_seterr(pktp, ccbp);
		} else if (ccbp->ccb_hastat == HS_SELTO) {
			pktp->pkt_reason = CMD_TIMEOUT;
			pktp->pkt_statistics |= STAT_TIMEOUT;
		} else if (ccbp->ccb_hastat == HS_DATARUN) {
			blogic_run_err(pktp, ccbp);
		} else if (ccbp->ccb_hastat == HS_BADFREE) {
			pktp->pkt_reason = CMD_UNX_BUS_FREE;
		} else if (
		    !(blogic_blkp->bb_dev_type[ccbp->ccb_tf_tid]
			& TAPE_DEVICE) &&
		    (blogic_blkp->bb_flag & INSTALLED) &&
		    (ccbp->ccb_hastat || (ccbp->ccb_tarstat == STATUS_QFULL)) &&
		    (ccbp->ccb_retrycnt < BLOGIC_MAX_RETRY)) {
			ccbp->ccb_retrycnt++;
			if (blogic_outmbx(blogic_blkp, ccbp, MBX_CMD_START)) {
				/*
				* We'll get here if the adapter is hung
				* up for some reason.  We can try to hard
				* reset the adapter to bring it back to
				* life.
				*/
				blogic_blkp->bb_reset_cnt++;
				blogic_reset_ctlr(blogic_blkp);
			}
			return (NULL);
		} else {
			pktp->pkt_reason = CMD_INCOMPLETE;
		}

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

/*
 * Handle the cases of data overrun and data underrun
 * resid is kept in ccb_datalen by Buslogic
 */
static void
blogic_run_err(register struct scsi_pkt *pktp, register struct blogic_ccb *ccbp)
{

	if (ccbp->ccb_tarstat == STATUS_CHECK) {
/*		underrun						*/
		if (ccbp->ccb_datalen != 0) {
			pktp->pkt_resid  = ccbp->ccb_datalen;
/*			note that pkt_reason set to CMD_CMPLT per Frits */
			blogic_seterr(pktp, ccbp);
		} else {
			blogic_seterr(pktp, ccbp);
			pktp->pkt_reason = CMD_DATA_OVR;
		}
	} else {
		pktp->pkt_state =
			(STATE_XFERRED_DATA|STATE_GOT_BUS|
			STATE_GOT_TARGET|STATE_SENT_CMD);
/*		handle overrun case with no target status		*/
		if (ccbp->ccb_datalen == 0) {
			pktp->pkt_reason = CMD_DATA_OVR;
		} else {
			pktp->pkt_reason = CMD_CMPLT;
			pktp->pkt_resid  = ccbp->ccb_datalen;
		}
	}
}

/*ARGSUSED*/
static void
blogic_seterr(register struct scsi_pkt *pktp, register struct blogic_ccb *ccbp)
{
	register struct	 scsi_arq_status *arqp;

	pktp->pkt_reason = CMD_CMPLT;
	pktp->pkt_state  = (STATE_GOT_BUS|STATE_GOT_TARGET|
		STATE_SENT_CMD|STATE_GOT_STATUS);
	if (!(PKT2BLOGICUNITP(pktp))->au_arq)
		return;

	pktp->pkt_state  |= STATE_ARQ_DONE;
	arqp = (struct scsi_arq_status *)pktp->pkt_scbp;
	arqp->sts_rqpkt_reason = CMD_CMPLT;

	arqp->sts_rqpkt_resid  = sizeof (struct scsi_extended_sense) -
		BLOGIC_SENSE_LEN;
	arqp->sts_rqpkt_state |= STATE_XFERRED_DATA;
}

/*
* blogic_reset_ctlr()
*
* Description
*	Perform hard reset on the controller and resend any
*	outstanding requests at the time of the reset.
*/
static void
blogic_reset_ctlr(struct blogic_blk *blogic_blkp)
{
	register struct	blogic_ccb	*ccbp;
	register struct mbox_entry	*mbep;
	int				req_resent = 0;
	int				i;
	unchar *blogicbuf = blogic_blkp->bb_buf;

	/*
	* Wait a little bit before doing hard reset.
	*/
	drv_usecwait(SEC_INUSEC);

	/*
	* Reset the host adapter and SCSI bus.
	*/
	outb(blogic_blkp->bb_ctrl_ioaddr, CTL_HRST);

	/*
	* Give the F/W enough time to recover and set up the
	* Status register.
	*/
	drv_usecwait(3 * SEC_INUSEC);

	/*
	* Wait until adapter is ready.
	*/
	if (blogic_wait(blogic_blkp->bb_stat_ioaddr, STAT_MASK,
	    (STAT_INIT | STAT_IDLE),
	    (STAT_STST | STAT_CDF | STAT_DF | STAT_INVDCMD))) {
		cmn_err(CE_WARN,
		"blogic_reset_ctlr: ctlr not ready after hard reset.");
		return;
	}

	/*
	* Need to send test unit ready commands to the
	* devices to clear bus reset condition.  The
	* host adapter command CMD_INSTDEV will do this.
	*/
	if (blogic_docmd(blogic_blkp, CMD_INSTDEV, NULL, blogicbuf)) {
		cmn_err(CE_WARN,
		    "blogic_reset_ctlr: CMD_INSTDEV command failed.");
		return;
	}

	/*
	* Inform host adapter of where we have allocated
	* the mailboxes.
	*/
	blogic_mbox_init(blogic_blkp);

	/*
	* Set up for strict round-robin mode
	* if controller supports it.
	*/
	blogic_rndrbn_init(blogic_blkp);

#ifdef BLOGIC_DEBUG
	bzero((caddr_t) blogic_to, sizeof (blogic_to));
	bzero((caddr_t) blogic_from, sizeof (blogic_from));
	blogic_to_idx = 0;
	blogic_from_idx = 0;
#endif

	/*
	* Flag tape devices so that next request will fail.
	* This is to make sure we don't overwrite the beginning
	* of the tape if a rewind caused by a reset has just
	* occurred.
	*/
	for (i = 0; i < BLOGIC_MAX_TARG_WIDE; i++) {
		if (blogic_blkp->bb_dev_type[i] & TAPE_DEVICE)
			blogic_blkp->bb_dev_type[i] |= RESET_CONDITION;
	}

	/*
	* Resend outstanding I/O requests.
	*/
	for (i = 0, ccbp = (struct blogic_ccb *) blogic_blkp->bb_ccblist;
	    i < blogic_blkp->bb_num_mbox; i++, ccbp++) {
		ccbp->ccb_retrycnt = 0;
		ccbp->ccb_abortcnt = 0;

		if (!(ccbp->ccb_flag & ACTIVE))
			continue;

		/*
		* Don't resend requests to sequential devices.  Just
		* return reset condition.
		*/
		if (blogic_blkp->bb_dev_type[ccbp->ccb_tf_tid] & TAPE_DEVICE) {
			struct scsi_pkt			 *pktp;

			ccbp->ccb_flag &= ~ACTIVE;
			blogic_blkp->bb_active_cnt--;

			if ((pktp = (struct scsi_pkt *)ccbp->ccb_ownerp) ==
				(struct scsi_pkt *)0)
				continue;

			pktp->pkt_reason = CMD_RESET;
			pktp->pkt_statistics |= STAT_BUS_RESET;

			/*
			* Clear reset condition for this device.
			* We do this here since there was a tape
			* request pending.  If no tape request
			* had been pending, we clear reset condition
			* in blogic_transport().
			*/
			blogic_blkp->bb_dev_type[ccbp->ccb_tf_tid] &=
			    ~RESET_CONDITION;

			mutex_exit(&blogic_blkp->bb_mutex);
			scsi_run_cbthread(blogic_blkp->bb_cbthdl,
			    (struct scsi_cmd *) pktp);
			mutex_enter(&blogic_blkp->bb_mutex);

			continue;
		}

		ccbp->ccb_flag &= ~TIMEDOUT;
		ccbp->ccb_starttime = lbolt;

		/*
		* Get the current mailbox.
		*/
		mbep = blogic_blkp->bb_CurrMboxOut;

		/*
		* See if the current mailbox is free to use.
		*/
		if (mbep->mbx_cmdstat != MBX_FREE) {
			cmn_err(CE_WARN,
			"blogic_reset_ctlr: no free mbox for ccb out");
			return;
		}

		/*
		* Increment current outgoing mailbox ptr.
		*/
		if (++blogic_blkp->bb_CurrMboxOut >
		    blogic_blkp->bb_LastMboxOut) {
			blogic_blkp->bb_CurrMboxOut =
			    blogic_blkp->bb_FirstMboxOut;
		}
		mbep->mbx_ccb_addr = ccbp->ccb_paddr;
		mbep->mbx_cmdstat = MBX_CMD_START;

		req_resent++;
	}

	if (!req_resent)
		return;

	/*
	* Make sure the host adapter is not busy.
	*/
	if (blogic_wait(blogic_blkp->bb_stat_ioaddr, STAT_CDF, 0, STAT_CDF)) {
		cmn_err(CE_WARN, "blogic_reset_ctlr: adapter busy (%x)",
		    inl(blogic_blkp->bb_base_ioaddr));
		return;
	}

	outb(blogic_blkp->bb_datacmd_ioaddr, CMD_DOSCSI);
}

/*
* blogic_init_lists
*
* Descripition
*	Initialize the lists used by the blogic_blk structure.
*/
int
blogic_init_lists(register struct blogic_blk *blogic_blkp)
{
	int			i;
	uint			ccblist_size;
	uint			dmalist_size;
	struct blogic_ccb	*ccbp;

	/*
	* Allocate space for ccb's.
	*/
	ccblist_size = (sizeof (struct blogic_ccb) + sizeof (caddr_t)) *
	    blogic_blkp->bb_num_mbox;

	if (ddi_iopb_alloc(blogic_blkp->bb_dip, blogic_blkp->bb_dmalim_p,
	    ccblist_size, &blogic_blkp->bb_ccblist)) {
		return (DDI_FAILURE);
	}
	bzero(blogic_blkp->bb_ccblist, ccblist_size);

	blogic_blkp->bb_pccblist = BLOGIC_KVTOP(blogic_blkp->bb_ccblist);

	/*
	* Allocate space for scatter-gather lists.
	*/
	dmalist_size = sizeof (struct blogic_dma_seg) * BLOGIC_MAX_DMA_SEGS *
	    blogic_blkp->bb_num_mbox;

	if (ddi_iopb_alloc(blogic_blkp->bb_dip, blogic_blkp->bb_dmalim_p,
	    dmalist_size, &blogic_blkp->bb_dmalist)) {
		ddi_iopb_free(blogic_blkp->bb_ccblist);
		return (DDI_FAILURE);
	}
	bzero(blogic_blkp->bb_dmalist, dmalist_size);

	blogic_blkp->bb_ccb_freelist = NULL;
	for (i = 0, ccbp = (struct blogic_ccb *) blogic_blkp->bb_ccblist;
	    i < blogic_blkp->bb_num_mbox; i++, ccbp++) {
		ccbp->ccb_sensep = BLOGIC_KVTOP(&ccbp->ccb_sense.sts_sensedata);
		ccbp->ccb_paddr = BLOGIC_CCB_KVTOPHYS(blogic_blkp, ccbp);

		ccbp->ccb_sg_list = (struct blogic_dma_seg *)
		    (blogic_blkp->bb_dmalist +
		    ((sizeof (struct blogic_dma_seg) *
		    BLOGIC_MAX_DMA_SEGS) * i));

		ccbp->ccb_forw = blogic_blkp->bb_ccb_freelist;
		ccbp->ccb_end = 0xFF;
		blogic_blkp->bb_ccb_freelist = ccbp;
	}

	return (DDI_SUCCESS);
}

/*
* blogic_getccb
*
* Description
*	Gets a ccb from the freelist.  Returns NULL if no more
*	free ccb's are available.  Must be called with mutex
*	held.
*/
struct blogic_ccb *
blogic_getccb(register struct blogic_blk *blogic_blkp)
{
	register struct blogic_ccb	*ccbp;

	if ((ccbp = blogic_blkp->bb_ccb_freelist) ==
		(struct blogic_ccb *)0)
		return (NULL);

	blogic_blkp->bb_ccb_freelist = ccbp->ccb_forw;
	ccbp->ccb_forw = NULL;
	return (ccbp);
}

/*
* blogic_freeccb
*
* Description
*	Frees ccb by putting it at the head of the freelist.
*	Must be called with mutex held.
*/
void
blogic_freeccb(register struct blogic_blk *blogic_blkp,
    register struct blogic_ccb *ccbp)
{
	ccbp->ccb_forw = blogic_blkp->bb_ccb_freelist;
	ccbp->ccb_ownerp = NULL;
	ccbp->ccb_flag = 0;
	blogic_blkp->bb_ccb_freelist = ccbp;
}

/*
* blogic_timer()
*
* Description
*	This is the watchdog timer that moniters the status of the
*	controller.  If ccb's are stuck on a controller and a
*	specified number of aborts and retries have not worked,
*	and a specified number of hard resets to the controller
*	have no effect, disable the controller.
*/
void
blogic_timer(caddr_t arg)
{
	register struct blogic_blk *blogic_blkp;
	register struct blogic_ccb *ccbp;
	timer_t		time_elapsed;
	int		i;

	blogic_blkp = (struct blogic_blk *) arg;

	/*
	* Make sure the controller is installed.
	*/
	if (!(blogic_blkp->bb_flag & INSTALLED))
		return;

	if (!blogic_blkp->bb_active_cnt)
		goto restart_timer;

	mutex_enter(&blogic_blkp->bb_mutex);

	for (i = 0, ccbp = (struct blogic_ccb *) blogic_blkp->bb_ccblist;
	    i < blogic_blkp->bb_num_mbox; i++, ccbp++) {
		if (!(ccbp->ccb_flag & ACTIVE))
			continue;

		/*
		* Don't attempt to abort I/O requests on slow devices.
		* Certain SCSI commands can take longer than the
		* timeout period.
		*/
		if (blogic_blkp->bb_dev_type[ccbp->ccb_tf_tid] & SLOW_DEVICE)
			continue;

		/*
		* If the ccb has not reached its timeout
		* stage, continue to the next ccb.
		*/
		time_elapsed =
		    (ccbp->ccb_starttime + blogic_blkp->bb_timeout_ticks);

		if (lbolt < time_elapsed)
			continue;

		/*
		* If the command has not reached its abort
		* count limit, abort it.
		*/
		if (ccbp->ccb_abortcnt < blogic_blkp->bb_retry_max) {
			ccbp->ccb_abortcnt++;
			ccbp->ccb_flag |= TIMEDOUT;
			if (blogic_outmbx(blogic_blkp, ccbp, MBX_CMD_ABORT)) {
				/*
				* We'll get here if the adapter is hung
				* up for some reason.  We can try to hard
				* reset the adapter to bring it back to
				* life.
				*/
				blogic_blkp->bb_reset_cnt++;
				blogic_blkp->bb_reset_cnt++;
				blogic_reset_ctlr(blogic_blkp);
			}
			continue;
		} else if (blogic_blkp->bb_reset_cnt
			< blogic_blkp->bb_reset_max) {
			blogic_blkp->bb_reset_cnt++;
			blogic_reset_ctlr(blogic_blkp);
			break;
		} else {
			/*
			* We get here if the controller is no
			* longer responding.  Disable it.
			*/
			blogic_blkp->bb_flag &= ~INSTALLED;
			mutex_exit(&blogic_blkp->bb_mutex);
			blogic_flush_ccbs(blogic_blkp);
			mutex_enter(&blogic_blkp->bb_mutex);
			break;
		}
	}

	mutex_exit(&blogic_blkp->bb_mutex);

restart_timer:
	blogic_blkp->bb_timeout_id =
	    timeout(blogic_timer, (caddr_t) blogic_blkp,
	    blogic_blkp->bb_timeout_ticks);
}

/*
* blogic_flush_ccbs()
*
* Description
*	Flush ccbs that have not been serviced by the controller.
*/
void
blogic_flush_ccbs(struct blogic_blk *blogic_blkp)
{
	int				i;
	register struct blogic_ccb	*ccbp;
	register struct scsi_pkt	*pktp;

	mutex_enter(&blogic_blkp->bb_mutex);

	for (i = 0, ccbp = (struct blogic_ccb *) blogic_blkp->bb_ccblist;
	    i < blogic_blkp->bb_num_mbox; i++, ccbp++) {
		if (!(ccbp->ccb_flag & ACTIVE))
			continue;

		if ((pktp = (struct scsi_pkt *)ccbp->ccb_ownerp) ==
			(struct scsi_pkt *)0) {
			cmn_err(CE_WARN,
			"blogic_flush_ccbs: pkt=NULL (blkp=%x, ccbp=%x)",
			blogic_blkp, ccbp);
			continue;
		}

		blogic_freeccb(blogic_blkp, ccbp);

		pktp->pkt_reason = CMD_TIMEOUT;
		pktp->pkt_statistics |= STAT_TIMEOUT;

		mutex_exit(&blogic_blkp->bb_mutex);
		scsi_run_cbthread(blogic_blkp->bb_cbthdl,
		    (struct scsi_cmd *) pktp);
		mutex_enter(&blogic_blkp->bb_mutex);
	}
	mutex_exit(&blogic_blkp->bb_mutex);
}

/*
* blogic_sendback_reset()
*
* Description
*	Send reset status of I/O request back to target driver.
*/
void
blogic_sendback_reset(caddr_t arg)
{
	struct blogic_blk		*blogic_blkp;
	struct scsi_pkt			*pktp;
	struct blogic_ccb		*ccbp;

	pktp		= (struct scsi_pkt *) arg;
	blogic_blkp	= PKT2BLOGICBLKP(pktp);
	ccbp		= (struct blogic_ccb *)SCMD_PKTP(pktp)->cmd_private;

	untimeout(ccbp->ccb_timeout_id);

	mutex_enter(&blogic_blkp->bb_mutex);

	pktp->pkt_reason = CMD_RESET;
	pktp->pkt_statistics |= STAT_BUS_RESET;

	/*
	* Clear reset condition for this device.
	* We do this here if there was no tape
	* request pending at the time of a reset.
	*/
	blogic_blkp->bb_dev_type[ccbp->ccb_tf_tid] &= ~RESET_CONDITION;

	mutex_exit(&blogic_blkp->bb_mutex);

	scsi_run_cbthread(blogic_blkp->bb_cbthdl,
	    (struct scsi_cmd *) pktp);
}

#ifdef SCSI_SYS_DEBUG
void
blogic_dump_blogicblk(struct blogic_blk *p)
{
	PRF("numdev %d flag 0x%x targetid 0x%x bdid 0x%x intr 0x%x\n",
		p->bb_numdev & 0xff, p->bb_flag & 0xff,
		p->bb_targetid & 0xff, p->bb_bdid,
		p->bb_intr & 0xff);
	PRF("dmachan %d dmaspeed 0x%x buson 0x%x busoff 0x%x dmasiz 0x%x\n",
		p->bb_dmachan & 0xff, p->bb_dmaspeed & 0xff,
		p->bb_buson & 0xff, p->bb_busoff & 0xff, p->bb_dma_reqsize);
	PRF("dip 0x%x ioaddr 0x%x\n",
		p->bb_dip, p->bb_base_ioaddr);
}

char *blogic_err_strings[] = {
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
blogic_dump_ccb(struct blogic_ccb *p)
{
	int index;

	PRF("op code 0x%x targ %d cdblen 0x%x scsi_cmd 0x%x\n",
		p->ccb_op & 0xff, p->ccb_tf_tid & 0xff,
		p->ccb_cdblen & 0xff, p->ccb_ownerp);

	if (!(PKT2BLOGICUNITP((struct scsi_pkt *)p->ccb_ownerp))->au_arq)
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
		for (index = 0; index < BLOGIC_MAX_DMA_SEGS; index++) {
		if (!(p->ccb_sg_list[index].data_ptr))
				break;
			PRF(" a:0x%x l:0x%x",
			    p->ccb_sg_list[index].data_ptr,
			    p->ccb_sg_list[index].data_len);
		}
	} else {
		PRF(" Data ptr 0x%x data len 0x%x",
		    p->ccb_datap,
		    p->ccb_datalen);
	}

	index = p->ccb_hastat & 0xff;
	if (index < 0 || index > HS_BADSEG)
		index = HS_UNKNOWN_ERR;
	PRF("\nctlr stat %s \n", blogic_err_strings[index]);
}
#endif
