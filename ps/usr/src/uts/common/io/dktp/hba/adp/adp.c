/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident	"@(#)adp.c	1.29	96/07/30 SMI"

#include "../ghd/ghd.h"

#include "him_equ.h"
#include "him_scb.h"
#include "adp.h"
#include "adpcmd.h"
#include "adp_debug.h"

/*
 *
 * External references
 */

static	int	adp_tran_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static	void	adp_tran_destroy_pkt(struct scsi_address *ap,
					struct scsi_pkt *pkt);
static	void	adp_tran_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);
static	int	adp_tran_getcap(struct scsi_address *ap, char *cap,
					int tgtonly);
static struct scsi_pkt *adp_tran_init_pkt(struct scsi_address *ap,
				struct scsi_pkt *pktp, struct buf *bp,
				int cmdlen, int statuslen, int tgtlen,
				int flags, int (*callback)(), caddr_t arg);
static	int	adp_tran_reset(struct scsi_address *ap, int level);
static	int	adp_tran_setcap(struct scsi_address *ap, char *cap, int value,
					int tgtonly);
static	int	adp_tran_start(struct scsi_address *ap, struct scsi_pkt *pktp);
static	void	adp_tran_sync_pkt(struct scsi_address *ap,
					struct scsi_pkt *pkt);

static	void	adp_tran_tgt_free(dev_info_t *, dev_info_t *, scsi_hba_tran_t *,
					struct scsi_device *);
static	int	adp_tran_tgt_init(dev_info_t *, dev_info_t *, scsi_hba_tran_t *,
					struct scsi_device *);
static	int	adp_tran_tgt_probe(struct scsi_device *, int (*)());

#ifdef ADP_DEBUG
void		adp_dump_adp(adp_t *adpp);
void		adp_dump_scb(sp_struct *scbp);
#endif

/*
 * Local Function Prototypes
 */

static	adp_t	*adp_alloc_adp(dev_info_t *dip);
static	int	 adp_capchk(char *cap, int tgtonly, int *cidxp);
static	int	 adp_cfginit(dev_info_t *devi, adp_t *adp);
#ifdef COMMON_IO_EMULATION
static	int	 adp_check_device_id(dev_info_t *devi, int device_id);
#endif
static	void	 adp_chkstatus(struct scsi_pkt *pktp, sp_struct *scbp);
static	void	 adp_fix_negotiation(struct scsi_device *sd);
static	int	 adp_get_status(void *hba_handle, void *arg);
static	void	 adp_getedt(adp_t *adpp);
static	int	 adp_init(dev_info_t *dip, adp_t *adpp);
static	u_int	 adp_intr(caddr_t arg);
static	void	 adp_process_intr(void *hba_handle, void *arg);
static	int	 adp_reg_map(dev_info_t *, ddi_acc_handle_t *, caddr_t *);
static	gcmd_t	*adp_scballoc(void *tgt_private, void *bufp, int cmdlen,
			      int statuslen, int tgtlen, int ccblen);
static	void	 adp_scbfree(void *p);
static	int	 adp_search_pci(dev_info_t *);
static	int	 adp_setup_tran(dev_info_t *dip, adp_t *adpp);
static	void	 adp_sg_func(gcmd_t *gcmdp, ddi_dma_cookie_t *dmackp,
			     int single_segment, int seg_index);
static	int	 adp_start(void *hba_handle, gcmd_t *gcmdp);
static	int	 adp_timeout_action(void *hba_handle, gcmd_t *gcmdp,
				    void *tgt_handle, gact_t action );
static	int	 adp_xlate_vec(adp_t *adpp);
void		 PH_ScbCompleted(sp_struct *scbp);

/*
 * Local static data
 */

static	int	adp_settle_time = 2;	/* 2 sec delay for adp_reset_bus()*/
static	int	adp_inquiry_timeout = 60; /* timeout for adp_do_inquiry */


static	int	adp_pgsz = 0;
static	int	adp_pgmsk;
static	int	adp_pgshf;
static	int	adp_up = 0;

/*
 * DMA limits for data transfer
 */
static ddi_dma_lim_t adp_dmalim = {
	0,		/* address low				*/
	0xffffffffU,	/* address high				*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version			*/
	0xffffffffU,	/* address register			*/
	0x003fffff,	/* counter register			*/
	512,		/* sector size				*/
	ADP_MAX_DMA_SEGS, /* scatter/gather list length		*/
	0xffffffffU	/* request size				*/
};


int	adp_debug_flags = 0;

static int adp_errs[] = {
	CMD_CMPLT,		/* 0x00	No adapter status available	*/
	CMD_TRAN_ERR,		/* 0x01 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x02 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x03 Illegal code			*/
	CMD_ABORTED,		/* 0x04	Command aborted by host		*/
	CMD_ABORTED,		/* 0x05	Command aborted by host adapter */
	CMD_TRAN_ERR,   	/* 0x06 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x07 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x08 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x09 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x0a Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x0b Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x0c Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x0d Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x0e Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x0f Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x10 Illegal code			*/
	CMD_INCOMPLETE,		/* 0x11	Selection timeout		*/
/*	could be CMD_TIMEOUT, but unexpected by scsi_hba_probe		*/
	CMD_DATA_OVR,		/* 0x12	Data overrun/underrun error	*/
	CMD_UNX_BUS_FREE,	/* 0x13	Unexpected bus free		*/
	CMD_TRAN_ERR,		/* 0x14	Target bus phase sequence error */
	CMD_TRAN_ERR,   	/* 0x15 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x16 Illegal code			*/
	CMD_TRAN_ERR,		/* 0x17	Invalid SCSI linking operation	*/
	CMD_TRAN_ERR,   	/* 0x18 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x19 Illegal code			*/
	CMD_TRAN_ERR,   	/* 0x1a Illegal code			*/
	CMD_TRAN_ERR,		/* 0x1b	Auto-request sense failed	*/
	CMD_TAG_REJECT,		/* 0x1c	Taged Queing rejected by target */
	CMD_TRAN_ERR,		/* 0x1d Illegal code			*/
	CMD_TRAN_ERR,		/* 0x1e Illegal code			*/
	CMD_TRAN_ERR,		/* 0x1f Illegal code			*/
	CMD_TRAN_ERR,		/* 0x20	Host adpater hardware error	*/
	CMD_BDR_FAIL,		/* 0x21	Target did'nt respond to ATN (RESET) */
	CMD_ABORTED,		/* 0x22	SCSI bus reset by host adapter	*/
	CMD_RESET,		/* 0x23	SCSI bus reset by other device	*/
	CMD_TRAN_ERR,		/* 0x24 Illegal code			*/
	CMD_TRAN_ERR,		/* 0x25 Illegal code			*/
	CMD_TRAN_ERR,		/* 0x26 Illegal code			*/
	CMD_TRAN_ERR,		/* 0x27 Illegal code			*/
	CMD_TRAN_ERR,		/* 0x28 Illegal code			*/
	CMD_TRAN_ERR,		/* 0x29 Illegal code			*/
	CMD_TRAN_ERR,		/* 0x2a Illegal code			*/
	CMD_TRAN_ERR,		/* 0x2b Illegal code			*/
	CMD_TRAN_ERR,		/* 0x2c Illegal code			*/
	CMD_TRAN_ERR,		/* 0x2d Illegal code			*/
	CMD_TRAN_ERR,		/* 0x2e Illegal code			*/
	CMD_TRAN_ERR,		/* 0x2f Illegal code			*/
	CMD_TRAN_ERR,		/* 0x30 No available index?		*/
};

static int adp_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd,
		void *arg, void **result);
static int adp_identify(dev_info_t *dev);
static int adp_probe(dev_info_t *);
static int adp_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int adp_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

static struct dev_ops	adp_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	adp_getinfo,		/* info */
	adp_identify,		/* identify */
	adp_probe,		/* probe */
	adp_attach,		/* attach */
	adp_detach,		/* detach */
	nulldev,		/* no reset */
	(struct cb_ops *)0,	/* driver operations */
	NULL			/* bus operations */
};

/*
 * Create a single CCB timeout list for all instances
 */
static  tmr_t   adp_timer_conf;
static  long    adp_watchdog_tick = 2 * HZ;  /* check timeouts every 2 secs */



#include <sys/modctl.h>

#ifdef COMMON_IO_EMULATION
char _depends_on[] = "misc/xpci misc/scsi";
#else
char _depends_on[] = "misc/scsi";
#endif

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"Adaptec 2940/3940 Type SCSI Host Adapter Driver",
	&adp_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

int
_init( void )
{
	int	status;

	if ((status = scsi_hba_init(&modlinkage)) != 0) {
		return (status);
	}

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		return (status);
	}

        /*
         * Initialize the per driver timer info
         */
        ghd_timer_init(&adp_timer_conf, "ADP CCB timer", adp_watchdog_tick);

	return (status);
}

int
_fini( void )
{
	int	status;

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		ghd_timer_fini(&adp_timer_conf);
	}
	return (status);
}

int
_info( struct modinfo *modinfop )
{
	return (mod_info(&modlinkage, modinfop));
}


/*ARGSUSED*/

/*
 *	the adp_t structure includes the cfp structure
 *
 *	 tran_tgt_private points to the adptgt_t structure
 *	 tran_hba_private points to the adp_t
 */

static int
adp_tran_tgt_init(	dev_info_t	*hba_dip,
			dev_info_t	*tgt_dip,
			scsi_hba_tran_t	*hba_tran,
			struct scsi_device *sd )
{
	adptgt_t *tgtp;
	adp_t	 *adpp;
	u_short	  target;
	u_char	  lun;

	target = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

	ADBG_INIT(("?%s%d: %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		target, lun));

	adpp = SDEV2ADPP(sd);

	/* validity check the address and disallow the HBA's target ID */
	if (target > ADPP2CFPP(adpp)->Cf_MaxTargets || lun > 7
	||  target == ADPP2CFPP(adpp)->Cf_ScsiId) {
		ADBG_INIT(("?%s%d: %s%d invalid address <%d,%d>\n",
			ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
			ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
			target, lun));
		return (DDI_FAILURE);
	}


	tgtp = kmem_zalloc(sizeof (*tgtp), KM_SLEEP);
	ASSERT(tgtp != NULL);
	tgtp->au_adpp = adpp;
	tgtp->au_target = target;
	tgtp->au_lun = lun;
	tgtp->au_lim = adp_dmalim;
	tgtp->au_tran = hba_tran;

	hba_tran->tran_tgt_private = tgtp;

	ADBG_INIT(("?adp_tran_tgt_init: <%d,%d> adp 0x%x tgtp 0x%x\n",
		target, lun, adpp, tgtp));
	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static void
adp_tran_tgt_free(	dev_info_t	*hba_dip,
			dev_info_t	*tgt_dip,
			scsi_hba_tran_t	*hba_tran,
			struct scsi_device *sd )
{
	adptgt_t *tgtp = TRAN2TGTP(hba_tran);

	ADBG_INIT(("?adp_tran_tgt_free: <%d,%d> adp_unit at 0x%x\n",
			sd->sd_address.a_target, sd->sd_address.a_lun, tgtp));
	kmem_free((caddr_t)tgtp, sizeof (*tgtp));
}

/*ARGSUSED*/
static int
adp_tran_tgt_probe( struct scsi_device *sd, int (*callback)() )
{
	int	 rval;
	char	*s;

	/*
	 * Refer to adp_getedt for explanation of adp_fix_negotiation
	 */
	adp_fix_negotiation(sd);

	rval = scsi_hba_probe(sd, callback);

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

	ADBG_PROBE(("?adp%d: %s target %d lun %d %s\n",
			ddi_get_instance(SDEV2ADPP(sd)->ab_dip),
			ddi_get_name(sd->sd_dev),
			sd->sd_address.a_target, sd->sd_address.a_lun, s));
	return (rval);
}


/*
 *
 * adp_fix_negotiation()
 *
 *	For comments, refer to adp_getedt() and adp_do_inquiry().
 *
 */

static void
adp_fix_negotiation( struct scsi_device *sd )
{
	adp_t		*adpp = SDEV2ADPP(sd);
	cfp_struct	*cfpp = ADPP2CFPP(adpp);
	sp_struct	*scbp;

	if (ddi_iopb_alloc(adpp->ab_dip, &adp_dmalim, sizeof (*scbp),
			  (caddr_t *)&scbp) == DDI_FAILURE) {
		ADBG_ERROR(("adp_fix_negotiation: alloc fail\n"));
		return;
	}
	bzero((caddr_t)scbp, sizeof (*scbp));
	scbp->SP_ConfigPtr = cfpp;
	scbp->SP_Tarlun = ((sd->sd_address.a_target) << 4);

	(void)PH_Special(FORCE_RENEGOTIATE, cfpp, scbp);

	ddi_iopb_free((caddr_t)scbp);

	ADBG_PROBE(("?adp_fix: %s target %d lun %d\n", ddi_get_name(sd->sd_dev),
		sd->sd_address.a_target, sd->sd_address.a_lun));
	return;
}


/*
 *
 * This routine is needed to track information on the ability of each
 * target to correctly handle wide and/or synchronous SCSI negotiation.
 * The Adaptec HIM code does not support all mechanisms indicating inability
 * to support wide or synchronous SCSI transfer with targets.
 * Hence we must prevent
 * some forms of negotiation with targets that will fail to negotiate.
 * Here we query each target using a SCSI inquiry, and store the capabilities
 * in the Cf_ScsiOption array. Then when adp_tran_tgt_probe is called, we
 * use a HIM function (FORCE_RENEGOTIATE) to turn back wide and/or synchronous
 * negotiation for those targets that can support it.
 */

static void
adp_do_inquiry(	cfp_struct	*cfpp,
		struct scsi_address *ap,
		int		 target )
{
	struct scsi_inquiry *inqp;
	struct buf	*bp;
	struct scsi_pkt	*pktp;

	bp = scsi_alloc_consistent_buf(ap, NULL, sizeof (*inqp), B_READ,
					SLEEP_FUNC, NULL);
	if (bp == NULL) {
		goto err_exit1;
	}
	inqp = (struct scsi_inquiry *)bp->b_un.b_addr;

	pktp = scsi_init_pkt(ap, NULL, bp, CDB_GROUP0, 1, 0, PKT_CONSISTENT,
				SLEEP_FUNC, NULL);

	if (pktp == NULL) {
		goto err_exit2;
	}

	bzero((caddr_t)pktp->pkt_cdbp, CDB_GROUP0);
	pktp->pkt_cdbp[0] = SCMD_INQUIRY;
	pktp->pkt_cdbp[4] = sizeof (*inqp);

	pktp->pkt_time = adp_inquiry_timeout;
	pktp->pkt_flags |= FLAG_NOINTR;

	bzero((caddr_t)inqp, sizeof (*inqp));

	/*
	 * issue the Inquiry request and check for errors
	 */
	if (scsi_transport(pktp) != TRAN_ACCEPT
	||  pktp->pkt_reason != CMD_CMPLT
	||  *(pktp->pkt_scbp) != STATUS_GOOD) {
		goto err_exit3;
	}

	/* override the BIOS Sync and Wide settings if necessary */
	if ((cfpp->Cf_ScsiOption[target] & SYNC_MODE)
	&&  !inqp->inq_sync) {
		cmn_err(CE_CONT, "?adp(%d,%d): ID %d "
			"Synchronous negotiation disabled\n",
			cfpp->Cf_BusNumber, cfpp->Cf_DeviceNumber,
			target);
		cfpp->Cf_ScsiOption[target] &= ~SYNC_MODE;
	}

	if ((cfpp->Cf_ScsiOption[target] & WIDE_MODE)
	&&   !inqp->inq_wbus16) {
		cmn_err(CE_CONT, "?adp(%d,%d): ID %d "
			"Wide negotiation disabled\n",
			cfpp->Cf_BusNumber, cfpp->Cf_DeviceNumber,
			target);
		cfpp->Cf_ScsiOption[target] &= ~WIDE_MODE;
	}

	scsi_destroy_pkt(pktp);
	scsi_free_consistent_buf(bp);
	return;

err_exit3:
	scsi_destroy_pkt(pktp);
err_exit2:
	scsi_free_consistent_buf(bp);
err_exit1:
	/* use a safe default if any kind of error */
	cfpp->Cf_ScsiOption[target] &= ~SYNC_MODE;
	cfpp->Cf_ScsiOption[target] &= ~WIDE_MODE;
	return;
}


/*
 *
 * adp_getedt()
 *
 *	Send a SCSI Inquiry request to every target device and
 *	check the response to see if it knows how to handle Wide
 *	and/or Synchronous negotiation.
 *
 */

static void
adp_getedt( adp_t *adpp )
{
	cfp_struct	*cfpp = ADPP2CFPP(adpp);
	struct scsi_address addr = {0};
	scsi_hba_tran_t	 hba_tran = {0};
	adptgt_t	 adptgt = {0};
	int		 rc = TRUE;
	u_short		 target;

	/* patch together a phony target instance */
	hba_tran = *((scsi_hba_tran_t *)ddi_get_driver_private(adpp->ab_dip));
	hba_tran.tran_tgt_private = &adptgt;

	adptgt.au_lim = adp_dmalim;
	adptgt.au_adpp = adpp;
	adptgt.au_tran = &hba_tran;
	adptgt.au_arq = TRUE;

	addr.a_hba_tran = &hba_tran;
	addr.a_lun = 0;
	addr.a_sublun = 0;

	for (target = 0; target < cfpp->Cf_MaxTargets; target++) {

		/* skip over the HBA's target id */
		if (target == cfpp->Cf_ScsiId)
			continue;

		/* if Sync and Wide are already disabled skip the Inquiry */
		if (!(cfpp->Cf_ScsiOption[target] & SYNC_MODE)
		&&  !(cfpp->Cf_ScsiOption[target] & WIDE_MODE))
			continue;

		/* send the INQUIRY cmd to the target and check results */
		addr.a_target = target;
		adptgt.au_target = target;
		adp_do_inquiry(cfpp, &addr, target);
	}

	/*
	 * allow the HIM code to do sync/wide negotiations now
	 * that we've identified any non-SCSI-2 compliant drives
	 */
	cfpp->CFP_SuppressNego = 0;
	return;
}


/*
 *		Autoconfiguration routines
 */

/*ARGSUSED*/
static int
adp_getinfo(	dev_info_t *dip,
		ddi_info_cmd_t infocmd,
		void	 *arg,
		void	**result )
{
	return (DDI_FAILURE);
}

static int
adp_identify( dev_info_t *dip )
{

	ADBG_INIT(("?adp_identify: %s\n", ddi_get_name(dip)));
	return (DDI_IDENTIFIED);
}

static int
adp_probe( dev_info_t *devi )
{
	if (!adp_search_pci(devi))
		return (DDI_PROBE_FAILURE);
	return (DDI_SUCCESS);
}

static int
adp_search_pci( dev_info_t *devi )
{
#ifdef COMMON_IO_EMULATION
	ddi_acc_handle_t	cfg_handle;
	ushort_t		vendorid, deviceid;

	if (pci_config_setup(devi, &cfg_handle) != DDI_SUCCESS) {
		ADBG_INIT(("?adp_search_pci: setup fail\n"));
		return (FALSE);
	}

	vendorid = pci_config_getw(cfg_handle, PCI_CONF_VENID);

	if (vendorid != ADAPTEC_PCI_ID) {
		ADBG_INIT(("?adp_search_pci: bad vendor 0x%x\n", vendorid));
		pci_config_teardown(&cfg_handle);
		return (FALSE);
	}

	deviceid = pci_config_getw(cfg_handle, PCI_CONF_DEVID);

	if (!adp_check_device_id(devi, (int)deviceid)) {
		ADBG_INIT(("?adp_search_pci: bad device 0x%x\n", deviceid));
		pci_config_teardown(&cfg_handle);
		return (FALSE);
	}
	pci_config_teardown(&cfg_handle);
#endif

	return (TRUE);
}

#ifdef COMMON_IO_EMULATION

/*
 *
 * adp_check_device_id()
 *
 *	In the Solaris 2.4 release there's no support for self
 *	identifying devices. Therefore, the 2940 compatible 
 *	devices must be explicitly specified via a devinfo node
 *	property. The property is iniaitialized via a global
 *	property in the adp.conf file.
 *
 *	In later releases the supported device IDs are specified
 *	in the driver_aliases file.
 */

static int
adp_check_device_id( dev_info_t *devi, int device_id )
{
	int	len;
	int	count;
	int	num_devices;
	int	*p;
	int	*dev_ids;

	if (ddi_getlongprop(DDI_DEV_T_ANY, devi, DDI_PROP_DONTPASS,
		"device_ids", (caddr_t)&dev_ids, &len) != DDI_PROP_SUCCESS) {
			ADBG_ERROR(("adp_check_device_id: "
					"no device_ids property\n"));
		return (FALSE);
	}
	if (len < sizeof (int)) {
		/* no valid device IDs were specified */
		return (FALSE);
	}
	num_devices = len / sizeof (int);

	/* check for a match */
	for (count = num_devices, p = dev_ids; --count >= 0; p++) {
		if (device_id == *p) {
			(void) kmem_free(dev_ids, len);
			return (TRUE);
		}
	}
	(void) kmem_free(dev_ids, len);
	return (FALSE);
}
#endif


/*
 *
 * adp_reg_map()
 *
 *	The 2940/78xx chips are I/O mapped via BAR 0
 *
 */

static int
adp_reg_map(	dev_info_t	*dip,
		ddi_acc_handle_t *handlep,
		caddr_t		*ioaddrp )
{
        static ddi_device_acc_attr_t attr = {
                DDI_DEVICE_ATTR_V0,
                DDI_NEVERSWAP_ACC,
                DDI_STRICTORDER_ACC,
        };

	/*
	 * enable I/O access to the device
	 * the argument "1" below denotes access to I/O space
	 */
	if (ddi_regs_map_setup(dip, 1, ioaddrp, (offset_t)0, (offset_t)0,
				&attr, handlep) != DDI_SUCCESS) {
		ADBG_INIT(("?adp_reg_map: ddi_regs_map_setup: failed\n"));
		return (FALSE);
	}

	ADBG_INIT(("?adp_reg_map: %s:%d 0x%x\n", ddi_get_name(dip),
			ddi_get_instance(dip), *ioaddrp));
	return (TRUE);
}


/*
 *
 * adp_pci_enable()
 *
 *	The 2.5 [PPC] pci nexus doesn't yet set
 *	the PCI command register. Eliminate this
 *	routine when the nexus does the right thing.
 *
 */

static int
adp_pci_enable( dev_info_t *devi )
{
/* ??? why !i386, should this be #if defined(ppc) ??? */
/* ??? or maybe it's safe to just do it for all platforms ??? */
#if !defined(i386)
	ddi_acc_handle_t	cfg_handle;

	if (pci_config_setup(devi, &cfg_handle) != DDI_SUCCESS) {
		ADBG_INIT(("?adp_pci_enable: setup fail\n"));
		return (FALSE);
	}

	/*
	 * Enable access to device, and enable device access to memory.
	 * For x86, we need to respect the user's setup which could
	 * possibly disable the device.
	 */
	pci_config_putw(cfg_handle, PCI_CONF_COMM,
			pci_config_getw(cfg_handle, PCI_CONF_COMM) |
					PCI_COMM_IO | PCI_COMM_ME);
	pci_config_teardown(&cfg_handle);
#endif
	return (TRUE);
}


static int
adp_detach( dev_info_t *dip, ddi_detach_cmd_t cmd )
{
	scsi_hba_tran_t	*tran;
	adp_t		*adpp;
	cfp_struct	*cfpp;

	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}

/*		tran saved by scsi_hba_attach (3rd param) called in attach */
	tran = (scsi_hba_tran_t *)ddi_get_driver_private(dip);
	if (tran == NULL)
		return (DDI_SUCCESS);

	adpp = TRAN2ADPP(tran);
	if (adpp == NULL)
		return (DDI_SUCCESS);
	cfpp = ADPP2CFPP(adpp);

	/*
	 * disable hardware interrupts from the HBA
	 */
	PH_DisableInt(cfpp);

	/*
	 * discard the hsp_struct
	 */
	ddi_iopb_free((caddr_t)(cfpp->CFP_HaDataPtr));

	/*
	 * unmap the device from I/O space
	 */
	ddi_regs_map_free(&cfpp->ab_handle);

	/*
	 * Unconfigure this HBA instance
	 */
	ghd_unregister(&adpp->ab_ccc);

	/*
	 * discard the adp_t and cfp_struct structures
	 */
	kmem_free((caddr_t)adpp, sizeof (*adpp));

	scsi_hba_tran_free(tran);

	ddi_prop_remove_all(dip);

	if (scsi_hba_detach(dip) != DDI_SUCCESS) {
		ADBG_INIT(("?adp: scsi_hba_detach failed\n"));
	}

	ADBG_INIT(("?adp_detach: success\n"));
	return (DDI_SUCCESS);
}


/*
 *
 * adp_alloc_adp()
 *
 *	Allocate an adp_t structure (with an embedded cfp_struct).
 *	Link the cfp_struct to the adp_t struct.
 *
 */

static adp_t *
adp_alloc_adp( dev_info_t *dip )
{
	adp_t	*adpp;

	adpp = (adp_t *)kmem_zalloc(sizeof (*adpp), KM_SLEEP);
	if (adpp == NULL)
		return (NULL);

	adpp->ab_dip = dip;
	ADPP2CFPP(adpp)->Cf_OSspecific = (DWORD)adpp;
	return (adpp);
}

/*ARGSUSED*/
static int
adp_attach( dev_info_t *dip, ddi_attach_cmd_t cmd )
{
	adp_t	*adpp;

	if (cmd != DDI_ATTACH)
		goto err_exit1;

	/* verify it's a recognized device */
	if (!adp_search_pci(dip))
		goto err_exit1;

	/* allocate the HBA soft state structure */
	if (!(adpp = adp_alloc_adp(dip)))
		goto err_exit1;

	/* map the device and initialize some driver stuff */
	if (!adp_init(dip, adpp))
		goto err_exit2;

	/* set the PCI Master Enable and I/O Mapped modes (PPC only?) */
	if (!adp_pci_enable(dip))
		goto err_exit3;

	/* initialize the HIM code, start the sequencer and disable ints  */
	if (!adp_cfginit(dip, adpp))
		goto err_exit3;

	/* setup up the hba_tran structure and attach to SCSA */
	if (!adp_setup_tran(dip, adpp))
		goto err_exit4;

        /*
         * configure GHD and enable timeout processing on this HBA
         */
        if (!ghd_register("adp", &adpp->ab_ccc, dip, 0, adpp,
                          adp_scballoc, adp_scbfree, adp_sg_func, adp_start,
                          adp_intr, adp_get_status, adp_process_intr,
                          adp_timeout_action, &adp_timer_conf)) {

                ADBG_INIT(("?adp_attach: ghd_register failed\n"));
                goto err_exit5;
        }

	/* ever' ting otay now */
	ddi_report_dev(dip);

	/* enable hardware interrupts */
	PH_EnableInt(ADPP2CFPP(adpp));

	/*
	 * send SCSI INQUIRY cmd to each target to correct
	 * for HIM code sync/wide negotiation problem
	 */
	adp_getedt(adpp);

	ADBG_INIT(("?adp_attach: adpp 0x%x\n", adpp));
	return (DDI_SUCCESS);

err_exit5:
	(void)scsi_hba_detach(dip);
	scsi_hba_tran_free((scsi_hba_tran_t *)ddi_get_driver_private(dip));

err_exit4:
	ddi_iopb_free((caddr_t)ADPP2CFPP(adpp)->CFP_HaDataPtr);

err_exit3:
	ddi_regs_map_free(&ADPP2CFPP(adpp)->ab_handle);

err_exit2:
	kmem_free((caddr_t)adpp, sizeof (*adpp));

err_exit1:
	return (DDI_FAILURE);
}

static int
adp_setup_tran( dev_info_t *dip, adp_t *adpp )
{
	scsi_hba_tran_t	*hba_tran;

	if ((hba_tran = scsi_hba_tran_alloc(dip, 0)) == NULL) {
		return (FALSE);
	}

	/* hba_private always points to the adp_t (him_config) struct */
	hba_tran->tran_hba_private	= adpp;
	hba_tran->tran_tgt_private	= NULL;
	hba_tran->tran_tgt_init		= adp_tran_tgt_init;
	hba_tran->tran_tgt_probe	= adp_tran_tgt_probe;
	hba_tran->tran_tgt_free		= adp_tran_tgt_free;
	hba_tran->tran_start 		= adp_tran_start;
	hba_tran->tran_abort		= adp_tran_abort;
	hba_tran->tran_reset		= adp_tran_reset;
	hba_tran->tran_getcap		= adp_tran_getcap;
	hba_tran->tran_setcap		= adp_tran_setcap;
	hba_tran->tran_init_pkt 	= adp_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= adp_tran_destroy_pkt;
	hba_tran->tran_dmafree		= adp_tran_dmafree;
	hba_tran->tran_sync_pkt		= adp_tran_sync_pkt;

	if (scsi_hba_attach(dip, &adp_dmalim, hba_tran, SCSI_HBA_TRAN_CLONE,
				NULL) == DDI_SUCCESS)
		return (TRUE);

	ADBG_INIT(("?adp_setup_tran: scsi_hba_attach fail\n"));
	scsi_hba_tran_free((scsi_hba_tran_t *)ddi_get_driver_private(dip));
	return (FALSE);
}


/*
 *
 * adp_init()
 *
 *	Do various adp initializations.
 *
 */

static int
adp_init( dev_info_t *dip, adp_t *adpp )
{
	cfp_struct *cfpp = ADPP2CFPP(adpp);
	caddr_t	ioaddr;

	/*
	 * get the I/O space handle and the io address
	 */
	if (!adp_reg_map(dip, &cfpp->ab_handle, &ioaddr))
		return (FALSE);

	/*
	 * tell the HIM code the current base address
	 */
/* ??? I don't think this is necessary any more, I/O is done via a handle ???*/
	cfpp->CFP_BaseAddress = (DWORD)ioaddr;

	/*
	 * initialize page size variables for ADP_KVTOP() macro
	 */
	if (!(adp_up)) {
		int	i;
		int	len;

		adp_pgsz = ddi_ptob(adpp->ab_dip, 1L);
		adp_pgmsk = adp_pgsz - 1;
		for (i = adp_pgsz, len = 0; i > 1; len++)
			i >>= 1;
		adp_pgshf = len;
		adp_up = 1;
	}
	return (TRUE);
}


/*
 *
 * adp_cfginit()
 *
 *	Determine the HBA's bus and device numbers allocate the
 *	required hsp_struct and then call the HIM code to initialize
 *	the cfp_struct.
 *
 */

static int
adp_cfginit( dev_info_t *dip, adp_t *adpp )
{
	cfp_struct	*cfpp = ADPP2CFPP(adpp);
	hsp_struct	*him_datap;
	bios_info	*biosp;
	uint		 him_data_size;
	caddr_t		 buf;
	int		 status;
	ulong		*regp;
	int		 reglen;
	int		 bus;
	int		 device;

	/*
	 * get the HBA's PCI address (bus, device, function) from the
	 * first tuple of the reg property. The HIM code assumes that
	 * the function is always zero so ignore it here.
	 */
	if (ddi_getlongprop(DDI_DEV_T_NONE, dip, DDI_PROP_DONTPASS,
	    "reg", (caddr_t)&regp, &reglen) != DDI_PROP_SUCCESS) {
		ADBG_INIT(("?adp_cfginit: reg property not found\n"));
		return (FALSE);
	}

	/* extract the PCI bus and device numbers from the reg property */
#ifdef PCI_DDI_EMULATION
	*regp <<= 8;
#endif
	device = PCI_REG_DEV_G(*regp);
	bus = PCI_REG_BUS_G(*regp);
	kmem_free(regp, reglen);

	cfpp->Cf_BusNumber = (unsigned char) bus;
	cfpp->Cf_DeviceNumber = (unsigned char) device;
	ADBG_INIT(("?adp_cfginit: bus 0x%x dev 0%x\n", bus, device));


	cfpp->Cf_NumberScbs = DEFAULT_NUM_SCBS;

/*
 *	Suppress negotiation of synchronous or Wide (16 bit) xfers
 */
	cfpp->CFP_SuppressNego = 1;

/*	Note that Cf_AccessMode is set to 0 (default) by bzero 		*/
	PH_GetConfig(cfpp);

/*	Impose narrow async SCSI until getedt done			*/
	cfpp->CFP_SuppressNego = 1;
	cfpp->CFP_ResetBus = 1;

	him_data_size = cfpp->Cf_HimDataSize;

	ADBG_INIT(("?adp_cfginit(%d,%d): IO Addr 0x%x Chan %d IRQ 0x%x "
		   "DataSize 0x%x\n", cfpp->Cf_BusNumber, cfpp->Cf_DeviceNumber,
		   cfpp->CFP_BaseAddress, cfpp->Cf_ScsiChannel,
		   cfpp->Cf_IrqChannel, cfpp->Cf_HimDataSize));
	ADBG_INIT(("?\t AccessMode 0x%x Host id 0x%x NonTagScbs %d\n",
		   cfpp->Cf_AccessMode, cfpp->Cf_ScsiId,
		   cfpp->Cf_MaxNonTagScbs));

/*	allocate hsp						*/
	if (ddi_iopb_alloc(adpp->ab_dip, &adp_dmalim, him_data_size, &buf)
			== DDI_FAILURE) {
		ADBG_ERROR(("adp_cfginit: hsp alloc fail\n"));
		return (FALSE);
	}
	bzero(buf, him_data_size);
	him_datap = (hsp_struct *)buf;
	cfpp->CFP_HaDataPtr = him_datap;


/*	allocate bios_info area						*/
/* ??? why isn't this just kmem_zalloc() ??? */
	if (ddi_iopb_alloc(adpp->ab_dip, &adp_dmalim, sizeof (bios_info),
				&buf) == DDI_FAILURE) {
		ADBG_ERROR(("adp_cfginit: biosinfo alloc fail\n"));
		ddi_iopb_free((caddr_t)him_datap);
		return (FALSE);
	}
	bzero((caddr_t)buf, sizeof (bios_info));
	biosp = (bios_info *)buf;

#if defined(i386)
	if (PH_GetBiosInfo(ADPP2CFPP(adpp), cfpp->Cf_BusNumber,
				cfpp->Cf_DeviceNumber, biosp) == 0) {
		if (biosp->bi_global & BI_GIGABYTE) {
			adpp->ab_flag |= ADP_FLAG_GT1GIG;
			ADBG_INIT(("?adp_cfginit(%d,%d): bios GT1GIT\n",
				   cfpp->Cf_BusNumber, cfpp->Cf_DeviceNumber));
		}
	} else {
		adpp->ab_flag |= ADP_FLAG_NOBIOS;
		ADBG_INIT(("?adp_cfginit(%d,%d): get biosinfo fail\n",
			   cfpp->Cf_BusNumber, cfpp->Cf_DeviceNumber));
	}
#endif

	ddi_iopb_free((caddr_t)biosp);

/*	Release the first scb used by the Bios				*/
	cfpp->CFP_BiosActive = 0;
	cfpp->Cf_HaDataPhy = ADP_KVTOP(him_datap);

	ADBG_INIT(("?adp_cfginit: hsp paddr 0x%x\n", cfpp->Cf_HaDataPhy));

/*	Set to 2 per advice of Paul Von Stanwitz 04/26/95		*/
	cfpp->Cf_MaxNonTagScbs = 1;

	if ((status = PH_InitHA(cfpp)) != 0) {
		ADBG_INIT(("?adp_cfginit: initHA fail 0x%x\n", status));
		ddi_iopb_free((caddr_t)him_datap);
		return (FALSE);
	}

	drv_usecwait(2 * SEC_INUSEC);

/*	PH_InitHA may adjust the physical address of the data block	*/

	ADBG_INIT(("?adp_cfginit: hsp paddr 0x%x vaddr 0x%x\n",
		cfpp->Cf_HaDataPhy, cfpp->CFP_HaDataPtr));

/*
 *	PH_InitHA just enabled interrupts, but we are not ready
 *	to take interrupts, so disable them
 */
	PH_DisableInt(cfpp);

#ifdef COMMON_IO_EMULATION
	if ((adpp->ab_intr_idx = adp_xlate_vec(adpp)) < 0) {
		ddi_iopb_free((caddr_t)him_datap);
		return (FALSE);
	}
#else
	adpp->ab_intr_idx = 0;
#endif
	return (TRUE);
}




/*
 *
 * adp_do_abort_cmd()
 *
 *	Abort specific command on target device
 *
 *	adpp	- ptr to HBA soft-state structure
 *	tgtp	- ptr to target instance soft-state structure
 *	gcmdp	- ptr to GHD cmd
 *
 */

static int
adp_do_abort_cmd( adp_t *adpp, adptgt_t *tgtp, gcmd_t *gcmdp )
{
	ASSERT(adpp != NULL);
	ASSERT(tgtp != NULL);
	ASSERT(gcmdp != NULL);

	ADBG_ENT(("adp_do_abort_cmd: adpp 0x%x tgtp 0x%x gcmdp 0x%x\n",
			adpp, tgtp, gcmdp));

	PH_Special(ABORT_SCB, ADPP2CFPP(adpp), GCMDP2SCBP(gcmdp));
/* ??? do I have to check for error here or is SCB updated by HIM code ??? */

	return (TRUE);
}


/*
 *
 * adp_do_abort_dev()
 *
 *	Abort all outstanding requests on a specified device
 *	If for some reason the HIM code or the device can't
 *	clean up all the incomplete requests then one or more
 *	of them will be timed-out by GHD and cause additional
 *	
 *
 *	adpp	- ptr to HBA soft-state structure
 *	tgtp	- ptr to target instance soft-state structure
 *
 */

static int
adp_do_abort_dev( adp_t *adpp, adptgt_t *tgtp, gcmd_t *gcmdp1 )
{
	ccc_t	*cccp = &adpp->ab_ccc;
	gcmd_t	*gcmdp2;

	ASSERT(adpp != NULL);
	ASSERT(tgtp != NULL);

	ADBG_ENT(("adp_do_abort_dev: adpp 0x%x tgtp 0x%x\n", adpp, tgtp));

/* ??? move this code to ghd_timer.c to share with other drivers */
/* ??? ghd_register() should have a flag to indicate that abort_dev */
/* ??? needs to be simulated */

	/*
	 * The adp HIM code doesn't support a function to abort all
	 * active requests. Therefore, abort the current request (if
	 * any) and then scan the GHD timer list for all active
	 * requests and abort them one at a time.
	 */
	if (gcmdp1 != NULL) {
		adp_do_abort_cmd(adpp, tgtp, gcmdp1);
	}

	mutex_enter(&cccp->ccc_activel_mutex);
	gcmdp2 = (gcmd_t *)L2_next(&cccp->ccc_activel);
	while (gcmdp2) {
		if (gcmdp1 != gcmdp2
		&&  tgtp == gcmdp2->cmd_tgtp
		&&  gcmdp2->cmd_state == GCMD_STATE_ACTIVE) {
			/*
			 * change the state so if additional timeouts
			 * occur the abort and abort-dev steps will
			 * be skipped. Not really necessary but it
			 * should speedup error recovery on a busy system.
			 */
			gcmdp2->cmd_state = GCMD_STATE_ABORTING_DEV;

			adp_do_abort_cmd(adpp, tgtp, gcmdp2);
		}
		gcmdp2 = (gcmd_t *)L2_next(&gcmdp2->cmd_timer_link);
	}
	mutex_exit(&cccp->ccc_activel_mutex);
	return (TRUE);
}


/*
 *
 * adp_do_reset_target()
 *
 *	Reset a target device on the bus. The HIM code generates
 *	a Bus Device Reset command so that only the specified target
 *	device should be affected. The HIM code is supposed to cleanly
 *	reclaim all incomplete requests for the device. If that fails
 *	the GHD framework will eventually tell me to do a SCSI bus
 *	reset (see the following function).
 *
 */

static int
adp_do_reset_target( adp_t *adpp, adptgt_t *tgtp )
{
	cfp_struct	*cfpp = ADPP2CFPP(adpp);
	adpcmd_t	*cmdp;
	gcmd_t		*gcmdp;
	sp_struct	*scbp;

	ASSERT(adpp != NULL);
	ASSERT(tgtp != NULL);

	ADBG_ENT(("adp_do_reset_target: adpp 0x%x tgtp 0x%x\n", adpp, tgtp));

	if ((cmdp = kmem_zalloc(sizeof (*cmdp), KM_SLEEP)) == NULL) {
		return (FALSE);
	}

	if ((gcmdp = adp_scballoc(tgtp, cmdp, 0, 0, 0, 0)) == NULL) {
		kmem_free(cmdp, sizeof (*cmdp));
		return (FALSE);
	}
	scbp = GCMDP2SCBP(gcmdp);
	ASSERT(scbp != NULL);

	/* send Bus Device Reset to specified target */
	scbp->SP_Tarlun = tgtp->au_target << 4;
	scbp->SP_Cmd = HARD_RST_DEV;
	scbp->SP_ConfigPtr = cfpp;

	/*
	 * Add this request to the packet timer list and start its
	 * abort timer.
	 */
	gcmdp->cmd_state = GCMD_STATE_RESETTING_DEV;
	ghd_timer_start(&adpp->ab_ccc, gcmdp, 10);

	/*
	 * Send it off to the HIM layer, it's eventually
	 * (or perhaps immediately) returned via PH_ScbCompleted().
	 */
	PH_ScbSend(scbp);
#ifdef ADP_DEBUG
	if (scbp->SP_Stat == INV_SCB_CMD) {
		ADBG_ERROR(("scbp=0x%x\n", scbp));
		debug_enter("\n\nadp_do_reset_target\n\n");
	}
#endif

	return (TRUE);
}


/*
 *
 * adp_do_reset_bus()
 *
 *	Reset the SCSI bus. On the 2940 this is handled almost the same
 *	as the adp_do_reset_target() but this time with the HBA's
 *	target ID.
 *
 */

static int
adp_do_reset_bus( adp_t *adpp, adptgt_t *tgtp )
{
	cfp_struct	*cfpp = ADPP2CFPP(adpp);
	adpcmd_t	*cmdp;
	gcmd_t		*gcmdp;
	sp_struct	*scbp;

	if ((cmdp = kmem_zalloc(sizeof (*cmdp), KM_SLEEP)) == NULL) {
		return (FALSE);
	}

	if ((gcmdp = adp_scballoc(tgtp, cmdp, 0, 0, 0, 0)) == NULL) {
		kmem_free(cmdp, sizeof (*cmdp));
		return (FALSE);
	}
	scbp = GCMDP2SCBP(gcmdp);

	/*
	 * Use HBA's target ID to cause HIM code to reset the SCSI bus.
	 */
	scbp->SP_Tarlun = cfpp->Cf_ScsiId << 4;
	scbp->SP_Cmd = HARD_RST_DEV;
	scbp->SP_ConfigPtr = cfpp;

	/*
	 * Add this request to the packet timer list and start its
	 * abort timer.
	 */
	gcmdp->cmd_state = GCMD_STATE_RESETTING_BUS;
	ghd_timer_start(&adpp->ab_ccc, gcmdp, 10);

	/*
	 * Send it off to the HIM layer, it's eventually
	 * (or perhaps immediately) returned via PH_ScbCompleted().
	 */
	PH_ScbSend(scbp);
#ifdef ADP_DEBUG
	if (scbp->SP_Stat == INV_SCB_CMD) {
		ADBG_ERROR(("scbp=0x%x\n", scbp));
		debug_enter("\n\nadp_do_reset_bus\n\n");
	}
#endif

	return (TRUE);
}


/*
 * adp_reset_hba()
 *
 *	When all else fails, use a bigger hammer. Try doing a complete
 *	reset of the adapter. If it's dead, this just reclaims
 *	the incomplete requests and returns them to the target driver
 *	with hardware-error status. The target driver will eventually
 *	admit defeat ("I'm not dead yet" ... "you're not fooling anyone")
 *	and stop sending it's bleedin' requests.
 *
 *
 */

static int
adp_reset_hba( adp_t *adpp, adptgt_t *tgtp, gcmd_t *gcmdp )
{
	cfp_struct	*cfpp = ADPP2CFPP(adpp);

	ASSERT(mutex_owned(&adpp->ab_ccc.ccc_hba_mutex));

	/*
	 * reset the adapter
	 */
	cfpp->CFP_InitNeeded = TRUE;
	cfpp->CFP_ResetBus = TRUE;
	if (PH_InitHA(cfpp) != 0) {
		/*
		 * this shouldn't fail, if it does we're 
		 * completely hosed and a system reboot is probably
		 * needed
		 */
		ADBG_ERROR(("?adp_reset_hba() failed, bus=%d device=%d\n",
			    cfpp->Cf_BusNumber, cfpp->Cf_DeviceNumber));
		return (FALSE);
	}

	/* delay a bit to let things settle */
	drv_usecwait(adp_settle_time * SEC_INUSEC);

	/*
	 * I assume PH_InitHA() resets the adapter without cleaning
	 * up any incomplete I/O requests. So clear the incomplete I/O
	 * requests by scanning the timer queue. Any requests which were
	 * in progress just before the adapter reset should have an
	 * entry in the timer queue and can be completed with hardware
	 * error status.
	 */
	if (gcmdp != NULL) {
		sp_struct *scbp = GCMDP2SCBP(gcmdp);

		scbp->SP_HaStat = HOST_HW_ERROR;
		PH_ScbCompleted(scbp);
	}

	/*
	 * The HBA mutex is held, therefore no one is going to
	 * modify the the ccc_activel list.
	 *
	 * I.e., it's safe to call L2_next() without grabbing the
	 * list mutex.
	 */
	while (gcmdp = (gcmd_t *)L2_next(&adpp->ab_ccc.ccc_activel)) {
		sp_struct *scbp = GCMDP2SCBP(gcmdp);

		scbp->SP_HaStat = HOST_HW_ERROR;
		PH_ScbCompleted(scbp);
	}
	return (TRUE);
}



/*
 * adp_timeout_action()
 *
 *
 *      Called when a request has timed out. Start out subtle and
 *      just try to abort the specific request. Escalate all the
 *      way upto resetting the HBA.
 *
 *
 */

static int
adp_timeout_action(	void	*hba_handle,
                        gcmd_t	*gcmdp,
			void	*tgt_handle,
			gact_t	 action )
{
        adp_t		*adpp = hba_handle;
	adptgt_t	*tgtp = tgt_handle;
        struct scsi_pkt *pktp;

	ASSERT(mutex_owned(&adpp->ab_ccc.ccc_hba_mutex));

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
		ghd_complete(&adpp->ab_ccc, gcmdp);
                return (TRUE);

        case GACTION_EARLY_TIMEOUT:
                /* timeout before request was started */
                if (pktp != NULL) {
                        if (pktp->pkt_reason == CMD_CMPLT)
                                pktp->pkt_reason = CMD_TIMEOUT;
                        pktp->pkt_statistics |= STAT_TIMEOUT;
                }
		ghd_complete(&adpp->ab_ccc, gcmdp);
                return (TRUE);

        case GACTION_ABORT_CMD:
		/* timeout while active or scsi_abort() called */
		ASSERT(adpp != NULL);
                ASSERT(gcmdp != NULL);
		ASSERT(GCMDP2CMDP(gcmdp) != NULL);
		return (adp_do_abort_cmd(adpp, tgtp, gcmdp));

        case GACTION_ABORT_DEV:
		/* abort cmd failed or scsi_abort(NULL) called */
		ASSERT(adpp != NULL);
		ASSERT(tgtp != NULL);
		return (adp_do_abort_dev(adpp, tgtp, gcmdp));

        case GACTION_RESET_TARGET:
		/* abort dev failed or scsi_reset() called */
		ASSERT(adpp != NULL);
		ASSERT(tgtp != NULL);
		return (adp_do_reset_target(adpp, tgtp));

        case GACTION_RESET_BUS:
		/* reset target failed or scsi_reset(NULL) called */
		ASSERT(adpp != NULL);
                return (adp_do_reset_bus(adpp, tgtp));

	case GACTION_INCOMPLETE:
		/* the HBA is kaput, reset it and clear the GHD timer list */
		ASSERT(adpp != NULL);
                return (adp_reset_hba(adpp, tgtp, gcmdp));

        }
        return (FALSE);
}


/*
 *
 * adp_tran_abort()
 *
 *      Abort specific command on a target or all commands on
 *	a specific target.
 *
 */

static int
adp_tran_abort( struct scsi_address *ap, struct scsi_pkt *pktp )
{
        if (pktp) {
                return (ghd_tran_abort(&ADDR2ADPP(ap)->ab_ccc, PKTP2GCMDP(pktp),
					ap, NULL));
	}
        return (ghd_tran_abort_lun(&ADDR2ADPP(ap)->ab_ccc, ap, NULL));
}


/*
 *
 * adp_tran_reset()
 *
 * 	reset the scsi bus, or just one target device
 *
 * returns	0 == failure
 *		1 == success
 *
 */

static int
adp_tran_reset( struct scsi_address *ap, int level )
{
        if (level == RESET_TARGET)
                return (ghd_tran_reset_target(&ADDR2ADPP(ap)->ab_ccc,
					      ADDR2TGTP(ap), NULL));
        if (level == RESET_ALL)
                return (ghd_tran_reset_bus(&ADDR2ADPP(ap)->ab_ccc,
					   ADDR2TGTP(ap), NULL));
        return (FALSE);
}




static int
adp_capchk( char *cap, int tgtonly, int *cidxp )
{
	int	cidx;

	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *) 0)
		return (FALSE);

	if ((cidx = scsi_hba_lookup_capstr(cap)) == -1)
		return (FALSE);

	*cidxp = cidx;
	return (TRUE);
}

static int
adp_tran_getcap( struct scsi_address *ap, char *cap, int tgtonly )
{
	int	ckey;

	if (adp_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {

		case SCSI_CAP_GEOMETRY:
		{
			int	total_sectors, h, s;

			total_sectors = (ADDR2TGTP(ap))->au_total_sectors;

			if ((ADDR2ADPP(ap)->ab_flag & ADP_FLAG_GT1GIG)
			&&  total_sectors > 0x200000) {
				h = 255;
				s = 63;
			} else {
				h = 64;
				s = 32;
			}
			return (ADP_SETGEOM(h, s));

		}
		case SCSI_CAP_ARQ:
			return (TRUE);
		default:
			break;
	}
	return (UNDEFINED);
}

static int
adp_tran_setcap( struct scsi_address *ap, char *cap, int value, int tgtonly )
{
	int	ckey, status = FALSE;

	if (adp_capchk(cap, tgtonly, &ckey) != TRUE) {
		return (UNDEFINED);
	}

	switch (ckey) {
		case SCSI_CAP_SECTOR_SIZE:
			(ADDR2TGTP(ap))->au_lim.dlim_granular = (u_int)value;
			status = TRUE;
			break;

		case SCSI_CAP_ARQ:
			if (tgtonly) {
				(ADDR2TGTP(ap))->au_arq = (u_int)value;
				status = TRUE;
			}
			break;

		case SCSI_CAP_TOTAL_SECTORS:
			(ADDR2TGTP(ap))->au_total_sectors = value;
			status = TRUE;
			break;

		case SCSI_CAP_GEOMETRY:
		default:
			break;
	}
	return (status);
}



static struct scsi_pkt *
adp_tran_init_pkt(	struct scsi_address *ap,
			struct scsi_pkt	*pktp,
			struct buf	*bp,
			int		 cmdlen,
			int		 statuslen,
			int		 tgtlen,
			int		 flags,
			int		(*callback)(),
			caddr_t		 arg )
{
	adptgt_t	*tgtp = ADDR2TGTP(ap);
	adp_t		*adpp = TGTP2ADPP(tgtp);
	struct scsi_pkt *new_pktp;
	sp_struct	*scbp;

	ADBG_ENT(("adp_tran_init_pkt: ap 0x%x pktp 0x%x bp 0x%x\n",
		  ap, pktp, bp));
	ADBG_ENT(("\t cmdlen %d slen %d tlen %d flg 0x%x\n",
		  cmdlen, statuslen, tgtlen, flags));


	/*
	 * The HIM code needs an phys buffer for the Req Sense data.
	 * Make certain to allocate at least enough room for
	 * a scsi_extended_sense structure.
	 */
	if (statuslen < sizeof (struct scsi_arq_status)) {
		statuslen = sizeof (struct scsi_arq_status);
	}

	/*
	 * call the GHD pkt allocator with appropriate args
	 */
	new_pktp = ghd_tran_init_pkt(&adpp->ab_ccc, ap, pktp, bp, cmdlen,
				     statuslen, tgtlen, flags, callback,
				     arg, sizeof (adpcmd_t), &tgtp->au_lim);

	if (!new_pktp)
		return (NULL);

	/*
	 * initialize target and lun id
	 */
	scbp = PKTP2SCBP(new_pktp);
	scbp->SP_Tarlun = (ap->a_target << 4) | ap->a_lun;

#if 0
	/* turn on ARQ in the SCB if target driver expects it */
	scbp->SP_AutoSense = tgtp->au_arq;
#endif

	if (bp) {
		if (!bp->b_bcount) {
			scbp->SP_RejectMDP = 0;
			/*  XXX workaround for scdk_start_drive mistake */
			SCBP2CMDP(scbp)->cmd_sg_cnt = 0;
		}
	}
	return (new_pktp);
}


static void
adp_tran_destroy_pkt( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	ADBG_ENT(("adp_tran_destroy_pkt: ap 0x%x pktp 0x%x\n", ap, pktp));

	ghd_dmafree(PKTP2GCMDP(pktp));
	ghd_pktfree(&ADDR2ADPP(ap)->ab_ccc, ap, pktp);
}


/*
 * adp_scballoc()
 *
 *	carve up the the buffer allocated by GHD into my cmd_t 
 *	structure and GHD's gcmd_t structure. Allocate a SCB
 *	structure from IOPB space. Initializing everything and
 *	return the ptr to the gcmd_t to GHD.
 *
 */

static gcmd_t *
adp_scballoc(   void	*tgt_private,
                void    *bufp,
                int      cmdlen,
                int      statuslen,
                int      tgtlen,
                int      ccblen )

{
	adptgt_t *tgtp = (adptgt_t *)tgt_private;
	adpcmd_t *cmdp = (adpcmd_t *)bufp;
	adp_t	 *adpp = TGTP2ADPP(tgtp);
	gcmd_t	 *gcmdp = CMDP2GCMDP(cmdp);
	sp_struct *scbp;
	int	  senselen;

#define	SIZEOF_ARQ_HEADER	(sizeof (struct scsi_arq_status)	\
				- sizeof (struct scsi_extended_sense))
	/*
	 * Determine just the size of the Request Sense Data buffer within
	 * the scsi_arq_status structure. Allocate at least enough room
	 * for the scsi_extended_sense structure (20 bytes).
	 */

	senselen = statuslen - SIZEOF_ARQ_HEADER;

	/* allocate ccb from IOPB memory pool */
	if (ddi_iopb_alloc(adpp->ab_dip, &tgtp->au_lim,
			   sizeof (*scbp) + senselen,
			   (caddr_t *)&scbp) != DDI_SUCCESS) {
		return (NULL);
	}

	bzero((caddr_t)scbp, sizeof (*scbp) + senselen);

	/*
	 * initialize the HIM layer scb
	 */
	scbp->SP_Cmd = EXEC_SCB;
	scbp->SP_ConfigPtr = ADPP2CFPP(adpp);

	scbp->Sp_paddr = ADP_KVTOP(scbp);

	/*
	 * set SCSI cdb physical address
	 */
	scbp->SP_CDBPtr = scbp->Sp_paddr +
		((u_char *)&(scbp->Sp_CDB[0]) - (u_char *)scbp);

	/*
	 * Save Auto Request Sense Data virtual address and length
	 */

	/* ARQ Sense Data buffer follows the SCB buffer */
	cmdp->cmd_sensep = (caddr_t)(scbp + 1);
	cmdp->cmd_senselen = senselen;

	/*
	 * Save the length of the SCSI Command
	 */
	cmdp->cmd_cdblen = cmdlen;

	/*
	 * Even if this is a single segment xfer (according to the
	 * HIM Spec) set this bit which is related to Scatter/Gather-mode.
	 */
	scbp->SP_RejectMDP = 1;

	ADBG_PKT(("?adp_scballoc: gcmdp 0x%x cmdp 0x%x scbp 0x%x\n",
			gcmdp, cmdp, scbp));

	/*
	 * cross-link the kmem and iopb buffers
	 */
	scbp->Sp_cmdp = cmdp;
	cmdp->cmd_sp = scbp;

	GHD_GCMD_INIT(gcmdp, cmdp, tgtp);

	/*
	 * return to GHD the ptr to its gcmd_t structure
	 */
	return (gcmdp);
}


/*
 * SCB free
 */
static void
adp_scbfree( void *p )
{
	ADBG_ENT(("adp_scbfree: p 0x%x\n", p));

	ddi_iopb_free((caddr_t)((adpcmd_t *)p)->cmd_sp);
	return;
}


/*
 * Dma resource deallocation
 */

/*ARGSUSED*/
static void
adp_tran_dmafree( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	ADBG_ENT(("adp_tran_dmafree: ap 0x%x pktp 0x%x\n", ap, pktp));

	ghd_dmafree(PKTP2GCMDP(pktp));
	return;
}


/*
 * Dma sync
 */

/*ARGSUSED*/
static void
adp_tran_sync_pkt( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	ghd_tran_sync_pkt(ap, pktp);
	return;
}


/*
 *
 * adp_sg_func()
 *
 *	When the GHD framework does the DMA resource allocation
 *	it calls this function to build up the scatter/gather
 *	list one segment at a time.
 *
 *	gcmdp		ptr to the GHD cmd buffer
 *	dmackp		ptr to the current DMA cookie
 *	single_segment	TRUE if this I/O request will fit in a 
 *			DMA cookie
 *	seg_index	[0 ... N] index of the current S/G segment
 *			increments on each call
 *
 */

static void
adp_sg_func(	gcmd_t	*gcmdp,
		ddi_dma_cookie_t *dmackp,
		int	 single_segment,
		int	 seg_index )
{
	adpcmd_t	*cmdp = GCMDP2CMDP(gcmdp);
	sp_struct	*scbp = CMDP2SCBP(cmdp);
	struct adp_sg	*dmap;

	ADBG_ENT(("adp_sg_func: gcmdp 0x%x dmackp 0x%x s %d idx %d\n",
		gcmdp, dmackp, single_segment, seg_index));

	/* set address of current entry in scatter/gather list */
	dmap = scbp->Sp_sg_list + seg_index;

	/* store the phys addr and count from the cookie */
	dmap->data_len = (ulong) dmackp->dmac_size;
	dmap->data_addr = (ulong) dmackp->dmac_address;

	/* save the count of scatter/gather segments */
	cmdp->cmd_sg_cnt = seg_index + 1;
	return;
}


/*ARGSUSED*/
static int
adp_tran_start( struct scsi_address *ap, struct scsi_pkt *pktp )
{
	adpcmd_t  *cmdp = PKTP2CMDP(pktp);
	sp_struct *scbp = CMDP2SCBP(cmdp);
	adp_t	  *adpp = ADDR2ADPP(ap);

	/* normal SCSI CDB */
	scbp->SP_Cmd = EXEC_SCB;

#if 1
	/* always reset ARQ because HIM code sometimes clobbers this bit */
	scbp->SP_AutoSense = ADDR2TGTP(ap)->au_arq;
#else
	/*
	 * save data clobbered by HIM code when it does error recovery
	 */
	 scbp->Sp_control_save = scbp->Sp_control;
#endif

	/* set the phys addr of the sense buffer and its length */
	scbp->Sp_SensePtr = scbp->Sp_paddr + (cmdp->cmd_sensep - (caddr_t)scbp);
	scbp->Sp_SenseLen = cmdp->cmd_senselen;

	/* copy the CDB to the HIM code buffer */
	bcopy((char *)pktp->pkt_cdbp, (char *)scbp->Sp_CDB, cmdp->cmd_cdblen);
	scbp->SP_CDBLen = cmdp->cmd_cdblen;

	/* set the physical address of scatter gather list */
	if ((scbp->SP_SegCnt = cmdp->cmd_sg_cnt) != 0) {
		scbp->SP_SegPtr = (paddr_t)(scbp->Sp_paddr +
				((caddr_t)scbp->Sp_sg_list - (caddr_t)scbp));
	} else {
		scbp->SP_SegPtr = 0;
	}

/*	initialize in case of packet reuse 				*/
	scbp->SP_TargStat = 0;
	scbp->SP_HaStat = 0;
	scbp->SP_ResCnt = 0;
	pktp->pkt_state = 0;
	pktp->pkt_statistics = 0;
	pktp->pkt_resid = 0;
	pktp->pkt_reason = CMD_CMPLT;


/*	Set default, then handle special cases				*/
/*	Do this here because of packet reuse by both target and him	*/
	scbp->SP_DisEnable = 1;

#ifdef nodiscon_is_a_bad_idea
/*	disable disconnect when requested or for polling		*/
	if (pktp->pkt_flags & (FLAG_NODISCON | FLAG_NOINTR)) {
		scbp->SP_DisEnable = 0;
	}
#else
	/*
	 * NODISCON should always be controlled by the HIM code. It
	 * "freezes" the bus when polling slow devices and confuses
	 * devices which do tagged queuing with multiple initiators.
	 */
#endif

	ADBG_PKT(("?adp_tran(%d,%d): scbp 0x%x adpp 0x%x pktp 0x%x\n",
			ap->a_target, ap->a_lun, scbp, adpp, pktp));

	/*
	 * Stick the request on the tail of the wait queue
	 */
	return (ghd_transport(&adpp->ab_ccc, PKTP2GCMDP(pktp),
			      PKTP2TARGET(pktp), pktp->pkt_time,
			      ((pktp->pkt_flags & FLAG_NOINTR) ? TRUE : FALSE),
			      NULL));
}


/*
 * Start the first request from the head of the wait queue.
 */
static int
adp_start( void *hba_handle, gcmd_t *gcmdp )
{
	register sp_struct *scbp = GCMDP2SCBP(gcmdp);

	ADBG_ENT(("adp_start: scbp 0x%x cdblen %d cnt %d ptr 0x%x\n",
		   scbp, scbp->SP_CDBLen, scbp->SP_SegCnt, scbp->SP_SegPtr));
	ADBG_ENT(("\t CDB[0..11] %x %x %x %x %x %x %x %x %x %x %x %x\n",
		  scbp->Sp_CDB[0], scbp->Sp_CDB[1], scbp->Sp_CDB[2],
		  scbp->Sp_CDB[3], scbp->Sp_CDB[4], scbp->Sp_CDB[5],
		  scbp->Sp_CDB[6], scbp->Sp_CDB[7], scbp->Sp_CDB[8],
		  scbp->Sp_CDB[9], scbp->Sp_CDB[10], scbp->Sp_CDB[11]));

	PH_ScbSend(scbp);

#ifdef ADP_DEBUG
	if (GCMDP2SCBP(gcmdp)->SP_Stat == INV_SCB_CMD) {
		ADBG_ERROR(("hba=0x%x gcmdp=0x%x scbp=0x%x\n",
			hba_handle, gcmdp, GCMDP2SCBP(gcmdp)));
		debug_enter("\n\nadp_start\n\n");
	}
#endif
	return (TRAN_ACCEPT);
}



/*ARGSUSED*/
static void
adp_chkstatus( struct scsi_pkt *pktp, sp_struct *scbp )
{
	static struct scsi_status zero_scsi_status = { 0 };

	ASSERT(scbp->SP_HaStat < ADP_UNKNOWN_ERROR);

	pktp->pkt_state = 0;
	*pktp->pkt_scbp = scbp->SP_TargStat;
	pktp->pkt_reason = adp_errs[scbp->SP_HaStat];


	ADBG_STUS(("?adp_chkstatus(%d,%d): scbp 0x%x hstat 0x%x tstat 0x%x\n",
		   (scbp->SP_Tarlun >> 4), (scbp->SP_Tarlun & LUN), scbp,
		   scbp->SP_HaStat, scbp->SP_TargStat));
	ADBG_STUS(("\t reason 0x%x cdblen %d cnt %d ptr 0x%x\n",
		   pktp->pkt_reason, scbp->SP_CDBLen, scbp->SP_SegCnt,
		   scbp->SP_SegPtr));
		   
	if (pktp->pkt_reason == CMD_CMPLT
	&&  scbp->SP_TargStat == UNIT_GOOD) {
		pktp->pkt_resid = 0;
		pktp->pkt_state = (STATE_XFERRED_DATA|STATE_GOT_BUS|
				   STATE_GOT_TARGET|STATE_SENT_CMD|
				   STATE_GOT_STATUS);
		return;
	}

	if (scbp->SP_TargStat == UNIT_CHECK
	&&  scbp->SP_AutoSense) {
		if (scbp->SP_HaStat != HOST_SNS_FAIL) {
			struct	scsi_arq_status *arqp;

			ADBG_STUS(("?adp_chkstatus: ARQ okay scbp 0x%x\n",
					scbp));

			pktp->pkt_reason = CMD_CMPLT;
			pktp->pkt_state |= (STATE_GOT_BUS|STATE_GOT_TARGET|
					     STATE_SENT_CMD|STATE_GOT_STATUS|
					     STATE_ARQ_DONE);

			arqp = (struct scsi_arq_status *)pktp->pkt_scbp;
			arqp->sts_rqpkt_reason = CMD_CMPLT;
			arqp->sts_rqpkt_state = STATE_XFERRED_DATA;
			arqp->sts_rqpkt_status = zero_scsi_status;
			arqp->sts_rqpkt_resid = 0;
			arqp->sts_rqpkt_statistics = 0;

			/* copy the sense data to target-driver's buffer */
			bcopy((caddr_t)PKTP2CMDP(pktp)->cmd_sensep,
			      (caddr_t)&arqp->sts_sensedata,
			      PKTP2CMDP(pktp)->cmd_senselen);

		} else {
			/* pkt_reason has been set to CMD_TRAN_ERR */
			ADBG_STUS(("?adp_chkstatus: ARQ failed scbp 0x%x\n",
					scbp));
		}
	}


	switch (scbp->SP_HaStat) {
	case  HOST_DU_DO:
		ADBG_STUS(("?adp_chkstatus: %s: resid 0x%x\n",
			(scbp->SP_ResCnt == 0 ? "overrun" : "underrun"),
			scbp->SP_ResCnt));
		if (scbp->SP_ResCnt != 0) {
			/* underrun per Frits of scsi-steer */
			pktp->pkt_reason = CMD_CMPLT;
			pktp->pkt_resid = scbp->SP_ResCnt;
		}

		pktp->pkt_state = (STATE_XFERRED_DATA|STATE_GOT_BUS|
				   STATE_GOT_TARGET|STATE_SENT_CMD|
				   STATE_GOT_STATUS);
		break;

	case  HOST_SEL_TO:
		pktp->pkt_statistics |= STAT_TIMEOUT;
		pktp->pkt_state |= STATE_GOT_BUS;
		ADBG_STUS(("?adp: selection timeout\n"));
		break;

	case  HOST_ABT_HA:
	case  HOST_ABT_HOST:
		ADBG_STUS(("?adp: cmd aborted\n"));
		pktp->pkt_statistics |= STAT_ABORTED;
		break;

	case  HOST_RST_HA:
	case  HOST_RST_OTHER:
		ADBG_STUS(("?adp: bus reset\n"));
		pktp->pkt_statistics |= STAT_DEV_RESET;
		break;

	default:
		break;
	}

#if 0
	/* restore scb fields clobbered by the HIM code */
	bcopy((char *)&scbp->Sp_control_save, (char *)&scbp->Sp_control, 
		sizeof (scbp->Sp_control));
#endif
	return;
}

/* Autovector Interrupt Entry Point */

static u_int
adp_intr( caddr_t arg )
{
	adp_t	*adpp = (adp_t *)arg;

	return (ghd_intr(&adpp->ab_ccc, NULL));
}

static void
adp_process_intr( void *hba_handle, void *arg )
{
	PH_IntHandler(ADPP2CFPP(((adp_t *)hba_handle)));
	return;
}

static int
adp_get_status( void *hba_handle, void *arg )
{
	return (PH_PollInt(ADPP2CFPP(((adp_t *)hba_handle))));
}

/*
 *
 * PH_ScbCompleted()
 *
 *	This function is called from PH_IntHandler HIM layer for each
 *	completed SCB. It may be called multiple times per interrupt.
 *
 *	The HBA mutex is held.
 *
 */
void
PH_ScbCompleted( sp_struct *scbp )
{
	gcmd_t	*gcmdp = SCBP2GCMDP(scbp);


	/*
	 * Process completions of normal SCSI requests
	 */
	if (scbp->SP_Cmd == EXEC_SCB) {
		struct scsi_pkt *pktp = GCMDP2PKTP(gcmdp);
		adp_t	*adpp = PKTP2ADPP(pktp);

		ASSERT(pktp != NULL);
		ASSERT(mutex_owned(&adpp->ab_ccc.ccc_hba_mutex));

		/*
		 * translate HIM status codes to SCSA status
		 */
		adp_chkstatus(pktp, scbp);

		/*
		 * add it to the done queue to await completion callback
		 */
		ghd_complete(&adpp->ab_ccc, gcmdp);
		return;
	}

	/*
	 * Process completions of internal reset requests. These
	 * don't use a scsi_pkt and don't have a completion callback.
	 */
	if (scbp->SP_Cmd == HARD_RST_DEV) {
		adpcmd_t *cmdp = GCMDP2CMDP(gcmdp);
		adptgt_t *tgtp = gcmdp->cmd_tgtp;
		adp_t	 *adpp = TGTP2ADPP(tgtp);

		ASSERT(cmdp != NULL);
		ASSERT(mutex_owned(&adpp->ab_ccc.ccc_hba_mutex));

		/*
		 * stop the timer for this request
		 */
		ghd_timer_stop(&adpp->ab_ccc, gcmdp);

		/*
		 * just free the cmd and SCB buffers
		 */
		kmem_free(cmdp, sizeof (*cmdp));
		ddi_iopb_free((caddr_t)scbp);
		return;
	}

	/* shouldn't happen */
	ADBG_ERROR(("adp: PH_ScbCompleted, invalid scbp 0x%x\n", scbp));
	return;
}

#ifdef COMMON_IO_EMULATION
static int
adp_xlate_vec( adp_t *adpp )
{
	int	vec = ADPP2CFPP(adpp)->Cf_IrqChannel;
	int	intrspec[3];

	if (vec < 3 || vec > 15) {
		ADBG_INIT(("?adp_xlate_vec: bad IRQ %d\n", vec));
		return (-1);
	}

	/* create an interrupt spec using default interrupt priority level */
	intrspec[0] = 2;
	intrspec[1] = 5;
	intrspec[2] = vec; /* set irq */

	if (ddi_ctlops(adpp->ab_dip, adpp->ab_dip, DDI_CTLOPS_XLATE_INTRS,
		(caddr_t)intrspec, ddi_get_parent_data(adpp->ab_dip))
	!= DDI_SUCCESS) {
		ADBG_INIT(("?adp_xlate_vec: interrupt create failed\n"));
		return (-1);
	}

	return (0);
}
#endif

#ifdef ADP_DEBUG

void
adp_dump_adp( adp_t *adpp )
{
	cfp_struct *cfpp = ADPP2CFPP(adpp);
	int	i;

	PRF("id 0x%x dip 0x%x ioaddr 0x%x IRQ %d int_idx %d conf flg 0x%x",
		cfpp->CFP_AdapterId, adpp->ab_dip,
		cfpp->CFP_BaseAddress, cfpp->Cf_IrqChannel,
		adpp->ab_intr_idx, cfpp->CFP_ConfigFlags);

	PRF(" dma 0x%x him_data 0x%x flg 0x%x\n", cfpp->Cf_DmaChannel,
		cfpp->CFP_HaDataPtr, adpp->ab_flag);

	PRF("\nSCSI options ");
	for (i = 0; i < cfpp->Cf_MaxTargets; i++) {
		PRF(" 0x%x ", cfpp->Cf_ScsiOption[i]);
	}
	PRF("\n");
	PRF("cccp 0x%x\n", &adpp->ab_ccc);
}

char *_HBA_status[] = {
	"No adapter status available",		/* 0x00 */
	"HBA Unknown Status",			/* 0x01 */
	"HBA Unknown Status",			/* 0x02 */
	"HBA Unknown Status",			/* 0x03 */
	"Command aborted by host",		/* 0x04 */
	"HBA Unknown Status",			/* 0x05 */
	"HBA Unknown Status",			/* 0x06 */
	"HBA Unknown Status",			/* 0x07 */
	"HBA Unknown Status",			/* 0x08 */
	"HBA Unknown Status",			/* 0x09 */
	"HBA Unknown Status",			/* 0x0a */
	"HBA Unknown Status",			/* 0x0b */
	"HBA Unknown Status",			/* 0x0c */
	"HBA Unknown Status",			/* 0x0d */
	"HBA Unknown Status",			/* 0x0e */
	"HBA Unknown Status",			/* 0x0f */
	"HBA Unknown Status",			/* 0x10 */
	"Selection timeout",			/* 0x11 */
	"Data overrun or underrun error",	/* 0x12 */
	"Unexpected bus free",			/* 0x13 */
	"Target bus phase sequence error",	/* 0x14 */
	"HBA Unknown Status",			/* 0x15 */
	"HBA Unknown Status",			/* 0x16 */
	"Invalid SCSI linking operation",	/* 0x17 */
	"HBA Unknown Status",			/* 0x18 */
	"HBA Unknown Status",			/* 0x19 */
	"HBA Unknown Status",			/* 0x1a */
	"Auto request sense failed",		/* 0x1b */
	"Tagged Queuing rejected by target",	/* 0x1c */
	"HBA Unknown Status",			/* 0x1d */
	"HBA Unknown Status",			/* 0x1e */
	"HBA Unknown Status",			/* 0x1f */
	"Host adpater hardware error",		/* 0x20 */
	"Target did'nt respond to ATN (RESET)",	/* 0x21 */
	"SCSI bus reset by host adapter",	/* 0x22 */
	"SCSI bus reset by other device",	/* 0x23 */
	"HBA Unknown Status",			/* 0x24 */
	"HBA Unknown Status",			/* 0x25 */
	"HBA Unknown Status",			/* 0x26 */
	"HBA Unknown Status",			/* 0x27 */
	"HBA Unknown Status",			/* 0x28 */
	"HBA Unknown Status",			/* 0x29 */
	"HBA Unknown Status",			/* 0x2a */
	"HBA Unknown Status",			/* 0x2b */
	"HBA Unknown Status",			/* 0x2c */
	"HBA Unknown Status",			/* 0x2d */
	"HBA Unknown Status",			/* 0x2e */
	"HBA Unknown Status",			/* 0x2f */
	"No Available Index"			/* 0x30 */
};

void
adp_dump_scb( sp_struct *scbp )
{
	int i;

	PRF("Cmd 0x%x targ %d lun %d addr adp_scsi_cmd 0x%x\n",
		(unchar)(scbp->SP_Cmd), scbp->SP_Tarlun >> 4,
		scbp->SP_Tarlun & LUN, scbp->Sp_cmdp);

	if (scbp->SP_AutoSense)
		PRF("Auto Request Sense Enabled\n");
	else
		PRF("No Auto Request Sense\n");

	if (scbp->SP_NoUnderrun)
		PRF("Data Underrun, not considered as error\n");
	else
		PRF("Data Underrun, considered as error\n");

	if (scbp->SP_DisEnable)
		PRF("Allow target disconnection\n");
	else
		PRF("No target disconnection\n");

	if (scbp->SP_TagEnable)
		PRF("Tagged Queuing supported\n");
	else
		PRF("No tagged Queuing\n");

	PRF("SegCnt 0x%x SegPtr 0x%x CDBLen 0x%x\n", scbp->SP_SegCnt,
		scbp->SP_SegPtr, scbp->SP_CDBLen);

	PRF("SP_MgrStat 0x%x SP_Stat 0x%x SP_ResCnt 0x%x\n",
		scbp->SP_MgrStat, scbp->SP_Stat, scbp->SP_ResCnt);

	PRF("CDB: ");
	for (i = 0; i < scbp->SP_CDBLen; i++) {
		PRF("0x%x ", scbp->Sp_CDB[i]);
	}
	PRF("\nSensePtr 0x%x SenseLen 0x%x\n", scbp->Sp_SensePtr,
		scbp->Sp_SenseLen);

	i = (unchar)(scbp->SP_HaStat);
	if (i > HOST_NOAVL_INDEX)
		i = 0x24;
	PRF("HBA Status = 0x%x,  %s\n", i, _HBA_status[i]);

	PRF("Target Status 0x%x\n", scbp->SP_TargStat);
}
#endif

/* The following functions need to be supplied for HIM */


/*ARGSUSED*/
ushort
Ph_CalcStandardSize( ushort number_scbs )
{
	return (0);
}

/*ARGSUSED*/
void
Ph_SetStandardHaData( cfp_struct  *cfpp )
{
	ASSERT(cfpp != NULL);
}

/*ARGSUSED*/
void
Ph_GetStandardConfig( cfp_struct  *cfpp )
{
	ASSERT(cfpp != NULL);
}

/*ARGSUSED*/
void
Ph_StandardLoadFuncPtrs( cfp_struct  *cfpp )
{
	ASSERT(cfpp != NULL);
}

/*ARGSUSED*/
ulong
PH_ReadConfigOSM( cfp_struct *cfpp, u_char bus, u_char dev, u_char reg )
{
	ddi_acc_handle_t	cfg_handle;
	ulong ret;

	ASSERT(CFPP2ADPP(cfpp)->ab_dip != NULL);

	if (pci_config_setup(CFPP2ADPP(cfpp)->ab_dip, &cfg_handle)
				!= DDI_SUCCESS) {
		pci_config_teardown(&cfg_handle);
		return (NO_CONFIG_OSM);
	}

	ret = pci_config_getl(cfg_handle, (ulong) reg);
	pci_config_teardown(&cfg_handle);

	return (ret);
}

/*ARGSUSED*/
ulong
PH_WriteConfigOSM(	cfp_struct *cfpp,
			u_char	bus,
			u_char	dev,
			u_char	reg,
			ulong	val )
{
	ddi_acc_handle_t	cfg_handle;

	ASSERT(CFPP2ADPP(cfpp)->ab_dip != NULL);

	if (pci_config_setup(CFPP2ADPP(cfpp)->ab_dip, &cfg_handle)
				!= DDI_SUCCESS) {
		pci_config_teardown(&cfg_handle);
		return (NO_CONFIG_OSM);
	}

	pci_config_putl(cfg_handle, (ulong) reg, val);
	pci_config_teardown(&cfg_handle);

	return (val);
}

ulong
PH_GetNumOfBusesOSM( )
{
	return (NO_CONFIG_OSM);
}
