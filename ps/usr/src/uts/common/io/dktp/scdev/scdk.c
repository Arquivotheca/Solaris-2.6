/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)scdk.c	1.76	96/09/17 SMI"

/*
 *	SCSI Target Disk
 */

#include <sys/scsi/scsi.h>
#include <sys/vtoc.h>
#include <sys/dkio.h>
#include <sys/cdio.h>
#include <sys/file.h>

#include <sys/dktp/sctarget.h>
#include <sys/dktp/objmgr.h>
#include <sys/dktp/flowctrl.h>
#include <sys/dktp/tgcom.h>
#include <sys/dktp/tgdk.h>
#include <sys/dktp/tgcd.h>
#include <sys/dktp/tgpassthru.h>
#include <sys/dktp/bbh.h>
#include <sys/dktp/scdk.h>
#include <sys/dktp/scdkwatch.h>

/*
 * Global Error Levels for Error Reporting
 */
int scdk_error_level = SCSI_ERR_RECOVERED;

/*
 * Default number of retries for most situations
 */
int sd_retry_count = SD_RETRY_COUNT;

/*
 * Make retries of aborted/reset commands configurable
 */
int sd_retry_all_failures = TRUE;

/*
 * Default timeout value
 */
 int sd_io_time = SD_IO_TIME;

/*
 * Some CDROM drives are very slow to start up, allow
 * them twice as long as other device types.
 */
int sd_io_time_rmb = 2 * SD_IO_TIME;

/*
 *	Object Management
 */
static opaque_t scdk_create();

/*
 * Local Function Prototypes
 */
static void scdk_pktfree(struct scsi_pkt *pktp, struct buf *bp);
static void scdk_setcap(struct scdk *scdkp, struct scsi_pkt *pktp);
static void scdk_polldone(struct buf *bp);
static void scdk_iodone(struct buf *bp);
static void scdk_pktcb(struct scsi_pkt *pktp);
static void scdk_pktcbrs(struct scsi_pkt *pktp);
static void scdk_restart(struct scsi_pkt *pktp);
static int  scdk_ioprep(struct scdk *scdkp, struct scsi_pkt *pktp, int cmd);
static int  scdk_phyg(struct scdk *scdkp);
static int  scdk_rqsprep(struct scdk *scdkp, struct scsi_pkt *pktp);
static int  scdk_chkerr(struct scsi_pkt *pktp);
static int  scdk_incomplete(struct scsi_pkt *pktp);
static int  scdk_ioretry(struct scsi_pkt *pktp, int action);
static int  scdk_exam_rqspkt(struct scsi_pkt *fltpktp);
static int  scdk_rqshdl(struct scdk *scdkp, struct scsi_pkt *pktp,
		struct  scsi_extended_sense *rqsp, int rqslen);
static struct scsi_pkt *scdk_pktprep(struct scdk *scdkp,
		struct scsi_pkt **in_pktp, struct buf *bp, int cdblen,
		void (*cb_func)(), int flags, int (*func)(), caddr_t arg);

static void scdkmin(struct buf *bp);
static void scdk_uscsi(struct scdk *scdkp, struct uscsi_cmd *scmdp,
		struct buf *bp);
static void scdk_uscsi_free(struct uscsi_cmd *scmdp);
static struct uscsi_cmd *scdk_uscsi_alloc();
static int scdk_uscsi_wrapper(struct buf *bp);
static int scdk_uscsi_ioctl(struct scdk *scdkp, dev_t dev, int arg, int flag);
static int scdk_cdioctl(struct scdk *scdkp, dev_t dev, int cmd,
		int arg, int flag);
static int scdk_init_cdioctl(struct scdk *scdkp, dev_t dev, int cmd,
		int arg, int flag);
static int scdk_start_stop(struct scdk *scdkp, dev_t dev, u_char data);
static int scdk_lock_unlock(struct scdk *scdkp, dev_t dev, u_char lock);
static int scdk_eject(struct scdk *scdkp, dev_t dev);
static int scdk_media_watch_cb(caddr_t arg, struct scdk_watch_result *resultp);
static void scdk_delayed_cv_broadcast(caddr_t arg);
static int scdk_config(struct scdk *scdkp);

static	int	scdk_clear_drive(struct scdk *scdkp);
static	int	scdk_start_drive(struct scdk *scdkp);
static	int	scdk_test_unit(struct scdk *scdkp, int retries);
static	int	scdk_req_sense(struct scdk *scdkp);
static	void	scdk_media_lock(struct scdk *scdkp);
static	int	scdk_read_capacity(struct scdk *scdkp);

static	struct	scsi_pkt *scdk_consist_pkt(struct scdk *scdkp, int datalen,
					   ulong bflags, int cdblen);
static	void	scdk_send_pkt(struct scdk *scdkp, struct scsi_pkt *pktp);
static	void	scdk_consist_iodone(register struct buf *bp);


static int  scdk_pkt(struct scdk *scdkp, struct buf *bp, int (*func)(),
		caddr_t arg);
static void scdk_transport(struct scdk *scdkp, struct buf *bp);

struct tgcom_objops scdk_com_ops = {
	nulldev,
	nulldev,
	scdk_pkt,
	scdk_transport,
	0, 0
};

static int scdk_uscsi_buf_setup(struct scdk *scdkp, opaque_t *cmdp,
		dev_t dev, enum uio_seg dataspace);

struct 	tgpassthru_objops scdk_pt_ops = {
	nulldev,
	nulldev,
	scdk_uscsi_buf_setup,
	0, 0
};

static int scdk_init(struct scdk *scdkp, struct scsi_device *devp,
	opaque_t flcobjp, opaque_t queobjp, opaque_t bbhobjp, void *lkarg,
	void (*cbfunc)(void *), void *cbarg);
static int scdk_free(struct tgdk_obj *dkobjp);
static int scdk_probe(struct scdk *scdkp, int kmsflg);
static int scdk_attach(struct scdk *scdkp);
static int scdk_start_drive(struct scdk *scdkp);
static int scdk_open(struct scdk *scdkp, int flag);
static int scdk_close(struct scdk *scdkp);
static int scdk_ioctl(struct scdk *scdkp, dev_t dev, int cmd, int arg, int flag,
		cred_t *cred_p, int *rval_p);
static int scdk_strategy(struct scdk *scdkp, struct buf *bp);
static int scdk_setgeom(struct scdk *scdkp, struct tgdk_geom *dkgeom_p);
static int scdk_getgeom(struct scdk *scdkp, struct tgdk_geom *dkgeom_p);
static int scdk_getphygeom(struct scdk *scdkp, struct tgdk_geom *dkgeom_p);
static struct tgdk_iob *scdk_iob_alloc(struct scdk *scdkp, daddr_t blkno,
		long xfer, int kmsflg);
static int scdk_iob_free(struct scdk *scdkp, struct tgdk_iob *iobp);
static caddr_t scdk_iob_htoc(struct scdk *scdkp, struct tgdk_iob *iobp);
static caddr_t scdk_iob_xfer(struct scdk *scdkp, struct tgdk_iob *iobp, int rw);
static int scdk_dump(struct scdk *scdkp, struct buf *bp);
static int scdk_check_media(struct scdk *scdkp, enum dkio_state *state);
static int scdk_inquiry(struct scdk *scdkp, struct scsi_inquiry **inqpp);

struct tgdk_objops scdk_ops = {
	scdk_init,
	scdk_free,
	scdk_probe,
	scdk_attach,
	scdk_open,
	scdk_close,
	scdk_ioctl,
	scdk_strategy,
	scdk_setgeom,
	scdk_getgeom,
	scdk_iob_alloc,
	scdk_iob_free,
	scdk_iob_htoc,
	scdk_iob_xfer,
	scdk_dump,
	scdk_getphygeom,
	nulldev,
	scdk_check_media,
	scdk_inquiry,
	0, 0
};

/* Routines from scdk_watch.c */
extern void scdk_watch_init();
extern void scdk_watch_fini();
extern void scdk_watch_request_terminate(opaque_t token);
extern opaque_t scdk_watch_request_submit(struct scsi_device *devp,
    int interval, int sense_length, int (*callback)(), caddr_t cb_arg);

/*
 * Local static data
 */

#ifdef	SCDK_DEBUG
#define	DENT	0x0001
#define	DERR	0x0002
#define	DIO	0x0004
#define	DGEOM	0x0010
#define	DSTATE  0x0020
static	int	scdk_debug = 0;

#endif	/* SCDK_DEBUG */

static char *scdk_name = "Disk";
static int scdk_uscsi_maxphys = 0x80000;
static int scdk_check_media_time = 3000000;	/* 3 Second State Check */

/*
 * Error Printing
 */

struct scsi_key_strings scdk_cmds[] = {
	0x00, "test unit ready",
	0x01, "rezero",
	0x03, "request sense",
	0x04, "format",
	0x07, "reassign",
	0x08, "read",
	0x0a, "write",
	0x0b, "seek",
	0x10, "write file mark",
	0x11, "space",
	0x12, "inquiry",
	0x15, "mode select",
	0x16, "reserve",
	0x17, "release",
	0x18, "copy",
	0x19, "erase tape",
	0x1a, "mode sense",
	0x1b, "start/stop",
	0x1e, "door lock",
	0x25, "read capacity",
	0x28, "read(10)",
	0x2a, "write(10)",
	0x2f, "verify",
	0x37, "read defect data",
	0x42, "read subchannel",
	0x43, "read toc",
	0x44, "read header",
	0x47, "play audio msf",
	0x48, "play audio track/index",
	0x4b, "pause/resume",
	-1, NULL
};

/*
 *	This is the loadable module wrapper
 */
#include <sys/modctl.h>

extern struct mod_ops mod_miscops;

static struct modlmisc modlmisc = {
	&mod_miscops,	/* Type of module */
	"SCSI Target Disk Object"
};

static struct modlinkage modlinkage = {
	MODREV_1, (void *)&modlmisc, NULL
};

char _depends_on[] = "misc/scsi drv/objmgr";

int
_init(void)
{
#ifdef SCDK_DEBUG
	if (scdk_debug & DENT)
		PRF("scdk_init: call\n");
#endif
	objmgr_ins_entry("scdk", (opaque_t)scdk_create, OBJ_MODGRP_SNGL);
	scdk_watch_init();
	return (mod_install(&modlinkage));
}

int
_fini(void)
{
#ifdef SCDK_DEBUG
	if (scdk_debug & DENT)
		PRF("scdk_fini: call\n");
#endif
	if (objmgr_del_entry("scdk") == DDI_FAILURE)
		return (EBUSY);
	scdk_watch_fini();
	return (mod_remove(&modlinkage));
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

static opaque_t
scdk_create()
{
	register struct tgdk_obj *dkobjp;
	register struct	scdk *scdkp;

	dkobjp = (struct tgdk_obj *)kmem_zalloc((sizeof (*dkobjp) +
		sizeof (*scdkp)), KM_NOSLEEP);
	if (!dkobjp)
		return (NULL);
	scdkp = (struct scdk *)(dkobjp+1);

	dkobjp->tg_ops  = (struct  tgdk_objops *)&scdk_ops;
	dkobjp->tg_data = (opaque_t)scdkp;
	dkobjp->tg_ext = &(dkobjp->tg_extblk);
	scdkp->scd_extp = &(dkobjp->tg_extblk);

#ifdef SCDK_DEBUG
	if (scdk_debug & DENT)
		PRF("scdk_create: tgdkobjp= 0x%x scdkp= 0x%x \n", dkobjp,
			scdkp);
#endif
	return ((opaque_t)dkobjp);
}

/*ARGSUSED*/
static int
scdk_init(struct scdk *scdkp, struct scsi_device *devp, opaque_t flcobjp,
	opaque_t queobjp, opaque_t bbhobjp, void *lkarg, 
	void (*cbfunc)(void *), void *cbarg)
{
	scdkp->scd_sd = devp;
	devp->sd_private = (caddr_t)scdkp;

	scdkp->scd_cdioctl = scdk_init_cdioctl;

/*	initialize the communication object				*/
	scdkp->scd_com.com_data = (opaque_t)scdkp;
	scdkp->scd_com.com_ops  = &scdk_com_ops;

	scdkp->scd_flcobjp = flcobjp;

	scdkp->scd_cbfunc = cbfunc;
	scdkp->scd_cbarg = cbarg;
	sema_init(&scdkp->scd_drvsema, 1, "scdk", SEMA_DRIVER, NULL);

	return (FLC_INIT(flcobjp, &(scdkp->scd_com), queobjp, lkarg));
}

static int
scdk_free(struct tgdk_obj *dkobjp)
{
	struct scdk *scdkp;

	scdkp = (struct scdk *)(dkobjp->tg_data);
	if (scdkp->scd_sd)
		scdkp->scd_sd->sd_private = NULL;
	if (scdkp->scd_flcobjp)
		FLC_FREE(scdkp->scd_flcobjp);
	if (scdkp->scd_cdobjp) {
		TGCD_FREE(scdkp->scd_cdobjp);
		objmgr_destroy_obj(scdkp->scd_cdname);
		objmgr_unload_obj(scdkp->scd_cdname);
	}
	sema_destroy(&scdkp->scd_drvsema);
	(void) kmem_free((caddr_t)dkobjp, (sizeof (*dkobjp) + sizeof (*scdkp)));
	return (0);
}


static int
scdk_probe(struct scdk *scdkp, int kmsflg)
{

	/* assert that all devices are block devices */
	scdkp->scd_extp->tg_nodetype = DDI_NT_BLOCK_CHAN;
	scdkp->scd_extp->tg_ctype = DKC_SCSI_CCS;
	return (DDI_PROBE_SUCCESS);
}

static int
scdk_attach(struct scdk *scdkp)
{
	struct scsi_device *devp = scdkp->scd_sd;

	/* do an INQUIRY to determine the type of attached device */
	if (!scdk_config(scdkp)) {
		scsi_unprobe(devp);
		return (DDI_FAILURE);
	}
	scsi_unprobe(devp);
	return (DDI_SUCCESS);
}


/*
 * scdk_config()
 *
 *	Determine what kind of device is attached and what it's
 *	capabilities are based on the response from the INQUIRY command.
 */

static int
scdk_config(struct scdk *scdkp)
{
	struct scsi_device *devp = scdkp->scd_sd;
	char   name[80];

	/*
	 * Always do a scsi_probe
	 */
	if (scsi_probe(devp, DDI_DMA_SLEEP) != SCSIPROBE_EXISTS) {
		return (FALSE);
	}

	switch (devp->sd_inq->inq_dtype) {
		case DTYPE_RODIRECT:
			scdkp->scd_ctype = DKC_CDROM;
			scdkp->scd_extp->tg_rdonly = 1;
			scdkp->scd_rdonly = 1;
			scdkp->scd_cdrom = 1;
			scdkp->scd_extp->tg_nodetype = DDI_NT_CD_CHAN;
			scdkp->scd_extp->tg_ctype = DKC_CDROM;
			break;
		case DTYPE_DIRECT:
		case DTYPE_OPTICAL:
			scdkp->scd_ctype = DKC_SCSI_CCS;
			scdkp->scd_extp->tg_nodetype = DDI_NT_BLOCK_CHAN;
			scdkp->scd_extp->tg_ctype = DKC_SCSI_CCS;
			break;
		case DTYPE_WORM:
		default:
			return (FALSE);
	}

	scdkp->scd_extp->tg_rmb = scdkp->scd_rmb = devp->sd_inq->inq_rmb;
	scdkp->scd_options = 0;
	scdkp->scd_pktflag = 0;

	scdkp->scd_secshf = SCTRSHFT;
	scdkp->scd_blkshf = 0;

/* 	enable autorequest sense					*/
	scdkp->scd_arq = ((scsi_ifsetcap(SCDK2ADDR(scdkp), "auto-rqsense",
			1, 1) == 1)? 1: 0);

/*	enable tagged queueing						*/
	if ((devp->sd_inq->inq_rdf == RDF_SCSI2) && (devp->sd_inq->inq_cmdque))
		scdkp->scd_tagque = ((scsi_ifsetcap(SCDK2ADDR(scdkp),
					"tagged-qing", 1, 1) == 1)? 1: 0);

/*	display the device name						*/
	strcpy(name, "Vendor '");
	scsi_inqfill((caddr_t)devp->sd_inq->inq_vid, 8, &name[strlen(name)]);
	strcat(name, "' Product '");
	scsi_inqfill((caddr_t)devp->sd_inq->inq_pid, 16, &name[strlen(name)]);
	strcat(name, "'");
	scsi_log(devp->sd_dev, scdk_name, CE_NOTE, "?<%s>\n", name);

#ifdef SCDK_DEBUG2
	if (scdk_debug & DENT) {
		PRF("scdk_config: ctype= %d rmb= %d secshf= 0x%x\n",
			scdkp->scd_ctype, scdkp->scd_rmb, scdkp->scd_secshf);
	}
#endif
	return (TRUE);
}


/*
 * scdk_clear_drive()
 *
 *	Send alternating TEST UNIT READ and REQUEST SENSE commands in
 *	order to clear any pending status.
 *
 */

static int
scdk_clear_drive(struct scdk *scdkp)
{
	int i;

#ifdef SCDK_DEBUG
	if (scdk_debug & DIO)
		PRF("scdk_clear_drive: TUR\n");
#endif

	/*
	 * follow Appendix F of SCSI-2 spec
	 * issue three TUR / REQSEN pairs to clear pending status.
	 * ignore error since there may be data over or under flow
	 * due to the sense length allocated
	 */
	for (i = 0; i < 3; i++) {
		/*
		 * Send TUR, but set the retries to zero so I
		 * can get the actual Request Sense response.
	 	 *
		 */
		if (scdk_test_unit(scdkp, 0)) {
			/* we don't need to send a start */
			return (TRUE);
		}

		/*
		 * Check for Selection Timeout or other fatal error
		 */

		if (!scdk_req_sense(scdkp)) {
			return (FALSE);
		}
	}
	return (TRUE);

}

/*
 * scdk_start_drive()
 *
 *	Send a START command with automatic retries
 *
 */
static int
scdk_start_drive(struct scdk *scdkp)
{
	struct scsi_pkt *pktp;

	/*
	 * send START_DEVICE
	 */
	if (!(pktp = scdk_consist_pkt(scdkp, 0, B_READ, CDB_GROUP0))) {
		return (FALSE);
	}

	GSC_START(pktp);
	pktp->pkt_flags |= FLAG_SILENT;
	SC_XPKTP(pktp)->x_retry = SD_OPEN_RETRY_COUNT;
	scdk_send_pkt(scdkp, pktp);

	/*
	 * Check for errors
	 */
	if (pktp->pkt_reason != CMD_CMPLT
	||  SCBP_C(pktp) != STATUS_GOOD) {
		scdk_pktfree(pktp, PKT_BP(pktp));
		return (FALSE);
	}

	/* free pkt and buf */
	scdk_pktfree(pktp, PKT_BP(pktp));
	return (TRUE);
}

/*
 * scdk_test_unit()
 *
 *	Send a Test Unit Ready command with option retries
 *
 */
static int
scdk_test_unit(struct scdk *scdkp, int retries)
{
	struct scsi_pkt *pktp;


	/*
	 * send TEST_UNIT_READY CDB
	 */
	if (!(pktp = scdk_consist_pkt(scdkp, 0, B_READ, CDB_GROUP0))) {
		return (FALSE);
	}

	GSC_TESTUNIT(pktp);
	pktp->pkt_flags |= FLAG_SILENT;
	SC_XPKTP(pktp)->x_retry = retries;
	scdk_send_pkt(scdkp, pktp);

	/*
	 * Check for errors 
	 */
	if (pktp->pkt_reason != CMD_CMPLT
	||  SCBP_C(pktp) != STATUS_GOOD) {
		scdk_pktfree(pktp, PKT_BP(pktp));
		return (FALSE);
	}

	/* free pkt and buf */
	scdk_pktfree(pktp, PKT_BP(pktp));
	return (TRUE);
}

/*
 * scdk_req_sense()
 *
 *	Send Request Sense command to retrieve the Sense Data
 *
 */

static int
scdk_req_sense(struct scdk *scdkp)
{
	struct scsi_pkt *pktp;

	if (!(pktp = scdk_consist_pkt(scdkp, SENSE_LENGTH, B_READ,
					CDB_GROUP0))) {
		return (FALSE);
	}

#ifdef SCDK_DEBUG
	if (scdk_debug & DIO)
		PRF("scdk_req_sense: REQSEN\n");
#endif
	GSC_REQSEN(pktp);
	pktp->pkt_flags |= FLAG_SILENT;
	SC_XPKTP(pktp)->x_retry = SD_OPEN_RETRY_COUNT;
	scdk_send_pkt(scdkp, pktp);

	if (pktp->pkt_reason == CMD_TIMEOUT) {
		/* this drive is offline */
		scdk_pktfree(pktp, PKT_BP(pktp));
		return (FALSE);
	}

	/* free pkt and buf */
	scdk_pktfree(pktp, PKT_BP(pktp));
	return (TRUE);
}

/*
 * scdk_media_lock()
 *
 * 	Lock the door if the it has remove-able media.  Don't check
 *	for any errors???
 *
 */
static void
scdk_media_lock(struct scdk *scdkp)
{
	struct scsi_pkt *pktp;

#ifdef SCDK_DEBUG
	if (scdk_debug & DIO)
		PRF("scdk_media_lock: LOCK\n");
#endif

	if (!(pktp = scdk_consist_pkt(scdkp, 0, B_READ, CDB_GROUP0))) {
		return;
	}

	GSC_LOCK(pktp);
	pktp->pkt_flags |= FLAG_SILENT;
	SC_XPKTP(pktp)->x_retry = SD_OPEN_RETRY_COUNT;
	scdk_send_pkt(scdkp, pktp);

	/* free pkt and buf */
	scdk_pktfree(pktp, PKT_BP(pktp));
	return;
}

/*
 * scdk_read_capacity()
 *
 *	Get device capacity and save it for later
 *
 */

static int
scdk_read_capacity(struct scdk *scdkp)
{
	struct scsi_pkt *pktp;

#ifdef SCDK_DEBUG
	if (scdk_debug & DIO)
		PRF("scdk_read_capacity: RDCAP\n");
#endif

	if (!(pktp = scdk_consist_pkt(scdkp, sizeof (struct scsi_capacity),
				B_READ, CDB_GROUP1))) {
		return (FALSE);
	}

	GSC_RDCAP(pktp);
	SC_XPKTP(pktp)->x_retry = SCDK_RTYCNT;
	scdk_send_pkt(scdkp, pktp);

	if (pktp->pkt_reason != CMD_CMPLT
	||  SCBP_C(pktp) != STATUS_GOOD) {
		/* return error if Read Capacity failed */
		scdk_pktfree(pktp, PKT_BP(pktp));
		return (FALSE);
	}
	/*
	 * save the info
	 */
	scdk_setcap(scdkp, pktp);

	scdk_pktfree(pktp, PKT_BP(pktp));
	return (TRUE);
}

/*
 * scdk_consist_pkt()
 *
 *	Allocate a consistent buffer and scsi packet
 *
 */

static struct scsi_pkt *
scdk_consist_pkt(struct scdk *scdkp, int datalen, ulong bflags, int cdblen)
{
	struct buf *bp;
	struct scsi_pkt *pktp;

	if ((bp = scsi_alloc_consistent_buf(SDEV2ADDR(scdkp->scd_sd),
		NULL, datalen, bflags, DDI_DMA_SLEEP, NULL)) == NULL)
		return (NULL);

	pktp = scdk_pktprep(scdkp, NULL, bp, cdblen, scdk_consist_iodone,
			    PKT_CONSISTENT, DDI_DMA_SLEEP, NULL);
	if (!pktp) {
		scsi_free_consistent_buf(bp);
		return (NULL);
	}
	return (pktp);
}

/*
 * scdk_send_pkt()
 *
 *	I/O start routine for internally generated scsi CDBs which
 *	return data in the buf or status info in the scsi packet
 *	allocated by scdk_consist_pkt().
 *
 */

static void
scdk_send_pkt(struct scdk *scdkp, struct scsi_pkt *pktp)
{
	struct buf *bp = PKT_BP(pktp);

	bp->b_flags |= B_BUSY;

	/* add it to the device queue */
	FLC_ENQUE(scdkp->scd_flcobjp, bp);

	/* wait for it to complete */
	biowait(bp);
	bp->b_flags &= ~(B_DONE|B_BUSY);
}

/*
 * scdk_consist_iodone()
 *
 *	I/O completion routine for consistent bufs started
 *	via scdk_send_pkt()
 *
 */

static void
scdk_consist_iodone(register struct buf *bp)
{
	register struct	scsi_pkt *pktp;
	register struct	scdk *scdkp;

	pktp  = BP_PKT(bp);
	scdkp = PKT2SCDK(pktp);

	/* start next one from the queue */
	FLC_DEQUE(scdkp->scd_flcobjp, bp);

	/*
	 * mark the buf done but don't free the pkt so the status
	 * info is preserved
	 */
	biodone(bp);
}

/*ARGSUSED*/
static int
scdk_open(struct scdk *scdkp, int flag)
{
	struct scsi_device *devp = scdkp->scd_sd;
	int		i;



	/*
	 * Just return if the drive is already started.
	 */
	if (SCDK_DRIVE_READY(scdkp)) {
		/* if it's really ready just return */
		if (scdk_test_unit(scdkp, 0)) {
			FLC_START_KSTAT(scdkp->scd_flcobjp, "disk",
				ddi_get_instance(scdkp->scd_sd->sd_dev));
			return (DDI_SUCCESS);
		}
		/* The drive fell offline, restart it */
	}

	/*
	 * do the INQUIRY to determine the type of attached device
	 */
	if (!scdk_config(scdkp))
		goto error_exit;

	if (scdkp->scd_rmb) {
		mutex_enter(&scdkp->scd_mutex);
		scdkp->scd_iostate = DKIO_NONE;
		cv_broadcast(&scdkp->scd_state_cv);
		mutex_exit(&scdkp->scd_mutex);
	}

	/*
	 * clear any pending status
	 */
	if (!scdk_clear_drive(scdkp))
		goto error_exit;

	/*
	 * spin up the media
	 */
	if (!scdk_start_drive(scdkp))
		goto error_exit;

	/*
	 * prevent media removal; ignore failures since the command
	 * is optional even for CDROM drives.
	 */
	if (scdkp->scd_rmb)
		scdk_media_lock(scdkp);

	/*
	 * Check if it's ready
	 */
	if (!scdk_test_unit(scdkp, SD_OPEN_RETRY_COUNT))
		goto error_exit;

	/* XXX redo inquiry here by calling unprobe/probe */
	scsi_unprobe(devp);
	scsi_probe(devp, KM_SLEEP);

	if (!scdk_read_capacity(scdkp))
		goto error_exit;

	/* set drive state and force re-read of labels */
	SCDK_SET_DRIVE_READY(scdkp, TRUE);

	mutex_enter(&scdkp->scd_mutex);
	scdkp->scd_iostate = DKIO_INSERTED;
	cv_broadcast(&scdkp->scd_state_cv);
	mutex_exit(&scdkp->scd_mutex);

/*	start profiling						*/
	FLC_START_KSTAT(scdkp->scd_flcobjp, "disk",
		ddi_get_instance(scdkp->scd_sd->sd_dev));

/*	stop here if CDrom device				*/
	if (scdkp->scd_cdrom)
		return (DDI_SUCCESS);

/*	get physical disk geometry				*/
	(void) scdk_phyg(scdkp);
	return (DDI_SUCCESS);


	/*
	 * Bail out due to some uncorrectable an error.
	 */

error_exit:
	SCDK_SET_DRIVE_READY(scdkp, FALSE);
	return (DDI_FAILURE);
}

/*
 *	set physical geometry
 */
static int
scdk_phyg(struct scdk *scdkp)
{
	struct scsi_pkt *pktp;
	struct scsi_pkt *repktp;
	struct buf 	*bp;
	struct mode_format *page3p;
	struct mode_geometry *page4p;
	int		i;
	long 		spc;
	long 		sec;
	long 		secsiz;
	long		capacity;
	long 		cyl;
	long		head;

#ifdef SCDK_DEBUG
	if (scdk_debug & DIO)
		PRF("scdk_phyg: MODE-SENSE PAGE 3\n");
#endif

	/* Allocate a scsi packet. */
	if (!(pktp = scdk_consist_pkt(scdkp,
			MODE_PARAM_LENGTH + sizeof (struct mode_format),
			B_READ, CDB_GROUP0))) {
		return (DDI_FAILURE);
	}
	bp = PKT_BP(pktp);

	/* get page 3 info - sectors per track */

	GSC_DK_MSEN(pktp, DAD_MODE_FORMAT);
	pktp->pkt_flags |= FLAG_SILENT;
	SC_XPKTP(pktp)->x_retry = SCDK_RTYCNT;

	scdk_send_pkt(scdkp, pktp);
	if (pktp->pkt_reason != CMD_CMPLT
	||  SCBP_C(pktp) != STATUS_GOOD) {
		scdk_pktfree(pktp, bp);
		return (DDI_FAILURE);
	}

	page3p = (struct mode_format *)(bp->b_un.b_addr + MODE_PARAM_LENGTH);
	if (page3p->mode_page.code != DAD_MODE_FORMAT)  {
		scdk_pktfree(pktp, bp);
		return (DDI_FAILURE);
	}
	sec    = (long)scsi_stoh_short(page3p->sect_track);
	secsiz = (long)scsi_stoh_short(page3p->data_bytes_sect);
	scdk_pktfree(pktp, bp);


#ifdef SCDK_DEBUG
	if (scdk_debug & DIO)
		PRF("scdk_phyg: MODE-SENSE PAGE 4\n");
#endif

	/* Allocate a scsi packet. */
	if (!(pktp = scdk_consist_pkt(scdkp,
			MODE_PARAM_LENGTH + sizeof (struct mode_format),
			B_READ, CDB_GROUP0))) {
		return (DDI_FAILURE);
	}
	bp = PKT_BP(pktp);

	/* get page 4 info - cylinders and heads */
	GSC_DK_MSEN(pktp, DAD_MODE_GEOMETRY);
	pktp->pkt_flags |= FLAG_SILENT;
	SC_XPKTP(pktp)->x_retry = SCDK_RTYCNT;

	scdk_send_pkt(scdkp, pktp);
	if (pktp->pkt_reason != CMD_CMPLT
	||  SCBP_C(pktp) != STATUS_GOOD) {
		scdk_pktfree(pktp, bp);
		return (DDI_FAILURE);
	}

	page4p = (struct mode_geometry *)(bp->b_un.b_addr + MODE_PARAM_LENGTH);
	if (page4p->mode_page.code != DAD_MODE_GEOMETRY)  {
		scdk_pktfree(pktp, bp);
		return (DDI_FAILURE);
	}

	head   = (long)page4p->heads;
	spc    = head * sec;
	cyl    = (page4p->cyl_ub<<16) + (page4p->cyl_mb<<8) + page4p->cyl_lb;
	capacity = spc * cyl;

/*	check for valid physical geometry				*/
	if (capacity >= scdkp->scd_phyg.g_cap) {
		if (secsiz != 0) {
			/* 1243403: The NEC D38x7 drives don't support secsiz */
			scdkp->scd_phyg.g_secsiz = secsiz;
		}
		scdkp->scd_phyg.g_sec    = sec;
		scdkp->scd_phyg.g_head   = head;
		scdkp->scd_phyg.g_acyl   = (capacity - scdkp->scd_phyg.g_cap
					    + spc - 1)/spc;
		scdkp->scd_phyg.g_cap    = capacity;
		scdkp->scd_phyg.g_cyl    = cyl - scdkp->scd_phyg.g_acyl;
	}


#ifdef SCDK_DEBUG
	if (scdk_debug & DGEOM) {
		PRF("scdk_phyg: \n");
		scdk_prtgeom(scdkp);
	}
#endif

	scdk_pktfree(pktp, bp);
	return (DDI_SUCCESS);
}

/*
 *	set device geometry
 */
static void
scdk_setcap(struct scdk *scdkp, struct scsi_pkt *pktp)
{
	register int	 totsize;
	register struct	 scsi_capacity *rcdp;
	register int	 spc;
	struct	 buf *bp;
	int	 i;

	bp = PKT_BP(pktp);
	rcdp = (struct scsi_capacity *)bp->b_un.b_addr;

/*	get physical sector size					*/
	totsize = scsi_stoh_long(rcdp->lbasize);

	if (totsize == 0) {
		if (scdkp->scd_cdrom)
			totsize = 2048;
		else
			totsize = NBPSCTR;
	} else
		totsize &= ~(NBPSCTR-1);
	scdkp->scd_phyg.g_secsiz = totsize;

/*	set sec,block shift factor - (512->0, 1024->1, 2048->2, etc.)	*/
	totsize >>= SCTRSHFT;
	for (i = 0; totsize > 1; i++, totsize >>= 1);
	scdkp->scd_blkshf = i;
	scdkp->scd_secshf = i + SCTRSHFT;

	/*
	 * bug 1175930: correct missing last block on cdrom without
	 * disturbing current calculation of hard drive geometry.
	 * This is a kludge used because the risk of changing
	 * geometry calculation {see scdk_phyg()} of hard drives
	 * in corner cases for 2.4, or 2.5 upgrade from 2.4, is
	 * too high. The right fix is to change snlb to return
	 * capacity + 1 as partition size, and investigate all places
	 * where capacity is used, especially where geometry is
	 * calculated and compared to capacity.
	 */
	if (scdkp->scd_cdrom)
		scdkp->scd_phyg.g_cap = scsi_stoh_long(rcdp->capacity) + 1;
	else
		scdkp->scd_phyg.g_cap = scsi_stoh_long(rcdp->capacity);

/*	set sector size							*/
	scsi_ifsetcap(SCDK2ADDR(scdkp), "sector-size", scdkp->scd_phyg.g_secsiz,
		1);

/*	set total sectors						*/
	scsi_ifsetcap(SCDK2ADDR(scdkp), "total-sectors", scdkp->scd_phyg.g_cap,
		1);

/*	get logical geometry (head,sec per track)			*/
	i = scsi_ifgetcap(SCDK2ADDR(scdkp), "geometry", 1);

	if (i == UNDEFINED)
		return;

/*	set physical geometry						*/
	if (scdkp->scd_cdrom) {
		scdkp->scd_phyg.g_head = 1;
		scdkp->scd_phyg.g_sec  = SCDK_GETGEOM_HEAD(i) *
			SCDK_GETGEOM_SEC(i);
	} else {
		scdkp->scd_phyg.g_head = SCDK_GETGEOM_HEAD(i);
		scdkp->scd_phyg.g_sec  = SCDK_GETGEOM_SEC(i);
	}
	spc = scdkp->scd_phyg.g_head * scdkp->scd_phyg.g_sec;
	scdkp->scd_phyg.g_cyl    = scdkp->scd_phyg.g_cap / spc;
	scdkp->scd_phyg.g_acyl   = 0;

/*	set logical geometry						*/
	scdkp->scd_logg.g_head   = SCDK_GETGEOM_HEAD(i);
	scdkp->scd_logg.g_sec    = SCDK_GETGEOM_SEC(i);
	scdkp->scd_logg.g_secsiz = NBPSCTR;

	scdkp->scd_logg.g_cap    = scdkp->scd_phyg.g_cap << scdkp->scd_blkshf;
	spc = scdkp->scd_logg.g_head * scdkp->scd_logg.g_sec;
	scdkp->scd_logg.g_cyl    = scdkp->scd_logg.g_cap / spc;
	scdkp->scd_logg.g_acyl   = 0;

#ifdef SCDK_DEBUG
	if (scdk_debug & DGEOM) {
		PRF("scdk_setcap: \n");
		scdk_prtgeom(scdkp);
	}
#endif
}

static int
scdk_close(struct scdk *scdkp)
{
	register struct scsi_pkt *pktp;
	register struct buf 	*bp;

	FLC_STOP_KSTAT(scdkp->scd_flcobjp);

	if (!scdkp->scd_rmb)
		return (DDI_SUCCESS);

#ifdef SCDK_DEBUG
	if (scdk_debug & DIO)
		PRF("scdk_close: STOP\n");
#endif

	/* Allocate a consistent buffer and scsi packet */
	if (!(pktp = scdk_consist_pkt(scdkp, 0, B_READ, CDB_GROUP0))) {
		goto done2;
	}
	bp = PKT_BP(pktp);

	SC_XPKTP(pktp)->x_retry = 0;
	pktp->pkt_flags |= FLAG_SILENT;

	/*
	 * stop device if CDROM or removable
	 */
	GSC_STOP(pktp);
	scdk_send_pkt(scdkp, pktp);
	if ((pktp->pkt_reason != CMD_CMPLT) ||
	    (SCBP_C(pktp) != STATUS_GOOD)) {
		goto done1;
	}
	scdk_pktfree(pktp, bp);

	/*
	 * allow media removal
	 */
#ifdef SCDK_DEBUG
	if (scdk_debug & DIO)
		PRF("scdk_close: UNLOCK\n");
#endif

	/* Allocate a new consistent buffer and scsi packet */
	if (!(pktp = scdk_consist_pkt(scdkp, 0, B_READ, CDB_GROUP0))) {
		goto done2;
	}
	bp = PKT_BP(pktp);

	SC_XPKTP(pktp)->x_retry = 0;
	pktp->pkt_flags |= FLAG_SILENT;
	GSC_UNLOCK(pktp);
	scdk_send_pkt(scdkp, pktp);

done1:
	scdk_pktfree(pktp, bp);

done2:
	/* force restart and re-read of labels on next access */
	SCDK_SET_DRIVE_READY(scdkp, FALSE);
	return (DDI_SUCCESS);
}

static int
scdk_strategy(register struct scdk *scdkp, register struct buf *bp)
{
	if (scdkp->scd_rdonly && !(bp->b_flags & B_READ)) {
		SETBPERR(bp, EROFS);
		return (DDI_FAILURE);
	}

	if (bp->b_bcount & (scdkp->SCD_SECSIZ-1)) {
		SETBPERR(bp, ENXIO);
		return (DDI_FAILURE);
	}

	SET_BP_SEC(bp, (LBLK2SEC(GET_BP_SEC(bp), scdkp->scd_blkshf)));
	FLC_ENQUE(scdkp->scd_flcobjp, bp);

	return (DDI_SUCCESS);
}

static int
scdk_dump(struct scdk *scdkp, struct buf *bp)
{
	struct scsi_pkt *pktp;
#ifdef XYZ
	struct scsi_address *ap;
#endif
	int cdblen;

	if (scdkp->scd_rdonly) {
		SETBPERR(bp, EROFS);
		return (DDI_FAILURE);
	}

	if (bp->b_bcount & (scdkp->SCD_SECSIZ-1)) {
		SETBPERR(bp, ENXIO);
		return (DDI_FAILURE);
	}

#ifdef XYZ
	ap = SCDK2ADDR(scdkp);
	scsi_abort(ap, (struct scsi_pkt *)0);
	if (scsi_reset(ap, RESET_ALL) == 0)
		return (EIO);
	drv_usecwait(2*1000000);
#endif

	SET_BP_SEC(bp, (LBLK2SEC(GET_BP_SEC(bp), scdkp->scd_blkshf)));

	if (GET_BP_SEC(bp) >= (2 << 20) || (bp->b_bcount > 0xff))
		cdblen = CDB_GROUP1;
	else
		cdblen = CDB_GROUP0;
	pktp = scdk_pktprep(scdkp, NULL, bp, cdblen, scdk_polldone,
		0, NULL, NULL);
	if (!pktp) {
		cmn_err(CE_WARN, "no resources for dumping");
		SETBPERR(bp, EIO);
		return (DDI_FAILURE);
	}
	pktp->pkt_flags |= FLAG_NOINTR|FLAG_NODISCON;

	(void) scdk_ioprep(scdkp, pktp, SCDK_IOSTART);
	scdk_transport(scdkp, bp);

	while (!(bp->b_flags & B_ERROR) && (SC_XPKTP(pktp)->x_byteleft)) {
		scsi_init_pkt(SCDK2ADDR(scdkp), pktp, bp, 0, 0, 0, 0,
			DDI_DMA_DONTWAIT, NULL);
/*		move the residue bytes back into private area		*/
		SC_XPKTP(pktp)->x_byteleft = pktp->pkt_resid;
		pktp->pkt_resid = 0;

		(void) scdk_ioprep(scdkp, pktp, SCDK_IOCONT);

/*		transport the next one					*/
		scdk_transport(scdkp, bp);
	}

	scdk_pktfree(pktp, NULL);
	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
scdk_ioctl(struct scdk *scdkp, dev_t dev, int cmd, int arg, int flag,
	cred_t *cred_p, int *rval_p)
{
	switch (cmd) {
	case USCSICMD:
		return (scdk_uscsi_ioctl(scdkp, dev, arg, flag));
	case CDROMSTOP:
		return (scdk_start_stop(scdkp, dev, ((unsigned char)0)));
	case CDROMSTART:
		return (scdk_start_stop(scdkp, dev, ((unsigned char)1)));

	case DKIOCLOCK:
		return (scdk_lock_unlock(scdkp, dev, ((unsigned char)1)));

	case DKIOCUNLOCK:
		return (scdk_lock_unlock(scdkp, dev, ((unsigned char)0)));

	case DKIOCEJECT:
	case CDROMEJECT:
		return (scdk_eject(scdkp, dev));
	case CDROMPAUSE:
	case CDROMRESUME:
	case CDROMPLAYMSF:
	case CDROMPLAYTRKIND:
	case CDROMREADTOCHDR:
	case CDROMREADTOCENTRY:
	case CDROMVOLCTRL:
	case CDROMSUBCHNL:
	case CDROMREADMODE2:
	case CDROMREADMODE1:
	case CDROMREADOFFSET:
		if (!scdkp->scd_cdrom)
			return (ENOTTY);
		return (scdkp->scd_cdioctl(scdkp, dev, cmd, arg, flag));
	default:
		return (ENOTTY);
	}
}

static int
scdk_getphygeom(struct scdk *scdkp, struct tgdk_geom *dkgeom_p)
{
	bcopy((caddr_t)&scdkp->scd_phyg, (caddr_t)dkgeom_p,
		sizeof (struct tgdk_geom));
	return (DDI_SUCCESS);
}

static int
scdk_getgeom(struct scdk *scdkp, struct tgdk_geom *dkgeom_p)
{
	bcopy((caddr_t)&scdkp->scd_logg, (caddr_t)dkgeom_p,
		sizeof (struct tgdk_geom));
	return (DDI_SUCCESS);
}

static int
scdk_setgeom(struct scdk *scdkp, struct tgdk_geom *dkgeom_p)
{
	scdkp->scd_logg.g_cyl = dkgeom_p->g_cyl;
	scdkp->scd_logg.g_head = dkgeom_p->g_head;
	scdkp->scd_logg.g_sec  = dkgeom_p->g_sec;
	scdkp->scd_logg.g_cap  = dkgeom_p->g_cap;
	return (DDI_SUCCESS);
}


static tgdk_iob_handle
scdk_iob_alloc(struct scdk *scdkp, daddr_t blkno, long xfer, int kmsflg)
{
	struct 	buf *bp;
	struct 	tgdk_iob *iobp;

	iobp = kmem_zalloc(sizeof (*iobp), kmsflg);
	if (iobp == NULL)
		return (NULL);
	if ((bp = getrbuf(kmsflg)) == NULL) {
		kmem_free(iobp, sizeof (*iobp));
		return (NULL);
	}

	iobp->b_psec  = LBLK2SEC(blkno, scdkp->scd_blkshf);
	iobp->b_pbyteoff = (blkno & ((1<<scdkp->scd_blkshf) - 1)) << SCTRSHFT;
	iobp->b_pbytecnt = ((iobp->b_pbyteoff + xfer + scdkp->SCD_SECSIZ - 1)
				>> scdkp->scd_secshf) << scdkp->scd_secshf;

	bp->b_un.b_addr = 0;
	if (ddi_iopb_alloc((scdkp->scd_sd)->sd_dev,
			(ddi_dma_lim_t *)0, (u_int)iobp->b_pbytecnt,
			&bp->b_un.b_addr)) {
		freerbuf(bp);
		kmem_free(iobp, sizeof (*iobp));
		return (NULL);
	}
	iobp->b_flag |= IOB_BPALLOC | IOB_BPBUFALLOC;

	iobp->b_bp = bp;
	iobp->b_lblk = blkno;
	iobp->b_xfer = xfer;
	return (iobp);

}

/*ARGSUSED*/
static int
scdk_iob_free(struct scdk *scdkp, struct tgdk_iob *iobp)
{
	struct 	buf *bp;

	if (iobp) {
		if (iobp->b_bp && (iobp->b_flag & IOB_BPALLOC)) {
			bp = iobp->b_bp;
			if (bp->b_un.b_addr && (iobp->b_flag & IOB_BPBUFALLOC))
				ddi_iopb_free((caddr_t)bp->b_un.b_addr);
			freerbuf(bp);
		}
		kmem_free(iobp, sizeof (*iobp));
	}
	return (0);
}

/*ARGSUSED*/
static caddr_t
scdk_iob_htoc(struct scdk *scdkp, struct tgdk_iob *iobp)
{
	return (iobp->b_bp->b_un.b_addr+iobp->b_pbyteoff);
}

static caddr_t
scdk_iob_xfer(struct scdk *scdkp, struct tgdk_iob *iobp, int rw)
{
	struct	buf *bp;
	int	err;

	bp = iobp->b_bp;
	if (scdkp->scd_rdonly && !(rw & B_READ)) {
		SETBPERR(bp, EROFS);
		return (NULL);
	}

	bp->b_flags |= B_BUSY | rw;
	bp->b_bcount = iobp->b_pbytecnt;
	SET_BP_SEC(bp, iobp->b_psec);
	bp->av_back = (struct buf *)0;
	bp->b_resid = 0;

/*	call flow control						*/
	FLC_ENQUE(scdkp->scd_flcobjp, bp);
	err = biowait(bp);

	bp->b_bcount = iobp->b_xfer;
	bp->b_flags &= ~(B_DONE|B_BUSY);

	if (err)
		return (NULL);

	return (bp->b_un.b_addr+iobp->b_pbyteoff);
}

/*ARGSUSED*/
static void
scdk_transport(register struct scdk *scdkp, register struct buf *bp)
{
	if (scsi_transport(BP_PKT(bp)) == TRAN_ACCEPT)
		return;
	scdk_restart(BP_PKT(bp));
}

static int
scdk_pkt(register struct scdk *scdkp, struct buf *bp, int (*func)(),
	caddr_t arg)
{
	register struct scsi_pkt *pktp;
	register int cdblen;

	if (BP_PKT(bp))
		return (DDI_SUCCESS);

	if ((GET_BP_SEC(bp) + bp->b_bcount - 1) >= (2 << 20) ||
		(bp->b_bcount > 0xff))
		cdblen = CDB_GROUP1;
	else
		cdblen = CDB_GROUP0;

	pktp = scdk_pktprep(scdkp, NULL, bp, cdblen,
		scdk_iodone, PKT_DMA_PARTIAL, func, arg);
	if (!pktp)
		return (DDI_FAILURE);
	return (scdk_ioprep(scdkp, pktp, SCDK_IOSTART));
}

/*
 * 	Read, Write preparation
 */
static int
scdk_ioprep(register struct scdk *scdkp, register struct scsi_pkt *pktp,
	int cmd)
{
	register struct	buf *bp;

	bp = PKT_BP(pktp);
	if (cmd == SCDK_IOSTART) {
		SC_XPKTP(pktp)->x_srtsec = GET_BP_SEC(bp);
	} else {
		SC_XPKTP(pktp)->x_srtsec += SC_XPKTP(pktp)->x_seccnt;
		SC_XPKTP(pktp)->x_tot_bytexfer += SC_XPKTP(pktp)->x_bytexfer;
		SC_XPKTP(pktp)->x_bytexfer = bp->b_bcount -
				SC_XPKTP(pktp)->x_byteleft -
				SC_XPKTP(pktp)->x_tot_bytexfer;
	}

	SC_XPKTP(pktp)->x_seccnt = SC_XPKTP(pktp)->x_bytexfer >>
					scdkp->scd_secshf;

	if (SC_XPKTP(pktp)->x_cdblen == CDB_GROUP1) {
		if (bp->b_flags & B_READ) {
			GSC_DK_SXREAD(pktp);
		} else {
			GSC_DK_SXWRITE(pktp);
		}
	} else {
		if (bp->b_flags & B_READ) {
			GSC_DK_SCREAD(pktp);
		} else {
			GSC_DK_SCWRITE(pktp);
		}
	}
	return (DDI_SUCCESS);
}

static void
scdk_pktfree(register struct scsi_pkt *pktp, register struct buf *bp)
{
	register struct scsi_pkt *fltpktp;
	struct buf *fltbp;

	if (!pktp)
		return;

	fltpktp = SC_XPKTP(pktp)->x_fltpktp;
	if (fltpktp) {
		if (SC_XPKTP(fltpktp)->x_rqsbp &&
			(SC_XPKTP(fltpktp)->x_flags & PKT_CONSISTENT)) {
			fltbp = SC_XPKTP(fltpktp)->x_rqsbp;
			scsi_destroy_pkt(fltpktp);
			scsi_free_consistent_buf(fltbp);
		} else
			scsi_destroy_pkt(fltpktp);
	}
	if (bp && (SC_XPKTP(pktp)->x_flags & PKT_CONSISTENT)) {
		scsi_destroy_pkt(pktp);
		scsi_free_consistent_buf(bp);
	} else
		scsi_destroy_pkt(pktp);

}

static struct scsi_pkt *
scdk_pktprep(struct scdk *scdkp, struct scsi_pkt **in_pktp, struct buf *bp,
    int cdblen, void (*cb_func)(), int flags, int (*func)(), caddr_t arg)
{
	register struct scsi_pkt *pktp;
	register struct scsi_pkt *inpkt;

	/*
	 * If we're reusing a previously allocated pkt, but
	 * the cdb length doesn't match, we must allocate
	 * a new pkt instead.
	 */
	if (in_pktp && *in_pktp) {
		if (SC_XPKTP(*in_pktp)->x_cdblen == cdblen) {
			inpkt = *in_pktp;
		} else {
			inpkt = NULL;
		}
	} else {
		inpkt = NULL;
	}

	pktp = scsi_init_pkt(SCDK2ADDR(scdkp), inpkt, bp,
			cdblen, SECMDS_STATUS_SIZE, PKT_PRIV_LEN,
			flags, func, arg);

	if (!pktp)
		return (NULL);

	/*
	 * If we're allocating a new pkt to replace the previous...
	 */
	if (in_pktp && *in_pktp && inpkt == NULL) {
		pktp->pkt_flags |= (*in_pktp)->pkt_flags;
		bcopy((caddr_t)SC_XPKTP(*in_pktp),
			(caddr_t)SC_XPKTP(pktp),
				sizeof (struct target_private));
		scsi_destroy_pkt(*in_pktp);
		*in_pktp = pktp;
	}

	/*
	 * Some CDROM drives are very slow to start up, allow
	 * them twice as long as other device types.
	 */
	if (scdkp->scd_rmb)
		pktp->pkt_time = sd_io_time_rmb;
	else
		pktp->pkt_time = sd_io_time;

	pktp->pkt_comp = scdk_pktcb;

/*	move the residue bytes back into private area			*/
	SC_XPKTP(pktp)->x_byteleft = pktp->pkt_resid;
	pktp->pkt_resid = 0;

	SC_XPKTP(pktp)->x_tot_bytexfer = 0;
	SC_XPKTP(pktp)->x_bytexfer = bp->b_bcount -
		SC_XPKTP(pktp)->x_byteleft;

	SC_XPKTP(pktp)->x_sdevp    = (opaque_t)scdkp;
	SC_XPKTP(pktp)->x_callback = cb_func;
	SC_XPKTP(pktp)->x_retry    = sd_retry_count;
	SC_XPKTP(pktp)->x_bp	   = bp;
	SC_XPKTP(pktp)->x_flags    = flags;
	bp->av_back = (struct buf *)pktp;

	SC_XPKTP(pktp)->x_cdblen = (u_short)cdblen;

	return (pktp);
}

static void
scdk_restart(struct scsi_pkt *pktp)
{
	if (scdk_ioretry(pktp, QUE_COMMAND) == JUST_RETURN)
		return;

	SC_XPKTP(pktp)->x_callback(PKT_BP(pktp));
}

static int
scdk_ioretry(register struct scsi_pkt *pktp, int action)
{
	struct  buf *bp;
	struct 	scsi_device *sd;

	switch (action) {
	case QUE_SENSE:
		pktp = SC_XPKTP(pktp)->x_fltpktp;
		/*FALLTHROUGH*/
	case QUE_COMMAND:
		if (SC_XPKTP(pktp)->x_retry) {
			SC_XPKTP(pktp)->x_retry--;
			if (scsi_transport(pktp) == TRAN_ACCEPT)
				return (JUST_RETURN);
			if (SC_XPKTP(pktp)->x_retry) {
				(void) timeout(scdk_restart, (caddr_t)pktp,
					SD_BSY_TIMEOUT);
				return (JUST_RETURN);
			}
			sd = PKT2SCDK(pktp)->scd_sd;
			if (!(pktp->pkt_flags & FLAG_SILENT))
			    scsi_log(sd->sd_dev, scdk_name, CE_WARN,
				"!transport of command fails after retries\n");
		}
		bp = PKT_BP(pktp);
		bp->b_error = ENXIO;
		/*FALLTHROUGH*/
	case COMMAND_DONE_ERROR:
		bp = PKT_BP(pktp);
		bp->b_resid += bp->b_bcount;
		bp->b_flags |= B_ERROR;
		/*FALLTHROUGH*/
	case COMMAND_DONE:
	default:
		return (COMMAND_DONE);
	}
}

/*ARGSUSED*/
static void
scdk_polldone(struct buf *bp)
{
}

static void
scdk_iodone(register struct buf *bp)
{
	register struct	scsi_pkt *pktp;
	register struct	scdk *scdkp;

	pktp  = BP_PKT(bp);
	scdkp = PKT2SCDK(pktp);

/*	check for all iodone						*/
	if (!(bp->b_flags & B_ERROR) && (SC_XPKTP(pktp)->x_byteleft)) {
		scsi_init_pkt(SCDK2ADDR(scdkp), pktp, bp, 0, 0, 0, 0,
			DDI_DMA_DONTWAIT, NULL);
/*		move the residue bytes back into private area		*/
		SC_XPKTP(pktp)->x_byteleft = pktp->pkt_resid;
		pktp->pkt_resid = 0;

		(void) scdk_ioprep(scdkp, pktp, SCDK_IOCONT);

/*		transport the next one					*/
		if (scsi_transport(pktp) == TRAN_ACCEPT)
			return;
		if ((scdk_ioretry(pktp, QUE_COMMAND)) == JUST_RETURN)
			return;
	}

/*	start next one							*/
	FLC_DEQUE(scdkp->scd_flcobjp, bp);

/*	free pkt							*/
	scdk_pktfree(pktp, NULL);
	biodone(bp);
}

static void
scdk_pktcb(register struct scsi_pkt *pktp)
{
	register int 	action;

#ifdef SCDK_DEBUG2
	if (scdk_debug & DENT)
		PRF("scdk_pktcb: pktp= 0x%x \n", pktp);
#endif

	switch (pktp->pkt_reason) {
	case CMD_CMPLT:
		if (SCBP_C(pktp) == STATUS_GOOD) {
			SC_XPKTP(pktp)->x_callback(PKT_BP(pktp));
			return;
		}
		action = scdk_chkerr(pktp);
		break;

	case CMD_ABORTED:
		action = QUE_COMMAND;
		break;

	case CMD_DATA_OVR:
		if (pktp->pkt_resid) {
			PKT_BP(pktp)->b_error = 0;
			PKT_BP(pktp)->b_resid += pktp->pkt_resid;
			action =  COMMAND_DONE;
			break;
		}
		/*FALLTHROUGH*/
	default:
		action = scdk_incomplete(pktp);
	}

	if (action == JUST_RETURN)
		return;

	if (action != COMMAND_DONE) {
		if ((scdk_ioretry(pktp, action)) == JUST_RETURN)
			return;
	}
	SC_XPKTP(pktp)->x_callback(PKT_BP(pktp));
}

static void
scdk_pktcbrs(register struct scsi_pkt *fltpktp)
{
	register int 	action;
	register struct	scsi_pkt *pktp;

#ifdef SCDK_DEBUG2
	if (scdk_debug & DENT)
		PRF("scdk_pktcbrs: fltpktp= 0x%x \n", fltpktp);
#endif
	pktp = SC_XPKTP(fltpktp)->x_fltpktp;
	if ((fltpktp->pkt_reason == CMD_CMPLT) ||
	    (fltpktp->pkt_reason == CMD_DATA_OVR)) {
		action = scdk_exam_rqspkt(fltpktp);

		if (action == JUST_RETURN)
			return;

		if (action != COMMAND_DONE) {
			if ((scdk_ioretry(pktp, action)) == JUST_RETURN)
				return;
		}
	} else
		(void) scdk_ioretry(pktp, COMMAND_DONE_ERROR);
	SC_XPKTP(pktp)->x_callback(PKT_BP(pktp));
}


static int
scdk_exam_rqspkt(struct scsi_pkt *fltpktp)
{
	int 	amt;
	struct 	scsi_extended_sense *rqsp;
	struct	scdk *scdkp;
	int	rval = 0;
	struct scsi_device *sd;

	scdkp = PKT2SCDK(fltpktp);
	sd = scdkp->scd_sd;
	if (SCBP(fltpktp)->sts_busy) {
		scsi_log(sd->sd_dev, scdk_name, CE_WARN,
			"Busy Status on REQUEST SENSE\n");
		rval = QUE_COMMAND;
	}

	if (SCBP(fltpktp)->sts_chk) {
		scsi_log(sd->sd_dev, scdk_name, CE_WARN,
			"Check Condition on REQUEST SENSE\n");
		rval = QUE_COMMAND;
	}

	amt = SENSE_LENGTH - fltpktp->pkt_resid;
	if ((fltpktp->pkt_state & STATE_XFERRED_DATA) == 0 || amt == 0) {
		scsi_log(sd->sd_dev, scdk_name, CE_WARN,
			"Request Sense couldn't get sense data\n");
		rval = QUE_COMMAND;
	}

	if (rval) {
		if (SC_XPKTP(fltpktp)->x_retry) {
			SC_XPKTP(fltpktp)->x_retry--;
			(void) timeout(scdk_restart, (caddr_t)fltpktp,
			    SD_BSY_TIMEOUT);
			return (JUST_RETURN);
		}
		return (QUE_COMMAND);
	}

	rqsp = (struct scsi_extended_sense *)
			(SC_XPKTP(fltpktp)->x_rqsbp->b_un.b_addr);
	return (scdk_rqshdl(scdkp, SC_XPKTP(fltpktp)->x_fltpktp, rqsp, amt));
}

static int
scdk_rqshdl(struct scdk *scdkp, struct scsi_pkt *pktp,
		struct  scsi_extended_sense *rqsp, int rqslen)
{
	int 	rval;
	int 	severity, i;
	int	blkno;
	char 	*p;
	static 	char *hex = " 0x%x";
	daddr_t err_blkno;
	daddr_t secleft;
	struct	buf *bp;
	u_char 	com;
	struct	scsi_device *sd;
	struct  uscsi_cmd *scmdp;

	sd = scdkp->scd_sd;
	if (rqsp->es_class != CLASS_EXTENDED_SENSE) {
		auto char tmp[8];
		auto char buf[128];

		p = (char *)rqsp;
		strcpy(buf, "undecodable sense information:");
		if (rqslen < 0 || rqslen > SENSE_LENGTH)
			rqslen = SENSE_LENGTH;
		for (i = 0; i < rqslen; i++) {
			(void) sprintf(tmp, hex, *(p++)&0xff);
			strcpy(&buf[strlen(buf)], tmp);
		}
		i = strlen(buf);
		strcpy(&buf[i], "-(assumed fatal)\n");
		scsi_log(sd->sd_dev, scdk_name, CE_WARN, buf);
/*		check for user scsi command				*/
		if (SC_XPKTP(pktp)->x_scmdp) {
			scmdp = SC_XPKTP(pktp)->x_scmdp;
			scmdp->uscsi_status = SCBP_C(pktp);
/*			copy request sense info				*/
			scmdp->uscsi_rqresid = scmdp->uscsi_rqlen - rqslen;
			bcopy((caddr_t)rqsp, scmdp->uscsi_rqbuf, rqslen);
		}
		return (COMMAND_DONE_ERROR);
	}

	switch (rqsp->es_key) {
	case KEY_NOT_READY:
		if  (rqsp->es_add_code == 0x3a) {
			/* Don't retry if Medium Not Present */
			rval = COMMAND_DONE_ERROR;
			severity = SCSI_ERR_FATAL;
		}
		/*
		 * If we get a not-ready indication, wait a bit and
		 * try it again. Some drives pump this out for about
		 * 2-3 seconds after a reset.
		 */
		else if (SC_XPKTP(pktp)->x_retry) {
			SC_XPKTP(pktp)->x_retry--;
			(void) timeout(scdk_restart, (caddr_t)pktp,
			    SD_BSY_TIMEOUT);
			severity = SCSI_ERR_RETRYABLE;
			rval = JUST_RETURN;
		} else {
			rval = COMMAND_DONE_ERROR;
			severity = SCSI_ERR_FATAL;
		}
		break;

	case KEY_NO_SENSE:
		severity = SCSI_ERR_RETRYABLE;
		rval = QUE_COMMAND;
		break;

	case KEY_RECOVERABLE_ERROR:

		severity = SCSI_ERR_RECOVERED;
		rval = COMMAND_DONE;
		break;

	case KEY_MISCOMPARE:
	case KEY_HARDWARE_ERROR:
	case KEY_VOLUME_OVERFLOW:
	case KEY_WRITE_PROTECT:
	case KEY_BLANK_CHECK:
	case KEY_ILLEGAL_REQUEST:
		/* remap audio cdrom errors */
		if (scdkp->scd_cdrom && rqsp->es_key != KEY_WRITE_PROTECT &&
		    (rqsp->es_add_code == 0x64 || rqsp->es_add_code == 0x24))
			severity = SCSI_ERR_INFO;
		else
			severity = SCSI_ERR_FATAL;
		rval = COMMAND_DONE_ERROR;
		break;

	case KEY_MEDIUM_ERROR:
	case KEY_ABORTED_COMMAND:
		/* remap retryable and audio cdrom errors on cdrom */
		if (scdkp->scd_cdrom &&
		    (SC_XPKTP(pktp)->x_retry > 0 ||
		    rqsp->es_add_code == 0x64))
			severity = SCSI_ERR_INFO;
		else
			severity = SCSI_ERR_RETRYABLE;
		rval = QUE_COMMAND;
		break;
	case KEY_UNIT_ATTENTION:
		rval = QUE_COMMAND;
		severity = SCSI_ERR_INFO;
		/*
		 * handle power down or media change
		 */
		SCDK_SET_DRIVE_READY(scdkp, FALSE);
		break;
	default:
		/*
		 * Undecoded sense key.  Try retries and hope
		 * that will fix the problem.  Otherwise, we're
		 * dead.
		 */
		scsi_log(sd->sd_dev, scdk_name, CE_WARN,
			"Unhandled Sense Key '%s\n", sense_keys[rqsp->es_key]);
		severity = SCSI_ERR_RETRYABLE;
		rval = QUE_COMMAND;
	}

	com = CDBP(pktp)->scc_cmd & 0x1f;
	switch (com) {
		case SCMD_READ:
		case SCMD_WRITE:
			if (rqsp->es_valid) {
				err_blkno = (rqsp->es_info_1 << 24) |
					    (rqsp->es_info_2 << 16) |
					    (rqsp->es_info_3 << 8)  |
					    (rqsp->es_info_4);
			} else
				err_blkno = SC_XPKTP(pktp)->x_srtsec;
			blkno = SC_XPKTP(pktp)->x_srtsec;
			break;
		default:
			blkno = -1;
			err_blkno = -1;
			break;

	}
	if ((rval == QUE_COMMAND) && (SC_XPKTP(pktp)->x_retry == 0)) {
		bp = PKT_BP(pktp);
		if (err_blkno != -1) {
			secleft = SC_XPKTP(pktp)->x_srtsec +
				    SC_XPKTP(pktp)->x_seccnt - err_blkno;

			bp->b_resid += (SC_XPKTP(pktp)->x_byteleft +
					(secleft << scdkp->scd_secshf));
		} else
			bp->b_resid += bp->b_bcount;
		bp->b_flags |= B_ERROR;

/*		check for user scsi command				*/
		if (SC_XPKTP(pktp)->x_scmdp) {
			scmdp = SC_XPKTP(pktp)->x_scmdp;
			scmdp->uscsi_status = SCBP_C(pktp);
/*			copy request sense info				*/
			if (rqslen < 0 || rqslen > SENSE_LENGTH)
				rqslen = SENSE_LENGTH;
			scmdp->uscsi_rqresid = scmdp->uscsi_rqlen - rqslen;
			bcopy((caddr_t)rqsp, scmdp->uscsi_rqbuf, rqslen);
		}

		rval = COMMAND_DONE;
	}

	if (scdk_error_level < 0 || (severity >= scdk_error_level &&
	    !(pktp->pkt_flags & FLAG_SILENT))) {
		scsi_errmsg(scdkp->scd_sd, pktp, scdk_name, severity,
			blkno, err_blkno, scdk_cmds, rqsp);
	}
	return (rval);
}

static int
scdk_rqsprep(struct scdk *scdkp, struct scsi_pkt *pktp)
{
	register struct scsi_pkt *fltpktp;
	register struct	buf *bp;
	int	(*callback)() = DDI_DMA_DONTWAIT;
	caddr_t	arg = NULL;

	if (scdkp->scd_arq)
		return (scsi_exam_arq((opaque_t)scdkp, pktp, scdk_rqshdl,
			scdkp->scd_sd->sd_dev, scdk_name));

	fltpktp = SC_XPKTP(pktp)->x_fltpktp;
	if (!fltpktp) {
		bp = scsi_alloc_consistent_buf(SDEV2ADDR(scdkp->scd_sd), NULL,
				SENSE_LENGTH, B_READ, callback, arg);
		if (!bp)
			return (COMMAND_DONE_ERROR);
	} else
		bp = SC_XPKTP(fltpktp)->x_rqsbp;

	fltpktp = scsi_init_pkt(SDEV2ADDR(scdkp->scd_sd), fltpktp, bp,
			CDB_GROUP0, SECMDS_STATUS_SIZE, PKT_PRIV_LEN,
			PKT_CONSISTENT, callback, arg);
	if (!fltpktp) {
/*		check for existence of fltpkt 				*/
		if (!SC_XPKTP(pktp)->x_fltpktp)
			scsi_free_consistent_buf(bp);
		return (COMMAND_DONE_ERROR);
	}

	SC_XPKTP(fltpktp)->x_retry = sd_retry_count;
	if (SC_XPKTP(pktp)->x_fltpktp)
		return (QUE_SENSE);

	fltpktp->pkt_time = sd_io_time;
	fltpktp->pkt_comp = scdk_pktcbrs;

/*	link the command and fault pkt's together			*/
	SC_XPKTP(pktp)->x_fltpktp    = fltpktp;
	SC_XPKTP(fltpktp)->x_fltpktp = pktp;
	SC_XPKTP(fltpktp)->x_bp	= SC_XPKTP(pktp)->x_bp;

	SC_XPKTP(fltpktp)->x_rqsbp   = bp;
	SC_XPKTP(fltpktp)->x_sdevp   = (opaque_t)scdkp;
	SC_XPKTP(fltpktp)->x_callback = SC_XPKTP(pktp)->x_callback;
	SC_XPKTP(fltpktp)->x_flags   = PKT_CONSISTENT;

	GSC_REQSEN(fltpktp);
	return (QUE_SENSE);
}

static int
scdk_chkerr(struct scsi_pkt *pktp)
{
	struct  scdk	 *scdkp;
	struct	scsi_address *ap;
	int	com;
	int 	tval;

	scdkp = PKT2SCDK(pktp);
	if (SCBP(pktp)->sts_chk) {
#ifdef SCDK_DEBUG
		if (scdk_debug & DERR)
			PRF("scdk_chkerr: pktp= 0x%x return status check\n",
				pktp);
#endif
		return (scdk_rqsprep(scdkp, pktp));
	} else if (SCBP(pktp)->sts_busy) {
#ifdef SCDK_DEBUG
		if (scdk_debug & DERR)
			PRF("scdk_chkerr: pktp= 0x%x return busy\n", pktp);
#endif
/* 		Queue Full status 					*/
		if (SCBP(pktp)->sts_scsi2)
			tval = 0;
		else if (SCBP(pktp)->sts_is) {
			if (!(pktp->pkt_flags & FLAG_SILENT))
				scsi_log(scdkp->scd_sd->sd_dev, scdk_name,
					CE_NOTE, "reservation conflict\n");
			tval = SD_IO_TIME * drv_usectohz(1000000);
		} else
			tval = SD_BSY_TIMEOUT;

		if (SC_XPKTP(pktp)->x_retry) {
			SC_XPKTP(pktp)->x_retry--;
			if (tval)
				(void) timeout(scdk_restart, (caddr_t)pktp,
				    tval);
			return (JUST_RETURN);
		} else {
			if (!(pktp->pkt_flags & FLAG_SILENT))
				scsi_log(scdkp->scd_sd->sd_dev, scdk_name,
					CE_WARN, "device busy too long\n");
			ap = SCDK2ADDR(scdkp);
			if (scsi_reset(ap, RESET_TARGET))
				return (QUE_COMMAND);
			else if (scsi_reset(ap, RESET_ALL))
				return (QUE_COMMAND);
		}
	} else {
#ifdef SCDK_DEBUG
	if (scdk_debug & DERR)
		PRF("scdk_chkerr: pktp= 0x%x return partial xfer\n", pktp);
#endif
		com = GETCMD((union scsi_cdb *)pktp->pkt_cdbp);

		if (pktp->pkt_resid && (com == SCMD_READ || com == SCMD_WRITE))
			return (QUE_COMMAND);

		SC_XPKTP(pktp)->x_bp->b_resid += pktp->pkt_resid;
		return (COMMAND_DONE);
	}
	return (COMMAND_DONE_ERROR);
}

static int
scdk_incomplete(register struct scsi_pkt *pktp)
{
	static char *fail = "SCSI transport failed: reason '%s': %s\n";
	struct scdk	 *scdkp;
	struct scsi_address *ap;
	struct scsi_device *sd;

	scdkp = PKT2SCDK(pktp);
	sd = scdkp->scd_sd;
	if (SC_XPKTP(pktp)->x_retry <= 0)
		scsi_incmplmsg(scdkp->scd_sd, scdk_name, pktp);

	switch (pktp->pkt_reason) {
	case CMD_RESET:		/* SCSI bus reset command 		*/
	case CMD_ABORTED:	/* Command aborted on request 		*/
		if (!sd_retry_all_failures)
			break;
	default:
		if (SC_XPKTP(pktp)->x_retry) {
			if (!(pktp->pkt_flags & FLAG_SILENT))
				scsi_log(sd->sd_dev, scdk_name, CE_WARN, fail,
					scsi_rname(pktp->pkt_reason),
					"retrying command");
			return (QUE_COMMAND);
		}
		if (pktp->pkt_reason == CMD_DATA_OVR)
			PKT_BP(pktp)->b_error = EOVERFLOW;
		else if (!(pktp->pkt_statistics &
			(STAT_BUS_RESET|STAT_DEV_RESET|STAT_ABORTED))) {
			ap = SCDK2ADDR(scdkp);
			if (!scsi_reset(ap, RESET_TARGET))
				(void) scsi_reset(ap, RESET_ALL);
		}
	}
	if (!(pktp->pkt_flags & FLAG_SILENT))
		scsi_log(sd->sd_dev, scdk_name, CE_WARN, fail,
			scsi_rname(pktp->pkt_reason), "giving up");
	return (COMMAND_DONE_ERROR);
}

#ifdef SCDK_DEBUG
void
scdk_prtgeom(struct scdk *scdkp)
{
	PRF("PHY GEOM: head= %d sector= %d secsize= %d"
		"cyl= 0x%x acyl= %d capacity= 0x%x\n",
		scdkp->scd_phyg.g_head, scdkp->scd_phyg.g_sec,
		scdkp->scd_phyg.g_secsiz, scdkp->scd_phyg.g_cyl,
		scdkp->scd_phyg.g_acyl, scdkp->scd_phyg.g_cap);
	PRF("VIRT GEOM: head= %d sector= %d secsize= %d"
		"cyl= 0x%x acyl= %d capacity= 0x%x\n",
		scdkp->scd_logg.g_head, scdkp->scd_logg.g_sec,
		scdkp->scd_logg.g_secsiz, scdkp->scd_logg.g_cyl,
		scdkp->scd_logg.g_acyl, scdkp->scd_logg.g_cap);
}
#endif


static struct uscsi_cmd *
scdk_uscsi_alloc()
{
	register struct uscsi_cmd *scmdp;

	scmdp = kmem_zalloc((size_t)(sizeof (struct uscsi_cmd) + CDB_GROUP5 +
		SENSE_LENGTH), KM_SLEEP);
	scmdp->uscsi_cdb = (caddr_t)(scmdp + 1);
	scmdp->uscsi_rqbuf = (caddr_t)(scmdp + 1) + CDB_GROUP5;
	scmdp->uscsi_rqlen = SENSE_LENGTH;
	scmdp->uscsi_rqresid = SENSE_LENGTH;

	return (scmdp);
}

static void
scdk_uscsi_free(register struct uscsi_cmd *scmdp)
{
	kmem_free((caddr_t)scmdp, (sizeof (struct uscsi_cmd) +
		CDB_GROUP5 + SENSE_LENGTH));
}

static int
scdk_uscsi_ioctl(struct scdk *scdkp, dev_t dev, int arg, int flag)
{
	struct	uscsi_cmd scmd;
	struct	uscsi_cmd *inp = &scmd;
	struct	uscsi_cmd *scmdp;
	int	status;
	int	rqlen;

	if (ddi_copyin((caddr_t)arg, (caddr_t)inp, sizeof (*inp), flag) ||
	    !inp->uscsi_cdb || !inp->uscsi_cdblen)
		return (EFAULT);

	if ((scmdp = scdk_uscsi_alloc()) == (struct uscsi_cmd *)NULL)
		return (ENOMEM);

	if (ddi_copyin(inp->uscsi_cdb, scmdp->uscsi_cdb,
		(u_int)inp->uscsi_cdblen, flag)) {
		scdk_uscsi_free(scmdp);
		return (EFAULT);
	}

	scmdp->uscsi_flags	= inp->uscsi_flags & ~USCSI_NOINTR;
	scmdp->uscsi_timeout	= inp->uscsi_timeout;
	scmdp->uscsi_cdblen	= inp->uscsi_cdblen;
	scmdp->uscsi_buflen 	= inp->uscsi_buflen;
	if (scmdp->uscsi_buflen)
		scmdp->uscsi_bufaddr = inp->uscsi_bufaddr;

	status = scdk_uscsi_buf_setup(scdkp, (opaque_t)scmdp, dev,
			(flag & FKIOCTL) ? UIO_SYSSPACE : UIO_USERSPACE);

	inp->uscsi_resid  = scmdp->uscsi_resid;
	inp->uscsi_status = scmdp->uscsi_status;
	rqlen = scmdp->uscsi_rqlen - scmdp->uscsi_rqresid;
	rqlen = min(((int)inp->uscsi_rqlen), rqlen);
	inp->uscsi_rqresid = inp->uscsi_rqlen - rqlen;
	inp->uscsi_rqstatus = scmdp->uscsi_rqstatus;
	if ((inp->uscsi_flags & USCSI_RQENABLE) && inp->uscsi_rqbuf && rqlen) {
		if (ddi_copyout(scmdp->uscsi_rqbuf, inp->uscsi_rqbuf, rqlen,
			flag)) {
			if (status)
				status = EFAULT;
		}
	}
	if (ddi_copyout((caddr_t)inp, (caddr_t)arg, sizeof (*inp), flag)) {
		if (status)
			status = EFAULT;
	}
	scdk_uscsi_free(scmdp);
	return (status);
}

static int
scdk_uscsi_buf_setup(struct scdk *scdkp, opaque_t *cmdp, dev_t dev,
	enum uio_seg dataspace)
{
	register struct uscsi_cmd *scmdp = (struct uscsi_cmd *)cmdp;
	register struct	buf  *bp;
	int	status;
	int 	rw;

	if ((bp = getrbuf(KM_SLEEP)) == NULL)
		return (ENXIO);

	rw = (scmdp->uscsi_flags & USCSI_READ) ? B_READ : B_WRITE,
	bp->av_forw = (struct buf *)scdkp;
	bp->b_back  = (struct buf *)scmdp;

	if (scmdp->uscsi_buflen) {
		auto struct iovec aiov;
		auto struct uio auio;
		register struct uio *uio = &auio;

		bzero((caddr_t)&auio, sizeof (struct uio));
		bzero((caddr_t)&aiov, sizeof (struct iovec));
		aiov.iov_base = scmdp->uscsi_bufaddr;
		aiov.iov_len = scmdp->uscsi_buflen;
		uio->uio_iov = &aiov;

		uio->uio_iovcnt = 1;
		uio->uio_resid = scmdp->uscsi_buflen;
		uio->uio_segflg = dataspace;
		uio->uio_loffset = 0;
		uio->uio_fmode = 0;

/* 		Let physio do the rest...  				*/
		status = physio(scdk_uscsi_wrapper, bp, dev, rw, scdkmin, uio);
	} else {
		bp->b_flags = B_BUSY | rw;
		bp->b_edev = dev;
		bp->b_dev = cmpdev(dev);
		scdk_uscsi(scdkp, scmdp, bp);
		status = biowait(bp);
	}

	scmdp->uscsi_resid = bp->b_resid;
	freerbuf(bp);
	return (status);
}

static void
scdkmin(struct buf *bp)
{
	if (bp->b_bcount > scdk_uscsi_maxphys)
		bp->b_bcount = scdk_uscsi_maxphys;
}

static int
scdk_uscsi_wrapper(register struct buf *bp)
{
	scdk_uscsi((struct scdk *)bp->av_forw, (struct uscsi_cmd *)bp->b_back,
			bp);
	return (0);
}

static void
scdk_uscsi(struct scdk *scdkp, struct uscsi_cmd *scmdp, struct buf *bp)
{
	struct  scsi_pkt *pktp;
	daddr_t	sec;
	u_char	com;

	pktp = scdk_pktprep(scdkp, NULL, bp, scmdp->uscsi_cdblen,
		scdk_iodone, PKT_DMA_PARTIAL, DDI_DMA_SLEEP, NULL);
	if (!pktp) {
		SETBPERR(bp, ENOMEM);
		biodone(bp);
		return;
	}

	SC_XPKTP(pktp)->x_scmdp = scmdp;
	if (scmdp->uscsi_timeout > 0)
		pktp->pkt_time = scmdp->uscsi_timeout;
	else
		pktp->pkt_time = sd_io_time;
	if (scmdp->uscsi_flags & USCSI_SILENT)
		pktp->pkt_flags |= FLAG_SILENT;
	if (scmdp->uscsi_flags & USCSI_NOINTR)
		pktp->pkt_flags |= FLAG_NOINTR;
	if (scmdp->uscsi_flags & USCSI_NOPARITY)
		pktp->pkt_flags |= FLAG_NOPARITY;

	com = scmdp->uscsi_cdb[0] & 0x1f;
	if (bp->b_bcount && ((com == SCMD_READ) || (com == SCMD_WRITE))) {
		if (scmdp->uscsi_cdb[0] & 0x20)
			sec = GETG1ADDR(((union scsi_cdb *)scmdp->uscsi_cdb));
		else
			sec = GETG0ADDR(((union scsi_cdb *)scmdp->uscsi_cdb));
		SET_BP_SEC(bp, (LBLK2SEC(sec, scdkp->scd_blkshf)));
		(void) scdk_ioprep(scdkp, pktp, SCDK_IOSTART);
	} else {
		bcopy(scmdp->uscsi_cdb, (caddr_t)pktp->pkt_cdbp,
			scmdp->uscsi_cdblen);
		MAKECOM_COMMON(pktp, scdkp->scd_sd, pktp->pkt_flags,
			scmdp->uscsi_cdb[0]);
	}

	FLC_ENQUE(scdkp->scd_flcobjp, bp);
}

static int
scdk_cdioctl(register struct scdk *scdkp, dev_t dev, int cmd, int arg, int flag)
{
	int	status;
	register struct	uscsi_cmd *scmdp;

	if ((scmdp = scdk_uscsi_alloc()) == (struct uscsi_cmd *)NULL)
		return (ENOMEM);

	status = TGCD_IOCTL(scdkp->scd_cdobjp, (opaque_t)scmdp, dev, cmd,
			arg, flag);

	scdk_uscsi_free(scmdp);

	return (status);
}

static int
scdk_init_cdioctl(register struct scdk *scdkp, dev_t dev, int cmd,
	int arg, int flag)
{

	char		cd_keyvalp[3*OBJNAMELEN];
	int		cd_keylen;
	register int	i, j;

/*	initialize the pass through object				*/
	scdkp->scd_passthru.pt_data = (opaque_t)scdkp;
	scdkp->scd_passthru.pt_ops  = &scdk_pt_ops;

	cd_keylen = sizeof (cd_keyvalp);
	if (ddi_prop_op(DDI_DEV_T_NONE, scdkp->scd_sd->sd_dev,
		PROP_LEN_AND_VAL_BUF, DDI_PROP_CANSLEEP, "scsi_audio",
		(caddr_t)cd_keyvalp, &cd_keylen) != DDI_PROP_SUCCESS) {
		strcpy(cd_keyvalp, "sccd_std");
		cd_keylen = strlen("sccd_std") + 1;
	} else {
		cd_keyvalp[cd_keylen] = (char)0;
		cd_keylen++;
	}

	for (i = 0, j = 0; i < cd_keylen; i++) {
		if ((scdkp->scd_cdname[j] = cd_keyvalp[i]) != (char)0) {
			j++;
			continue;
		}

		if (objmgr_load_obj(scdkp->scd_cdname) != DDI_SUCCESS) {
			j = 0;
			continue;
		}

		if (scdkp->scd_cdobjp = objmgr_create_obj(scdkp->scd_cdname)) {
			TGCD_INIT(scdkp->scd_cdobjp, &(scdkp->scd_passthru));
			if (TGCD_IDENTIFY(scdkp->scd_cdobjp,
					scdkp->scd_sd->sd_inq) == DDI_SUCCESS) {
				scdkp->scd_cdioctl = scdk_cdioctl;
				return (scdk_cdioctl(scdkp, dev, cmd, arg,
					flag));
			}
			TGCD_FREE(scdkp->scd_cdobjp);
			scdkp->scd_cdobjp = 0;
			objmgr_destroy_obj(scdkp->scd_cdname);
		}

		objmgr_unload_obj(scdkp->scd_cdname);
		j = 0;
	}
	return (ENOTTY);
}

static int
scdk_lock_unlock(struct scdk *scdkp, dev_t dev, u_char lock)
{
	register u_char *cdb;
	register struct	uscsi_cmd *scmdp;
	int	status;

	if ((scmdp = scdk_uscsi_alloc()) == (struct uscsi_cmd *)NULL)
		return (ENOMEM);

	cdb    = (u_char *)scmdp->uscsi_cdb;
	cdb[0] = SCMD_DOORLOCK;
	cdb[4] = lock;
	scmdp->uscsi_flags   = USCSI_DIAGNOSE|USCSI_SILENT|USCSI_WRITE;
	scmdp->uscsi_cdblen  = CDB_GROUP0;
	scmdp->uscsi_timeout = 15;

	status = scdk_uscsi_buf_setup(scdkp, (opaque_t)scmdp, dev,
		UIO_SYSSPACE);

	scdk_uscsi_free(scmdp);

	return (status);
}

static int
scdk_start_stop(struct scdk *scdkp, dev_t dev, u_char start)
{
	int	status;
	register u_char *cdb;
	register struct	uscsi_cmd *scmdp;

	if ((scmdp = scdk_uscsi_alloc()) == (struct uscsi_cmd *)NULL)
		return (ENOMEM);

	cdb    = (u_char *)scmdp->uscsi_cdb;
	cdb[0] = SCMD_START_STOP;
	cdb[4] = start;

	scmdp->uscsi_cdblen  = CDB_GROUP0;
	scmdp->uscsi_flags   = USCSI_DIAGNOSE|USCSI_SILENT;
	scmdp->uscsi_timeout = 15;

	status = scdk_uscsi_buf_setup(scdkp, (opaque_t)scmdp, dev,
		UIO_SYSSPACE);

	scdk_uscsi_free(scmdp);

	if (!status)
		SCDK_SET_DRIVE_READY(scdkp, FALSE);

	return (status);
}

/*
 * This routine ejects the CDROM disc
 */
static int
scdk_eject(register struct scdk *scdkp, dev_t dev)
{
	int	status;

	if (!scdkp->scd_rmb) {
		/* just stop the drive */
		status = scdk_start_stop(scdkp, dev, (u_char)0);
		return (status);
	}

	/* unlock the door, stop the drive, and eject the media */
	if ((status = scdk_lock_unlock(scdkp, dev, 0))
	||  (status = scdk_start_stop(scdkp, dev, (u_char)2)))
		return (status);

	mutex_enter(&scdkp->scd_mutex);
	scdkp->scd_iostate = DKIO_EJECTED;
	cv_broadcast(&scdkp->scd_state_cv);
	mutex_exit(&scdkp->scd_mutex);

	return (0);


}

static int
scdk_check_media(struct scdk *scdkp, enum dkio_state *state)
{
	register opaque_t token = NULL;

#ifdef SCDK_DEBUG
	if (scdk_debug & DSTATE)
		PRF("scdk_check_media: user state %x disk state %x\n",
			*state, scdkp->scd_iostate);
#endif

	if (*state != scdkp->scd_iostate) {
		*state = scdkp->scd_iostate;
		return (0);
	}

	token = scdk_watch_request_submit(scdkp->scd_sd,
		scdk_check_media_time, SENSE_LENGTH,
			scdk_media_watch_cb, (caddr_t)scdkp);
	if (token == NULL)
		return (EAGAIN);

	mutex_enter(&scdkp->scd_mutex);
	while (*state == scdkp->scd_iostate) {
		if (cv_wait_sig(&scdkp->scd_state_cv, &scdkp->scd_mutex) == 0) {
			mutex_exit(&scdkp->scd_mutex);
			scdk_watch_request_terminate(token);
			return (EINTR);
		}
#ifdef SCDK_DEBUG
		if (scdk_debug & DSTATE)
			PRF("scdk_chedk_media: received sig for state %x\n",
				scdkp->scd_iostate);
#endif
	}
	*state = scdkp->scd_iostate;
	mutex_exit(&scdkp->scd_mutex);
	scdk_watch_request_terminate(token);
	return (0);
}

/*
 * delayed cv_broadcast to allow for target to recover
 * from media insertion
 */
static void
scdk_delayed_cv_broadcast(caddr_t arg)
{
	struct scdk *scdkp = (struct scdk *)arg;

#ifdef SCDK_DEBUG
	scsi_log(scdkp->scd_sd->sd_dev, scdk_name, CE_NOTE,
		"delayed cv_broadcast\n");
#endif

	mutex_enter(&scdkp->scd_mutex);
	cv_broadcast(&scdkp->scd_state_cv);
	mutex_exit(&scdkp->scd_mutex);
}


/*
 * scdk_media_watch_cb() is called by scdk_watch_thread for
 * verifying the request sense data (if any)
 */
#define	MEDIA_ACCESS_DELAY 2000000

static int
scdk_media_watch_cb(caddr_t arg, struct scdk_watch_result *resultp)
{
	register struct scsi_status *statusp = resultp->statusp;
	register struct scsi_extended_sense *sensep = resultp->sensep;
	u_char actual_sense_length = resultp->actual_sense_length;
	enum dkio_state state = DKIO_NONE;
	register struct scdk *scdkp = (struct scdk *)arg;

#ifdef SCDK_DEBUG
	if (scdk_debug & DSTATE)
		scsi_log(scdkp->scd_sd->sd_dev, scdk_name, CE_NOTE,
		"scdk_media_watch_cb: status=%x, sensep=%x, len=%x\n",
			*((char *)statusp), sensep,
			actual_sense_length);
#endif

	/*
	 * if there was a check condition then sensep points to valid
	 * sense data
	 * if status was not a check condition but a reservation or busy
	 * status then the new state is DKIO_NONE
	 */
	if (sensep) {
		if (actual_sense_length >= SENSE_LENGTH) {
			if (sensep->es_key == KEY_UNIT_ATTENTION) {
				if (sensep->es_add_code == 0x28) {
					state = DKIO_INSERTED;
				}
			} else if (sensep->es_add_code == 0x3a) {
				state = DKIO_EJECTED;
			}
		}
	} else if (*((char *)statusp) == STATUS_GOOD) {
		state = DKIO_INSERTED;
	}
#ifdef SCDK_DEBUG
	if (scdk_debug & DSTATE)
		scsi_log(scdkp->scd_sd->sd_dev, scdk_name, CE_NOTE,
		"state=%x\n", state);
#endif

	/*
	 * now signal the waiting thread if this is *not* the specified state;
	 * delay the signal if the state is DKIO_INSERTED
	 * to allow the target to recover
	 */
	if (state != scdkp->scd_iostate) {
		scdkp->scd_iostate = state;
		if (state == DKIO_INSERTED) {
			/*
			 * delay the signal to give the drive a chance
			 * to do what it apparently needs to do
			 */
#ifdef SCDK_DEBUG
			if (scdk_debug & DSTATE)
				scsi_log(scdkp->scd_sd->sd_dev, scdk_name,
					CE_NOTE, "delayed cv_broadcast\n");
#endif
			(void) timeout(scdk_delayed_cv_broadcast,
			    (caddr_t)scdkp,
			    drv_usectohz((clock_t)MEDIA_ACCESS_DELAY));
		} else {
#ifdef SCDK_DEBUG
			if (scdk_debug & DSTATE)
				scsi_log(scdkp->scd_sd->sd_dev, scdk_name,
					CE_NOTE, "immediate cv_broadcast\n");
#endif
			cv_broadcast(&scdkp->scd_state_cv);
		}
	}
	return (0);
}

static int
scdk_inquiry(struct scdk *scdkp, struct scsi_inquiry **inqpp)
{
	if (scdkp && scdkp->scd_sd && scdkp->scd_sd->sd_inq) {
		*inqpp = scdkp->scd_sd->sd_inq;
		return (DDI_SUCCESS);
	}

	return (DDI_FAILURE);
}
