/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)esa.c	1.32	95/12/13 SMI"

#include <sys/scsi/scsi.h>
#include <sys/debug.h>
#include <sys/eisarom.h>
#include <sys/nvm.h>


#include <sys/dktp/him_equ.h>
#include <sys/dktp/esacmd.h>
#include <sys/dktp/esa.h>
#include <sys/dktp/him_scb.h>

/*
 * External references
 */

static int esa_tran_tgt_init(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
static int esa_tran_tgt_probe(struct scsi_device *, int (*)());
static void esa_tran_tgt_free(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
static struct scsi_pkt *esa_tran_init_pkt(struct scsi_address *ap,
	struct scsi_pkt *pkt, struct buf *bp, int cmdlen, int statuslen,
	int tgtlen, int flags, int (*callback)(), caddr_t arg);
static void esa_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);

static int esa_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static int esa_reset(struct scsi_address *ap, int level);
static int esa_capchk(char *cap, int tgtonly, int *cidxp);
static int esa_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static int esa_setcap(struct scsi_address *ap, char *cap, int value, int
    tgtonly);
static struct scsi_pkt *esa_pktalloc(struct scsi_address *ap, int cmdlen,
	int statuslen, int tgtlen, int (*callback)(), caddr_t arg);
static void esa_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt);
static struct scsi_pkt *esa_dmaget(struct scsi_pkt *pkt, struct buf *bp,
	int (*callback)(caddr_t), caddr_t arg);
static int esa_scsi_impl_dmaget(struct scsi_pkt *pkt, struct buf *bp,
    int (*callback)(caddr_t), caddr_t callback_arg, ddi_dma_lim_t *dmalimp);
static void esa_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);
static void esa_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pkt);
static int esa_transport(struct scsi_address *ap, struct scsi_pkt *pktp);

#ifdef ESA_DEBUG
void esa_dump_block(struct him_config_block *esa_blkp);
void esa_dump_scb(struct sequencer_ctrl_block *scbp);
void esa_dump_card(struct esa_card *c);
#endif

#ifdef ESA_DEBUG
static void esa_savescb(struct him_config_block *esa_blkp,
    struct sequencer_ctrl_block *scbp);
static struct sequencer_ctrl_block *esa_retscb(struct him_config_block
*esa_blkp, struct sequencer_ctrl_block *scbp);
#endif

/*
 * Local Function Prototypes
 */
static int esa_cfginit(struct esa *esa);
static void esa_unlink_card(struct esa_card *esa_cardp, dev_info_t *dip);
static int esa_xlate_vec(u_char vec);
static u_int esa_dummy_intr(caddr_t arg);
static u_int esa_intr(caddr_t arg);
static void esa_pollret(struct him_config_block *esa_blkp,
    struct scsi_pkt *pktp);
static void esa_chkstatus(struct him_config_block *esa_blkp,
    struct scsi_pkt *pktp, struct sequencer_ctrl_block *scbp);
void SCBCompleted(struct him_config_block *esa_blkp,
    struct sequencer_ctrl_block *scbp);
static int esa_setup_tran(dev_info_t *dip, struct esa *esap);
static struct esa *esa_alloc_esa(dev_info_t *dip);
static int esa_cardinit(dev_info_t *dip, struct esa *esap);
static void esa_probe_stop_ints(unsigned int ioaddr);
static void esa_enable_int(unsigned int ioaddr);
static void esa_disable_int(unsigned int ioaddr);
static u_int esa_run_callbacks(caddr_t arg);
static struct sequencer_ctrl_block
	*esa_retpkt(struct him_config_block *esa_blkp);

/*
 * External references
 */
extern int	scb_send(struct him_config_block *esa_blkp,
    struct sequencer_ctrl_block *scbp);
extern u_char	scb_findha(u_short);
extern void	scb_getconfig(struct him_config_block *);
extern u_char	scb_initHA(struct him_config_block *);
extern u_char	int_handler(struct him_config_block *);
extern int	scb_special(u_char, struct him_config_block *,
    struct sequencer_ctrl_block *);
extern	int	scb_get_bios_info(u_short, struct bios_info_block *);

extern int eisa_nvm();


/*
 * Local static data
 */
static int esa_pgsz = 0;
static int esa_pgmsk;
static int esa_pgshf;

static kmutex_t esa_global_mutex;

unsigned short esa_vlb_probe = 0;	/* Used for enabling vlb probes */
int esa_nounload = 0;

/*
 * head of the global esa structs
 */
static struct esa_card *esa_hd_crdp = (struct esa_card *)0;

/* save interrupt vectors enabled per possible IRQ		*/
static int esa_interrupts[] = {
	0, 0, 0, 0, 0, 0, 0
};

/*
 * DMA limits for data transfer
 */
static ddi_dma_lim_t esa_dmalim = {
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
	ESA_MAX_DMA_SEGS, /* scatter/gather list length		*/
	0xffffffffU	/* request size				*/
};


#ifdef ESA_DEBUG
#define		DENT	0x0001	/* Display function names on entry	*/
#define		DPKT	0x0002 	/* Display packet data			*/
#define		DDATA	0x0004	/* Display all data			*/
#define		DTEMP	0x0008	/* Display wrt to currrent debugging	*/
#define		DCHN	0x0010	/* Display channel number on fn. entry	*/
#define		DSTUS	0x0020	/* Display interrupt status		*/
#define		DINIT	0x0040	/* Display init data			*/
#define		DTEST	0x0080	/* Display test data			*/
#define		DPROBE	0x0100	/* Display probe data			*/

int	esa_debug = 0;
static void	esa_trace();
int	esa_dotrace = 0;

#endif

static int esa_errs[] = {
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
	CMD_RESET		/* 0x23	SCSI bus reset by other device	*/
};

static int esa_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd,
		void *arg, void **result);
static int esa_identify(dev_info_t *dev);
static int esa_probe(dev_info_t *);
static int esa_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int esa_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

static struct dev_ops	esa_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	esa_getinfo,		/* info */
	esa_identify,		/* identify */
	esa_probe,		/* probe */
	esa_attach,		/* attach */
	esa_detach,		/* detach */
	nulldev,		/* no reset */
	(struct cb_ops *)0,	/* driver operations */
	NULL			/* bus operations */
};

char _depends_on[] = "misc/scsi";

#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,	/* Type of module. This one is a driver */
	"Adaptec 7770 SCSI Host Adapter Driver", /* Name of the module. */
	&esa_ops,	/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modldrv, NULL
};

int
_init(void)
{
	int	status;

#ifdef ESA_DEBUG
	if (esa_debug & DINIT)
	PRF("_init for esa\n");
#endif

	if ((status = scsi_hba_init(&modlinkage)) != 0) {
		return (status);
	}

	mutex_init(&esa_global_mutex, "AHA7770 global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&esa_global_mutex);
	}
	return (status);
}

int
_fini(void)
{
	int	status;
	/* XXX KLUDGE do not unload when forceloaded from DU distribution */
	if (esa_nounload != 0)
		return (1);

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&esa_global_mutex);
	}
	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}


/*ARGSUSED*/

/*
 * The traditional x86 scsi hba xxx_blk structure is the
 * him_config_block structure in the esa driver.
 * The card structure lies behind both esa_blk structures,
 * and is pointed to by esa_blk->eb_cardp.
 *
 * tran_tgt_private points to the esa structure
 * tran_hba_private points to the esa_blk (him_config_block) structure
 */

static int
esa_tran_tgt_init(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	int 	targ;
	int	lun;
	struct 	esa *hba_esap;
	struct 	esa *unit_esap;

	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef ESA_DEBUG
	if (esa_debug & DINIT)
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

	hba_esap = SDEV2ESA(sd);

#ifdef ESA_DEBUG
	if (esa_debug & DINIT) {
		cmn_err(CE_CONT, "esa_tran_tgt_init: <%d,%d> hba_esa %x\n",
		targ, lun, hba_esap);
	}
#endif

	unit_esap = kmem_zalloc(sizeof (struct esa) + sizeof (struct esa_unit),
			KM_SLEEP);

	ASSERT(unit_esap != (struct esa *)0);

	bcopy((caddr_t)hba_esap, (caddr_t)unit_esap, sizeof (*hba_esap));
	unit_esap->e_unitp = (struct esa_unit *)(unit_esap+1);
	unit_esap->e_unitp->eu_lim = esa_dmalim;

	if (TRAN2CARD(hba_tran)->ec_flags & ESA_GT1GIG)
		unit_esap->e_unitp->eu_gt1gig = 1;

	/*
	 * Asymmetry in the cloning process: must dereference pointers in
	 * tran_tgt_free before deallocating the unit
	 */

	hba_tran->tran_tgt_private = unit_esap;

	mutex_enter(&esa_global_mutex);
/*	increment child count in card	*/
	TRAN2CARD(hba_tran)->ec_child++;
/*	increment child count in block	*/
	ESA2BLK(hba_esap)->eb_child++;
	mutex_exit(&esa_global_mutex);

#ifdef ESA_DEBUG
	if (esa_debug & DINIT) {
	cmn_err(CE_CONT, "esa_tran_tgt_init: <%d,%d> esa %x unit %x tran %x\n",
		targ, lun, unit_esap, unit_esap->e_unitp, hba_tran);
	}
#endif
	return (DDI_SUCCESS);
}

/*
 * Note that unlink of card is protected by card->ec_child, incremented in
 * tgt_init and decremented in tgt_free
 * Detach of driver instance (SCSI bus) is protected by
 * block->eb_child, incremented in tgt_init and decremented in tgt_free
 */

int esa_detach_test = 1;

/*ARGSUSED*/
static void
esa_tran_tgt_free(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	struct esa *esa;
	struct esa *esa_unitp;

	esa_unitp = hba_tran->tran_tgt_private;

	mutex_enter(&esa_global_mutex);
	esa = SDEV2ESA(sd);
	ESA2CARD(esa)->ec_child--; /* decrement child count in card	*/
	ESA2BLK(esa)->eb_child--;   /* decrement child count in block	*/
	mutex_exit(&esa_global_mutex);

	kmem_free((caddr_t) esa_unitp, sizeof (struct esa)
		+ sizeof (struct esa_unit));

#ifdef ESA_DEBUG
	if (esa_debug & DINIT) {
		PRF("esa_tran_tgt_free: <%d,%d> esa_unit at %x\n",
		sd->sd_address.a_target, sd->sd_address.a_lun, esa_unitp);
	}
#endif

}


/*ARGSUSED*/
static int
esa_tran_tgt_probe(
	struct scsi_device	*sd,
	int			(*callback)())
{
	int	rval;
#ifdef ESA_DEBUG
	char		*s;
	struct esa	*esa = SDEV2ESA(sd);
#endif

	rval = scsi_hba_probe(sd, callback);

#ifdef ESA_DEBUG
	if (esa_debug & DPROBE) {

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
		cmn_err(CE_CONT, "esa%d: %s target %d lun %d %s\n",
			ddi_get_instance(ESA_DIP(esa)),
			ddi_get_name(sd->sd_dev),
			sd->sd_address.a_target,
			sd->sd_address.a_lun, s);
	}
#endif	/* ESA_DEBUG */

	return (rval);
}

/*
 *	Autoconfiguration routines
 */

/*ARGSUSED*/
static int
esa_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	return (DDI_FAILURE);
}

static int
esa_identify(dev_info_t *dip)
{
	char *dname = ddi_get_name(dip);

	if (strcmp(dname, "esa") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}


int
eisa_probe(unsigned int ioaddr)
{
	unsigned long board_id = (HA_PRODUCT<<16)|HA_ID;
	char buf[sizeof(short) + sizeof(NVM_SLOTINFO) + sizeof(NVM_FUNCINFO)];
	int nram;

	nram = eisa_nvm(buf, EISA_SLOT | EISA_CFUNCTION | EISA_BOARD_ID,
		ioaddr >> 12, 0,  board_id, 0x00FFFFFF);
	if (!nram)
		return (DDI_FAILURE);
	return (DDI_SUCCESS);

}

/*
 * Because 1) interrupts must be turned off after probe and on after attach,
 * 2) we cannot turn interrupts off on a card with another channel running,
 * and 3) probe cannot save state, we must require probe and attach to be
 * called for channel A before Channel B
 */

static int
esa_probe(register dev_info_t *dip)
{
	unsigned int ioaddr, channel;
	int len;
	unchar status;

	len = sizeof (int);
	if (ESA_INTPROP(dip, "ioaddr", &ioaddr, &len) != DDI_PROP_SUCCESS)
		return (DDI_PROBE_FAILURE);

	if (ESA_INTPROP(dip, "channel", &channel, &len) != DDI_PROP_SUCCESS)
		return (DDI_PROBE_FAILURE);

#ifdef ESA_DEBUG
	if (esa_debug & DINIT)
		cmn_err(CE_WARN, "esa_probe for ioaddr %x chan %x",
			ioaddr, channel);
#endif

	if ((eisa_probe(ioaddr)) == DDI_FAILURE) {
		/* not a 274x eisa card in this ioaddr */
		/* check whether or not we want to probe vlb at this ioaddr */
		if (!(esa_vlb_probe & (0x1 << (ioaddr >> 12)))) {
			cmn_err(CE_CONT,
				"?esa:probe vlb slot %x is disabled\n",
				ioaddr >> 12);
			return (DDI_PROBE_FAILURE);
		}
	}

	mutex_enter(&esa_global_mutex);
	status = scb_findha(ioaddr);
	if (channel == 0) {
		if (status & 3) {
			esa_probe_stop_ints(ioaddr);
			mutex_exit(&esa_global_mutex);
			return (DDI_PROBE_SUCCESS);
		}
		mutex_exit(&esa_global_mutex);
		return (DDI_PROBE_FAILURE);
	}

	/*
	 * Next test assumes that there can only be 2 channels.  Needs to
	 * be cleaned up when driver is changed so that second channel
	 * is channel 1 NOT channel 8.
	 */

/*	This code assumes that channel 0 must probe before 1		*/
	if ((status & 3) > 1) {
		if (channel == 0)
			esa_probe_stop_ints(channel);
		mutex_exit(&esa_global_mutex);
		return (DDI_PROBE_SUCCESS);
	}

	mutex_exit(&esa_global_mutex);
	return (DDI_PROBE_FAILURE);
}

static void
esa_probe_stop_ints(unsigned int ioaddr)
{
	struct esa_card *this;

	this = esa_hd_crdp;
	for (;;) {
		if ((this == (struct esa_card *)0) ||
			(this->ec_ioaddr == ioaddr))
			break;
		this = this->ec_next;
	}

	if (this) {
#ifdef ESA_DEBUG
		if (esa_debug & DINIT)
			cmn_err(CE_WARN,
			"esa_probe_stop_ints: found initialized card at %x\n",
				ioaddr);
#endif
		return;
	}

#ifdef ESA_DEBUG
	if (esa_debug & DINIT)
		cmn_err(CE_WARN, "esa_probe_stop_ints: will idle card at %x\n",
			ioaddr);
#endif
	esa_disable_int(ioaddr);
}

static void
esa_disable_int(unsigned int ioaddr)
{
	unsigned int cntrl_ioaddr;

	cntrl_ioaddr = ioaddr + HCNTRL + EISA_HOST;
	outb(cntrl_ioaddr, inb(cntrl_ioaddr) & ~INTEN);
}

static void
esa_enable_int(unsigned int ioaddr)
{
	unsigned int cntrl_ioaddr;

	cntrl_ioaddr = ioaddr + HCNTRL + EISA_HOST;
	outb(cntrl_ioaddr, inb(cntrl_ioaddr) | INTEN);
}

static int
esa_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	register struct	him_config_block *esa_blkp;

	switch (cmd) {
	case DDI_DETACH:
	{
		scsi_hba_tran_t	*tran;

/*		tran saved by scsi_hba_attach (3rd param) called in attach */
		tran = (scsi_hba_tran_t *) ddi_get_driver_private(dip);
		if (!tran)
			return (DDI_SUCCESS);

		esa_blkp = TRAN2BLK(tran);
		if (!esa_blkp)
			return (DDI_SUCCESS);

		mutex_enter(&esa_global_mutex);
		/* check if there are any active children */
		if (esa_blkp->eb_child) {
#ifdef ESA_DEBUG
			if (esa_debug & DINIT) {
				cmn_err(CE_WARN,
				"esa_detach: blk %x failure with %d children",
				esa_blkp, esa_blkp->eb_child);
			}
#endif
			mutex_exit(&esa_global_mutex);
			return (DDI_FAILURE);
		}

		ddi_remove_softintr(esa_blkp->eb_softid);

		/*
		 * esa_unlink_card frees the card and him_data_block structs,
		 * removes the interrupt and destroys the mutex
		 */
		esa_unlink_card(BLK2CARD(esa_blkp), dip);
		mutex_exit(&esa_global_mutex);

		(void) kmem_free((caddr_t)TRAN2ESA(tran), sizeof (struct esa) +
			sizeof (struct him_config_block));

		scsi_hba_tran_free(tran);

		ddi_prop_remove_all(dip);
		if (scsi_hba_detach(dip) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "esa: scsi_hba_detach failed\n");
		}

#ifdef ESA_DEBUG
		if (esa_debug & DINIT)
			cmn_err(CE_WARN, "esa_detach success");
#endif

		return (DDI_SUCCESS);
	}
	default:
		return (DDI_FAILURE);
	}
}



/*
 * esa_unlink_card is always called with the esa global mutex held
 */
static void
esa_unlink_card(register struct esa_card *esa_cardp, dev_info_t *dip)
{
	struct esa_card *this, *prev;

	if (esa_cardp->ec_child) {
#ifdef ESA_DEBUG
		if (esa_debug & DINIT)
			cmn_err(CE_WARN,
				"esa_unlink_card: %d children still there",
			esa_cardp->ec_child);
#endif
		return;
	}

/*	locate this esa_card in the linked list				*/
	prev = (struct esa_card *)0;
	this = esa_hd_crdp;
	for (;;) {
		if (this == (struct esa_card *)0)
			break;

		if (this->ec_ioaddr == esa_cardp->ec_ioaddr)
			break;
		else {
			prev = this;
			this = esa_cardp->ec_next;
		}
	}

	if (this) {
/*		take this esa_card off the linked list			*/
		if (prev) {
			if (this->ec_next)
				prev->ec_next = this->ec_next;
			else
				prev->ec_next = (struct esa_card *)0;
		} else
			if (this->ec_next)
				esa_hd_crdp = this->ec_next;
			else
				esa_hd_crdp = (struct esa_card *)0;

/*		disable interrupts on the board				*/
		esa_disable_int(esa_cardp->ec_ioaddr);

/*		XXX this may be a bug in other drivers			*/
/*		make sure we don't remove vector if used by another card */
		if (esa_interrupts[esa_cardp->ec_intr_idx] == 1) {
			ddi_remove_intr(dip, esa_cardp->ec_intr_idx,
				esa_cardp->ec_iblock);
			esa_interrupts[esa_cardp->ec_intr_idx]--;
		}
		mutex_destroy(&esa_cardp->ec_mutex);
		kmem_free((caddr_t)esa_cardp, sizeof (struct esa_card));
		ddi_iopb_free((caddr_t)esa_cardp->ec_datap);
	}
#ifdef ESA_DEBUG
	else {
		if (esa_debug & DINIT)
			cmn_err(CE_WARN,
				"esa_unlink_card: esa_card list corrupt");
	}
#endif

}

/*
 * Allocate an esa structure, an esa_blk (him_config_block) structure
 * and call scsi_hba_tran_alloc to allocate a scsi_tranport structure
 */
static struct esa *
esa_alloc_esa(dev_info_t *dip)
{
	register struct esa *esa;

	esa = (struct esa *) kmem_zalloc(sizeof (struct esa) +
			sizeof (struct him_config_block), KM_SLEEP);
	if (!esa)
		return ((struct esa *)0);

	esa->e_blkp = (struct him_config_block *) (esa + 1);
	esa->e_blkp->eb_dip = dip;

	if ((esa->e_tran = scsi_hba_tran_alloc(dip, 0)) ==
		(scsi_hba_tran_t *)0) {
		kmem_free((caddr_t)esa, sizeof (struct esa) +
			sizeof (struct him_config_block));
		return ((struct esa *)0);
	}

	return (esa);
}

/*ARGSUSED*/
static int
esa_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	register struct esa *esa;

	switch (cmd) {
	case DDI_ATTACH:
		break;

	default:
		return (DDI_FAILURE);
	}

	esa = esa_alloc_esa(dip);
	if (!esa)
		return (DDI_FAILURE);

	if (esa_cardinit(dip, esa) != DDI_SUCCESS) {
		kmem_free((caddr_t)esa, sizeof (struct esa) +
			sizeof (struct him_config_block));
		scsi_hba_tran_free(esa->e_tran);
		return (DDI_FAILURE);
	}

	if (esa_setup_tran(dip, esa) != DDI_SUCCESS)
		return (DDI_FAILURE);

	if (ddi_add_softintr(dip, DDI_SOFTINT_LOW,
		&esa->e_blkp->eb_softid, 0, 0, esa_run_callbacks,
		(caddr_t)esa->e_blkp) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "esa)attach: cannot add softintr\n");
		return (DDI_FAILURE);
	}

/*	turn on interrupts						*/
	mutex_enter(&esa_global_mutex);
	esa_enable_int(ESA2CARD(esa)->ec_ioaddr);
	mutex_exit(&esa_global_mutex);

#ifdef ESA_DEBUG
	if (esa_debug & DINIT)
		PRF("esa_attach: card %x esa %x blk %x\n",
			ESA2CARD(esa), esa, esa->e_blkp);
#endif
	return (DDI_SUCCESS);
}

static int
esa_setup_tran(dev_info_t *dip, struct esa *esap)
{
	register scsi_hba_tran_t	*hba_tran;

	hba_tran = esap->e_tran;

/*	tgt_private always points to the esa structure			*/
	hba_tran->tran_tgt_private	= esap;

/*	hba_private always points to the esa_blk (him_config) struct	*/
	hba_tran->tran_hba_private	= esap->e_blkp;

	hba_tran->tran_tgt_init		= esa_tran_tgt_init;
	hba_tran->tran_tgt_probe	= esa_tran_tgt_probe;
	hba_tran->tran_tgt_free		= esa_tran_tgt_free;

	hba_tran->tran_start 		= esa_transport;
	hba_tran->tran_abort		= esa_abort;
	hba_tran->tran_reset		= esa_reset;
	hba_tran->tran_getcap		= esa_getcap;
	hba_tran->tran_setcap		= esa_setcap;
	hba_tran->tran_init_pkt 	= esa_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= esa_tran_destroy_pkt;
	hba_tran->tran_dmafree		= esa_dmafree;
	hba_tran->tran_sync_pkt		= esa_sync_pkt;

	if (scsi_hba_attach(dip, &esa_dmalim, hba_tran,
			SCSI_HBA_TRAN_CLONE, NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "esa_attach: scsi_hba_attach fail");

		mutex_enter(&esa_global_mutex);
		esa_unlink_card(ESA2CARD(esap), dip);
		mutex_exit(&esa_global_mutex);

		kmem_free((caddr_t)esap, sizeof (struct esa) +
			sizeof (struct him_config_block));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);

	return (DDI_SUCCESS);
}
/*
 * The fact that esa_probe cannot save state requires that probe and
 * attach be called for Channel A before Channel B. Here we rely
 * on this ordering.
 */
static int
esa_cardinit(dev_info_t *dip, struct esa *esap)
{
	int i, len, channel;
	unsigned int ioaddr;
	struct esa_card *esa_cardp, *esa_prev_cardp;
	struct him_config_block *esa_blkp = esap->e_blkp;
#ifdef ESA_DEBUG
	u_char status;
#endif

	len = sizeof (int);
	if (ESA_INTPROP(dip, "ioaddr", &ioaddr, &len) != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);

	if (ESA_INTPROP(esa_blkp->eb_dip, "channel", &channel, &len)
		!= DDI_PROP_SUCCESS) {
		return (DDI_FAILURE);
	}

	if (!(esa_hd_crdp)) {
		esa_pgsz = ddi_ptob(esa_blkp->eb_dip, 1L);
		esa_pgmsk = esa_pgsz - 1;
		for (i = esa_pgsz, len = 0; i > 1; len++)
			i >>= 1;
		esa_pgshf = len;
	}

/*	is there already a card at this io address			*/
	mutex_enter(&esa_global_mutex);
	esa_prev_cardp = esa_cardp = esa_hd_crdp;
	for (;;) {
		if ((!esa_cardp) ||
			(esa_cardp->ec_ioaddr == ioaddr))
			break;
		esa_prev_cardp = esa_cardp;
		esa_cardp = esa_cardp->ec_next;
	}

	if (!esa_cardp) {

		esa_cardp = (struct esa_card *)
			kmem_zalloc(sizeof (struct esa_card),
				KM_SLEEP);
		if (!esa_cardp) {
			mutex_exit(&esa_global_mutex);
			return (DDI_FAILURE);
		}

/*		put this new card on the list of cards for this driver	*/
		if (esa_prev_cardp)
			esa_prev_cardp->ec_next = esa_cardp;
		else
			esa_hd_crdp = esa_cardp;
		esa_blkp->eb_cardp = esa_cardp;
		esa_blkp->eb_mutex = &esa_cardp->ec_mutex;
		esa_cardp->ec_ioaddr = ioaddr;

		if (channel == A_CHANNEL) {
			esa_blkp->Cfe_SCSIChannel = A_CHANNEL;
			esa_cardp->ec_blkp = esa_blkp;
			esa_blkp->Cfe_PortAddress = ioaddr;
			if (esa_cfginit(esap) != DDI_SUCCESS) {
				if (esa_prev_cardp)
					esa_prev_cardp->ec_next =
					(struct esa_card *)0;
				else
					esa_hd_crdp = (struct esa_card *)0;
				mutex_exit(&esa_global_mutex);
				kmem_free((caddr_t)esa_cardp,
					sizeof (struct esa_card));
				return (DDI_FAILURE);
			}
		}
#ifdef ESA_DEBUG
		else {
			if (esa_debug & DINIT)
				cmn_err(CE_WARN, "Channel A must come first");
			return (DDI_FAILURE);
		}
#endif

	} else {
/*		there is already a card structure, so deal with channel B */
		if (channel != B_CHANNEL) {
			mutex_exit(&esa_global_mutex);
#ifdef ESA_DEBUG
			if (esa_debug & DINIT)
				cmn_err(CE_WARN,
				"esa_cardinit: card already has channel A");
#endif
			return (DDI_FAILURE);
		}

		esa_blkp->Cfe_SCSIChannel = B_CHANNEL;
		esa_blkp->eb_cardp = esa_cardp;
		esa_blkp->Cfe_PortAddress = esa_cardp->ec_ioaddr;
		esa_blkp->Cfe_HaDataPtr = esa_cardp->ec_datap;
		esa_blkp->eb_mutex = &esa_cardp->ec_mutex;

/*		restore him_data_block address from Channel A		*/
		esa_blkp->Cfe_ScbParam.Prm_HimDataPhysaddr =
			esa_cardp->ec_him_data_paddr;

		scb_getconfig(esa_blkp);

#ifdef ESA_DEBUG
		if (esa_debug & DINIT) {
		PRF("esa_cardinit:B IO Addr %x Chan %d IRQ %x DataSize %x\n",
		esa_blkp->Cfe_PortAddress, esa_blkp->Cfe_SCSIChannel,
		esa_blkp->Cfe_IrqChannel,
			esa_blkp->Cfe_ScbParam.Prm_HimDataSize);
		PRF("Host id %x option byte [0] %x\n", esa_blkp->Cfe_ScsiId,
			esa_blkp->Cfe_ScsiOption[0]);
		}
#endif

/*		This will have to be changed for 2.5			*/
		esa_disable_int(esa_blkp->Cfe_PortAddress);
#ifdef ESA_DEBUG
		if ((status = scb_initHA(esa_blkp)) != 0) {
			if (esa_debug & DINIT)
				cmn_err(CE_WARN,
				"esa_cardinit: initHA fail %x", status);
			return (DDI_FAILURE);
		} else {
			if (esa_debug & DINIT)
			cmn_err(CE_WARN, "esa_cardinit: initHA OK blk %x",
				esa_blkp);
		}
#else
		if (scb_initHA(esa_blkp)) {
			mutex_exit(&esa_global_mutex);
			return (DDI_FAILURE);
		}
#endif
		drv_usecwait(2 * SEC_INUSEC);
	}

	mutex_exit(&esa_global_mutex);
	return (DDI_SUCCESS);
}

/*
 * This is called with the global mutex held
 */
static int
esa_cfginit(struct esa *esa)
{
	struct him_config_block *esa_blkp = esa->e_blkp;
	struct esa_card *esa_cardp;
	struct him_data_block *him_datap;
	struct bios_info_block *biosp;
	int			intr_idx, i;
	unsigned int him_data_size;
	caddr_t buf;
#ifdef ESA_DEBUG
	int status;
#endif

	esa_cardp = esa_blkp->eb_cardp;
	esa_cardp->ec_blkp = esa_blkp;
/*
 * In the new him version, call scb_getconfig before allocating him_data_block
 */
	scb_getconfig(esa_blkp);

	him_data_size = esa_blkp->Cfe_ScbParam.Prm_HimDataSize;

	esa_cardp->ec_him_data_size = esa_blkp->Cfe_ScbParam.Prm_HimDataSize;

#ifdef ESA_DEBUG
	if (esa_debug & DINIT) {
		PRF("esa_cfginit: IO Addr %x Chan %d IRQ %x DataSize %x\n",
		esa_blkp->Cfe_PortAddress, esa_blkp->Cfe_SCSIChannel,
		esa_blkp->Cfe_IrqChannel, esa_cardp->ec_him_data_size);
		PRF("Param.Access %x Host id %x\n",
			esa_blkp->Cfe_ScbParam.Prm_AccessMode,
			esa_blkp->Cfe_ScsiId);
	}
#endif

/*	allocate him_data_block						*/
	if (ddi_iopb_alloc(esa_blkp->eb_dip, &esa_dmalim,
		him_data_size, &buf) == DDI_FAILURE) {
#ifdef ESA_DEBUG
		cmn_err(CE_WARN, "esa_cfginit: him_data_block alloc fail");
#endif
		return (DDI_FAILURE);
	}
	bzero((caddr_t)buf, him_data_size);
	him_datap = (struct him_data_block *) buf;
	esa_cardp->ec_datap = him_datap;


/*	allocate bios_info area						*/
	if (ddi_iopb_alloc(esa_blkp->eb_dip, &esa_dmalim,
		sizeof (struct bios_info_block),
		&buf) == DDI_FAILURE) {
#ifdef ESA_DEBUG
		cmn_err(CE_NOTE, "esa_cfginit: biosinfo alloc fail");
#endif
		ddi_iopb_free((caddr_t) him_datap);
		return (DDI_FAILURE);
	}
	bzero((caddr_t)buf, sizeof (struct bios_info_block));
	biosp = (struct bios_info_block *) buf;

	if (scb_get_bios_info(esa_blkp->Cfe_PortAddress, biosp) == 0) {
		if (biosp->bi_global & BI_GIGABYTE) {
			esa_cardp->ec_flags |= ESA_GT1GIG;
#ifdef ESA_DEBUG
			if (esa_debug & DINIT)
				cmn_err(CE_NOTE,
					"esa_cfginit: bios GT1GIT");
#endif
		}
	}
#ifdef ESA_DEBUG
	else {
		if (esa_debug & DINIT)
			cmn_err(CE_NOTE, "esa_cfginit: get biosinfo fail");
	}
#endif

	ddi_iopb_free((caddr_t)biosp);

	esa_blkp->Cfe_HaDataPtr = esa_cardp->ec_datap;

	esa_blkp->Cfe_ConfigFlags &= ~BIOS_ACTIVE;
	esa_blkp->Cfe_ScbParam.Prm_HimDataPhysaddr =
		ESA_KVTOP(him_datap);
	/*
	 * Force board to use default mode, i.e. Optima for 2840
	 *
	 * esa_blkp->Cfe_ScbParam.Prm_AccessMode = 0;
	 */

#ifdef ESA_DEBUG
	if (esa_debug & DINIT)
		cmn_err(CE_NOTE, "esa_cfginit: him_data_block paddr %x",
		esa_blkp->Cfe_ScbParam.Prm_HimDataPhysaddr);
#endif

		esa_disable_int(esa_blkp->Cfe_PortAddress);
#ifdef ESA_DEBUG
	if ((status = scb_initHA(esa_blkp)) != 0) {
		if (esa_debug & DINIT)
			cmn_err(CE_WARN, "esa_cfginit: initHA fail %x", status);
			ddi_iopb_free((caddr_t) him_datap);
		return (DDI_FAILURE);
	}
#else
	if (scb_initHA(esa_blkp)) {
		ddi_iopb_free((caddr_t) him_datap);
		return (DDI_FAILURE);
	}
#endif
	drv_usecwait(2 * SEC_INUSEC);

/*	scb_initHA may adjust the physical address of the data block	*/
	esa_cardp->ec_him_data_paddr =
		esa_blkp->Cfe_ScbParam.Prm_HimDataPhysaddr;
/*	and also the virtual address					*/
	esa_cardp->ec_datap = esa_blkp->Cfe_HaDataPtr;

#ifdef ESA_DEBUG
	if (esa_debug & DINIT)
	cmn_err(CE_NOTE, "esa_cfginit: him_data_block paddr %x vaddr %x",
		esa_cardp->ec_him_data_paddr, esa_cardp->ec_datap);
#endif

	/*
	 * scb_initHA just enabled interrupts, but we are not ready
	 * to take interrupts, so disable them
	 * this is safe because we hold the global mutex
	 */

	esa_disable_int(esa_blkp->Cfe_PortAddress);

	intr_idx = esa_xlate_vec(esa_blkp->Cfe_IrqChannel);
	if (intr_idx < 0) {
		ddi_iopb_free((caddr_t) him_datap);
		return (DDI_FAILURE);
	}

	BLK2CARD(esa_blkp)->ec_intr_idx = intr_idx;
/*
 * 	Establish initial dummy interrupt handler
 * 	get iblock cookie to initialize mutexes used in the
 * 	real interrupt handler
 */
	if (ddi_add_intr(esa_blkp->eb_dip, intr_idx,
			(ddi_iblock_cookie_t *) &esa_cardp->ec_iblock,
			(ddi_idevice_cookie_t *) 0, esa_dummy_intr,
			(caddr_t) esa)) {
			ddi_iopb_free((caddr_t) him_datap);
		cmn_err(CE_WARN, "esa_cfginit: cannot add dummy intr");
		return (DDI_FAILURE);
	}

	mutex_init(&esa_cardp->ec_mutex, "esa mutex", MUTEX_DRIVER,
			esa_cardp->ec_iblock);

	ddi_remove_intr(esa_blkp->eb_dip, intr_idx,
			esa_cardp->ec_iblock);

	/* Establish real interrupt handler */
	if (ddi_add_intr(esa_blkp->eb_dip, intr_idx,
			(ddi_iblock_cookie_t *) &esa_cardp->ec_iblock,
			(ddi_idevice_cookie_t *) 0, esa_intr,
			(caddr_t) esa)) {
			cmn_err(CE_WARN, "esa_cfginit: cannot add intr");
			ddi_iopb_free((caddr_t) him_datap);
			mutex_destroy(esa_blkp->eb_mutex);
		return (DDI_FAILURE);
	}
	esa_interrupts[intr_idx]++;

	return (DDI_SUCCESS);
}

/* Abort specific command on target device */
/* returns 0 on failure, 1 on success */
static int
esa_abort(struct scsi_address *ap, struct scsi_pkt *pktp)
{
	struct	him_config_block *esa_blkp;
	struct  sequencer_ctrl_block *scbp;
	int 	status;

	esa_blkp = ADDR2BLK(ap);

	mutex_enter(esa_blkp->eb_mutex);

/*	abort last packet transmistted by this controller 		*/
	if (!pktp) {
		scbp = esa_blkp->eb_last_scbp;
	} else {
		scbp = (struct sequencer_ctrl_block *)
			PKT2CMD(pktp)->cmd_private;
	}

	if (scb_special(ABORT_SCB, esa_blkp, scbp))
		status = 0;
	else
		status = 1;

	mutex_exit(esa_blkp->eb_mutex);

	return (status);
}

/* returns 0 on failure, 1 on success */
static int
esa_reset(struct scsi_address *ap, int level)
{

	struct	him_config_block *esa_blkp;
	int ret = 0;

	esa_blkp = ADDR2BLK(ap);

	mutex_enter(esa_blkp->eb_mutex);

	switch (level) {

	case RESET_ALL:
		if (scb_special(SOFT_HA_RESET, esa_blkp,
			(struct sequencer_ctrl_block *) 0) == 0)
			ret = 1;

		drv_usecwait(2*SEC_INUSEC);
		/* Delay 2 seconds for the devices on scsi bus to settle */
		break;

/*	not supported by the controller					*/
	case RESET_TARGET:
		break;

	default:
		break;
	}

	mutex_exit(esa_blkp->eb_mutex);

	return (ret);
}

static int
esa_capchk(char *cap, int tgtonly, int *cidxp)
{
	register int	cidx;

	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *) 0)
		return (FALSE);

	if ((cidx = scsi_hba_lookup_capstr(cap)) == -1)
		return (FALSE);

	*cidxp = cidx;
	return (TRUE);
}

static int
esa_getcap(struct scsi_address *ap, char *cap, int tgtonly)
{
	int	ckey;

	if (esa_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {

		case SCSI_CAP_GEOMETRY:
		{
			int	total_sectors, h, s;

			total_sectors = (ADDR2ESAUNITP(ap))->eu_total_sectors;

			if (((ADDR2ESAUNITP(ap))->eu_gt1gig) &&
				total_sectors > 0x200000) {
				h = 255;
				s = 63;
			} else {
				h = 64;
				s = 32;
			}
			return (ESA_SETGEOM(h, s));

		}
		case SCSI_CAP_ARQ:
			return (TRUE);
		default:
			break;
	}
	return (UNDEFINED);
}

static int
esa_setcap(struct scsi_address *ap, char *cap, int value, int tgtonly)
{
	int	ckey, status = FALSE;

	if (esa_capchk(cap, tgtonly, &ckey) != TRUE) {
		return (UNDEFINED);
	}

	switch (ckey) {
		case SCSI_CAP_SECTOR_SIZE:
			(ADDR2ESAUNITP(ap))->eu_lim.dlim_granular =
				(u_int)value;
			status = TRUE;
			break;

		case SCSI_CAP_ARQ:
			if (tgtonly) {
				(ADDR2ESAUNITP(ap))->eu_arq = (u_int)value;
				status = TRUE;
			}
			break;

		case SCSI_CAP_TOTAL_SECTORS:
			(ADDR2ESAUNITP(ap))->eu_total_sectors = value;
			status = TRUE;
			break;

		case SCSI_CAP_GEOMETRY:
		default:
			break;
	}
	return (status);
}

static struct scsi_pkt *
esa_tran_init_pkt(struct scsi_address *ap, struct scsi_pkt *pkt,
	struct buf *bp, int cmdlen, int statuslen, int tgtlen,
	int flags, int (*callback)(caddr_t), caddr_t arg)
{
	struct scsi_pkt		*new_pkt = (struct scsi_pkt *)0;
	struct esa_scsi_cmd	*cmd;

	/*
	 * Allocate a pkt
	 */
	if (!pkt) {
		pkt = esa_pktalloc(ap, cmdlen, statuslen,
			tgtlen, callback, arg);
		if (pkt == (struct scsi_pkt *)0)
			return ((struct scsi_pkt *)0);
		cmd = PKT2CMD(pkt);
		cmd->cmd_cflags = flags;
		new_pkt = pkt;
	} else {
		new_pkt = (struct scsi_pkt *)0;
	}

	/*
	 * Set up dma info
	 */
	if (bp) {
		if (esa_dmaget(pkt, bp, callback, arg) ==
				(struct scsi_pkt *)0) {
			if (new_pkt)
				esa_pktfree(ap, new_pkt);
			return ((struct scsi_pkt *)0);
		}
	}

	return (pkt);
}

static void
esa_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	esa_dmafree(ap, pkt);
	esa_pktfree(ap, pkt);
}

static struct scsi_pkt *
esa_pktalloc(struct scsi_address *ap, int cmdlen, int statuslen,
    int tgtlen, int (*callback)(), caddr_t arg)
{
	register struct esa_scsi_cmd *cmd;
	register struct scsi_pkt *pkt;
	register struct sequencer_ctrl_block	*scbp;
	struct	 him_config_block	*esa_blkp;
	caddr_t			buf;

	esa_blkp = ADDR2BLK(ap);

/*	request minimum target_privte area from scsi_hba_pkt_alloc	*/
	if (tgtlen <= PKT_PRIV_LEN)
		tgtlen = PKT_PRIV_LEN;

/*	must pass 0 for sizeof status and cdb fields, as these use dma	*/
	pkt = scsi_hba_pkt_alloc(esa_blkp->eb_dip, ap, 0,
		0, tgtlen, sizeof (struct esa_scsi_cmd),
		callback, arg);
	if (pkt == (struct scsi_pkt *)0) {
		return ((struct scsi_pkt *)0);
	}

/*	scsi_hba_pkt_alloc put addr of target_private in pkt_private	*/
/*	and put addr struct esa_scsi_cmd in pkt->pkt_ha_private		*/
/*	it also set pkt->pkt_address					*/

	cmd = PKT2CMD(pkt);
	cmd->cmd_pkt = pkt;

	if (ddi_iopb_alloc(esa_blkp->eb_dip, &esa_dmalim,
			sizeof (struct sequencer_ctrl_block),
				&buf) == DDI_FAILURE) {
#ifdef ESA_DEBUG
		cmn_err(CE_NOTE, "esa: scb alloc failed");
#endif
		scsi_hba_pkt_free(ap, pkt);
		return ((struct scsi_pkt *)0);
	}
	bzero((caddr_t)buf, sizeof (struct sequencer_ctrl_block));
	scbp = (struct sequencer_ctrl_block *) buf;

	cmd->cmd_private = (opaque_t) buf;
/*	cmd_private is used to communicate the completed scb		*/
/*	and to save and free scb memory					*/
/*	in other drivers this area is known as the ccb			*/

/* 	initialize scb 							*/
	scbp->SCB_Cmd = EXEC_SCB;
	scbp->SCB_CDBLen  = (unchar)cmdlen;
	scbp->SCB_cmdp = cmd;

	/*	initialize arq 						*/
	if ((ADDR2ESAUNITP(ap))->eu_arq)  {
		scbp->SCB_Flags = AUTO_SENSE;
	}

/* 	initialize target, lun and channel 				*/
	scbp->SCB_Tarlun = (ap->a_target << 4) | ap->a_lun |
		esa_blkp->Cfe_SCSIChannel;

	scbp->SCB_paddr	  = ESA_KVTOP(scbp);

/* 	auto request sense data physical address 			*/
	scbp->SCB_SensePtr = scbp->SCB_paddr +
		((caddr_t)(&scbp->SCB_sense.sts_sensedata) - (caddr_t)scbp);
	scbp->SCB_SenseLen = ESA_SENSE_LEN;
/* 	set SCSI cdb physical address 					*/
	scbp->SCB_CDBPtr = scbp->SCB_paddr +
		((u_char *)&(scbp->SCB_CDB[0]) - (u_char *)scbp);

	cmd->cmd_cdblen		= (u_char) cmdlen;
	cmd->cmd_scblen		= (u_char) statuslen;
	cmd->cmd_privlen	= sizeof (struct esa_scsi_cmd);

	pkt->pkt_cdbp		= (opaque_t) scbp->SCB_CDB;
	pkt->pkt_scbp		= (u_char *) &scbp->SCB_sense;

#ifdef ESA_DEBUG
	if (esa_debug & DPKT) {
		PRF("esa_pktalloc:cmdpktp %x pkt_cdbp %x pkt_sdbp %x\n",
			cmd, cmd->cmd_pkt->pkt_cdbp, cmd->cmd_pkt->pkt_scbp);
		PRF("scbp %x pktp %x\n", scbp, pkt);
	}
#endif
	return (pkt);
}

/*
 * packet free
 */
static void
esa_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	register struct esa_scsi_cmd *cmd = PKT2CMD(pkt);

	ddi_iopb_free((caddr_t)cmd->cmd_private);
	scsi_hba_pkt_free(ap, pkt);
}

/*
 * Dma resource deallocation
 */
/*ARGSUSED*/
static void
esa_dmafree(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct esa_scsi_cmd *cmd = PKT2CMD(pktp);

/* 	Free the mapping.  						*/
	if (cmd->cmd_dmahandle) {
		if (ddi_dma_free(cmd->cmd_dmahandle) == DDI_FAILURE)
			cmn_err(CE_PANIC, "ddi_dma_free error");
		cmd->cmd_dmahandle = NULL;
	}
}


/*
 * Dma sync
 */
/*ARGSUSED*/
static void
esa_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pktp)
{
	register struct esa_scsi_cmd *cmd = PKT2CMD(pktp);
	int	i;

	if (cmd->cmd_dmahandle) {
		i = ddi_dma_sync(cmd->cmd_dmahandle, 0, 0,
			(cmd->cmd_cflags & CFLAG_DMASEND) ?
			DDI_DMA_SYNC_FORDEV : DDI_DMA_SYNC_FORCPU);
		if (i != DDI_SUCCESS) {
			cmn_err(CE_WARN, "esa: sync pkt fail");
		}
	}
}

/*
 * Dma resource allocation
 */
static struct scsi_pkt *
esa_dmaget(struct scsi_pkt *pktp, struct buf *bp,
	int (*callback)(caddr_t), caddr_t arg)
{

	register struct esa_scsi_cmd *cmd = PKT2CMD(pktp);
	register struct sequencer_ctrl_block *scbp;
	struct esa_sg *dmap;
	ddi_dma_cookie_t dmack;
	ddi_dma_cookie_t *dmackp = &dmack;
	int	cnt;
	int	bxfer;
	off_t	offset;
	off_t	len;

	scbp = (struct sequencer_ctrl_block *)cmd->cmd_private;

	if (!bp->b_bcount) {
		pktp->pkt_resid = 0;
/* 		XXX workaround for scdk_start_drive mistake		*/
		scbp->SCB_SegCnt = 0;
		scbp->SCB_SegPtr = 0;
		return (pktp);
	}

/* 	for read or write						*/
	scbp->SCB_Cntrl |= REJECT_MDP;

/*	setup dma memory and position to the next xfer segment		*/
	if (esa_scsi_impl_dmaget(pktp, bp, callback, arg,
			&(PKT2ESAUNITP(pktp)->eu_lim)) == 0)
		return ((struct scsi_pkt *)0);
	if (ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len,
			dmackp) == DDI_FAILURE)
		cmn_err(CE_PANIC, "ddi_dma_segtocookie error");

	if (bp->b_bcount <= dmackp->dmac_size)
		cmd->cmd_totxfer = 0;

/* 	set address of scatter gather segs 				*/
	dmap = scbp->SCB_sg_list;

/* 	whether or not single block xfer, 7770 wants scatter gather	*/
	for (bxfer = 0, cnt = 1; ; cnt++, dmap++) {
		bxfer += dmackp->dmac_size;

		dmap->data_len = (ulong) dmackp->dmac_size;
		dmap->data_addr = (ulong) dmackp->dmac_address;

#ifdef ESA_DEBUG
		if (esa_debug & DPKT)
		PRF("totxfer %x bxfer %x cnt %x b_bcount %x siz %x paddr %x\n",
			cmd->cmd_totxfer, bxfer, cnt,
			bp->b_bcount, dmackp->dmac_size,
			dmackp->dmac_address);

#endif
/*		check for end of list condition			*/
		if (bp->b_bcount == (bxfer + cmd->cmd_totxfer))
			break;
		ASSERT(bp->b_bcount > (bxfer + cmd->cmd_totxfer));
/* 		check end of physical scatter-gather list limit */
		if (cnt >= (int)ESA_MAX_DMA_SEGS)
			break;
		if (ddi_dma_nextseg(cmd->cmd_dmawin, cmd->cmd_dmaseg,
			&cmd->cmd_dmaseg) != DDI_SUCCESS)
			break;
		if (ddi_dma_segtocookie(cmd->cmd_dmaseg, &offset, &len,
				dmackp) == DDI_FAILURE)
			cmn_err(CE_PANIC, "ddi_dma_segtocookie error");
	}

	scbp->SCB_SegCnt = (unsigned char) cnt;

/* 		physical address of scatter gather list 		*/
	scbp->SCB_SegPtr = (paddr_t)(scbp->SCB_paddr +
		((caddr_t)scbp->SCB_sg_list - (caddr_t)scbp));

	cmd->cmd_totxfer += bxfer;
	pktp->pkt_resid = bp->b_bcount - cmd->cmd_totxfer;

	return (pktp);
}

static int
esa_scsi_impl_dmaget(struct scsi_pkt *pktp, struct buf *bp,
    int (*callback)(caddr_t), caddr_t callback_arg, ddi_dma_lim_t *dmalimp)
{
	register struct esa_scsi_cmd *cmd = PKT2CMD(pktp);
	int 		flags;
	int 		status;

	if (!cmd->cmd_dmahandle) {
		if (bp->b_flags & B_READ)
			flags = DDI_DMA_READ;
		else
			flags = DDI_DMA_WRITE;

		if (cmd->cmd_flags & PKT_CONSISTENT)
			flags |= DDI_DMA_CONSISTENT;
		if (cmd->cmd_flags & PKT_DMA_PARTIAL)
			flags |= DDI_DMA_PARTIAL;

		status = ddi_dma_buf_setup(PKT2TRAN(pktp)->tran_hba_dip,
				bp, flags, callback, callback_arg, dmalimp,
				&cmd->cmd_dmahandle);

		if (status) {
			switch (status) {
			case DDI_DMA_NORESOURCES:
				bp->b_error = 0;
				break;
			case DDI_DMA_TOOBIG:
				bp->b_error = EINVAL;
				break;
			case DDI_DMA_NOMAPPING:
			default:
				bp->b_error = EFAULT;
				break;
			}
			return (0);
		}
	} else {
		/*
		 * get next segment
		 */
		status = ddi_dma_nextseg(cmd->cmd_dmawin, cmd->cmd_dmaseg,
				&cmd->cmd_dmaseg);
		if (status == DDI_SUCCESS)
			return (1);
		else if (status == DDI_DMA_STALE)
			return (0);

		/* fall through at end of segment */
	}

	/*
	 * move to the next window
	 */
	status = ddi_dma_nextwin(cmd->cmd_dmahandle, cmd->cmd_dmawin,
			&cmd->cmd_dmawin);
	if (status == DDI_DMA_STALE)
		return (0);
	if (status == DDI_DMA_DONE) {
		/*
		 * reset to first window
		 */
		if (ddi_dma_nextwin(cmd->cmd_dmahandle, NULL,
		    &cmd->cmd_dmawin) != DDI_SUCCESS)
			return (0);
	}

	/*
	 * get first segment
	 */
	if (ddi_dma_nextseg(cmd->cmd_dmawin, NULL, &cmd->cmd_dmaseg) !=
	    DDI_SUCCESS)
		return (0);
	return (1);
}

/*ARGSUSED*/
static int
esa_transport(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct him_config_block *esa_blkp;
	register struct	sequencer_ctrl_block *scbp;
	struct esa_card *esa_cardp;
	u_char *p;
	int i;

	scbp = (struct sequencer_ctrl_block *)PKT2CMD(pktp)->cmd_private;
	esa_blkp = PKT2BLK(pktp);
	esa_cardp = BLK2CARD(esa_blkp);

#if ESA_DEBUG
	if (esa_debug & DPKT)
		cmn_err(CE_WARN, "esa_tran: chan %x scbp %x blk %x cmd %x",
			esa_blkp->Cfe_SCSIChannel, scbp, esa_blkp,
				PKT2CMD(pktp));
#endif

	mutex_enter(esa_blkp->eb_mutex);

/*	track the last outstanding scb (scsi_pkt) for esa_abort		*/
	esa_blkp->eb_last_scbp = scbp;

/*	save data stolen by him code on error			*/
	for (i = 0, p = pktp->pkt_cdbp; i < MAX_CDB_LEN; i++)
		scbp->SCB_CDB_save[i] = *p++;
	scbp->SCB_CDBLen_save = scbp->SCB_CDBLen;
	scbp->SCB_Cntrl_save = scbp->SCB_Cntrl;
	scbp->SCB_SegCnt_save = scbp->SCB_SegCnt;
	scbp->SCB_SegPtr_save = scbp->SCB_SegPtr;

/*	initialize in case of packet reuse 				*/
	scbp->SCB_TargStat = 0;
	scbp->SCB_HaStat = 0;
	scbp->SCB_ResCnt = 0;

/*	initialize the compleded poll packet to null			*/
	esa_blkp->eb_scbp = (struct sequencer_ctrl_block *)0;

	/* if either polled cmd or "no-disconnect", don't allow disconnects */
	if (pktp->pkt_flags & (FLAG_NOINTR | FLAG_NODISCON))
		scbp->SCB_Cntrl &= ~DIS_ENABLE;
	else
		scbp->SCB_Cntrl |= DIS_ENABLE;

	pktp->pkt_state = 0;
	pktp->pkt_statistics = 0;
	pktp->pkt_resid = 0;

#ifdef ESA_DEBUG
	esa_savescb(esa_blkp, scbp);
#endif

	if (pktp->pkt_flags & FLAG_NOINTR)
		esa_cardp->ec_flags |= ESA_POLLING;

	if (scb_send(esa_blkp, scbp)) {
		esa_cardp->ec_flags &= ~ESA_POLLING;
		mutex_exit(esa_blkp->eb_mutex);
		return (TRAN_BUSY);
	}

#ifdef ESA_DEBUG
	esa_blkp->eb_pkts_out++;
#endif

	if (pktp->pkt_flags & FLAG_NOINTR) {
		esa_pollret(esa_blkp, pktp);
		esa_cardp->ec_flags &= ~ESA_POLLING;
	}

	mutex_exit(esa_blkp->eb_mutex);
	return (TRAN_ACCEPT);
}

/*
 * allows us to run stacked up callbacks beyond the
 * polling loop inside esa_pollret
 */
static u_int
esa_run_callbacks(caddr_t arg)
{
	register struct him_config_block *esa_blkp =
		(struct him_config_block *) arg;
	register struct	sequencer_ctrl_block *scbp;
	struct scsi_pkt *pkt;

	mutex_enter(esa_blkp->eb_mutex);
	if (!esa_blkp->eb_scbp_que) {
		ASSERT(!(esa_blkp->eb_flag & ESA_POLL_TRIGGER_ON));
		mutex_exit(esa_blkp->eb_mutex);
		return (DDI_INTR_UNCLAIMED);
	}
	ASSERT(esa_blkp->eb_flag & ESA_POLL_TRIGGER_ON);

	while (esa_blkp->eb_scbp_que) {
		scbp = esa_retpkt(esa_blkp);
		ASSERT(scbp);

#ifdef ESA_DEBUG
		if (esa_retscb(esa_blkp, scbp) != scbp)
			cmn_err(CE_WARN, "pollret retscb fail scb %x", scbp);
		esa_blkp->eb_pkts_out--;
#endif

		mutex_exit(esa_blkp->eb_mutex);

		pkt = CMD2PKT(scbp->SCB_cmdp);
		esa_chkstatus(esa_blkp, pkt, scbp);
		ASSERT(pkt->pkt_comp);
		(*pkt->pkt_comp)(pkt);
		mutex_enter(esa_blkp->eb_mutex);
	}

	esa_blkp->eb_flag &= ~ESA_POLL_TRIGGER_ON;
	esa_blkp->eb_pkts_done = 0;

	mutex_exit(esa_blkp->eb_mutex);
	return (DDI_INTR_CLAIMED);
}

static void
esa_savepkt(register struct him_config_block *esa_blkp,
	register struct sequencer_ctrl_block *scbp)
{
	register struct	sequencer_ctrl_block *cp;

#ifdef ESA_DEBUG
	if (esa_debug & DPKT)
		cmn_err(CE_WARN, "esa_savepkt scb: %x", scbp);
#endif

	cp = esa_blkp->eb_scbp_que;
	if (!cp) {
		esa_blkp->eb_scbp_que = scbp;
		scbp->scb_pforw = scbp;
		scbp->scb_pback = scbp;
		return;
	}

	cp->scb_pback->scb_pforw = scbp;
	scbp->scb_pback = cp->scb_pback;
	scbp->scb_pforw = cp;
	cp->scb_pback = scbp;
}

static struct sequencer_ctrl_block *
esa_retpkt(register struct him_config_block *esa_blkp)
{
	register struct	sequencer_ctrl_block *cp;
	register struct	sequencer_ctrl_block *scbp;

	cp = esa_blkp->eb_scbp_que;

/*	empty list						*/
	if (!cp) {
		return ((struct sequencer_ctrl_block *)0);
	}

/*	single entry on the list				*/
	if (cp == cp->scb_pforw) {
		esa_blkp->eb_scbp_que =
			(struct sequencer_ctrl_block *)0;
		return (cp);
	}

	if (esa_blkp->eb_scbp_que == cp)
			esa_blkp->eb_scbp_que = cp->scb_pforw;
	cp->scb_pback->scb_pforw = cp->scb_pforw;
	cp->scb_pforw->scb_pback = cp->scb_pback;
	return (cp);
}

/*
 * esa_savescb() and esa_retscb() are for debugging purposes only
 */
#ifdef ESA_DEBUG
static void
esa_savescb(register struct him_config_block *esa_blkp,
	register struct sequencer_ctrl_block *scbp)
{
	register struct	sequencer_ctrl_block *cp;

	cp = esa_blkp->eb_scboutp;
	if (!cp) {
		esa_blkp->eb_scboutp = scbp;
		scbp->scb_forw = scbp;
		scbp->scb_back = scbp;
		return;
	}

	cp->scb_back->scb_forw = scbp;
	scbp->scb_back = cp->scb_back;
	scbp->scb_forw = cp;
	cp->scb_back = scbp;
}

static struct sequencer_ctrl_block *
esa_retscb(register struct him_config_block *esa_blkp,
	register struct sequencer_ctrl_block *scbp)
{
	register struct	sequencer_ctrl_block *cp;

	for (cp = esa_blkp->eb_scboutp; cp; ) {
		if (cp != scbp) {
			cp = cp->scb_forw;
			ASSERT(cp != esa_blkp->eb_scboutp);
			continue;
		}
/*		check for single entry on the list			*/
		if (cp == cp->scb_forw) {
			esa_blkp->eb_scboutp = NULL;
			return (cp);
		}

/*		check for first entry on the list			*/
		if (esa_blkp->eb_scboutp == cp)
			esa_blkp->eb_scboutp = cp->scb_forw;
		cp->scb_back->scb_forw = cp->scb_forw;
		cp->scb_forw->scb_back = cp->scb_back;
		return (cp);

	}
	cmn_err(CE_WARN, "esa_retscb: NO match");
}

#endif

/*
 * esa_pollret is always called with the mutex held
 * int_handler is used to poll for completion because
 * it transfers control to the HIM SCSI state machine
 *
 * polled packets are single threaded by control of the mutex
 * and are not allowed to disconnect
 */
static void
esa_pollret(register struct him_config_block *esa_blkp,
		register struct scsi_pkt *pktp)
{
	register struct sequencer_ctrl_block *scbp;
	struct	esa_scsi_cmd *cmd;
	struct	esa_scsi_cmd *cmd_hdp = (struct  esa_scsi_cmd *)0;
	int poll_done = FALSE;
	ushort pkts_done = esa_blkp->eb_pkts_done;
	int i, j;
#ifdef ESA_DEBUG
	u_char OMgrStat, status;
#endif

	scbp = (struct sequencer_ctrl_block *)
		PKT2CMD(pktp)->cmd_private;

#if ESA_DEBUG
	if (esa_debug & DPKT) {
		OMgrStat = 0xff;
		PRF("esa_pollret: Chan %d scbp %x pktp %x \n",
			esa_blkp->Cfe_SCSIChannel, scbp, pktp);
	}
#endif

	i = 50;

	while (i > 0) {

		for (j = 0; j < 1000; j++)
			drv_usecwait(10);

#ifdef ESA_DEBUG
		status = int_handler(esa_blkp);

		if (esa_debug & DPKT && OMgrStat != scbp->SCB_MgrStat)
			cmn_err(CE_WARN,
				"pollret:[%x] int_handler stat %x Mgr stat %x",
				i, status, scbp->SCB_MgrStat);
			OMgrStat = scbp->SCB_MgrStat;
#else
		(void) int_handler(esa_blkp);

#endif
		if (scbp == esa_blkp->eb_scbp) {
			poll_done = TRUE;
			esa_blkp->eb_scbp =
				(struct sequencer_ctrl_block *)0;
			break;
		}

/*		a non-poll packet has finished scsi phases	*/
		if (esa_blkp->eb_pkts_done > pkts_done) {
			pkts_done = esa_blkp->eb_pkts_done;
			continue;
		}

		i--;
	}

	if (poll_done == TRUE)
		esa_chkstatus(esa_blkp, pktp, scbp);
	else {
		(void) scb_special(ABORT_SCB, esa_blkp, scbp);
		pktp->pkt_reason   = CMD_INCOMPLETE;
		scbp->SCB_SegCnt = 0;
		scbp->SCB_SegPtr = 0;
		for (i = 0; i < sizeof (scbp->SCB_RsvdX); i++) {
			scbp->SCB_RsvdX[i] = 0;
		}
#ifdef ESA_DEBUG
		if (esa_debug & DPKT)
			cmn_err(CE_WARN, "pollret timeout pkt %x scb %x",
			pktp, scbp);
#endif
	}

#ifdef ESA_DEBUG
	if (esa_retscb(esa_blkp, scbp) != scbp)
		cmn_err(CE_WARN, "pollret retscb fail scb %x", scbp);
	esa_blkp->eb_pkts_out--;
#endif

}

/*ARGSUSED*/
static void
esa_chkstatus(struct him_config_block *esa_blkp,
	register struct scsi_pkt *pktp,
		register struct sequencer_ctrl_block *scbp)
{
	struct	scsi_arq_status *arqp;
	int	i;
	u_char	*p;

	ASSERT(scbp->SCB_HaStat < ESA_UNKNOWN_ERROR);

	pktp->pkt_state  = 0;
	*pktp->pkt_scbp  = scbp->SCB_TargStat;
	pktp->pkt_reason   = esa_errs[scbp->SCB_HaStat];


#ifdef ESA_DEBUG
	if (esa_debug & DPKT)
		cmn_err(CE_WARN, "cmd %x t %x l %x tstat %x reason %x",
		(*(pktp->pkt_cdbp)),
		((scbp->SCB_Tarlun >> 4) & 0xF), scbp->SCB_Tarlun & LUN,
		scbp->SCB_TargStat,
		pktp->pkt_reason);
#endif

	if (pktp->pkt_reason == CMD_CMPLT &&
		scbp->SCB_TargStat == UNIT_GOOD) {

		pktp->pkt_resid = 0;
		pktp->pkt_state = STATE_XFERRED_DATA | STATE_GOT_BUS |
		    STATE_GOT_TARGET | STATE_SENT_CMD | STATE_GOT_STATUS;
		scbp->SCB_SegCnt = 0;
		scbp->SCB_SegPtr = 0;
		for (i = 0; i < sizeof (scbp->SCB_RsvdX); i++)
			scbp->SCB_RsvdX[i] = 0;
		return;
	}

	if (scbp->SCB_TargStat == STATUS_CHECK &&
		scbp->SCB_Flags & AUTO_SENSE) {

		if (scbp->SCB_HaStat != HOST_SNS_FAIL) {
			pktp->pkt_reason   = CMD_CMPLT;
			pktp->pkt_state  |=
			(STATE_GOT_BUS|STATE_GOT_TARGET|STATE_SENT_CMD|
			STATE_GOT_STATUS|STATE_ARQ_DONE);

			arqp = (struct scsi_arq_status *)pktp->pkt_scbp;
			arqp->sts_rqpkt_reason = CMD_CMPLT;
/*			XXX need valid data here			*/
			arqp->sts_rqpkt_resid  =  0;
			arqp->sts_rqpkt_state |= STATE_XFERRED_DATA;

		} else {
/*			pkt_reason has been set to CMD_TRAN_ERR		*/
			cmn_err(CE_WARN,
				"esa_chkstatus: auto request sense fail");
		}
	}


	switch (scbp->SCB_HaStat) {

		case  HOST_DU_DO:
#ifdef ESA_DEBUG
			if (scbp->SCB_ResCnt == 0) {
				if (esa_debug & DSTUS)
					cmn_err(CE_WARN, "overrun");
			} else {
#else
			if (scbp->SCB_ResCnt != 0) {

#endif
/*				underrun per Frits of scsi-steer	*/
				pktp->pkt_reason   = CMD_CMPLT;
				pktp->pkt_resid  = scbp->SCB_ResCnt;
#ifdef ESA_DEBUG
				if (esa_debug & DSTUS)
					cmn_err(CE_WARN, "underrun: resid %x",
						pktp->pkt_resid);
#endif
			}

			pktp->pkt_state =
			(STATE_XFERRED_DATA|STATE_GOT_BUS|
			STATE_GOT_TARGET|STATE_SENT_CMD|STATE_GOT_STATUS);
			break;

		case  HOST_SEL_TO:
			pktp->pkt_statistics |= STAT_TIMEOUT;
			pktp->pkt_state |= STATE_GOT_BUS;
#ifdef ESA_DEBUG
			if (esa_debug & DSTUS)
				cmn_err(CE_WARN, "esa: selection timeout");
#endif
			break;

		case  HOST_ABT_HA:
		case  HOST_ABT_HOST:
#ifdef ESA_DEBUG
			if (esa_debug & DSTUS)
				cmn_err(CE_WARN, "esa: cmd aborted");
#endif

			pktp->pkt_statistics |= STAT_ABORTED;
			break;

		case  HOST_RST_HA:
		case  HOST_RST_OTHER:
#ifdef ESA_DEBUG
			if (esa_debug & DSTUS)
				cmn_err(CE_WARN, "esa: bus reset");
#endif
			pktp->pkt_statistics |= STAT_DEV_RESET;
			break;

		default:
			break;
	}

#ifdef ESA_DEBUG
	if (esa_debug & DSTUS)
	PRF("chkstat2: reason %x host stat %x targ stat %x cmd %x chan %d\n",
			pktp->pkt_reason, scbp->SCB_HaStat, scbp->SCB_TargStat,
			(*(pktp->pkt_cdbp)), esa_blkp->Cfe_SCSIChannel);
#endif

/*	restore the cdb in case it was re-used by him code on error	*/
	for (i = 0, p = pktp->pkt_cdbp; i < MAX_CDB_LEN; i++)
		*p++ = scbp->SCB_CDB_save[i];
/*	restore scb as well						*/
	for (i = 0; i < sizeof (scbp->SCB_RsvdX); i++)
		scbp->SCB_RsvdX[i] = 0;
	scbp->SCB_CDBLen = scbp->SCB_CDBLen_save;
	scbp->SCB_Cntrl = scbp->SCB_Cntrl_save;
	scbp->SCB_SegCnt = scbp->SCB_SegCnt_save;
	scbp->SCB_SegPtr = scbp->SCB_SegPtr_save;
}

/* Autovector Interrupt Entry Point */
/* Dummy return to be used before mutexes has been initialized		*/
/* guard against interrupts from drivers sharing the same irq line	*/
/*ARGSUSED*/
static u_int
esa_dummy_intr(caddr_t arg)
{
	return (DDI_INTR_UNCLAIMED);
}

static u_int
esa_intr(caddr_t arg)
{
	struct him_config_block *esa_blkp;
	unsigned char status;

	esa_blkp = ESA_BLKP(arg);

	mutex_enter(esa_blkp->eb_mutex);
	status = int_handler(esa_blkp);
	mutex_exit(esa_blkp->eb_mutex);

	if (status & 0x70) {
		return (DDI_INTR_CLAIMED);
	} else {
		return (DDI_INTR_UNCLAIMED);
	}
}

/*
 * This function is called from int_handler(HIM layer) for a completed command.
 * the controller mutex is held
 */
void
SCBCompleted(register struct him_config_block *esa_blkp,
	register struct sequencer_ctrl_block *scbp)
{
	register struct scsi_pkt *pkt = CMD2PKT(scbp->SCB_cmdp);
	struct esa_card *esa_cardp = BLK2CARD(esa_blkp);

/* 	just save the pointer to the polled scb that completed 		*/
	if (pkt->pkt_flags & FLAG_NOINTR) {

#ifdef ESA_DEBUG
		if (esa_debug &DPKT)
			cmn_err(CE_WARN, "SCBComplete scb %x pkt %x", scbp,
				pkt);
#endif
		ASSERT(esa_blkp->eb_scbp == (struct sequencer_ctrl_block *)0);
		esa_blkp->eb_scbp = scbp;

/*		esa_pollret will handle all details			*/
		return;
	}

	/*
	 * esa_pollret holds the esa_blk mutex, so we should not run the
	 * callback here because esa_pollret cannot give up the mutex.
	 * Stack up non-polled completions and maintain a counter
	 */

	if (esa_cardp->ec_flags & ESA_POLLING) {
		esa_blkp->eb_pkts_done++;
		esa_savepkt(esa_blkp, scbp);
		if (!(esa_blkp->eb_flag & ESA_POLL_TRIGGER_ON)) {
			ddi_trigger_softintr(esa_blkp->eb_softid);
			esa_blkp->eb_flag |= ESA_POLL_TRIGGER_ON;
		}
		return;
	}

#ifdef ESA_DEBUG
	if (esa_retscb(esa_blkp, scbp) != scbp)
		cmn_err(CE_WARN, "pollret retscb fail scb %x", scbp);
	esa_blkp->eb_pkts_out--;
#endif

	mutex_exit(esa_blkp->eb_mutex);
	esa_chkstatus(esa_blkp, pkt, scbp);
	(*pkt->pkt_comp)(pkt);
	mutex_enter(esa_blkp->eb_mutex);
}

static int
esa_xlate_vec(register u_char vec)
{
	static u_char esa_vec[] = {9, 10, 11, 12, 13, 14, 15};
	register int i;

	for (i = 0; i < (sizeof (esa_vec)/sizeof (u_char)); i++) {
		if (esa_vec[i] == vec)
			return (i);
	}
	return (-1);
}

#ifdef ESA_DEBUG
void
esa_dump_card(struct esa_card *c)
{
	struct him_data_block *datap;

	PRF("blkp %x ioaddr %x intr_idx %d flags %x next %x datap %x ",
		c->ec_blkp, c->ec_ioaddr, c->ec_intr_idx, c->ec_flags,
		c->ec_next, c->ec_datap);
	PRF("paddr %x children %x\n", c->ec_him_data_paddr,
		c->ec_child);
	if (c->ec_datap) {
		datap = c->ec_datap;
		PRF("him_data APtr %x BPtr %x\n", datap->AConfigPtr,
		datap->BConfigPtr);
	}
}

void
esa_dump_block(struct him_config_block *esa_blkp)
{
	struct sequencer_ctrl_block *s, *last;
	int i;

	PRF("id %x dip 0x%x ioaddr 0x%x channel %d IRQ %d config flags 0x%x",
	    esa_blkp->Cfe_AdapterID & 0xffff, esa_blkp->eb_dip,
		esa_blkp->Cfe_PortAddress, esa_blkp->Cfe_SCSIChannel,
	    esa_blkp->Cfe_IrqChannel, esa_blkp->Cfe_ConfigFlags & 0xff);

	PRF(" dma %x card_ptr %x scbp %x last scb %x him_data %x child %d\n",
		esa_blkp->Cfe_DmaChannel,
	    esa_blkp->eb_cardp, esa_blkp->eb_scbp,
		esa_blkp->eb_last_scbp, esa_blkp->Cfe_HaDataPtr,
		esa_blkp->eb_child);
	PRF("eb_scbp_que %x pkts_out %d pkts_done %d\n",
		esa_blkp->eb_scbp_que,
		esa_blkp->eb_pkts_out,
		esa_blkp->eb_pkts_done);
	PRF("scb head %x ", esa_blkp->eb_scboutp);
	s = esa_blkp->eb_scboutp;
	i = 0;
	while (s) {
		PRF(" %x ", s);
		s = s->scb_forw;
		if (esa_blkp->eb_scboutp == s)
			break;
		i++;
		if (i > 8)
			break;
	}
}

char *SCB_HBA_status[] = {
	"No adapter status available",		/* 0x00 */
	"HBA Unknown Status"			/* 0x01 */
	"HBA Unknown Status"			/* 0x02 */
	"HBA Unknown Status"			/* 0x03 */
	"Command aborted by host",		/* 0x04 */
	"HBA Unknown Status"			/* 0x05 */
	"HBA Unknown Status"			/* 0x06 */
	"HBA Unknown Status"			/* 0x07 */
	"HBA Unknown Status"			/* 0x08 */
	"HBA Unknown Status"			/* 0x09 */
	"HBA Unknown Status"			/* 0x0a */
	"HBA Unknown Status"			/* 0x0b */
	"HBA Unknown Status"			/* 0x0c */
	"HBA Unknown Status"			/* 0x0d */
	"HBA Unknown Status"			/* 0x0e */
	"HBA Unknown Status"			/* 0x0f */
	"HBA Unknown Status"			/* 0x10 */
	"Selection timeout",			/* 0x11 */
	"Data overrun or underrun error",	/* 0x12 */
	"Unexpected bus free",			/* 0x13 */
	"Target bus phase sequence error",	/* 0x14 */
	"HBA Unknown Status"			/* 0x15 */
	"HBA Unknown Status"			/* 0x16 */
	"Invalid SCSI linking operation",	/* 0x17 */
	"HBA Unknown Status"			/* 0x18 */
	"HBA Unknown Status"			/* 0x19 */
	"HBA Unknown Status"			/* 0x1a */
	"Auto request sense failed",		/* 0x1b */
	"Tagged Queuing rejected by target",	/* 0x1c */
	"HBA Unknown Status"			/* 0x1d */
	"HBA Unknown Status"			/* 0x1e */
	"HBA Unknown Status"			/* 0x1f */
	"Host adpater hardware error",		/* 0x20 */
	"Target did'nt respond to ATN (RESET)",	/* 0x21 */
	"SCSI bus reset by host adapter",	/* 0x22 */
	"SCSI bus reset by other device",	/* 0x23 */
	"HBA Unknown Status"			/* 0x24 */
};

void
esa_dump_scb(struct sequencer_ctrl_block *scbp)
{
	int i;

	PRF("Cmd %x targ %d lun %d Channel %d addr esa_scsi_cmd %x\n",
		scbp->SCB_Cmd & 0xff,
	    ((scbp->SCB_Tarlun >> 4) & 0xF), scbp->SCB_Tarlun & LUN,
	    scbp->SCB_Tarlun & CHANNEL, scbp->SCB_cmdp);

	if (scbp->SCB_Flags & AUTO_SENSE)
		PRF("Auto Request Sense Enabled\n");
	else
		PRF("No Auto Request Sense\n");

	if (scbp->SCB_Flags & NO_UNDERRUN)
		PRF("Data Underrun, not considered as error\n");
	else
		PRF("Data Underrun, considered as error\n");

	if (scbp->SCB_Cntrl & DIS_ENABLE)
		PRF("Allow target disconnection\n");
	else
		PRF("No target disconnection\n");

	if (scbp->SCB_Cntrl & TAG_ENABLE)
		PRF("Tagged Queuing supported\n");
	else
		PRF("No tagged Queuing\n");

	PRF("SegCnt %x SegPtr %x CDBLen %x\n", scbp->SCB_SegCnt,
		scbp->SCB_SegPtr, scbp->SCB_CDBLen);

	PRF("Status of SCB %x ResCnt %x\n", scbp->SCB_Stat,
		scbp->SCB_ResCnt);

	PRF("CDB: ");
	for (i = 0; i < scbp->SCB_CDBLen; i++) {
		PRF("%x ", scbp->SCB_CDB[i] & 0xff);
	}
	PRF("\nSensePtr %x SenseLen %x\n", scbp->SCB_SensePtr,
		scbp->SCB_SenseLen);

	i = scbp->SCB_HaStat & 0xff;
	if ((i < 0) || (i > HOST_RST_OTHER))
		i = 0x24;
	PRF("i 0x%x HBA Status %s\n", i, SCB_HBA_status[i]);

	PRF("Target Status %x scb_forw %x scb_back %x\n",
		scbp->SCB_TargStat, scbp->scb_forw, scbp->scb_back);
}

static void
esa_trace()
{
}
#endif
