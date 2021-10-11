/*
 * Copyright (c) 1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident "@(#)ata.c	1.65	96/08/05 SMI"

#if defined(i386)
#define	_mca_bus_supported
#define	_eisa_bus_supported
#define	_isa_bus_supported
#endif

#if defined(__ppc)
#define	_isa_bus_supported
#endif

#include <sys/types.h>
#include <sys/promif.h>
#include <sys/esunddi.h>
#include <sys/byteorder.h>

#include <sys/scsi/scsi.h>
#include <sys/dkio.h>
#include <sys/dktp/dadkio.h>
#include <sys/dktp/dadev.h>
#include <sys/dktp/hba.h>
#include <sys/dktp/ata.h>
#include <sys/dktp/tgdk.h>
#include <sys/dktp/flowctrl.h>
#include <sys/debug.h>
#include <sys/cdio.h>
#include <sys/modctl.h>
#include <sys/sunddi.h>
#if defined(_eisa_bus_supported)
#include <sys/nvm.h>
#include <sys/eisarom.h>
#endif


/*
 * Entry Points.
 */
static struct cmpkt *ata_pktalloc(struct ata *atap, int (*callback)(),
				    caddr_t arg);
static void ata_pktfree(struct ata *atap, struct cmpkt *pktp);
static struct cmpkt *ata_memsetup(struct ata *atap, struct cmpkt *pktp,
				    struct buf *bp, int (*callback)(),
				    caddr_t arg);
static void ata_memfree(struct ata *atap, struct cmpkt *pktp);
static struct cmpkt *ata_iosetup(struct ata *atap, struct cmpkt *pktp);
static int ata_transport(struct ata *atap, struct cmpkt *pktp);
static int ata_reset(struct ata *atap, int level);
static int ata_abort(struct ata *atap, struct cmpkt *pktp);
static int ata_ioctl(struct ata *atap, int cmd, int arg, int flag);
static u_int ata_intr(caddr_t arg);

static int ata_bus_ctl(dev_info_t *d, dev_info_t *r, ddi_ctl_enum_t o,
			void *a, void *v);
static int ata_identify(dev_info_t *dev);
static int ata_probe(dev_info_t *);
static int ata_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int ata_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

/*
 * Local Function Prototypes
 */
static u_int ata_dummy_intr(caddr_t arg);
static int ata_initchild(dev_info_t *mdip, dev_info_t *cdip);
static int ata_propinit(struct ata_blk *ata_blkp);
static int ata_pollret(struct ata_blk *ata_blkp);
static int ata_wait(ushort port, ushort mask, ushort onbits,
		    ushort offbits);
static int ata_wait1(ushort port, ushort mask, ushort onbits,
		    ushort offbits, int interval);
static int ata_start(struct ata *atap);
static int ata_checkpresence(uint ioaddr1, uint ioaddr2);
static void ata_get_drv_cfg(struct ata *atap);
static int ata_id(uint ioaddr, ushort *buf);
static int atapi_id(uint ioaddr, ushort *buf);
static int ata_check_for_atapi_12(uint ioaddr);
static int ata_shuffle_up_and_start(struct ata *atap, int delay);
static int ata_gen_intr(struct ata_blk *ata_blkp, int *rc);
static void ata_gen_intr_end(struct ata_cmpkt *ata_pktp);
static int ata_start_next_cmd(struct ata *this_ata);
static void ata_translate_error(struct ata_cmpkt *ata_pktp);
static int ata_send_data(struct ata *atap, int count);
static int ata_get_data(struct ata *atap, int count);
static int ata_getedt(struct ata_blk *ata_blkp);
static u_char ata_drive_type(uint ioaddr, ushort *buf);
static int ata_setpar(struct ata_blk *ata_blkp,
			int drive, int heads, int sectors);
static void ata_byte_swap(char *buf, int n);
static void ata_fake_inquiry(struct atarpbuf *rpbp, struct scsi_inquiry *inqp);
static int ata_set_rw_multiple(struct ata_blk *ata_blkp, int drive);
static void ata_clear_queues(struct ata *atap);
static void ata_nack_packet(struct ata_cmpkt *ata_pktp, kmutex_t *mutex);
static int ata_inquiry(struct ata_blk *ata_blkp, struct scsi_inquiry *inqp,
	int drive);
static void ata_ack_media_change(struct ata_blk *ata_blkp,
	struct ata_cmpkt *ata_pktp);
static int ata_lock_unlock(struct ata *atap, struct ata_cmpkt *ata_pktp,
			int lock);
static int ata_start_stop_motor(struct ata *atap, struct ata_cmpkt *ata_pktp,
			int start);
static int ata_get_state(struct ata *atap, struct ata_cmpkt *ata_pktp,
	enum dkio_state *statep);
static struct cmpkt *ata_read(struct ata *atap, struct cmpkt *pktp);
static int ata_read_capacity(struct ata *atap, struct ata_cmpkt *pktp);
static int ata_cd_pause_resume(struct ata *atap, struct ata_cmpkt *ata_pktp,
				int resume);
static int ata_cd_play_msf(struct ata *atap, struct ata_cmpkt *ata_pktp,
				caddr_t data, int flag);
static int ata_cd_read_subchannel(struct ata *atap, struct ata_cmpkt *ata_pktp,
				caddr_t data, int flag);
static int ata_cd_read_tochdr(struct ata *atap, struct ata_cmpkt *ata_pktp,
				caddr_t data, int flag);
static int ata_cd_read_tocentry(struct ata *atap, struct ata_cmpkt *ata_pktp,
				caddr_t data, int flag);
static int ata_cd_volume_ctrl(struct ata *atap, struct ata_cmpkt *ata_pktp,
				caddr_t data, int flag);
static void ata_enable_intr(struct ata_blk *ata_blkp);
static ulong ata_stoh_long(ulong ai);

#if defined(_eisa_bus_supported)
static int ata_detect_dpt(uint ioaddr);
static int ata_check_io_addr(NVM_PORT *ports, uint ioaddr);
#endif

#ifdef	ATA_DEBUG
static void ata_print_errflag(int evalue);
static void ata_print_sttflag(int evalue);
#endif

/*
 * Local static data
 */
static int ata_cb_id = 0;
static kmutex_t ata_global_mutex;
static kmutex_t ata_rmutex;
static int ata_global_init = 0;
static int ata_indump;
static ushort ata_bit_bucket[ATA_CD_SECSIZ << 1];
static int irq13_addr;

#ifdef	ATA_DEBUG
#define	DENT	0x0001	/* function entry points info */
#define	DPKT	0x0002	/* packet info */
#define	DIO	0x0004  /* IO info */
#define	DERR	0x0010	/* errors */
#define	DINIT	0x0020	/* initialization */

static	int	ata_debug;

#endif	/* ATA_DEBUG */

#define	IDE_PRIMARY		(1 << 0)
#define	IDE_SECONDARY		(1 << 1)

#ifdef	DADKIO_RWCMD_READ
/* this is equivalent to (struct ata_cmpkt *)ata_pktp */
#define	CMPKT	ata_pktp->ac_pkt
#define	RWCMDP	((struct dadkio_rwcmd *)(ata_pktp->ac_pkt.cp_bp->b_back))
#endif

/* mask for tweaking timing code in ata_start */
#define	ATA_BSY_WAIT_1		0x1

static int ata_sense_table[] = {
	DERR_SUCCESS,	/* 0 No sense (successful cmd)	*/
	DERR_RECOVER,	/* 1 Recovered error		*/
	DERR_NOTREADY,	/* 2 Device not ready		*/
	DERR_MEDIUM,	/* 3 Medium error		*/
	DERR_HW,	/* 4 Hardware error		*/
	DERR_ILL,	/* 5 Illegal request		*/
	DERR_UNIT_ATTN,	/* 6 Unit attention		*/
	DERR_DATA_PROT, /* 7 Data protection		*/
	DERR_RESV,	/* 8 Reserved			*/
	DERR_RESV,	/* 9 Reserved			*/
	DERR_RESV,	/* A Reserved			*/
	DERR_ABORT,	/* B Aborted cmd		*/
	DERR_RESV,	/* C Reserved			*/
	DERR_RESV,	/* D Reserved			*/
	DERR_MISCOMP,	/* E Miscompare			*/
	DERR_RESV	/* F Reserved			*/
};

/*
 * 	bus nexus operations.
 */
static struct bus_ops ata_bus_ops = {
#ifdef	BUSO_REV
	BUSO_REV,
#endif
	nullbusmap,
	0,	/* ddi_intrspec_t	(*bus_get_intrspec)(); */
	0,	/* int		(*bus_add_intrspec)(); */
	0,	/* void		(*bus_remove_intrspec)(); */
	i_ddi_map_fault,
	ddi_dma_map,
#ifdef	BUSO_REV
	ddi_dma_allochdl,
	ddi_dma_freehdl,
	ddi_dma_bindhdl,
	ddi_dma_unbindhdl,
	ddi_dma_flush,
	ddi_dma_win,
#endif
	ddi_dma_mctl,
	ata_bus_ctl,
	ddi_bus_prop_op,
};

static struct dev_ops	ata_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	ata_identify,		/* identify */
	ata_probe,		/* probe */
	ata_attach,		/* attach */
	ata_detach,		/* detach */
	nulldev,		/* reset */
	(struct cb_ops *)0,	/* driver operations */
	&ata_bus_ops		/* bus operations */
};

static struct ctl_objops ata_objops = {
	ata_pktalloc,
	ata_pktfree,
	ata_memsetup,
	ata_memfree,
	ata_iosetup,
	ata_transport,
	ata_reset,
	ata_abort,
	nulldev,
	nulldev,
	ata_ioctl,
	0, 0
};

/*
 * This is the driver loadable module wrapper.
 */
static struct modldrv modldrv = {
	&mod_driverops,		/* Type of module. This one is a driver */
	"ATA AT-bus attachment disk controller Driver",	/* module name */
	&ata_ops,					/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};


int
_init(void)
{
	int	status;

	mutex_init(&ata_global_mutex, "ATA global Mutex",
		MUTEX_DRIVER, NULL);
	status = mod_install(&modlinkage);
	if (status != 0) {
		mutex_destroy(&ata_global_mutex);
	}
#ifdef ATA_DEBUG
	PRF("ATA_DEBUG ON\n");
	debug_enter("ata.c _init");
#endif
	return (status);
}

int
_fini(void)
{
	int	status;

	status = mod_remove(&modlinkage);
	if (status == 0)
		mutex_destroy(&ata_global_mutex);
	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*ARGSUSED*/
static int
ata_bus_ctl(dev_info_t *d, dev_info_t *r, ddi_ctl_enum_t o, void *a, void *v)
{
	switch (o) {

	case DDI_CTLOPS_REPORTDEV:
	{
		int 	len = sizeof (int);
		int 	targ;

		cmn_err(CE_CONT, "?%s%d at %s%d",
			ddi_get_name(r), ddi_get_instance(r),
			ddi_get_name(d), ddi_get_instance(d));
		if (HBA_INTPROP(r, "target", &targ, &len) != DDI_SUCCESS)
			targ = 0;
		cmn_err(CE_CONT, "? target %d lun %d\n", targ, 0);
		return (DDI_SUCCESS);
	}
	case DDI_CTLOPS_INITCHILD:
		return (ata_initchild(d, a));

	case DDI_CTLOPS_UNINITCHILD:
	{
		register struct scsi_device *devp;
		register dev_info_t *dip = (dev_info_t *)a;
		struct ctl_obj	*ctlobjp;
		struct ata	*atap;

		devp = (struct scsi_device *)ddi_get_driver_private(dip);
		if (devp != (struct scsi_device *)NULL) {
			/* unlink the ata from the chain */
			ctlobjp = (struct ctl_obj *)devp->sd_address.a_hba_tran;
			atap = (struct ata *)ctlobjp->c_data;

			mutex_enter(&atap->a_blkp->ab_mutex);

			if (atap == atap->a_forw && atap == atap->a_back) {
				/*
				 * There's only one
				 * clear the pointer that points to it.
				 */
				atap->a_blkp->ab_link = NULL;
			} else {
				/*
				 * there's more than one...
				 * unlink it from the list (carefully).
				 */
				atap->a_forw->a_back = atap->a_back;
				atap->a_back->a_forw = atap->a_forw;
				if (atap->a_blkp->ab_link == atap)
					atap->a_blkp->ab_link = atap->a_forw;
			}

			mutex_exit(&atap->a_blkp->ab_mutex);

			mutex_destroy(&devp->sd_mutex);
			kmem_free((caddr_t)devp,
			    sizeof (*devp) + sizeof (struct ctl_obj) +
			    sizeof (struct ata) + sizeof (struct ata_unit));
		}

		ddi_set_driver_private(dip, NULL);
		ddi_set_name_addr(dip, NULL);
		return (DDI_SUCCESS);
	}

	case DDI_CTLOPS_IOMIN:
	{
		*((int *)v) = 1;
		return (DDI_SUCCESS);
	}

	default:
		cmn_err(CE_CONT, "%s%d: invalid op (%d) from %s%d\n",
			ddi_get_name(d), ddi_get_instance(d),
			o, ddi_get_name(r), ddi_get_instance(r));
		return (ddi_ctlops(d, r, o, a, v));
	}

}

static int
ata_initchild(dev_info_t *mdip, dev_info_t *cdip)
{
	int 	len;
	int 	targ;
	int	lun;
	struct 	scsi_device *devp;
	char 	name[MAXNAMELEN];
	struct	ctl_obj *ctlobjp;
	struct	ata *atap;
	struct	ata_unit *ata_unitp;
	struct	ata_blk *ata_blkp;
	struct	ata *atalinkp;
	short	*chs;
	char	configname[35];

	len = sizeof (int);
	/*
	 * check LUNs first.  there are more of them.
	 * If lun isn't specified, assume 0
	 * If a lun other than 0 is specified, fail it now.
	 */
	if (HBA_INTPROP(cdip, "lun", &lun, &len) != DDI_SUCCESS)
		lun = 0;
	if (lun != 0) 	/* no support for lun's				*/
		return (DDI_NOT_WELL_FORMED);
	if (HBA_INTPROP(cdip, "target", &targ, &len) != DDI_SUCCESS)
		return (DDI_NOT_WELL_FORMED);

	ata_blkp = (struct ata_blk *)ddi_get_driver_private(mdip);
	if ((ata_blkp->ab_rpbp[targ] == NULL) ||
		(ata_blkp->ab_dev_type[targ] == ATA_DEV_NONE))
		return (DDI_NOT_WELL_FORMED);

	/*
	 * NB: When devices other than disks (ex. SCSI tapes) come through
	 * INITCHILD in the ata driver, the result is bogus ata structs
	 * left on the chain.  The rest of the code must deal with this.
	 */

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_initchild processing target %x lun %x\n", targ, lun);
	}
#endif
	if (!(devp = (struct scsi_device *)kmem_zalloc(sizeof (*devp) +
						sizeof (*ctlobjp) +
						sizeof (*atap) +
						sizeof (*ata_unitp),
						KM_NOSLEEP)))
		return (DDI_NOT_WELL_FORMED);

	ctlobjp		= (struct ctl_obj *)(devp+1);
	atap		= (struct ata *)(ctlobjp+1);
	ata_unitp	= (struct ata_unit *)(atap+1);

	atap->a_blkp  = ata_blkp;
	atap->a_unitp = ata_unitp;
	atap->a_ctlobjp = ctlobjp;

	ata_unitp->au_targ = (u_char)targ;
	ata_unitp->au_drive_bits =
			(targ == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1);
	ata_unitp->au_rpbuf = ata_blkp->ab_rpbp[targ];
	ata_unitp->au_phhd = ata_unitp->au_rpbuf->atarp_heads;
	ata_unitp->au_phsec = ata_unitp->au_rpbuf->atarp_sectors;
	ata_unitp->au_bioshd   = ata_unitp->au_rpbuf->atarp_heads;
	ata_unitp->au_biossec  = ata_unitp->au_rpbuf->atarp_sectors;
	ata_unitp->au_bioscyl  = ata_unitp->au_rpbuf->atarp_fixcyls - 2;
	ata_unitp->au_acyl = 2;
	ata_unitp->au_block_factor = ata_blkp->ab_block_factor[targ];
	ata_unitp->au_rd_cmd = ata_blkp->ab_rd_cmd[targ];
	ata_unitp->au_wr_cmd = ata_blkp->ab_wr_cmd[targ];
	ata_unitp->au_bytes_per_block = ata_unitp->au_block_factor << SCTRSHFT;

	sprintf(configname, "SUNW-ata-%x-d%d-chs", ata_blkp->ab_data, targ+1);
	if (ddi_getlongprop(DDI_DEV_T_ANY, ddi_root_node(), 0,
			configname, (caddr_t)&chs, &len) == DDI_PROP_SUCCESS) {
		/*
		 * if the number of sectors and heads in bios matches the
		 * physical geometry, then so should the number of cylinders
		 * this is to prevent the 1023 limit in the older bios's
		 * causing loss of space.
		 */
		if (chs[1] == (ata_unitp->au_bioshd - 1) &&
					chs[2] == ata_unitp->au_biossec)
			chs[0] = ata_unitp->au_rpbuf->atarp_fixcyls;
		else if (!ata_unitp->au_rpbuf->atarp_cap & ATAC_LBA_SUPPORT) {
			/*
			 * if the the sector/heads do not match that of the
			 * bios and the drive does not support LBA. We go ahead
			 * and advertise the bios geometry but use the physical
			 * geometry for sector translation.
			 */
			cmn_err(CE_WARN, "!Disk 0x%x,%d: BIOS geometry "
				"different from physical, and no LBA support.",
				ata_blkp->ab_data, targ);

		}

		/*
		 * cylinders are indexed from 0 therefore chs[0] - 1
		 * gets 1023 for a 1024 cylinders drive. The heads
		 * as stored in the bios represents heads - 1.
		 */
		ata_unitp->au_bioscyl = chs[0] - 1;
		ata_unitp->au_bioshd = chs[1] + 1;
		ata_unitp->au_biossec = chs[2];
#ifdef ATA_DEBUG
		if ((ata_debug & DINIT) &&
			(chs[0] != (ata_unitp->au_rpbuf->atarp_fixcyls - 1) ||
			chs[1] != ata_unitp->au_rpbuf->atarp_heads ||
			chs[2] != ata_unitp->au_rpbuf->atarp_sectors))
			printf("ata: using bios "
				"cyls %d -> %d  "
				"heads %d -> %d  "
				"secs %d -> %d\n",
				ata_unitp->au_rpbuf->atarp_fixcyls, chs[0] - 1,
				ata_unitp->au_rpbuf->atarp_heads, chs[1] + 1,
				ata_unitp->au_rpbuf->atarp_sectors, chs[2]);
#endif
	}
	if (ata_unitp->au_rpbuf->atarp_cap & ATAC_LBA_SUPPORT)
		ata_unitp->au_drive_bits |= ATDH_LBA;

	switch (ata_blkp->ab_dev_type[targ]) {
	case ATA_DEV_17:
		ata_unitp->au_17b = 1;
		/* FALLTHROUGH */
	case ATA_DEV_12:
		ata_unitp->au_atapi = 1;
	}

	devp->sd_inq = ata_blkp->ab_inqp[targ];
	devp->sd_dev = cdip;
	devp->sd_address.a_hba_tran = (scsi_hba_tran_t *)ctlobjp;
	devp->sd_address.a_target = (ushort)targ;
	devp->sd_address.a_lun = (u_char)lun;
	mutex_init(&devp->sd_mutex, "sd_mutex", MUTEX_DRIVER, NULL);

	ctlobjp->c_ops  = (struct ctl_objops *)&ata_objops;
	ctlobjp->c_data = (opaque_t)atap;
	ctlobjp->c_ext  = &(ctlobjp->c_extblk);
	ctlobjp->c_extblk.c_ctldip = mdip;
	ctlobjp->c_extblk.c_devdip = cdip;
	ctlobjp->c_extblk.c_targ   = targ;
	ctlobjp->c_extblk.c_blksz  = NBPSCTR;

	mutex_enter(&ata_blkp->ab_mutex);

	if (!ata_blkp->ab_link) {
		ata_blkp->ab_link = atap;
		atap->a_forw = atap;
		atap->a_back = atap;
	} else {
		atalinkp = ata_blkp->ab_link;

		atalinkp->a_back->a_forw  = atap;
		atap->a_back = atalinkp->a_back;
		atap->a_forw = atalinkp;
		atalinkp->a_back  = atap;
	}

	mutex_exit(&ata_blkp->ab_mutex);

	sprintf(name, "%d,%d", targ, lun);
	ddi_set_name_addr(cdip, name);
	ddi_set_driver_private(cdip, (caddr_t)devp);

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_initchild: <%d, %d> devp= 0x%x ctlobjp=0x%x\n",
			targ, lun, devp, ctlobjp);
	}
#endif
	return (DDI_SUCCESS);
}

static int
ata_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if ((strcmp(dname, "ata") == 0) || (strcmp(dname, "ide") == 0))
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}

static int
ata_probe(register dev_info_t *devi)
{
	int	ioaddr1;
	int	ioaddr2;
#if defined(_eisa_bus_supported)
	char	bus_type[16];
#endif
	int	len;

#ifdef ATA_DEBUG
	if (ata_debug & DENT)
		PRF("ataprobe: ata devi= 0x%x\n", devi);
#endif
	len = sizeof (int);
	if ((HBA_INTPROP(devi, "ioaddr1", &ioaddr1, &len) != DDI_SUCCESS) ||
	    (HBA_INTPROP(devi, "ioaddr2", &ioaddr2, &len) != DDI_SUCCESS))
		return (DDI_PROBE_FAILURE);

#if defined(_eisa_bus_supported)
	len = sizeof (bus_type);
	if (ddi_prop_op(DDI_DEV_T_ANY, devi, PROP_LEN_AND_VAL_BUF, 0,
		"device_type", (caddr_t)bus_type, &len) == DDI_PROP_SUCCESS) {

		/*
		 * if we are on an EISA bus system, check for DPT
		 * cards that conflict with ata IO addresses
		 */
		if (strncmp(bus_type, DEVI_EISA_NEXNAME, len) == 0)
			if (ata_detect_dpt(ioaddr1)) {
				cmn_err(CE_CONT, "?ata_probe(0x%x): I/O port "
					"conflict with EISA DPT HBA\n",
					ioaddr1);
				return (DDI_PROBE_FAILURE);
		}
	}
#endif

	if (ata_checkpresence(ioaddr1, ioaddr2) == 0)
		return (DDI_PROBE_FAILURE);

	if (HBA_INTPROP(devi, "irq13_share", &irq13_addr, &len) != DDI_SUCCESS)
		irq13_addr = 0;

	return (DDI_PROBE_SUCCESS);
}

static int
ata_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
	register struct	ata_blk *ata_blkp;
	int	i;

	switch (cmd) {
	case DDI_DETACH:
	{
		ata_blkp = (struct ata_blk *)ddi_get_driver_private(devi);
		if (!ata_blkp)
			return (DDI_SUCCESS);
		if (ata_blkp->ab_link)
			return (DDI_FAILURE);

		for (i = 0; i < ATA_MAXDRIVE; i++) {
			if (ata_blkp->ab_rpbp[i] == NULL)
				continue;
			kmem_free((caddr_t)ata_blkp->ab_rpbp[i],
				(sizeof (struct atarpbuf) +
				    sizeof (struct scsi_inquiry)));
		}
		ddi_remove_intr(devi, 0,
				(ddi_iblock_cookie_t)ata_blkp->ab_lkarg);
		mutex_destroy(&ata_blkp->ab_mutex);

		mutex_enter(&ata_global_mutex);
		ata_global_init--;
		if (ata_global_init == 0) {
			mutex_destroy(&ata_rmutex);
		}
		mutex_exit(&ata_global_mutex);

		kmem_free((caddr_t)ata_blkp, sizeof (*ata_blkp));

		ddi_prop_remove_all(devi);
		ddi_set_driver_private(devi, (caddr_t)NULL);
		return (DDI_SUCCESS);
	}
	default:
		return (EINVAL);
	}
}

static int
ata_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	struct ata_blk		*ata_blkp;
	int 			drive, dcount;
	ddi_iblock_cookie_t	tmp;

	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	ata_blkp = (struct ata_blk *)kmem_zalloc((unsigned)sizeof (*ata_blkp),
	    KM_NOSLEEP);
	if (!ata_blkp)
		return (DDI_FAILURE);

	ata_blkp->ab_dip = devi;
	ata_blkp->ab_status_flag &= ~ATA_OFFLINE; 	/* set online */
	if ((ata_propinit(ata_blkp) == DDI_FAILURE) ||
	    (ata_getedt(ata_blkp) == DDI_FAILURE)) {
		goto errout;
	}

	/*
	 * Establish and set the value for read and write multiple
	 * and the highest PIO mode.
	 */
	for (dcount = drive = 0; drive < 2; drive++) {
		if (ata_blkp->ab_dev_type[drive] != ATA_DEV_NONE) {
			if ((ata_set_rw_multiple(ata_blkp, drive) ==
						DDI_FAILURE)) {
				kmem_free(ata_blkp->ab_rpbp[drive],
					(sizeof (struct atarpbuf) +
					sizeof (struct scsi_inquiry)));
				ata_blkp->ab_dev_type[drive] = ATA_DEV_NONE;
			} else {
				dcount++;
			}
		}
	}

	if (dcount == 0)
		goto errout;

	/*
	 * Establish initial dummy interrupt handler.
	 * Get iblock cookie to initialize mutexes used in the
	 * real interrupt handler.
	 */
	if (ddi_add_intr(devi, 0, &tmp, (ddi_idevice_cookie_t *)0,
					ata_dummy_intr, (caddr_t)ata_blkp)) {
		cmn_err(CE_WARN, "ata_attach: cannot add intr");
		goto errout;
	}

	ata_blkp->ab_lkarg = (void *)tmp;

	mutex_init(&ata_blkp->ab_mutex, "ata mutex", MUTEX_DRIVER, (void *)tmp);

	ddi_remove_intr(devi, 0, tmp);
	/* Establish real interrupt handler */
	if (ddi_add_intr(devi, (u_int)0, &tmp, (ddi_idevice_cookie_t *)0,
					ata_intr, (caddr_t)ata_blkp)) {
		cmn_err(CE_WARN, "ata_attach: cannot add intr");
		goto errout;
	}

	mutex_enter(&ata_global_mutex);	/* protect multithreaded attach	*/
	if (ata_global_init == 0) {
		mutex_init(&ata_rmutex, "ATA Resource Mutex", MUTEX_DRIVER,
			(void *)tmp);
	}
	ata_global_init++;
	mutex_exit(&ata_global_mutex);

	ddi_set_driver_private(devi, (caddr_t)ata_blkp);
	/* This finally enables interrupts for this controller */
	ata_enable_intr(ata_blkp);
	ddi_report_dev(devi);

	return (DDI_SUCCESS);
errout:

	for (drive = 0; drive < 2; drive++)
		if (ata_blkp->ab_dev_type[drive] != ATA_DEV_NONE) {
			(void) kmem_free(ata_blkp->ab_rpbp[drive],
				(sizeof (struct atarpbuf) +
				sizeof (struct scsi_inquiry)));
		}
	(void) kmem_free((caddr_t)ata_blkp, sizeof (*ata_blkp));
	return (DDI_FAILURE);
}


/*
 *	Common controller object interface
 */
static struct cmpkt *
ata_pktalloc(register struct ata *atap, int (*callback)(), caddr_t arg)
{
	register struct ata_cmpkt *cmdp;
	int		kf;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_pktalloc (%x, %x, %x)\n", atap, callback, arg);
	}
#endif
	kf = GDA_KMFLAG(callback); /* determines whether or not we sleep */

	cmdp = (struct ata_cmpkt *)kmem_zalloc(sizeof (*cmdp),  kf);

	if (!cmdp) {
		if (callback != DDI_DMA_DONTWAIT)
			ddi_set_callback(callback, arg, &ata_cb_id);
		return ((struct cmpkt *)NULL);
	}

	cmdp->ac_pkt.cp_cdblen = 1;
	cmdp->ac_pkt.cp_cdbp   = (opaque_t)&cmdp->ac_cdb;
	cmdp->ac_pkt.cp_scbp   = (opaque_t)&cmdp->ac_scb;
	cmdp->ac_pkt.cp_scblen = 1;
	cmdp->ac_pkt.cp_ctl_private = (opaque_t)atap;
	cmdp->ac_bytes_per_block = atap->a_unitp->au_bytes_per_block;

#ifdef ATA_DEBUG
	if (ata_debug & DPKT) {
		PRF("ata_pktalloc:cmdp = 0x%x\n", cmdp);
	}
#endif
	return ((struct cmpkt *)cmdp);
}

/* ARGSUSED */
static void
ata_pktfree(struct ata *atap, struct cmpkt *pktp)
{
	register struct ata_cmpkt *cmdp = (struct ata_cmpkt *)pktp;

	ASSERT(!(cmdp->ac_flags & HBA_CFLAG_FREE)); /* check not free already */
	cmdp->ac_flags = HBA_CFLAG_FREE;

	kmem_free((void *)cmdp, sizeof (*cmdp));

	if (ata_cb_id)
		ddi_run_callback(&ata_cb_id);
}

/*
 * 1157317 sez that drivers shouldn't call bp_mapout(), as either
 * biodone() or biowait() will end up doing it, but after they
 * call bp->b_iodone(), which is a necessary sequence for
 * Online Disk Suite.  However, the DDI group wants to rethink
 * bp_mapin()/bp_mapout() and how they should behave in the
 * presence of layered drivers, etc.  For the moment, fix
 * the OLDS problem by removing the bp_mapout() call.
 */

#define	BUG_1157317

/* ARGSUSED */
static void
ata_memfree(struct ata *atap, struct cmpkt *pktp)
{
#if !defined(BUG_1157317)
	bp_mapout(pktp->cp_bp);
#endif
}

/* ARGSUSED */
static struct cmpkt *
ata_memsetup(struct ata *atap, struct cmpkt *pktp,
	struct buf *bp, int (*callback)(), caddr_t arg)
{
	bp_mapin(bp);
	((struct ata_cmpkt *)pktp)->ac_start_v_addr = bp->b_un.b_addr;
	return (pktp);
}

static struct cmpkt *
ata_iosetup(struct ata *atap, struct cmpkt *pktp)
{
	register struct ata_cmpkt *ata_pktp = (struct ata_cmpkt *)pktp;
	struct ata_unit	*unitp = atap->a_unitp;
	register ulong	sec_count;
	ulong	start_sec;
	ulong	resid;
	ulong	cyl;
	unchar	head;
	unchar	drvheads;
	unchar	drvsectors;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_iosetup (%x, %x)\n", atap, pktp);
	}
	if (ata_debug & DIO) {
		PRF("ata_iosetup: asking for start 0x%x count 0x%x\n",
				pktp->cp_srtsec, pktp->cp_bytexfer >> SCTRSHFT);
	}
#endif
/*	setup the task file registers					*/

	if ((ata_pktp->ac_cdb == DCMD_READ) && ATAPI(unitp)) {
		return (ata_read(atap, pktp));
	}
	drvheads = unitp->au_phhd;
	drvsectors = unitp->au_phsec;

	/* check for error retry */
	if (ata_pktp->ac_flags & HBA_CFLAG_ERROR) {
		ata_pktp->ac_bytes_per_block = NBPSCTR;
		sec_count = 1;
	} else {
		/*
		 * Limit request to ab_max_transfer sectors.
		 * The value is specified by the user in the
		 * max_transfer property. It must be in the range 1 to 256.
		 * When max_transfer is 0x100 it is bigger than 8 bits.
		 * The spec says 0 represents 256 so it should be OK.
		 */
		sec_count = min((pktp->cp_bytexfer >> SCTRSHFT),
					atap->a_blkp->ab_max_transfer);
	}

	/*
	 * Need to distinguish between old-style calls, and the
	 * new dadk ioctl's - check if (ata_pktp->cp_bp->av_forw == dadkp)
	 */

#ifdef	DADKIO_RWCMD_READ
	start_sec = CMPKT.cp_passthru ? RWCMDP->blkaddr : pktp->cp_srtsec;
#else
	start_sec = pktp->cp_srtsec;
#endif
	ata_pktp->ac_devctl = unitp->au_ctl_bits;
	ata_pktp->ac_v_addr = ata_pktp->ac_start_v_addr;
	ata_pktp->ac_count = (u_char)sec_count;
	if (unitp->au_drive_bits & ATDH_LBA) {
		ata_pktp->ac_sec = start_sec & 0xff;
		ata_pktp->ac_lwcyl = (start_sec >> 8) & 0xff;
		ata_pktp->ac_hicyl = (start_sec >> 16) & 0xff;
		ata_pktp->ac_hd = (start_sec >> 24) & 0xff;
		ata_pktp->ac_hd |= unitp->au_drive_bits;
	} else {
		resid = start_sec / drvsectors;
		head = resid % drvheads;
		cyl = resid / drvheads;
		ata_pktp->ac_sec = (start_sec % drvsectors) + 1;
		ata_pktp->ac_hd = head | unitp->au_drive_bits;
		ata_pktp->ac_lwcyl = cyl;  /* auto truncate to char */
				/* automatically truncate to char */
		ata_pktp->ac_hicyl = (cyl >> 8);
	}

#ifdef	DADKIO_RWCMD_READ
	if (CMPKT.cp_passthru) {
		switch (RWCMDP->cmd) {
		case DADKIO_RWCMD_READ:
			ata_pktp->ac_cmd = ATC_RDSEC;
			ata_pktp->ac_direction = AT_IN;
			break;
		case DADKIO_RWCMD_WRITE:
			ata_pktp->ac_cmd = ATC_WRSEC;
			ata_pktp->ac_direction = AT_OUT;
			break;
		}
		pktp->cp_bytexfer = pktp->cp_resid = RWCMDP->buflen;
	} else
#endif
	{
		pktp->cp_resid = pktp->cp_bytexfer = sec_count << SCTRSHFT;

		/* setup the task file registers */

		switch (ata_pktp->ac_cdb) {
		case DCMD_READ:
			ata_pktp->ac_cmd = unitp->au_rd_cmd;
			ata_pktp->ac_direction = AT_IN;
			break;
		case DCMD_WRITE:
			ata_pktp->ac_cmd = unitp->au_wr_cmd;
			ata_pktp->ac_direction = AT_OUT;
			break;
		case DCMD_RECAL:
			ata_pktp->ac_cmd = ATC_RECAL;
			ata_pktp->ac_direction = AT_NO_DATA;
			break;
		case DCMD_SEEK:
			ata_pktp->ac_cmd = ATC_SEEK;
			ata_pktp->ac_direction = AT_NO_DATA;
			break;
		case DCMD_RDVER:
			ata_pktp->ac_cmd = ATC_RDVER;
			ata_pktp->ac_direction = AT_NO_DATA;
			break;
		case DCMD_GETDEF:
			if (ATAPI(unitp)) {
				return (NULL);
			}
			ata_pktp->ac_devctl = unitp->au_ctl_bits;
			ata_pktp->ac_count = 1;
			ata_pktp->ac_hd = pktp->cp_bp->b_blkno |
					unitp->au_drive_bits;
			ata_pktp->ac_cmd = ATC_READDEFECTS;
			ata_pktp->ac_lwcyl = 0;
			ata_pktp->ac_hicyl = 0;
			ata_pktp->ac_direction = AT_IN;
			break;
		default:
#ifdef ATA_DEBUG
			if (ata_debug & DERR) {
				PRF("ATA_IOSETUP: unrecognized command 0x%x\n",
						ata_pktp->ac_cdb);
			}
#endif
			pktp = NULL;
			break;
		}
	}
	return (pktp);
}

static int
ata_transport(struct ata *atap, struct cmpkt *pktp)
{
	struct ata_blk		*ata_blkp = atap->a_blkp;
	struct ata_cmpkt	*ata_pktp = (struct ata_cmpkt *)pktp;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_transport (%x, %x)\n", atap, pktp);
	}
#endif
	mutex_enter(&ata_blkp->ab_mutex);
	if (ata_blkp->ab_status_flag & ATA_OFFLINE) {
		mutex_exit(&ata_blkp->ab_mutex);
		return (CTL_SEND_FAILURE);
	}

	if (pktp->cp_flags & CPF_NOINTR) {	/* polling */

		/*
		 * If we are in panic mode and drive is busy
		 * we must reset it so dump will work!
		 */
		if (ddi_in_panic()) {
			struct ata_unit	*ata_unitp = atap->a_unitp;
			/*
			 * Workaround: Manually fail transports
			 * if too many ata_wait() timeouts have
			 * occurred (20 seconds have passed).
			 * The real solution will be using clock
			 * interrupts to timeout.
			 */
			if (!ata_indump)
				ata_indump = 1;
			else if (ata_indump > 4)
				return (CTL_SEND_FAILURE);

			/*
			 * If the drive is busy at this point, we must
			 * stop anything that is not done and
			 * try to get the drive in a state that will
			 * let use do a crash dump.
			 */
			if (inb(ata_blkp->ab_altstatus) & ATS_BSY) {
				(void) ata_abort(atap, pktp);
				(void) ata_reset(atap, 0);
				/*
				 * Set the drive params back the way they
				 * were before the reset so the dump will
				 * go where it should on the disk.
				 */
				(void) ata_setpar(ata_blkp, ata_unitp->au_targ,
				ata_unitp->au_phhd, ata_unitp->au_phsec);
			}
		}

		/*
		 * NB.  The flow control module does not control the
		 * flow of the polling packets, and queuing such
		 * packets is meaningless as we should not return
		 * before their completion.
		 */
		if (!ata_indump && ata_blkp->ab_active != NULL) {
			/*
			 * H/W is tied up with the active packet.
			 * Have to wait until we can start the
			 * incoming polling packet.  i.e. Treat
			 * the active packet as a polling packet.
			 *
			 * As ata_blkp->ab_mutex is held, possible
			 * invokations of ata_intr() cannot interfere
			 * with this polling.
			 */
			if (ata_pollret(ata_blkp) == DDI_FAILURE) {
				ata_blkp->ab_active = NULL;
				mutex_exit(&ata_blkp->ab_mutex);
				return (CTL_SEND_BUSY);
			}
		}
		/*
		 * The controller is idle.
		 * Put the packet in ab_active....
		 */
		ata_blkp->ab_active = ata_pktp;
		/*
		 * ... and start it off
		 */
		if (ata_start(atap) == DDI_FAILURE) {
			ata_blkp->ab_active = NULL;
			mutex_exit(&ata_blkp->ab_mutex);
			return (CTL_SEND_FAILURE);
		}

		if (ata_pollret(ata_blkp) == DDI_FAILURE) {
			ata_blkp->ab_active = NULL;
			mutex_exit(&ata_blkp->ab_mutex);
			return (CTL_SEND_BUSY);
		}

		if (ata_start_next_cmd(atap) == DDI_FAILURE) {
			mutex_exit(&ata_blkp->ab_mutex);
			return (CTL_SEND_FAILURE);
		}
	} else {				/* interrupt packet */
		if (ata_blkp->ab_active == NULL) {
			/*
			 * The controller is idle.
			 * Put the packet in ab_active.
			 */
			ata_blkp->ab_active = ata_pktp;
			/*
			 * ... and start it off
			 */
			if (ata_start(atap) == DDI_FAILURE) {
				ata_blkp->ab_active = NULL;
				mutex_exit(&ata_blkp->ab_mutex);
				return (CTL_SEND_FAILURE);
			}
		} else {
			struct ata_unit	*ata_unitp;
			/*
			 * the controller is busy now so put the packet
			 * on au_head or au_last.
			 */
			ata_unitp = atap->a_unitp;
			if (ata_unitp->au_head == NULL)
				ata_unitp->au_head = ata_pktp;
			else {
				ASSERT(ata_unitp->au_last == NULL);
				ata_unitp->au_last = ata_pktp;
			}
		}
	}
	mutex_exit(&ata_blkp->ab_mutex);
	return (CTL_SEND_SUCCESS);
}

static int
ata_start(struct ata *atap)
{
	struct ata_blk		*ata_blkp = atap->a_blkp;
	struct ata_cmpkt	*ata_pktp = ata_blkp->ab_active;
	struct cmpkt		*pktp = (struct cmpkt *)ata_pktp;
	struct ata_unit		*unitp = atap->a_unitp;

	ASSERT(mutex_owned(&ata_blkp->ab_mutex));
	ASSERT(ata_pktp != NULL);

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_start (%x) status %x error %x\n", atap,
			inb(ata_blkp->ab_altstatus), inb(ata_blkp->ab_error));
	}
	if (ata_debug & DIO) {
		PRF("ATA_START: count - %x ", ata_pktp->ac_count);
		PRF("secnum - %x ", ata_pktp->ac_sec);
		PRF("lowcyl - %x ", ata_pktp->ac_lwcyl);
		PRF("hicyl  - %x ", ata_pktp->ac_hicyl);
		PRF("drv-hd - %x\n", ata_pktp->ac_hd);
		PRF("ctl    - %x ", ata_pktp->ac_devctl);
		PRF("           cmd - %x\n", ata_pktp->ac_cmd);
	}
#endif

	/*
	 * if ATA_BSY_WAIT_1 is set, wait for controller to not be busy,
	 * before issuing a command.  if ATA_BSY_WAIT_1 is not set,
	 * skip the wait.  this is for backwards compatibility with systems
	 * that may not correctly be dropping the busy bit before a command
	 * is issued
	 */
	if (ata_blkp->ab_timing_flags & ATA_BSY_WAIT_1) {
		if (ata_wait1(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY,
			500000)) {
#ifdef ATA_DEBUG
			if (ata_debug & DERR)
				PRF("ATA_START: controller is busy: "
					"status = 0x%x\n",
					inb(ata_blkp->ab_status));
#endif
			return (DDI_FAILURE);
		}
	}

	outb(ata_blkp->ab_drvhd, ata_pktp->ac_hd);
	outb(ata_blkp->ab_sect, ata_pktp->ac_sec);
	outb(ata_blkp->ab_count, ata_pktp->ac_count);
	outb(ata_blkp->ab_lcyl, ata_pktp->ac_lwcyl);
	outb(ata_blkp->ab_hcyl, ata_pktp->ac_hicyl);
	outb(ata_blkp->ab_feature, 0);

	/*
	 * This next one sets the controller in motion
	 */
	outb(ata_blkp->ab_cmd, ata_pktp->ac_cmd);

	if (ATAPI(unitp)) {
		if ((unitp->au_rpbuf->atarp_config & ATARP_DRQ_TYPE) !=
		    ATARP_DRQ_INTR) {
			/*
			 * For anything other than type ATARP_DRQ_INTR
			 * we don't receive an interrupt requesting
			 * the scsi packet, so we must poll for DRQ.
			 * Then send out the packet.
			 */
			if (ata_wait(ata_blkp->ab_status,
			    (ATS_BSY | ATS_DRQ), ATS_DRQ, ATS_BSY)) {
				return (DDI_FAILURE);
			}
			repoutsw(ata_blkp->ab_data,
				(ushort *)&ata_pktp->ac_scsi_pkt,
				sizeof (union scsi_cdb) >> 1);
		}
	} else {
		/*
		 * If there's data to go along with the command,
		 * send it now.
		 */
		if (ata_pktp->ac_direction == AT_OUT &&
		    ata_send_data(atap,
			    min(pktp->cp_resid,
			    ata_pktp->ac_bytes_per_block)) == DDI_FAILURE) {
			return (DDI_FAILURE);
		}
	}
	return (DDI_SUCCESS);
}

/* ARGSUSED */
static int
ata_abort(struct ata *atap, struct cmpkt *pktp)
{
#ifdef ATA_DEBUG
	if (ata_debug & DERR) {
		PRF("ata_abort (%x, %x)\n", atap, pktp);
	}
#endif
	return (DDI_SUCCESS);
}

static
void
ata_enable_intr(struct ata_blk *ata_blkp)
{
	outb(ata_blkp->ab_devctl, 8);
}

/* ARGSUSED */
static int
ata_reset(struct ata *atap, int level)
{
	register struct ata_blk	*ata_blkp = atap->a_blkp;

#ifdef ATA_DEBUG
	if (ata_debug & DERR) {
		PRF("ata_reset (%x, %d)\n", atap, level);
	}
#endif
	outb(ata_blkp->ab_devctl, 8 | AT_NIEN | AT_SRST);
	drv_usecwait(30000);
	ata_enable_intr(ata_blkp);
	drv_usecwait(30000);

	return (DDI_SUCCESS);
}

static	ulong
ata_stoh_long(ulong ai)
{
#if defined(_LITTLE_ENDIAN)
	return (ntohl(ai));
#elif defined(_BIG_ENDIAN)
	return (ai);
#else
#error Unknown endianess!
#endif
}

static int
ata_ioctl(struct ata *atap, int cmd, int a, int flag)
{
	struct ata_cmpkt *ata_pktp;
	caddr_t arg;
	struct tgdk_geom	*tg = (struct tgdk_geom *)a;
	struct ata_unit	*unitp = atap->a_unitp;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_ioctl (%x, %x, %x, %x)\n", atap, cmd, a, flag);
	}
#endif
	switch (cmd) {

	case DIOCTL_GETGEOM:
	case DIOCTL_GETPHYGEOM:
		if (ATAPI(unitp)) {
			struct scsi_capacity *cap = &unitp->au_capacity;
			u_long	sectors, secsize;
			/*
			 * have to interpret the scsi capacity
			 * read in from via a DCM_UPDATE_GEOM cmd
			 */
			tg->g_cyl = 1;
			tg->g_acyl = 0;
			tg->g_head = 1;
			secsize = ata_stoh_long(cap->lbasize);
#if defined(ATA_DEBUG)
			if (ata_debug & DIO)
				PRF("ata_ioctl: secsize %d\n", secsize);
#endif
			ASSERT((secsize & ~(NBPSCTR-1)) ==  ATA_CD_SECSIZ);
			sectors = ata_stoh_long(cap->capacity);
			if (cmd == DIOCTL_GETGEOM) {
				tg->g_secsiz = 512;
				tg->g_cap = tg->g_sec = sectors *4;
			} else {
				tg->g_secsiz = secsize;
				tg->g_cap = tg->g_sec = sectors;
			}
		} else {
			/*
			 * if a removeable media drive need to re-read ata
			 * inquiry data and reset the drive info.
			 */
			if (unitp->au_rpbuf->atarp_config & ATARP_REM_DRV)
				ata_get_drv_cfg(atap);
			tg->g_cyl = unitp->au_bioscyl;
			tg->g_head = unitp->au_bioshd;
			tg->g_sec = unitp->au_biossec;
			tg->g_acyl = unitp->au_acyl;
			tg->g_secsiz = 512;
			tg->g_cap = tg->g_cyl * tg->g_head * tg->g_sec;
		}
		return (0);
	}

	ata_pktp = (struct ata_cmpkt *)a;

	switch (cmd) {

	case DCMD_LOCK:
		return (ata_lock_unlock(atap, ata_pktp, 1));
	case DCMD_UNLOCK:
		return (ata_lock_unlock(atap, ata_pktp, 0));
	case DCMD_START_MOTOR:
		return (ata_start_stop_motor(atap, ata_pktp, 1));
	case DCMD_STOP_MOTOR:
		return (ata_start_stop_motor(atap, ata_pktp, 0));
	case DCMD_EJECT:
		return (ata_start_stop_motor(atap, ata_pktp, 2));
	case DCMD_UPDATE_GEOM:
		if (ATAPI(unitp))
			return (ata_read_capacity(atap, ata_pktp));
		else
			return (ata_ioctl(atap, DIOCTL_GETPHYGEOM, a, flag));
	case DCMD_GET_STATE:
		arg = (caddr_t)ata_pktp->ac_pkt.cp_bp->b_back;
		return (ata_get_state(atap, ata_pktp, (enum dkio_state *) arg));
	case DCMD_PAUSE:
		return (ata_cd_pause_resume(atap, ata_pktp, 0));
	case DCMD_RESUME:
		return (ata_cd_pause_resume(atap, ata_pktp, 1));
	case DCMD_PLAYMSF:
		arg = (caddr_t)ata_pktp->ac_pkt.cp_bp->b_back;
		return (ata_cd_play_msf(atap, ata_pktp, arg, flag));
	case DCMD_SUBCHNL:
		arg = (caddr_t)ata_pktp->ac_pkt.cp_bp->b_back;
		return (ata_cd_read_subchannel(atap, ata_pktp, arg, flag));
	case DCMD_READTOCHDR:
		arg = (caddr_t)ata_pktp->ac_pkt.cp_bp->b_back;
		return (ata_cd_read_tochdr(atap, ata_pktp, arg, flag));
	case DCMD_READTOCENT:
		arg = (caddr_t)ata_pktp->ac_pkt.cp_bp->b_back;
		return (ata_cd_read_tocentry(atap, ata_pktp, arg, flag));
	case DCMD_VOLCTRL:
		arg = (caddr_t)ata_pktp->ac_pkt.cp_bp->b_back;
		return (ata_cd_volume_ctrl(atap, ata_pktp, arg, flag));
	case DCMD_READMODE2:
	case DCMD_READOFFSET:
		return (ENOTTY);
	case DCMD_READMODE1:
	case DCMD_PLAYTRKIND:
	default:
#ifdef ATA_DEBUG
		if (ata_debug & DERR) {
			PRF("ata_ioctl: unsupported cmd 0x%x\n", cmd);
		}
#endif
		return (ENOTTY);
	}
}


/*
 *	controller dependent funtions
 */
static int
ata_propinit(register struct ata_blk *ata_blkp)
{
	register dev_info_t *devi;
	int	val;
	int	len;
	ushort	ioaddr1;
	ushort	ioaddr2;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_propinit (%x)\n", ata_blkp);
	}
#endif
	devi = ata_blkp->ab_dip;
	len = sizeof (int);
	if (HBA_INTPROP(devi, "ioaddr1", &val, &len) != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);
	ioaddr1   = (ushort)val;
	if (HBA_INTPROP(devi, "ioaddr2", &val, &len) != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);
	ioaddr2   = (ushort)val;
	if (HBA_INTPROP(devi, "drive0_pio_mode", &val, &len) !=
				DDI_PROP_SUCCESS)
		ata_blkp->ab_pio_mode[0] = -1;
	else
		ata_blkp->ab_pio_mode[0] = (int)val;
	if (HBA_INTPROP(devi, "drive1_pio_mode", &val, &len) !=
				DDI_PROP_SUCCESS)
		ata_blkp->ab_pio_mode[1] = -1;
	else
		ata_blkp->ab_pio_mode[1] = (int)val;
	if (HBA_INTPROP(devi, "drive0_block_factor", &val, &len) !=
				DDI_PROP_SUCCESS)
		ata_blkp->ab_block_factor[0] = -1;
	else {
		ata_blkp->ab_block_factor[0] = (int)val;
		if (ata_blkp->ab_block_factor[0] == 0)
			ata_blkp->ab_block_factor[0] = 1;
	}
	if (HBA_INTPROP(devi, "drive1_block_factor", &val, &len) !=
				DDI_PROP_SUCCESS)
		ata_blkp->ab_block_factor[1] = -1;
	else {
		ata_blkp->ab_block_factor[1] = (int)val;
		if (ata_blkp->ab_block_factor[1] == 0)
			ata_blkp->ab_block_factor[1] = 1;
	}
	if (HBA_INTPROP(devi, "timing_flags", &val, &len) !=
				DDI_PROP_SUCCESS)
		ata_blkp->ab_timing_flags = 0;
	else
		ata_blkp->ab_timing_flags = val;
	if (HBA_INTPROP(devi, "max_transfer", &val, &len) != DDI_PROP_SUCCESS)
		ata_blkp->ab_max_transfer = 0x100;
	else {
		ata_blkp->ab_max_transfer = (int)val;
		if (ata_blkp->ab_max_transfer < 1)
			ata_blkp->ab_max_transfer = 1;
		if (ata_blkp->ab_max_transfer > 0x100)
			ata_blkp->ab_max_transfer = 0x100;
	}

#ifdef ATA_DEBUG
	if (ata_debug & DIO) {
		PRF("ata_propinit ioaddr1 = 0x%x  ioaddr2 = 0x%x\n",
				ioaddr1, ioaddr2);
	}
#endif
	/*
	 * port addresses associated with ioaddr1
	 */
	ata_blkp->ab_data	= ioaddr1 + AT_DATA;
	ata_blkp->ab_error	= ioaddr1 + AT_ERROR;
	ata_blkp->ab_feature	= ioaddr1 + AT_FEATURE;
	ata_blkp->ab_count	= ioaddr1 + AT_COUNT;
	ata_blkp->ab_sect	= ioaddr1 + AT_SECT;
	ata_blkp->ab_lcyl	= ioaddr1 + AT_LCYL;
	ata_blkp->ab_hcyl	= ioaddr1 + AT_HCYL;
	ata_blkp->ab_drvhd	= ioaddr1 + AT_DRVHD;
	ata_blkp->ab_status	= ioaddr1 + AT_STATUS;
	ata_blkp->ab_cmd	= ioaddr1 + AT_CMD;

	/*
	 * port addresses associated with ioaddr2
	 */
	ata_blkp->ab_altstatus = ioaddr2 + AT_ALTSTATUS;
	ata_blkp->ab_devctl    = ioaddr2 + AT_DEVCTL;
	ata_blkp->ab_drvaddr   = ioaddr2 + AT_DRVADDR;

	return (DDI_SUCCESS);
}

/*
 * It returns DDI_SUCCESS only after ALL the data is transferred.
 * (ata_gen_intr_end() should be called only after this case)
 *
 * It returns DDI_FAILURE and sets rc to DDI_INTR_CLAIMED when partial
 * transfer was successful, still more transfer needs to be done.
 *
 * It returns DDI_FAILURE and sets rc to DDI_INTR_UNCLAIMED when there
 * was an actual failure in data transfer
 */

static int
ata_gen_intr(struct ata_blk *ata_blkp, int *rc)
{
	struct ata_cmpkt	*ata_pktp;
	struct cmpkt		*pktp;
	struct ata		*this_ata;
	int		bytes_this_transfer;
	int		bytes_next_transfer;
	u_char	status;
	u_char	error;
	u_char	intr;

	ASSERT(mutex_owned(&ata_blkp->ab_mutex));

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_gen_intr (%x, %x)\n", ata_blkp, rc);
	}
#endif

	*rc = DDI_INTR_CLAIMED;

	/*
	 * This clears the interrupt
	 */
	status = inb(ata_blkp->ab_status);
	if (status & ATS_BSY) {
#ifdef ATA_DEBUG
		if (ata_debug & DERR) {
			PRF("ata_gen_intr: interrupting, but busy!?!\n");
		}
#endif
		/*
		 * interrupts are not valid if the controller is busy
		 */
		*rc = DDI_INTR_UNCLAIMED;
		return (DDI_FAILURE);
	}

	ata_pktp = ata_blkp->ab_active;
	if (ata_pktp ==  NULL) {
#ifdef ATA_DEBUG
		if (ata_debug & DERR) {
			PRF("ata_gen_intr: interrupt but nothing active!?!\n");
			PRF("              count - %x ",
				inb(ata_blkp->ab_count));
			PRF("              secnum - %x ",
				inb(ata_blkp->ab_sect));
			PRF("              lowcyl - %x ",
				inb(ata_blkp->ab_lcyl));
			PRF("              hicyl  - %x ",
				inb(ata_blkp->ab_hcyl));
			PRF("              drv-hd - %x\n",
				inb(ata_blkp->ab_drvhd));
			PRF("              status - %x ",
				status);
			PRF("              error - %x ",
				inb(ata_blkp->ab_error));
			PRF("              drvaddr - %x\n",
				inb(ata_blkp->ab_drvaddr));
		}
#endif
		*rc = DDI_INTR_UNCLAIMED;
		return (DDI_FAILURE);
	}
	pktp = (struct cmpkt *)ata_pktp;
	this_ata = (struct ata *)pktp->cp_ctl_private;

#ifdef ATA_DEBUG
	if (ata_debug & DIO) {
		PRF("ATA_GEN_INTR: count - %x ", inb(ata_blkp->ab_count));
		PRF("secnum - %x ", inb(ata_blkp->ab_sect));
		PRF("lowcyl - %x ", inb(ata_blkp->ab_lcyl));
		PRF("hicyl  - %x ", inb(ata_blkp->ab_hcyl));
		PRF("drv-hd - %x\n", inb(ata_blkp->ab_drvhd));
		PRF("          status - %x ", status);
		PRF("error - %x ", inb(ata_blkp->ab_error));
		PRF("drvaddr - %x\n", inb(ata_blkp->ab_drvaddr));
	}
#endif

	if (ata_pktp->ac_atapi) {
		/*
		 * The atapi interrupt register (ata count) CoD and
		 * IO bits and status register DRQ bit combined
		 * define the state of the atapi packet command.
		 *   IO  DRQ  CoD
		 *   --  ---  ---
		 *    0    1   1  Ready for atapi (scsi) pkt
		 *    1    1   1  Future use
		 *    1    1   0  Data from device.
		 *    0    1   0  Data to device
		 *    1    0   1  Status ready
		 *
		 * There is a separate interrupt for each of phases.
		 */
		intr = inb(ata_blkp->ab_count);
		if (status & ATS_DRQ) {
			if ((intr & (ATI_COD | ATI_IO)) == ATI_COD) {
				/*
				 * send out scsi pkt
				 */
				repoutsw(ata_blkp->ab_data,
					(ushort *)&ata_pktp->ac_scsi_pkt,
					sizeof (union scsi_cdb) >> 1);
#ifdef ATA_DEBUG
				{
				    char *cp = (char *)&ata_pktp->ac_scsi_pkt;
				    int i;

				    if (ata_debug & DIO) {
					PRF("atapi scsi cmd ");
					for (i = 0; i < 12; i++) {
						PRF("0x%x ", *cp++);
					}
					PRF("\n");
				    }
				}
#endif
				return (DDI_FAILURE);
			}

			bytes_this_transfer =
					(int)(inb(ata_blkp->ab_hcyl) << 8)
					+ inb(ata_blkp->ab_lcyl);
			ASSERT(!(bytes_this_transfer & 1)); /* even bytes */
			if ((intr & (ATI_COD | ATI_IO)) == ATI_IO) {
				/*
				 * Data from device
				 */
				if (pktp->cp_resid >= bytes_this_transfer) {
#ifdef ATA_DEBUG
				    if (ata_debug & DIO) {
					PRF("read 0x%x ",
						bytes_this_transfer);
				    }
#endif
				    repinsw(ata_blkp->ab_data,
					(ushort *)ata_pktp->ac_v_addr,
					(bytes_this_transfer >> 1));
				    pktp->cp_resid -= bytes_this_transfer;
				    ata_pktp->ac_v_addr += bytes_this_transfer;
				} else {
				    if (pktp->cp_resid) {
					repinsw(ata_blkp->ab_data,
					    (ushort *)ata_pktp->ac_v_addr,
					    (pktp->cp_resid >> 1));
					bytes_this_transfer -=
					    pktp->cp_resid;
					ata_pktp->ac_v_addr += pktp->cp_resid;
					pktp->cp_resid = 0;
				    }
#ifdef ATA_DEBUG
				    if (ata_debug & DIO) {
					PRF("discard 0x%x ",
						bytes_this_transfer);
				    }
#endif
				    repinsw(ata_blkp->ab_data,
					ata_bit_bucket,
					(bytes_this_transfer >> 1));
				}
				return (DDI_FAILURE);
			}
			if ((intr & (ATI_COD | ATI_IO)) == 0) {
				/*
				 * Data to device
				 *
				 * Send data flow is quite different
				 * from regular ata.
				 */
				repoutsw(ata_blkp->ab_data,
				    (ushort *)ata_pktp->ac_v_addr,
				    (bytes_this_transfer >> 1));
				ata_pktp->ac_v_addr += bytes_this_transfer;
				pktp->cp_resid -= bytes_this_transfer;
				return (DDI_FAILURE);
			}
			/*
			 * Unsupported intr combination
			 */
			*rc = DDI_INTR_UNCLAIMED;
			return (DDI_FAILURE);
		} else {
			/*
			 * End of command - check status.
			 */
			if (status & ATS_ERR) {
				/*
				 * put the error status in the right place
				 */
				ata_pktp->ac_error = inb(ata_blkp->ab_error);
				ata_pktp->ac_status = status;
				pktp->cp_reason = CPS_CHKERR;
#ifdef ATA_DEBUG
				if ((ata_debug & DERR) &&
				    (!pktp->cp_passthru)) {
					PRF("atapi err e 0x%x s 0x%x c 0x%x",
						ata_pktp->ac_error, status,
						ata_pktp->ac_scsi_pkt.scc_cmd);
					PRF(" p 0x%x\n", ata_pktp);
				}
#endif
			} else {
#ifdef ATA_DEBUG
				if (ata_debug & DIO) {
					PRF("good status\n");
				}
#endif
				pktp->cp_reason = CPS_SUCCESS;
				ata_pktp->ac_start_v_addr = ata_pktp->ac_v_addr;
			}
			return (DDI_SUCCESS);
		}
	}
	/*
	 * non atatpi commands
	 */
	bytes_this_transfer = min(pktp->cp_resid,
				ata_pktp->ac_bytes_per_block);
	error = inb(ata_blkp->ab_error);
	if (ata_pktp->ac_direction == AT_IN) {
		if (!(status & ATS_ERR) || !(error & ATE_ABORT)) {
			/*
			 * do the read of the block
			 */
			if (ata_get_data(this_ata, bytes_this_transfer) ==
						DDI_FAILURE) {
				/*
				 * If the controller never presented the data
				 * and the error bit isn't set, there's a real
				 * problem.  Kill it now.
				 *
				 * If the error bit is set, drop through and let
				 * normal error recovery pick it up.
				 */
				ata_clear_queues(this_ata);
				return (DDI_FAILURE);
			}
		}
	}

	if (status & ATS_ERR) {
		/*
		 * put the error status in the right place
		 */
		ata_pktp->ac_error = error;
		ata_pktp->ac_status = status;
		pktp->cp_reason = CPS_CHKERR;
	} else {
		/*
		 * update counts...
		 * Only continue with the current packet if there
		 * was no error.
		 */
		ata_pktp->ac_v_addr += bytes_this_transfer;
		pktp->cp_resid -= bytes_this_transfer;
		pktp->cp_reason = CPS_SUCCESS;

		if (pktp->cp_resid) {
			if (ata_pktp->ac_direction == AT_OUT) {
				/*
				 * somehow get the parameters to do
				 * the write of the next block
				 */
				bytes_next_transfer = min(pktp->cp_resid,
						ata_pktp->ac_bytes_per_block);
				if (ata_send_data(this_ata,
						bytes_next_transfer) ==
							DDI_FAILURE) {
					ata_clear_queues(this_ata);
				}
			}
			/*
			 * This is really not a failure but it is convenient
			 * to return failure to prevent the interrupt
			 * routine from starting a new request
			 */
			return (DDI_FAILURE);
		}
		ata_pktp->ac_start_v_addr = ata_pktp->ac_v_addr;
	}
	return (DDI_SUCCESS);
}

/*
 * It should be called only after ata_gen_intr() returns DDI_SUCCESS
 */
static void
ata_gen_intr_end(struct ata_cmpkt *ata_pktp)
{
	struct cmpkt *pktp = (struct cmpkt *)ata_pktp;

	if (ata_pktp->ac_status & ATS_ERR) {
		/*
		 * translate the error here
		 */
		ata_translate_error(ata_pktp);
	} else {
		ata_pktp->ac_scb = DERR_SUCCESS;
	}
	/*
	 * callback
	 */
	(*pktp->cp_callback)(pktp);
}

static int
ata_pollret(struct ata_blk *ata_blkp)
{
	int rc;
	struct ata_cmpkt *active = ata_blkp->ab_active;

	ASSERT(mutex_owned(&ata_blkp->ab_mutex));

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_pollret (%x)\n", ata_blkp);
	}
#endif
	for (;;) {
		/*
		 * Wait for not busy
		 */
		if (ata_wait(ata_blkp->ab_altstatus, ATS_BSY, 0, ATS_BSY)) {
			active->ac_status |= (ATS_ERR | ATS_BSY);
			active->ac_error = inb(ata_blkp->ab_error);
			((struct cmpkt *)active)->cp_reason = CPS_CHKERR;
			return (DDI_FAILURE);
		}

		if (ata_gen_intr(ata_blkp, &rc) == DDI_SUCCESS) {
			break;
		} else {
			if (rc == DDI_INTR_UNCLAIMED) {
				return (DDI_FAILURE);
			}
		}
	}

	mutex_exit(&ata_blkp->ab_mutex);
	ata_gen_intr_end(ata_blkp->ab_active);
	mutex_enter(&ata_blkp->ab_mutex);

	return (DDI_SUCCESS);
}

/*	Autovector Interrupt Entry Point				*/
/* Dummy return to be used before mutexes has been initialized		*/
/* guard against interrupts from drivers sharing the same irq line	*/

/* ARGSUSED */
static u_int
ata_dummy_intr(caddr_t arg)
{
	return (DDI_INTR_UNCLAIMED);
}


static u_int
ata_intr(caddr_t arg)
{
	register struct ata_blk	*ata_blkp;
	struct ata_cmpkt	*ata_pktp;
	struct cmpkt		*pktp;
	struct ata		*this_ata;
	int	rc;
	char	irq13;
	int	our_interrupt;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_intr(0x%x)\n", arg);
	}
#endif

	ata_blkp = (struct ata_blk *)arg;

	/*
	 * When ata_indump is set we are doing a panic and the drive is in
	 * polled mode. No interrupts should occur, but if they do this is
	 * the safest thing to do with the interrupt.
	 */
	if (ata_indump)
		return (DDI_INTR_CLAIMED);

	mutex_enter(&ata_blkp->ab_mutex);

	if (irq13_addr != 0) {
		/* Check which (if either) IDE controller is interrupting */
		irq13 = inb(irq13_addr);
		switch (ata_blkp->ab_data) {
		case ATA_BASE0:
			our_interrupt = (irq13 & IDE_PRIMARY);
			if (our_interrupt)
				outb(irq13_addr, ~IDE_PRIMARY);
			break;
		case ATA_BASE1:
			our_interrupt = (irq13 & IDE_SECONDARY);
			if (our_interrupt)
				outb(irq13_addr, ~IDE_SECONDARY);
			break;
		default:
			cmn_err(CE_WARN, "ata_intr: interrupt from neither "
				"primary nor secondary IDE "
				"controller! (0x%x)\n",
				ata_blkp->ab_data);
			break;
		}

		if (! our_interrupt) {
			mutex_exit(&ata_blkp->ab_mutex);
			return (DDI_INTR_UNCLAIMED);
		}
	}

	if (ata_gen_intr(ata_blkp, &rc) == DDI_FAILURE) {
		mutex_exit(&ata_blkp->ab_mutex);
		return (rc);
	}

	ata_pktp = ata_blkp->ab_active;
	pktp = (struct cmpkt *)ata_pktp;
	this_ata = (struct ata *)pktp->cp_ctl_private;

	if (ata_start_next_cmd(this_ata) == DDI_FAILURE) {
		mutex_exit(&ata_blkp->ab_mutex);
		return (DDI_INTR_CLAIMED);
	}

	mutex_exit(&ata_blkp->ab_mutex);

	ata_gen_intr_end(ata_pktp);
	return (DDI_INTR_CLAIMED);
}

static int
ata_start_next_cmd(struct ata *this_ata)
{
	struct ata *next_ata;
	struct ata_blk  *ata_blkp = this_ata->a_blkp;
	int delay = 0;

	/*
	 * start next command
	 * find next drive with an outstanding command
	 */

	ASSERT(mutex_owned(&this_ata->a_blkp->ab_mutex));

	if (this_ata->a_blkp->ab_active->ac_cmd == ATC_READDEFECTS) {
		/*
		 * The last command was a GET_DEFECTS so
		 * delay the next command a bit
		 */
		delay = 1;
	}

	next_ata = this_ata;

	do {
		next_ata = next_ata->a_forw;

		if (next_ata->a_unitp->au_head != NULL) {
			if (ata_shuffle_up_and_start(next_ata, delay)
					== DDI_FAILURE) {
				ata_clear_queues(next_ata);
				return (DDI_FAILURE);
			}
			else
				return (DDI_SUCCESS);
		}
	} while (next_ata != this_ata);

	/*
	 * we went once all the way around the device chain
	 * and there was nothing to do
	 */

	ata_blkp->ab_active = NULL;

	return (DDI_SUCCESS);
}

static void
ata_translate_error(struct ata_cmpkt *ata_pktp)
{
	u_char	error;
	u_char	status;

	/*
	 * We already know there has been an error
	 * Translate it for the upper layers.
	 */
	error = ata_pktp->ac_error;
	status = ata_pktp->ac_status;

	if (ata_pktp->ac_atapi) {
		if (error & ATE_ILI)
			ata_pktp->ac_scb = DERR_ILI;
		else if (error & ATE_MCR)
			ata_pktp->ac_scb = DERR_MCR;
		else if (error & ATE_EOM)
			ata_pktp->ac_scb = DERR_EOM;
		else if (error & ATS_SENSE_KEY)
			ata_pktp->ac_scb =
			    ata_sense_table[(int)(error & ATS_SENSE_KEY)
			    >> ATS_SENSE_KEY_SHIFT];
		else if (status & ATS_BSY)
			ata_pktp->ac_scb = DERR_BUSY;
		else if (error & ATE_ABORT)
			ata_pktp->ac_scb = DERR_ABORT;
	} else { /* ata device */
		if (error & ATE_BBK)
			ata_pktp->ac_scb = DERR_BBK;
		else if (error & ATE_UNC)
			ata_pktp->ac_scb = DERR_UNC;
		else if (error & ATE_IDNF)
			ata_pktp->ac_scb = DERR_IDNF;
		else if (error & ATE_TKONF)
			ata_pktp->ac_scb = DERR_TKONF;
		else if (error & ATE_AMNF)
			ata_pktp->ac_scb = DERR_AMNF;
		else if (status & ATS_BSY)
			ata_pktp->ac_scb = DERR_BUSY;
		else if (status & ATS_DWF)
			ata_pktp->ac_scb = DERR_DWF;
		else /* ATE_ABORT or any unknown error			*/
			ata_pktp->ac_scb = DERR_ABORT;
	}
	ata_pktp->ac_flags |= HBA_CFLAG_ERROR;
}

/*
 * The interrupt routine nominated (atap) as being the next drive
 * to get attention.  Shuffle its queue and assign a new ab_active.
 * Then start it.
 */
static int
ata_shuffle_up_and_start(struct ata *atap, int delay)
{
	register struct ata_unit	*ata_unitp;

	ASSERT(mutex_owned(&atap->a_blkp->ab_mutex));

	if (delay)
		drv_usecwait(1000);
	ata_unitp = atap->a_unitp;
	atap->a_blkp->ab_active = ata_unitp->au_head;
	ata_unitp->au_head = ata_unitp->au_last;
	ata_unitp->au_last = NULL;

	return ((atap->a_blkp->ab_active == NULL) ? DDI_SUCCESS :
						    ata_start(atap));
}

#if defined(_eisa_bus_supported)
/* the following masks are all in little-endian byte order */
#define	DPT_MASK1		0xffff
#define	DPT_MASK2		0xffffff
#define	DPT_ID1			0x1412
#define	DPT_ID2			0x82a338

#define	TRUE			1
#define	FALSE			0

static int
ata_detect_dpt(uint ioaddr)
{
	struct {
		short slotnum;
		NVM_SLOTINFO slot;
		NVM_FUNCINFO func;
	} eisa_data;
	short func;
	short slot;
	uint bytes;
	uint boardid;
	NVM_PORT *ports;
	int eisa_nvm(char *data, KEY_MASK key_mask, ...);
	KEY_MASK key_mask = {0};

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_detect_dpt\n");
	}
#endif

	/*
	 * walk through all the eisa slots looking for DPT cards.  for
	 * each slot, get only one function, function 0
	 */
	key_mask.slot = TRUE;
	key_mask.function = TRUE;
	for (slot = 0; slot < 16; slot++) {
		bytes = eisa_nvm((char *)&eisa_data, key_mask, slot, 0);
		if (bytes == 0)
			continue;
		if (slot != eisa_data.slotnum)
			/* shouldn't happen */
			continue;

		/*
		 * check if found card is a DPT card.  if not, move
		 * on to next slot.  note that boardid will be in
		 * little-endian byte order
		 */
		boardid = eisa_data.slot.boardid[3] << 24;
		boardid |= eisa_data.slot.boardid[2] << 16;
		boardid |= eisa_data.slot.boardid[1] << 8;
		boardid |= eisa_data.slot.boardid[0];
		if ((boardid & DPT_MASK1) != DPT_ID1 &&
			(boardid & DPT_MASK2) != DPT_ID2)
				continue;

		/*
		 * check all the functions of the card in this slot, looking
		 * for port descriptions.  note that the info for function
		 * 0 is already in eisa_data.func (from above call to
		 * eisa_nvm)
		 */
		key_mask.board_id = FALSE;
		for (func = 1; ; func++) {
			if (eisa_data.func.fib.port != 0) {
				ports = eisa_data.func.un.r.port;
				if (ata_check_io_addr(ports, ioaddr)) {
#ifdef ATA_DEBUG
					if (ata_debug & DINIT)
						printf("DPT card is using IO "
							"address in the range "
							"%x to %x\n",
							ioaddr, ioaddr + 7);
#endif
					return (TRUE);
				}
			}

			/* get info for next function of card in this slot */
			bytes = eisa_nvm((char *)&eisa_data, key_mask, slot,
				func);

			/*
			 * no more functions for card in this slot so go
			 * on to next slot
			 */
			if (bytes == 0)
				break;
			if (slot != eisa_data.slotnum)
				/* shouldn't happen */
				break;
		}
	}

	return (FALSE);
}

static int
ata_check_io_addr(NVM_PORT *ports, uint ioaddr)
{
	int indx;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_check_io_addr\n");
	}
#endif
	/*
	 * do I/O port range check to see if IDE addresses are being
	 * used.  note that ports.count is really count - 1.
	 */
	for (indx = 0; indx < NVM_MAX_PORT; indx++) {
		if (ports[indx].address > ioaddr + 7 ||
			ports[indx].address + ports[indx].count < ioaddr) {
				if (ports[indx].more != 1)
					break;
		}
		else
			return (TRUE);
	}

	return (FALSE);
}
#endif

static int
ata_checkpresence(uint ioaddr1, uint ioaddr2)
{
	int	drive;
	ushort *secbuf;

	/* toggle reset bit to trigger a software reset	*/
	outb(ioaddr2 + AT_DEVCTL, 8|AT_SRST|AT_NIEN);
	drv_usecwait(1000);
	outb(ioaddr2 + AT_DEVCTL, 8|AT_NIEN);
	drv_usecwait(600000);

	secbuf = (ushort *)kmem_zalloc(NBPSCTR, KM_NOSLEEP);
	if (!secbuf) {
		return (0);
	}

	for (drive = 0; drive < 2; drive++) {
		/*
		 * load up with the drive number
		 */
		if (drive == 0) {
			outb(ioaddr1 + AT_DRVHD, ATDH_DRIVE0);
		} else {
			outb(ioaddr1 + AT_DRVHD, ATDH_DRIVE1);
		}
		outb(ioaddr1 + AT_FEATURE, 0);

		if (ata_drive_type(ioaddr1, secbuf) != ATA_DEV_NONE) {
			kmem_free(secbuf, NBPSCTR);
			return (1);
		}
	}

	kmem_free(secbuf, NBPSCTR);
	return (0);
}


/*ARGSUSED*/
static void
ata_get_drv_cfg(struct ata *atap)
{
}

static int
ata_id(uint ioaddr, ushort *buf)
{
#ifdef ATA_DEBUG
	struct atarpbuf	*rpbp = (struct atarpbuf *)buf;

	if (ata_debug & DINIT)
		printf("**** id controller: 0x%x ", ioaddr);
#endif
	outb(ioaddr + AT_CMD, ATC_READPARMS);

	/*
	 * According to the ATA specification, some drives may have
	 * to read the media to complete this command.  We need to
	 * make sure we give them enough time to respond.
	 */

	if (ata_wait1(ioaddr + AT_STATUS, ATS_DRQ | ATS_BSY,
	    ATS_DRQ, ATS_BSY, 100000)) {
#ifdef ATA_DEBUG
		if (ata_debug & DINIT)
			printf("failed drive did not settle.\n");
#endif
		return (1);
	}

	repinsw(ioaddr + AT_DATA, buf, NBPSCTR >> 1);

#ifdef ATA_DEBUG
	if (ata_debug & DINIT) {
		if ((inb(ioaddr + AT_STATUS) & ATS_ERR) == 0) {
			ata_byte_swap(rpbp->atarp_model,
				sizeof (rpbp->atarp_model));
			rpbp->atarp_model[sizeof (rpbp->atarp_model)-1] = '\0';
			printf("succeeded: %s\n",  rpbp->atarp_model);
			ata_byte_swap(rpbp->atarp_model,
				sizeof (rpbp->atarp_model));
		} else {
			printf("failed drive drive read error.\n");
		}
	}
#endif

	/*
	 * wait for the drive to recognize I've read all the data.  some
	 * drives have been observed to take as much as 3msec to finish
	 * sending the data; allow 5 msec just in case.
	 */
	if (ata_wait1(ioaddr + AT_STATUS, ATS_DRDY | ATS_BSY | ATS_DRQ,
		ATS_DRDY, ATS_BSY | ATS_DRQ, 500)) {
			return (1);
	}

	return (inb(ioaddr + AT_STATUS) & ATS_ERR);
}

static int
atapi_id(uint ioaddr, ushort *buf)
{
#ifdef ATA_DEBUG
	struct atarpbuf	*rpbp = (struct atarpbuf *)buf;

	if (ata_debug & DINIT)
		printf("* atapi controller: 0x%x ", ioaddr);
#endif
	outb(ioaddr + AT_CMD, ATC_PI_ID_DEV);
	if (ata_wait1(ioaddr + AT_STATUS, ATS_DRQ | ATS_BSY,
	    ATS_DRQ, ATS_BSY, 10000)) {
#ifdef ATA_DEBUG
	if (ata_debug & DINIT)
		printf("failed drive did not settle.\n");
#endif
		return (1);
	}
	drv_usecwait(10000);

	repinsw(ioaddr + AT_DATA, buf, NBPSCTR >> 1);
#ifdef ATA_DEBUG
	if (ata_debug & DINIT) {
		if (inb(ioaddr + AT_STATUS) & ATS_ERR)
			printf("failed drive drive read error.\n");
	} else {
		ata_byte_swap(rpbp->atarp_model, sizeof (rpbp->atarp_model));
		rpbp->atarp_model[sizeof (rpbp->atarp_model)-1] = '\0';
		printf("succeeded: %s\n",  rpbp->atarp_model);
		ata_byte_swap(rpbp->atarp_model, sizeof (rpbp->atarp_model));
	}
#endif

	/*
	 * wait for the drive to recognize I've read all the data.  some
	 * drives have been observed to take as much as 3msec to finish
	 * sending the data; allow 5 msec just in case.
	 */
	if (ata_wait1(ioaddr + AT_STATUS, ATS_DRDY | ATS_BSY | ATS_DRQ,
		ATS_DRDY, ATS_BSY | ATS_DRQ, 500)) {
			return (1);
	}

	return (inb(ioaddr + AT_STATUS) & ATS_ERR);
}

/*
 * Check for 1.2 atapi spec device (eg cdrom)
 *
 * Note, that 1.7B atapi units are seen as regular disks but
 * fail the initial identify drive (ATC_READPARMS) command.
 */
static int
ata_check_for_atapi_12(uint ioaddr)
{
#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_check_for_atapi_12(0x%x)\n", ioaddr);
	}
#endif

	return ((inb(ioaddr + AT_HCYL) == ATAPI_SIG_HI) &&
		(inb(ioaddr + AT_LCYL) == ATAPI_SIG_LO));
}

/*
 * Wait for a register of a controller to achieve a
 * specific state.  Arguments are a mask of bits we care about,
 * and two sub-masks.  To return normally, all the bits in the
 * first sub-mask must be ON, all the bits in the second sub-
 * mask must be OFF.  If 5 seconds pass without the controller
 * achieving the desired bit configuration, we return 1, else 0.
 */
static int
ata_wait(register ushort port, ushort mask, ushort onbits, ushort offbits)
{
	register int i;
	register ushort maskval;

	i = 400000;	/* 4 seconds worth */
	while (i > 0) {
		maskval = inb(port) & mask;
		if (((maskval & onbits) == onbits) &&
		    ((maskval & offbits) == 0))
			return (0);
		drv_usecwait(10);
		i--;
	}
	/*
	 * Workaround: manually keep track of timeouts,
	 * and give up at ata_transport() level if we are
	 * hopelessly out of sync. The real solution will be
	 * to drop spl() and use clock() to break us out.
	 */
	if (ata_indump)
		ata_indump++;

	return (1);
}

/*
 * Similar to ata_wait but the timeout is varaible.
 */
static int
ata_wait1(register ushort port, ushort mask, ushort onbits, ushort offbits,
    int interval)
{
	register int i;
	register ushort maskval;

	for (i = interval; i; i--) {
		maskval = inb(port) & mask;
		if (((maskval & onbits) == onbits) &&
		    ((maskval & offbits) == 0))
			return (0);
		drv_usecwait(10);
	}
	return (1);
}

static int
ata_send_data(struct ata *atap, int count)
{
	register struct ata_cmpkt	*ata_pktp = atap->a_blkp->ab_active;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_send_data (%x, %x)\n", atap, count);
	}
#endif
	if (ata_wait(atap->a_blkp->ab_altstatus, ATS_DRQ, ATS_DRQ, 0)) {
		PRF("ATA_SEND_DATA - NOT READY\n");
		return (DDI_FAILURE);
	}
	/*
	 * copy count bytes from ata_pktp->v_addr to the data port
	 */
#ifdef ATA_DEBUG
	if (ata_debug & DIO) {
		PRF("port = %x addr = %x count = %x\n",
			atap->a_blkp->ab_data,
			ata_pktp->ac_v_addr,
			count);
	}
#endif
	repoutsw(atap->a_blkp->ab_data, (ushort *)ata_pktp->ac_v_addr,
			(count >> 1));

	return (DDI_SUCCESS);
}

static int
ata_get_data(struct ata *atap, int count)
{
	register struct ata_cmpkt	*ata_pktp = atap->a_blkp->ab_active;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_get_data (%x, %x)\n", atap, count);
	}
#endif
	if (ata_wait(atap->a_blkp->ab_altstatus, ATS_DRQ, ATS_DRQ, 0)) {
		PRF("ATA_GET_DATA - NOT READY\n");
		return (DDI_FAILURE);
	}
	/*
	 * copy count bytes from the data port to ata_pktp->ac_v_addr
	 */
#ifdef ATA_DEBUG
	if (ata_debug & DIO) {
		PRF("port = %x addr = %x count = %x\n",
			atap->a_blkp->ab_data,
			ata_pktp->ac_v_addr,
			count);
	}
#endif
	repinsw(atap->a_blkp->ab_data, (ushort *)ata_pktp->ac_v_addr,
			(count >> 1));
	return (DDI_SUCCESS);
}

static int
ata_getedt(struct ata_blk *ata_blkp)
{
	struct scsi_inquiry *inqp;
	ushort *secbuf;
	struct atarpbuf	*rpbp;
	int drive, dcount;
#ifdef ATA_DEBUG
	int i;
	char buf[41];

	if (ata_debug & DENT) {
		PRF("ata_getedt (%x)\n", ata_blkp);
	}
#endif
	/* toggle reset bit to trigger a software reset		*/
	outb(ata_blkp->ab_devctl, 8|AT_SRST|AT_NIEN);
	drv_usecwait(1000);
	outb(ata_blkp->ab_devctl, 8|AT_NIEN);
	drv_usecwait(600000);

	secbuf = (ushort *)kmem_zalloc(NBPSCTR, KM_NOSLEEP);
	if (!secbuf) {
		return (DDI_FAILURE);
	}

	for (dcount = drive = 0; drive < 2; drive++) {
		if (!(rpbp = (struct atarpbuf *)kmem_zalloc(
				(sizeof (struct atarpbuf) +
				sizeof (struct scsi_inquiry)), KM_NOSLEEP))) {
			kmem_free(secbuf, NBPSCTR);
			return (DDI_FAILURE);
		}
		inqp = (struct scsi_inquiry *)(rpbp + 1);

		/*
		 * load up with the drive number
		 */
		if (drive == 0) {
			outb(ata_blkp->ab_drvhd, ATDH_DRIVE0);
		} else {
			outb(ata_blkp->ab_drvhd, ATDH_DRIVE1);
		}
		outb(ata_blkp->ab_feature, 0);

		ata_blkp->ab_dev_type[drive] =
		    ata_drive_type((uint)ata_blkp->ab_data, secbuf);

		if (ata_blkp->ab_dev_type[drive] == ATA_DEV_NONE) {
			kmem_free(rpbp, (sizeof (struct atarpbuf) +
					    sizeof (struct scsi_inquiry)));
			continue;
		}
		dcount++;
		bcopy((caddr_t)secbuf, (caddr_t)rpbp,
				sizeof (struct atarpbuf));

		ata_blkp->ab_rpbp[drive] = rpbp;
		ata_blkp->ab_inqp[drive] = inqp;

		ata_byte_swap(rpbp->atarp_drvser, sizeof (rpbp->atarp_drvser));
		ata_byte_swap(rpbp->atarp_fw, sizeof (rpbp->atarp_fw));
		ata_byte_swap(rpbp->atarp_model, sizeof (rpbp->atarp_model));

#ifdef	ATA_DEBUG
		strncpy(buf, rpbp->atarp_model, sizeof (rpbp->atarp_model));
		buf[sizeof (rpbp->atarp_model)-1] = '\0';
		/* truncate model */
		for (i = sizeof (rpbp->atarp_model) - 2; buf[i] == ' '; i--) {
			buf[i] = '\0';
		}
		PRF("ata_getedt model %s, targ %d, stat %x, err %x\n",
			buf,
			drive,
			inb(ata_blkp->ab_status),
			inb(ata_blkp->ab_error));
		PRF("	cfg 0x%x, cyl %d, hd %d, sec/trk %d\n",
			rpbp->atarp_config,
			rpbp->atarp_fixcyls,
			rpbp->atarp_heads,
			rpbp->atarp_sectors);
		PRF("	mult1 0x%x, mult2 0x%x, dwcap 0x%x, cap 0x%x\n",
			rpbp->atarp_mult1,
			rpbp->atarp_mult2,
			rpbp->atarp_dwcap,
			rpbp->atarp_cap);
		PRF("	piomode 0x%x, dmamode 0x%x, advpiomode 0x%x\n",
			rpbp->atarp_piomode,
			rpbp->atarp_dmamode,
			rpbp->atarp_advpiomode);
		PRF("	minpio %d, minpioflow %d",
			rpbp->atarp_minpio,
			rpbp->atarp_minpioflow);
		PRF(", valid 0x%x, dwdma 0x%x\n",
			rpbp->atarp_validinfo,
			rpbp->atarp_dworddma);
#endif
		drv_usecwait(10000);
		inb(ata_blkp->ab_status);
		inb(ata_blkp->ab_error);
	}
	kmem_free(secbuf, NBPSCTR);
	if (dcount == 0)
		return (DDI_FAILURE);

	for (dcount = drive = 0; drive < 2; drive++) {

		if ((rpbp = ata_blkp->ab_rpbp[drive]) == NULL) {
			continue; /* no drive here */
		}
		inqp = ata_blkp->ab_inqp[drive];

		switch (ata_blkp->ab_dev_type[drive]) {
		case ATA_DEV_DISK:
			/*
			 * feed some of the info back in a set_params call.
			 */
			if (ata_setpar(ata_blkp, drive, rpbp->atarp_heads,
				rpbp->atarp_sectors) == DDI_FAILURE) {
				/*
				 * there should have been a drive here but it
				 * didn't respond properly. It stayed BUSY.
				 */
				kmem_free(rpbp, (sizeof (struct atarpbuf) +
					    sizeof (struct scsi_inquiry)));
				ata_blkp->ab_dev_type[drive] = ATA_DEV_NONE;
				continue;
			}
			ata_fake_inquiry(rpbp, inqp);
			break;

		case ATA_DEV_12:
		case ATA_DEV_17:
			if (!ata_inquiry(ata_blkp, inqp, drive)) {
				kmem_free(rpbp, (sizeof (struct atarpbuf) +
					    sizeof (struct scsi_inquiry)));
				ata_blkp->ab_dev_type[drive] = ATA_DEV_NONE;
				continue;
			}
			/*
			 * Kludge. The NEC CDR-260R cdrom drive has the
			 * configuration word (atarp_config) and other
			 * bit fields byte swapped.
			 * The only field used by atapi is the
			 * configuration word. Other information in atapi
			 * is picked up from the scsi inquiry packet
			 * which appears to be correct.
			 *
			 * We have to byte swap the configuration back
			 * to normal. There are no indications that
			 * the word is byte swapped. However, we know
			 * That the device type must currently be a cdrom (5)
			 * so we can search for that. See the atapi 1.2 spec.
			 */

			if ((rpbp->atarp_config & ATARP_DEV_TYPE) !=
			    ATARP_DEV_CDR) {
				ata_byte_swap((char *)&rpbp->atarp_config, 2);
			}

			/*
			 * Check for unsupported future standard.
			 */
			if ((rpbp->atarp_config & ATARP_PKT_SZ) !=
			    ATARP_PKT_12B) {
				PRF("Only 12 byte ATAPI packets supported ");
				PRF("ioaddr 0x%x, drive %d\n",
				    ata_blkp->ab_data, drive);
				kmem_free(rpbp, (sizeof (struct atarpbuf) +
					    sizeof (struct scsi_inquiry)));
				ata_blkp->ab_dev_type[drive] = ATA_DEV_NONE;
				continue;
			}
			break;
		default:
			PRF("Unknown IDE attachment at 0x%x.\n",
					ata_blkp->ab_cmd - AT_CMD);
			continue;
		}
		dcount++;
	}
#ifdef ATA_DEBUG
	if (ata_debug & DINIT)
		printf("**** probed %d device%s 0x%x\n",
			dcount, dcount == 1 ? "." : "s.",
			ata_blkp->ab_cmd - AT_CMD);
#endif

	return (dcount ? DDI_SUCCESS : DDI_FAILURE);

}

/*
 * ata_drive_type()
 *
 * The timeout values and exact sequence of checking is critical
 * especially for atapi device detection, and should not be changed lightly.
 * This algorithm posted to the atapi reflector "is the most reliable in
 * detecting the presense of ata and atapi devices". Its been tested
 * on "virtually every cdrom drive vendor in the world"
 * The only enhancement has been to additionally check for atapi 1.7B
 * spec units.
 */
static u_char
ata_drive_type(uint ioaddr, ushort *secbuf)
{
	if (ata_wait1(ioaddr + AT_STATUS,
	    (ATS_BSY | ATS_DRDY | ATS_DSC | ATS_ERR),
	    (ATS_DRDY | ATS_DSC), (ATS_BSY | ATS_ERR), 100000)) {
		/*
		 * No disk, check for 1.2 atapi unit.
		 * 1.2 spec allows status to be 0x00 or 0x10 after reset
		 */
		if (ata_check_for_atapi_12(ioaddr) &&
		    ((inb(ioaddr + AT_STATUS) & ~ATS_DSC) == 0)) {
			if (atapi_id(ioaddr, secbuf) == 0) {
				return (ATA_DEV_12);
			}
		}

		return (ATA_DEV_NONE);
	}

	if (ata_id(ioaddr, secbuf) != 0 /* fails */) {
		if (ata_check_for_atapi_12(ioaddr)) {
			if (atapi_id(ioaddr, secbuf) == 0) {
				return (ATA_DEV_12);
			}
		} else {
			/*
			 * Check for old (but prevalent) atapi 1.7B
			 * spec device, the only known example is the
			 * NEC CDR-260 not 260R which is atapi 1.2
			 * compliant). This device has no signature
			 * and requires conversion from hex to BCD
			 * for some scsi audio commands.
			 */
			if (atapi_id(ioaddr, secbuf) == 0) {
				return (ATA_DEV_17);
			}
		}
	} else {
		return (ATA_DEV_DISK);
	}
	return (ATA_DEV_NONE);
}

/*
 * Drive set params command.
 */
static int
ata_setpar(struct ata_blk *ata_blkp, int drive, int heads, int sectors)
{
	outb(ata_blkp->ab_drvhd, (heads - 1) |
		(drive == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1));
	outb(ata_blkp->ab_count, sectors);
	outb(ata_blkp->ab_cmd, ATC_SETPARAM);
	if (ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY))
		return (DDI_FAILURE);
	return (DDI_SUCCESS);
}

static void
ata_byte_swap(char *buf, int n)
{
	int	i;
	char	c;

	for (i = 0; i < n; i += 2) {
		c = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = c;
	}
}

static void
ata_fake_inquiry(struct atarpbuf *rpbp, struct scsi_inquiry *inqp)
{
	if (rpbp->atarp_config & ATARP_REM_DRV)	/* ide removable bit */
		inqp->inq_rmb = 1;		/* scsi removable bit */
	strncpy(inqp->inq_vid, "Gen-ATA ", sizeof (inqp->inq_vid));
	inqp->inq_dtype = DTYPE_DIRECT;
	inqp->inq_qual = DPQ_POSSIBLE;
	strncpy(inqp->inq_pid, rpbp->atarp_model, sizeof (inqp->inq_pid));
	strncpy(inqp->inq_revision, rpbp->atarp_fw,
				sizeof (inqp->inq_revision));
}

#ifdef PIO_SUPPORT
static int
ata_set_piomode(struct ata_blk *ata_blkp, int drive)
{
	int	usermax, drivemax, piomode, i, laststat;
	int	bitpattern;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_set_piomode (%x, %d) ", ata_blkp, drive);
	}
#endif
	drivemax = 2;
	bitpattern = ata_blkp->ab_rpbp[drive]->atarp_advpiomode;
	while (bitpattern & 1) {
		drivemax++;
		bitpattern >>= 1;
	}
	usermax = ata_blkp->ab_pio_mode[drive];
	if (drivemax > 2 && usermax > 3) {
		outb(ata_blkp->ab_drvhd, drive == 0 ?
					ATDH_DRIVE0 : ATDH_DRIVE1);
		if (drivemax > usermax)
			piomode = usermax;
		else
			piomode = drivemax;
		outb(ata_blkp->ab_count, piomode|FC_PIO_MODE);
		outb(ata_blkp->ab_feature, SET_TFER_MODE);
		outb(ata_blkp->ab_cmd, ATC_SET_FEAT);
		if (ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY)) {
			return (DDI_FAILURE);
		}
		for (i = 0; i < ATA_LOOP_CNT; i++) {
			if (((laststat = inb(ata_blkp->ab_status)) &
			    (ATS_DRDY | ATS_ERR)) != 0)
				break;
			drv_usecwait(10);
		}
		if (i == ATA_LOOP_CNT || laststat & ATS_ERR)
			return (DDI_FAILURE);
	}
}
#endif

static int
ata_set_rw_multiple(struct ata_blk *ata_blkp, int drive)
{
	int	i;
	int	laststat;
	int	size;
	int	accepted_size = -1;
	int	remembered_size = 1;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_set_rw_multiple (%x, %d) ", ata_blkp, drive);
	}
#endif
	if (ata_blkp->ab_dev_type[drive] != ATA_DEV_DISK) {
		/*
		 * Can't set RW multiple for atapi devices
		 */
		return (DDI_SUCCESS);
	}
	/*
	 * Assume we're going to use read/write multiple until the controller
	 * says it doesn't understang them.
	 */
	ata_blkp->ab_rd_cmd[drive] = ATC_RDMULT;
	ata_blkp->ab_wr_cmd[drive] = ATC_WRMULT;

	/*
	 * set drive number
	 */
	outb(ata_blkp->ab_drvhd, drive == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1);
	for (size = 32; size > 0 && accepted_size == -1; size >>= 1) {
		outb(ata_blkp->ab_count, size);
		/* send the command */
		outb(ata_blkp->ab_cmd, ATC_SETMULT);
		if (ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY)) {
			/*
			 * there should have been a drive here but it
			 * didn't respond properly. It stayed BUSY.
			 * complete failure!
			 */
			return (DDI_FAILURE);
		}
		/* Wait for DRDY or error status */
		for (i = 0; i < ATA_LOOP_CNT; i++) {
			if (((laststat = inb(ata_blkp->ab_status)) &
			    (ATS_DRDY | ATS_ERR)) != 0)
				break;
			drv_usecwait(10);
		}
		if (i == ATA_LOOP_CNT) {
			/*
			 * Didn't get ready OR error...  complete failure!
			 * there should have been a drive here but it
			 * didn't respond properly. It didn't set ERR or DRQ.
			 */
			return (DDI_FAILURE);
		}

		/* See if DRQ or error */
		if (laststat & ATS_ERR) {

			/*
			 * there should have been a drive here but it
			 * didn't respond properly. There was an error.
			 * Try the next value.
			 */
			continue;
		}
		/*
		 * Got ready.. use the value that worked.
		 */
		accepted_size = size;
	}
	if (accepted_size > 1) {
		remembered_size = accepted_size;
		accepted_size >>= 1;
		/*
		 * Allow a user specified block factor to override the
		 * system chosen value.
		 * Only allow the user to reduce the value.
		 * -1 indicates the user didn't specify anything
		 */
		if ((ata_blkp->ab_block_factor[drive] != -1) &&
			(ata_blkp->ab_block_factor[drive] < accepted_size))
			accepted_size = ata_blkp->ab_block_factor[drive];
	}

	if (accepted_size == -1 || accepted_size == 1) {
		/*
		 * if its -1
		 * None of the values worked...
		 * the controller responded correctly though so it probably
		 * doesn't support the read/write multiple commands.
		 *
		 * if its 1
		 * There's no benefit to using multiple commands.
		 * May as well stick with the simple stuff.
		 */

		ata_blkp->ab_rd_cmd[drive] = ATC_RDSEC;
		ata_blkp->ab_wr_cmd[drive] = ATC_WRSEC;
		ata_blkp->ab_block_factor[drive] = 1;
		return (DDI_SUCCESS);
	}
	outb(ata_blkp->ab_count, accepted_size);
	outb(ata_blkp->ab_cmd, ATC_SETMULT);
	if (ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY)) {
		/*
		 * there should have been a drive here but it
		 * didn't respond properly. It stayed BUSY.
		 */
		return (DDI_FAILURE);
	}
	/* Wait for DRDY or error status */
	for (i = 0; i < ATA_LOOP_CNT; i++) {
		if (((laststat = inb(ata_blkp->ab_status)) &
		    (ATS_DRDY | ATS_ERR)) != 0)
			break;
		drv_usecwait(10);
	}
	if (i == ATA_LOOP_CNT) {
		/*
		 * Didn't get ready OR error...  complete failure!
		 * there should have been a drive here but it
		 * didn't respond properly. It didn't set ERR or DRQ.
		 */
		return (DDI_FAILURE);
	}

	/* See if DRQ or error */
	if (laststat & ATS_ERR) {
		/*
		 * there should have been a drive here but it
		 * didn't respond properly. There was an error.
		 * This is strange because the value we're using is
		 * either remembered_size / 2 or an unknown size
		 * specified in the .conf file.
		 *
		 * Just cram remembered_size back into the controller and hope
		 * for the best.
		 */
		accepted_size = remembered_size;
		outb(ata_blkp->ab_count, accepted_size);
		outb(ata_blkp->ab_cmd, ATC_SETMULT);
		if (ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY))
			/*
			 * there should have been a drive here but it
			 * didn't respond properly. It stayed BUSY.
			 */
			return (DDI_FAILURE);
		/* Wait for DRDY or error status */
		for (i = 0; i < ATA_LOOP_CNT; i++) {
			if (((laststat = inb(ata_blkp->ab_status)) &
			    (ATS_DRDY | ATS_ERR)) != 0)
				break;
			drv_usecwait(10);
		}
		if (i == ATA_LOOP_CNT) {
			/*
			 * Didn't get ready OR error...  complete failure!
			 * there should have been a drive here but it
			 * didn't respond properly. It didn't set ERR or DRQ.
			 */
			return (DDI_FAILURE);
		}

		/* See if DRQ or error */
		if (laststat & ATS_ERR) {
			/*
			 * there should have been a drive here but it
			 * didn't respond properly. There was an error.
			 */
			ata_blkp->ab_rd_cmd[drive] = ATC_RDSEC;
			ata_blkp->ab_wr_cmd[drive] = ATC_WRSEC;
			ata_blkp->ab_block_factor[drive] = 1;
			return (DDI_SUCCESS);
		}
		/*
		 * Got ready.. use the value that worked.
		 */
	}
	/*
	 * Got ready.. the multiple read/write commands are programmed
	 */
#ifdef	ATA_DEBUG
		PRF("setting block factor for drive %d to %d\n",
					drive, accepted_size);
#endif
	ata_blkp->ab_block_factor[drive] = accepted_size;
	return (DDI_SUCCESS);
}

static void
ata_clear_queues(struct ata *atap)
{
	kmutex_t		*mutex;
	struct ata_cmpkt	*ata_pktp;
	struct ata *this_ata = atap;

	ASSERT(mutex_owned(&atap->a_blkp->ab_mutex));

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_clear_queues (%x)\n", atap);
	}
#endif

	/*
	 * nack the active request
	 */
	atap->a_blkp->ab_status_flag |= ATA_OFFLINE;
	mutex = &atap->a_blkp->ab_mutex;

	ata_pktp = atap->a_blkp->ab_active;
	atap->a_blkp->ab_active = NULL;
	ata_nack_packet(ata_pktp, mutex);

	/*
	 * now nack all queued requests
	 */

	this_ata = atap;

	do {
		ata_pktp = this_ata->a_unitp->au_head;
		this_ata->a_unitp->au_head = NULL;
		ata_nack_packet(ata_pktp, mutex);

		ata_pktp = this_ata->a_unitp->au_last;
		this_ata->a_unitp->au_last = NULL;
		ata_nack_packet(ata_pktp, mutex);

		this_ata = this_ata->a_forw;
	} while (this_ata != atap);
}

static void
ata_nack_packet(struct ata_cmpkt *ata_pktp, kmutex_t *mutex)
{
	struct cmpkt		*pktp;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_nack_packet (%x)\n", ata_pktp);
	}
#endif
	if (ata_pktp == NULL)
		return;

	pktp = (struct cmpkt *)ata_pktp;
	pktp->cp_reason = CPS_CHKERR;
	ata_pktp->ac_scb = DERR_ABORT;
	mutex_exit(mutex);
	(*pktp->cp_callback)(pktp);
	mutex_enter(mutex);
}

/*
 * atapi inquiry
 */
static int
ata_inquiry(struct ata_blk *ata_blkp, struct scsi_inquiry *inqp,
	int drive)
{
	union scsi_cdb	atapi_inq;
	ushort		*sbuf, word_count, words_left;
	int		retry = 0;

#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_inquiry(0x%x, 0x%x, 0x%x)\n", ata_blkp, inqp, drive);
	}
#endif

retry:
	if (drive == 0) {
		outb(ata_blkp->ab_drvhd, ATDH_DRIVE0);
	} else {
		outb(ata_blkp->ab_drvhd, ATDH_DRIVE1);
	}
	if (ata_wait1(ata_blkp->ab_status, ATS_DRQ | ATS_BSY,
	    0, ATS_DRQ | ATS_BSY, 100000)) {
		goto err;
	}

	/*
	 * Set up task file
	 *	- turn off interrupts
	 * 	- set max size of data expected
	 *	- output command
	 */
	outb(ata_blkp->ab_hcyl, 0);
	outb(ata_blkp->ab_lcyl, sizeof (*inqp));
	outb(ata_blkp->ab_feature, 0);
	outb(ata_blkp->ab_cmd, ATC_PI_PKT);

	/*
	 * Construct a simple scsi inquiry pkt
	 */
	bzero((caddr_t)&atapi_inq, sizeof (atapi_inq));
	atapi_inq.scc_cmd = SCMD_INQUIRY;
	FORMG0COUNT(&atapi_inq, sizeof (*inqp));

	if (ata_wait1(ata_blkp->ab_status, ATS_BSY | ATS_ERR | ATS_DRQ,
	    ATS_DRQ, ATS_BSY | ATS_ERR, 100000)) {
		goto err;
	}
	drv_usecwait(10000);

	/*
	 * Transfer inquiry command
	 */
	repoutsw(ata_blkp->ab_data, (ushort *)&atapi_inq,
	    sizeof (atapi_inq) >> 1);

	/*
	 * Wait for not busy, then read inq data
	 */

	if (ata_wait(ata_blkp->ab_count,  ATI_COD | ATI_IO,
		ATI_IO, ATI_COD)) {
		goto err;
	}

	if (ata_wait(ata_blkp->ab_status, ATS_DRQ | ATS_BSY,
	    ATS_DRQ, ATS_BSY)) {
		goto err;
	}

	/*
	 * Read the inquiry data.
	 * Some drives provide the data a few bytes at a time, flow
	 * controlled by BSY/DRQ and the bytecounts.  Refer to ATAPI
	 * spec rev 1.2 PIO data in flow chart (section 5.7 page 25)
	 * The total size of the inquiry data may be larger than our
	 * our buffer (since it allows for vendor specific extensions)
	 * so we may read some extra data and dump it.
	 */

	sbuf = (ushort *)inqp;
	words_left = sizeof (struct scsi_inquiry) >> 1;

	do {
		/*
		 * Some drives return an odd number of bytes, so adjust here
		 */
		word_count = (ushort)((ushort)(inb(ata_blkp->ab_hcyl) << 8)
				+ inb(ata_blkp->ab_lcyl) + 1) >> 1;

		repinsw(ata_blkp->ab_data, sbuf, min(word_count, words_left));
		sbuf += min(word_count, words_left);

		while (word_count > words_left) {
			inw(ata_blkp->ab_data);
			word_count--;
		}

		words_left -= word_count;

		if (ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY))
			goto err;

	} while (inb(ata_blkp->ab_status) & ATS_DRQ);

	if (ata_wait(ata_blkp->ab_status, ATS_DRQ | ATS_BSY,
	    0, ATS_DRQ | ATS_BSY)) {
		goto err;
	}

	return (1);
err:
#ifdef ATA_DEBUG
	if (ata_debug & DERR) {
		PRF("bad ata_inquiry hi %x, lo %x, dr %x",
			inb(ata_blkp->ab_hcyl),
			inb(ata_blkp->ab_lcyl),
			inb(ata_blkp->ab_drvhd));
		PRF(", er %x, st %x, ioad1 %x, drive %x\n",
			inb(ata_blkp->ab_error),
			inb(ata_blkp->ab_status),
			ata_blkp->ab_data,
			drive);
	}
#endif
	if (++retry < 3) {
		/*
		 * Unknown as to why a reset should sometimes be needed.
		 * The unit has just sucessfully performed a
		 * ATC_PI_ID_DEV command.
		 */
		outb(ata_blkp->ab_cmd, ATC_PI_SRESET);
		drv_usecwait(600000);
		goto retry;
	}
	outb(ata_blkp->ab_devctl, 8); /* turn interrupts back on */
	return (0);
}


static int
ata_lock_unlock(struct ata *atap, struct ata_cmpkt *ata_pktp, int lock)
{
	struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	struct ata_blk		*ata_blkp = atap->a_blkp;

	if (ATAPI(atap->a_unitp)) {
		ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
		pktp->cp_resid = 0;
		ata_pktp->ac_atapi = 1;
		ata_pktp->ac_cmd = ATC_PI_PKT;
		ata_pktp->ac_hicyl = 0;
		ata_pktp->ac_lwcyl = 0;
		/*
		 * Construct scsi prevent/allow medium removal packet
		 */
		bzero((caddr_t)&ata_pktp->ac_scsi_pkt, sizeof (union scsi_cdb));
		ata_pktp->ac_scsi_pkt.scc_cmd = SCMD_DOORLOCK;
		FORMG0COUNT(&ata_pktp->ac_scsi_pkt, (u_char)lock);

		FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
		return (biowait(pktp->cp_bp));
	} else {
		if (lock)
			ata_ack_media_change(ata_blkp, ata_pktp);
		outb(ata_blkp->ab_drvhd, ata_pktp->ac_hd);
		outb(ata_blkp->ab_cmd, lock ? ATC_DOOR_LOCK : ATC_DOOR_UNLOCK);
		if (ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY))
			return (DDI_FAILURE);
		return (DDI_SUCCESS);
	}
}

static void
ata_ack_media_change(struct ata_blk *ata_blkp, struct ata_cmpkt *ata_pktp)
{
	if (inb(ata_blkp->ab_error) & ATE_MC) {
		outb(ata_blkp->ab_drvhd, ata_pktp->ac_hd);
		outb(ata_blkp->ab_cmd, ATC_ACK_MC);
		ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY);
	}
}

static int
atapi_request_sense(struct ata *atap, struct ata_cmpkt *ata_pktp)
{
	struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	struct scsi_extended_sense ata_rqs; /* bit bucket */
	int retries = 3;

	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;

	ata_pktp->ac_lwcyl = sizeof (ata_rqs);
	bzero((caddr_t)&ata_pktp->ac_scsi_pkt,
				sizeof (union scsi_cdb));
	ata_pktp->ac_scsi_pkt.scc_cmd = SCMD_REQUEST_SENSE;
	FORMG0COUNT(&ata_pktp->ac_scsi_pkt, sizeof (ata_rqs));
	do {
		pktp->cp_resid = sizeof (ata_rqs);
		ata_pktp->ac_v_addr = (caddr_t)&ata_rqs;

		FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw,
				pktp->cp_bp);
		(void) biowait(pktp->cp_bp); /* ignore errors */
	} while (((((int)(ata_pktp->ac_error & ATS_SENSE_KEY))
		>> ATS_SENSE_KEY_SHIFT) == KEY_UNIT_ATTENTION) && --retries);

	return (((int)(ata_pktp->ac_error & ATS_SENSE_KEY))
			>> ATS_SENSE_KEY_SHIFT);
}

static int
ata_start_stop_motor(struct ata *atap, struct ata_cmpkt *ata_pktp, int start)
{
	struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	struct ata_blk		*ata_blkp = atap->a_blkp;


	if (ATAPI(atap->a_unitp)) {

		if (start == 1)
			(void) atapi_request_sense(atap, ata_pktp);

		/*
		 * Construct scsi start/stop/eject packet
		 */
		ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
		ata_pktp->ac_atapi = 1;
		ata_pktp->ac_cmd = ATC_PI_PKT;
		ata_pktp->ac_hicyl = 0;
		ata_pktp->ac_lwcyl = 0;
		pktp->cp_resid = 0;
		bzero((caddr_t)&ata_pktp->ac_scsi_pkt, sizeof (union scsi_cdb));
		ata_pktp->ac_scsi_pkt.scc_cmd = SCMD_START_STOP;
		FORMG0COUNT(&ata_pktp->ac_scsi_pkt, (u_char)start);

		FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
		return (biowait(pktp->cp_bp));
	} else {
		if (start == 2)
			return (DDI_SUCCESS);
		if (start == 0)
			return (DDI_SUCCESS);
		outb(ata_blkp->ab_drvhd, ata_pktp->ac_hd);
		outb(ata_blkp->ab_cmd, ATC_IDLE_IMMED);
		if (ata_wait(ata_blkp->ab_status, ATS_BSY, 0, ATS_BSY))
			return (DDI_FAILURE);
		return (DDI_SUCCESS);
	}
}

static int
ata_get_state(struct ata *atap, struct ata_cmpkt *ata_pktp,
	enum dkio_state *statep)
{
	register struct cmpkt *pktp = (struct cmpkt *)ata_pktp;

	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	pktp->cp_resid = 0;
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;
	ata_pktp->ac_lwcyl = 0;
	/*
	 * Construct scsi test unit ready packet.
	 */
	bzero((caddr_t)&ata_pktp->ac_scsi_pkt, sizeof (union scsi_cdb));
	ata_pktp->ac_scsi_pkt.scc_cmd = SCMD_TEST_UNIT_READY;

	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	(void) biowait(pktp->cp_bp);

	if (ata_pktp->ac_status & ATS_ERR) {
		*statep = DKIO_EJECTED;
	} else {
		*statep = DKIO_INSERTED;
	}
	return (0);
}

/*
 * atapi read
 */
static struct cmpkt *
ata_read(struct ata *atap, struct cmpkt *pktp)
{
	register struct ata_cmpkt *ata_pktp = (struct ata_cmpkt *)pktp;
	ushort byte_count, sector_count;

	ASSERT(ATAPI(atap->a_unitp));
#ifdef ATA_DEBUG
	if (ata_debug & DENT) {
		PRF("ata_read(0x%x, 0x%x)\n", atap, pktp);
	}
	if (ata_debug & DIO) {
		PRF("srtsec 0x%x, count 0x%x ",
			pktp->cp_srtsec, pktp->cp_bytexfer);
	}
#endif

	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;

	byte_count = min((pktp->cp_bytexfer), ATAPI_MAX_XFER);
	pktp->cp_resid = pktp->cp_bytexfer = byte_count;

	ASSERT((byte_count & (ATA_CD_SECSIZ - 1)) == 0);
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = (byte_count >> 8);
	ata_pktp->ac_lwcyl = byte_count; /* auto truncate to char */
	ata_pktp->ac_v_addr = ata_pktp->ac_start_v_addr;

	/*
	 * Construct scsi read packet
	 */
	sector_count = byte_count >> ATA_CD_SCTRSHFT;
	bzero((caddr_t)&ata_pktp->ac_scsi_pkt, sizeof (union scsi_cdb));
	ata_pktp->ac_scsi_pkt.scc_cmd = SCMD_READ_G1;
	FORMG1ADDR(&ata_pktp->ac_scsi_pkt, pktp->cp_srtsec);
	FORMG1COUNT(&ata_pktp->ac_scsi_pkt, sector_count);
	return ((struct cmpkt *)ata_pktp); /* success */
}

static int
ata_read_capacity(struct ata *atap, struct ata_cmpkt *ata_pktp)
{
	register struct cmpkt *pktp = (struct cmpkt *)ata_pktp;

	ASSERT(ATAPI(atap->a_unitp));

	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	pktp->cp_resid = sizeof (struct scsi_capacity);
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;
	ata_pktp->ac_lwcyl = sizeof (struct scsi_capacity);
	ata_pktp->ac_v_addr = (caddr_t)&atap->a_unitp->au_capacity;

	/*
	 * Construct scsi read capacity packet
	 */
	bzero((caddr_t)&ata_pktp->ac_scsi_pkt, sizeof (union scsi_cdb));
	ata_pktp->ac_scsi_pkt.scc_cmd = SCMD_READ_CAPACITY;

	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	return (biowait(pktp->cp_bp));
}

static int
ata_cd_pause_resume(struct ata *atap, struct ata_cmpkt *ata_pktp, int resume)
{
	register struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	u_char *cdb = (u_char *)&ata_pktp->ac_scsi_pkt;

	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	pktp->cp_resid = 0;
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;
	ata_pktp->ac_lwcyl = 0;
	/*
	 * Construct scsi pause/resume packet.
	 */
	bzero((caddr_t)cdb, sizeof (union scsi_cdb));
	cdb[0] = SCMD_PAUSE_RESUME;
	cdb[8] = (u_char)resume;
	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	return (biowait(pktp->cp_bp));
}

static int
ata_cd_play_msf(struct ata *atap, struct ata_cmpkt *ata_pktp,
	caddr_t data, int flag)
{
	register struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	u_char *cdb = (u_char *)&ata_pktp->ac_scsi_pkt;
	struct cdrom_msf msf;

	if (ddi_copyin((caddr_t)data, (caddr_t)&msf, sizeof (msf), flag)) {
		return (EFAULT);
	}

	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	pktp->cp_resid = 0;
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;
	ata_pktp->ac_lwcyl = 0;
	/*
	 * Construct scsi play audio by msf packet
	 */
	bzero((caddr_t)cdb, sizeof (union scsi_cdb));
	cdb[0] = SCMD_PLAYAUDIO_MSF;
	if (atap->a_unitp->au_17b) {
		cdb[3] = BYTE_TO_BCD(msf.cdmsf_min0);
		cdb[4] = BYTE_TO_BCD(msf.cdmsf_sec0);
		cdb[5] = BYTE_TO_BCD(msf.cdmsf_frame0);
		cdb[6] = BYTE_TO_BCD(msf.cdmsf_min1);
		cdb[7] = BYTE_TO_BCD(msf.cdmsf_sec1);
		cdb[8] = BYTE_TO_BCD(msf.cdmsf_frame1);
	} else {
		cdb[3] = msf.cdmsf_min0;
		cdb[4] = msf.cdmsf_sec0;
		cdb[5] = msf.cdmsf_frame0;
		cdb[6] = msf.cdmsf_min1;
		cdb[7] = msf.cdmsf_sec1;
		cdb[8] = msf.cdmsf_frame1;
	}
	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	return (biowait(pktp->cp_bp));
}

static int
ata_cd_read_subchannel(struct ata *atap, struct ata_cmpkt *ata_pktp,
	caddr_t data, int flag)
{
	register struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	u_char *cdb = (u_char *)&ata_pktp->ac_scsi_pkt;
	struct cdrom_subchnl subchnl;
	caddr_t buffer;
	int	ret;

	if (ddi_copyin((caddr_t)data, (caddr_t)&subchnl,
	    sizeof (subchnl), flag)) {
		return (EFAULT);
	}

	buffer = kmem_zalloc(16, KM_SLEEP);

	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	pktp->cp_resid = 16;
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;
	ata_pktp->ac_lwcyl = 16;
	ata_pktp->ac_v_addr = buffer;

	/*
	 * Construct scsi read subchannel packet.
	 */
	bzero((caddr_t)cdb, sizeof (union scsi_cdb));
	cdb[0] = SCMD_READ_SUBCHANNEL;
	cdb[1] = (subchnl.cdsc_format & CDROM_LBA) ? 0 : 0x02;
	/*
	 * set the Q bit in byte 2 to 1.
	 */
	cdb[2] = 0x40;
	/*
	 * This byte (byte 3) specifies the return data format. Proposed
	 * by Sony. To be added to SCSI-2 Rev 10b
	 * Setting it to one tells it to return time-data format
	 */
	cdb[3] = 0x01;
	cdb[8] = 0x10;

	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	if (ret = biowait(pktp->cp_bp)) {
		kmem_free(buffer, 16);
		return (ret);
	}

	subchnl.cdsc_audiostatus = buffer[1];
	subchnl.cdsc_trk = buffer[6];
	subchnl.cdsc_ind = buffer[7];
	subchnl.cdsc_adr = buffer[5] & 0xF0;
	subchnl.cdsc_ctrl = buffer[5] & 0x0F;
	if (subchnl.cdsc_format & CDROM_LBA) {
		subchnl.cdsc_absaddr.lba =
		    ((u_char)buffer[8] << 24) +
		    ((u_char)buffer[9] << 16) +
		    ((u_char)buffer[10] << 8) +
		    ((u_char)buffer[11]);
		subchnl.cdsc_reladdr.lba =
		    ((u_char)buffer[12] << 24) +
		    ((u_char)buffer[13] << 16) +
		    ((u_char)buffer[14] << 8) +
		    ((u_char)buffer[15]);
	} else {
		if (atap->a_unitp->au_17b) {
			subchnl.cdsc_absaddr.msf.minute =
				BCD_TO_BYTE(buffer[9]);
			subchnl.cdsc_absaddr.msf.second =
				BCD_TO_BYTE(buffer[10]);
			subchnl.cdsc_absaddr.msf.frame =
				BCD_TO_BYTE(buffer[11]);
			subchnl.cdsc_reladdr.msf.minute =
				BCD_TO_BYTE(buffer[13]);
			subchnl.cdsc_reladdr.msf.second =
				BCD_TO_BYTE(buffer[14]);
			subchnl.cdsc_reladdr.msf.frame =
				BCD_TO_BYTE(buffer[15]);
		} else {
			subchnl.cdsc_absaddr.msf.minute = buffer[9];
			subchnl.cdsc_absaddr.msf.second = buffer[10];
			subchnl.cdsc_absaddr.msf.frame = buffer[11];
			subchnl.cdsc_reladdr.msf.minute = buffer[13];
			subchnl.cdsc_reladdr.msf.second = buffer[14];
			subchnl.cdsc_reladdr.msf.frame = buffer[15];
		}
	}
	kmem_free(buffer, 16);
	if (ddi_copyout((caddr_t)&subchnl, (caddr_t)data,
	    sizeof (subchnl), flag))
		return (EFAULT);
	return (0);
}

static int
ata_cd_read_tochdr(struct ata *atap, struct ata_cmpkt *ata_pktp,
	caddr_t data, int flag)
{
	register struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	u_char *cdb = (u_char *)&ata_pktp->ac_scsi_pkt;
	struct cdrom_tochdr		hdr;
	caddr_t buffer;
	int	ret;

	buffer = kmem_zalloc(4, KM_SLEEP);

	pktp->cp_resid = 4;
	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;
	ata_pktp->ac_lwcyl = 4;
	ata_pktp->ac_v_addr = buffer;
	/*
	 * Construct scsi read toc packet.
	 */
	bzero((caddr_t)cdb, sizeof (union scsi_cdb));
	cdb[0] = SCMD_READ_TOC;
	/*
	 * byte 7, 8 are the allocation length. In this case, it is 4 bytes.
	 */
	cdb[8] = 0x04;

	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	if (ret = biowait(pktp->cp_bp)) {
		kmem_free(buffer, 4);
		return (ret);
	}

	hdr.cdth_trk0 = buffer[2];
	hdr.cdth_trk1 = buffer[3];
	kmem_free(buffer, 4);
	if (ddi_copyout((caddr_t)&hdr, (caddr_t)data, sizeof (hdr), flag))
		return (EFAULT);
	return (0);
}

/*
 * This routine read the toc of the disc and returns the information
 * of a particular track. The track number is specified by the ioctl
 * caller.
 */
static int
ata_cd_read_tocentry(struct ata *atap, struct ata_cmpkt *ata_pktp,
	caddr_t data, int flag)
{
	register struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	u_char *cdb = (u_char *)&ata_pktp->ac_scsi_pkt;
	struct cdrom_tocentry		entry;
	caddr_t buffer;
	int	ret;
	int	lba;

	if (ddi_copyin(data, (caddr_t)&entry, sizeof (entry), flag))
		return (EFAULT);

	if (!(entry.cdte_format & (CDROM_LBA | CDROM_MSF))) {
		return (EINVAL);
	}

	buffer = kmem_zalloc(12, KM_SLEEP);

	pktp->cp_resid = 12;
	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;
	ata_pktp->ac_lwcyl = 12;
	ata_pktp->ac_v_addr = buffer;

	bzero((caddr_t)cdb, sizeof (union scsi_cdb));
	cdb[0] = SCMD_READ_TOC;
	/* set the MSF bit of byte one */
	cdb[1] = (entry.cdte_format & CDROM_LBA) ? 0 : 2;
	cdb[6] = entry.cdte_track;
	/*
	 * byte 7, 8 are the allocation length. In this case, it is 4 + 8
	 * = 12 bytes, since we only need one entry.
	 */
	cdb[8] = 0x0C;

	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	if (ret = biowait(pktp->cp_bp)) {
		kmem_free(buffer, 12);
		return (ret);
	}

	entry.cdte_adr = (buffer[5] & 0xF0) >> 4;
	entry.cdte_ctrl = (buffer[5] & 0x0F);
	if (entry.cdte_format & CDROM_LBA) {
		PRF(" %x", buffer[8]);
		PRF(" %x", buffer[9]);
		PRF(" %x", buffer[10]);
		PRF(" %x\n", buffer[11]);
		entry.cdte_addr.lba =
		    ((u_char)buffer[8] << 24) +
		    ((u_char)buffer[9] << 16) +
		    ((u_char)buffer[10] << 8) +
		    ((u_char)buffer[11]);
	} else {
		if (atap->a_unitp->au_17b) {
			entry.cdte_addr.msf.minute = BCD_TO_BYTE(buffer[9]);
			entry.cdte_addr.msf.second = BCD_TO_BYTE(buffer[10]);
			entry.cdte_addr.msf.frame = BCD_TO_BYTE(buffer[11]);
		} else {
			entry.cdte_addr.msf.minute = buffer[9];
			entry.cdte_addr.msf.second = buffer[10];
			entry.cdte_addr.msf.frame = buffer[11];
		}
	}

	/*
	 * Now read the header to determine which data mode it is in.
	 * ...If the track is a data track
	 */
	if ((entry.cdte_ctrl & CDROM_DATA_TRACK) &&
	    (entry.cdte_track != CDROM_LEADOUT)) {
		/*
		 * The NEC CDR-260 doesn't support the 1.2 spec
		 * read header command, so we issue a read cd
		 * for just the header because both the 1.7B and
		 * 1.2 specs support this.
		 */
		if (entry.cdte_format & CDROM_LBA) {
			lba = entry.cdte_addr.lba;
		} else {
			lba =
			    (((entry.cdte_addr.msf.minute * 60) +
			    (entry.cdte_addr.msf.second)) * 75) +
			    entry.cdte_addr.msf.frame;
		}
		pktp->cp_resid = 4;
		ata_pktp->ac_lwcyl = 4;
		ata_pktp->ac_v_addr = buffer;
		bzero((caddr_t)cdb, sizeof (union scsi_cdb));
		if (atap->a_unitp->au_17b) {
			cdb[0] = 0xd4; /* READ CD */
		} else {
			cdb[0] = 0xbe; /* READ CD */
		}
		cdb[2] = (u_char)((lba >> 24) & 0xFF);
		cdb[3] = (u_char)((lba >> 16) & 0xFF);
		cdb[4] = (u_char)((lba >> 8) & 0xFF);
		cdb[5] = (u_char)(lba & 0xFF);
		cdb[8] = 0x08;
		cdb[9] = 0x20; /* read header only */

		FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
		if (ret = biowait(pktp->cp_bp)) {
			kmem_free(buffer, 12);
			return (ret);
		}
		entry.cdte_datamode = buffer[0];

	} else
		entry.cdte_datamode = (u_char) -1;

	kmem_free(buffer, 12);

	if (ddi_copyout((caddr_t)&entry, data, sizeof (entry), flag))
		return (EFAULT);

	return (0);
}

static int
ata_cd_volume_ctrl(struct ata *atap, struct ata_cmpkt *ata_pktp,
	caddr_t data, int flag)
{
	register struct cmpkt *pktp = (struct cmpkt *)ata_pktp;
	u_char *cdb = (u_char *)&ata_pktp->ac_scsi_pkt;
	struct cdrom_volctrl		vol;
	caddr_t buffer;
	int	rtn;


	if (ddi_copyin((caddr_t)data, (caddr_t)&vol, sizeof (vol), flag))
		return (EFAULT);

	buffer = kmem_zalloc(24, KM_SLEEP);

	pktp->cp_resid = 24;
	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	ata_pktp->ac_atapi = 1;
	ata_pktp->ac_cmd = ATC_PI_PKT;
	ata_pktp->ac_hicyl = 0;
	ata_pktp->ac_lwcyl = 24;
	ata_pktp->ac_v_addr = buffer;

	bzero((caddr_t)cdb, sizeof (union scsi_cdb));
	cdb[0] = 0x5A; /* Group 5 version of SCMD_MODE_SENSE */
	cdb[2] = 0xe;
	cdb[8] = 24;

	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	rtn = biowait(pktp->cp_bp);
	if (rtn) {
		kmem_free(buffer, 24);
		return (rtn);
	}

	pktp->cp_resid = 24;
	ata_pktp->ac_hd = atap->a_unitp->au_drive_bits;
	ata_pktp->ac_v_addr = buffer;

	cdb[0] = 0x55; /* Group 5 version of SCMD_MODE_SELECT */
	cdb[1] = 0x10;

	/*
	 * Clear 8 byte Mode Parameter Header set by mode sense.
	 */
	buffer [1] = 0;
	buffer [2] = 0;

	/*
	 * Fill in the input data. Set the output channel 0, 1 volumes to
	 * output port 0, 1 respectively.
	 */
	buffer[17] = vol.channel0;
	buffer[19] = vol.channel1;

	FLC_ENQUE((opaque_t)pktp->cp_bp->b_forw, pktp->cp_bp);
	rtn = biowait(pktp->cp_bp);
	kmem_free(buffer, 24);
	return (rtn);
}

#ifdef	ATA_DEBUG
static char *
ata_errvals[] = {"", "amnf", "tkonf", "abort", "mc", "idnf", "unc", "bbk"};

static void
ata_print_errflag(int evalue)
{
	int	i, count;

	printf("#%x ", evalue);
	for (i = 0; i < 8; i++, evalue >>= 1) {
		if ((evalue & 1) == 0)
			continue;
		if (count)
			printf("+");
		printf("%s", ata_errvals[i]);
		count++;
	}
	printf("\n");
}

static char *
ata_sttvals[] = { "bsy", "drdy", "dwf", "dsc", "drq", "corr", "idx", "err"};

static void
ata_print_sttflag(int svalue)
{
	int	i, count;

	printf("#%x ", svalue);
	for (i = 0; i < 8; i++, svalue >>= 1) {
		if ((svalue & 1) == 0)
			continue;
		if (count)
			printf("+");
		printf("%s", ata_sttvals[i]);
		count++;
	}
	printf("\n");
}
#endif
