/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma ident	"@(#)trantor.c	95/02/16 SMI"

/*
 *	Solaris device driver for Trantor T348 SCSI HBA.
 */

/* #define	TRANTOR_DEBUG */

/* Following group of includes used for callback thread definitions */
#include <sys/var.h>
#include <sys/thread.h>
#include <sys/proc.h>

#include <sys/scsi/scsi.h>
#include <sys/debug.h>

#include <sys/dktp/trantor/tran_port.h>
#include <sys/dktp/trantor/trantor.h>
#include <sys/dktp/trantor/tran_n5380.h>
#include <sys/dktp/trantor/tran_card.h>
#include <sys/dktp/trantor/tran_scsi.h>

typedef unsigned char uchar;

/*
 * External SCSA Interface
 */

static int trantor_tran_tgt_init(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
static void trantor_tran_tgt_free(dev_info_t *, dev_info_t *,
			scsi_hba_tran_t *, struct scsi_device *);
static struct scsi_pkt *trantor_tran_init_pkt(struct scsi_address *ap,
	struct scsi_pkt *pkt, struct buf *bp, int cmdlen, int statuslen,
	int tgtlen, int flags, int (*callback)(), caddr_t arg);
static void trantor_tran_destroy_pkt(struct scsi_address *ap,
	struct scsi_pkt *pkt);

static int trantor_abort(struct scsi_address *ap, struct scsi_pkt *pkt);
static int trantor_reset(struct scsi_address *ap, int level);
static int trantor_capchk(char *cap, int tgtonly, int *cidxp);
static int trantor_getcap(struct scsi_address *ap, char *cap, int tgtonly);
static int trantor_setcap(struct scsi_address *ap, char *cap, int value,
		int tgtonly);
static struct scsi_pkt *trantor_pktalloc(struct scsi_address *ap, int cmdlen,
	int statuslen, int tgtlen, int (*callback)(), caddr_t arg);
static void trantor_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt);
static struct scsi_pkt *trantor_dmaget(struct scsi_pkt *pktp, struct buf *bp,
	int (*callback)(), caddr_t arg);
static void trantor_dmafree(struct scsi_address *ap, struct scsi_pkt *pkt);
static void trantor_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pktp);
static int trantor_transport(struct scsi_address *ap, struct scsi_pkt *pktp);

/*
 * Local Function Prototypes
 */
static void trantor_callback(struct trantor_blk *trantor_blkp);
static kthread_t *trantor_create_cbthread(struct trantor_blk *trantor_blkp);
void trantor_destroy_cbthread(struct trantor_blk *trantor_blkp);
static int trantor_propinit(struct trantor_blk *trantor_blkp);
static int trantor_findhba(dev_info_t *devi, uint baseAddr);
static int trantor_send_cmd(struct trantor_blk *trantor_blkp,
    struct trantor_ccb *ccbp);
static void trantor_pollret(struct trantor_blk *trantor_blkp,
    struct scsi_pkt *pktp, int ret);
static void trantor_saveccb(struct trantor_blk *trantor_blkp,
    struct trantor_ccb *ccbp);
static struct trantor_ccb *trantor_retccb(struct trantor_blk *trantor_blkp,
    struct trantor_ccb *ccbp);
static struct scsi_pkt *trantor_chkstatus(struct trantor_blk *trantor_blkp,
    struct scsi_pkt *pktp, int ret);
static void trantor_reset_delay();
void trantor_run_cbthread(struct trantor_blk *trantor_blkp,
    struct scsi_pkt *pktp);
#ifdef TRANTOR_DEBUG
static void trantor_dump_ccb(struct trantor_ccb *p);
#endif	/* TRANTOR_DEBUG */

/*
 * Local static data
 */
static kmutex_t trantor_rmutex;
static kmutex_t trantor_global_mutex;
static int trantor_global_init = 0;

/*
 * DMA limits for data transfer
 */
static ddi_dma_lim_t trantor_dma_lim = {
	0,		/* address low				*/
	0xffffffffU,	/* address high				*/
	0,		/* counter max				*/
	1,		/* burstsize 				*/
	DMA_UNIT_8,	/* minimum xfer				*/
	0,		/* dma speed				*/
	(u_int)DMALIM_VER0,	/* version				*/
	0xffffffffU,	/* address register			*/
	0x003fffff,	/* counter max 				*/
	512,		/* sector size				*/
	TRANTOR_MAX_DMA_SEGS, /* scatter/gather list length	*/
	0xffffffffU	/* request size				*/
};

#ifdef TRANTOR_DEBUG
/*
 *	trantor_debug is used to turn on/off specific debug statements, it is a
 *	bit mapped value as follows:
 */
#define	DENTRY		0x01	/* data entry */
#define	DIO		0x02	/* io processing */
#define	LOW_LEVEL	0x04	/* low level debugging */
#define	BYTE		0x08	/* low level debugging */
#define	DPROBE		0x10
#define	DSPECIAL	0x20	/* debugging special situations */
#define	DERROR		0x40	/* report errors via printf */

int trantor_debug = DPROBE | DERROR;
#endif	/* TRANTOR_DEBUG */

static int trantor_identify(dev_info_t *dev);
static int trantor_probe(dev_info_t *);
static int trantor_attach(dev_info_t *dev, ddi_attach_cmd_t cmd);
static int trantor_detach(dev_info_t *dev, ddi_detach_cmd_t cmd);

struct dev_ops	trantor_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	ddi_no_info,		/* info */
	trantor_identify,	/* identify */
	trantor_probe,		/* probe */
	trantor_attach,		/* attach */
	trantor_detach,		/* detach */
	nulldev,		/* reset */
	(struct cb_ops *)0,	/* driver operations */
	NULL			/* bus operations */
};

char _depends_on[] = "misc/scsi";

/*
 * This is the loadable module wrapper.
 */
#include <sys/modctl.h>

extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,			/* module type: driver */
	"Trantor MiniSCSIPlus Parallel Port SCSI Adapter Driver",  /* name */
	&trantor_ops,			/* driver ops */
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

	mutex_init(&trantor_global_mutex, "TRANTOR global Mutex",
		MUTEX_DRIVER, (void *)NULL);

	if ((status = mod_install(&modlinkage)) != 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&trantor_global_mutex);
	}
	return (status);
}

int
_fini(void)
{
	int	status;

	if ((status = mod_remove(&modlinkage)) == 0) {
		scsi_hba_fini(&modlinkage);
		mutex_destroy(&trantor_global_mutex);
	}
	return (status);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

/*
 * Include the other C source files that comprise this driver.  This
 * strategy allows us to preserve the file divisions from the vendor's
 * code while making most routine names static.
 */
#include "tran_pp.c"
#include "tran_card.c"
#include "tran_n5380.c"
#include "tran_p3c.c"
#include "tran_scsi.c"


/*ARGSUSED*/
static int
trantor_tran_tgt_init(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	struct 	trantor *hba_trantorp;
	struct 	trantor *trantor_unitp;
	int 	targ;
	int	lun;

	targ = sd->sd_address.a_target;
	lun = sd->sd_address.a_lun;

#ifdef TRANTOR_DEBUG
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

	hba_trantorp = SDEV2HBA(sd);

	if ((trantor_unitp = kmem_zalloc(
		sizeof (struct trantor) + sizeof (struct trantor_unit),
			KM_NOSLEEP)) == NULL) {
		return (DDI_FAILURE);
	}

	bcopy((caddr_t)hba_trantorp, (caddr_t)trantor_unitp,
		sizeof (*hba_trantorp));
	trantor_unitp->t_unitp = (struct trantor_unit *)(trantor_unitp+1);

	trantor_unitp->t_unitp->tu_lim = trantor_dma_lim;

	hba_tran->tran_tgt_private = trantor_unitp;

	/* increment active child cnt */
	TRANTOR_BLKP(hba_trantorp)->tb_child++;

#ifdef TRANTOR_DEBUG
	if (trantor_debug & DENTRY) {
		printf("trantor_tran_tgt_init: <%d,%d>\n", targ, lun);
	}
#endif
	return (DDI_SUCCESS);
}


/*ARGSUSED*/
static void
trantor_tran_tgt_free(
	dev_info_t		*hba_dip,
	dev_info_t		*tgt_dip,
	scsi_hba_tran_t		*hba_tran,
	struct scsi_device	*sd)
{
	struct trantor		*trantor_unitp;
	struct trantor_blk	*trantor_blkp;

#ifdef	TRANTORDEBUG
	cmn_err(CE_CONT, "trantor_tran_tgt_free: %s%d %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif	/* TRANTORDEBUG */

	trantor_unitp = hba_tran->tran_tgt_private;
	trantor_blkp = TRANTOR_BLKP(SDEV2TRANTOR(sd));

	kmem_free(trantor_unitp,
		sizeof (struct trantor) + sizeof (struct trantor_unit));

	trantor_blkp->tb_child--;
}

/*
 *	Autoconfiguration routines
 */
static int
trantor_identify(dev_info_t *devi)
{
	char *dname = ddi_get_name(devi);

	if (strcmp(dname, "trantor") == 0)
		return (DDI_IDENTIFIED);
	else
		return (DDI_NOT_IDENTIFIED);
}


static int
trantor_probe(register dev_info_t *devi)
{
	int	len;
	uint	newBase;
	int	result;

	len = sizeof (int);
	if (HBA_INTPROP(devi, "ioaddr", &newBase, &len) != DDI_SUCCESS) {
#ifdef TRANTOR_DEBUG
		if (trantor_debug & (DPROBE | DERROR))
			printf("trantor_probe: HBA_INTPROP failed\n");
#endif
		return (DDI_PROBE_FAILURE);
	}

	result = trantor_findhba(devi, newBase);
#ifdef TRANTOR_DEBUG
	if ((trantor_debug & DPROBE) || (result != DDI_PROBE_SUCCESS &&
			(trantor_debug & DERROR)))
		printf("trantor_probe: device %sfound at I/O address %x\n",
			result == DDI_PROBE_SUCCESS ? "" : "not ", newBase);
#endif
	return (result);
}

static int
trantor_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	struct 	trantor *trantor;
	struct	trantor_blk *trantor_blkp;

	switch (cmd) {
	case DDI_DETACH:
	{
		scsi_hba_tran_t	*tran;

		tran = (scsi_hba_tran_t *)ddi_get_driver_private(dip);
		if (!tran)
			return (DDI_SUCCESS);

		trantor = TRAN2HBA(tran);
		if (!trantor)
			return (DDI_SUCCESS);

		trantor_blkp = TRANTOR_BLKP(trantor);
		if (!trantor_blkp)
			return (DDI_SUCCESS);

		/* Fail if there are active children */
		if (trantor_blkp->tb_child)
			return (DDI_FAILURE);

		trantor_destroy_cbthread(trantor_blkp);

		mutex_destroy(&trantor_blkp->tb_mutex);

		ddi_prop_remove_all(dip);

		if (scsi_hba_detach(dip) != DDI_SUCCESS) {
			cmn_err(CE_WARN, "trantor: scsi_hba_detach failed\n");
		}
		scsi_hba_tran_free(trantor->t_tran);

		mutex_enter(&trantor_global_mutex);
		trantor_global_init--;
		if (trantor_global_init == 0)
			mutex_destroy(&trantor_rmutex);

		kmem_free((caddr_t)trantor,
		    (sizeof (*trantor)+sizeof (*trantor_blkp)));
		mutex_exit(&trantor_global_mutex);

		return (DDI_SUCCESS);
	}
	default:
		return (DDI_FAILURE);
	}
}

static int
trantor_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
	register struct trantor 	*trantor;
	register struct trantor_blk	*trantor_blkp;
	int 			unit;
	scsi_hba_tran_t		*hba_tran;

	switch (cmd) {
	case DDI_ATTACH:
		break;

	default:
		return (DDI_FAILURE);
	}

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_attach: enter\n");
#endif /* TRANTOR_DEBUG */
	trantor = (struct trantor *)kmem_zalloc((unsigned)(sizeof (*trantor) +
		sizeof (*trantor_blkp)), KM_NOSLEEP);
	if (!trantor)
		return (DDI_FAILURE);
	trantor_blkp = (struct trantor_blk *)(trantor + 1);
	TRANTOR_BLKP(trantor) = trantor_blkp;
	trantor_blkp->tb_dip = devi;
	trantor_blkp->tb_targetid = HOST_ID;

	if (trantor_propinit(trantor_blkp) == DDI_FAILURE) {
		kmem_free((caddr_t)trantor,
		    (sizeof (*trantor)+sizeof (*trantor_blkp)));
		return (DDI_FAILURE);
	}

	/*
	 * Allocate a transport structure
	 */
	hba_tran = scsi_hba_tran_alloc(devi, 0);
	if (hba_tran == NULL) {
		cmn_err(CE_WARN,
			"trantor_attach: scsi_hba_tran_alloc failed\n");
		kmem_free((caddr_t)trantor,
			(sizeof (*trantor)+sizeof (*trantor_blkp)));
		return (DDI_FAILURE);
	}

	/*
	 * Create a callback thread
	 */
	trantor_blkp->tb_cbth = trantor_create_cbthread(trantor_blkp);
	if (trantor_blkp->tb_cbth == 0) {
		cmn_err(CE_WARN,
			"trantor_attach: trantor_create_cbthread failed\n");
		kmem_free((caddr_t)trantor,
			(sizeof (*trantor)+sizeof (*trantor_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

/*
 *	Establish initial dummy interrupt handler
 *	get iblock cookie to initialize mutexes used in the
 *	real interrupt handler
 */
	/*
	 *	No interrupt will be used, so pass NULL instead of
	 *	ddi_iblock_cookie.
	 */
	mutex_init(&trantor_blkp->tb_mutex, "trantor mutex", MUTEX_DRIVER,
	    (void *)NULL);

	trantor->t_tran = hba_tran;

	hba_tran->tran_hba_private	= trantor;
	hba_tran->tran_tgt_private	= NULL;

	hba_tran->tran_tgt_init		= trantor_tran_tgt_init;
	hba_tran->tran_tgt_probe	= scsi_hba_probe;
	hba_tran->tran_tgt_free		= trantor_tran_tgt_free;

	hba_tran->tran_start 		= trantor_transport;
	hba_tran->tran_abort		= trantor_abort;
	hba_tran->tran_reset		= trantor_reset;
	hba_tran->tran_getcap		= trantor_getcap;
	hba_tran->tran_setcap		= trantor_setcap;
	hba_tran->tran_init_pkt		= trantor_tran_init_pkt;
	hba_tran->tran_destroy_pkt	= trantor_tran_destroy_pkt;
	hba_tran->tran_dmafree		= trantor_dmafree;
	hba_tran->tran_sync_pkt		= trantor_sync_pkt;

	if (scsi_hba_attach(devi, &trantor_dma_lim,
			hba_tran, SCSI_HBA_TRAN_CLONE, NULL) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "trantor_attach: scsi_hba_attach failed\n");
		trantor_destroy_cbthread(trantor_blkp);
		kmem_free((caddr_t)trantor,
			(sizeof (*trantor)+sizeof (*trantor_blkp)));
		scsi_hba_tran_free(hba_tran);
		return (DDI_FAILURE);
	}

	mutex_enter(&trantor_global_mutex); /* protect multithreaded attach */
	/*
	 *	No interrupt will be used, so pass NULL instead of
	 *	ddi_iblock_cookie.
	 */
	if (!trantor_global_init) {
		mutex_init(&trantor_rmutex, "TRANTOR Resource Mutex",
		    MUTEX_DRIVER, (void *)NULL);
	}
	trantor_global_init++;
	mutex_exit(&trantor_global_mutex);

	unit = D_MP;
	(void) ddi_prop_create(DDI_DEV_T_NONE, devi, DDI_PROP_CANSLEEP,
	    "mt-attr", (caddr_t)&unit, (int)sizeof (int));

	ddi_report_dev(devi);

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_attach: exit.\n");
#endif /* TRANTOR_DEBUG */
	return (DDI_SUCCESS);
}

static int
trantor_propinit(register struct trantor_blk *trantor_blkp)
{
	register dev_info_t *devi;
	int	val;
	int	len;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_propinit: enter\n");
#endif /* TRANTOR_DEBUG */
	devi = trantor_blkp->tb_dip;
	len = sizeof (int);
	if (HBA_INTPROP(devi, "ioaddr", &val, &len) != DDI_PROP_SUCCESS)
		return (DDI_FAILURE);

	trantor_blkp->tb_ioaddr = (ushort) val;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_propinit: exit.\n");
#endif /* TRANTOR_DEBUG */
	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
trantor_transport(struct scsi_address *ap, register struct scsi_pkt *pktp)
{
	register struct trantor_blk *trantor_blkp;
	register struct	trantor_ccb *ccbp;
	int	status;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DSPECIAL)
		printf("\
transport(I): targ = %d, s_cmd = %xh, p_flags = %xh\n",
		ap->a_target, *(pktp->pkt_cdbp), pktp->pkt_flags);
#endif /* TRANTOR_DEBUG */
	ccbp = (struct trantor_ccb *)pktp->pkt_ha_private;

	trantor_blkp = PKT2TRANTORBLKP(pktp);
	mutex_enter(&trantor_blkp->tb_mutex);

/*
 *	If we think the device was unplugged, or this command is a request
 *	sense, determine whether the device is plugged in before proceeding.
 */
	if (trantor_blkp->tb_unplugged ||
			*(pktp->pkt_cdbp) == SCMD_REQUEST_SENSE) {
		if (!CardCheckAdapter(trantor_blkp->tb_ioaddr,
				trantor_blkp->tb_unplugged)) {
			trantor_blkp->tb_unplugged = 1;
			mutex_exit(&trantor_blkp->tb_mutex);
			pktp->pkt_reason = CMD_TRAN_ERR;
#if defined(TRAN_FATAL_ERROR)
			return (TRAN_FATAL_ERROR);
#else
			return (TRAN_BADPKT);
#endif
		}
		if (trantor_blkp->tb_unplugged) {
			/*
			 * Device was just plugged back in and was reset
			 * as part of testing for its presence.  Allow
			 * the bus and devices to settle.
			 */
			trantor_reset_delay();
		}
		trantor_blkp->tb_unplugged = 0;
	}

/* 	put this ccb on linked list of outstanding ccbs 		*/
	trantor_saveccb(trantor_blkp, ccbp);

	status = trantor_send_cmd(trantor_blkp, ccbp);

	/* Trantor is not interrupt driven */
	trantor_pollret(trantor_blkp, pktp, status);

	/*
	 * Exit mutex before doing callback because callback can call
	 * trantor_transport recursively.
	 */
	mutex_exit(&trantor_blkp->tb_mutex);

	/* Issue a callback if requested */
	if (pktp->pkt_comp)
		trantor_run_cbthread(trantor_blkp, pktp);

#ifdef	TRANTOR_DEBUG
	if ((trantor_debug & DSPECIAL) || (status && (trantor_debug & DERROR)))
		printf("trantor_transport: exit status %x.\n", status);
#endif /* TRANTOR_DEBUG */
	return (TRAN_ACCEPT);
}

/* Abort specific command on target device */
/*ARGSUSED*/
static int
trantor_abort(struct scsi_address *ap, struct scsi_pkt *pktp)
{
	register struct	trantor_blk *trantor_blkp;
	register struct trantor_ccb *ccbp;
	struct  trantor_ccb *rccbp;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_abort: enter\n");
#endif /* TRANTOR_DEBUG */
	if (!pktp)
		return (1);

	trantor_blkp = PKT2TRANTORBLKP(pktp);
	ccbp = (struct  trantor_ccb *)pktp->pkt_ha_private;

	mutex_enter(&trantor_blkp->tb_mutex);
	rccbp = trantor_retccb(trantor_blkp, ccbp);
	mutex_exit(&trantor_blkp->tb_mutex);


	if (!rccbp) {
#ifdef TRANTOR_DEBUG
		printf("trantor_abort: called for command not outstanding\n");
#endif
		return (TRUE);
	}

	if (ccbp != rccbp) {
#ifdef TRANTOR_DEBUG
		printf("trantor_abort: bad ccbp 0x%x rccbp 0x%x\n",
		    ccbp, rccbp);
#endif
		return (FALSE);
	}

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_abort: exit\n");
#endif /* TRANTOR_DEBUG */
	return (TRUE);
}

/* reset the scsi bus, or just one target device */
static int
trantor_reset(struct scsi_address *ap, int level)
{
	register struct	trantor_blk *trantor_blkp;
	unchar ccb_cdblen; /* length in bytes of SCSI CDB */
	unchar ccb_cdb[HBA_MAX_CDB_LEN]; /* CDB bytes 0-11 */
	unchar ret, status;
	unchar sense[4];
	int answer = TRUE;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_reset: enter\n");
#endif /* TRANTOR_DEBUG */
	trantor_blkp = ADDR2TRANTORBLKP(ap);
	mutex_enter(&trantor_blkp->tb_mutex);

	switch (level) {
		case RESET_ALL:
			CardResetBus(trantor_blkp->tb_ioaddr);
			trantor_reset_delay();
			break;

		case RESET_TARGET:
		/* 	set up scsi cdb 			*/
			ccb_cdblen  = 6;
			bzero((caddr_t)ccb_cdb, 6);
			ccb_cdb[0] = SCMD_REQUEST_SENSE;
			ccb_cdb[4] = 0;
			ret = CardStartCommandInterrupt(
			    (uint)trantor_blkp->tb_ioaddr,
			    (uchar)ap->a_target, (uchar)0, (uchar *)ccb_cdb,
			    (uchar)ccb_cdblen, (uint) 0, (uchar *)sense,
			    (ulong)sizeof (struct scsi_sense),
			    (uchar *)&status);
			if (ret)
				answer = FALSE;
#ifdef	TRANTOR_DEBUG
			if (trantor_debug & DENTRY) {
				printf("\
Trantor_reset: RESET_TARGET(%d), ret = %xh, status = %xh\n",
				    ap->a_target, ret, status);
				printf("req sense data = %xh, %xh, %xh, %xh\n",
				    sense[0], sense[1], sense[2], sense[3]);
			}
#endif	/* TRANTOR_DEBUG */
			break;

		default:
#ifdef TRANTOR_DEBUG
			if (trantor_debug & DENTRY)
				printf("trantor_reset: bad level %d\n", level);
#endif	/* TRANTOR_DEBUG */
			answer = FALSE;
			break;
	}

	mutex_exit(&trantor_blkp->tb_mutex);

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_reset: exit\n");
#endif /* TRANTOR_DEBUG */
	return (answer);
}

static void
trantor_reset_delay()
{
	/*
	 * Delay for two seconds after a Trantor bus reset.
	 */
	delay(drv_usectohz(2 * SEC_INUSEC));
}

static int
trantor_capchk(char *cap, int tgtonly, int *cidxp)
{
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_capchk: enter, cap = %s\n", cap);
#endif /* TRANTOR_DEBUG */

	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *)0)
		return (FALSE);

	*cidxp = scsi_hba_lookup_capstr(cap);

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_capchk: exit\n");
#endif /* TRANTOR_DEBUG */
	return (TRUE);
}

/*ARGSUSED*/
static int
trantor_getcap(struct scsi_address *ap, char *cap, int tgtonly)
{
	int	ckey;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_getcap: enter\n");
#endif /* TRANTOR_DEBUG */
	if (trantor_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {
		case SCSI_CAP_GEOMETRY:
			return (TRANTOR_SETGEOM(64, 32));
		case SCSI_CAP_ARQ:
			return (FALSE);
		case SCSI_CAP_TAGGED_QING:
			return (FALSE);
		default:
			return (UNDEFINED);
	}
}

static int
trantor_setcap(struct scsi_address *ap, char *cap, int value, int tgtonly)
{
	int	ckey;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_setcap: enter\n");
#endif /* TRANTOR_DEBUG */
	if (trantor_capchk(cap, tgtonly, &ckey) != TRUE)
		return (UNDEFINED);

	switch (ckey) {
		case SCSI_CAP_TAGGED_QING:
			return (FALSE);

		case SCSI_CAP_ARQ:
			return (FALSE);
		case SCSI_CAP_SECTOR_SIZE:
			(ADDR2TRANTORUNITP(ap))->tu_lim.dlim_granular =
			    (u_int)value;
			break;
		case SCSI_CAP_GEOMETRY:
			break;
		default:
			return (UNDEFINED);
	}
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_setcap: exit.\n");
#endif /* TRANTOR_DEBUG */
	return (TRUE);
}


/*ARGSUSED*/
static struct scsi_pkt *
trantor_tran_init_pkt(struct scsi_address *ap, struct scsi_pkt *pkt,
	struct buf *bp, int cmdlen, int statuslen, int tgtlen,
	int flags, int (*callback)(), caddr_t arg)
{
	struct scsi_pkt		*new_pkt = NULL;

	/*
	 * Allocate a pkt
	 */
	if (!pkt) {
		pkt = trantor_pktalloc(ap, cmdlen, statuslen,
			tgtlen, callback, arg);
		if (pkt == NULL)
			return (NULL);
		new_pkt = pkt;
	} else {
		new_pkt = NULL;
	}

	/*
	 * Set up dma info
	 */
	if (bp) {
		if (trantor_dmaget(pkt, bp, callback, arg) == NULL) {
			if (new_pkt)
				trantor_pktfree(ap, new_pkt);
			return (NULL);
		}
	}

	return (pkt);
}


static void
trantor_tran_destroy_pkt(struct scsi_address *ap, struct scsi_pkt *pkt)
{
	trantor_dmafree(ap, pkt);
	trantor_pktfree(ap, pkt);
}

static struct scsi_pkt *
trantor_pktalloc(struct scsi_address *ap, int cmdlen, int statuslen,
    int tgtlen, int (*callback)(), caddr_t arg)
{
	register struct scsi_pkt *pkt;
	register struct trantor_ccb	*ccbp = NULL;
	struct	 trantor_blk	*trantor_blkp;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_pktalloc: enter\n");
#endif /* TRANTOR_DEBUG */
	trantor_blkp = ADDR2TRANTORBLKP(ap);

	if (tgtlen < PKT_PRIV_LEN)
		tgtlen = PKT_PRIV_LEN;

	pkt = scsi_hba_pkt_alloc(trantor_blkp->tb_dip, ap, cmdlen,
		statuslen, tgtlen, sizeof (struct trantor_ccb), callback, arg);
	if (!pkt) {
		return ((struct scsi_pkt *)0);
	}

	/* 	initialize ccb 		*/
	ccbp = (struct trantor_ccb *)pkt->pkt_ha_private;
	ccbp->ccb_cdblen = (unchar)cmdlen;
	ccbp->ccb_ownerp = pkt;

	ccbp->ccb_lun = ap->a_lun;
	ccbp->ccb_target = ap->a_target;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_pktalloc: exit.\n");
#endif /* TRANTOR_DEBUG */
	return (pkt);
}

/*
 * packet free
 */
static void
trantor_pktfree(struct scsi_address *ap, struct scsi_pkt *pkt)
{
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_pktfree: enter\n");
#endif /* TRANTOR_DEBUG */

	scsi_hba_pkt_free(ap, pkt);

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_pktfree: exit.\n");
#endif /* TRANTOR_DEBUG */
}

/*
 * Dma sync
 */
/*ARGSUSED*/
static void
trantor_sync_pkt(struct scsi_address *ap, struct scsi_pkt *pktp)
{
}

/*
 * Dma resource allocation
 */
/*ARGSUSED*/
static struct scsi_pkt *
trantor_dmaget(struct scsi_pkt *pktp, struct buf *bp, int (*callback)(),
    caddr_t arg)
{
	register struct trantor_ccb *ccbp;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("\
trantor_dmaget: enter, bp->b_flags = %xh, bp->b_pages = %xh, \
bp->b_un.b_addr = %xh, bp->b_bcount = %d\n",
		    bp->b_flags, bp->b_pages, bp->b_un.b_addr, bp->b_bcount);
#endif /* TRANTOR_DEBUG */

	ccbp = (struct trantor_ccb *)pktp->pkt_ha_private;

	if (!bp->b_bcount) {
		pktp->pkt_resid = 0;

		/*	turn off direction flags		*/
		ccbp->ccb_dataout = 1;
		return (pktp);
	}

	/* check direction for data transfer */
	if (bp->b_flags & B_READ)
		ccbp->ccb_dataout = 0;
	else
		ccbp->ccb_dataout = 1;

	bp_mapin(bp);

	ccbp->ccb_datap = (caddr_t)bp->b_un.b_addr;
	ccbp->ccb_datalen = bp->b_bcount;
	pktp->pkt_resid	= 0;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_dmaget: exit.\n");
#endif /* TRANTOR_DEBUG */
	return (pktp);
}


/*
 * Dma resource free
 */
/*ARGSUSED*/
static void
trantor_dmafree(struct scsi_address *ap, struct scsi_pkt *pktp)
{
#ifdef  TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_dmafree: enter\n");
#endif	/* TRANTOR_DEBUG */

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_dmafree: exit\n");
#endif	/* TRANTOR_DEBUG */
}


/*
 * Returns DDI_SUCCESS if found a controller, else DDI_FAILURE
 */
/*ARGSUSED*/
static int
trantor_findhba(register dev_info_t *devi, register uint baseAddr)
{
#ifdef  TRANTOR_DEBUG
	if (trantor_debug & DPROBE)
		printf("trantor_findhba: entered for address %x\n", baseAddr);
#endif	/* TRANTOR_DEBUG */
	/*
	 * There are several devices that can be plugged into parallel
	 * ports and we want the Trantor probe to interfere as little
	 * as possible with any other device already in use.  The Trantor
	 * does not use interrupts.  If the port has interrupts enabled,
	 * assume that the port is being used for some other device.
	 */
	if (ParallelIntEnabled(baseAddr)) {
#ifdef  TRANTOR_DEBUG
		if (trantor_debug & DPROBE)
			printf("trantor_findhba: interrupts enabled\n");
#endif	/* TRANTOR_DEBUG */
		return (DDI_PROBE_FAILURE);
	}

	if (CardCheckAdapter(baseAddr, 1)) {
#ifdef  TRANTOR_DEBUG
		if (trantor_debug & DPROBE)
			printf("trantor_findhba: found an adapter at %x\n",
				baseAddr);
#endif	/* TRANTOR_DEBUG */
		trantor_reset_delay();
		return (DDI_PROBE_SUCCESS);
	}
#ifdef  TRANTOR_DEBUG
	if (trantor_debug & DPROBE)
		printf("trantor_findhba: no adapter at %x\n", baseAddr);
#endif	/* TRANTOR_DEBUG */
	return (DDI_PROBE_FAILURE);
}


static struct scsi_pkt *
trantor_chkstatus(register struct trantor_blk *trantor_blkp,
    register struct scsi_pkt *pktp, int ret)
{
	register struct trantor_ccb *ccbp;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_chkstatus: enter, \tret = %xh\n", ret);
#endif /* TRANTOR_DEBUG */

	/* extract physical address of ccb */
	ccbp = (struct trantor_ccb *)pktp->pkt_ha_private;
	ccbp = trantor_retccb(trantor_blkp, ccbp);
	if (!ccbp)
		return ((struct scsi_pkt *)NULL);

	pktp = (struct scsi_pkt *)ccbp->ccb_ownerp;

	/*
	 * Fill in appropriate status in packet.  Some errors can occur
	 * when the Trantor is removed, so assume it has been removed when
	 * these errors occur.
	 */
	switch (ret) {
		case HOST_CMD_OK:
			*pktp->pkt_scbp  = STATUS_GOOD;
			pktp->pkt_reason = CMD_CMPLT;
			pktp->pkt_resid  = 0;
			pktp->pkt_state = (STATE_XFERRED_DATA|STATE_GOT_BUS|
			    STATE_GOT_TARGET|STATE_SENT_CMD|STATE_GOT_STATUS);
			break;

		case HOST_MSG_ERR:
			*pktp->pkt_scbp  = ccbp->ccb_status;
			pktp->pkt_reason = CMD_CMPLT;
			pktp->pkt_resid  = 0;
			pktp->pkt_state = (STATE_XFERRED_DATA|STATE_GOT_BUS|
			    STATE_GOT_TARGET|STATE_SENT_CMD|STATE_GOT_STATUS);
			break;

		case HOST_BUS_BUSY:
		case HOST_PHASE_ERR:
			trantor_blkp->tb_unplugged = 1;
			pktp->pkt_reason = CMD_TRAN_ERR;
			break;

		case HOST_SEL_TO:
			pktp->pkt_reason = CMD_TIMEOUT;
			break;

		default:
			pktp->pkt_reason = CMD_INCOMPLETE;
			break;
	}
	return (pktp);
}

/* poll for status of a command sent to hba without interrupts 		*/
static void
trantor_pollret(register struct trantor_blk *trantor_blkp,
    struct scsi_pkt *pktp, int ret)
{
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_pollret: enter\n");
#endif /* TRANTOR_DEBUG */

	if (!trantor_chkstatus(trantor_blkp, pktp, ret)) {
		pktp->pkt_reason = CMD_TRAN_ERR;
	}

	/* clear any interrupt condition on the 5380 */
	CardDisableInterrupt(trantor_blkp->tb_ioaddr);

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_pollret: exit.\n");
#endif	/* TRANTOR_DEBUG */
}

static int
trantor_send_cmd(register struct trantor_blk *trantor_blkp,
    register struct trantor_ccb *ccbp)
{
	register unchar status;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_send_cmd: enter, Lun = %d, Target = %d\n ",
			ccbp->ccb_lun, ccbp->ccb_target);
	if (trantor_debug & DENTRY) {
		printf("Lun = %d, Target = %d, dataout = %d ",
			ccbp->ccb_lun, ccbp->ccb_target,
			ccbp->ccb_dataout);
		(void) trantor_dump_ccb(ccbp);
	}
#endif /* TRANTOR_DEBUG */
	ccbp->ccb_status = 0;
	status = CardStartCommandInterrupt((uint)trantor_blkp->tb_ioaddr,
		(uchar)ccbp->ccb_target, (uchar)ccbp->ccb_lun,
		(uchar *)ccbp->ccb_ownerp->pkt_cdbp, (uchar)ccbp->ccb_cdblen,
		(uint)ccbp->ccb_dataout, (uchar *)ccbp->ccb_datap,
		(ulong)ccbp->ccb_datalen, (uchar *) &(ccbp->ccb_status));
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_send_cmd : exit, \t\tstatus = %xh\n", status);
#endif /* TRANTOR_DEBUG */
	return (status);
}

struct trantor_ccb *
trantor_retccb(struct trantor_blk *trantor_blkp,
		register struct trantor_ccb *ccbp)
{
	register struct	trantor_ccb *cp;
	register struct trantor_ccb *prev = NULL;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_retccb: enter, ccbp = %xh\n", ccbp);
#endif /* TRANTOR_DEBUG */
	for (cp = trantor_blkp->tb_ccboutp; cp; prev = cp, cp = cp->ccb_forw) {
#ifdef	TRANTOR_DEBUG
		if (trantor_debug & DENTRY)
			printf("cp = %xh\n", cp);
#endif /* TRANTOR_DEBUG */
		if (cp != ccbp)
			continue;

		/* if first one on list */
		if (trantor_blkp->tb_ccboutp == cp)
			trantor_blkp->tb_ccboutp = cp->ccb_forw;
		else
			/* intermediate or last */
			prev->ccb_forw = cp->ccb_forw;

		/* if last one on list */
		if (trantor_blkp->tb_last == cp)
			trantor_blkp->tb_last = prev;

		return (cp);
	}
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("trantor_retccb: exit.\n");
#endif /* TRANTOR_DEBUG */

	return ((struct trantor_ccb *)NULL);

}


static void
trantor_saveccb(register struct trantor_blk *trantor_blkp,
    register struct trantor_ccb *ccbp)
{
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DENTRY)
		printf("\
trantor_saveccb: enter, ccbp = %xh\n", ccbp);
#endif /* TRANTOR_DEBUG */
	if (!trantor_blkp->tb_ccboutp)
		trantor_blkp->tb_ccboutp = ccbp;
	else
		trantor_blkp->tb_last->ccb_forw = ccbp;

	ccbp->ccb_forw = NULL;
	trantor_blkp->tb_last = ccbp;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & DIO)
		printf("trantor_saveccb: exit.\n");
#endif /* TRANTOR_DEBUG */
}

char *trantor_err_strings[] = {
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

#ifdef TRANTOR_DEBUG
static void
trantor_dump_ccb(struct trantor_ccb *p)
{
	int index;

	printf("cdb-> ");
	for (index = 0; index < p->ccb_cdblen; index++) {
		printf("%xh ", p->ccb_ownerp->pkt_cdbp[index]);
	}
	printf("\n");

	printf("cdblen 0x%x scsi_pkt 0x%x, cdb[0] = %d\n",
		p->ccb_cdblen & 0xff, p->ccb_ownerp,
		p->ccb_ownerp->pkt_cdbp[0]&0x1f);

	printf("lun %d target %d", p->ccb_lun, p->ccb_target & 0xff);

	printf(" Data ptr 0x%x data len 0x%x\n", (ulong)p->ccb_datap,
		p->ccb_datalen);
}
#endif	/* TRANTOR_DEBUG */

/*
 * The routines trantor_create_cbthread, trantor_destroy_cbthread,
 * trantor_run_cbthread and trantor_callback are included to work
 * around a limitation in the generic SCSI framework.  The SCSI
 * framework does not expect drivers to call the callback routine
 * before returning from the transport routine.  The Trantor is
 * a very primitive device and works by polling so it is always
 * ready to do a callback before leaving transport.  Doing the
 * callback directly from transport is unsafe because the callback
 * can do a nested call to transport to start another queued
 * transaction.  If enough transaction come in quickly enough it
 * is possible to run off the end of the stack of the thread which
 * ran the initial transport call.  To get around this problem the
 * trantor driver maintains a special thread for running callbacks.
 * The transport routine calls trantor_run_cbthread which simply
 * queues up the request and notifies the callback thread that there
 * is work to do.  The callbacks no longer cause nesting but get
 * run in succession from the callback thread.  The code for handling
 * the extra thread is non-DDI-compliant.  The Trantor driver will
 * not be made DDI-compliant unless the SCSI framework is revised
 * to remove the problem described.
 */
static kthread_t *
trantor_create_cbthread(struct trantor_blk *trantor_blkp)
{
	register kthread_id_t id;

	cv_init(&trantor_blkp->tb_scv, "trantor_callback_scv", CV_DRIVER, NULL);
	cv_init(&trantor_blkp->tb_dcv, "trantor_callback_dcv", CV_DRIVER, NULL);
	id = thread_create((caddr_t)NULL, 0, trantor_callback,
		(caddr_t)trantor_blkp, 0, &p0, TS_RUN, v.v_maxsyspri - 2);
	if (!id) {
		cv_destroy(&trantor_blkp->tb_scv);
		cv_destroy(&trantor_blkp->tb_dcv);
	}
	return (id);
}

void
trantor_destroy_cbthread(struct trantor_blk *trantor_blkp)
{
	/*
	 * If there is a callback thread, tell it to exit, wait for it
	 * to do so, then destroy the semaphores.
	 */
	mutex_enter(&trantor_blkp->tb_mutex);
	if (trantor_blkp->tb_cbth) {
		trantor_blkp->tb_cbexit = 1;
		cv_signal(&trantor_blkp->tb_scv);
		do cv_wait(&trantor_blkp->tb_dcv, &trantor_blkp->tb_mutex);
		while (trantor_blkp->tb_cbexit);
		cv_destroy(&trantor_blkp->tb_scv);
		cv_destroy(&trantor_blkp->tb_dcv);
		trantor_blkp->tb_cbth = 0;
	}
	mutex_exit(&trantor_blkp->tb_mutex);
}

void
trantor_run_cbthread(struct trantor_blk *trantor_blkp, struct scsi_pkt *pktp)
{
	register struct	trantor_ccb *ccbp;
	register struct	trantor_ccb *tail;

	/*
	 * Do the callback directly if there is no callback thread.
	 * Otherwise record the packet information and wake up the
	 * callback thread.
	 *
	 * Must release the mutex before calling callback routine
	 * directly because callback routine can call transport
	 * routine.  Otherwise keep the mutex until signalling is
	 * complete.
	 *
	 * We cannot be sure that there will not be more than one
	 * outstanding callback per device although the SCSI subsystem
	 * currently will not start another transaction until the
	 * previous one has finished.  Rather than assume that will
	 * never change, we keep a list of callback packets.
	 */
	mutex_enter(&trantor_blkp->tb_mutex);
	if (!trantor_blkp->tb_cbth) {
		mutex_exit(&trantor_blkp->tb_mutex);
		(*pktp->pkt_comp)(pktp);
	} else {
		ccbp = (struct trantor_ccb *)pktp->pkt_ha_private;
		ccbp->ccb_forw = 0;
		if (!trantor_blkp->tb_cbccb)
			trantor_blkp->tb_cbccb = ccbp;
		else {
			for (tail = trantor_blkp->tb_cbccb; tail->ccb_forw;
					tail = tail->ccb_forw)
				continue;
			tail->ccb_forw = ccbp;
		}
		cv_signal(&trantor_blkp->tb_scv);
		mutex_exit(&trantor_blkp->tb_mutex);
	}
}

static void
trantor_callback(struct trantor_blk *trantor_blkp)
{
	register struct scsi_pkt *pktp;

	/*
	 * Perform any pending callback.  If no callbacks pending,
	 * exit if requested.  Otherwise wait until there is something
	 * to do.
	 */
	mutex_enter(&trantor_blkp->tb_mutex);
	for (;;) {
		if (trantor_blkp->tb_cbccb) {
			pktp = trantor_blkp->tb_cbccb->ccb_ownerp;
			trantor_blkp->tb_cbccb =
				trantor_blkp->tb_cbccb->ccb_forw;
			mutex_exit(&trantor_blkp->tb_mutex);
			(*pktp->pkt_comp)(pktp);
			mutex_enter(&trantor_blkp->tb_mutex);
			continue;
		}
		if (trantor_blkp->tb_cbexit) {
			trantor_blkp->tb_cbexit = 0;
			cv_signal(&trantor_blkp->tb_dcv);
			mutex_exit(&trantor_blkp->tb_mutex);
			return;
		}
		cv_wait(&trantor_blkp->tb_scv, &trantor_blkp->tb_mutex);
	}
}
