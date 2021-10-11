/*
 * Copyright (c) 1995-96, by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident	"@(#)dpt.c	1.82	96/08/29 SMI"

#include <sys/types.h>
#include <sys/scsi/scsi.h>
#include <sys/debug.h>
#include <sys/stat.h>
#include <sys/cpu.h>
#include <sys/pci.h>

#include "dptghd.h"
#include "dpt_eisa.h"
#include <sys/dktp/dpt/dpt.h>
#include <sys/dktp/dpt/dptioctl.h>
#include <sys/dktp/dpt/dptsig.h>

#ifdef PCI_DDI_EMULATION
#define	OLD_PCI
#else
#undef OLD_PCI
#endif

/*
 * External references
 */

static int dpt_tran_tgt_init(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
static int dpt_tran_tgt_probe(struct scsi_device *, int (*)());
static void dpt_tran_tgt_free(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);

static int dpt_transport(struct scsi_address *ap, struct scsi_pkt *pktp);
static int dpt_tran_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static int dpt_tran_reset(struct scsi_address *ap, int level);
static int dpt_capchk(char *cap, int tgtonly, int *cidxp);
static int dpt_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static int dpt_setcap(struct scsi_address *ap, char *cap, int value,
	int tgtonly);
static struct scsi_pkt *dpt_tran_init_pkt(struct scsi_address *ap,
	struct scsi_pkt *pkt, struct buf *bp, int cmdlen, int statuslen,
	int tgtlen, int flags, int (*callback)(), caddr_t arg);
static void dpt_tran_destroy_pkt(struct scsi_address *ap,
	struct scsi_pkt *pkt);
static void dpt_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt);
static void dpt_tran_dmafree(struct scsi_address *ap,
	struct scsi_pkt *pkt);
static void dpt_tran_sync_pkt(struct scsi_address *ap,
	struct scsi_pkt *pkt);
static int dpt_flush_cache(dev_info_t *dip, ddi_reset_cmd_t cmd);
extern void dpt_send_cmd(unsigned int port, paddr_t addr,
	unsigned char command);
static int dpt_open(dev_t *dev_p, int flag, int otyp, cred_t *cred_p);
static int dpt_close(dev_t dev, int flag, int otyp, cred_t *cred_p);
static int dpt_prop_op(dev_t dev, dev_info_t *dip, ddi_prop_op_t prop_op,
	int mod_flags, char *name, caddr_t valuep, int *lengthp);
static int dpt_ioctl(dev_t dev, int cmd, int arg, int flag, cred_t *cred_p,
	int *rval_p);

/*
 * Local Function Prototypes
 */
static int dpt_findhba(uint ioaddr);
static int dpt_propinit(struct dpt_blk *dpt_blkp);
static int dpt_cfginit(struct  dpt_blk *dpt_blkp);
static int dpt_wait(ushort port, ushort onbits, ushort offbits);
static u_int dpt_xlate_vec(struct dpt_blk *dpt_blkp);
static u_int dpt_intr(caddr_t arg);
static int dpt_send(ushort ioaddr, struct dpt_ccb *ccbp);
static int dpt_find_eisa_ctrl(uint ioaddr, dev_info_t *devi);
static int dpt_rdconf(int port, char *buf);
static void dpt_chkerr(struct dpt_blk *dpt_blkp, struct dpt_stat *sp,
		struct scsi_pkt *pktp, struct dpt_ccb *ccbp);
static int dpt_poll_disabled(struct dpt_blk *dpt_blkp, struct dpt_ccb *ccbp,
	int timeout);
static int dpt_wait_disabled(ushort port, struct dpt_stat *spp, int timeout);
static int dpt_enable_interrupts(ushort port);
static int dpt_disable_interrupts(ushort port);
static int dpt_search_pci(uint *ioaddr, dev_info_t *devi);
static int dpt_find_pci_ctrl(struct dpt_blk *dpt_blkp);
static int dpt_usr_cmd(int arg, int flag);
static void dpt_ioctl_intr(struct dpt_blk *dpt_blkp, struct dpt_ccb *ccbp);
static struct dpt_ccb * dpt_uccballoc(struct dpt_blk *dpt_blkp,
	struct dpt_ccb *uccbp, int flag, int *status);
static int dpt_udmaget(dev_info_t *dip, struct dpt_ccb *uccbp,
	struct dpt_ccb *ccbp, int flag);
static void dpt_udmafree();
static void dpt_uccbfree(struct dpt_ccb *ccbp);

static	int	dpt_start(void *hba_handle, gcmd_t *gcdmp);
static	void	dpt_process_intr(void *hba_handle, void *arg);
static	int	dpt_get_status(void *hba_handle, void *arg);
static	void	dpt_timeout_action(void *hba_handle, gcmd_t *gcmdp,
				   struct scsi_address *ap, gact_t action);
static	gcmd_t	*dpt_ccballoc(struct scsi_address *ap, struct scsi_pkt *pktp,
				void *bufp, int cmdlen, int statuslen,
				int tgtlen, int ccblen);
static	void	dpt_ccbfree(struct scsi_address *ap, struct scsi_pkt *pktp);

static	void	dpt_sg_func(struct scsi_pkt *pktp, gcmd_t *gcmdp,
				ddi_dma_cookie_t *dmackp, int single_segment,
				int seg_num, void *arg);


/*
 * Local static data
 */
static int dpt_pgsz = 0;
static int dpt_pgmsk;
static int dpt_pgshf;

static int dpt_cb_id = 0;
static kmutex_t dpt_global_mutex;
static kcondvar_t dpt_cv;
static int dpt_global_init = 0;
static ddi_dma_handle_t	dpt_usr_dmahandle = (void *)0;
static ddi_dma_win_t	dpt_usr_dmawin;
static ddi_dma_seg_t	dpt_usr_dmaseg;

static dpt_controllers_t dpt_controllers = {
	0,
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static ddi_dma_lim_t dpt_eisadma_lim = {
	0,		/* address low				*/
	0xffffffffU,	/* address high				*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version				*/
	0xffffffffU,	/* address register			*/
	0x003fffff,	/* counter register			*/
	512,		/* sector size				*/
	DPT_MAX_DMA_SEGS, /* scatter/gather list length		*/
	0xffffffffU	/* request size				*/
};

static ddi_dma_lim_t dpt_isadma_lim = {
	0,		/* address low				*/
	0x00ffffffU,	/* address high				*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version				*/
	0x00ffffffU,	/* address register			*/
	0x0000ffff,	/* counter register			*/
	512,		/* sector size				*/
	DPT_MAX_DMA_SEGS, /* scatter/gather list length		*/
	0xffffffffU	/* request size				*/
};


#ifdef DPT_DEBUG
#define		DENT	0x0001	/* Display function names on entry	*/
#define		DPKT	0x0002 	/* Display packet data 			*/
#define		DDATA	0x0004	/* Display all data			*/
#define		DINIT	0x0008	/* Display init state			*/
#define		DIOCTL	0x0010	/* Display ioctl flow			*/
#define		DTST	0x0020	/* Special debug			*/

int	dpt_debug = 0;

#endif

static struct {
	int	reason;
	u_long	state;
	u_long	statistics;
} dpt_errs[] = {

/* No Error                 */	{ CMD_CMPLT,0, 0 },
/* Selection Timeout        */	{ CMD_INCOMPLETE, STATE_GOT_BUS, 0 },
/* Command Timeout          */	{ CMD_TIMEOUT, STATE_GOT_BUS|STATE_GOT_TARGET,
							STAT_TIMEOUT },
/* Scsi Bus Reset           */	{ CMD_RESET, 0, STAT_ABORTED },
/* Ctrl in Powerup          */	{ CMD_RESET, 0, STAT_BUS_RESET },
/* Unexpected Bus Phase     */	{ CMD_TRAN_ERR, STATE_GOT_BUS, 0 },
/* Unexpected Bus Free      */	{ CMD_UNX_BUS_FREE, 0, 0 }, 
/* Bus Parity Error         */	{ CMD_TRAN_ERR, 0, 0 },
/* Scsi Bus Hung            */	{ CMD_TRAN_ERR, 0, 0 },
/* Unexpected Msg Reject    */	{ CMD_REJECT_FAIL,
					STATE_GOT_BUS|STATE_GOT_TARGET, 0 },
/* Reset Stuck              */	{ CMD_TRAN_ERR, 0, 0 },
/* Auto Request Sense Fail  */	{ CMD_INCOMPLETE, STATE_GOT_BUS, 0 },
/* Host Adaptor Parity Err  */	{ CMD_TRAN_ERR, 0, 0 },
/* Abort not Active         */	{ CMD_ABORTED,  0, STAT_ABORTED },
/* CP Aborted on Bus        */	{ CMD_ABORTED, STATE_GOT_BUS|STATE_GOT_TARGET,
							STAT_ABORTED },
/* CP Reset Not Active      */	{ CMD_RESET,    0, STAT_ABORTED },
/* CP Reset on Scsi Bus     */	{ CMD_RESET, STATE_GOT_BUS|STATE_GOT_TARGET,
							STAT_DEV_RESET },
/* Controller RAM ECC Error */	{ CMD_TRAN_ERR, 0, 0 },
/* PCI Parity Error         */	{ CMD_TRAN_ERR, 0, 0 },
/* PCI Rcvd Master Abort    */	{ CMD_TRAN_ERR, 0, 0 },
/* PCI Rcvd Target Abort    */	{ CMD_TRAN_ERR, 0, 0 },
/* PCI Sent Target Abort    */	{ CMD_TRAN_ERR, 0, 0 },
};

static int dpt_identify(dev_info_t *dev);
static int dpt_probe(dev_info_t *);
static int dpt_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int dpt_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

/*	cb_ops for dptctl pseudo driver		*/
static struct cb_ops dpt_cb_ops = {
	dpt_open, 		/* open */
	dpt_close, 		/* close */
	nodev, 			/* strategy */
	nodev, 			/* print */
	nodev, 			/* dump */
	nodev, 			/* read */
	nodev, 			/* write */
	dpt_ioctl, 		/* ioctl */
	nodev, 			/* devmap */
	nodev, 			/* mmap */
	nodev, 			/* segmap */
	nochpoll, 		/* poll */
	dpt_prop_op, 		/* cb_prop_op */
	0, 			/* streamtab  */
	D_64BIT | D_MP | D_NEW	/* Driver comaptibility flag */
};

struct dev_ops	dpt_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	dpt_identify,		/* identify */
	dpt_probe,		/* probe */
	dpt_attach,		/* attach */
	dpt_detach,		/* detach */
	dpt_flush_cache,	/* reset or flush the cache */
	&dpt_cb_ops,		/* pseudo driver ops */
	NULL			/* bus operations */
};

static dpt_sig_t dpt_signature = {
	'd', 'P', 't', 'S', 'i', 'G', SIG_VERSION,
	0, 0, FT_HBADRVR, 0, OEM_DPT, OS_SOLARIS,
	CAP_PASS | CAP_OVERLAP, DEV_ALL,
	ADF_2012A | ADF_PLUS_ISA | ADF_PLUS_EISA,
	0, 0, 0, 0, 0, 0, 0, 0,
	"DPT Solaris Unix Driver"
};

static ddi_dma_lim_t dpt_usr_dmalim = {
	0,		/* address low				*/
	0x00ffffff,	/* address high				*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version				*/
	0xffffffffU,	/* address register			*/
	0x003fffff,	/* counter register			*/
	512,		/* sector size				*/
	DPT_MAX_DMA_SEGS, /* scatter/gather list length		*/
	0x040000	 /* request size			*/
};

static u_int dpt_found = 0;

#ifdef OLD_PCI
char _depends_on[] = "misc/xpci misc/scsi";
#else
char _depends_on[] = "misc/scsi";
#endif

/*
 * Create a single CCB timeout list for all instances
 */
static	tmr_t	dpt_timer_conf;
static	long	dpt_watchdog_tick = 2*HZ; /* check timeouts every 2 sec. */

/*
 * This is the loadable module wrapper.
 */
#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"DPT SCSI Host Adapter Driver",	/* Name of the module. */
	&dpt_ops,	/* driver ops */
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

	mutex_init(&dpt_global_mutex, "DPT global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&dpt_global_mutex);
	}

	/*
	 * Initialize the per driver timer info
	 */
	dptghd_timer_init(&dpt_timer_conf, "DPT CCB timer", dpt_watchdog_tick);

	return (status);
}

int
_fini(void)
{
	int	status;

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&dpt_global_mutex);
	}

	dptghd_timer_fini(&dpt_timer_conf);

	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}



/*ARGSUSED*/
static int
dpt_tran_tgt_init(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	int 	targ;
	int	lun;
	struct 	dpt *hba_dptp;
	struct 	dpt *unit_dptp;

	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef DPT_DEBUG
	if (dpt_debug & DINIT)
		cmn_err(CE_CONT, "%s%d: %s%d <%d,%d>\n",
			ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
			ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
			targ, lun);
#endif
	hba_dptp = SDEV2HBA(sd);

	if (targ < 0 || targ > hba_dptp->d_blkp->db_max_target || lun < 0 ||
		lun > hba_dptp->d_blkp->db_max_lun) {
		cmn_err(CE_WARN, "%s%d: %s%d bad address <%d,%d>\n",
			ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
			ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
			targ, lun);
		return (DDI_FAILURE);
	}

	/* skip the HBA's assigned target number */
	if (targ == hba_dptp->d_blkp->db_targetid)
		return (DDI_FAILURE);

	if ((unit_dptp = kmem_zalloc(
			sizeof (struct dpt) + sizeof (struct dpt_unit),
			KM_NOSLEEP)) == NULL) {
		return (DDI_FAILURE);
	}

	*unit_dptp = *hba_dptp;
	unit_dptp->d_unitp = (struct dpt_unit *)(unit_dptp+1);

	unit_dptp->d_unitp->du_lim = *hba_dptp->d_blkp->db_limp;
	unit_dptp->d_unitp->du_lim.dlim_sgllen =
				hba_dptp->d_blkp->db_scatgath_siz;

	hba_tran->tran_tgt_private = unit_dptp;

#ifdef DPT_DEBUG
	if (dpt_debug & DINIT) {
		cmn_err(CE_WARN, "dpt_tran_tgt_init: <%d,%d>\n", targ, lun);
	}
#endif
	return (DDI_SUCCESS);
}



/*ARGSUSED*/
static int
dpt_tran_tgt_probe(
	struct scsi_device	*sd,
	int			(*callback)())
{
	int	rval;

	rval = scsi_hba_probe(sd, callback);

#ifdef DPT_DEBUG
	if (dpt_debug & DTST) {

		char		*s;
		struct dpt	*dpt = SDEV2DPTP(sd);

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
		cmn_err(CE_CONT, "dpt%d: %s target %d lun %d %s\n",
			ddi_get_instance(DPT_DIP(dpt)),
			ddi_get_name(sd->sd_dev),
			sd->sd_address.a_target,
			sd->sd_address.a_lun, s);
	}
#endif	/* DPT_DEBUG */

	return (rval);
}


/*ARGSUSED*/
static void
dpt_tran_tgt_free(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	struct dpt		*dpt;
	struct dpt		*unit_dptp;
#ifdef	DPT_DEBUG
	int			targ, lun;

	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

	if (dpt_debug & DINIT)
		cmn_err(CE_CONT, "dpt_tran_tgt_free: %s%d %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif	/* DPT_DEBUG */

/* BUG - should flush the cache here */

	unit_dptp = TRAN2DPTP(hba_tran);
	kmem_free(unit_dptp,
		sizeof (struct dpt) + sizeof (struct dpt_unit));
	return;
}


/*
 *	Autoconfiguration routines
 */
static int
dpt_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, "dpt") == 0)
		return (DDI_IDENTIFIED);
	else if (strcmp(dname, "pci1044,a400") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

static int
dpt_prop_op(dev_t dev, dev_info_t *dip, ddi_prop_op_t prop_op, int mod_flags,
    char *name, caddr_t valuep, int *lengthp)
{

#ifdef DPT_DEBUG
	if (dpt_debug & DINIT)
		cmn_err(CE_WARN, "dpt_prop_op: call\n");
#endif
	return (ddi_prop_op(dev, dip, prop_op, mod_flags, name, valuep,
		lengthp));
}

static int
dpt_probe(register dev_info_t *devi)
{
	unsigned int	ioaddr, bustype;
	int	len, status;

	status = DDI_FAILURE;
	len = sizeof (int);

	if (HBA_INTPROP(devi, "bustype", &bustype, &len) == DDI_SUCCESS) {
		if (HBA_INTPROP(devi, "ioaddr", &ioaddr, &len) != DDI_SUCCESS) {
			return (status);
		}
	} else {
		bustype = DPT_PCI_ADAPTER;
	}

#ifdef DPT_DEBUG
	if (dpt_debug & DINIT)
		cmn_err(CE_NOTE, "dpt_probe for HBA addr %x bustype %x",
			ioaddr, bustype);
#endif

	if (bustype == DPT_PCI_ADAPTER)
		status = dpt_search_pci(&ioaddr, devi);
	else if (bustype == DPT_EISA_ADAPTER)
		status =  dpt_find_eisa_ctrl(ioaddr, devi);
	else if (bustype == DPT_ISA_ADAPTER)
		status = dpt_findhba(ioaddr);

	if (status == DDI_SUCCESS) {
#ifdef DPT_DEBUG
		len = dpt_disable_interrupts(ioaddr);
		if (dpt_debug & DINIT)
			cmn_err(CE_WARN, "dpt_probe: disable ints ret %x", len);
#else
		(void) dpt_disable_interrupts(ioaddr);
#endif

	}

	return (status);
}

static int
dpt_search_pci(register uint *ioaddr, dev_info_t *devi)
{
	ulong base;
	struct 	ReadConfig cfp;
	ddi_acc_handle_t	cfg_handle;
	ushort_t	vendorid, deviceid;

	if (pci_config_setup(devi, &cfg_handle) != DDI_SUCCESS) {
#ifdef DPT_DEBUG
	if (dpt_debug & DINIT)
		cmn_err(CE_CONT, "dpt_search_pci: setup fail");
#endif
		return (DDI_PROBE_FAILURE);
	}

	vendorid = pci_config_getw(cfg_handle, PCI_CONF_VENID);
	deviceid = pci_config_getw(cfg_handle, PCI_CONF_DEVID);

	if (vendorid != PCI_DPT_VEND_ID || deviceid !=
		PCI_DPT_DEV_ID) {
		pci_config_teardown(&cfg_handle);
#ifdef DPT_DEBUG
	if (dpt_debug & DINIT)
		cmn_err(CE_CONT, "dpt_search_pci: fail for reg %x\n",
		*ioaddr);
#endif
		return (DDI_PROBE_FAILURE);
	}

	base = pci_config_getl(cfg_handle, PCI_CONF_BASE0);

/*	get the base address and and off the lower bit if it is set */
	base &= 0xfffffffe;

	pci_config_teardown(&cfg_handle);

#ifdef DPT_DEBUG
	if (dpt_debug & DINIT)
		cmn_err(CE_CONT,
			"dpt_search_pci: reg %x vend %x dev %x base%x\n",
			*ioaddr, vendorid, deviceid, base);
#endif


/* 	If the adapter is in EISA Forced Addr Mode, Move On */
	if ((inb(base) == 0x12) && (inb(base + 1) == 0x14))
		return (DDI_PROBE_FAILURE);

/* 	Add 16 to the base address to put us at the correct offset */

	base += 0x10;

/* 	If the read config fails, Move On */

	if ((dpt_rdconf((ushort)base, (caddr_t)&cfp) == DDI_FAILURE))
		return (DDI_PROBE_FAILURE);

/* 	If the forced address bit is set, move on */
	if (cfp.ForceAddr)
		return (DDI_PROBE_FAILURE);

/* 	We found the card					*/
	*ioaddr = base;
	mutex_enter(&dpt_global_mutex);
	dpt_found = TRUE;
	mutex_exit(&dpt_global_mutex);
	return (DDI_SUCCESS);
}

static int
dpt_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	register struct dpt 	*dpt;
	register struct	dpt_blk *dpt_blkp;
	int	i;

	switch (cmd) {
	case DDI_DETACH:
	{
		scsi_hba_tran_t	*tran;
		tran = (scsi_hba_tran_t *) ddi_get_driver_private(devi);
		if (!tran)
			return (DDI_SUCCESS);
		dpt = TRAN2HBA(tran);
		if (!dpt)
			return (DDI_SUCCESS);
		dpt_blkp = DPT_BLKP(dpt);

		dpt_flush_cache(devi, 0);

		/*
		 * Unconfigure GHD on this HBA instance
		 */
		dptghd_unregister(&dpt_blkp->db_ccc);

		mutex_destroy(&dpt_blkp->db_rmutex);

		mutex_enter(&dpt_global_mutex);
		dpt_global_init--;
		if (dpt_global_init == 0) {
			cv_destroy(&dpt_cv);
		}

		for (i = 0; i < DPT_MAX_CONTROLLERS; i++) {
			if (dpt_controllers.dc_addr[i] == dpt_blkp->db_ioaddr)
				break;
		}
		ASSERT(i < DPT_MAX_CONTROLLERS);
		dpt_controllers.dc_addr[i] = 0;

		mutex_exit(&dpt_global_mutex);

		scsi_hba_tran_free(dpt->d_tran);
		kmem_free((caddr_t)dpt, (sizeof (*dpt) + sizeof (*dpt_blkp)));

		ddi_prop_remove_all(devi);
		if (scsi_hba_detach(devi) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "dpt: scsi_hba_detach failed\n");
		}

		return (DDI_SUCCESS);

	}
	default:
		return (DDI_FAILURE);
	}
}

/*ARGSUSED*/
static int
dpt_open(dev_t *dev_p, int flag, int otyp, cred_t *cred_p)
{
	int status;

	mutex_enter(&dpt_global_mutex);

	if (dpt_found) {
		if (dpt_found & DPT_CTRL_HELD) {
			mutex_exit(&dpt_global_mutex);
			return (DDI_FAILURE);
		}
		dpt_found |= DPT_CTRL_HELD;
		status = DDI_SUCCESS;
#ifdef DPT_DEBUG
		if (dpt_debug & DINIT)
			cmn_err(CE_WARN, "dptctl open OK");
#endif
	} else {
		status = DDI_FAILURE;
#ifdef DPT_DEBUG
		if (dpt_debug & DINIT)
			cmn_err(CE_WARN, "dptctl open fail");
#endif
	}

	mutex_exit(&dpt_global_mutex);
	return (status);
}

/*ARGSUSED*/
static int
dpt_close(dev_t dev, int flag, int otyp, cred_t *cred_p)
{
	int status;

	mutex_enter(&dpt_global_mutex);

	if (!(dpt_found & DPT_CTRL_HELD)) {
#ifdef DPT_DEBUG
		if (dpt_debug & DINIT)
			cmn_err(CE_WARN, "dptctl close fail");
#endif
		status = DDI_FAILURE;
	} else {
		dpt_found &= ~DPT_CTRL_HELD;
		status = DDI_SUCCESS;
#ifdef DPT_DEBUG
		if (dpt_debug & DINIT)
			cmn_err(CE_WARN, "dptctl close OK");
#endif
	}

	mutex_exit(&dpt_global_mutex);
	return (status);
}

/*ARGSUSED*/
static int
dpt_ioctl(dev_t dev, int cmd, int arg, int flag, cred_t *cred_p, int *rval_p)
{
	int status = 0;

	switch (cmd) {

	case DPT_EATA_USR_CMD:
	{
		status = dpt_usr_cmd(arg, flag);

		break;
	}

	case DPT_GET_SIG:
	{
		if (ddi_copyout((caddr_t) &dpt_signature, (caddr_t) arg,
			sizeof (dpt_sig_t), flag))
			status = EFAULT;
		break;
	}

	case DPT_GET_CTLRS:
	{
		int i;
		dpt_get_ctlrs_t d;

		mutex_enter(&dpt_global_mutex);
			d.dc_number = (u_short) dpt_global_init;
			for (i = 0; i < DPT_MAX_CONTROLLERS; i++)
				d.dc_addr[i] = dpt_controllers.dc_addr[i];
		mutex_exit(&dpt_global_mutex);

		if (ddi_copyout((caddr_t) &d, (caddr_t) arg, sizeof (d), flag))
			status = EFAULT;
		break;
	}

	default:
		status =  ENXIO;

	}

	return (status);
}

/*ARGSUSED*/
static int
dpt_usr_cmd(int arg, int flag)
{
	/* ??? bug: the ioctl shouldn't use (struct dpt_ccb), dpt_ccb is ??? */
	/* ??? an internal driver data structure. Because of this, ??? */
	/* ??? the dtp_ccb structure can't easily be changed. ??? */
	struct dpt_ccb *uccbp = (struct dpt_ccb *) arg;
	register struct dpt_ccb *ccbp;
	register struct dpt_blk *dpt_blkp;
	int	i, status;

/* ??? bug: *uccbp is in User address space ??? */
	if (uccbp->ccb_ioaddr == (ushort)0) {
#ifdef DPT_DEBUG
		if (dpt_debug & DIOCTL)
			cmn_err(CE_WARN, "dpt_usr_cmd ioaddr 0");
#endif
		return (ENXIO);
	}
	mutex_enter(&dpt_global_mutex);
/*	find controller							*/
	for (i = 0; i < DPT_MAX_CONTROLLERS; i++) {
		if (dpt_controllers.dc_addr[i] == uccbp->ccb_ioaddr)
			break;
	}
	if (i >= DPT_MAX_CONTROLLERS) {
#ifdef DPT_DEBUG
		if (dpt_debug & DIOCTL)
			cmn_err(CE_WARN, "dpt_usr_cmd no board at %x",
			uccbp->ccb_ioaddr);
#endif
		mutex_exit(&dpt_global_mutex);
		return (ENXIO);
	}
	dpt_blkp = dpt_controllers.dc_blkp[i];

	ccbp = dpt_uccballoc(dpt_blkp, uccbp, flag, &status);
	if (ccbp == (struct dpt_ccb *)0) {
		mutex_exit(&dpt_global_mutex);
		return (status);
	}

/* ??? bug: *uccbp is still in User address space ??? */
	if (uccbp->ccb_optbyte & HA_DATA_IN ||
		uccbp->ccb_optbyte & HA_DATA_OUT) {
		status = dpt_udmaget(dpt_blkp->db_dip, uccbp, ccbp, flag);
		if (status) {
			dpt_uccbfree(ccbp);
			dpt_udmafree();
			mutex_exit(&dpt_global_mutex);
			return (status);
		}
	}

	mutex_enter(&dpt_blkp->db_ccc.ccc_hba_mutex);
	if (dpt_send(dpt_blkp->db_ioaddr, ccbp)) {
		dpt_uccbfree(ccbp);
		dpt_udmafree();
		mutex_exit(&dpt_blkp->db_ccc.ccc_hba_mutex);
		mutex_exit(&dpt_global_mutex);
		return (EIO);
	}
	mutex_exit(&dpt_blkp->db_ccc.ccc_hba_mutex);

/*	wait for the interrupt						*/
	cv_wait(&dpt_cv, &dpt_global_mutex);

/*	deallocate dma resources to flush data to usr space		*/
	dpt_udmafree();

	status = 0;

	if (ccbp->ccb_scsistat == STATUS_CHECK) {
		if (uccbp->ccb_optbyte & HA_AUTO_REQSEN &&
			ccbp->ccb_ctlrstat != DPT_REQSENFAIL) {
			if (ddi_copyout((caddr_t)
				&ccbp->ccb_sense.sts_sensedata,
				(caddr_t) uccbp->ccb_sensep,
				uccbp->ccb_senselen, flag))
				status = EFAULT;
		}
	}

/*	give user controller and target status bytes			*/
	if (ddi_copyout((caddr_t) &ccbp->ccb_ctlrstat,
		(caddr_t) &uccbp->ccb_ctlrstat,
		2, flag))
		status = EFAULT;

	dpt_uccbfree(ccbp);

	mutex_exit(&dpt_global_mutex);
	return (status);
}

/*
 * the global mutex is held
 * return 0 on success, else ioctl error
 */
/*ARGSUSED*/
static int
dpt_udmaget(dev_info_t *dip, register struct dpt_ccb *uccbp,
	register struct dpt_ccb *ccbp, int flag)
{
	int status, cnt;
	uint bytes;
	uint addr;
	int dma_flags;
	struct dpt_sg *dmap;
	ddi_dma_cookie_t dmack;
	ddi_dma_cookie_t *dmackp = &dmack;
	int	bxfer;
	off_t	offset, len;

	if (ccbp->ccb_optbyte & HA_DATA_OUT)
		dma_flags = DDI_DMA_WRITE | DDI_DMA_CONSISTENT|
			DDI_DMA_PARTIAL;
	else
		dma_flags = DDI_DMA_READ | DDI_DMA_CONSISTENT|
			DDI_DMA_PARTIAL;

/* ??? bug: *uccbp is in User Address space ??? */
/* ??? why are these ptr casts necessary ??? */
	bytes = (*(uint *)uccbp->ccb_datalen);
	addr = (*(uint *)uccbp->ccb_datap);

/* ??? bug: the buffer wasn't been locked into the address space ??? */
	status = ddi_dma_addr_setup(dip, (struct as *)0,
		(caddr_t) addr, bytes, dma_flags,
		DDI_DMA_DONTWAIT, 0, &dpt_usr_dmalim,
		&dpt_usr_dmahandle);

	if (status != DDI_SUCCESS) {
#ifdef DPT_DEBUG
		if (dpt_debug & DIOCTL)
			cmn_err(CE_WARN, "dpt_udmaget: addr_setup %x",
				status);
#endif
		switch (status) {
		case DDI_DMA_NORESOURCES:
			status = ENOMEM;
			break;
		case DDI_DMA_TOOBIG:
			status = EFBIG;
			break;
		case DDI_DMA_NOMAPPING:
		default:
			status = EFAULT;
			break;
		}
		return (status);
	}

	status = ddi_dma_nextwin(dpt_usr_dmahandle, NULL,
		&dpt_usr_dmawin);

	if (status != DDI_SUCCESS) {
#ifdef DPT_DEBUG
		if (dpt_debug & DIOCTL)
			cmn_err(CE_WARN, "dpt_udmaget: nextwin %x",
			status);
#endif
		return (ENOMEM);
	}

/*	get first segment					*/
	dpt_usr_dmaseg = (ddi_dma_seg_t) 0;
	status = ddi_dma_nextseg(dpt_usr_dmawin, dpt_usr_dmaseg,
			&dpt_usr_dmaseg);

	if (status != DDI_SUCCESS) {
#ifdef DPT_DEBUG
		if (dpt_debug & DIOCTL)
			cmn_err(CE_WARN, "dpt_udmaget: first nextseg %x",
			status);
#endif
		return (ENOMEM);
	}

	ddi_dma_segtocookie(dpt_usr_dmaseg, &offset, &len, dmackp);

/* 	check for one single block transfer 				*/
	if (bytes <= dmackp->dmac_size) {
#ifdef DPT_DEBUG
		if (dpt_debug & DIOCTL)
			cmn_err(CE_WARN, "dpt_udmaget [sngl] count %x datap %x",
				bytes, dmackp->dmac_address);
#endif
		scsi_htos_long(ccbp->ccb_datap, dmackp->dmac_address);
		scsi_htos_long(ccbp->ccb_datalen, bytes);
	} else {
/* 		set address of scatter gather segs 			*/
		dmap = ccbp->ccb_sg_list;

		for (bxfer = 0, cnt = 1; ; cnt++, dmap++) {
			bxfer += dmackp->dmac_size;
#ifdef DPT_DEBUG
			if (dpt_debug & DIOCTL)
				cmn_err(CE_WARN,
				"dpt_udmaget [mult] size %x count %x datap %x",
				bxfer, dmackp->dmac_size, dmackp->dmac_address);
#endif
			scsi_htos_long((unchar *)&dmap->data_len,
					dmackp->dmac_size);
			scsi_htos_long((unchar *)&dmap->data_addr,
					dmackp->dmac_address);

/*			check for end of list condition			*/
			if (bytes == bxfer)
				break;
			ASSERT(bytes > bxfer);
/* 			check end of physical scatter-gather list limit */
			if (cnt >= (int)ccbp->ccb_scatgath_siz) {
				break;
			}
			if (ddi_dma_nextseg(dpt_usr_dmawin, dpt_usr_dmaseg,
				&dpt_usr_dmaseg) != DDI_SUCCESS) {
				return (ENOMEM);
			}
			ddi_dma_segtocookie(dpt_usr_dmaseg, &offset, &len,
				dmackp);
		}

		scsi_htos_long(ccbp->ccb_datalen, (ulong)(cnt*sizeof (*dmap)));
		cnt = (int)((paddr_t)ccbp->ccb_paddr +
			((caddr_t)ccbp->ccb_sg_list - (caddr_t)ccbp));
		scsi_htos_long(ccbp->ccb_datap, (ulong)cnt);
	}
	return (0);
}

static void
dpt_udmafree()
{
	if (dpt_usr_dmahandle) {
		ddi_dma_free(dpt_usr_dmahandle);
		dpt_usr_dmahandle = NULL;
	}
}

/*
 * allocate ccb for user ioctl command
 * global dpt mutex is held
 */
static struct dpt_ccb *
dpt_uccballoc(register struct dpt_blk *dpt_blkp, struct dpt_ccb *uccbp,
	int flag, int *status)
{
	register struct dpt_ccb *ccbp;
	int	i;
	caddr_t buf;
	paddr_t	 local;

/*	allocate ccb							*/
	i = sizeof (struct dpt_ccb);
	if (ddi_iopb_alloc(dpt_blkp->db_dip, (ddi_dma_lim_t *)0, (u_int) i,
			&buf)) {
#ifdef DPT_DEBUG
	if (dpt_debug & DIOCTL)
		cmn_err(CE_WARN, "dpt_uccballoc: cannot allocate ccb");
#endif
		*status = ENXIO;
		return ((struct dpt_ccb *)0);
	}
	bzero(buf, i);
	ccbp = (struct dpt_ccb *) buf;

/*	copy in top 24 bytes from user ccb				*/
	if (ddi_copyin((caddr_t)uccbp, (caddr_t)buf, DPT_CORE_CCB_SIZ,
		flag)) {
		ddi_iopb_free(buf);
		*status = EFAULT;
#ifdef DPT_DEBUG
		if (dpt_debug & DIOCTL)
			cmn_err(CE_WARN, "dpt_uccballoc: cannot copyin data");
#endif
		return ((struct dpt_ccb *)0);
	}
	ccbp->ccb_vp = ccbp;
	ccbp->ccb_paddr	  = DPT_KVTOP(ccbp);

/* 	pointer to EATA status packet 					*/
	scsi_htos_long(ccbp->ccb_statp, dpt_blkp->db_stat_paddr);

/* 	auto request sense data physical address 			*/
	local = ccbp->ccb_paddr + ((caddr_t)(&ccbp->ccb_sense.sts_sensedata) -
					(caddr_t)ccbp);
	scsi_htos_long(ccbp->ccb_sensep, local);

#ifdef DPT_DEBUG
	if (dpt_debug & DIOCTL)
		if (uccbp->ccb_optbyte & HA_AUTO_REQSEN)
			cmn_err(CE_WARN, "arq len %x", ccbp->ccb_senselen);
		else
			cmn_err(CE_WARN, "no arq");
#endif

/* 	save scatter gather max 					*/
	ccbp->ccb_scatgath_siz = dpt_blkp->db_scatgath_siz;

/*	need ccb_ioaddr for interrupt handling				*/
	ccbp->ccb_ioaddr = uccbp->ccb_ioaddr;

	return (ccbp);
}

static void
dpt_uccbfree(struct dpt_ccb *ccbp)
{
	ddi_iopb_free((caddr_t)ccbp);
}

/*ARGSUSED*/
static int
dpt_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	register struct dpt 	*dpt;
	register struct dpt_blk	*dpt_blkp;
	int 			unit, len;
	unsigned int		bustype;
	scsi_hba_tran_t		*hba_tran;
	char buf[12];
	int buflen;

#ifdef DPT_DEBUG
if (dpt_debug)
	debug_enter("\n\nDPT ATTACH\n\n");
#endif
	switch (cmd) {
	case DDI_ATTACH:
		break;

	default:
		return (DDI_FAILURE);
	}

	/*
	 * Attach handling
	 */
	dpt = (struct dpt *) kmem_zalloc((unsigned) (sizeof (*dpt) +
		sizeof (*dpt_blkp)), KM_NOSLEEP);
	if (!dpt)
		return (DDI_FAILURE);

	dpt_blkp = (struct dpt_blk *)(dpt + 1);
	DPT_BLKP(dpt)    = dpt_blkp;
	dpt_blkp->db_dip = devi;

	if ((dpt_propinit(dpt_blkp) == DDI_FAILURE) ||
	    (dpt_cfginit(dpt_blkp)  == DDI_FAILURE)) {
		goto err_exit1;
	}

	/*
	 * Allocate a transport structure
	 */
	hba_tran = scsi_hba_tran_alloc(devi, 0);
	if (hba_tran == NULL) {
		cmn_err(CE_WARN, "dpt_attach: scsi_hba_tran_alloc fail\n");
		goto err_exit2;
	}

	dpt->d_tran = hba_tran;

	hba_tran->tran_hba_private	= dpt;
	hba_tran->tran_tgt_private	= NULL;

	hba_tran->tran_tgt_init		= dpt_tran_tgt_init;
	hba_tran->tran_tgt_probe	= dpt_tran_tgt_probe;
	hba_tran->tran_tgt_free		= dpt_tran_tgt_free;

	hba_tran->tran_start 		= dpt_transport;
	hba_tran->tran_abort		= dpt_tran_abort;
	hba_tran->tran_reset		= dpt_tran_reset;
	hba_tran->tran_getcap		= dpt_getcap;
	hba_tran->tran_setcap		= dpt_setcap;
	hba_tran->tran_init_pkt 	= dpt_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= dpt_tran_destroy_pkt;
	hba_tran->tran_dmafree		= dpt_tran_dmafree;
	hba_tran->tran_sync_pkt		= dpt_tran_sync_pkt;

	if (scsi_hba_attach(devi, dpt_blkp->db_limp, hba_tran,
			SCSI_HBA_TRAN_CLONE, NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "dpt_attach: scsi_hba_attach fail");
		goto err_exit3;
	}

/*
 *	If the adapter is PCI, there is no individual conf file entry,
 *	and "bustype" is not defined, and the framework sets up the
 *	"interrupt" property for us, otherwise (for ISA and EISA) we must
 *	do it in dpt_xlate_vec.
 */
	len = sizeof (int);
	if (HBA_INTPROP(devi, "bustype", &bustype, &len) == DDI_PROP_SUCCESS) {
		if (dpt_xlate_vec(dpt_blkp) == -1)
			goto err_exit3;
	}

	/*
	 * configure GHD and enable timeout processing on this HBA
	 */
	if (!dptghd_register(&dpt_blkp->db_ccc, devi, 0, "DPT", dpt_blkp,
			  dpt_ccballoc, dpt_ccbfree, dpt_sg_func, dpt_start,
			  dpt_intr, dpt_get_status, dpt_process_intr,
			  dpt_timeout_action, &dpt_timer_conf)) {

		cmn_err(CE_WARN, "dpt_attach: cannot add real intr");
		goto err_exit3;
	}

	/*
	 * Use a separate mutex for the pkt allocation functions
	 */
	mutex_init(&dpt_blkp->db_rmutex, "DPT Resource Mutex",
		   MUTEX_DRIVER, dpt_blkp->db_iblock);

	mutex_enter(&dpt_global_mutex);	/* protect multithreaded attach	*/
	if (!dpt_global_init) {
		cv_init(&dpt_cv, "dpt_cv", CV_DRIVER, NULL);
		if (ddi_create_minor_node(devi, "control", S_IFCHR, 0,
			DDI_PSEUDO, 0) != DDI_SUCCESS) {
			goto err_exit4;
		}
/* ??? BUG: this doesn't correctly handle PCI cards or */
/* ??? BUG: a ISA card in a EISA slot */
		buflen = sizeof (buf);
		if (ddi_prop_op(DDI_DEV_T_NONE, devi, PROP_LEN_AND_VAL_BUF,
			0, "bus-type", (caddr_t)buf, &buflen) !=
			DDI_PROP_SUCCESS) {
#ifdef DPT_DEBUG
			if (dpt_debug & DIOCTL)
			cmn_err(CE_WARN, "dpt_attach: get bus-type fail");
#endif
			goto err_exit4;
		}
		if (strcmp(buf, "isa") == 0) {
			dpt_usr_dmalim.dlim_ctreg_max = 0xffff;
			dpt_usr_dmalim.dlim_adreg_max = 0xffffff;
		} else {
			dpt_usr_dmalim.dlim_ctreg_max = 0x03fffff;
			dpt_usr_dmalim.dlim_adreg_max = 0x0ffffffffU;
		}
		dpt_usr_dmalim.dlim_sgllen =
			dpt_blkp->db_scatgath_siz;
	}
	dpt_global_init++;


	for (unit = 0; unit < DPT_MAX_CONTROLLERS; unit++) {
		if (dpt_controllers.dc_addr[unit] == 0)
			break;
	}

	ASSERT(unit < DPT_MAX_CONTROLLERS);
	dpt_controllers.dc_addr[unit] = dpt_blkp->db_ioaddr;
	dpt_controllers.dc_blkp[unit] = dpt_blkp;

	mutex_exit(&dpt_global_mutex);


	dpt_enable_interrupts(dpt_blkp->db_ioaddr);

	ddi_report_dev(devi);

	return (DDI_SUCCESS);



	/*
	 * if an error occurs, unwind the allocations in reverse order
	 */
err_exit4:
	cv_destroy(&dpt_cv);
	mutex_exit(&dpt_global_mutex);

	mutex_destroy(&dpt_blkp->db_rmutex);
	dptghd_unregister(&dpt_blkp->db_ccc);

err_exit3:
	scsi_hba_tran_free(hba_tran);

err_exit2:
	kmem_free((caddr_t)dpt, (sizeof (*dpt) + sizeof (*dpt_blkp)));

err_exit1:
	return (DDI_FAILURE);
}

static int
dpt_propinit(register struct dpt_blk *dpt_blkp)
{
	register dev_info_t *devi;
	int	i;
	int	val;
	int	len;
	unsigned int	bustype;

	devi = dpt_blkp->db_dip;
	len = sizeof (int);
	if (HBA_INTPROP(devi, "ioaddr", &val, &len) == DDI_PROP_SUCCESS)
		dpt_blkp->db_ioaddr   = (ushort) val;

	if (HBA_INTPROP(devi, "dmachan", &val, &len) == DDI_PROP_SUCCESS)
		dpt_blkp->db_dmachan  = (unchar) val;

	if (HBA_INTPROP(devi, "bustype", &bustype, &len) != DDI_SUCCESS) {
		bustype = DPT_PCI_ADAPTER;
	}

	mutex_enter(&dpt_global_mutex);
	if (!dpt_pgsz) {
		dpt_pgsz = ddi_ptob(devi, 1L);
		dpt_pgmsk = dpt_pgsz - 1;
		for (i = dpt_pgsz, len = 0; i > 1; len++)
			i >>= 1;
		dpt_pgshf = len;
	}
	mutex_exit(&dpt_global_mutex);

/*	now initialize the status block phy address			*/
	dpt_blkp->db_stat_paddr = DPT_KVTOP(&dpt_blkp->db_stat);

	if (bustype == DPT_PCI_ADAPTER)
		return (dpt_find_pci_ctrl(dpt_blkp));

	if (bustype == DPT_EISA_ADAPTER) {
		val =  dpt_find_eisa_ctrl(dpt_blkp->db_ioaddr, devi);
		if (val == DDI_SUCCESS) {
			dpt_blkp->db_limp = &dpt_eisadma_lim;
		}
		return (val);
	}
	dpt_blkp->db_limp = &dpt_isadma_lim;
	return (DDI_SUCCESS);
}

static void
dpt_do_abort_cmd( struct dpt_blk *dpt_blkp, paddr_t ccb_paddr )
{
	ushort	port = dpt_blkp->db_ioaddr;

	if (dpt_wait(port + HA_AUX_STATUS, 0, HA_AUX_BUSY))
		return;

	/* Write the CP physical address in 1F2-1F5 LSB to MSB */
	outb(port + HA_ERROR, 0);
	outb(port + HA_DMA_BASE + 0, (unchar)(ccb_paddr));
	outb(port + HA_DMA_BASE + 1, (unchar)(ccb_paddr >> 8));
	outb(port + HA_DMA_BASE + 2, (unchar)(ccb_paddr >> 16));
	outb(port + HA_DMA_BASE + 3, (unchar)(ccb_paddr >> 24));
	outb(port + HA_IMMED_FUNC, CP_EI_ABORT_CP);
	outb(port + HA_COMMAND, CP_EATA_IMMED);
	return;
}


static void
dpt_do_abort_dev( struct dpt_blk *dpt_blkp, unchar target, unchar lun )
{
	ushort	port = dpt_blkp->db_ioaddr;

	if (dpt_wait(port + HA_AUX_STATUS, 0, HA_AUX_BUSY))
		return;

	/* Write the CP physical address in 1F2-1F5 LSB to MSB */
	outb(port + HA_ERROR, 0);
	outb(port + HA_DMA_BASE + 0, 0);
	outb(port + HA_DMA_BASE + 1, 0);
	outb(port + HA_DMA_BASE + 2, lun);
	outb(port + HA_DMA_BASE + 3, target);
	outb(port + HA_IMMED_FUNC, CP_EI_ABORT_MSG);
	outb(port + HA_COMMAND, CP_EATA_IMMED);
	return;
}


static void
dpt_do_reset_target( struct dpt_blk *dpt_blkp, unchar target, unchar lun )
{
	ushort	port = dpt_blkp->db_ioaddr;

	if (dpt_wait(port + HA_AUX_STATUS, 0, HA_AUX_BUSY))
		return;

	/* Write the CP physical address in 1F2-1F5 LSB to MSB */
	outb(port + HA_ERROR, 0);
	outb(port + HA_DMA_BASE + 0, 0);
	outb(port + HA_DMA_BASE + 1, 0);
	outb(port + HA_DMA_BASE + 2, lun);
	outb(port + HA_DMA_BASE + 3, target);
	outb(port + HA_IMMED_FUNC, CP_EI_RESET_MSG);
	outb(port + HA_COMMAND, CP_EATA_IMMED);
	return;
}


static void
dpt_do_reset_bus( struct dpt_blk *dpt_blkp, unchar bus )
{
	ushort	port = dpt_blkp->db_ioaddr;
	unchar	bus_mask = 1 << (bus - 1);

	if (dpt_wait(port + HA_AUX_STATUS, 0, HA_AUX_BUSY))
		return;

	/* Write the CP physical address in 1F2-1F5 LSB to MSB */
	outb(port + 1, 0);
	outb(port + 2, 0);
	outb(port + 3, 0);
	outb(port + 4, 0);

#ifdef __notyet__
	/* This doesn't work on firmware rev 7C */
	outb(port + HA_IMMED_MOD, bus_mask);
	outb(port + HA_IMMED_FUNC, CP_EI_RESET_BUSES);
	outb(port + HA_COMMAND, CP_EATA_IMMED);
#else
	/* so reset all the buses */
	outb(port + HA_IMMED_MOD, 0);
	outb(port + HA_IMMED_FUNC, CP_EI_RESET_BUS);
	outb(port + HA_COMMAND, CP_EATA_IMMED);
#endif

	/* Delay 1 second for the devices on scsi bus to settle */
	drv_usecwait(1000000);

	return;
}

/*
 * dpt_timeout_action()
 *
 *
 *	Called when a request has timed out. Start out subtle and
 *	just try to abort the specific request. Escalate all the
 *	way upto resetting the whole SCSI bus.
 *
 *
 */

static void
dpt_timeout_action(	void	*hba_handle,
			gcmd_t	*gcmdp,
			struct scsi_address *ap,
			gact_t	 action )
{
	struct dpt_blk	*dpt_blkp = hba_handle;
	struct scsi_pkt	*pktp;
	
	if (gcmdp != NULL) {
		pktp = GCMDP2PKTP(gcmdp);
	} else {
		pktp = NULL;
	}


	switch (action) {
	case GACTION_EARLY_ABORT:
		/*
		 * abort before request was started, just
		 * set the pkt_reason and pkt_statistics 
		 */
		if (pktp != NULL) {
			if (pktp->pkt_reason == CMD_CMPLT)
				pktp->pkt_reason = CMD_ABORTED;
			pktp->pkt_statistics |= STAT_ABORTED;
		}
		break;

	case GACTION_EARLY_TIMEOUT:
		/* timeout before request was started */
		if (pktp != NULL) {
			if (pktp->pkt_reason == CMD_CMPLT)
				pktp->pkt_reason = CMD_TIMEOUT;
			pktp->pkt_statistics |= STAT_TIMEOUT;
		}
		break;
		
	case GACTION_ABORT_CMD:
		if (gcmdp != NULL && gcmdp->cmd_private != NULL) {
			dpt_do_abort_cmd(dpt_blkp,
					 GCMDP2CCBP(gcmdp)->ccb_paddr);
		}
		break;

	case GACTION_ABORT_DEV:
		if (ap)  {
			dpt_do_abort_dev(dpt_blkp, ap->a_target,
						   ap->a_lun & MSG_LUNRTN);
		}
		break;

	case GACTION_RESET_TARGET:
		if (ap) {
			dpt_do_reset_target(dpt_blkp, ap->a_target,
						      ap->a_lun & MSG_LUNRTN);
		}
		break;

	case GACTION_RESET_BUS:
		dpt_do_reset_bus(dpt_blkp, 0);
		break;
	}
	return;
}

/*
 * dpt_tran_abort()
 *
 *	Abort specific command on a target or all commands on
 * a specific target.
 *
 */
static int
dpt_tran_abort(struct scsi_address *ap, struct scsi_pkt *pktp)
{
	ulong	intr_status;

	if (pktp)
		return (dptghd_tran_abort(&ADDR2DPTBLKP(ap)->db_ccc,
					PKTP2GCMDP(pktp), ap, &intr_status));

	return (dptghd_tran_abort_lun(&ADDR2DPTBLKP(ap)->db_ccc, ap,
				   &intr_status));

}



/* reset the scsi bus, or just one target device */
/* returns 0 on failure, 1 on success */
static int
dpt_tran_reset( struct scsi_address *ap, int level )
{
	ulong	intr_status;

	if (level == RESET_TARGET)
		return (dptghd_tran_reset_target(&ADDR2DPTBLKP(ap) ->db_ccc, ap,
					   &intr_status));
	if (level == RESET_ALL)
		return (dptghd_tran_reset_bus(&ADDR2DPTBLKP(ap) ->db_ccc, 
					   &intr_status));
	return (FALSE);
}


static int
dpt_capchk(char *cap, int tgtonly, int *cidxp)
{
	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *) 0)
		return (FALSE);

	*cidxp = scsi_hba_lookup_capstr(cap);
	return (TRUE);
}

static int
dpt_getcap(struct scsi_address *ap, char *cap, int tgtonly)
{
	int	ckey;
	int	total_sectors, h, s;

	if (dpt_capchk(cap, tgtonly, &ckey) != TRUE)
		return (-1);

/*	Copy the parameter banding scheme used in the SmartROM, this is	*/
/*	really only necessary for the boot drive with no CMOS entry.	*/
/*	However, it could save user confusion since all drives will be	*/
/*	parameterized the same in the ROM and in UNIX.			*/
/*	Upto 1G (200000 blocks) use 64 x 32				*/
/*	2G (400000 blocks) use 65 x 63					*/
/*	4G (800000 blocks) use 128 x 63					*/
/*	Over 4G (800001 blocks) use 255 x 63				*/

	switch (ckey) {
		case SCSI_CAP_GEOMETRY:

		total_sectors = (ADDR2DPTUNITP(ap))->du_total_sectors;
		if (total_sectors <= 0)
			break;

		if (total_sectors <= 0x200000) {
			h = 64;
			s = 32;
		} else if (total_sectors <= 0x400000) {
			h = 65;
			s = 63;
		} else if (total_sectors <= 0x800000) {
			h = 128;
			s = 63;
		} else {
			h = 255;
			s = 63;
		}
		return (HBA_SETGEOM(h, s));

		case SCSI_CAP_ARQ:
			return (TRUE);
		default:
			break;
	}
	return (-1);
}

static int
dpt_setcap(struct scsi_address *ap, char *cap, int value, int tgtonly)
{
	int	ckey, status = FALSE;

	if (dpt_capchk(cap, tgtonly, &ckey) != TRUE) {
		return (-1);
	}

	switch (ckey) {
		case SCSI_CAP_SECTOR_SIZE:
			(ADDR2DPTUNITP(ap))->du_lim.dlim_granular
				= (u_int)value;
			status = TRUE;
			break;

		case SCSI_CAP_TOTAL_SECTORS:
			(ADDR2DPTUNITP(ap))->du_total_sectors = value;
			status = TRUE;
			break;

		case SCSI_CAP_ARQ:
			if (tgtonly) {
				(ADDR2DPTUNITP(ap))->du_arq = (u_int)value;
				status = TRUE;
			}
			break;

		case SCSI_CAP_GEOMETRY:
		default:
			break;
	}

	return (status);
}


static struct scsi_pkt *
dpt_tran_init_pkt(	struct scsi_address	*ap,
			struct scsi_pkt		*pktp,
			struct buf		*bp,
			int			 cmdlen,
			int			 statuslen,
			int			 tgtlen,
			int			 flags,
			int			(*callback)(),
			caddr_t			 arg )
{
	struct dpt_blk	*dpt_blkp = ADDR2DPTBLKP(ap);
	struct scsi_pkt *new_pktp;
	int		 cnt = 0;

	mutex_enter(&dpt_blkp->db_rmutex);
	/*
	 * call the GHD pkt allocator with appropriate args
	 */
	new_pktp = dptghd_tran_init_pkt(&dpt_blkp->db_ccc, ap, pktp, bp, 0,
				statuslen, tgtlen, flags, callback, arg,
				sizeof (dwrap_t),
				&(ADDR2DPTUNITP(ap)->du_lim), &cnt);

	if (!new_pktp) {
		mutex_exit(&dpt_blkp->db_rmutex);
		return (NULL);
	}

	if (bp) {
		struct dpt_ccb	*ccbp = PKTP2CCBP(new_pktp);

		/*
		 * if dpt_sg_func() created a scatter-gather list for this
		 * request, then store the size of list in the CCB
		 */

		if (cnt != 0) {
			scsi_htos_long(ccbp->ccb_datalen,
						cnt * sizeof(struct dpt_sg));
		} 

		if (!bp->b_bcount) {
			ccbp->ccb_optbyte &= ~(HA_DATA_IN | HA_DATA_OUT);
		}

		if (bp->b_flags & B_READ) {
			ccbp->ccb_optbyte &= ~HA_DATA_OUT;
			ccbp->ccb_optbyte |= HA_DATA_IN;
		} else {
			ccbp->ccb_optbyte &= ~HA_DATA_IN;
			ccbp->ccb_optbyte |= HA_DATA_OUT;
		}

#ifdef DPT_DEBUG
		if (dpt_debug & DPKT) {
			PRF("dpt_tran_init_pkt(): pktp= 0x%x pkt_cdbp=0x%x"
					"pkt_sdbp=0x%x\n",
					new_pktp, new_pktp->pkt_cdbp,
					new_pktp->pkt_scbp);
			PRF("ccbp= 0x%x\n", ccbp);
		}
#endif
	}

	mutex_exit(&dpt_blkp->db_rmutex);
	return (new_pktp);
}

static void
dpt_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pktp)
{
	struct dpt_blk	*dpt_blkp = ADDR2DPTBLKP(ap);

	mutex_enter(&dpt_blkp->db_rmutex);
	dpt_tran_dmafree(ap, pktp);
	dpt_pktfree(ap, pktp);
	mutex_exit(&dpt_blkp->db_rmutex);
}


/*
 * dpt_ccballoc()
 *
 *	The DPT adapter requires a mailbox allocated from IOPB memory.
 *	Allocate it here and save its address in the HBA cmd strucuture
 *	that's already been allocated (via kmem_alloc) by GHD.
 *
 *	The dpt_cmd structure serves as a wrapper for the GHD gcmd_t
 *	structure plus a ptr to the DPT mailbox allocated from IOPB space.
 */

static gcmd_t *
dpt_ccballoc(	struct scsi_address	*ap,
		struct scsi_pkt *pktp,
		void	*bufp,
		int	 cmdlen,
		int	 statuslen,
		int	 tgtlen,
		int  	 ccblen )
{
	struct dpt_blk	*dpt_blkp = ADDR2DPTBLKP(ap);
	dwrap_t		*dwp = (dwrap_t *)bufp;
	gcmd_t		*gcmdp = &dwp->dw_gcmd;
	struct scsi_arq_status	*stsp;
	ccb_t		*ccbp;
	ulong		 tmp;
	int		 senselen;


	if (statuslen < sizeof (struct scsi_arq_status)) {
		statuslen = sizeof (struct scsi_arq_status);
		senselen = sizeof (struct scsi_extended_sense);
	} else {
		senselen = sizeof (struct scsi_extended_sense) +
			   (statuslen - sizeof (struct scsi_arq_status));
	}

	/* allocate ccb from IOPB memory pool */
	if (ddi_iopb_alloc(dpt_blkp->db_dip, &ADDR2DPTUNITP(ap)->du_lim,
			sizeof (ccb_t) + statuslen,
			(caddr_t *)&ccbp) != DDI_SUCCESS) {
		return (NULL);
	}
	bzero((caddr_t)ccbp, sizeof (ccb_t) + statuslen);

	/*
	 * Initialize the scsi_pkt and DPT CCB 
	 */

	/* the CDB buffer is in within the CCB in IOPB space */
	pktp->pkt_cdbp = ccbp->ccb_cdb;

	/* ARQ status buffer follows the CP buffer */
	stsp = (struct scsi_arq_status *)(ccbp + 1);
	pktp->pkt_scbp = (unchar *)stsp;
	ccbp->ccb_senselen = senselen;

	if ((ADDR2DPTUNITP(ap))->du_arq)
		ccbp->ccb_optbyte = HA_AUTO_REQSEN;

	ccbp->ccb_id = ap->a_target;
	ccbp->ccb_msg0 = ap->a_lun + HA_IDENTIFY_MSG;

	ccbp->ccb_vp = ccbp;

	/* CP physical address */
	tmp = ccbp->ccb_paddr = DPT_KVTOP(ccbp);

	/* auto request sense data physical address */
	tmp += sizeof (ccb_t) + ((ulong)(&stsp->sts_sensedata) - (ulong)stsp);
	scsi_htos_long(ccbp->ccb_sensep, tmp);

	/* pointer to EATA status packet */
	scsi_htos_long(ccbp->ccb_statp, dpt_blkp->db_stat_paddr);


	/* save scatter gather max */
	ccbp->ccb_scatgath_siz = dpt_blkp->db_scatgath_siz;



	/*
	 * cross-link the kmem and iopb buffers
	 */
	ccbp->ccb_ownerp = dwp;
	dwp->dw_ccbp = ccbp;

	/*
	 * return to GHD the ptr to its gcmd_t structure 
	 */
	return (gcmdp);
}



/*
 * DMA resource allocation
 */
static void
dpt_sg_func(	struct scsi_pkt	*pktp,
		gcmd_t	 	*gcmdp,
		ddi_dma_cookie_t *dmackp,
		int		 single_segment,
		int		 seg_num,
		void		*arg )
{
	struct dpt_ccb	*ccbp = GCMDP2CCBP(gcmdp);
	struct dpt_sg	*dmap;


	if (seg_num == 0) {
		ulong	tmp;

		if (single_segment) {
			scsi_htos_long(ccbp->ccb_datalen, dmackp->dmac_size);
			scsi_htos_long(ccbp->ccb_datap, dmackp->dmac_address);
			return;
		}

		/* else setup for scatter-gather transfer */

		ccbp->ccb_optbyte |= HA_SCATTER;
		tmp = (ulong)(ccbp->ccb_paddr);
		tmp += (ulong)(ccbp->ccb_sg_list) - (ulong)ccbp;
		scsi_htos_long(ccbp->ccb_datap, tmp);
	}

	/* set address of scatter gather segs */
	dmap = &ccbp->ccb_sg_list[seg_num];

	scsi_htos_long((unchar *)&dmap->data_len, dmackp->dmac_size);
	scsi_htos_long((unchar *)&dmap->data_addr, dmackp->dmac_address);

	*(int *)arg = seg_num + 1;
	return;
}



static void
dpt_ccbfree( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	ddi_iopb_free((caddr_t)PKTP2CCBP(pktp));
	return;
}

/*
 * packet free
 */
/*ARGSUSED*/
void
dpt_pktfree( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	dptghd_pktfree(&ADDR2DPTBLKP(ap)->db_ccc, ap, pktp);
	return;
}


/*ARGSUSED*/
static void
dpt_tran_sync_pkt( struct scsi_address *ap, register struct scsi_pkt *pktp )
{
	dptghd_tran_sync_pkt(ap, pktp);
	return;
}

static void
dpt_tran_dmafree( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	dptghd_tran_dmafree(ap, pktp);
	return;
}

/*ARGSUSED*/
static int
dpt_transport(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	struct dpt_blk	*dpt_blkp;
	struct dpt_ccb	*ccbp;
	ulong		 intr_status;

	dpt_blkp = PKT2DPTBLKP(pktp);

	pktp->pkt_reason = CMD_CMPLT;
	pktp->pkt_state = 0;
	pktp->pkt_statistics = 0;

	return (dptghd_transport(&dpt_blkp->db_ccc, PKTP2GCMDP(pktp),
		ap, pktp->pkt_time, (pktp->pkt_flags & FLAG_NOINTR),
		&intr_status));
}

/*ARGSUSED*/
static int
dpt_start( void	*hba_handle, gcmd_t *gcmdp )
{
	struct dpt_blk	*dpt_blkp = hba_handle;
	struct scsi_pkt	*pktp = GCMDP2PKTP(gcmdp);
	struct dpt_ccb	*ccbp = PKTP2CCBP(pktp);


	if (pktp->pkt_flags & (FLAG_NOINTR | FLAG_NODISCON))
		ccbp->ccb_msg0 &= ~HA_DISCO_RICO;
	else
		ccbp->ccb_msg0 |= HA_DISCO_RICO;


	if (dpt_send(dpt_blkp->db_ioaddr, ccbp)) {
		return (TRAN_BUSY);
	}

	return (TRAN_ACCEPT);
}


/* Mutex is held when this function is called */
static int
dpt_send(ushort ioaddr, struct dpt_ccb *ccbp)
{
	register int loopc;

	/* Wait for controller AUX STATUS not BUSY */
	for (loopc = 0; loopc < 500000; loopc++) {
		if (!(inb(ioaddr+HA_AUX_STATUS) & HA_AUX_BUSY)) {
			dpt_send_cmd(ioaddr, ccbp->ccb_paddr, CP_DMA_CMD);
			return (0);
		}
		drv_usecwait(10);
	}
	return (1);
}


static void
dpt_chkerr(	struct dpt_blk	*dpt_blkp,
		struct dpt_stat	*sp,
		struct scsi_pkt	*pktp,
		struct dpt_ccb	*ccbp )
{
	static struct scsi_status okay_status = {0};	/* init-ed to zeros */
	struct   scsi_arq_status *arqp;
	int	 cstatus;

	cstatus = ccbp->ccb_ctlrstat = sp->sp_hastat & HA_STATUS_MASK;

	if (cstatus > sizeof dpt_errs / sizeof dpt_errs[0]) {
		pktp->pkt_reason = CMD_TRAN_ERR;
		return;
	}

	/*
	 * Map the DPT controller status into SCSA values
	 */
	pktp->pkt_reason = dpt_errs[cstatus].reason;
	pktp->pkt_state |= dpt_errs[cstatus].state;
	pktp->pkt_statistics |= dpt_errs[cstatus].statistics;

	ccbp->ccb_scsistat = sp->sp_scsi_stat;
	*pktp->pkt_scbp = sp->sp_scsi_stat;
	pktp->pkt_resid = scsi_stoh_long(*(long *) sp->sp_inv_residue);

	switch (ccbp->ccb_ctlrstat) {
	case  DPT_OK:
		if ((SCBP_C(pktp) == STATUS_GOOD)) {
			pktp->pkt_resid = 0;
			pktp->pkt_state = (STATE_XFERRED_DATA|STATE_GOT_BUS|
					   STATE_GOT_TARGET|STATE_SENT_CMD|
					   STATE_GOT_STATUS);
			return;
		}

		if ((SCBP(pktp)->sts_chk)
		&&  (PKT2DPTUNITP(pktp))->du_arq
		&&  (ccbp->ccb_ctlrstat != DPT_REQSENFAIL)) {
			/* ARQ did not fail*/
			pktp->pkt_state  |=
				(STATE_GOT_BUS|STATE_GOT_TARGET|
				STATE_SENT_CMD|STATE_GOT_STATUS|
				STATE_ARQ_DONE);

			arqp = (struct scsi_arq_status *)pktp->pkt_scbp;
			arqp->sts_rqpkt_status = okay_status;
			arqp->sts_rqpkt_reason = CMD_CMPLT;
			arqp->sts_rqpkt_resid  = 0;
			arqp->sts_rqpkt_state |= STATE_XFERRED_DATA;
		}
		break;

	case  DPT_BUSRST:
		if (sp->sp_scsi_stat == 4) {
			/* this request was on the bus when it was reset */
			pktp->pkt_state |= (STATE_GOT_BUS | STATE_GOT_TARGET);
			pktp->pkt_statistics |= STAT_BUS_RESET;
#ifdef DPT_DEBUG
			if (dpt_debug & DTST)
				cmn_err(CE_WARN,
					"dpt_chkerr: bus reset active\n");
#endif

#ifdef DPT_DEBUG
		} else {
			if (dpt_debug & DTST)
				cmn_err(CE_WARN,
					"dpt_chkerr: bus reset not active\n");
#endif
		}
		break;

#ifdef DPT_DEBUG
	default:
		if (dpt_debug & DTST)
			cmn_err(CE_WARN,
				"dpt_chkerr: ctrlstat=0x%x\n",
				ccbp->ccb_ctlrstat);
		break;
#endif
	}
	return;
}


/*ARGSUSED*/
static int
dpt_get_status( void *hba_handle, void *arg )
{
	struct dpt_blk	 *dpt_blkp = (struct dpt_blk *)hba_handle;

	if (!(inb(dpt_blkp->db_ioaddr + HA_AUX_STATUS) & HA_AUX_INTR)) {
		return (FALSE);
	}
	return (TRUE);
}


/*ARGSUSED*/
static void
dpt_process_intr(	void	*hba_handle,
			void	*arg )
{
	struct dpt_blk	*dpt_blkp = (struct dpt_blk *)hba_handle;
	struct dpt_stat	*statp = &dpt_blkp->db_stat;
	struct dpt_ccb	*ccbp;
	struct scsi_pkt *pktp;
	gcmd_t		*gcmdp;

	/* extract virtual pointer to ccb */
/***
 *** BUG: it's a bad idea to depend on the HBA to pass back a valid
 ***	kernel pointer. Should set up a lookup table and pass the
 ***	table index back and forth.
 ***/
	ccbp = statp->sp_vp;

	/* make sure the ccb is not re-used */
	statp->sp_vp = NULL;

	if (ccbp == (struct dpt_ccb *)NULL) {
		cmn_err(CE_CONT, "dpt_intr(%d): null ptr\n",
				 dpt_blkp->db_ioaddr);

	} else if (ccbp->ccb_ioaddr == 0) {

		gcmdp = CCBP2GCMDP(ccbp);
		pktp = GCMDP2PKTP(gcmdp);
		dpt_chkerr(dpt_blkp, statp, pktp, ccbp);

		/*
		 * The request is now complete. Stop the packet timer
		 * and schedule the pkt completion callback.
		 */
		dptghd_complete(&dpt_blkp->db_ccc, gcmdp);

	} else {
		dpt_ioctl_intr(dpt_blkp, ccbp);
	}

	/* clear the interrupt */
	inb(dpt_blkp->db_ioaddr + HA_STATUS);
}



static u_int
dpt_intr(caddr_t arg)
{
	struct dpt_blk	*dpt_blkp = (struct dpt_blk *)arg;
	ulong	intr_status;

	return (dptghd_intr(&dpt_blkp->db_ccc, &intr_status));
}

/*
 * dpt_ioctl_intr must hold the HBA mutex
 */
/*ARGSUSED*/
static void
dpt_ioctl_intr(register struct dpt_blk *dpt_blkp, struct dpt_ccb *ccbp)
{
	register struct	dpt_stat *sp;

/* 	get status block from hba - 1 per controller 			*/
	sp = &dpt_blkp->db_stat;

	ccbp->ccb_ctlrstat = sp->sp_hastat & HA_STATUS_MASK;
	ccbp->ccb_scsistat = sp->sp_scsi_stat;

	mutex_enter(&dpt_global_mutex);
/*	wake up the sleeping ioctl thread				*/
	cv_signal(&dpt_cv);
	mutex_exit(&dpt_global_mutex);

}

/*
 * dpt_findhba -- Determine if we really have a card in the machine.
 */
static int
dpt_findhba(register uint ioaddr)
{
/*	check for ioaddr conflict with IDE				*/
	if ((ioaddr == 0x1f0) || (ioaddr == 0x230)) {
		if (inb(ioaddr + HA_AUX_STATUS) == 0xFF)
			return (DDI_FAILURE);
	}
	if (inb(ioaddr + HA_STATUS) == 0xFF)
		return (DDI_FAILURE);

/*	If controller is present and Sane then it should be presenting	*/
/*	SeekComplete and Ready.	*/

/*
 * MODIFICATION : changed the and value from FE to DC to remove the bits
 * in the register that are not being used.
 */

	if ((inb(ioaddr + HA_STATUS) & 0xDC) == (HA_ST_SEEK_COMP+HA_ST_READY)) {
		dpt_found = TRUE;
#ifdef DPT_DEBUG
	if (dpt_debug & DINIT)
		cmn_err(CE_WARN, "dpt_findhba: dpt at ioaddr %x", ioaddr);
#endif
		return (DDI_SUCCESS);
	}

	return (DDI_FAILURE);
}

/*
 * These are the known EISA product IDs for DPT HBAs
 */
static pid_spec_t default_pids[] = {
	/* note: these are ulong-s in little-endian byte order */
	{ 0x00001412, 0x0000ffff },	/* DPT ID Pal */
	{ 0x0082a338, 0x00ffffff }	/* NEC ID Pal on DPT EISA Ctlrs */

	/*
	 * ??? these product ID values are incomplete but
	 * they're all I've got; each of the above table entries should
	 * really specify the full seven nibbles like the EISA spec says to.
	 */
};

static int
dpt_find_eisa_ctrl(register uint ioaddr, dev_info_t *devi)
{

	if (ioaddr < 0x1000)
		return (DDI_FAILURE);

	/* check the EISA NVRAM for presence of DPT adapter */
	if (dpt_eisa_probe(devi, ioaddr, &default_pids,
				sizeof default_pids / sizeof default_pids[0]))
		return (DDI_SUCCESS);

	return (DDI_FAILURE);
}

static int
dpt_find_pci_ctrl(register struct dpt_blk *dpt_blkp)
{
	if (dpt_search_pci((uint *)&dpt_blkp->db_ioaddr,
		dpt_blkp->db_dip)
		== DDI_SUCCESS) {
		dpt_blkp->db_limp   = &dpt_eisadma_lim;
		return (DDI_SUCCESS);
	}
	else
		return (DDI_FAILURE);
}

static u_int
dpt_xlate_vec(struct dpt_blk *dpt_blkp)
{
	register int vec = dpt_blkp->db_intr;
	int	 intrspec[3];

	/* create an interrupt spec using default interrupt priority level */
	intrspec[0] = 2;
	intrspec[1] = 5;
	intrspec[2] = vec; /* set irq */

	if (ddi_ctlops(dpt_blkp->db_dip, dpt_blkp->db_dip,
		DDI_CTLOPS_XLATE_INTRS,
		(caddr_t)intrspec, ddi_get_parent_data(dpt_blkp->db_dip))
		!= DDI_SUCCESS) {
#ifdef DPT_DEBUG
		if (dpt_debug & DINIT)
		cmn_err(CE_WARN, "dpt_xlate_vec: get parent data failed");
#endif
		return ((u_int) -1);
	}

	return (0);
}

static int
dpt_cfginit(register struct  dpt_blk *dpt_blkp)
{
	struct ReadConfig *cfp;
	short	sg_sz;
	int	cfg_sz;

/* 	allocate ReadConfig buffer RC 					*/
	if (ddi_iopb_alloc(dpt_blkp->db_dip, (ddi_dma_lim_t *)0,
		(u_int)(sizeof (*cfp)), (caddr_t *)&cfp)) {
#ifdef DPT_DEBUG
		if (dpt_debug & DDATA)
			PRF("dpt_cfginit: could not alloc ReadConfig struct\n");
#endif
		return (DDI_FAILURE);
	}

/* 	Get Host Adapter configuration info  				*/
	if ((dpt_rdconf(dpt_blkp->db_ioaddr, (caddr_t)cfp) == DDI_FAILURE) ||
	    (cfp->EATAsignature[0] != 'E') || (cfp->EATAsignature[1] != 'A') ||
	    (cfp->EATAsignature[2] != 'T') || (cfp->EATAsignature[3] != 'A') ||
	    (!cfp->DMAsupported)) {
		ddi_iopb_free((caddr_t)cfp);
		return (DDI_FAILURE);
	}

/*	If this card requires a DMA Channel then set it up now.		*/
	if (cfp->DMAChannelValid) {
		dpt_blkp->db_dmachan = 8 - ((int)(cfp->DMA_Channel & 7));
		if (ddi_dmae_1stparty(dpt_blkp->db_dip, dpt_blkp->db_dmachan)
			!= DDI_SUCCESS) {
			ddi_iopb_free((caddr_t)cfp);
			return (DDI_FAILURE);
		}
	}

/*	save and check interrupt vector number				*/
	dpt_blkp->db_intr = cfp->IRQ_Number;

/* 	store queue size 						*/
	dpt_blkp->db_q_siz = scsi_stoh_short(*(ushort *)cfp->QueueSize);

/* 	save in the SCSI ID of the adapter 				*/
	dpt_blkp->db_targetid = cfp->HBA[3];

	sg_sz = scsi_stoh_short(*(ushort *)cfp->SG_Size);
	if (sg_sz < DPT_MAX_DMA_SEGS)
		dpt_blkp->db_scatgath_siz = (u_char) sg_sz;
	else
		dpt_blkp->db_scatgath_siz = DPT_MAX_DMA_SEGS;
	cfg_sz = scsi_stoh_long(*(ulong *)cfp->ConfigLength);

	/*
	 * allow for Wide SCSI target numbers
	 */
	dpt_blkp->db_max_target = 15;
	dpt_blkp->db_max_lun = 7;
	if (cfg_sz >= HBA_BUS_TYPE_LENGTH) {
		if (cfp->MaxScsiID < 15)
			dpt_blkp->db_max_target = cfp->MaxScsiID;
		if (cfp->MaxLUN < 7)
			dpt_blkp->db_max_lun = cfp->MaxLUN;
	}

	ddi_iopb_free((caddr_t)cfp);
	return (DDI_SUCCESS);
}

/*
 * dpt_rdconf - Issue an EATA Read Config Command, Process PIO just
 * in case we have a PM2011 with the ROM disabled, so it can't do DMA yet
 */
static int
dpt_rdconf(register int port, caddr_t buf)
{
	register int padcnt;
	int enabled;

/*	Disable Interrupts To The System */

	enabled = dpt_disable_interrupts(port);

/* 	A 1 Returned From The Disable Means Busy Did Not Go Down, Error */

	if (enabled == 1)
		return (DDI_FAILURE);

/* 	Send the Read Config EATA PIO Command */
	outb(port + HA_STATUS, CP_READ_CFG_PIO);

/*	Wait for DRQ Interrupt						*/
	if (dpt_wait(port + HA_STATUS, HA_ST_DATA_RDY, 0)) {
		inb(port + HA_STATUS);
		return (DDI_FAILURE);
	}

/*	Take the Config Data						*/
	repinsw(port + HA_DATA, (unsigned short *)buf,
		(sizeof (struct ReadConfig) + 1) / 2);

/*	Take the remaining data						*/
	for (padcnt = (512 - sizeof (struct ReadConfig))/2; padcnt; padcnt--)
		inw(port + HA_DATA);

	inb(port + HA_STATUS);

#ifdef DPT_DEBUG
	if (dpt_debug & DINIT) {
		struct ReadConfig *r = (struct ReadConfig *)buf;

		r->EATAversion = 0;
		PRF("EATAcfg->signature    = %s\n", r->EATAsignature);
		PRF("EATAcfg->OverLapCmds  = %d\n", r->OverLapCmds);
		PRF("EATAcfg->DMAsupported = %d\n", r->DMAsupported);
		PRF("EATAcfg->IRQ_Number   = %d\n", r->IRQ_Number);
		PRF("EATAcfg->IRQ_Trigger  = %d\n", r->IRQ_Trigger);
		if (r->DMAChannelValid)
		PRF("EATAcfg->DMA_Channel  = %d\n", (8 - r->DMA_Channel) & 7);
		PRF("EATAcfg->Secondary    = %d\n", r->Secondary);
		PRF("EATAcfg->SG_Size      = 0x%x\n", (int)r->SG_Size[3]);
		PRF("EATAcfg->HBAvalid     = %d  HBA=%d\n", r->HBAvalid,
		r->HBA[3]);
	}
#endif
	return (DDI_SUCCESS);
}

/*
 * dpt_wait --  wait for a register of a controller to achieve a
 * specific state.  Arguments are two masks.  To return normally, all
 * the bits in the first mask must be ON, all the bits in the second
 * mask must be OFF.  If 5 seconds pass without the controller
 * achieving the desired bit configuration, we return 1, else 0.
 *
 */
static int
dpt_wait(register ushort port, ushort onbits, ushort offbits)
{
	register int i;
	register ushort val;

	for (i = 500000; i; i--) {
		val = inb(port);
		if ((val & onbits) == onbits && (val & offbits) == 0)
			return (0);
		drv_usecwait(10);
	}
	return (1);
}

static int
dpt_wait_disabled(register ushort port, struct dpt_stat *spp, int timeout)
{
	int i;

/* 	Wait For busy to go down before we poll the packet */

	while (dpt_wait(port + HA_AUX_STATUS, 0, HA_AUX_BUSY)) {

/* 	If This command has a timeout set, return an error */
		if (timeout)
			return (1);
	}

/* 	When The 0xae Value That We Put In The Status Packet Before We Sent */
/* 	Off The Command Goes Away, The Command Will Be Complete. The Reason */
/* 	That We Don't Poll On The EOC Bit Is That It Is The First Byte In   */
/* 	The Packet, And If For Some Reason The DMA Is Temporarialy halted   */
/* 	Before The Packet Is Completely Transfered, The Process May Get To  */
/* 	Run And Think The I/O Is Complete And Use The Old Status Values.    */

	i = 50000;
	while (i) {

		if (spp->sp_Messages[0] != 0xae)
			return (0);
		drv_usecwait(10);
		if (timeout)
			--i;
	}
	return (2);
}

/*
 * dpt_enable_interrupts
 * This function will re-enable interrupts to the system
 * returns 0 on success,
 * 1 for the dpt_wait function failed
 */
static int
dpt_enable_interrupts(register ushort port)
{

/* 	Make Sure That The Controller Is Not Busy. */

	if (dpt_wait(port + HA_AUX_STATUS, 0, HA_AUX_BUSY))
	return (1);

/* 	Set Up The enable Interrupts Modifier, 0 Enables The Interrupts */

	outb(port + HA_IMMED_MOD, CP_EI_MOD_ENABLE);

/* 	Set Up The Eata Immediate Enable/Disable Function (4) */

	outb(port + HA_IMMED_FUNC, CP_EI_INTERRUPTS);

/* 	Send Off The EATA Immediate Command */

	outb(port + HA_COMMAND, CP_EATA_IMMED);
	return (0);
}

/*
 * dpt_disable_interrupts
 * Disabling The Interrupts Is A Single Threaded Interface. The Caller
 * Should Make Sure That There Are No Other Out Standing Commands On
 * The Adapter Before Calling Or Else There Will Be Confusion As To
 * which Commands Are Completing. When We Go Into Disable Interrupt
 * Mode With No Out Standing Commands, Once We Send Off A Command The
 * Busy Bit Will Stay High Until The Command Has Completed, But The
 * Interrupt Bit Will No Longer Be Functional.
 * returns 0 on success,
 * 1 for the dpt_wait function failed
 * 2 for disable command failed
 */
static int
dpt_disable_interrupts(register ushort port)
{
	int i;

/* BUG: what the heck is a "phantom attach" ??? */
/*	Bail out if we have a phantom attach				*/
	if (inb(port + HA_AUX_STATUS) & HA_AUX_INTR)
		return (1);

/* We Are Assuming That There Aer No Out Standing Commands When This */
/* Was Called, But Let's Clear Any Pending Interrupts Anyway */

	inb(port + HA_STATUS);

/* 	Make Sure That The Controller Is Not Busy. */

	if ((dpt_wait(port + HA_AUX_STATUS, 0, HA_AUX_BUSY)))
	return (1);

/* 	Set Up The Diasble Interrupts Modifier, 1 Disables The Interrupts */

	outb(port + HA_IMMED_MOD, CP_EI_MOD_DISABLE);

	/* Set Up The Eata Immediate Enable/Disable Function (4) */

	outb(port + HA_IMMED_FUNC, CP_EI_INTERRUPTS);

/* 	Send Off The EATA Immediate Command */

	outb(port + HA_COMMAND, CP_EATA_IMMED);

/* Wait For Busy To Go Down, And Then Read The Status Register. If The */
/* Value returned Is An 0x50, This Is The Old Firmware And This Command */
/* Is Not Supported So Return A 2. If The Command Is Supported, A 0x52 */
/* Should Be Set In The Status Register Once Busy Goes Down. */

	dpt_wait(port + HA_AUX_STATUS, 0, HA_AUX_BUSY);
	i = inb(port + HA_STATUS);

/* 	If Bit 2 Is Set We Have A Winner */

	if (i & 0x02)
/***
 *** BUG: that's obviously bit-1 not bit-2 and the documentation says it's
 *** 	the interrupt pending bit not an interrupt mask bit
 ***/
		return (0);
	return (2);
}

/*
 * dpt_poll_disabled -- Execute a SCSI command
 * using no interrupts or command overlapping
 * returns 0 on success,
 * 1 for command terminated with error
 * 2 command could not be sent to controller
 * 3 busy did not go down
 * 4 wait_disabled failed
 */
static int
dpt_poll_disabled(register struct dpt_blk *dpt_blkp,
	register struct dpt_ccb *ccbp, int timeout)
{
	unsigned int port = dpt_blkp->db_ioaddr;
	int ret, enabled;
	struct dpt_stat *spp;

	ccbp->ccb_ctlrstat = HA_SELTO;

	enabled = dpt_disable_interrupts(port);

/* 	A 1 Returned From The Disable Means Busy Did Not Go Down, Error */

	if (enabled == 1)
		return (3);

/*	Put A Unique Value In The Lower Part Of The Status Pack That We */
/*	Can Poll On. When It Goes Away, The Command Will Be Complete    */

	spp = &dpt_blkp->db_stat;
	spp->sp_Messages[0] = 0xae;
	if (dpt_send(port, ccbp)) {
		return (2);
	}

/* Wait For The Command To Complete. If The Disable Failed, Call */
/* dpt_wait The Old Way, Otherwise Call dpt_wait_disabled */

	if (enabled) {
		ret = 1;
		while (ret) {
			ret = dpt_wait(port + HA_AUX_STATUS, HA_AUX_INTR, 0);
			if (timeout)
				break;
		}
	} else
		ret = dpt_wait_disabled(port, spp, timeout);

/* 	If The Wait Fails Return An Error */
	if (ret) {
		ret = 4;
/* 	The Command Completed OK, So Parse The Status */
	} else {
		ccbp->ccb_ctlrstat = spp->sp_hastat & HA_STATUS_MASK;
		ccbp->ccb_scsistat = spp->sp_scsi_stat;
		if (ccbp->ccb_ctlrstat || ccbp->ccb_scsistat)
			ret = 1;
		else
			ret = 0;
	}

/* Enable The Interrupts To The System On The Way Out If Disabled, */
/* Otherwise Clear The Interrupt On The Adapter */

	if (!enabled)
		dpt_enable_interrupts(port);
	else {
		inb(port + HA_STATUS);
	}
	return (ret);
}

/*
 *	Called when the system is being halted to disable all hardware
 *	interrupts.  Note that we *can't* block at all, not even on mutexes.
 */
/*ARGSUSED*/
static int
dpt_flush_cache(dev_info_t *dip, ddi_reset_cmd_t cmd)
{
	register struct  dpt_ccb *ccbp;
	caddr_t buf;
	struct	dpt_blk *dpt_blkp;
	struct  dpt *dptp;
	int lun;
	paddr_t	 local;
	scsi_hba_tran_t *tran;

	/* allocate a ccb for the flush */
	if (ddi_iopb_alloc(dip, (ddi_dma_lim_t *)0,
			(u_int) sizeof (struct dpt_ccb), &buf)) {
		cmn_err(CE_WARN,
			"dpt_flush_cache: unable to allocate memory\n");
		return (DDI_FAILURE);
	}

	ccbp = (struct dpt_ccb *) buf;
	bzero((caddr_t)ccbp, sizeof (*ccbp));
	ccbp->ccb_paddr	 = DPT_KVTOP(ccbp);
	ccbp->ccb_cdb[0] = FLUSH_CACHE_CMD;
	ccbp->ccb_optbyte = HA_AUTO_REQSEN;

/* 	auto request sense data physical address 			*/
	local = ccbp->ccb_paddr + ((caddr_t)(&ccbp->ccb_sense.sts_sensedata) -
					(caddr_t)ccbp);
	scsi_htos_long(ccbp->ccb_sensep, local);
	ccbp->ccb_senselen = DPT_SENSE_LEN;

	tran = (scsi_hba_tran_t *)ddi_get_driver_private(dip);
	if (!tran)
		return (DDI_FAILURE);

	dptp = TRAN2HBA(tran);
	if (!dptp)
		return (DDI_FAILURE);

	dpt_blkp = DPT_BLKP(dptp);

	scsi_htos_long(ccbp->ccb_statp, dpt_blkp->db_stat_paddr);

	/* loop for target ids 						*/
	for (ccbp->ccb_id = 0; ccbp->ccb_id <= dpt_blkp->db_max_target;
			ccbp->ccb_id++) {
		if (ccbp->ccb_id == dpt_blkp->db_targetid)
			continue;

		ccbp->ccb_msg0 = HA_IDENTIFY_MSG;
		for (lun = 0; lun <= dpt_blkp->db_max_lun;
			ccbp->ccb_msg0++, lun++) {

			if (dpt_poll_disabled(dpt_blkp, ccbp, 1)) {
				/* If no 1st lun, go on to next target */
				if (ccbp->ccb_ctlrstat == HA_SELTO)
					break;
			}
		}
	}

	ddi_iopb_free((caddr_t)ccbp);
	return (DDI_SUCCESS);
}

#ifdef DPT_DEBUG
void
dpt_dump_dptblk(struct dpt_blk *p)
{
	PRF("ha_stat %d scsi_stat 0x%x\n",
		p->db_stat.sp_hastat & HA_STATUS_MASK,
		p->db_stat.sp_scsi_stat & 0xff);
	PRF("dip 0x%x ioaddr 0x%x ccb_cnt %d ccbp 0x%x out %x\n",
		p->db_dip, p->db_ioaddr, p->db_ccb_cnt,
		p->db_ccbp, p->db_ccboutp);
}

char *dpt_err_strings[] = {
	"No Error",
	"Selection Timeout",
	"Command Timeout",
	"Scsi Bus Reset",
	"Ctrl in Powerup",
	"Unexpected Bus Phase",
	"Unexpected Bus Free",
	"Bus Parity Error",
	"Scsi Bus Hung",
	"Unexpected Msg Reject",
	"Reset Stuck",
	"Request Sense Fail",
	"Host Adaptor Parity Err",
	"Abort not Active",
	"CP Aborted on Bus",
	"CP Reset Not Active",
	"CP Reset on Scsi Bus",
	"Unknown Error State"
};

void
dpt_dump_ccb(struct dpt_ccb *p)
{
	int index;

	PRF("option 0x%x targ %d msg0 0x%x scsi_cmd 0x%x\n",
		p->ccb_option.b_byte & 0xff, p->ccb_id & 0xff,
		p->ccb_msg0 & 0xff, CCBP2GCMDP(p));

	PRF("lun %d", p->ccb_msg0 & 0x0f);
	if (p->ccb_msg0 & HA_IDENTIFY_MSG)
		PRF(" identify");
	if (p->ccb_msg0 & HA_DISCO_RICO)
		PRF(" disco");
	else
		PRF(" no disco");
	if (p->ccb_optbyte & HA_AUTO_REQSEN)
		PRF(" ARS");
	else
		PRF(" no ARS");
	if (p->ccb_optbyte & HA_DATA_OUT)
		PRF(" xfer out");
	else
		PRF(" xfer in");
	if (p->ccb_optbyte & HA_SCATTER)
		PRF(" scatter gather list\n");
	else
		PRF(" no scatter");

	if (p->ccb_optbyte & HA_SCATTER) {
		for (index = 0; index < DPT_MAX_DMA_SEGS; index++) {
		if (!(scsi_stoh_long((ulong)p->ccb_sg_list[index].data_addr)))
				break;
			PRF(" a:0x%x l:0x%x",
		scsi_stoh_long((ulong)p->ccb_sg_list[index].data_addr),
		scsi_stoh_long((ulong)p->ccb_sg_list[index].data_len));
		}
	} else {
		PRF(" Data ptr 0x%x data len 0x%x",
		scsi_stoh_long(*(ulong *)p->ccb_datap),
		scsi_stoh_long(*(ulong *)p->ccb_datalen));
	}

	index = p->ccb_ctlrstat & 0xff;
	if (index < 0 || index > DPT_CPRESETONBUS)
		index = DPT_UNKNOWN_ERROR;
	PRF("\nctlr stat %s scsi stat 0x%x\n", dpt_err_strings[index],
	p->ccb_scsistat & 0xff);
}
#endif
