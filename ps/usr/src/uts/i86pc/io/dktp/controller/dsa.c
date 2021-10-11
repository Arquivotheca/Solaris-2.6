/*
 * Copyright (c) 1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)dsa.c	1.11	96/08/13 SMI"


#include <sys/scsi/scsi.h>
#include <sys/dktp/dadev.h>
#include <sys/dktp/hba.h>
#include <sys/dktp/dadkio.h>
#include <sys/dktp/dsa.h>
#include <sys/dktp/tgdk.h>
#include <sys/dktp/bmic.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/dkio.h>
#include <sys/scsi/generic/inquiry.h>
#include <sys/debug.h>

/*
 * External references
 */

/*
 * Local Function Prototypes
 */
static int dsa_initchild(dev_info_t *mdip, dev_info_t *cdip);
static int dsa_propinit(struct dsa_blk *dsa_blkp);
static int dsa_pollret(struct dsa_blk *dsa_blkp, struct dsa_cmpkt *dsa_pktp);
static u_int dsa_dummy_intr(caddr_t arg);
static u_int dsa_intr(caddr_t arg);

static struct cmpkt *dsa_pktalloc(struct dsa *dsap, int (*callback)(), 
		caddr_t arg);
static struct cmpkt *dsa_memsetup(struct dsa *dsap, struct cmpkt *pktp, 
	struct buf *bp, int (*callback)(), caddr_t arg);
static struct cmpkt *dsa_iosetup(struct dsa *dsap, struct cmpkt *pktp);
static u_int dsa_xlate_vec(register struct  dsa_blk *dsa_blkp);

static void dsa_pktfree(struct dsa *dsap, struct cmpkt *pktp);
static void dsa_memfree(struct dsa *dsap, struct cmpkt *pktp);
static int dsa_transport(struct dsa *dsap, struct cmpkt *pktp);
static int dsa_abort(struct dsa *dsap, struct cmpkt *pktp);
static int dsa_reset(struct dsa *dsap, int level);
static int dsa_ioctl(struct dsa *dsap, int cmd, int arg, int flag);
static int dsa_findctl(ushort ioaddr);
static int dsa_try_insem(ushort sem0_ioaddr, uint timeout);
static int dsa_init_cmd(ushort ioaddr, struct dsa_mboxes *mboxp);
static void dsa_fake_inquiry (struct dsarpbuf *rpbp, struct scsi_inquiry *inqp);
static void dsa_dump_mboxes(char *);
static int dsa_send_lcmd(struct dsa_blk *dsa_blkp, struct dsa_cmpkt *pktp);
static void dsa_reset_tos(struct dsa_blk *dsa_blkp, struct dsa_cmpkt *dsa_pktp);
static void dsa_chkstatus(uint cmd_stat, uint xfer_count, struct dsa_cmpkt *dsa_pktp);

static struct ctl_objops dsa_objops = {
	dsa_pktalloc,
	dsa_pktfree,
	dsa_memsetup,
	dsa_memfree,
	dsa_iosetup,
	dsa_transport,
	dsa_reset,
	dsa_abort,
	nulldev,
	nulldev,
	dsa_ioctl,
	0, 0
};

/*
 * Local static ddsa
 */
static int dsa_pgsz = 0;
static int dsa_pgmsk;
static int dsa_pgshf;

static int dsa_cb_id = 0;
static caddr_t dsaccb = (caddr_t) 0;
static kmutex_t dsa_global_mutex;
static int dsa_global_init = 0;

static ddi_dma_lim_t dsa_dma_lim = {
	0,		/* address low				*/
	0xffffffff,	/* address high				*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	DMALIM_VER0,	/* version				*/
	0xffffffff,	/* address register			*/
	0x0000ffff,	/* counter register XXX: ctreg_max	*/
	512,		/* sector size: granular		*/
	DSA_DMAMAX,	/* scatter/gather list length: sgllen	*/
	0x00100000	/* request size	XXX: reqsize		*/ 
};
/* Note that sgllen is read from the controller into dlim_sgllen */

#ifdef	DSA_DEBUG
#define	DENT	0x0001
#define	DPKT	0x0002
#define	DIO	0x0004
#define DINIT	0x0004
#define DPOLL	0x0008
#define DTST	0x0010
static	int	dsa_debug = 0;

/* logical commands */
static char *dsa_logical_cmds[] = {
	"recalibrate",					/* 0x00 */
	"read",						/* 0x01 */
	"write",					/* 0x02 */
	"verify",					/* 0x03 */
	"seek",						/* 0x04 */
	"verify guard",					/* 0x05 */
	"read no xfer",					/* 0x06 */
	"read buffer diag",				/* 0x07 */
	"write buffer diag",				/* 0x08 */
	"write and verify",				/* 0x09 */
	"composite unit info",				/* 0x0a */
	"read buffer with crc",				/* 0x0b */
	"write buffer with crc",			/* 0x0c */
	"read next log entry",				/* 0x0d */
	"read with scatter gather",			/* 0x0e */
	"write with scatter gather",			/* 0x0f */
	"init event log",				/* 0x10 */
	"flush event log",				/* 0x11 */
	"native remap block",				/* 0x12 */
	"synch write",					/* 0x13 */
	"write verify with scatter gather",		/* 0x14 */
	"synch write and verify",			/* 0x15 */
	"synch write with scatter gather",		/* 0x16 */
	"synch write verify with scatter gather",	/* 0x17 */
	"byte read with scatter gather",		/* 0x18 */
	"byte write with scatter gather",		/* 0x19 */
	"byte write verify with scatter gather",	/* 0x1a */
	"byte sync write with scatter gather",		/* 0x1b */
	"byte sync write verify with scatter gather",	/* 0x1c */
	"synch flush all requests on drive",		/* 0x1d */
	"read next physical errlog entry",		/* 0x1e */
	"init physical errlog entry",			/* 0x1f */
	"read next ctlr errlog entry",			/* 0x20 */
	"init ctlr errlog entry",			/* 0x21 */
	"convert phys drive to pun-spare",		/* 0x22 */
	"quiesce physical device",			/* 0x23 */
	"scan for new devices"				/* 0x24 */
};

/* extended commands */
static char *dsa_ext_cmds[] = {
	"reserved",					/* 0x00 */
	"run diagnostics",				/* 0x01 */
	"get firmware revision",			/* 0x02 */
	"get logical handles",				/* 0x03 */
	"get physical drive config",			/* 0x04 */
	"set composite parms",				/* 0x05 */
	"get composite drives",				/* 0x06 */
	"get composite drive capacity",			/* 0x07 */
	"reserved ",					/* 0x08 */
	"get controller status",			/* 0x09 */
	"begin restore",				/* 0x0a */
	"get rebuild progress",				/* 0x0b */
	"reserved ",					/* 0x0c */
	"reserved ",					/* 0x0d */
	"set date and time",				/* 0x0e */
	"get hardware config",				/* 0x0f */
	"set ping interval",				/* 0x10 */
	"reserved ",					/* 0x11 */
	"reserved ",					/* 0x12 */
	"reserved ",					/* 0x13 */
	"get cache hit rate",				/* 0x14 */
	"reset cache stats",				/* 0x15 */
	"get request counts on composite drives",	/* 0x16 */
	"get misc info on composite drives",		/* 0x17 */
	"get error count on physical drive",		/* 0x18 */
	"reserved ",					/* 0x19 */
	"get physical disk geometry",			/* 0x1a */
	"set mode of option ram",			/* 0x1b */
	"reserved ",					/* 0x1c */
	"get drive i/o bus config",			/* 0x1d */
	"reserved ",					/* 0x1e */
	"get compisite drive info",			/* 0x1f */
	"do restore",					/* 0x20 */
	"control rebuilds",				/* 0x21 */
	"get physical drive info",			/* 0x22 */
};

/* drive array power up status errors */
static char *dsa_pup_errs[] = {
	"controller died",
	"normal",
	"no configuration (virgin)",
	"bad drive configuration",
	"new drive - recovery possible",
	"drive failed - correctable",
	"drive failed - uncorrectable",
	"no drives attached",
	"more drives than expected",
	"maintain mode",
	"manufacturing mode",
	"new - needs remap generated",
	"same as PUP_NEW with rebuild",
	"same as PUP_NEW but correctble",
	"no drive configuration"
};

#endif	/* DSA_DEBUG */

/*
 * 	bus nexus operations.
 */

static int
dsa_bus_ctl(dev_info_t *d, dev_info_t *r, ddi_ctl_enum_t o, void *a, void *v);

static struct bus_ops dsa_bus_ops = {
#if _SOLARIS_PS_RELEASE >= 250
	BUSO_REV,
#endif
	nullbusmap,
	0,	/* ddi_intrspec_t	(*bus_get_intrspec)(); */
	0,	/* int		(*bus_add_intrspec)(); */
	0,	/* void		(*bus_remove_intrspec)(); */
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
	dsa_bus_ctl,
	ddi_bus_prop_op,
};


static int dsa_identify(dev_info_t *dev);
static int dsa_probe(dev_info_t *);
static int dsa_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int dsa_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);
static struct dev_ops	dsa_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	dsa_identify,		/* identify */
	dsa_probe,		/* probe */
	dsa_attach,		/* attach */
	dsa_detach,		/* detach */
	nulldev,		/* reset */
	(struct cb_ops *)0,	/* driver operations */
	&dsa_bus_ops		/* bus operations */
};

/*
 * This is the driver loadable module wrapper.
 */
char _depends_on[] = "misc/dadk misc/strategy";

#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"Dell Scsi Disk Array Controller Driver",/* Name of the module. */
	&dsa_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};


#define	CMPKT	dsa_pktp->dc_pkt
#define	RWCMDP	((struct dadkio_rwcmd *)(dsa_pktp->dc_pkt.cp_bp->b_back))

int
_init(void)
{
	int	status;

	status = mod_install(&modlinkage);
	if (!status)
		mutex_init(&dsa_global_mutex, "Dell DSA global Mutex", 
			MUTEX_DRIVER, (void *)NULL);
	return status;
}

int
_fini(void)
{
	int	status;

	status = mod_remove(&modlinkage);
	if (!status)
		mutex_destroy(&dsa_global_mutex);
	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}


/*ARGSUSED*/
static int
dsa_bus_ctl(dev_info_t *d, dev_info_t *r, ddi_ctl_enum_t o, void *a, void *v)
{

	switch (o) {
	case DDI_CTLOPS_REPORTDEV:
	{
		struct 	ctl_obj *cltobjp;

		cltobjp = (struct ctl_obj *) ddi_get_driver_private(r);
		cmn_err(CE_CONT, "?%s%d at %s%d",
			ddi_get_name(r), ddi_get_instance(r),
			ddi_get_name(d), ddi_get_instance(d));
		cmn_err(CE_CONT, "? target %d lun %d\n", 
			CTL_GET_TARG(cltobjp), 0);
		return DDI_SUCCESS;
	}
	case DDI_CTLOPS_INITCHILD:
		return(dsa_initchild(d,a));

	case DDI_CTLOPS_UNINITCHILD:
	{
		register struct scsi_device *devp;
		register dev_info_t *dip = (dev_info_t *)a;
		struct ctl_obj	*ctlobjp;
		struct dsa	*dsap;

		devp = (struct scsi_device *) ddi_get_driver_private(dip);
		ctlobjp = (struct ctl_obj *) devp->sd_address.a_hba_tran;
		dsap =  (struct dsa *) ctlobjp->c_data;

		if (devp != (struct scsi_device *) 0) {
			kmem_free((caddr_t) devp, (sizeof(*devp) +
				+ sizeof (struct dsa) + sizeof(*ctlobjp)
				+ sizeof(struct dsa_unit)));
			mutex_enter (&dsap->d_blkp->db_mutex); 
			dsap->d_blkp->db_refcount--;
			mutex_exit (&dsap->d_blkp->db_mutex);
		}
		ddi_set_driver_private(dip, NULL);
		ddi_set_name_addr(dip, NULL);
		return DDI_SUCCESS;
	}

	default:
		cmn_err(CE_CONT, "%s%d: invalid op (%d) from %s%d\n",
			ddi_get_name(d), ddi_get_instance(d),
			o, ddi_get_name(r), ddi_get_instance(r));
		return (ddi_ctlops(d, r, o, a, v));
	}

}


static int
dsa_initchild(dev_info_t *mdip, dev_info_t *cdip)
{
	int 	len;
	int 	targ;
	int	lun;
	struct 	scsi_device *devp;
	char 	name[MAXNAMELEN];
	char	buf[40];
	struct	ctl_obj *ctlobjp;
	struct	dsa *dsap;
	struct	dsa_unit *dsa_unitp;
	struct	dsa_blk *dsa_blkp;

	len = sizeof (int);
	/*
	 * check LUNs first.  there are more of them.
	 * If lun isn't specified, assume 0
	 * If a lun other than 0 is specified, fail it now.
	 */
	if (HBA_INTPROP(cdip, "lun", &lun, &len) != DDI_SUCCESS) 
		lun = 0;
	if (lun != 0) 	/* no support for lun's				*/
		return DDI_NOT_WELL_FORMED;
	if (HBA_INTPROP(cdip, "target", &targ, &len) != DDI_SUCCESS)
		return DDI_NOT_WELL_FORMED;


	dsa_blkp = (struct dsa_blk *)ddi_get_driver_private(mdip);
	if (dsa_blkp->db_rpbp[targ] == (struct dsarpbuf *)0) {
		return (DDI_NOT_WELL_FORMED);
	}

#ifdef DSA_DEBUG
	if (dsa_debug & DENT) {
		PRF("dsa_initchild processing target %x lun %x\n", targ, lun);
	}
#endif
	if (!(devp = (struct scsi_device *)kmem_zalloc(
		(sizeof(*devp) + sizeof(*ctlobjp) +
		sizeof(*dsap) + sizeof(*dsa_unitp)), KM_NOSLEEP)))
		return DDI_NOT_WELL_FORMED;

	ctlobjp   = (struct ctl_obj *)(devp+1);
	dsap      = (struct dsa *)(ctlobjp+1);
	dsa_unitp = (struct dsa_unit *)(dsap+1);
	
	devp->sd_inq   		 = dsa_blkp->db_inqp[targ]; 
	devp->sd_dev   		 = cdip;
	devp->sd_address.a_hba_tran = (struct scsi_hba_tran *)ctlobjp;
	devp->sd_address.a_target = (u_short) targ;
	devp->sd_address.a_lun    = (u_char)lun;

	ctlobjp->c_ops  = (struct ctl_objops *) &dsa_objops;
	ctlobjp->c_data = (opaque_t) dsap;
	ctlobjp->c_ext  = &(ctlobjp->c_extblk);
	ctlobjp->c_extblk.c_ctldip = mdip;
	ctlobjp->c_extblk.c_devdip = cdip;
	ctlobjp->c_extblk.c_targ   = targ;
	ctlobjp->c_extblk.c_blksz = NBPSCTR;

	dsap->d_blkp = dsa_blkp;
	dsap->d_unitp = dsa_unitp;
	dsap->d_ctlobjp = ctlobjp;

	/* copy in template static dma_lim_t, then modify to suit 	*/
	dsa_unitp->du_lim = dsa_dma_lim; 
	dsa_unitp->du_lim.dlim_sgllen = 
				dsa_blkp->db_max_sglen;

	/* use the temporary "target" as holder for drive unit 		*/
	dsa_unitp->du_drive_unit = targ;
	dsa_unitp->du_rpbuf = dsa_blkp->db_rpbp[targ];
	dsa_unitp->du_cap = dsa_unitp->du_rpbuf->dsarp_cap;
	dsa_unitp->du_heads = dsa_unitp->du_rpbuf->dsarp_heads;
	dsa_unitp->du_sectors = dsa_unitp->du_rpbuf->dsarp_sectors;
	dsa_unitp->du_cylinders = dsa_unitp->du_rpbuf->dsarp_cylinders;
	dsa_unitp->du_secs_per_track = dsa_unitp->du_rpbuf->dsarp_secs_per_track;

	strcpy(buf, "sd mutex ");
	sprintf(name, "dsa:%d:%d", ddi_get_instance(mdip), targ);
	(void) strcat(&buf[strlen(buf)], name);
	mutex_init(&devp->sd_mutex, buf, MUTEX_DRIVER, (void *)NULL);

	sprintf(name, "%d,%d", targ, lun);
	ddi_set_name_addr(cdip, name);
	ddi_set_driver_private(cdip, (caddr_t)devp);

	mutex_enter (&dsa_blkp->db_mutex); 
	dsa_blkp->db_refcount++;
	mutex_exit (&dsa_blkp->db_mutex);

#ifdef DSA_DEBUG
	if (dsa_debug & DENT) {
		PRF("dsa_initchild: <%d> devp= 0x%x ctlobjp=0x%x\n",
			targ, devp, ctlobjp);
	}
#endif
	return DDI_SUCCESS;
}

static int
dsa_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, "dsa") == 0) 
		return DDI_IDENTIFIED;
	else 
		return DDI_NOT_IDENTIFIED;
}


static int
dsa_probe(register dev_info_t *devi)
{
	int	ioaddr;
	int	drives_found;
	int	len;

	len = sizeof(int);
	if ((HBA_INTPROP(devi, "ioaddr", &ioaddr, &len) != DDI_SUCCESS) ||
	    ((drives_found = dsa_findctl((ushort)ioaddr)) == 0))
		return DDI_PROBE_FAILURE;

	ddi_set_driver_private(devi, (caddr_t) drives_found);
	return DDI_PROBE_SUCCESS;
}

static int 
dsa_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	register struct	dsa_blk *dsa_blkp;

	switch (cmd) {
	case DDI_DETACH:
	{
		dsa_blkp = (struct dsa_blk *)ddi_get_driver_private(devi);
		if (!dsa_blkp) 
			return DDI_SUCCESS;

		ddi_remove_intr(devi,dsa_xlate_vec(dsa_blkp),
			(ddi_iblock_cookie_t)dsa_blkp->db_lkarg);
		mutex_destroy(&dsa_blkp->db_mutex);

		mutex_enter(&dsa_global_mutex);
		dsa_global_init--; 
		mutex_exit(&dsa_global_mutex);

		kmem_free((caddr_t)dsa_blkp, sizeof(*dsa_blkp));

		ddi_prop_remove_all(devi);
		ddi_set_driver_private(devi, (caddr_t) NULL);
		return DDI_SUCCESS;
	}
	default:
		return EINVAL;
	}
}

/*ARGSUSED*/
static int
dsa_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	struct dsa_blk		*dsa_blkp;
	int			drives_found;
	int 			drive;
	ddi_iblock_cookie_t	tmp;
	u_int			intr_idx;

	/*
	 * probe put this here for us
	 * now we've read it we don't need the private area any more
	 */
	drives_found = (int) ddi_get_driver_private(devi);

	dsa_blkp = (struct dsa_blk *) kmem_zalloc((unsigned)sizeof(*dsa_blkp), 
			KM_NOSLEEP);
	if (!dsa_blkp)
		return DDI_FAILURE;

	dsa_blkp->db_dip = devi;
	if ((dsa_propinit(dsa_blkp) == DDI_FAILURE) ||
	    (dsa_getedt(dsa_blkp, drives_found) == DDI_FAILURE)) {
		(void)kmem_free((caddr_t)dsa_blkp, sizeof(*dsa_blkp));
		return DDI_FAILURE;
	}

	intr_idx = dsa_xlate_vec(dsa_blkp);
/*
 *	Establish initial dummy interrupt handler 
 *	get iblock cookie to initialize mutexes used in the 
 *	real interrupt handler
 */
	if (ddi_add_intr(devi, intr_idx, &tmp, (ddi_idevice_cookie_t *) 0, 
		dsa_dummy_intr, (caddr_t) dsa_blkp)){
		(void)kmem_free((caddr_t)dsa_blkp, sizeof(*dsa_blkp));
		cmn_err(CE_WARN, "dsa_attach: cannot add intr");
		return DDI_FAILURE;
	}
	dsa_blkp->db_lkarg = (void *)tmp;

	mutex_init(&dsa_blkp->db_mutex, "dsa mutex", MUTEX_DRIVER, (void *)tmp);

	ddi_remove_intr(devi, intr_idx, tmp);
/*	Establish real interrupt handler				*/
	if (ddi_add_intr(devi, intr_idx, &tmp, (ddi_idevice_cookie_t *) 0, 
		dsa_intr, (caddr_t) dsa_blkp)) {
		(void)kmem_free((caddr_t)dsa_blkp, sizeof(*dsa_blkp));
		cmn_err(CE_WARN, "dsa_attach: cannot add intr");
		return DDI_FAILURE;
	}

	mutex_enter(&dsa_global_mutex);	/* protect multithreaded attach	*/
	dsa_global_init++; 
	dsa_blkp->db_numdev = drives_found;
	dsa_blkp->db_refcount = 0;
#ifdef DSA_DEBUG
	dsa_blkp->db_pkts_out = 0;
#endif
	mutex_exit(&dsa_global_mutex);

	ddi_set_driver_private(devi, (caddr_t) dsa_blkp);
	ddi_report_dev(devi);

/*	enable interrupts on the controller */
	outb(dsa_blkp->db_ioaddr | BMIC_SYS_INT_ENABLE, 0x01);

/*	enable interrupt outb (0x0e, 0x10) (write to BMIC doorbell) */
	outb(dsa_blkp->db_ioaddr | BMIC_EISA_DOOR_EN, NTV_LOGICAL_DOORBELL);
	/* turn on only logical command interrupts, not extended */

	return DDI_SUCCESS;
}


/*
 *	Common controller object interface
 */
struct cmpkt *
dsa_pktalloc(register struct dsa *dsap, int (*callback)(), caddr_t arg)
{
	register struct dsa_cmpkt *dsa_pktp;
	int 	 kf;

#ifdef DSA_DEBUG
	if (dsa_debug & DENT) {
		PRF("dsa_pktalloc (%x, %x, %x)\n", dsap, callback, arg);
	}
#endif
	kf = GDA_KMFLAG(callback); /* determines whether or not we sleep */
	dsa_pktp = (struct dsa_cmpkt *) 0;
	dsa_pktp = (struct dsa_cmpkt *)kmem_zalloc(sizeof(*dsa_pktp), kf);
	
	if (!dsa_pktp) {
		if (callback != DDI_DMA_DONTWAIT) 
			ddi_set_callback(callback, arg, &dsa_cb_id);
		return ((struct cmpkt *)NULL);
	}

	dsa_pktp->dc_pkt.cp_cdblen = 1;
	dsa_pktp->dc_pkt.cp_cdbp   = (opaque_t)&dsa_pktp->dc_cdb;
	dsa_pktp->dc_pkt.cp_scbp   = (opaque_t)&dsa_pktp->dc_scb;
	dsa_pktp->dc_pkt.cp_scblen = 1;
	dsa_pktp->dc_pkt.cp_ctl_private = (opaque_t)dsap;

/* 	use target as drive number					*/
	dsa_pktp->dc_drive_unit = dsap->d_ctlobjp->c_extblk.c_targ;

#ifdef DSA_DEBUG
	if (dsa_debug & DPKT) {
		PRF("dsa_pktalloc:dsa_pktp = 0x%x\n", dsa_pktp);
	}
#endif
	return ((struct cmpkt *) dsa_pktp);
}

void 
dsa_pktfree(struct dsa *dsap, struct cmpkt *pktp)
{
	register struct dsa_cmpkt *cmdp = (struct dsa_cmpkt *) pktp;

	if (cmdp->dc_flags & HBA_CFLAG_FREE) {
		cmn_err (CE_WARN, "dsa_pktfree: freeing free packet");
	}
	cmdp->dc_flags = HBA_CFLAG_FREE;

	kmem_free((caddr_t) cmdp, sizeof(*cmdp));
	if (dsa_cb_id) 
		ddi_run_callback(&dsa_cb_id);
}

/* free dma handle and controller handle allocated in memsetup */
void 
dsa_memfree(struct dsa *dsap, struct cmpkt *pktp)
{
	if(((struct dsa_cmpkt *)pktp)->dc_dma_handle)
		ddi_dma_free(((struct dsa_cmpkt *)pktp)->dc_dma_handle);
}

/* assumes that this function is called once per bp */
struct cmpkt *
dsa_memsetup(struct dsa *dsap, struct cmpkt *pktp, 
	struct buf *bp, int (*callback)(), caddr_t arg)
{
	register struct dsa_blk		*dsa_blkp = dsap->d_blkp;
	register struct dsa_cmpkt *dsa_pktp = (struct dsa_cmpkt *)pktp;
	int 	status, flags;

/* 	check direction for data transfer 				*/
	if (bp->b_flags & B_READ) 
		flags = DDI_DMA_READ;
	else 
		flags = DDI_DMA_WRITE;

	status = ddi_dma_buf_setup(CTL_DIP_DEV(dsap->d_ctlobjp), bp, flags, 
			callback, arg, &dsap->d_unitp->du_lim,
			&dsa_pktp->dc_dma_handle);

	if (status) {
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
#ifdef DSA_DEBUG
		if(dsa_debug & DTST)
			PRF("dsa_memsetup fail for bp->b_bcount 0x%x\n",
				bp->b_bcount);
#endif
		return ((struct cmpkt *) 0);
	}

/*	enable first call to ddi_dma_nextwin				*/
	dsa_pktp->dc_dmawin = (ddi_dma_win_t) 0;

#ifdef DSA_DEBUG
		if(dsa_debug & DTST)
			PRF("dsa_memsetup pkt %x bcount 0x%x resid %x\n",
				dsa_pktp, bp->b_bcount,  pktp->cp_resid );
#endif

	return pktp;
}

struct cmpkt *
dsa_iosetup(struct dsa *dsap, struct cmpkt *pktp)
{
	register struct dsa_blk		*dsa_blkp = dsap->d_blkp;
	register struct dsa_cmpkt *dsa_pktp = (struct dsa_cmpkt *)pktp;
	int	status, num_segs = 0;
	ddi_dma_cookie_t cookie;
	off_t off, len, coff, clen, dsacmd, lcmd;
	int	bytes_xfer = 0;

	if (CMPKT.cp_passthru) {
		switch (RWCMDP->cmd) {
		case DADKIO_RWCMD_READ:
		case DADKIO_RWCMD_WRITE:
			dsacmd =
			(RWCMDP->cmd == DADKIO_RWCMD_READ) ? NTV_BREADSCATT :
					NTV_BWRITEGATH;
			lcmd =
			(RWCMDP->cmd == DADKIO_RWCMD_READ) ? DCMD_READ :
					DCMD_WRITE;

			coff = RWCMDP->blkaddr;
			clen = RWCMDP->buflen;
			break;
		default:
			cmn_err(CE_WARN, "dsa_iosetup:bad command %x\n"
					, RWCMDP->cmd);
			return (pktp);
		}
	} else {
		switch (lcmd = dsa_pktp->dc_cdb) {
		case DCMD_READ:
			dsacmd = NTV_BREADSCATT;
			goto offsets;
		case DCMD_WRITE:
			dsacmd = NTV_BWRITEGATH;
		offsets:
			coff =  pktp->cp_srtsec;
			clen =  pktp->cp_bytexfer;
		}
	}

	switch (lcmd) {
	case DCMD_READ:
	case DCMD_WRITE:

		if (dsa_pktp->dc_error & DSA_TRANS_ERROR) {
#ifdef DSA_DEBUG
				if (dsa_debug & DPKT)
					PRF("iosetup error fast return\n");
#endif
			dsa_pktp->dc_error = 0;
			pktp->cp_reason = 0;
			dsa_pktp->dc_scb = 0;
			return (pktp);
		}

		nextwin:
		if (dsa_pktp->dc_dmawin == (ddi_dma_win_t)0) {

			if (ddi_dma_nextwin(dsa_pktp->dc_dma_handle,
				dsa_pktp->dc_dmawin,
				&dsa_pktp->dc_dmawin) != DDI_SUCCESS) {
#ifdef DSA_DEBUG
				if (dsa_debug & DTST)
					PRF("iosetup nextwin fail\n", status);
#endif
					return ((struct cmpkt *) 0);
			}
			dsa_pktp->dc_dmaseg = (ddi_dma_seg_t)0;
		}

		do {
			status = ddi_dma_nextseg(dsa_pktp->dc_dmawin,
				dsa_pktp->dc_dmaseg, &dsa_pktp->dc_dmaseg);

			if (status == DDI_DMA_STALE) {
#ifdef DSA_DEBUG
				if (dsa_debug & DTST)
					PRF("iosetup nextseg stale\n");
#endif
				return ((struct cmpkt *)0);
			}

			if (status == DDI_DMA_DONE) {
#ifdef DSA_DEBUG
				if (dsa_debug & DTST)
					PRF("iosetup nextseg done\n");
#endif
				dsa_pktp->dc_dmawin = (ddi_dma_win_t)0;
				if (num_segs == 0)
					goto nextwin;
				break;
			}

			ddi_dma_segtocookie(dsa_pktp->dc_dmaseg, &off, &len,
						&cookie);
			dsa_pktp->dc_sg_list[num_segs].data_ptr =
						cookie.dmac_address;
			dsa_pktp->dc_sg_list[num_segs].data_len =
						cookie.dmac_size;
			bytes_xfer += cookie.dmac_size;
#ifdef DSA_DEBUG
			if (dsa_debug & DTST)
				PRF("iosetup %d size %x status %x\n", num_segs,
				cookie.dmac_size, status);
#endif
			num_segs++;

		} while ((num_segs < dsap->d_unitp->du_lim.dlim_sgllen) &&
				(bytes_xfer < clen));


#ifdef DSA_DEBUG
			if (dsa_debug & DTST)
				PRF("iosetup xfer 0x%x bytes\n", bytes_xfer);
#endif

		pktp->cp_resid = pktp->cp_bytexfer = bytes_xfer;
		dsa_pktp->dc_command = dsacmd;

/*		drive_unit was set in pktalloc				*/

		dsa_pktp->dc_bytes = num_segs;
/*		handle was set in dsa_memsetup				*/
		dsa_pktp->dc_block = coff;
		dsa_pktp->dc_paddr = DSA_KVTOP(&dsa_pktp->dc_sg_list);
		dsa_pktp->dc_vaddr = (char *) &dsa_pktp->dc_sg_list;
		break;

	case DCMD_SEEK:
	case DCMD_RECAL:
		break;
	default:
		cmn_err(CE_WARN,"dsa_iosetup:bad command %x\n",
				dsa_pktp->dc_cdb);
		break;
	}

	return (pktp);
}

/* Caller must hold dsa_blkp->db_mutex */
static void
dsa_reset_tos(register struct dsa_blk *dsa_blkp, register struct dsa_cmpkt *dsa_pktp) 
{

/*	return the handle to the pool and reset tos		*/
	dsa_blkp->db_pktp[dsa_pktp->dc_handle] = (struct dsa_cmpkt *)
		dsa_blkp->db_tos_handle;

	dsa_blkp->db_tos_handle = dsa_pktp->dc_handle;
}

#ifdef DSA_DEBUG
int dsa_failures = 0;
#endif

int 
dsa_transport(struct dsa *dsap, struct cmpkt *pktp)
{
	struct dsa_blk		*dsa_blkp = dsap->d_blkp;
	struct dsa_cmpkt	*dsa_pktp = (struct dsa_cmpkt *)pktp;

	dsa_pktp->dc_error = 0;

	mutex_enter (&dsa_blkp->db_mutex);

/* 	look for a free handle on the controller 			*/
	if (dsa_blkp->db_tos_handle == -1) {
		mutex_exit (&dsa_blkp->db_mutex);
#ifdef DSA_DEBUG
		if (dsa_debug & DTST)
			PRF("dsa_transport: No handle\n");
		dsa_failures++;
#endif
/*		turn on error flag for use by iosetup in retry		*/
		dsa_pktp->dc_error |= DSA_TRANS_ERROR;
		return CTL_SEND_FAILURE;
	}

/* 	get the free handle off the top of stack 			*/
	dsa_pktp->dc_handle = dsa_blkp->db_tos_handle;

/* 	set new top of stack 						*/
	dsa_blkp->db_tos_handle = 
		((int )(dsa_blkp->db_pktp[dsa_blkp->db_tos_handle]));

/*	save the address of the packet 					*/
	dsa_blkp->db_pktp[dsa_pktp->dc_handle] = dsa_pktp;


	if (dsa_send_lcmd(dsa_blkp, dsa_pktp)) {
		mutex_exit(&dsa_blkp->db_mutex);
#ifdef DSA_DEBUG
		if(dsa_debug & DTST)
			PRF("send_lcmd fail: should retry\n");
		dsa_failures++;
#endif
/*		turn on error flag for use by iosetup in retry		*/
		dsa_pktp->dc_error |= DSA_TRANS_ERROR;
		return CTL_SEND_FAILURE;
	}

#ifdef DSA_DEBUG
	if(dsa_debug & DTST)
		dsa_blkp->db_pkts_out++;
#endif

	if (!pktp->cp_flags & CPF_NOINTR) {
		mutex_exit(&dsa_blkp->db_mutex);
		return CTL_SEND_SUCCESS;
	}

/*	pollret releases the mutex 				*/
	if ( dsa_pollret (dsa_blkp, dsa_pktp)) {
		pktp->cp_reason = CPS_CHKERR;
	} 

	return CTL_SEND_SUCCESS;
}

/* abort not implemented in the hardware */
int 
dsa_abort(struct dsa *dsap, struct cmpkt *pktp)
{
#ifdef DSA_DEBUG
	if (dsa_debug & DENT) {
		PRF("dsa_abort (%x, %x)\n", dsap, pktp);
	}
#endif
	return TRUE;
}

/* reset not implemented in the hardware */
int 
dsa_reset(struct dsa *dsap, int level)
{
	register struct dsa_blk	*dsa_blkp = dsap->d_blkp;

#ifdef DSA_DEBUG
	if (dsa_debug & DENT) {
		PRF("dsa_reset (%x, %d)\n", dsap, level);
	}
#endif
	return TRUE;
}

int 
dsa_ioctl(struct dsa *dsap, int cmd, int arg, int flag)
{
	struct scsi_inquiry *inqp;
#ifdef DSA_DEBUG
	if (dsa_debug & DENT) {
		PRF("dsa_ioctl (%x, %x, %x, %x)\n", dsap, cmd, arg, flag);
	}
#endif
	switch (cmd)
	{
#ifdef NOT_YET
	case DKIOC_INQUIRY:
		{
			int target;

			target = dsap->d_unitp->du_drive_unit;

			inqp = dsap->d_blkp->db_inqp[target]; 
			if(inqp) {

				if(ddi_copyout((caddr_t) inqp,
			    	(caddr_t) arg, SUN_INQSIZE, flag))
					return EFAULT;
				else
					return 0;
			} else 
				return ENXIO;
		}
		break;
#endif
	case DIOCTL_GETGEOM:
		{
			struct tgdk_geom	*tg = (struct tgdk_geom *)arg;
			struct dsa_unit	*unitp = dsap->d_unitp;

			tg->g_cyl = unitp->du_cylinders;
			tg->g_acyl = 2;
			tg->g_head = unitp->du_heads;
			tg->g_sec = unitp->du_sectors;
			tg->g_secsiz = NBPSCTR;
			tg->g_cap = unitp->du_cap;
		}
		break;
	case DIOCTL_GETPHYGEOM:
		{
			struct tgdk_geom	*tg = (struct tgdk_geom *)arg;
			struct dsa_unit	*unitp = dsap->d_unitp;

			tg->g_cyl = unitp->du_cylinders;
			tg->g_acyl = 2;
			tg->g_head = unitp->du_heads;
			tg->g_sec = unitp->du_sectors;
			tg->g_secsiz = NBPSCTR;
			tg->g_cap = unitp->du_cap;
		}
		break;
	default:
		return ENOTTY;
	}
	return 0;
}


/* ==========================================================================
 *
 *	controller dependent funtions
 */
static int
dsa_propinit(register struct dsa_blk *dsa_blkp)
{
	register dev_info_t *devi;
	register ushort ioaddr;
	int	i;
	int	val;
	int	len;
	int	status;
	unsigned long time;
	struct dsa_mboxes mbox;
	
#ifdef DSA_DEBUG
	if (dsa_debug & DINIT) {
		PRF("dsa_propinit (%x)\n", dsa_blkp);
	}
#endif
	devi = dsa_blkp->db_dip;
	len = sizeof(int);
	if (HBA_INTPROP(devi, "ioaddr", &val, &len) != DDI_PROP_SUCCESS)
		return DDI_FAILURE;
	dsa_blkp->db_ioaddr   = (ushort) val;

	mutex_enter(&dsa_global_mutex);	
	if (!dsa_pgsz) {
		dsa_pgsz = ddi_ptob(devi, 1L);
		dsa_pgmsk = dsa_pgsz - 1;
		for (i=dsa_pgsz, len=0; i > 1; len++)
			i >>=1;
		dsa_pgshf = len;
	}
	mutex_exit(&dsa_global_mutex);

	bzero((char *)&mbox, sizeof(struct dsa_mboxes));
	mbox.m_box0 = NTV_GETHWCFG;
	status = dsa_init_cmd(dsa_blkp->db_ioaddr, &mbox);
	if(status == -1)
		return DDI_FAILURE;
	dsa_blkp->db_intr = mbox.m_box0;
#ifdef DSA_DEBUG
	if (dsa_debug & DINIT) 
		PRF("IRQ %d\n", dsa_blkp->db_intr);
#endif

	bzero((char *)&mbox, sizeof(struct dsa_mboxes));
	mbox.m_box0 = NTV_GETNTVSIZE;
	status = dsa_init_cmd(dsa_blkp->db_ioaddr, &mbox);
	if(status == -1)
		return DDI_FAILURE;
/* 	save handles 							*/
	dsa_blkp->db_max_handles = mbox.m_box0;

/* 	and max length of scatter gather list 				*/
	dsa_blkp->db_max_sglen = mbox.m_box1;

/*	set up stack for free handles					*/
	for(i=0;i < dsa_blkp->db_max_handles; i++) {
		dsa_blkp->db_pktp[i] = (struct dsa_cmpkt *)(i+1); 
	}
	dsa_blkp->db_pktp[i] = (struct dsa_cmpkt *)-1;
	dsa_blkp->db_tos_handle = 0;

#ifdef DSA_DEBUG
	if (dsa_debug & DINIT) {
		PRF("dsa_propinit ioaddr 0x%x handles %d sgllen %d\n", 
		dsa_blkp->db_ioaddr , dsa_blkp->db_max_handles,
		dsa_blkp->db_max_sglen );
	}
#endif

	/* XXX status = drv_getparm(TIME, &time); */

	return DDI_SUCCESS;
}

static u_int
dsa_xlate_vec(register struct  dsa_blk *dsa_blkp)
{
	static u_char dsa_vec[] = {11, 12, 13, 14, 15};
	register int i;
	register u_char vec;

	vec = dsa_blkp->db_intr;
	for (i=0; i<(sizeof(dsa_vec)/sizeof(u_char)); i++) {
		if (dsa_vec[i] == vec)
			return ((u_int)i);
	}
	return ((u_int)-1);
}

/*	Autovector Interrupt Entry Point				*/
/* Dummy return to be used before mutexes has been initialized		*/
/* guard against interrupts from drivers sharing the same irq line	*/
static u_int
dsa_dummy_intr(caddr_t arg)
{
	return DDI_INTR_UNCLAIMED;
}

static u_int
dsa_intr(caddr_t arg)
{
	register struct dsa_blk		*dsa_blkp;
	register ushort 		ioaddr;
	register struct dsa_cmpkt	*dsa_pktp;
	struct cmpkt			*pktp;
	int 				status;
	uint 				cmd_stat, xfer_count, handle;

	dsa_blkp = (struct dsa_blk *)arg;
	ioaddr = dsa_blkp->db_ioaddr;

	mutex_enter (&dsa_blkp->db_mutex);

/*	read 0x0f (eisa doorbell register) 				*/
	status = (int) inb(ioaddr | BMIC_EISA_DOOR_INT_STAT );
	if (!(status & NTV_LOGICAL_DOORBELL)) {
#ifdef DSA_DEBUG
		if(dsa_debug & DPKT)
			PRF("dsa_intr fail type %d\n",status);
#endif
		mutex_exit(&dsa_blkp->db_mutex);
		return DDI_INTR_UNCLAIMED;
	}

	for(;;) {

/*		first write 0xff to 0x0f 					*/
		outb(ioaddr | BMIC_EISA_DOOR_INT_STAT, NTV_NOCOMMAND);

/*		read data							*/
		cmd_stat = (int) inb(ioaddr |BMIC_MBOX_12);
		xfer_count = (int) inb(ioaddr |BMIC_MBOX_13);
		handle = (int) inb(ioaddr |BMIC_MBOX_14);

/*		retrieve pointer to the packet that just completed		*/
		dsa_pktp = dsa_blkp->db_pktp[handle];
		ASSERT(((unsigned int )dsa_pktp) > DSA_MAX_HANDLES);

		dsa_reset_tos(dsa_blkp, dsa_pktp);

		dsa_blkp->db_pkts_out--;
		
/*		clear the semaphor						*/
		outb(ioaddr | BMIC_SEMAPHORE_1, 0); 

		mutex_exit (&dsa_blkp->db_mutex);

		dsa_chkstatus(cmd_stat, xfer_count, dsa_pktp);
		pktp = (struct cmpkt *)dsa_pktp;
			(*pktp->cp_callback)(pktp);
		mutex_enter (&dsa_blkp->db_mutex);

		if(!(inb(ioaddr | BMIC_EISA_DOOR_INT_STAT ) & NTV_LOGICAL_DOORBELL))
			break;
	}

	mutex_exit(&dsa_blkp->db_mutex);
	return DDI_INTR_CLAIMED;
}

static int
dsa_findctl(register ushort ioaddr)
{
	unchar	id;
	int	i, status, drives_found;
	unsigned long time;
	struct dsa_mboxes mbox;

	/* no need to protect findctl with a mutex: this probe routine
	will be called prior to any attach for each possible card address
	in the dsa.conf file */

	id = inb (ioaddr | BMIC_ID_0);
	if (id != 0x10)
		return 0;

	id = inb (ioaddr | BMIC_ID_1);
	if (id != 0xac)
		return 0;

	id = inb (ioaddr | BMIC_ID_2);
	if (id != 0x40)
		return 0;

#ifdef DSA_DEBUG
	if (dsa_debug & DINIT) {
		PRF("dsa_findctl good at (%x)\n", ioaddr);
	}
#endif

	/* write 0 to 9 (interrupt control register) to block interrupts */
	outb(ioaddr | BMIC_SYS_INT_ENABLE, 0);
	/* disable doorbell interrupts */
	outb(ioaddr | BMIC_EISA_DOOR_EN	, 0);

	bzero((char *)&mbox, sizeof(struct dsa_mboxes));
	mbox.m_box0 = NTV_GETVERSION;
	status = dsa_init_cmd(ioaddr, &mbox);
	if(status == -1)
		return 0;
	
	if(mbox.m_box0 < 2) {
#ifdef DSA_DEBUG
		if (dsa_debug & DINIT) 
			PRF("Bad controller version %d\n",status);
#endif
		return 0;
	}

	bzero((char *)&mbox, sizeof(struct dsa_mboxes));
	mbox.m_box0 = NTV_PUPSTAT;
	status = dsa_init_cmd(ioaddr, &mbox);
	if(status == -1 || mbox.m_box0 != PUP_OK)
		return 0;
#ifdef DSA_DEBUG
	if (dsa_debug & DINIT) 
		PRF("Power up status is (%d) %s\n", mbox.m_box0,
		dsa_pup_errs[mbox.m_box0]);
#endif

	bzero((char *)&mbox, sizeof(struct dsa_mboxes));
	mbox.m_box0 = NTV_GETNUMCUNS;
	status = dsa_init_cmd(ioaddr, &mbox);
	if(status == -1)
		return 0;
#ifdef DSA_DEBUG
	if (dsa_debug & DINIT) 
		PRF("Number of composite drives is 0x%x\n", mbox.m_box0 & 0xff);
#endif

	if(mbox.m_box1 ==  1) {
	/* If we're not in enhanced mode, let aha driver handle it */
#ifdef DSA_DEBUG
	if (dsa_debug & DINIT) 
		PRF("DSA controller at 0x%x in Adaptec emulation mode\n",ioaddr);
#endif
	}

	/* number of drives */
	return (mbox.m_box0);
}

/* Returns -1 for failure , 0 for success
 * No mutex necessary because this is called only at init time, and only once
 * from dsa.conf for each ioaddr */
static int
dsa_init_cmd(register ushort ioaddr, struct dsa_mboxes *mboxp) 
{
	int status, i;
#ifdef DSA_DEBUG
	unchar cmd;
#endif

	/* allocate inbound semaphor */
	if( dsa_try_insem(ioaddr| BMIC_SEMAPHORE_0, 10)) {
#ifdef DSA_DEBUG
		if(dsa_debug & DINIT) {
			cmd = mboxp->m_box0;
			PRF("dsa_init_cmd: try_insem failure for %s (%d)\n",
			dsa_ext_cmds[cmd],cmd);
		}
#endif
		return -1;
	}

	/* write out command and parameters */
	outb(ioaddr | BMIC_MBOX_0, mboxp->m_box0);
	outb(ioaddr | BMIC_MBOX_1, mboxp->m_box1);
	outb(ioaddr | BMIC_MBOX_2, mboxp->m_box2);
	outb(ioaddr | BMIC_MBOX_3, mboxp->m_box3);

	/* write 40 to register d (submits the extended command) */
	outb(ioaddr |BMIC_LOC_DOOR_INT_STAT, NTV_EXTENDED_DOORBELL);
	
	for(i=0;i < 1000;i++) {
		status = (int) inb(ioaddr | BMIC_EISA_DOOR_INT_STAT );
		if(status == NTV_EXTENDED_DOORBELL)
			break;
		drv_usecwait(10);
	}
	if(!(status & NTV_EXTENDED_DOORBELL)) {
#ifdef DSA_DEBUG
		if(dsa_debug & DINIT)
			PRF("%s command failed\n", dsa_ext_cmds[cmd]);
#endif
		return -1;
	}

	/* when on 0x40, first write 0xff to 0x0f */
	outb(ioaddr | BMIC_EISA_DOOR_INT_STAT, NTV_NOCOMMAND);

	/* read in data */
	mboxp->m_box0 = inb(ioaddr |BMIC_MBOX_0);
	mboxp->m_box1 = inb(ioaddr |BMIC_MBOX_1);
	mboxp->m_box2 = inb(ioaddr |BMIC_MBOX_2);
	mboxp->m_box3 = inb(ioaddr |BMIC_MBOX_3);
	mboxp->m_box4 = inb(ioaddr |BMIC_MBOX_4);
	mboxp->m_box5 = inb(ioaddr |BMIC_MBOX_5);
	mboxp->m_box6 = inb(ioaddr |BMIC_MBOX_6);
	mboxp->m_box7 = inb(ioaddr |BMIC_MBOX_7);
	mboxp->m_box8 = inb(ioaddr |BMIC_MBOX_8);
	mboxp->m_box9 = inb(ioaddr |BMIC_MBOX_9);
	mboxp->m_box10 = inb(ioaddr |BMIC_MBOX_10);
	mboxp->m_box11 = inb(ioaddr |BMIC_MBOX_11);
	mboxp->m_box12 = inb(ioaddr |BMIC_MBOX_12);
	mboxp->m_box13 = inb(ioaddr |BMIC_MBOX_13);
	mboxp->m_box14 = inb(ioaddr |BMIC_MBOX_14);
	mboxp->m_box15 = inb(ioaddr |BMIC_MBOX_15);

/* 	write 0 to semaphor 0 at 0x0a because card is defective
	outb(ioaddr | BMIC_SEMAPHORE_0, 0);  */

/*	clear the semaphor						*/
	outb(ioaddr | BMIC_SEMAPHORE_1, 0); 

	return 0;
}

/* always called with mutex held					*/
/* returns 0 on success, 1 on failure 					*/
static int
dsa_send_lcmd(struct dsa_blk *dsa_blkp, register struct dsa_cmpkt *pktp)
{
	register ushort ioaddr = dsa_blkp->db_ioaddr;


/*  	spin on semaphor_0 0x0a, wait for it to go to 0 
	(least significant bit) */
	if(dsa_try_insem(ioaddr| BMIC_SEMAPHORE_0, 15)) {
#ifdef DSA_DEBUG
		if(dsa_debug & DTST)
			PRF("dsa_send_lcmd: try_insem failure for %s (%d)\n",
			pktp->dc_command,dsa_logical_cmds[pktp->dc_command]);
#endif
		dsa_reset_tos(dsa_blkp, pktp);
		return 1;
	}

/*	write MBOX regs (0-11) 						*/
	outb(ioaddr | BMIC_MBOX_0,  pktp->dc_command);
	outb(ioaddr | BMIC_MBOX_1,  pktp->dc_drive_unit);
	outb(ioaddr | BMIC_MBOX_2,  pktp->dc_bytes);
	outb(ioaddr | BMIC_MBOX_3,  pktp->dc_handle);
	outl(ioaddr | BMIC_MBOX_4,  pktp->dc_block);
	outl(ioaddr | BMIC_MBOX_8,  pktp->dc_paddr);

/* 	write logical doorbell bit 0x10 into 0x0d 			*/
	outb(ioaddr |BMIC_LOC_DOOR_INT_STAT, NTV_LOGICAL_DOORBELL);

	return 0;
}

static int
dsa_getedt(struct dsa_blk *dsa_blkp, int drives_found)
{
	struct dsa_mboxes mbox;
	struct dsarpbuf	*rpbp;
	int	status, i;

	/*
	 * Interrupts should already be turned off -- see dsa_findctl()
	 */


	for(i=0;i < drives_found;i++) {
		bzero((char *)&mbox, sizeof(struct dsa_mboxes));
		mbox.m_box0 = NTV_GETCUNDATA;
		mbox.m_box1 = (unchar) i;
		status = dsa_init_cmd(dsa_blkp->db_ioaddr, &mbox);
		if(status == -1 || mbox.m_box11 != PUP_OK) {
#ifdef DSA_DEBUG
			if (dsa_debug & DINIT) {
				PRF("dsa_getedt: GETCUNDATA fail at 0x%x status %s)\n", 
				dsa_blkp, dsa_pup_errs[mbox.m_box11]);
			}
#endif
			return DDI_FAILURE;
		}

#ifdef DSA_DEBUG
		if (dsa_debug & DINIT)
			dsa_dump_mboxes((char *)&mbox);
#endif

		if (!(rpbp = (struct dsarpbuf *)kmem_zalloc(
				(sizeof (struct dsarpbuf) +
				 sizeof (struct scsi_inquiry)),
				KM_NOSLEEP)))
			return DDI_FAILURE;
		dsa_blkp->db_inqp[i] = (struct scsi_inquiry *)(rpbp + 1);
		dsa_blkp->db_rpbp[i] = rpbp;
		dsa_fake_inquiry (rpbp, dsa_blkp->db_inqp[i]);

		rpbp->dsarp_cap = mbox.m_box0;
		rpbp->dsarp_cap |= (mbox.m_box1 << 8);
		rpbp->dsarp_cap |= (mbox.m_box2 << 16);
		rpbp->dsarp_cap |= (mbox.m_box3 << 24);
		rpbp->dsarp_heads = mbox.m_box4;
		rpbp->dsarp_sectors = mbox.m_box5;
		rpbp->dsarp_cylinders = mbox.m_box6;
		rpbp->dsarp_cylinders |= (mbox.m_box7 << 8);
		rpbp->dsarp_secs_per_track = mbox.m_box8;
		rpbp->dsarp_secs_per_track |= (mbox.m_box9 << 8);
		rpbp->dsarp_phys_heads = mbox.m_box10;
#ifdef DSA_DEBUG
		if (dsa_debug & DINIT) {
			PRF("dsa_getedt: capacity   %d\n", rpbp->dsarp_cap);
			PRF("            heads      %d\n", rpbp->dsarp_heads); 
			PRF("            sectors    %d\n", rpbp->dsarp_sectors); 
			PRF("            cylinders  %d\n", rpbp->dsarp_cylinders); 
			PRF("            secs/track %d\n", rpbp->dsarp_secs_per_track); 
			PRF("            phys heads %d\n\n", rpbp->dsarp_phys_heads); 
		}
#endif
	}

	return DDI_SUCCESS;
}

/* always called with mutex held at run time */
/* returns 0 if semaphor available */
static int
dsa_try_insem(register ushort sem0_ioaddr, uint timeout)
{
	int i;
	unchar value;

	for (i = 0; i < timeout; i++)
	{
		value = inb(sem0_ioaddr);
		if(!value) {
			outb(sem0_ioaddr, 1);
			return 0;
		}
		drv_usecwait(20);
	}

#ifdef DSA_DEBUG
	if(dsa_debug & DPKT)
		PRF("dsa_try_insem: failed after %d trys for semaphor at 0x%x\n",
		i, sem0_ioaddr);
#endif

	return 1;
}

/*
 * This function is called with mutex held and releases it. 
 * It returns 0 on success, 1 on failure 
 */
static int
dsa_pollret(register struct dsa_blk *dsa_blkp, register struct dsa_cmpkt *dsa_pktp)
{
	register ushort ioaddr = dsa_blkp->db_ioaddr;
	int status, i, retval = 0;
	int counter = 0;
	uint cmd_stat, xfer_count, handle;
	struct cmpkt	*pktp = (struct cmpkt *) dsa_pktp;
	struct dsa_cmpkt *hw_pktp = (struct dsa_cmpkt *)0;
	struct	dsa_cmpkt *cmd;
	struct	dsa_cmpkt *cmd_hdp = (struct  dsa_cmpkt *)0;

	for(;;) {
		for(i=0;i < 10000;i++) {
/* 			read 0x0f (eisa doorbell register) 			*/
			status = (int) inb(ioaddr | BMIC_EISA_DOOR_INT_STAT );
			if(status & NTV_LOGICAL_DOORBELL)
				break;
			drv_usecwait(10);
		}

		if(!(status & NTV_LOGICAL_DOORBELL)) {
#ifdef DSA_DEBUG
			if(dsa_debug & DPOLL)
				PRF("dsa_pollret fail. Interrupt pending type %d\n",status);
#endif
			outb(ioaddr | BMIC_EISA_DOOR_INT_STAT, NTV_NOCOMMAND);
			outb(ioaddr | BMIC_SEMAPHORE_1, 0); 
			if(counter < 5) {
				counter++;
				continue;
			} else {
				mutex_exit (&dsa_blkp->db_mutex);
				return 1;
			}
		}

/*		when on 0x10, first write 0xff to 0x0f 			*/
		outb(ioaddr | BMIC_EISA_DOOR_INT_STAT, NTV_NOCOMMAND);

/* 		read in data */
		cmd_stat = (int) inb(ioaddr |BMIC_MBOX_12);
		xfer_count = (int) inb(ioaddr |BMIC_MBOX_13);
		handle = (int) inb(ioaddr |BMIC_MBOX_14);

/* 		retrieve pointer to the packet that just completed	*/
		hw_pktp = dsa_blkp->db_pktp[handle];

/* 		clear semaphor 1 					*/
		outb(ioaddr | BMIC_SEMAPHORE_1, 0); 

		if(hw_pktp) {
			dsa_reset_tos(dsa_blkp, hw_pktp);
			dsa_chkstatus(cmd_stat, xfer_count, hw_pktp);
#ifdef DSA_DEBUG
			if(dsa_debug & DTST) 
				dsa_blkp->db_pkts_out--;
#endif
		} else
			continue;

		if(hw_pktp == dsa_pktp) {
			break;
		}

/*		chain up packets until the polled packet returns 	*/
		if(hw_pktp) {
			hw_pktp->dc_linkp = (struct dsa_cmpkt *)0;
			if (!cmd_hdp)
				cmd_hdp = hw_pktp;
			else {
				for (cmd=cmd_hdp; cmd->dc_linkp;cmd=cmd->dc_linkp)
					;
				cmd->dc_linkp = hw_pktp;
			}
		}
	}

	mutex_exit (&dsa_blkp->db_mutex);

/*	run callback on the packet we expected				*/
	if(hw_pktp)
		(*pktp->cp_callback)(pktp);

/*	check for other completed packets that have been queued		*/
	if (cmd_hdp) {
		for (; cmd=cmd_hdp; ) {
			cmd_hdp = cmd->dc_linkp;
			pktp = (struct cmpkt *)cmd;
/* 			and run the callbacks 				*/
			(*pktp->cp_callback)(pktp);
		}
	}

	return retval;
}


static void
dsa_chkstatus(uint cmd_stat, uint xfer_count, struct dsa_cmpkt *dsa_pktp)
{
	struct cmpkt *pktp = (struct cmpkt *)dsa_pktp;

	if (cmd_stat == DSA_OK) {
		pktp->cp_reason = CPS_SUCCESS;
		dsa_pktp->dc_scb = DERR_SUCCESS;
		pktp->cp_resid =  0;
		return;
	}

/*	set bytes which did not transfer in this io op		*/
	pktp->cp_resid = xfer_count * NBPSCTR;
	pktp->cp_reason = CPS_CHKERR;
	dsa_pktp->dc_scb = DERR_HARD; /* force no retry 	*/
	pktp->cp_retry = 1;	/* print out block numbers	*/

#ifdef DSA_DEBUG
	if(dsa_debug & DTST)
		PRF("chkstatus error: resid %x error %x\n", pktp->cp_resid, cmd_stat);

	switch (cmd_stat) {

		case DSA_BADBLOCK:
			cmn_err(CE_WARN,"bad block\n");
			break;

		case DSA_UNCORECT:
			cmn_err(CE_WARN,"uncorrectable error\n");
			break;

		case DSA_WRITEFLT:
			cmn_err(CE_WARN,"write fault\n");
			break;

		case DSA_IDNFOUND:
			cmn_err(CE_WARN,"sector id not found\n");
			break;

		case DSA_CORRECT:
			cmn_err(CE_WARN,"correctable error\n");
			break;

		case DSA_ABORT:
			cmn_err(CE_WARN,"drive requested abort\n");
			break;

		case DSA_TRACK0NF:
			cmn_err(CE_WARN,"no track 0 found\n");
			break;

		case DSA_LTIMEOUT:
			cmn_err(CE_WARN,"logical drive timeout\n");
			break;
		default:
			if(dsa_debug & DPKT)
				PRF("Unexpected failure status %x\n",cmd_stat);
			break;
	}

	if(dsa_debug & DPKT)
		PRF("cmd stat %x scb %x\n",cmd_stat,  dsa_pktp->dc_scb);
#endif

}

static void
dsa_fake_inquiry (struct dsarpbuf *rpbp, struct scsi_inquiry *inqp)
{
	inqp->inq_dtype		= DTYPE_DIRECT;
	inqp->inq_qual		= DPQ_POSSIBLE;
#ifdef DSA_DEBUG
	inqp->inq_rmb		= 0;
	inqp->inq_ansi		= 0;
	inqp->inq_ecma		= 0;
	inqp->inq_iso		= 0;
	inqp->inq_rdf		= 0;
	inqp->inq_trmiop	= 0;
	inqp->inq_aenc		= 0;
	inqp->inq_len		= 0;
	inqp->inq_sftre		= 0;
	inqp->inq_cmdque	= 0;
	inqp->inq_linked	= 0;
	inqp->inq_sync		= 0;
	inqp->inq_wbus16	= 0;
	inqp->inq_wbus32	= 0;
	inqp->inq_reladdr	= 0;
#endif

	strncpy (inqp->inq_vid, "Dell DSA", 8);
	strncpy (inqp->inq_pid, "Scsi", 4);
	strncpy (inqp->inq_revision, "1234", 4);
}

#ifdef DSA_DEBUG
static void
dsa_dump_mboxes(char *p)
{
	int i;

	PRF("\nbox (0x%x) ");
	for(i=0;i < sizeof(struct dsa_mboxes);i++,p++) {
		PRF("%x ",*p & 0xff);
	}
	PRF("\n");
}

static void
dsa_dump_dsapkt(struct dsa_cmpkt *p)
{
	int i;
	PRF("cmpkt: scblen %x cdblen %x cdbp 0x%x reason %x callback 0x%x\n",
	p->dc_pkt.cp_scblen,  p->dc_pkt.cp_cdblen,  
	p->dc_pkt.cp_cdbp,  p->dc_pkt.cp_reason, p->dc_pkt.cp_callback);
	PRF("       buf 0x%x resid %x byteleft %x bytexfer %x\n",
	p->dc_pkt.cp_bp,  p->dc_pkt.cp_resid, p->dc_pkt.cp_byteleft,
	p->dc_pkt.cp_bytexfer);
	PRF("       srtsec 0x%x secleft %x retry %x\n",
	p->dc_pkt.cp_srtsec,  p->dc_pkt.cp_secleft, p->dc_pkt.cp_retry);
	PRF("cdb %x scb %x flags %x cmd %x drive %x bytes %x handle %x\n",
	p->dc_cdb, p->dc_scb, p->dc_flags, p->dc_command & 0xff, 
	p->dc_drive_unit & 0xff, p->dc_bytes & 0xff, p->dc_handle & 0xff);
	PRF("block %x paddr 0x%x vaddr 0x%x\n",p->dc_block, p->dc_paddr,
		p->dc_vaddr);
	for(i=0; p->dc_sg_list[i].data_len !=0; i++) {
		PRF("%d %x %x ", i, p->dc_sg_list[i].data_ptr,
		p->dc_sg_list[i].data_len);
		if(i >= 16)
			break;
	}
}

static void
dsa_dump_dsablk(struct dsa_blk *p)
{
	PRF("numdev %d refcnt %d flag 0x%x max_hand 0x%x act_hand 0x%x intr 0x%x\n",

		p->db_numdev & 0xff, p->db_refcount, p->db_flag & 0xff, 
		p->db_max_handles, p->db_active_handles, p->db_intr & 0xff);

	PRF("dip 0x%x ioaddr 0x%x inqp 0x%x pktsout 0x%x tos_hand %d act_hand %d db_pktp 0x%x\n",
		p->db_dip, p->db_ioaddr, p->db_inqp, p->db_pkts_out,
		p->db_tos_handle, p->db_active_handles, &p->db_pktp);

}

static void
dsa_dump_unit(struct dsa_unit *p)
{
	PRF("dsa_unit (0x%x) unit %d cap 0x%x heads %d sectors %d cyls %d\n",
	p, p->du_drive_unit, p->du_cap, p->du_heads, p->du_sectors, 
		p->du_cylinders);
	PRF("sectors per track %d physical heads %d\n",p->du_secs_per_track, 
		p->du_phys_heads);
}
#endif
