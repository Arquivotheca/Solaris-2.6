/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)corv.c	1.7	96/08/13 SMI"

/*
 * Corvette - also known as IBM SCSI-2 Fast/Wide Adapter/A
 */

#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#include <sys/varargs.h>
#include <sys/modctl.h>

#include "corv.h"

/*
 * External references
 */

static	int	corv_tran_tgt_init(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
static	int	corv_tran_tgt_probe(struct scsi_device *, int (*)(void *));
static	void	corv_tran_tgt_free(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);

static	int	corv_transport(struct scsi_address *ap, struct scsi_pkt *pktp);
static	int	corv_reset(struct scsi_address *ap, int level);
static	int	corv_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static	int	corv_capchk(char *cap, int tgtonly, int *cidxp);
static	int	corv_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static	int	corv_setcap(struct scsi_address *ap, char *cap, int value,
			int tgtonly);
static	struct scsi_pkt *corv_tran_init_pkt(struct scsi_address *ap,
			struct scsi_pkt *pkt, struct buf *bp, int cmdlen,
			int statuslen, int tgtlen, int flags, int (*callback)(),
			caddr_t arg);
static	void	corv_tran_destroy_pkt(struct scsi_address *ap,
			struct scsi_pkt *pkt);
static struct	scsi_pkt *corv_pktalloc(struct scsi_address *ap, int cmdlen,
			int statuslen, int tgtlen, int (*callback)(),
			caddr_t arg);
static	void	corv_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt);
static	CORV_CCB *corv_ccballoc(CORV_BLK *corv_blkp, CORV_INFO *corvp);
static	void	corv_ccbfree(CORV_BLK *corv_blkp, CORV_CCB *ccbp);
static	struct scsi_pkt *corv_dmaget(struct scsi_pkt *pkt, opaque_t dmatoken,
			int (*callback)(), caddr_t arg);
static	void	corv_tran_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);
static	void	corv_tran_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);

/*
 * Local Function Prototypes
 */
static	int	corv_hba_reset(u_int ioaddr);
static	int	corv_exist(u_int ioaddr, unchar *);
static	int	corv_propinit(dev_info_t *devi, CORV_BUS *sbp);
static	int	corv_cfginit(CORV_BLK *corv_blkp);
static	void	corv_getpos(CORV_BLK *corv_blkp);
static	int	corv_xlate_irq(CORV_BLK *corv_blkp);
static	void	corv_disable(CORV_BLK *corv_blkp);
static	int	corv_enable(CORV_BLK *corv_blkp);
static	int	corv_docmd(CORV_BLK *corv_blkp, int cmd, unchar dev,
				unchar opcode);
static	int	corv_wait(ushort ioaddr, long waittime, unchar *bsr_valp,
				unchar mask, unchar onbits, unchar offbits);
static	int	corv_pollret(CORV_BLK *corv_blkp);
static	int	corv_mm_pollret(CORV_BLK *corv_blkp, CORV_CCB *ccbp);
static	u_int	corvintr(caddr_t arg);
static	u_int	corv_dummy_intr(caddr_t arg);

static	int	corv_get_eid(CORV_BLK *corv_blkp, CORV_INFO *corvp, unchar bus,
				ushort target, unchar lun, int entity_id);
static	void	corv_free_eid(CORV_BLK *corv_blkp, CORV_INFO *corvp);
static	CORV_INFO *corv_map_target(CORV_BLK *corv_blkp,
				scsi_hba_tran_t	*hba_tran, unchar bus,
				ushort targ, unchar lun);
static	void	corv_unmap_target(CORV_BLK *corv_blkp, CORV_INFO *corvp);
static	int	corv_map_find(CORV_BLK *corv_blkp, unchar bus, ushort target,
				unchar lun);

static	void	corv_chksusp(CORV_BLK *corv_blkp, CORV_CCB *ccbp,
				EXT_REPLY_ELE *ce_ptr);
static	void	corv_seterr(struct scsi_pkt *pktp, ELEMENT *ce_ptr);
static	void	corv_process_intr(CORV_BLK *corv_blkp, ushort ioaddr,
				unchar status);
static	void	corv_chkstatus(void *corv_blkp, ELE_HDR *deqp);
static	void	corv_read_immed(CORV_BLK *corv_blkp,
				READ_IMMEDIATE_ELE *ce_ptr);
static	void	corv_abort_immed(CORV_BLK *corv_blkp, unchar bus,
				ushort target);

static	void	build_eid_req(CORV_CCB *ccbp, EID_MGMT_REQ *ce_ptr,
				int func, unchar bus, ushort targ, unchar lun,
				unchar entity_id);
static	void	build_send_scsi_req(CORV_CCB *ccbp, unchar eid, int cdblen,
				int nodisco, int arq);
static	void	build_rw_req(CORV_CCB *ccbp, struct scsi_pkt *pktp, unchar eid,
				unchar opcode, int arq);
static	void	build_read_devcap_req(CORV_CCB *ccbp, struct scsi_pkt *pktp,
				unchar eid, int arq);
static	void	build_abort_req(CORV_CCB *ccbp, CORV_INFO *corvp,
				ABORT_SCSI_REQ *ep, unchar option, ulong param);
static	void	build_init_req(CORV_CCB *ccbp, CORV_INFO *corvp,
				INIT_SCSI_REQ *ep, int bus);
static	void	build_read_immediate_req(CORV_CCB *ccbp);
static	int	corv_delay(int secs);
void	corv_err(char *fmt, ... );


static	void	corv_doneq_add(CORV_BLK *corv_blkp, CORV_CCB *ccbp);
static	CORV_CCB *corv_doneq_rm(CORV_BLK *corv_blkp);

/*
 * Local static data
 */

static	int	corv_cb_id = 0;

static	kmutex_t corv_global_mutex;
static	int	corv_global_init = 0;




static	ddi_dma_lim_t corv_dma_lim = {
	0,              /* address low                          */
	0xffffffff,     /* address high                         */
	0,              /* counter max                          */
	1,              /* burstsize                            */
	DMA_UNIT_8,     /* minimum xfer                         */
	0,              /* dma speed                            */
	DMALIM_VER0,    /* version                              */
	0xffffffff,     /* address register                     */
	/*
	* NOTE: in order to avoid a bug in rootnex_io_brkup the following
	*	 is set to 2^32 - 2 rather than 2^32 - 1.
	*/
	0xfffffffe,     /* counter register                     */
	512,            /* sector size                          */
	MAX_SG_SEGS,	/* scatter/gather list length         	*/
	0xffffffff      /* request size                         */
};

static	int	corv_identify(dev_info_t *dev);
static	int	corv_probe(dev_info_t *);
static	int	corv_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static	int	corv_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

struct dev_ops  corv_ops = {
        DEVO_REV,               /* devo_rev, */
        0,                      /* refcnt  */
        ddi_no_info,            /* info */
        corv_identify,          /* identify */
        corv_probe,             /* probe */
        corv_attach,            /* attach */
        corv_detach,            /* detach */
        nulldev,       		/* reset */
        (struct cb_ops *)0,     /* driver operations */
        NULL		        /* bus operations */
};

/*
 *	move mode routines
 */

/* pipe building and initialisation */
static	int	corv_mm_docmd(CORV_BLK *corv_blkp, CORV_CCB *ccbp);
static	int	config_delivery_pipe(CORV_BLK *corv_blkp);
static	void	unconfig_delivery_pipe(CORV_BLK *corv_blkp);
static	void	init_pipe_desc(PIPEDS *pipep, MGMT_CFG_PIPES *pe,
				CTRLAREA *adp_sigp, CTRLAREA *sys_sigp,
				caddr_t ip, caddr_t op);

#ifdef  CORV_DEBUG
/*
ulong	corv_debug = -1 & ~DQUE;
ulong	corv_debug = DQDBG;
*/
ulong	corv_debug = DERR | DENQERR;

CTRLAREA	*cdbg_adp_sca;
CTRLAREA	*cdbg_sys_sca;
caddr_t		cdbg_ib_pipep;
caddr_t		cdbg_ob_pipep;

static	void	dump_pkt(struct scsi_pkt *pktp);
static	void	dump_cmd(struct scsi_cmd *cmdp);
static	void	dump_ele(ELEMENT *ce_ptr);
static	void	dump_tsb(ELEMENT *ce_ptr, CORV_TSB *tsbp);

#endif

#ifdef CORV_QDEBUG

#define	CDBG_MAX_QSLOTS	255

/* ptrs to outstanding CCBs index by (MCAslot, srcid) */
typedef struct {
	long		qcnt;
	CORV_CCB	*ccbp[CDBG_MAX_QSLOTS];
} cdbg_ccb_t;

/* one array per adapter card indexed by slot number */
cdbg_ccb_t	cdbg_qed_ccbs[8];

static	void	cdbg_qdebug(CORV_BLK *corv_blkp, CORV_CCB *ccbp,
				ELEMENT  *ce_ptr);
#endif

/*
 * This is the driver loadable module wrapper.
 */
char _depends_on[] = "misc/scsi";

extern struct mod_ops mod_driverops;

static	struct modldrv modldrv = {
        &mod_driverops, 	/* Type of module. This one is a driver */
        "Corvette SCSI HBA Driver",     /* Name of the module. */
        &corv_ops,      		/* driver ops */
};

static	struct modlinkage modlinkage = {
        MODREV_1, (void *)&modldrv, NULL
};

int
_init( void )
{
        int     status;

	if ((status = scsi_hba_init(&modlinkage)) != 0) {
		return (status);
	}

	mutex_init(&corv_global_mutex, "CORV global Mutex"
			, MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&corv_global_mutex);
	}
	return (status);
}

int
_fini( void )
{
        int     status;

        status = mod_remove(&modlinkage);
        if (!status) {
                mutex_destroy(&corv_global_mutex);
		scsi_hba_fini(&modlinkage);
	}
        return (status);
}

int
_info( struct modinfo *modinfop )
{
        return (mod_info(&modlinkage, modinfop));
}


/*ARGSUSED*/
static int
corv_tran_tgt_init(	dev_info_t		*hba_dip,
			dev_info_t		*tgt_dip,
			scsi_hba_tran_t		*hba_tran,
			struct scsi_device	*sd )
{
	CORV_INFO	*corvp;
        CORV_BLK	*corv_blkp = TRAN2CORVBLKP(hba_tran);
	ushort		 bus = TRAN2CORVBUSP(hba_tran)->sb_bus;
        ushort		 targ = sd->sd_address.a_target;
        unchar 		 lun = sd->sd_address.a_lun;

	cmn_err(CE_CONT, "?%s%d: %s%d <%d,%d,%d>\n"
		       , ddi_get_name(hba_dip), ddi_get_instance(hba_dip)
		       , ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip)
		       , bus, targ, lun);

	if (bus > 1 || targ > 15 || lun > 7) {
		cmn_err(CE_WARN, "%s%d: %s%d bad address <%d,%d,%d>"
			       , ddi_get_name(hba_dip)
			       , ddi_get_instance(hba_dip)
			       , ddi_get_name(tgt_dip)
			       , ddi_get_instance(tgt_dip), bus, targ, lun);
		return (DDI_FAILURE);
	}

	mutex_enter(&corv_blkp->cb_mutex);

	/* disallow hba's internal and external target numbers */
	if ((bus == CORV_INTBUS && targ == corv_blkp->cb_itargetid)
	||  (bus == CORV_EXTBUS && targ == corv_blkp->cb_xtargetid)) {
		mutex_exit(&corv_blkp->cb_mutex);
		return (DDI_FAILURE);
	}

	if (!(corvp = corv_map_target(corv_blkp, hba_tran, bus, targ, lun))) {
		mutex_exit(&corv_blkp->cb_mutex);
		return (DDI_FAILURE);
	}

	hba_tran->tran_tgt_private = corvp;
	mutex_exit(&corv_blkp->cb_mutex);

	DBG_DINIT(("corv_tran_tgt_init: <%d,%d>\n", targ, lun));
	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static void
corv_tran_tgt_free(	dev_info_t		*hba_dip,
			dev_info_t		*tgt_dip,
			scsi_hba_tran_t		*hba_tran,
			struct scsi_device	*sd )
{
	CORV_INFO	*corvp = TRAN2CORVINFO(hba_tran);
	CORV_BLK	*corv_blkp = TRAN2CORVBLKP(hba_tran);
#ifdef	CORVDEBUG
	ushort		 bus;
        ushort		 targ;
        unchar		 lun;

	bus = TRAN2CORVBUSP(hba_tran)->sb_bus;
	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

	cmn_err(CE_CONT, "corv_tran_tgt_free: %s%d %s%d <%d,%d,%d>\n"
		       , ddi_get_name(hba_dip), ddi_get_instance(hba_dip)
		       , ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip)
		       , bus, targ, lun);
#endif	/* CORVDEBUG */

	/* call corv_unmap_target() to free the target->EID mapping */
	mutex_enter(&corv_blkp->cb_mutex);
	hba_tran->tran_tgt_private = NULL;
	corv_unmap_target(corv_blkp, corvp);
	mutex_exit(&corv_blkp->cb_mutex);
	return;
}

/*ARGSUSED*/
static int
corv_tran_tgt_probe(	struct scsi_device	*sd,
			int			(*callback)(void *) )
{
	int	rval;
	char		*s;
	CORV_INFO	*corvp = SDEV2CORV(sd);

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
	cmn_err(CE_CONT, "?corv%d: %s target %d lun %d %s\n"
		       , ddi_get_instance(CORV_DIP(corvp))
		       , ddi_get_name(sd->sd_dev), sd->sd_address.a_target
		       , sd->sd_address.a_lun, s);

	return (rval);
}

/*
 *      Autoconfiguration routines
 */
static int
corv_identify( dev_info_t *devi )
{
        char *dname = ddi_get_name(devi);

        if (strcmp(dname, "corvette") == 0)
                return (DDI_IDENTIFIED);
        else
                return (DDI_NOT_IDENTIFIED);
}

static int
corv_probe( dev_info_t *devi )
{
        int     ioaddr;
        int     len;

#ifdef CORV_DEBUG
	debug_enter("\ncorv_probe\n");
#endif

        len = sizeof(int);
        if ((HBA_INTPROP(devi, "ioaddr", &ioaddr, &len) != DDI_SUCCESS)) {
		DBG_DERR(("?corv_probe: invalid ioaddr prop 0x%x\n", devi));
                return (DDI_PROBE_FAILURE);
	}

	if (!corv_exist(ioaddr, NULL)) {
		DBG_DINIT(("corv_probe: failed 0x%x\n", ioaddr));
                return (DDI_PROBE_FAILURE);
	}

        return (DDI_PROBE_SUCCESS);
}

struct corv_map {
	ushort		 cm_ioaddr;
	ushort		 cm_refcnt;
	CORV_BLK	*cm_blkp;
} corv_map[MAX_BRDS];

static
corv_uninitcard(	dev_info_t	*devi,
			scsi_hba_tran_t	*hba_tran,
			CORV_BUS	*sbp )
{
        CORV_BLK	*corv_blkp = sbp->sb_blkp;
	ushort 		 ioaddr = sbp->sb_ioaddr;
	int		 slot = corv_blkp->cb_mca_slot;


	if (corv_map[slot].cm_refcnt-- > 1) {
		/* still in use by the other bus */
		return;
	}

	/* reset the board and disable hardware interrupts */
	corv_disable(corv_blkp);

	/* delete the interrupt handler */
	ddi_remove_intr(devi, 0, corv_blkp->cb_iblock);

	mutex_destroy(&corv_blkp->cb_mutex);

	/* release the pipe buffers */
	unconfig_delivery_pipe(corv_blkp);

	/* clear the bus to board map */
	corv_map[slot].cm_ioaddr = 0;
	corv_map[slot].cm_blkp = NULL;;

	/* release the board soft state structure */
	kmem_free((caddr_t)corv_blkp, sizeof (*corv_blkp));

	return;
}

static int
corv_initcard(	dev_info_t	*devi,
		scsi_hba_tran_t	*hba_tran,
		CORV_BUS	*sbp )
{
        CORV_BLK	*corv_blkp;
	int		 slot;

	for (slot = 0; slot < MAX_BRDS; slot++) {
		if (corv_map[slot].cm_ioaddr != sbp->sb_ioaddr)
			continue;

		/* found match, the other bus attach was already here */
		corv_map[slot].cm_refcnt++;
		sbp->sb_blkp = corv_map[slot].cm_blkp;
		ASSERT(sbp->sb_blkp != NULL);
		return (TRUE);
	}

        corv_blkp = (CORV_BLK *)kmem_zalloc(sizeof (CORV_BLK), KM_NOSLEEP);

        if (!corv_blkp)
                return (FALSE);

	/* save the info needed by corv_cfginit(), et al */
        corv_blkp->cb_dip = devi;
	corv_blkp->cb_ioaddr = sbp->sb_ioaddr;
	sbp->sb_blkp = corv_blkp;

        if (!corv_cfginit(corv_blkp)) {
		DBG_DERR(("?corv_initcard: cfginit failed ioaddr=0x%x\n"
				, corv_blkp->cb_ioaddr));
		goto bailout1;
	}

	/* corv_cfginit() has configured this board and found its MCA slot */
	slot = corv_blkp->cb_mca_slot;

	/* sanity checks */
	ASSERT(corv_map[slot].cm_refcnt == 0);
	ASSERT(corv_map[slot].cm_blkp == NULL);

	/* store this board's info in the MCA slot map */
	corv_map[slot].cm_ioaddr = corv_blkp->cb_ioaddr;
	corv_map[slot].cm_refcnt = 1;
	corv_map[slot].cm_blkp = corv_blkp;

	/* print the POS info on the console */
	corv_getpos(corv_blkp);

	/* S H I F T	    T O 	M O V E 	M O D E 	*/
	if (!config_delivery_pipe(corv_blkp)) {
		DBG_DERR(("?corv_initcard: attach failed\n"));
		goto bailout2;
	}

	/*
	 *      Establish initial dummy interrupt handler
	 *      get iblock cookie to initialize mutexes used in the
	 *      real interrupt handler
	 */
	if (ddi_add_intr(devi, 0, (ddi_iblock_cookie_t *)&corv_blkp->cb_iblock
			     , (ddi_idevice_cookie_t *)0, corv_dummy_intr
			     , NULL)) {
                DBG_DERR(("?corv_initcard: cannot add intr\n"));
		goto bailout3;
        }

        mutex_init(&corv_blkp->cb_mutex, "corv mutex", MUTEX_DRIVER
				       , (void *)corv_blkp->cb_iblock);

        ddi_remove_intr(devi, 0, corv_blkp->cb_iblock);

  	/*   Establish real interrupt handler   */
        if (ddi_add_intr(devi, corv_blkp->cb_intrx
			     , (ddi_iblock_cookie_t *)&corv_blkp->cb_iblock
			     , (ddi_idevice_cookie_t *)0
			     , corvintr, (caddr_t)corv_blkp)) {
		DBG_DERR(("?corv_initcard: cannot add intr\n"));
		goto bailout4;
        }

	/* grab the mutex so no interrupts occur */
  	mutex_enter(&corv_blkp->cb_mutex);

	/* activate the board */
	if (!corv_enable(corv_blkp)) {
		DBG_DERR(("?corv_initcard: cannot enable board\n"));
		goto bailout5;
	}

	mutex_exit(&corv_blkp->cb_mutex);
	DBG_DINIT(("corv_initcard: ioaddr 0x%x corv_blkp 0x%x\n"
			, corv_blkp->cb_ioaddr, corv_blkp));
	return (TRUE);


bailout5:
	/* reset the board and disable hardware interrupts */
	corv_disable(corv_blkp);

	mutex_exit(&corv_blkp->cb_mutex);
	ddi_remove_intr(devi, 0, corv_blkp->cb_iblock);
bailout4:
	mutex_destroy(&corv_blkp->cb_mutex);
bailout3:
	unconfig_delivery_pipe(corv_blkp);
bailout2:
	corv_map[slot].cm_ioaddr = 0;
	corv_map[slot].cm_refcnt = 0;
	corv_map[slot].cm_blkp = NULL;;

bailout1:
	kmem_free((caddr_t)corv_blkp, sizeof (*corv_blkp));

	return (FALSE);
}

static void
corv_uninitbus(	dev_info_t	*devi,
		scsi_hba_tran_t	*hba_tran )
{
        CORV_BUS	*sbp = TRAN2CORVBUSP(hba_tran);

	corv_uninitcard(devi, hba_tran, sbp);
	kmem_free((caddr_t)TRAN2CORVBUSP(hba_tran), sizeof (CORV_BUS));
	hba_tran->tran_hba_private = NULL;
	return;
}

static int
corv_initbus(	dev_info_t	*devi,
		scsi_hba_tran_t	*hba_tran )
{
        CORV_BUS	*sbp;

        if (!(sbp = (CORV_BUS *)kmem_zalloc(sizeof (CORV_BUS), KM_NOSLEEP)))
                return (FALSE);

        if (!corv_propinit(devi, sbp)) {
		DBG_DERR(("corv_initbus: propinit failed devi 0x%x\n", devi));
		goto failed;
	}
	if (sbp->sb_bus > CORV_MAXBUS) {
		DBG_DERR(("corv_initbus: invalid bus number 0x%x\n"
				, sbp->sb_bus));
		goto failed;
	}
	if (!corv_initcard(devi, hba_tran, sbp)) {
		DBG_DERR(("corv_initbus: initcard failed devi 0x%x\n", devi));
		goto failed;
	}
	hba_tran->tran_hba_private = sbp;
	DBG_DINIT(("corv_initbus: ioaddr 0x%x bus 0x%x\n"
			, sbp->sb_blkp->cb_ioaddr, sbp->sb_bus));
	return (TRUE);

failed:
	kmem_free(sbp, sizeof (CORV_BUS));
	return (FALSE);
}

static int
corv_detach(	dev_info_t	*devi,
		ddi_detach_cmd_t cmd )
{
	scsi_hba_tran_t	*hba_tran;

        switch (cmd) {
	case DDI_DETACH:
		break;

	default:
		return (DDI_FAILURE);
	}

/* detaching HBA drivers is not supported in 2.4 all this is unnecessary */

	hba_tran = (scsi_hba_tran_t *) ddi_get_driver_private(devi);
	if (!hba_tran)
		return (DDI_SUCCESS);

	mutex_enter(&corv_global_mutex);

	/* free the bus structure and possibly the board structure */
	corv_uninitbus(devi, hba_tran);

	mutex_exit(&corv_global_mutex);

	scsi_hba_tran_free(hba_tran);

	ddi_prop_remove_all(devi);
	if (scsi_hba_detach(devi) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "corv: scsi_hba_detach failed");
	}

	return (DDI_SUCCESS);
}

static int
corv_attach(	dev_info_t	*devi,
		ddi_attach_cmd_t cmd )
{
	scsi_hba_tran_t		*hba_tran;

	switch (cmd) {
	case DDI_ATTACH:
		break;

	default:
		return (DDI_FAILURE);
	}

	/*
	 * Allocate a transport structure
	 */
	if ((hba_tran = scsi_hba_tran_alloc(devi, 0)) == NULL) {
		cmn_err(CE_WARN, "corv_attach: scsi_hba_tran_alloc failed");
		return (DDI_FAILURE);
	}

	hba_tran->tran_tgt_private	= NULL;
	hba_tran->tran_tgt_init		= corv_tran_tgt_init;
	hba_tran->tran_tgt_probe	= corv_tran_tgt_probe;
	hba_tran->tran_tgt_free		= corv_tran_tgt_free;

	hba_tran->tran_start 		= corv_transport;
	hba_tran->tran_abort		= corv_abort;
	hba_tran->tran_reset		= corv_reset;
	hba_tran->tran_getcap		= corv_getcap;
	hba_tran->tran_setcap		= corv_setcap;
	hba_tran->tran_init_pkt 	= corv_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= corv_tran_destroy_pkt;
	hba_tran->tran_dmafree		= corv_tran_dmafree;
	hba_tran->tran_sync_pkt		= corv_tran_sync_pkt;


        mutex_enter(&corv_global_mutex);    /* protect multithreaded attach */

	if (scsi_hba_attach(devi, &corv_dma_lim, hba_tran, SCSI_HBA_TRAN_CLONE
				, NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "corv_attach: scsi_hba_attach failed");
		goto bailout;
	}

	if (!corv_initbus(devi, hba_tran))
		goto bailout;

        mutex_exit(&corv_global_mutex);
        ddi_report_dev(devi);
	return (DDI_SUCCESS);


bailout:
	scsi_hba_tran_free(hba_tran);
        mutex_exit(&corv_global_mutex);
	return (DDI_FAILURE);
}

static int
corv_propinit(	dev_info_t	*devi,
		CORV_BUS	*sbp )
{
	int	 val;
	int	 len;
	int	*pp;

	len = sizeof(int);

	if (HBA_INTPROP(devi, "ioaddr", &val, &len) != DDI_PROP_SUCCESS)
		return (FALSE);
	sbp->sb_ioaddr = val;

	if (ddi_getlongprop(DDI_DEV_T_NONE, devi, DDI_PROP_DONTPASS
					  , "reg", (caddr_t)&pp, &len)
	!= DDI_PROP_SUCCESS) {
		DBG_DERR(("corv_propinit: invalid reg property, devi 0x%x\n"
				, devi));
		return (FALSE);
	}

	if (len < sizeof (int)) {
		kmem_free((caddr_t)pp, len);
		return (FALSE);
	}

	sbp->sb_bus = CORV_BUS_NUM(pp[0]);
	kmem_free((caddr_t)pp, len);
	return (TRUE);
}


static void
corv_disable( CORV_BLK *corv_blkp )
{
	ushort	ioaddr = corv_blkp->cb_ioaddr;

	/* hardware reset - wait for at least 50ms and then
	 * turn off controller reset
	 */
        outb(ioaddr + CORV_BCR, BCR_SYS_RESET);
	drv_usecwait(50);
        outb(ioaddr + CORV_BCR, 0);
	return;
}

static int
corv_enable( CORV_BLK *corv_blkp )
{
	CORV_CCB	*ccbp;
	GEN_REPLY_ELE	*replyp;

	/* setup a Read Immediate so HBA will notify me about failures */
	ccbp = corv_ccballoc(corv_blkp, NULL);

	if (!ccbp)
		return (FALSE);

	build_read_immediate_req(ccbp);

	if (corv_mm_docmd(corv_blkp, ccbp) != DS_SUCCESS) {
		DBG_DERR(("?corv_hbainit : read_immd_req failed \n"));
		corv_ccbfree(corv_blkp, ccbp);
		return (FALSE);
	}

	/* wait for return status */
	if (!corv_mm_pollret(corv_blkp, ccbp)) {
		DBG_DERR(("?corv_enable: pollret failed\n"));
		return (FALSE);
	}

	replyp = (GEN_REPLY_ELE *)&ccbp->ccb_ele;

	if (replyp->gen_ctl.eid != REPLY) {
		DBG_DERR(("?corv_enable: failed\n"));
#ifdef CORV_DEBUG
		dump_ele((ELEMENT *)replyp);
#endif
		corv_ccbfree(corv_blkp, ccbp);
		return (FALSE);
	}
	corv_ccbfree(corv_blkp, ccbp);


	/* enable dma
	 * enable interrupts
	 * set the "clear on read" bit for move mode
	 */
        outb(corv_blkp->cb_ioaddr + CORV_BCR
		, BCR_DMA_ENABLE | BCR_INTR_ENABLE | BCR_CLR_ON_READ);
	return (TRUE);
}



static int
corv_transport(	struct scsi_address	*ap,
		struct scsi_pkt		*pktp )
{
	CORV_INFO	*corvp = PKT2CORVINFO(pktp);
        CORV_BLK	*corv_blkp = corvp->c_blkp;
	CORV_UNIT	*corv_unitp = corvp->c_unitp;
        GEN_REQ_ELE 	*ep;
	CORV_CCB	*ccbp;
	unchar		 entity_id;
	PIPEDS		*pipep;


	/*
	 * get the element back from the pkt which is initialized in
	 * pktalloc routine
	 */

        ccbp = (CORV_CCB *)SCMD_PKTP(pktp)->cmd_private;
	entity_id = corvp->c_entity_id;
	ep = (GEN_REQ_ELE *)&ccbp->ccb_ele;

	/* clear all flags except the direction flag */
	ep->flag1 &= DIR;

	pipep = (PIPEDS *)&(corv_blkp->cb_pipep);

  	mutex_enter(&corv_blkp->cb_mutex);

	/* xlate SCSI command to IBM vendor specific request element */
        switch (*(pktp->pkt_cdbp)) {
#ifdef notyet /* ??? check reladdr and cache flags in second byte of CDB ??? */
	case SCMD_READ_G1:
		build_rw_req(ccbp, pktp, entity_id, READ_LIST
				 , corv_unitp->cu_arq);
		break;

	case SCMD_WRITE_G1:
		build_rw_req(ccbp, pktp, entity_id, WRITE_LIST
				 , corv_unitp->cu_arq);
		break;

	case SCMD_READ_CAPACITY:
		build_read_devcap_req(ccbp, pktp, entity_id
					  , corv_unitp->cu_arq);
		break;
#endif

	default:
		DBG_DPKT(("corv_transport(%d,%d): send scsi command = 0x%x\n"
				, ap->a_target, ap->a_lun, *(pktp->pkt_cdbp)));
		build_send_scsi_req(ccbp, entity_id
					, SCMD_PKTP(pktp)->cmd_cdblen
					, (pktp->pkt_flags & FLAG_NODISCON)
					, corv_unitp->cu_arq);
		break;
	}

	if (corv_unitp->cu_suspended) {
		DBG_DERR2(("?corv_transport(%d,%d,%d): clearing suspended"
			   " state corvp=0x%x\n"
				, corv_unitp->cu_eidreq.bus
				, corv_unitp->cu_eidreq.targ
				, corv_unitp->cu_eidreq.lunn_1, corvp));
		corv_unitp->cu_suspended = FALSE;
		ep->flag1 |= REACT_Q;
	}

	/************************************************************/
	/* Due to a bug in the Corvette microcode, must set these
	 * bits on every request. Otherwise, all the adapters queues
	 * get suspended after some sort of error condition and 
	 * everything grinds to a halt.
	 */
	ep->flag1 |= (UNTAG_Q|REACT_Q);
	ep->gen_ctl.expedite = 1;
	/************************************************************/

	if (corv_mm_docmd(corv_blkp, ccbp) != DS_SUCCESS) {
		DBG_DERR2(("?corv_transport: do cmd fail :1\n"));
  		mutex_exit(&corv_blkp->cb_mutex);
    		return TRAN_BUSY;
	}

	DBG_DPKT(("corv_transport: after enqueueing\n"));

	if (pktp->pkt_flags & FLAG_NOINTR) {
		if (!corv_mm_pollret(corv_blkp, ccbp)) {
			DBG_DERR2(("?corv_transport: EXIT 1\n"));
			pktp->pkt_reason = CMD_TRAN_ERR;
  			mutex_exit(&corv_blkp->cb_mutex);
      			return TRAN_BUSY;
		}
		/* else */
		mutex_exit(&corv_blkp->cb_mutex);
		return TRAN_ACCEPT;
   	}


	DBG_DPKT(("corv_transport: (EXIT) target = %d lun = %d,"
		  " pktp = 0x%x\n", pktp->pkt_address.a_target
			, pktp->pkt_address.a_lun, pktp));

	mutex_exit(&corv_blkp->cb_mutex);
	return TRAN_ACCEPT;
}


static int
corv_capchk(	char	*cap,
		int	 tgtonly,
		int	*cidxp )
{
        int    cidx;

        if ((tgtonly != 0 && tgtonly != 1) || cap == (char *) 0)
                return (FALSE);

	*cidxp = scsi_hba_lookup_capstr(cap);
        return (TRUE);
}

static int
corv_getcap(	struct scsi_address	*ap,
		char			*cap,
		int			 tgtonly )
{
        int     ckey;
        int     status;
	ulong	totalsectors;
	ulong	heads;
	ulong	sectors;

        if ((status = corv_capchk(cap, tgtonly, &ckey)) != TRUE)
                return (UNDEFINED);

	switch (ckey) {
	case SCSI_CAP_GEOMETRY:
		totalsectors = ADDR2CORVUNITP(ap)->cu_tot_sects;

		if (totalsectors <= 64 * 32 * 1024) {
			/* less than or equal to about 1 Gigabyte */
			heads = 64;
			sectors = 32;

		} else if (totalsectors <= 128 * 63 * 1024) {
			/* less than or equal to about 3.9 Gigabytes */
			heads = 128;
			sectors = 63;

		} else  {
			heads = 254;
			sectors = 63;
		}
		return (HBA_SETGEOM(heads, sectors));

	case SCSI_CAP_ARQ:
		if (tgtonly)
			return(ADDR2CORVUNITP(ap)->cu_arq);
		else
			return (UNDEFINED);

	case SCSI_CAP_TAGGED_QING:
		if (tgtonly)
			return(ADDR2CORVUNITP(ap)->cu_tagque);
		else
			return (UNDEFINED);

	default:
		return (UNDEFINED);
	}
}

static int
corv_setcap(	struct scsi_address	*ap,
		char			*cap,
		int			 value,
		int			 tgtonly )
{
        int     ckey;
        int     status;

        if ((status = corv_capchk(cap, tgtonly, &ckey)) != TRUE)
                return (status);

	switch (ckey) {
	case SCSI_CAP_ARQ:
		if (tgtonly) {
			ADDR2CORVUNITP(ap)->cu_arq = (u_int)value;
			return(TRUE);
		} else {
			return (FALSE);
		}

	case SCSI_CAP_SECTOR_SIZE:
		(ADDR2CORVUNITP(ap))->cu_dmalim.dlim_granular = (u_int)value;
		return (TRUE);

	case SCSI_CAP_TOTAL_SECTORS:
		ADDR2CORVUNITP(ap)->cu_tot_sects = (ulong)value;
		return (TRUE);

	case SCSI_CAP_GEOMETRY:
		return (TRUE);

	default:
		return (UNDEFINED);
	}
}

static struct scsi_pkt *
corv_tran_init_pkt(	struct scsi_address	*ap,
			struct scsi_pkt		*pkt,
			struct buf		*bp,
			int			 cmdlen,
			int			 statuslen,
			int			 tgtlen,
			int			 flags,
			int			(*callback)(),
			caddr_t			 arg )
{
	struct scsi_pkt		*new_pkt = NULL;

	/*
	 * Allocate a pkt
	 */
	if (!pkt) {
		pkt = corv_pktalloc(ap, cmdlen, statuslen
				      , tgtlen, callback, arg);
		if (pkt == NULL)
			return (NULL);
		SCMD_PKTP(pkt)->cmd_flags = flags;
		new_pkt = pkt;
	} else {
		new_pkt = NULL;
	}

	/*
	 * Set up dma info
	 */
	if (bp) {
		if (!corv_dmaget(pkt, (opaque_t) bp, callback, arg)) {
			if (new_pkt)
				corv_pktfree(ap, new_pkt);
			return (NULL);
		}
	}

	return (pkt);
}

static void
corv_tran_destroy_pkt(	struct scsi_address	*ap,
			struct scsi_pkt		*pkt )
{
	corv_tran_dmafree(ap, pkt);
	corv_pktfree(ap, pkt);
}


static void
corv_ccbfree(	CORV_BLK	*corv_blkp,
		CORV_CCB	*ccbp )
{
	ddi_iopb_free((caddr_t)ccbp);
	return;
}

/*
 * corv_ccballoc()
 */

static CORV_CCB *
corv_ccballoc(	CORV_BLK	*corv_blkp,
		CORV_INFO	*corvp )
{
	CORV_CCB  	 *ccbp;

	/* allocate ccb	*/
	if (ddi_iopb_alloc(corv_blkp->cb_dip, NULL, sizeof (*ccbp)
					    , (caddr_t *)&ccbp) != DDI_SUCCESS) {
		DBG_DERR2(("?corv_ccballoc: failed\n"));
		return (NULL);
	}
        ccbp->ccb_paddr  = CORV_KVTOP(ccbp);
	ccbp->ccb_corvp = corvp;
	return (ccbp);
}


struct scsi_pkt *
corv_pktalloc(	struct scsi_address	*ap,
		int			 cmdlen,
		int			 statuslen,
		int			 tgtlen,
		int			(*callback)(),
		caddr_t			 arg)
{
	CORV_INFO	*corvp = TRAN2CORVINFO(ADDR2TRAN(ap));
        CORV_BLK	*corv_blkp = ADDR2CORVBLKP(ap);
	struct scsi_cmd	*cmd;
	CORV_CCB  	*ccbp;
        int     	 kf;
	SEND_SCSI_REQ	*ce_ptr;
	caddr_t		 tgt;

        kf = HBA_KMFLAG(callback);

	/*
	 * Allocate target-private data, if necessary
	 */
	if (tgtlen > PKT_PRIV_LEN) {
		tgt = kmem_zalloc(tgtlen, kf);
		if (!tgt) {
			ASSERT(callback != SLEEP_FUNC);
			if (callback != NULL_FUNC)
				ddi_set_callback(callback, arg, &corv_cb_id);
			return (NULL);
		}
	} else {
		tgt = NULL;
	}


        cmd = (struct scsi_cmd *)kmem_zalloc(sizeof (*cmd), kf);
	if (!cmd)
		DBG_DERR2(("?corv_pktalloc: cmd alloc failed\n"));

	if (cmd) {
		if (!(ccbp = corv_ccballoc(corv_blkp, corvp))) {
			kmem_free((void *)cmd, sizeof (*cmd));
			cmd = NULL;
		}
	}

        if (!cmd) {
		if (tgt)
			kmem_free(tgt, tgtlen);
		ASSERT(callback != SLEEP_FUNC);
                if (callback != NULL_FUNC)
                        ddi_set_callback(callback, arg, &corv_cb_id);
                return (NULL);
        }

	/* prepare the packet for normal command */
	ccbp->ccb_ownerp	 = cmd;
        cmd->cmd_private         = (opaque_t)ccbp;
        cmd->cmd_pkt.pkt_cdbp    = (opaque_t)ccbp->ccb_cdb;
        cmd->cmd_cdblen          = (u_char) cmdlen;
	cmd->cmd_pkt.pkt_scbp	 = (unchar *)&ccbp->ccb_sense;
        cmd->cmd_scblen          = (u_char)statuslen;
	cmd->cmd_pkt.pkt_address = *ap;

	/*
	 * Set up target-private data
	 */
	cmd->cmd_privlen = tgtlen;
	if (tgtlen > PKT_PRIV_LEN) {
		cmd->cmd_pkt.pkt_private = tgt;
	} else if (tgtlen > 0) {
		cmd->cmd_pkt.pkt_private = cmd->cmd_pkt_private;
	}

	return(&cmd->cmd_pkt);
}

/*
 * Free memory associated with scsi_pkt except DMA resources
 */
static void
corv_pktfree(	struct scsi_address	*ap,
		struct scsi_pkt		*pktp )
{
        struct scsi_cmd *cmd = SCMD_PKTP(pktp);

	if (cmd->cmd_privlen > PKT_PRIV_LEN) {
		kmem_free(pktp->pkt_private, cmd->cmd_privlen);
	}

        ASSERT(!(cmd->cmd_flags & CFLAG_FREE));


	/* deallocate the ccb */
        if (cmd->cmd_private)
                ddi_iopb_free((caddr_t)cmd->cmd_private);

	/* free the common packet */
        kmem_free((caddr_t)cmd, sizeof (*cmd));


        if (corv_cb_id)
                ddi_run_callback(&corv_cb_id);
}

/*
 * Free DMA resources associated with scsi_pkt.
 * Memory for pkt itself is freed in corv_pktfree.
 */

static void
corv_tran_dmafree(	struct scsi_address	*ap,
			struct scsi_pkt		*pktp )
{
        struct scsi_cmd *cmd = SCMD_PKTP(pktp);

	/* Free the mapping.*/
        if (cmd->cmd_dmahandle)
                ddi_dma_free(cmd->cmd_dmahandle);
}

struct scsi_pkt *
corv_dmaget(	struct scsi_pkt	*pktp,
		opaque_t	 dmatoken,
		int		(*callback)(),
		caddr_t		 arg )
{
        struct buf		*bp = (struct buf *) dmatoken;
        struct scsi_cmd 	*cmdp = SCMD_PKTP(pktp);
        CORV_CCB 		*ccbp;
        SG_SEGMENT 		*dmap;
        ddi_dma_cookie_t	 dmack;
        ddi_dma_cookie_t	*dmackp	= &dmack;
        int     		 cnt;
        int     		 bxfer;
        off_t   		 offset;
        off_t   		 len;

	SEND_SCSI_REQ	*ce_ptr;
	ccbp = (CORV_CCB *)cmdp->cmd_private;
	ce_ptr = (SEND_SCSI_REQ *)&ccbp->ccb_ele;


	if (!bp->b_bcount) {
		pktp->pkt_resid = 0;
		ce_ptr->gen_req.flag1 &= ~DIR;
		ccbp->ccb_blk_cnt = 0;
                return (pktp);
        }

	/* check for direction for data transfer */
        if (bp->b_flags & B_READ) {
                ce_ptr->gen_req.flag1 = DIR;
		cmdp->cmd_cflags &= ~CFLAG_DMASEND;
	} else {
		/* in case packets are reused */
                ce_ptr->gen_req.flag1 = 0;
		cmdp->cmd_cflags |= CFLAG_DMASEND;
	}

	ccbp->ccb_baddr = bp->b_un.b_addr;

	if (!scsi_impl_dmaget(pktp, (opaque_t)bp, callback, arg
				  , &(PKT2CORVUNITP(pktp)->cu_dmalim))){
                return (NULL);
	}

	ddi_dma_segtocookie(cmdp->cmd_dmaseg, &offset, &len, dmackp);


	dmap = ccbp->ccb_sg_list;

	for (bxfer = 0, cnt = 1; ; cnt++, dmap++) {
		bxfer += dmackp->dmac_size;

		dmap->data_len = (ulong) dmackp->dmac_size;
		dmap->data_ptr = (ulong) dmackp->dmac_address;

		/* check for end of list condition */
		if (bp->b_bcount <= bxfer + cmdp->cmd_totxfer) {
			break;
		}

		/*
		 * check end of physical scatter-gather list limit
		 */
		if (cnt >= MAX_SG_SEGS) {
			break;
		}

		if (bxfer >= (PKT2CORVUNITP(pktp)->cu_dmalim.dlim_reqsize)) {
			break;
		}

		if (ddi_dma_nextseg(cmdp->cmd_dmawin, cmdp->cmd_dmaseg
						    , &cmdp->cmd_dmaseg)
		!= DDI_SUCCESS)
			break;

		ddi_dma_segtocookie(cmdp->cmd_dmaseg, &offset
						    , &len, dmackp);
	}

	ccbp->ccb_blk_cnt = cnt;

	cmdp->cmd_totxfer += bxfer;
	pktp->pkt_resid = bp->b_bcount - cmdp->cmd_totxfer;
        return (pktp);
}

/*ARGSUSED*/
static void
corv_tran_sync_pkt(	struct scsi_address	*ap,
			struct scsi_pkt		*pktp )
{
	struct scsi_cmd	*cmd = SCMD_PKTP(pktp);
	u_int		 type;

	if (!cmd->cmd_dmahandle)
		return;

	type = (cmd->cmd_cflags & CFLAG_DMASEND) ? DDI_DMA_SYNC_FORDEV
						 : DDI_DMA_SYNC_FORCPU;

	if (ddi_dma_sync(cmd->cmd_dmahandle, 0, 0, type) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "corv: sync pkt failed");
	}
	return;
}



/**************************** Adapter Dependent Layer *******************/

/*
 *      Adapter detection routine
 */
corv_hba_reset( u_int ioaddr )
{
        int	 i;
        char	 status;
        char	*sp = &status;

	/* hardware reset - wait for at least 50ms and then
	 * turn off controller reset
	 */
        outb(ioaddr + CORV_BCR, BCR_SYS_RESET);
	drv_usecwait(50);
        outb(ioaddr + CORV_BCR, 0);

	/*      check busy in status reg   */
        if (CORV_BUSYWAIT(ioaddr, NULL)) {
		DBG_DINIT(("corv_hba_reset: fail busywait\n"));
        	return (FALSE);
        }

	/*      wait for interrupt  */
        if (CORV_QINTRWAIT(ioaddr, NULL)) {
		DBG_DINIT(("corv_hba_reset: fail intr wait\n"));
        	return (FALSE);
        }

        status = inb(ioaddr + CORV_ISR);

        CORV_SENDEOI(ioaddr, HBA_DEVICE);

        if (CORV_intrp(sp)->ri_code)  {
		DBG_DINIT(("corv_hba_reset: fail eoi code\n"));
        	return (FALSE);
        }

	/*  enable on board dma controller but keep interrupts off  */
        outb(ioaddr + CORV_BCR, BCR_DMA_ENABLE);

        status = inb(ioaddr + CORV_BCR);
        if (!CORV_bcrp(sp)->rc_edma) {
		DBG_DINIT(("corv_hba_reset: fail dma enable\n"));
/* ??? what about the model 95 ??? */
                /* return (FALSE); for model 95 */
        }

	DBG_DINIT(("corv_hba_reset: found at 0x%x\n", ioaddr));
        return(TRUE);
}


/*
 * Check POS ID, initialize
 *
 */

static int
corv_cfginit( CORV_BLK *corv_blkp )
{
	unchar	scsi_id;


	/* map the (MCA device id, ioaddr) tuple into the slot number */
	if (!corv_exist(corv_blkp->cb_ioaddr, &corv_blkp->cb_mca_slot)) {
		return (FALSE);
	}

	/* try to reset the board */
	if (!corv_hba_reset(corv_blkp->cb_ioaddr))
		return (FALSE);

	/* determine which IRQ this adapter is configured to use */
	if (mca_getb(corv_blkp->cb_mca_slot, 0x104) & 0x01)
		corv_blkp->cb_irq = 11;
	else
		corv_blkp->cb_irq = 14;


	if (!corv_xlate_irq(corv_blkp))
		return (FALSE);

	/* rearrange the bits to get the 4 bit (wide) internal target id */
	scsi_id = mca_getb(corv_blkp->cb_mca_slot, 0x103);
	corv_blkp->cb_itargetid = (scsi_id >> 5)
				| ((scsi_id & 0x10) << 3);

	/* get the 4 bit (wide) external target id */
	scsi_id = mca_getb(corv_blkp->cb_mca_slot, 0x104);
	corv_blkp->cb_xtargetid = (scsi_id >> 3) & 0x0f;

	DBG_DINIT(("corv_cfginit: ioaddr 0x%x HBAID (%d,%d)\n"
			, corv_blkp->cb_ioaddr, corv_blkp->cb_itargetid
			, corv_blkp->cb_xtargetid));

	return (TRUE);
}

/*
 * Take the IRQ number and find a match in the interrupts property
 */

corv_xlate_irq( CORV_BLK *corv_blkp )
{
	dev_info_t 	*devi = corv_blkp->cb_dip;
	int		 len;
	int		 nintrs;
	int		 rc;
	int		 index;
	struct intrprop {
		int	spl;
		int	irq;
	} *intrprop;

	rc = ddi_getlongprop(DDI_DEV_T_ANY, devi, DDI_PROP_DONTPASS
					 , "interrupts", (caddr_t)&intrprop
					 , &len);

	if (rc != DDI_PROP_SUCCESS) {
		cmn_err(CE_WARN
			, "corv_xlate_irq: invalid interrrupts property");
		return (FALSE);
	}

	nintrs = len / sizeof (struct intrprop);

	/* find the tuple with the matching IRQ value, return its index */
	for (index = 0; index < nintrs; index++) {
		if (corv_blkp->cb_irq == intrprop[index].irq) {
			corv_blkp->cb_intrx = index;
			kmem_free(intrprop, len);
			return (TRUE);
		}
	}
	kmem_free(intrprop, len);
	return (FALSE);
}


static void
corv_getpos( CORV_BLK *corv_blkp )
{
	CORV_POS_REQ	*posp;
	CORV_POS	*resp;
	int		 rc;

	rc = ddi_iopb_alloc(corv_blkp->cb_dip, (ddi_dma_lim_t *)0,
			    sizeof(*posp), (caddr_t *)&posp);

	if (rc != DDI_SUCCESS) {
		DBG_DINIT(("corv_getpos: unable to allocate memory\n"));
		return;
	}

	rc = ddi_iopb_alloc(corv_blkp->cb_dip, (ddi_dma_lim_t *)0,
			    sizeof(*resp) + 1, (caddr_t *)&resp);

	if (rc != DDI_SUCCESS) {
		DBG_DINIT(("corv_getpos: unable to allocate memory2\n"));
		return;
	}

	bzero((caddr_t)posp, sizeof(*posp));
	bzero((caddr_t)resp, sizeof(*resp) + 1);

	posp->pos_opcode = ADP_CMD_GETPOS;
	posp->pos_cmd_sup = sizeof(*posp);

	/* read, no TSB, suppress short, bypass buffer */
	posp->pos_options = 0xc600;

	posp->pos_datap = CORV_KVTOP(resp);
	posp->pos_data_len = sizeof(*resp);

	if (!corv_docmd(corv_blkp, (int)(posp), HBA_DEVICE, START_SCB_CMD)) {
                DBG_DINIT(("corv_getpos: docmd failed\n"));
		return;
	}

	if (!corv_pollret(corv_blkp)) {
                DBG_DINIT(("corv_getpos: pollret failed\n"));
		return;
	}

	cmn_err(CE_CONT, "?corvette: slot %d rev 0x%x, R4B 0x%x\n",
			corv_blkp->cb_mca_slot + 1, resp->p_rev, resp->p_4B);

#ifndef	CORV_NO_REV_WARNING

	if (resp->p_rev >= 0x71)
		return;

cmn_err(CE_WARN, "corvette, slot %d: This SCSI-2 Fast/Wide Adapter/A board"
	       , corv_blkp->cb_mca_slot + 1);
cmn_err(CE_CONT, "\thas microcode version 0x%x.  Microcode versions prior\n"
	       , resp->p_rev);
cmn_err(CE_CONT, "\tto version 0x71 may somtimes cause the system to hang.\n");

#endif
	return;
}


/*
 *      adapter command interface routine
 */

static int
corv_docmd(	CORV_BLK	*corv_blkp,
		int		 cmd,
		unchar		 dev,
		unchar		 opcode )
{
        ushort ioaddr;
        int    i;
        paddr_t outcmd;


        ioaddr = corv_blkp->cb_ioaddr;
        outcmd = cmd;
        if (opcode != START_IMMEDIATE_CMD)
                outcmd = CORV_KVTOP(outcmd);

	/* check busy in status reg */
        if (CORV_CMDOUTWAIT(ioaddr, NULL)) {
		DBG_DERR(("?corv_docmd: board BUSY\n"));
        	return (FALSE);
        }

	outb(ioaddr + CORV_CMD + 0, (outcmd & 0xff));
	outcmd >>= 8;
	outb(ioaddr + CORV_CMD + 1, (outcmd & 0xff));
	outcmd >>= 8;
	outb(ioaddr + CORV_CMD + 2, (outcmd & 0xff));
	outcmd >>= 8;
	outb(ioaddr + CORV_CMD + 3, (outcmd & 0xff));

        outb(ioaddr + CORV_ATTN, opcode | dev);

        return (TRUE);
}




/*
 * adapter ready wait routine
 *
 * returns:	0 on success
 *		1 on failure
 */
corv_wait(	ushort	 ioaddr,
		long	 waittime,
		unchar	*bsr_valp,
		unchar	 mask,
		unchar	 onbits,
		unchar	 offbits )
{
        ushort	port;
        unchar	maskval;
	unchar	val;

	/* ask IBM about wait time - we hold a mutex */
        port = ioaddr + CORV_BSR;

        while (waittime > 0) {
                val = inb(port);
                maskval = val & mask;
                if ((maskval & onbits) == onbits
		&&  (maskval & offbits)== 0) {
			if (bsr_valp)
				*bsr_valp = val;
                        return(0);
		}
                drv_usecwait(10);
		waittime -= 10;
        }
        return(1);
}

/*
 * This is the pollret() function to use after sending a
 * locate mode command.
 */

static int
corv_pollret( CORV_BLK *corv_blkp )
{
        ushort	ioaddr;
        char    status;
        char    *sp = &status;

        ioaddr = corv_blkp->cb_ioaddr;

	/* wait for interrupt */
        if (CORV_INTRWAIT(ioaddr, NULL))
                return (FALSE);

        status = inb(ioaddr + CORV_ISR);
        corv_blkp->cb_intr_code = CORV_intrp(sp)->ri_code;
        corv_blkp->cb_intr_dev  = CORV_intrp(sp)->ri_ldevid;

	status = inb(ioaddr + CORV_BSR);
        corv_blkp->cb_exp_cond = CORV_bsrp(sp)->rs_exp_cond;
        corv_blkp->cb_exp_sts  = CORV_bsrp(sp)->rs_exp_sts;

	CORV_SENDEOI(ioaddr, corv_blkp->cb_intr_dev);

        if (CORV_QBUSYWAIT(ioaddr, NULL))
                return (FALSE);

  	return (TRUE);
}


static int
corv_mm_pollret(	CORV_BLK	*corv_blkp,
			CORV_CCB	*poll_ccbp )
{
	CORV_CCB	 *ccbp;
	CORV_CCB	 *ccb_headp = NULL;
	CORV_CCB	**ccb_tailpp = &ccb_headp;
	int		  got_it = FALSE;
	ushort		  ioaddr = corv_blkp->cb_ioaddr;
	struct scsi_pkt	 *pktp;
	unchar		  status;


	/* unqueue and save all ccbs until I find the right one */
	while (!got_it) {
		/* wait for interrupt */
		if (CORV_INTRWAIT(ioaddr, &status)) {
			DBG_DERR(("?pollret: CORV_INTRWAIT ioaddr=0x%x\n"
					, ioaddr));
			break;
		}

		/* dequeue any completed requests or errors */
		corv_process_intr(corv_blkp, ioaddr, status);

		/* unqueue all the completed requests, look for mine */
		while (ccbp = corv_doneq_rm(corv_blkp)) {
			/* if it's my ccb, requeue the rest then return */
			if (ccbp == poll_ccbp) {
				got_it = TRUE;
				continue;
			}
			/* fifo queue the other ccbs on my local list */
			*ccb_tailpp = ccbp;
			ccb_tailpp = &ccbp->ccb_linkp;
		}
	}


	/* check for other completed packets that have been queued */
	if (ccb_headp) {
		mutex_exit(&corv_blkp->cb_mutex);
		while (ccbp = ccb_headp) {
			ccb_headp = ccb_headp->ccb_linkp;
			if (!ccbp->ccb_ownerp) {
				DBG_DERR(("?corv_mm_pollret: missing pktp\n"));
				continue;
			}
			pktp = CCBP2PKTP(ccbp);
			(pktp->pkt_comp)(pktp);
		}
		mutex_enter(&corv_blkp->cb_mutex);
	}

	if (got_it)
		return (TRUE);

	if (poll_ccbp->ccb_ownerp) {
		pktp = CCBP2PKTP(poll_ccbp);
		pktp->pkt_reason = CMD_INCOMPLETE;
		pktp->pkt_state = 0;
	}
	return (FALSE);
}


/* Autovector Interrupt Entry Point */
/* Dummy return to be used before mutexes has been initialized          */
/* guard against interrupts from drivers sharing the same irq line      */

static u_int
corv_dummy_intr( caddr_t arg )
{
        return (DDI_INTR_UNCLAIMED);
}

/*  Autovector Interrupt Entry Point    */
static u_int
corvintr( caddr_t arg )
{
	CORV_BLK	*corv_blkp = (CORV_BLK *)arg;
	CORV_CCB	*ccbp;
	ushort		 ioaddr;
	struct scsi_pkt *pktp;
	char		 status;

	ioaddr = corv_blkp->cb_ioaddr;

  	mutex_enter(&corv_blkp->cb_mutex);

	status = inb(ioaddr + CORV_BSR);
        if (!(status & (BSR_INTR_REQ | BSR_EXCPN_CONDN))) {
                mutex_exit(&corv_blkp->cb_mutex);
                return (DDI_INTR_UNCLAIMED);
        }


	do {
		/* dequeue any completed requests or errors */
		corv_process_intr(corv_blkp, ioaddr, status);

		/* run the completion routines of all the completed commands */
		while ((ccbp = corv_doneq_rm(corv_blkp)) != NULL) {
			mutex_exit(&corv_blkp->cb_mutex);
			pktp = CCBP2PKTP(ccbp);
#ifdef CORV_DEBUG
			if (!pktp || !pktp->pkt_comp)  {
				DBG_DERR(("?corvintr: ccbp 0x%x pktp 0x%x\n"
					, ccbp, pktp));
			}
#else
			ASSERT(pktp);
			ASSERT(pktp->pkt_comp);
#endif
			(*pktp->pkt_comp)(pktp);
			mutex_enter(&corv_blkp->cb_mutex);
		}
		status = inb(ioaddr + CORV_BSR);

	} while (status & (BSR_INTR_REQ | BSR_EXCPN_CONDN));

	mutex_exit(&corv_blkp->cb_mutex);
        return DDI_INTR_CLAIMED;
}

static void
corv_process_intr(	CORV_BLK	*corv_blkp,
			ushort		 ioaddr,
			unchar		 status )
{

	DBG_DINTR(("corv_process_intr: ioaddr 0x%x stat 0x%x\n"
			, ioaddr, status));

	if (status & BSR_EXCPN_CONDN) {
		unchar	bcr;

		/* clear the exception code from the bsr register */
		bcr = inb(ioaddr + CORV_BCR);
		outb(ioaddr + CORV_BCR, BCR_RESET_EXECP_COND | bcr);

		DBG_DERR(("?corv_process_intr: Exception Condition: "));
		switch (status & BSR_EXCPN_MASK) {
		case 0x00:
			DBG_DERR(("?Adapter Detected Sync Channel Check\n"));
			break;
		case 0x20:
			DBG_DERR(("?Adapter Hardware Failure\n"));
			break;
		case 0x40:
			DBG_DERR(("?Invalid Command / Low Level Management"
				  " Request Failure\n"));
			break;
		case 0x60:
			DBG_DERR(("?Adapter Inconsistent/Illogical State\n"));
			break;
		case 0x80:
			DBG_DERR(("?Corrupted Pipe Detected\n"));
			break;
		case 0xA0:
			DBG_DERR(("?Pipe Control Error\n"));
			break;
		case 0xC0:
		case 0xE0:
			DBG_DERR(("?Reserved\n"));
			break;
		}
	}

#ifdef notyet
	if (status & BSR_INTR_REQ)
#endif
	{
		deq_element(&corv_blkp->cb_pipep, ioaddr, corv_chkstatus
						, corv_blkp);
	}
	return;
}

static void
corv_chkstatus( void	*arg,
		ELE_HDR	*deqp )
{
	CORV_BLK	*corv_blkp = arg;
	ELEMENT 	*ce_ptr = (ELEMENT *)deqp;
	struct corv_ccb	*ccbp;
	struct scsi_pkt	*pktp = NULL;
	int 		 rc;


	DBG_DINTR(("corv_chkstatus: eid 0x%x\n", ce_ptr->hdr.eid));

	switch (ce_ptr->hdr.eid) {
	case REPLY:
		ccbp = (CORV_CCB *)ce_ptr->hdr.corrid;

#ifdef CORV_QDEBUG
		cdbg_qdebug(corv_blkp, ccbp, ce_ptr);
#endif

		if (!ccbp) {
			/* this shouldn't happen */
			DBG_DERR(("corv_chkstatus: REPLY 0x%x: null corrid\n"
					, ce_ptr));
#ifdef CORV_DEBUG
			dump_ele(ce_ptr);
#endif
			return;
		}

		pktp = CCBP2PKTP(ccbp);
		if (!pktp) {
			int	len = ce_ptr->hdr.length;

			/* internal request, copy out the whole reply */
			if (len > sizeof ccbp->ccb_ele)
				len = sizeof ccbp->ccb_ele;
			bcopy((caddr_t)ce_ptr, (caddr_t)&ccbp->ccb_ele, len);
			break;
		}

		DBG_DINTR(("REPLY: opcode 0x%x\n", ce_ptr->hdr.opcode));
		DBG_DINTR(("...REP...."));

		*pktp->pkt_scbp  = STATUS_GOOD;
		pktp->pkt_reason = CMD_CMPLT;
		pktp->pkt_resid = 0;
		pktp->pkt_state = STATE_XFERRED_DATA | STATE_GOT_BUS |
				  STATE_GOT_TARGET | STATE_SENT_CMD;

		if (ce_ptr->hdr.opcode == READ_LIST
		||  ce_ptr->hdr.opcode == WRITE_LIST
		||  ce_ptr->hdr.opcode == SEND_SCSI) {
			DBG_DINTR(("corv_chkstatus: SCSI_STATUS = 0x%x\n"
				, ((SND_RDLIST_REP *)ce_ptr)->scsi_sts));
			DBG_DINTR(("corv_chkstatus: RESIDUAL COUNT = %d\n"
				, ((SND_RDLIST_REP *)ce_ptr)->residual_count));

			pktp->pkt_resid
				= ((SND_RDLIST_REP *)ce_ptr)->residual_count;
			if (((SND_RDLIST_REP *)ce_ptr)->valid) {
				*pktp->pkt_scbp
					= ((SND_RDLIST_REP *)ce_ptr)->scsi_sts;
			}

		} else if (ce_ptr->hdr.opcode == READ_DEV_CAPACITY) {
			bcopy((caddr_t)&ccbp->ccb_sense.sts_sensedata
				, ccbp->ccb_baddr, 8);
		}
		break;

	case ERROR:
		DBG_DINTR(("corv_chkstatus: error\n"));

		ccbp = (CORV_CCB *)ce_ptr->hdr.corrid;

#ifdef CORV_QDEBUG
		cdbg_qdebug(corv_blkp, ccbp, ce_ptr);
#endif

		/* if it's a generic error element check to
		 * see if the device queue has been suspended
		 */
		corv_chksusp(corv_blkp, ccbp, (EXT_REPLY_ELE *)ce_ptr);

		if (!ccbp) {
			/* this shouldn't happen */
			DBG_DERR(("corv_chkstatus: ERROR 0x%x:"
				  " null correlation id\n", ce_ptr));
#ifdef CORV_DEBUG
			dump_ele(ce_ptr);
#endif
			return;
		}

		pktp = CCBP2PKTP(ccbp);
		if (!pktp) {
			int	len = ce_ptr->hdr.length;

			DBG_DINTR(("corv_chkstatus: ERROR: null pktp\n"));
			/* internal request, copy out the whole error */
			if (len > sizeof ccbp->ccb_ele)
				len = sizeof ccbp->ccb_ele;
			bcopy((caddr_t)ce_ptr, (caddr_t)&ccbp->ccb_ele, len);
			break;
		}
		corv_seterr(pktp, ce_ptr);
		break;

	case EVENT:
		DBG_DERR(("?corv_chkstatus: EVENT 0x%x eid %d length %d"
			  " opcode = %d\n"
				, ce_ptr, ce_ptr->hdr.eid
				, ce_ptr->hdr.length, ce_ptr->hdr.opcode));

		if (ce_ptr->hdr.opcode == LOOP_SG_EVENT) {
			cmn_err(CE_WARN, "corv_chkstatus: Loop S/G intrrpt");

		} else if (ce_ptr->hdr.opcode == READ_IMMEDIATE) {
			/* process the read immediate event */
			corv_read_immed(corv_blkp, (READ_IMMEDIATE_ELE*)ce_ptr);

		} else {
			cmn_err(CE_WARN, "corv_chkstatus: unknown event");
		}
		return;

	case REQUEST:
	default:
		DBG_DERR(("?corv_chkstatus: invalid element 0x%x\n", ce_ptr));
#ifdef CORV_DEBUG
		dump_ele(ce_ptr);
#endif
		return;
	}

	if (ccbp) {
		corv_doneq_add(corv_blkp, ccbp);
	}

	if (corv_blkp->cb_num_of_cmds_qd == 0) {
		DBG_DERR(("?corv_chkstatus: invalid queue count\n"));
	} else {
		corv_blkp->cb_num_of_cmds_qd -= 1;
	}

	return;
}


static void
corv_chksusp(	CORV_BLK	*corv_blkp,
		CORV_CCB	*ccbp,
		EXT_REPLY_ELE	*ce_ptr )
{
	CORV_INFO	*corvp;
	CORV_UNIT	*unitp;
	REACT_SCSI_REQ	 reactq;
	u_int		 opcode;
	int	 	 rc;

	opcode = ce_ptr->gen_rep.gen_ctl.opcode;

	/* these functions don't return TSB status */
	if (opcode == MANAGEMENT
	||  opcode == READ_IMMEDIATE
	||  opcode == INIT_SCSI) {
		return;
	}

	if (!(ce_ptr->tsb.t_rtrycnt & DEV_CMD_Q_SUSP)) {
		DBG_DERR2(("corv_chksusp: not suspended, opcode=0x%x", opcode));
		return;
	}

	if (!ccbp) {
		DBG_DERR2(("corv_chksusp: suspended null correlation id,"
			  " opcode=0x%x\n", opcode));
#ifdef CORV_DEBUG
		dump_ele((ELEMENT *)ce_ptr);
#endif
		return;
	}
	corvp = ccbp->ccb_corvp;

	/* need to reactivate the corvette command queue */
#ifdef corv_old_way
	unitp = corvp->c_unitp;
	unitp->cu_suspended = TRUE;

#else
	/* immediately reactivate the queue, suppress the response */
	bzero((caddr_t)&reactq, sizeof reactq);
	reactq.gen_ctl.length = sizeof (REACT_SCSI_REQ);
	reactq.gen_ctl.opcode = REACT_QUEUE;
	reactq.gen_ctl.expedite = 1;
	reactq.gen_ctl.sup = 1;
	reactq.gen_ctl.eid = REQUEST;
	reactq.gen_ctl.DESTID.dest_ent_id   = corvp->c_entity_id;
	reactq.gen_ctl.DESTID.dest_unit_id  = ADP_UID;

	rc = enq_element(&corv_blkp->cb_pipep, &reactq.gen_ctl
					     , corv_blkp->cb_ioaddr);
	if (rc != DS_SUCCESS) {
		DBG_DERR(("?corv_chksusp: reactivate request failed\n"));
	}
#endif

	DBG_DERR2(("corv_chksusp(%d,%d,%d): suspended entity %d ioadddr 0x%x\n"
			, corvp->c_sbp->sb_bus
			, corvp->c_unitp->cu_eidreq.targ
			, corvp->c_unitp->cu_eidreq.lunn_1
			, corvp->c_entity_id
			, corvp->c_blkp->cb_ioaddr));
	return;
}


static void
corv_seterr(	struct scsi_pkt	*pktp,
		ELEMENT		*ce_ptr )
{
        struct corv_tsb		*tsbp;
	struct scsi_arq_status	*arqp;
        struct scsi_cmd		*cmd = SCMD_PKTP(pktp);
	int			 temp;
	char			*errmsg;

       	tsbp = (CORV_TSB *)&((EXT_REPLY_ELE *)ce_ptr)->tsb;

	/* assume good status and no errors */
	pktp->pkt_reason = CMD_CMPLT;
	pktp->pkt_state = STATE_GOT_BUS | STATE_GOT_TARGET | STATE_SENT_CMD;
	pktp->pkt_statistics = 0;
        *pktp->pkt_scbp = tsbp->t_scsistat << 1;
	pktp->pkt_resid = tsbp->t_resid;

#ifdef CORV_DEBUG
        DBG_DERR(("?corv_seterr(%d,%d): cmd %x endstatus %x"
			" scsistat %x cmdstatus %x deverror %x cmderror %x"
			" resid %x\n"
			, pktp->pkt_address.a_target, pktp->pkt_address.a_lun
			, *(pktp->pkt_cdbp), tsbp->t_endstatus
			, tsbp->t_scsistat << 1, tsbp->t_cmdstatus
			, tsbp->t_deverror, tsbp->t_cmderror, tsbp->t_resid));
#endif


        if (tsbp->t_scsistat == STATUS_GOOD
	&&  tsbp->t_cmderror == CMD_NO_ERROR
	&&  tsbp->t_deverror == NO_ERROR) {
		/* this isn't supposed to happen but handle it anyways */
		return;
	}

	switch ((tsbp->t_scsistat << 1) & STATUS_MASK) {
	case STATUS_GOOD:
		/* process the cmd and dev errors below */
		break;

	case STATUS_MET:
	case STATUS_INTERMEDIATE:
	case STATUS_INTERMEDIATE_MET:
	case STATUS_BUSY:
	case STATUS_RESERVATION_CONFLICT:
	default:
		/* just return the non-zero scsi status byte */
		pktp->pkt_state |= STATE_GOT_STATUS;
		return;

	case STATUS_CHECK:
		pktp->pkt_state |= STATE_GOT_STATUS;
		if (!(tsbp->t_rtrycnt & ARQ_DTA_VALID))
			return;

		/* automatic request sense data valid */
		pktp->pkt_state |= STATE_ARQ_DONE;

		arqp = (struct scsi_arq_status *)pktp->pkt_scbp;
		*((char *)&arqp->sts_rqpkt_status) = STATUS_GOOD;
		arqp->sts_rqpkt_reason = CMD_CMPLT;
		arqp->sts_rqpkt_state = STATE_GOT_BUS | STATE_GOT_TARGET
				      | STATE_SENT_CMD | STATE_XFERRED_DATA;
		arqp->sts_rqpkt_statistics = 0;

		/* the t_resid count actually applies to the req. sense */
		arqp->sts_rqpkt_resid = tsbp->t_resid;
		pktp->pkt_resid = 0;
#ifdef CORV_DEBUG
		DBG_DERR(("?corv_seterr: arq done\n"));
#endif
		return;
	}

	/* else-if */
	if (tsbp->t_cmderror) {
		switch (tsbp->t_cmderror) {
		case CORV_CMDERR_BADCCB:
			errmsg = "invalid parameter in SCB";
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case CORV_CMDERR_BADCMD:
			errmsg = "command not supported";
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case CORV_CMDERR_SYSABORT:
			errmsg = "command aborted (by system)";
			pktp->pkt_reason = CMD_ABORTED;
			pktp->pkt_statistics |= STAT_ABORTED;
			break;

		case CORV_CMDERR_BADLD:
			errmsg = "Logical device not mapped";
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case CORV_CMDERR_END:
			errmsg = "maximum LBA exceeded";
			pktp->pkt_reason = CMD_CMPLT;
			*pktp->pkt_scbp  = STATUS_CHECK;
			break;

		case CORV_CMDERR_END16:
			errmsg = "16-bit card slot address range exceeded";
			pktp->pkt_reason = CMD_CMPLT;
			*pktp->pkt_scbp  = STATUS_CHECK;
			break;

		case CMD_REJECTED_4:
			errmsg = "suspended SCSI Queue and Adapter Queue Full";
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case CORV_CMDERR_BADDEV:
			errmsg = "invalid device for command";
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case CORV_CMDERR_TIMEOUT:
			errmsg = "adapter detected global command timeout";
			pktp->pkt_reason = CMD_TRAN_ERR;
			pktp->pkt_statistics |= STAT_TIMEOUT;
			break;

		case CORV_CMDERR_DMA:
			errmsg = "DMA error";
			pktp->pkt_reason = CMD_DMA_DERR;
			break;

		case CORV_CMDERR_ABORT:
			errmsg = "command aborted by adapter";
			pktp->pkt_reason = CMD_ABORTED;
			pktp->pkt_statistics |= STAT_ABORTED;
			break;

		default:
			DBG_DERR(("corv_seterr: command error: 0x%x\n"
					, tsbp->t_cmderror));
			errmsg = NULL;
			pktp->pkt_reason = CMD_TRAN_ERR;
		}
		if (errmsg)
			DBG_DERR(("?corv_seterr: command error: %s\n", errmsg));
		return;
	}

	/* else if (tsbp->t_deverror) */
	switch (tsbp->t_deverror) {
	case SCSI_BUS_RESET:
		errmsg = "bus reset";
		pktp->pkt_reason = CMD_RESET;
		pktp->pkt_statistics = STAT_BUS_RESET;
		break;

	case WRITE_ERROR:
		errmsg = "write error";
		pktp->pkt_reason = CMD_INCOMPLETE;
		pktp->pkt_state  = STATE_GOT_BUS | STATE_GOT_TARGET
				 | STATE_SENT_CMD | STATE_XFERRED_DATA;
		break;

	case SRT_LTH_RCD_ERR:
		/* the pkt residual count has already been set */
#ifdef CORV_DEBUG
		errmsg = "short length record";
#else
		errmsg = NULL;
#endif
		break;

	case SCSI_SEL_TO:
#ifdef CORV_DEBUG
		errmsg = "selection timeout";
#else
		errmsg = NULL;
#endif
		pktp->pkt_reason = CMD_TIMEOUT;
		pktp->pkt_statistics = STAT_TIMEOUT;
		pktp->pkt_state  = STATE_GOT_BUS;
		break;

	default:
		DBG_DERR(("?corv_seterr: device error = 0x%x\n"
				, tsbp->t_deverror));
		errmsg = NULL;
		pktp->pkt_reason = CMD_TRAN_ERR;
		pktp->pkt_state  = STATE_GOT_BUS;
		break;

	}
	if (errmsg )
		DBG_DERR(("?corv_seterr: device error = %s\n", errmsg));

	return;

}


/*
 * handle Read Immediate Event
 */

static void
corv_read_immed(	CORV_BLK		*corv_blkp,
			READ_IMMEDIATE_ELE	*ce_ptr )
{
	ushort	 rc = ce_ptr->ret_code;
	ushort	 ioaddr = corv_blkp->cb_ioaddr;
	char	*bmsg;
	unchar	 bus;
	ushort	 target;

	if ((rc & 0x000f) == 0x0001) {
		bmsg = "internal";
		bus = 0;
	} else if ((rc & 0x000f) == 0x0002) {
		bmsg = "external";
		bus = 1;
	} else {
		bmsg = "(unknown)";
		bus = -1;
	}

	switch (rc & 0xf00) {
	case 0x0000:
		cmn_err(CE_WARN,
			"corvette: ioaddr 0x%x: %s SCSI bus reset occurred"
			, ioaddr, bmsg);
		break;

	case 0x0100:
		target = (rc >> 4) & 0xf;
		cmn_err(CE_WARN,
			"corvette: ioaddr 0x%x: unexpected device reselect"
			" occurred on the %s SCSI bus device=%d"
			, ioaddr, bmsg, target);

		/* try to send an ABORT to this device */
		corv_abort_immed(corv_blkp, bus, target);
		break;

	case 0x0200:
		cmn_err(CE_WARN,
			"corvette: ioaddr 0x%x: terminator power circuit"
			" breaker open on %s SCSI bus"
			, ioaddr, bmsg);
		break;

	case 0x0300:
		cmn_err(CE_WARN,
			"corvette: ioaddr 0x%x: differential sense error"
			" on % SCSI bus"
			, ioaddr, bmsg);
		break;
	}


	return;

}


/*
 * Send an ABORT message if a READ IMMEDIATE event is received
 */

static void
corv_abort_immed(	CORV_BLK	*corv_blkp,
			unchar		 bus,
			ushort		 target )
{
	CORV_UNIT	*unitp;
	ABORT_SCSI_REQ	 abort;
	int		 index;
	int	 	 rc;

#ifdef CORV_DEBUG
	debug_enter("corv_abort_immed");
#endif

	/* check for an existing Entity ID mapping */
	index = corv_map_find(corv_blkp, bus, target, 0);

	if (index == -1) {
		/* not in the map and no free entries */
		DBG_DERR(("corv_abort_immed: find failed\n"));
		return;
	}

	if ((unitp = corv_blkp->cb_eid_map[index]) == NULL) {
		/* why is device trying to reselect */
		DBG_DERR(("corv_abort_immed: idle device\n"));
		return;
	}

	/* send an ABORT, suppress the response */
	bzero((caddr_t)&abort, sizeof (abort));
	abort.gen_req.gen_ctl.length = sizeof (abort);
	abort.gen_req.gen_ctl.opcode = ABORT_SCSI;
	abort.gen_req.gen_ctl.expedite = 1;
	abort.gen_req.gen_ctl.sup = 1;
	abort.gen_req.gen_ctl.eid = REQUEST;
	abort.gen_req.gen_ctl.DESTID.dest_ent_id = unitp->cu_eidreq.entity_id;
	abort.gen_req.gen_ctl.DESTID.dest_unit_id = ADP_UID;
	abort.gen_req.flag1 = REACT_Q;

	rc = enq_element(&corv_blkp->cb_pipep, &abort.gen_req.gen_ctl
					     , corv_blkp->cb_ioaddr);
	if (rc != DS_SUCCESS) {
		DBG_DERR(("?corv_abort_immed: abort failed\n"));
	}
	return;
}

/*
 * Get logical device mapping routine normally called
 * from corv_transport with nowait == 1 can be called
 * from corv_reset() with nowait == 0
 */
static int
corv_get_eid(	CORV_BLK	*corv_blkp,
		CORV_INFO	*corvp,
		unchar		 bus,
		ushort		 target,
		unchar		 lun,
		int		 entity_id )
{
	CORV_CCB	*ccbp;
	EID_MGMT_REQ	*ce_ptr;
	int		 rc;

	ASSERT(entity > 0);
	ASSERT(entity_id <= CORV_MAX_EID);

	ccbp = corv_ccballoc(corv_blkp, NULL);

	if (!ccbp) {
		DBG_DERR(("?corv_get_eid: ccballoc failed\n"));
		return (FALSE);
	}


	ce_ptr = (EID_MGMT_REQ *)&ccbp->ccb_ele;

	build_eid_req(ccbp, ce_ptr, ASSIGN_EID, bus, target, lun, entity_id);

	/* save a copy of the request element for later lookups */
	bcopy((caddr_t)ce_ptr, (caddr_t)&corvp->c_unitp->cu_eidreq
			     , sizeof corvp->c_unitp->cu_eidreq);

	if (corv_mm_docmd(corv_blkp, ccbp) != DS_SUCCESS) {
		DBG_DERR(("?corv_get_eid: request failed\n"));
#ifdef CORV_DEBUG
		dump_ele((ELEMENT *)ce_ptr);
#endif
		corv_ccbfree(corv_blkp, ccbp);

		return (FALSE);
	}


	/* wait for return status if specified - in device initialization */
	if (!corv_mm_pollret(corv_blkp, ccbp)) {
		DBG_DERR(("?corv_get_eid: pollret failed\n"));
		return (FALSE);
	}

	rc = (ce_ptr->gen_mgmt.gen_ctl.eid == REPLY);
	if (!rc) {
		DBG_DERR(("?corv_get_eid: failed eid = 0x%x retcod = 0x%x\n"
			, ce_ptr->gen_mgmt.gen_ctl.eid
			, ce_ptr->gen_mgmt.func_sts));
	}

	ASSERT(ce_ptr->gen_mgmt.func_sts == 0
	||     ce_ptr->gen_mgmt.func_sts == 0x0100);

	corv_ccbfree(corv_blkp, ccbp);

	return (rc);
}

static void
corv_free_eid(	CORV_BLK	*corv_blkp,
		CORV_INFO	*corvp )
{
	/* DBG_DINIT(("corv_free_eid: start\n")); */

	return;
}

static void
corv_unmap_target(	CORV_BLK	*corv_blkp,
			CORV_INFO	*corvp )
{
#ifdef notyet
	int		 index;

	/* should save the index in the corv_info struct so avoid lookup */
	for(index = 0; index < num_eids; index++) {
		if (corvp->c_unitp == corv_blkp->cb_eid_map[index])
			goto found_it;
	}
	/* this shouldn't happen */
	return;

	/*
	 * Only call corv_free_eid() when no other device is
	 * using the same EID.
	 */
	if (corvp->c_unitp->cu_refcnt-- == 1) {
		corv_free_eid(corv_blkp, corvp);
		corv_blkp->cb_eid_map[index] = NULL;
		kmem_free(corvp->c_unitp, sizeof (CORV_UNIT));
	}

	kmem_free(corvp, sizeof (CORV_INFO));
#endif
	return;
}


/*
 * Search the Entity ID map for an entry already in use by the
 * specified SCSI device
 */

static int
corv_map_find(	CORV_BLK	*corv_blkp,
		unchar		 bus,
		ushort		 target,
		unchar		 lun )
{
	int		  first_free = -1;
	CORV_UNIT	**unitpp;
	CORV_UNIT	 *tmp_unitp;
	int		  index;
	int		  num_eids;

	num_eids = sizeof corv_blkp->cb_eid_map
			/ sizeof corv_blkp->cb_eid_map[0];
	unitpp = &corv_blkp->cb_eid_map[0];

	/* is device still mapped, i.e, (targ/lun <-> ld) intact ? */
	for(index = 0; index < num_eids; index++, unitpp++) {
		if (*unitpp == NULL) {
			/* remember the first unused entry */
			if (first_free == -1)
				first_free = index;
			continue;
		}

		tmp_unitp = *unitpp;

		if (tmp_unitp->cu_eidreq.bus != bus
		||  tmp_unitp->cu_eidreq.targ != target
		||  tmp_unitp->cu_eidreq.lunn_1 != lun)
			continue;
		return (index);
	}
	return (first_free);
}

/*
 * map the particular target and lun to an entity in the active list
 */
static CORV_INFO  *
corv_map_target(	CORV_BLK	*corv_blkp,
			scsi_hba_tran_t	*hba_tran,
			unchar		 bus,
			ushort		 target,
			unchar		 lun )
{
	CORV_INFO	*corvp;
	CORV_UNIT	*unitp;
	int		 index;
	int		 first_free = -1;
	int		 new_entity_id;



	DBG_DINIT(("corv_map_target: (%d,%d) blkp 0x%x\n", target, lun
							 , corv_blkp));

        if (!(corvp = kmem_zalloc(sizeof (CORV_INFO), KM_NOSLEEP)))
                return (NULL);

	corvp->c_tranp = hba_tran;
	corvp->c_blkp = corv_blkp;
	corvp->c_sbp = TRAN2CORVBUSP(hba_tran);

	/* find and existing map entry or the first free entry */
	index = corv_map_find(corv_blkp, bus, target, lun);

	if (index == -1) {
		/* the map is full, can not proceed */
		kmem_free(corvp, sizeof (CORV_INFO));
		return (NULL);
	}

	/* check if this (target, LUN) still has an Entity ID assigned */
	if ((unitp = corv_blkp->cb_eid_map[index]) != NULL) {
		/* it's here, incr use count, re-use existing entity id */
		unitp->cu_refcnt++;
		corvp->c_entity_id = unitp->cu_eidreq.entity_id;
		corvp->c_unitp = unitp;
		return (corvp);
	}

	/*
	 * No existing Entity ID map entry, setup a new entry in the map.
	 */

	if (!(unitp = kmem_zalloc(sizeof (CORV_UNIT), KM_NOSLEEP))) {
		kmem_free(corvp, sizeof (CORV_INFO));
                return (NULL);
	}

	unitp->cu_refcnt = 1;
	unitp->cu_dmalim = corv_dma_lim;
	corvp->c_unitp = unitp;

	/* Entity IDs are 1-based (the cb_eid_map index is 0-based) */
	new_entity_id = index + 1;
	corvp->c_entity_id = new_entity_id;

	if (!corv_get_eid(corv_blkp, corvp, bus, target, lun, new_entity_id)) {
		/* unable to assign an Entity ID for this (target, LUN) */
		kmem_free(unitp, sizeof (CORV_UNIT));
		kmem_free(corvp, sizeof (CORV_INFO));
		DBG_DINIT(("corv_map_target: failed\n"));
		return (NULL);
	}
	corv_blkp->cb_eid_map[index] = unitp;

	DBG_DINIT(("corv_map_target: emp %d corv 0x%x\n", first_free, corvp));
	return (corvp);
}

/*
 * Abort the command pktp on the target pktp on the target/lun
 * lun in ap. If pktp is NULL, abort all outstanding commands
 * on that target/lun. If you can  abort them, return 1,
 * else return 0. Each packet that's aborted should be sent
 * back to the target driver thru the callback routine,
 * with pkt_reason set to CMD_ABORTED.
 */

static int
corv_abort(	struct scsi_address	*ap,
		struct scsi_pkt		*pktp )
{
	CORV_INFO	*corvp = TRAN2CORVINFO(ADDR2TRAN(ap));
        CORV_BLK 	*corv_blkp = corvp->c_blkp;
        CORV_CCB	*ccbp;
	ABORT_SCSI_REQ 	*abort_reqp;
        int     	 rc;

	DBG_DERR(("corv_abort(%d,%d): corv_blkp 0x%x pktp 0x%x\n"
			, ap->a_target, ap->a_lun, corv_blkp, pktp));

        mutex_enter(&corv_blkp->cb_mutex);

	/* allocate a ccb */
	ccbp = corv_ccballoc(corv_blkp, corvp);

        abort_reqp = (ABORT_SCSI_REQ *)&ccbp->ccb_ele;


	/* build abort scsi request control element */
        if (pktp) {
		/* do SCSI ABORT not ABORT_TAG because most SCSI
		 * devices are known to mishandle ABORT_TAG
		 */
#ifdef notyet
		build_abort_req(ccbp, corvp, abort_reqp, ABORT_REQ
				, SCMD_PKTP(pktp)->cmd_private);
#else
		build_abort_req(ccbp, corvp, abort_reqp, 0, 0);
#endif
	} else {
		/* send SCSI ABORT command */
		build_abort_req(ccbp, corvp, abort_reqp, 0, 0);
	}


	/* put the abort request element into the pipe */
	if (corv_mm_docmd(corv_blkp, ccbp) != DS_SUCCESS) {
		DBG_DERR(("?corv_abort: enqueue failed\n"));
		rc = 0;
		goto bailout;
        }

	/* wait for it to complete */
	if (!corv_mm_pollret(corv_blkp, ccbp)) {
		DBG_DERR(("?corv_abort: request failed\n"));
		rc = 0;
        } else {
		rc = 1;
	}

bailout:
	corv_ccbfree(corv_blkp, ccbp);

        mutex_exit(&corv_blkp->cb_mutex);
        return (rc);
}

static int
corv_reset(	struct scsi_address	*ap,
		int			 level )
{
	CORV_INFO	*corvp = TRAN2CORVINFO(ADDR2TRAN(ap));
        CORV_BLK 	*corv_blkp = corvp->c_blkp;
        CORV_CCB  	*ccbp;
	ABORT_SCSI_REQ 	*abort_reqp;
        int     	 rc;


	DBG_DERR(("corv_reset(%d,%d): corv_blkp 0x%x level %d\n"
			, ap->a_target, ap->a_lun, corv_blkp, level));

        mutex_enter(&corv_blkp->cb_mutex);

	/* allocate a ccb */
	ccbp = corv_ccballoc(corv_blkp, corvp);

	if (!ccbp) {
		mutex_exit(&corv_blkp->cb_mutex);
		return (0);
	}

        abort_reqp = (ABORT_SCSI_REQ *)&ccbp->ccb_ele;

	switch (level) {
	case RESET_ALL:
		/*
		 * Reset the scsi bus, kill all commands in progress
		 * (remove them from lists, etc.) Make sure you
		 * wait the specified time for the reset to settle,
		 * if your hardware dosen't do that for you somehow.
		 */
		build_init_req(ccbp, corvp, (INIT_SCSI_REQ *)abort_reqp, 0);
		break;

	case RESET_TARGET:

		/*
		 * Issue a Bus Device Reset message to the target/lun
		 * specified in ap; loop thru all outstanding
		 * packets for that target/lun and call pkt_comp
		 * after setting pkt_reason to CMD_RESET
		 */
		build_abort_req(ccbp, corvp, abort_reqp, ABORT_ALL, 0);
		break;

	default:
		rc = 0;
		goto bailout;
	}

	/* put the reset request element into the pipe */
	if (corv_mm_docmd(corv_blkp, ccbp) != DS_SUCCESS) {
		DBG_DERR(("?corv_reset: enqueue failed\n"));
		rc = 0;
		goto bailout;
        }

	/* wait for return status */
	if (!corv_mm_pollret(corv_blkp, ccbp)) {
		DBG_DERR(("?corv_reset(%d,%d): level %d failed\n"
				, level, ap->a_target, ap->a_lun));
		rc = 0;
	} else {
		rc = 1;
		if (level == RESET_ALL) {
			/* bus reset cancels all replys, reset my counter */
			corv_blkp->cb_num_of_cmds_qd = 0;
		}
	}


bailout:
	corv_ccbfree(corv_blkp, ccbp);

	mutex_exit(&corv_blkp->cb_mutex);
	return (rc);
}

/******************************************************************************
 *	Move Mode Implementation
 ******************************************************************************/

static void
unconfig_delivery_pipe( CORV_BLK *corv_blkp )
{

/* ??? need to first reset the adapter and/or stop the pipes ??? */

	if (corv_blkp->cb_adp_sca)
		ddi_iopb_free((caddr_t)corv_blkp->cb_adp_sca);

	if (corv_blkp->cb_sys_sca)
		ddi_iopb_free((caddr_t)corv_blkp->cb_sys_sca);

	if (corv_blkp->cb_ib_pipep)
		ddi_iopb_free((caddr_t)corv_blkp->cb_ib_pipep);

	if (corv_blkp->cb_ob_pipep)
		ddi_iopb_free((caddr_t)corv_blkp->cb_ob_pipep);

	return;
}

static int
config_delivery_pipe( CORV_BLK *corv_blkp )
{
        paddr_t		 phyaddr;
	int		 index;
	MGMT_CFG_PIPES	*cfg_pipep;
	CTRLAREA	*adp_sca;
	CTRLAREA	*sys_sca;
	caddr_t		 ib_pipep;
	caddr_t		 ob_pipep;

/* ??? all the failure returns need to free allocated buffers ??? */

	if (ddi_iopb_alloc(corv_blkp->cb_dip, (ddi_dma_lim_t *)0
					    , sizeof(*cfg_pipep)
					    , (caddr_t *)&cfg_pipep)) {

		DBG_DINIT(("corv_cfginit: unable to allocate memory\n"));
		return (FALSE);
	}

	bzero((caddr_t)cfg_pipep, sizeof(*cfg_pipep));

	if (ddi_iopb_alloc(corv_blkp->cb_dip, (ddi_dma_lim_t *)0
/* ??? why is this +4 necessary ??? */
					    , 2 * (sizeof(CTRLAREA) + 4)
					    , (caddr_t *)&adp_sca)) {

		DBG_DINIT(("corv_cfginit: unable to allocate memory\n"));
		return (FALSE);
	}
	bzero((caddr_t)(adp_sca), 2 * (sizeof(CTRLAREA) + 4));
	sys_sca =  (CTRLAREA *)(adp_sca + 1);


	if (ddi_iopb_alloc(corv_blkp->cb_dip, (ddi_dma_lim_t *)0
					    , PIPE_SIZE, &ib_pipep)) {

		DBG_DINIT(("corv_cfginit: unable to allocate memory\n"));
		return (FALSE);
	}

	bzero((caddr_t)ib_pipep, PIPE_SIZE);

	if (ddi_iopb_alloc(corv_blkp->cb_dip, (ddi_dma_lim_t *)0
					    , PIPE_SIZE, &ob_pipep)) {

		DBG_DINIT(("corv_cfginit: unable to allocate memory\n"));
		return (FALSE);
	}

	bzero((caddr_t)ob_pipep, PIPE_SIZE);

	/* save the pointers so unconfig_delivery_pipe can free the buffers */
	corv_blkp->cb_adp_sca = adp_sca;
	corv_blkp->cb_sys_sca = sys_sca;
	corv_blkp->cb_ib_pipep = ib_pipep;
	corv_blkp->cb_ob_pipep = ob_pipep;

	/* Type/Length */
	cfg_pipep->gen_mgmt.gen_ctl.length = 0x0058;
	cfg_pipep->gen_mgmt.gen_ctl.type = 0;

	/* Management Req */
	cfg_pipep->gen_mgmt.gen_ctl.opcode = 0x10;
	cfg_pipep->gen_mgmt.gen_ctl.expedite = 0;
	cfg_pipep->gen_mgmt.gen_ctl.fixed = 0;
	cfg_pipep->gen_mgmt.gen_ctl.cc = 0;
	cfg_pipep->gen_mgmt.gen_ctl.sup = 0;
	cfg_pipep->gen_mgmt.gen_ctl.eid = REQUEST;

	/* Source/Dest ID */
	cfg_pipep->gen_mgmt.gen_ctl.DESTID.dest_ent_id   = 0;
	cfg_pipep->gen_mgmt.gen_ctl.DESTID.dest_unit_id  = ADP_UID;
	cfg_pipep->gen_mgmt.gen_ctl.srcid  = 0;

	/* Corrid */
	cfg_pipep->gen_mgmt.gen_ctl.corrid  = (ulong)CORV_KVTOP(cfg_pipep);

	/* function / id */
	cfg_pipep->gen_mgmt.id  	= 0;
	cfg_pipep->gen_mgmt.func_sts  	= PIPE_CONFIG;

	/* status / id's */
	cfg_pipep->adp_uid  	= ADP_UID;
	cfg_pipep->peer_uid  	= 0;
	cfg_pipep->config_sts  	= 0xffff;	/* all 1's */
	cfg_pipep->peer_sigp	= (ulong)CORV_KVTOP(&sys_sca->signal);
	cfg_pipep->adp_sigp  	= (ulong)CORV_KVTOP(&adp_sca->signal);
	cfg_pipep->adp_ioaddr   = corv_blkp->cb_ioaddr;
	cfg_pipep->peer_ioaddr  = 0;

	/* Timer Control frequency */
	cfg_pipep->timer_freq 	= 0;
	cfg_pipep->time_unit 	= 0;
	cfg_pipep->sys_mid 	= 0;

	/* Configuration Option */

	/* System shared memory, unit is adapter, signal empty to not empty */
	cfg_pipep->adp_cfg_opt 	= PP_SYS_MEM | UT_IS_ADAPTER | SIG_NOT_EMPTY;

	/* System shared memory, unit is system, signal on empty to not empty */
	cfg_pipep->peer_cfg_opt = PP_SYS_MEM | UT_IS_SYSTEM | SIG_NOT_EMPTY;
 
	cfg_pipep->ob_pipe_sz = PIPE_SIZE;
	cfg_pipep->ib_pipe_sz = PIPE_SIZE;
	cfg_pipep->ib_pipep = (ulong)CORV_KVTOP(ib_pipep);

	/* Inbound ISDS addr */
	cfg_pipep->isdsp = (ulong)CORV_KVTOP(&(adp_sca->sds));

	/* Inbound ISSE addr */
	cfg_pipep->issep = (ulong)CORV_KVTOP(&(adp_sca->sse));

	/* Inbound ISES  addr */
	cfg_pipep->isesp = (ulong)CORV_KVTOP(&(sys_sca->ses));

	/* Inbound ISSF  addr */
	cfg_pipep->issfp = (ulong)CORV_KVTOP(&(sys_sca->ssf));

	/* address of outbound pipe */
	cfg_pipep->ob_pipep = (ulong)CORV_KVTOP(ob_pipep);

	/* outbound OSDS addr */
	cfg_pipep->osdsp = (ulong)CORV_KVTOP(&(sys_sca->sds));

	/* outbound OSSE addr */
	cfg_pipep->ossep = (ulong)CORV_KVTOP(&(sys_sca->sse));

	/* outbound OSES  addr */
	cfg_pipep->osesp = (ulong)CORV_KVTOP(&(adp_sca->ses));

	/* outbound OSSF addr */
	cfg_pipep->ossfp = (ulong)CORV_KVTOP(&(adp_sca->ssf));

	/* initialize the state of the pipes and my local pointers */
	init_pipe_desc(&corv_blkp->cb_pipep, cfg_pipep, adp_sca, sys_sca
				       , ib_pipep, ob_pipep);

	if (!corv_docmd(corv_blkp, (int)(cfg_pipep), 0, MANAGEMENT_REQUEST)) {
                DBG_DINIT(("config_delivery_pipe: docmd failed\n"));
		return (FALSE);
	}

	if (!corv_pollret(corv_blkp)) {
                DBG_DINIT(("config_delivery_pipe: pollret failed\n"));
		return (FALSE);
	}

	/* if exception, the error element is returned in my cfg_pipep buffer */
	if (corv_blkp->cb_exp_cond) {
                DBG_DINIT(("config_delivery_pipe: fail retcod=0x%x\n"
				, cfg_pipep->gen_mgmt.func_sts));
		return (FALSE);
	}

#ifdef CORV_DEBUG
	cdbg_adp_sca = adp_sca;
	cdbg_sys_sca = sys_sca;
	cdbg_ib_pipep = ib_pipep;
	cdbg_ob_pipep = ob_pipep;
#endif
	ddi_iopb_free((caddr_t)cfg_pipep);
	return (TRUE);

}

static void
init_pipe_desc( PIPEDS		*pipep,
		MGMT_CFG_PIPES	*pe,
		CTRLAREA	*adp_sigp,
		CTRLAREA	*sys_sigp,
		caddr_t		 ip,
		caddr_t		 op )
{
	/* pipe descriptor */

	/* pipe status	*/
	pipep->status = PIPESALLOCATED;

	/* !!! beware of Magic Number */
	pipep->pipe_id = 1;

	/* unit id 	*/
	pipep->unit_id = pe->peer_uid;

	/* adpter signalling control area */
	pipep->adp_sca = &adp_sigp->signal;

	/* system signalling control area */
	pipep->sys_sca = &sys_sigp->signal;

	/* adpater signalling options   */
	pipep->adp_cfg = pe->adp_cfg_opt;

	/* system signalling options   */
	pipep->sys_cfg = pe->peer_cfg_opt;

	/* in-bound pipe control info 	    */

	/* surrogate enq status    */
	pipep->ib_pipe.ses = (SES *)&sys_sigp->ses;

	/* surrogate start of free  */
	pipep->ib_pipe.ssf = (SSF *)&sys_sigp->ssf;

	/* surrogate deq status	    */
	pipep->ib_pipe.sds = (SDS *)&adp_sigp->sds;

	/* surrogate strt of eles   */
	pipep->ib_pipe.sse = (SSE *)&adp_sigp->sse;

	/* out_bound pipe control info 	    */

	/* surrogate enq status    */
	pipep->ob_pipe.ses = (SES *)&(adp_sigp->ses);

	/* surrogate start of free  */
	pipep->ob_pipe.ssf = (SSF *)&adp_sigp->ssf;

	/* surrogate deq status    */
	pipep->ob_pipe.sds = (SDS *)&(sys_sigp->sds);

	/* surrogate strt of eles   */
	pipep->ob_pipe.sse = (SSE *)&sys_sigp->sse;

	/* see if we can have some more here in future ???	*/

	/* initialize local dequeue control area */
	pipep->deq_ctrl.base   		= ip;
	pipep->deq_ctrl.we     		= 0;
	pipep->deq_ctrl.ds.flg.wrap 	= OFF;
	pipep->deq_ctrl.ds.flg.empty 	= ON;
	pipep->deq_ctrl.ds.flg.preempt 	= OFF;
	pipep->deq_ctrl.ds.flg.dequeued = OFF;
	pipep->deq_ctrl.ds.flg.full 	= OFF;
	pipep->deq_ctrl.se     		= 0;
	pipep->deq_ctrl.ee     		= 0;
	pipep->deq_ctrl.end    		= pe->ib_pipe_sz;
	pipep->deq_ctrl.top    		= pe->ib_pipe_sz - WRAP_EL_LEN;

	*(pipep->ib_pipe.ssf) 		= 0;
	pipep->ib_pipe.ses->flg.wrap 	= OFF;
	pipep->ib_pipe.ses->flg.full 	= OFF;

	/* initialize local enqueue control area */
	pipep->enq_ctrl.base   		= op;
	pipep->enq_ctrl.we     		= 0;
	pipep->enq_ctrl.es.flg.full 	= OFF;
	pipep->enq_ctrl.es.flg.empty 	= ON;
	pipep->enq_ctrl.es.flg.wrap 	= OFF;
	pipep->enq_ctrl.es.flg.queued 	= OFF;
	pipep->enq_ctrl.sf     		= 0;

	*(pipep->ob_pipe.sse) 		= 0;

	pipep->enq_ctrl.ef     		= pe->ob_pipe_sz - WRAP_EL_LEN;
	pipep->enq_ctrl.end    		= pe->ob_pipe_sz;
	pipep->enq_ctrl.top    		= pe->ob_pipe_sz - WRAP_EL_LEN;
}


/* move mode do cmd */
static int
corv_mm_docmd(	CORV_BLK	*corv_blkp,
		CORV_CCB	*ccbp )
{
	ELEMENT	*ep = &ccbp->ccb_ele;
	int	 rc;
#ifdef CORV_QDEBUG
	ushort	 qindex;
#endif


	if (corv_blkp->cb_num_of_cmds_qd >= 250) {
		DBG_DENQERR(("corv_mm_docmd: num cmds >= 250\n"));
		return (DS_PIPE_FULL);
	}

	rc = enq_element(&corv_blkp->cb_pipep, &ep->hdr, corv_blkp->cb_ioaddr);

	if (rc != DS_SUCCESS) {
		DBG_DENQERR(("corv_mm_docmd: 0x%x enqueue failed\n"
			, corv_blkp));
		return (rc);
	}

	corv_blkp->cb_num_of_cmds_qd += 1;
#ifdef CORV_QDEBUG
	for (qindex = 0; qindex < CDBG_MAX_QSLOTS; qindex++) {
		if (cdbg_qed_ccbs[corv_blkp->cb_mca_slot].ccbp[qindex])
			continue;

		cdbg_qed_ccbs[corv_blkp->cb_mca_slot].ccbp[qindex] = ccbp;
		cdbg_qed_ccbs[corv_blkp->cb_mca_slot].qcnt += 1;
		break;
	}
#endif
	return (rc);
}


static void
build_abort_req(	CORV_CCB	*ccbp,
			CORV_INFO	*corvp,
			ABORT_SCSI_REQ	*ce_ptr,
			unchar		 option,
			ulong		 param )
{
	/* length of the ctrl element in bytes 	*/
	ce_ptr->gen_req.gen_ctl.length = sizeof (ABORT_SCSI_REQ);
	ce_ptr->gen_req.gen_ctl.opcode = ABORT_SCSI;
	ce_ptr->gen_req.gen_ctl.expedite = 1;
	ce_ptr->gen_req.gen_ctl.eid = REQUEST;
	ce_ptr->gen_req.gen_ctl.DESTID.dest_ent_id = corvp->c_entity_id;
	ce_ptr->gen_req.gen_ctl.DESTID.dest_unit_id = ADP_UID;
	ce_ptr->gen_req.gen_ctl.srcid = 0;
	ce_ptr->gen_req.gen_ctl.corrid = (ulong)ccbp;
	ce_ptr->gen_req.flag1 = REACT_Q;

	/* request specific */
	ce_ptr->gen_req.specific1 = option;

	ce_ptr->abort_corr_id = param;

	return;
}


static void
build_init_req(	CORV_CCB	*ccbp,
		CORV_INFO	*corvp,
		INIT_SCSI_REQ	*ce_ptr,
		int		 bus )
{
	/* length of the ctrl element in bytes 	*/
	ce_ptr->gen_ctl.length = sizeof (INIT_SCSI_REQ);
	ce_ptr->gen_ctl.opcode = INIT_SCSI;
	ce_ptr->gen_ctl.expedite = 1;
	ce_ptr->gen_ctl.eid = REQUEST;
	ce_ptr->gen_ctl.DESTID.dest_ent_id = 0;
	ce_ptr->gen_ctl.DESTID.dest_unit_id = ADP_UID;
	ce_ptr->gen_ctl.srcid = 0;
	ce_ptr->gen_ctl.corrid = (ulong)ccbp;

	/* request specific */
	ce_ptr->delay = 0;
	if (bus == 0) {
		ce_ptr->flag = RESET_INTERNAL;
	} else if (bus = 1) {
		ce_ptr->flag = RESET_EXTERNAL;
	} else {
		ce_ptr->flag = HARD_RESET;
	}
	ce_ptr->resv1_0 = 0;
	return;
}

static void
build_eid_req(	CORV_CCB	*ccbp,
		EID_MGMT_REQ	*ce_ptr,
		int		 func_code,
		unchar		 bus,
		ushort		 targ,
		unchar		 lun,
		unchar		 eid )
{

	ce_ptr->gen_mgmt.gen_ctl.length = sizeof (EID_MGMT_REQ);
	ce_ptr->gen_mgmt.gen_ctl.opcode = MANAGEMENT;
	ce_ptr->gen_mgmt.gen_ctl.eid = REQUEST;

	ce_ptr->gen_mgmt.gen_ctl.DESTID.dest_ent_id   = 0;
	ce_ptr->gen_mgmt.gen_ctl.DESTID.dest_unit_id  = ADP_UID;
	ce_ptr->gen_mgmt.gen_ctl.srcid  = 0;

	ce_ptr->gen_mgmt.gen_ctl.corrid  =  (ulong)ccbp;

	ce_ptr->gen_mgmt.func_sts = func_code;

	ce_ptr->entity_id  =  eid;
	ce_ptr->bus  = bus;
	ce_ptr->targ  = targ;
	ce_ptr->lunn_1  = lun;

	/* don't allow tag queuing, our target drivers don't work correctly */
	ce_ptr->tag_queue = 1;
	return;
}

static void
build_send_scsi_req(	CORV_CCB	*ccbp,
			unchar		 entity_id,
			int		 cdblen,
			int		 nodisco,
			int		 arq )
{
	SEND_SCSI_REQ	*ce_ptr = (SEND_SCSI_REQ *)&ccbp->ccb_ele;
	int	i;

	bzero((caddr_t)ce_ptr, sizeof (ce_ptr->gen_req.gen_ctl));

	/* size of send_scsi_req */
	ce_ptr->gen_req.gen_ctl.length
		= sizeof (SEND_SCSI_REQ)
		- sizeof ce_ptr->buf_list
		+ (ccbp->ccb_blk_cnt * sizeof (ce_ptr->buf_list[0]));

	ce_ptr->gen_req.gen_ctl.opcode = SEND_SCSI;
	ce_ptr->gen_req.gen_ctl.eid = REQUEST;
	ce_ptr->gen_req.gen_ctl.DESTID.dest_ent_id   = entity_id;
	ce_ptr->gen_req.gen_ctl.DESTID.dest_unit_id  = ADP_UID;
	ce_ptr->gen_req.gen_ctl.corrid = (ulong)ccbp;

	ce_ptr->gen_req.flag1 &= DIR;
	ce_ptr->gen_req.flag1 |= SUP;
	ce_ptr->gen_req.flag2 = 0;
	ce_ptr->gen_req.time_out_cnt = 0;

	/* check if arq supported by target */
	if (arq) {
		ce_ptr->gen_req.flag1 |= ARQ;
		ce_ptr->arq_addr = (ulong)(ccbp->ccb_paddr +
				((caddr_t)(&ccbp->ccb_sense.sts_sensedata) -
				(caddr_t)ccbp));
		ce_ptr->arq_len = sizeof ccbp->ccb_sense.sts_sensedata;
	}

	if (ccbp->ccb_cdb[0] == REQUEST_SENSE) {
		/* set the Untag bit and Reactivate Q bit */
		ce_ptr->gen_req.flag1 |= (UNTAG_Q|REACT_Q);
		ce_ptr->gen_req.gen_ctl.expedite = 1;
	}

	/* check if scsi bus disconnect is disabled */
	if (nodisco)
		ce_ptr->gen_req.flag1 |= NODISCON;

	/* set the CDB length and copy it */
	ce_ptr->gen_req.specific1 = cdblen;
	for(i =0; i < cdblen; i++)
		ce_ptr->cdb[i] = ccbp->ccb_cdb[i];

	DBG_DPKT(("build_send_scsi: %d segment transfer entity id = 0x%x\n"
			, ccbp->ccb_blk_cnt, entity_id));

	/* copy the scatter/gather list */
	ce_ptr->xfer_len = 0;
	for(i = 0; i < ccbp->ccb_blk_cnt; i++) {
		ce_ptr->buf_list[i].data_ptr = ccbp->ccb_sg_list[i].data_ptr;
		ce_ptr->buf_list[i].data_len = ccbp->ccb_sg_list[i].data_len;
		ce_ptr->xfer_len += ccbp->ccb_sg_list[i].data_len;
	}

	/* zero the uncopied portions of the S/G list */
	for( ; i < MAX_SG_SEGS; i++) {
		ce_ptr->buf_list[i].data_ptr = 0;
		ce_ptr->buf_list[i].data_len = 0;
	}

	/* tagged queuing is not yet supported */
	ce_ptr->gen_req.flag1 |= UNTAG_Q;
	return;
}

/*
 * This function only handles the SCMD_READ_G1 and SCMD_WRITE_G1 commands
 */
static void
build_rw_req(	CORV_CCB	*ccbp,
		struct scsi_pkt *pktp,
		unchar		 eid,
		unchar		 opcode,
		int		 arq )
{
	RDWR_LIST_REQ	*ce_ptr = (RDWR_LIST_REQ *)&ccbp->ccb_ele;
	int	i;

	bzero((caddr_t)ce_ptr, sizeof (ce_ptr->gen_req.gen_ctl));

	/* size of send_scsi_req */
	ce_ptr->gen_req.gen_ctl.length
		= sizeof (RDWR_LIST_REQ)
		- sizeof ce_ptr->buf_list
		+ (ccbp->ccb_blk_cnt * sizeof (ce_ptr->buf_list[0]));

	ce_ptr->gen_req.gen_ctl.opcode = opcode;
	ce_ptr->gen_req.gen_ctl.eid = REQUEST;
	ce_ptr->gen_req.gen_ctl.DESTID.dest_ent_id   = eid;
	ce_ptr->gen_req.gen_ctl.DESTID.dest_unit_id  = ADP_UID;
	ce_ptr->gen_req.gen_ctl.corrid  = (ulong)ccbp;

	if (opcode == WRITE_LIST)
		ce_ptr->gen_req.flag1 = 0;
	else
		ce_ptr->gen_req.flag1 = DIR | SUP;

	ce_ptr->gen_req.specific1 = 0;
	ce_ptr->gen_req.flag2 = 0;
	ce_ptr->gen_req.time_out_cnt = 0;

	/* check if arq supported by target */
	if (arq) {
		ce_ptr->gen_req.flag1 |= ARQ;
		ce_ptr->arq_addr = (ulong)(ccbp->ccb_paddr +
				((caddr_t)(&ccbp->ccb_sense.sts_sensedata) -
				(caddr_t)ccbp));
		ce_ptr->arq_len = sizeof ccbp->ccb_sense.sts_sensedata;
	}

	/* check for scsi bus disconnect */
	if (pktp->pkt_flags & FLAG_NODISCON)
		ce_ptr->gen_req.flag1 |= NODISCON;

	ce_ptr->lba_addr = scsi_stoh_long(*(ulong *)(&ccbp->ccb_cdb[2]));
	ce_ptr->blk_cnt = scsi_stoh_short(*(ushort *)(&ccbp->ccb_cdb[7]));
	ce_ptr->blk_len = PKT2CORVUNITP(pktp)->cu_dmalim.dlim_granular;

	for(i = 0; i < ccbp->ccb_blk_cnt; i++) {
		ce_ptr->buf_list[i].data_ptr =
			ccbp->ccb_sg_list[i].data_ptr;
		ce_ptr->buf_list[i].data_len =
			ccbp->ccb_sg_list[i].data_len;
	}
	for( ; i < MAX_SG_SEGS; i++) {
		ce_ptr->buf_list[i].data_ptr = 0;
		ce_ptr->buf_list[i].data_len = 0;
	}

	/* tagged queuing is not yet supported */
	ce_ptr->gen_req.flag1 |= UNTAG_Q;
	return;
}

static void
build_read_devcap_req(	CORV_CCB	*ccbp,
			struct scsi_pkt	*pktp,
			unchar		 entity_id,
			int		 arq )
{
	READ_DEVCAP_REQ	*ce_ptr = (READ_DEVCAP_REQ *)&ccbp->ccb_ele;

	bzero((caddr_t)ce_ptr, sizeof (ce_ptr->gen_req.gen_ctl));

	ce_ptr->gen_req.gen_ctl.length = sizeof(READ_DEVCAP_REQ);
	ce_ptr->gen_req.gen_ctl.opcode = READ_DEV_CAPACITY;
	ce_ptr->gen_req.gen_ctl.eid = REQUEST;
	ce_ptr->gen_req.gen_ctl.DESTID.dest_ent_id   = entity_id;
	ce_ptr->gen_req.gen_ctl.DESTID.dest_unit_id  = ADP_UID;
	ce_ptr->gen_req.gen_ctl.srcid  = 0;
	ce_ptr->gen_req.gen_ctl.corrid  = (ulong)ccbp;

	ce_ptr->gen_req.flag1 = DIR | NODISCON;
	ce_ptr->gen_req.specific1 = 0;
	ce_ptr->gen_req.flag2 = 0;
	ce_ptr->gen_req.time_out_cnt = 0;

	/* use the arq buffer to receive the response */
	ce_ptr->buf_addr = (ulong)(ccbp->ccb_paddr +
			   ((caddr_t)&ccbp->ccb_sense.sts_sensedata -
			   (caddr_t)ccbp));
	ce_ptr->buf_size = 8;

	/* tagged queuing is not yet supported */
	ce_ptr->gen_req.flag1 |= UNTAG_Q;

	/* check if arq supported by target */
	if (arq) {
		ce_ptr->gen_req.flag1 |= ARQ;
		ce_ptr->arq_addr = (ulong)(ccbp->ccb_paddr +
				((caddr_t)(&ccbp->ccb_sense.sts_sensedata) -
				(caddr_t)ccbp));
		ce_ptr->arq_len = sizeof ccbp->ccb_sense.sts_sensedata;
	}
	return;
}


#ifdef CORV_DEBUG

static void
dump_pkt( struct scsi_pkt *pktp )
{
	PRF("pkt_ha_private	= %x\t", pktp->pkt_ha_private);
	PRF("a_hba_tran		= %x\t", pktp->pkt_address.a_hba_tran);
	PRF("a_target 		= %x\t", pktp->pkt_address.a_target);
	PRF("address.a_lun 	= %x\t", pktp->pkt_address.a_lun);
	PRF("address.a_sublun 	= %x\t", pktp->pkt_address.a_sublun);
	PRF("private 		= %x\t", pktp->pkt_private);
	PRF("comp 		= %x\t", pktp->pkt_comp);
	PRF("flags		= %x\t", pktp->pkt_flags);
	PRF("time 		=  %x\t", pktp->pkt_time);
	PRF("scbp 	 	= %x\t", pktp->pkt_scbp);
	PRF("cdbp 	 	= %x\t", pktp->pkt_cdbp);
	PRF("resid 	 	= %x\t", pktp->pkt_resid);
	PRF("state 	 	= %x\t", pktp->pkt_state);
	PRF("statistics 	= %x\t", pktp->pkt_statistics);
	PRF("reason  		= %x\t", pktp->pkt_reason);
}

static void
dump_cmd( struct scsi_cmd *cmdp )
{
	struct scsi_pkt *pktp = &cmdp->cmd_pkt;
	int	i;

 	for(i=0;i<PKT_PRIV_LEN;i++)
		PRF("cmd_pkt_private[%d] = %x\t", i, cmdp->cmd_pkt_private[i]);
}

static void
dump_ele( ELEMENT *ce_ptr )
{
	ushort	*temp = (ushort *)ce_ptr;
	int i;
	int len;

	len = sizeof(*ce_ptr);

	PRF("eid %x cmd %x\n", ((ELE_HDR *)ce_ptr)->eid
			     , ((ELE_HDR *)ce_ptr)->opcode);
	for(i=0;i<(len/2);i+=2) {

		PRF("[%d] = 0x%x [%d] = 0x%x\n", i+1, temp[i+1], i, temp[i]);
		if (i==24 || i==48)
			corv_delay(50);
	}
}

static void
dump_tsb(	ELEMENT		*ce_ptr,
		CORV_TSB	*tsbp )
{
	PRF("t_endstatus	0x%x\n", tsbp->t_endstatus);
	PRF("t_rtrycnt 		0x%x\n", tsbp->t_rtrycnt);
	PRF("t_resid 		0x%x\n", tsbp->t_resid);
	PRF("t_sg_addr 		0x%x\n", tsbp->t_sg_addr);
	PRF("t_add_sts_len 	0x%x\n", tsbp->t_add_sts_len);
	PRF("t_scsistat 	0x%x\n", tsbp->t_scsistat);
	PRF("t_cmdstatus 	0x%x\n", tsbp->t_cmdstatus);
	PRF("t_deverror		0x%x\n", tsbp->t_deverror);
	PRF("t_cmderror		0x%x\n", tsbp->t_cmderror);
	PRF("t_diag_err_mod 	0x%x\n", tsbp->t_diag_err_mod);
	PRF("t_resv2 		0x%x\n", tsbp->t_resv2);
	PRF("t_resv3 		0x%x\n", tsbp->t_resv3);
	PRF("t_ele_addr 	0x%x\n", tsbp->t_ele_addr);
	PRF("ce_ptr 		0x%x\n", ce_ptr);
}

#endif

static int corv_delay( int secs )
{
	drv_usecwait((secs*1000000));
}



static void
build_read_immediate_req( CORV_CCB *ccbp )
{
	READ_IMMED_REQ  *ce_ptr = (READ_IMMED_REQ *)&ccbp->ccb_ele;

	bzero((caddr_t)ce_ptr, sizeof(*ce_ptr));

	ce_ptr->gen_ctl.length = 16;	/* size of read immed req cmd */
	ce_ptr->gen_ctl.opcode = READ_IMMEDIATE;
	ce_ptr->gen_ctl.expedite = 0;
	ce_ptr->gen_ctl.eid = REQUEST;

	ce_ptr->gen_ctl.DESTID.dest_ent_id   = 0;
	ce_ptr->gen_ctl.DESTID.dest_unit_id  = ADP_UID;

	ce_ptr->gen_ctl.corrid  = (ulong)ccbp;
}

static int
corv_exist(	u_int	 ioaddr,
		unchar	*slotp )
{
	unchar	ioaddr_code;
	int	rc;

	if (ioaddr == 0x3540)
		ioaddr_code = 0x00 | 0x01;
	else if (ioaddr == 0x3548)
		ioaddr_code = 0x02 | 0x01;
	else if (ioaddr == 0x3550)
		ioaddr_code = 0x04 | 0x01;
	else if (ioaddr == 0x3558)
		ioaddr_code = 0x06 | 0x01;
	else if (ioaddr == 0x3560)
		ioaddr_code = 0x08 | 0x01;
	else if (ioaddr == 0x3568)
		ioaddr_code = 0x0A | 0x01;
	else if (ioaddr == 0x3570)
		ioaddr_code = 0x0C | 0x01;
	else if (ioaddr == 0x3578)
		ioaddr_code = 0x0E | 0x01;
	else
		return (FALSE);

	rc = mca_find_slot(CORV_MCA_POSID, MCA_SETUP_102, 0x0F, ioaddr_code
					 , slotp);
	return (rc);
}

void
corv_err( char *fmt, ... )
{
	va_list	ap;

	va_start(ap, fmt);
	vcmn_err(CE_CONT, fmt, ap);
	va_end(ap);
}


static void
corv_doneq_add(	CORV_BLK	*corv_blkp,
		CORV_CCB	*ccbp )
{
	DBG_DQUE(("corv_doneq_add: corv_blkp=0x%x ccbp=0x%x\n"
			, corv_blkp, ccbp));

	if (!corv_blkp->cb_donetail) {
		corv_blkp->cb_doneq = NULL;
		corv_blkp->cb_donetail = &corv_blkp->cb_doneq;
	}

	ccbp->ccb_linkp = NULL;
	*corv_blkp->cb_donetail = ccbp;
	corv_blkp->cb_donetail = &ccbp->ccb_linkp;
	return;
}

static CORV_CCB *
corv_doneq_rm( CORV_BLK *corv_blkp )
{
	CORV_CCB	*ccbp;

	/* pop one off the done queue */
	if ((ccbp = corv_blkp->cb_doneq) != NULL) {
		/* if the queue is now empty fix the tail pointer */
		if ((corv_blkp->cb_doneq = ccbp->ccb_linkp) == NULL)
			corv_blkp->cb_donetail = &corv_blkp->cb_doneq;
		ccbp->ccb_linkp = NULL;
	}
	DBG_DQUE(("corv_doneq_rm: corv_blkp=0x%x ccbp=0x%x\n"
			, corv_blkp, ccbp));
	return (ccbp);
}


#ifdef CORV_QDEBUG

static void cdbg_qmismatch(void) { return; }

static void
cdbg_qdebug(	CORV_BLK	*corv_blkp,
		CORV_CCB	*ccbp,
		ELEMENT		*ep )
{
	ushort	qindex;

	for (qindex = 0; qindex < CDBG_MAX_QSLOTS; qindex++) {
		if (cdbg_qed_ccbs[corv_blkp->cb_mca_slot].ccbp[qindex] != ccbp)
			continue;
		cdbg_qed_ccbs[corv_blkp->cb_mca_slot].qcnt -= 1;
		cdbg_qed_ccbs[corv_blkp->cb_mca_slot].ccbp[qindex] = NULL;
		return;
	}
	DBG_DQDBG(("cdbg_qdebug: 0x%x 0x%x 0x%x 0x%x\n"
				, corv_blkp, ccbp, ep, &ccbp->ccb_ele));
	cdbg_qmismatch();
	return;
}
#endif
