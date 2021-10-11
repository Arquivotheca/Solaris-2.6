/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)transport.c	1.11	95/12/12 SMI"

#include <sys/dktp/ncrs/ncr.h>

extern ddi_dma_lim_t ncr_dma_lim;
static	int	ncr_cb_id = 0;

/*
 * The (*tran_start) function.  Transport the command in pktp to the
 * addressed SCSI target device.  The command is not finished when this
 * returns, only sent to the target; ncr_intr() will call
 * (*pktp->pkt_comp)(pktp) when the target device has responded.
 */

int
ncr_transport(struct scsi_address *ap, struct scsi_pkt *pktp)
{
	ncr_t	*ncrp = PKT2NCRBLKP(pktp);
	npt_t	*nptp = PKT2NCRUNITP(pktp);
	nccb_t	*nccbp = PKTP2NCCBP(pktp);

	NDBG12(("ncr_transport\n"));
	/*
 	 * Send the command to target/lun, however your HBA requires it.
	 * If busy, return TRAN_BUSY; if there's some other formatting error
	 * in the packet, return TRAN_BADPKT; otherwise, fall through to the
	 * return of TRAN_ACCEPT.
 	 *
	 * Remember that access to shared resources, including the ncr_t
	 * data structure and the HBA hardware registers, must be protected
	 * with mutexes, here and everywhere.
	 *
	 * Also remember that at interrupt time, you'll get an argument
	 * to the interrupt handler which is a pointer to your ncr_t
	 * structure; you'll have to remember which commands are outstanding
	 * and which scsi_pkt is the currently-running command so the
	 * interrupt handler can refer to the pkt to set completion
	 * status, call the target driver back through pkt_comp, etc.
 	 */
	mutex_enter(&ncrp->n_mutex);
#ifdef NCR_DEBUG
if (nptp->nt_state != NPT_STATE_DONE)
debug_enter("\n\nNOT DONE\n\n");
#endif
	ncr_queue_ccb(ncrp, nptp, nccbp, NRQ_NORMAL_CMD);

	/*
 	 * If this is a NOINTR command, wait for a response with ncr_pollret;
	 * otherwise, return to caller (target driver) now and call him
 	 * back later (at interrupt time).
	 */
	if (pktp->pkt_flags & FLAG_NOINTR)
		ncr_pollret(ncrp, nccbp);

	NDBG12(("ncr_transport: okay nccbp=0x%x\n", nccbp));
	mutex_exit(&ncrp->n_mutex);

	return (TRAN_ACCEPT);
}

/*
 * (*tran_reset).  Reset the SCSI bus, or just one target device,
 * depending on level.  Return 1 on success, 0 on failure.  If level is
 * RESET_TARGET, all commands on that target should have their pkt_comp
 * routines called, with pkt_reason set to CMD_RESET.
 */

int
ncr_reset(	struct scsi_address	*ap,
		int			 level )
{
	ncr_t	*ncrp = ADDR2NCRBLKP(ap);
	npt_t	*nptp;
	nccb_t	*nccbp;
	int 	 rc;

	NDBG22(("ncr_reset\n"));

	switch (level) {

	case RESET_ALL:
		mutex_enter(&ncrp->n_mutex);
/***/
/***debug_enter("\n\nNCR_RESET ALL\n\n");
/***/
		/*
		 * Reset the SCSI bus, kill all commands in progress
		 * (remove them from lists, etc.)  Make sure you
		 * wait the specified time for the reset to settle,
		 * if your hardware doesn't do that for you somehow.
		 */
		NCR_BUS_RESET(ncrp);

		/* reset the sync i/o state for all LUNs */
		ncr_syncio_reset(ncrp, NULL);

		/* flush all commands for all LUNs */
		ncr_flush_hba(ncrp, TRUE, CMD_RESET, STATE_GOT_BUS
				  , STAT_BUS_RESET);

		/* now mark the hba as idle */
		ncrp->n_state = NSTATE_IDLE;

		/* run the completion routines of all the flushed commands */
		while ((nccbp = ncr_doneq_rm(ncrp)) != NULL) {
			/* run this command's completion routine */
#if 0
			struct scsi_pkt *pktp;
			mutex_exit(&ncrp->n_mutex);
			pktp = NCCBP2PKTP(nccbp);
			(*pktp->pkt_comp)(pktp);
#else
			mutex_exit(&ncrp->n_mutex);
			scsi_run_cbthread(ncrp->n_cbthdl, NCCBP2SCMDP(nccbp));
#endif

			mutex_enter(&ncrp->n_mutex);
		}
		mutex_exit(&ncrp->n_mutex);
		return (TRUE);
		break;

	case RESET_TARGET:
		mutex_enter(&ncrp->n_mutex);
/***/
/***debug_enter("\n\nNCR_RESET TARGET\n\n");
/***/

		/*
		 * Issue a Bus Device Reset message to the target/lun
		 * specified in ap;
		 */
		nptp = ADDR2NCRUNITP(ap);

		if (nptp->nt_state == NPT_STATE_ACTIVE) {
			/* can't do device reset while device is active */
			mutex_exit(&ncrp->n_mutex);
			return (FALSE);
		}
		/* flush all the requests for all LUN's on this target */
		ncr_flush_target(ncrp, nptp->nt_target, TRUE, CMD_RESET
				  , (STATE_GOT_BUS | STATE_GOT_TARGET)
				  , STAT_DEV_RESET);
		rc = ncr_send_dev_reset(ap, ncrp, nptp);
		mutex_exit(&ncrp->n_mutex);
		return (rc);
	}
	return (FALSE);
}

/*
 * (*tran_abort).  Abort specific command on target device, or all commands
 * on that target/LUN.
 */

int
ncr_abort(	struct scsi_address	*ap,
		struct scsi_pkt		*pktp )
{
	ncr_t	*ncrp = ADDR2NCRBLKP(ap);
	npt_t	*nptp;
	nccb_t	*nccbp;
	int	rc;

	NDBG23(("ncr_abort: pktp=0x%x\n", pktp));

	/*
 	 * Abort the command pktp on the target/lun in ap.  If pktp is
	 * NULL, abort all outstanding commands on that target/lun.
	 * If you can abort them, return 1, else return 0.
	 * Each packet that's aborted should be sent back to the target
	 * driver through the callback routine, with pkt_reason set to
	 * CMD_ABORTED.
 	 */

	/* abort cmd pktp on HBA hardware; clean out of outstanding
	 * command lists, etc.
	 */
	mutex_enter(&ncrp->n_mutex);

	if (pktp != NULL) {
		/* abort the specified packet */
		ncrp = PKT2NCRBLKP(pktp);
		nptp = PKT2NCRUNITP(pktp);
		nccbp = PKTP2NCCBP(pktp);

		if (nccbp->nc_queued) {
			ncr_waitq_delete(nptp, nccbp);
			ncr_set_done(ncrp, nptp, nccbp, CMD_ABORTED, 0
					 , STAT_ABORTED);
			mutex_exit(&ncrp->n_mutex);
			return (TRUE);
		}
		if (nptp->nt_state == NPT_STATE_DONE
		||  nptp->nt_state == NPT_STATE_ACTIVE) {
			/* If it's done then it's probably already on */
			/* the done queue. If it's active we can't abort. */
			mutex_exit(&ncrp->n_mutex);
			return (FALSE);
		}
		/* try to abort the target's current request */
		rc = ncr_abort_ccb(ap, ncrp, nptp);
	} else {
		/* Abort all the packets for a particular LUN */
		ncrp = ADDR2NCRBLKP(ap);
		nptp = ADDR2NCRUNITP(ap);

		if (nptp->nt_state == NPT_STATE_DONE
		||  nptp->nt_state == NPT_STATE_ACTIVE) {
			/* If it's done then it's probably already on */
			/* the done queue. If it's active we can't abort. */
			mutex_exit(&ncrp->n_mutex);
			return (FALSE);
		}
		if (nptp->nt_state == NPT_STATE_QUEUED) {
			/* it's the currently active ccb on this target but */
			/* the ccb hasn't been started yet */
			ncr_set_done(ncrp, nptp, nptp->nt_nccbp, CMD_ABORTED
					 , STATE_GOT_TARGET, STAT_ABORTED);
			nptp->nt_nccbp = NULL;
		} else {
			NDBG30(("ncr_abort_ccb: disconnected\n"));
#ifdef	NCR_DEBUG
if (nptp->nt_nccbp == NULL
||  nptp->nt_state != NPT_STATE_DISCONNECTED)
    debug_enter("\ninvalid nt_nccbp\n");
#endif
			ncr_flush_lun(ncrp, nptp, FALSE, CMD_ABORTED
					  , (STATE_GOT_BUS | STATE_GOT_TARGET)
					  , STAT_ABORTED);
		}
		/* try to abort the target's current request */
		if (ncr_abort_ccb(ap, ncrp, nptp) == FALSE) {
			mutex_exit(&ncrp->n_mutex);
			return (FALSE);
		}
		/* abort the queued requests */
		while ((nccbp = ncr_waitq_rm(nptp)) != NULL) {
			NDBG23(("ncr_abort: nccbp=0x%x\n", nccbp));
			ncr_set_done(ncrp, nptp, nccbp, CMD_ABORTED, 0
					 , STAT_ABORTED);
		}
		rc = TRUE;
	}
	mutex_exit(&ncrp->n_mutex);
	return (TRUE);
}

/* Utility routine for ncr_ifsetcap/ifgetcap */

static int
ncr_capchk(	char	*cap,
		int	 tgtonly,
		int	*cidxp )
{
	int	cidx;

	NDBG23(("ncr_capchk\n"));

	if (!cap)
		return (FALSE);

	*cidxp = scsi_hba_lookup_capstr(cap);
	return (TRUE);
}

/*
 * (*tran_getcap).  Get the capability named, and return its value.
 */

int
ncr_getcap(	struct scsi_address	*ap,
		char			*cap,
		int			 tgtonly )
{
	int	status;
	int	ckey;

	NDBG23(("ncr_getcap: %s\n", cap));

	if ((status = ncr_capchk(cap, tgtonly, &ckey)) != TRUE)
		return (UNDEFINED);

	switch (ckey) {

	case SCSI_CAP_ARQ:

		if (tgtonly)
			return (ADDR2NCRUNITP(ap)->nt_arq);
		else
			return (UNDEFINED);

	case SCSI_CAP_TAGGED_QING:

		if (tgtonly)
			return (ADDR2NCRUNITP(ap)->nt_tagque);
		else
			return (UNDEFINED);

	/* Add cases for other capabilities you support */
	case SCSI_CAP_GEOMETRY:
		return (ncr_geometry(ADDR2NCRBLKP(ap), ap));

	default:
		return (UNDEFINED);
	}
}

/*
 * (*tran_setcap).  Set the capability named to the value given.
 */

int
ncr_setcap(	struct scsi_address	*ap,
		char			*cap,
		int			 value,
		int			 tgtonly )
{
	int	ckey;
	int	status;

	NDBG23(("ncr_setcap: %s %d\n", cap, value));

	if ((status = ncr_capchk(cap, tgtonly, &ckey)) != TRUE)
		return (status);

	switch (ckey) {
/******/
/******/
/******/
/**	case SCSI_CAP_TAGGED_QING:
/**
/**		/* defined only per-target */
/**		if (tgtonly) {
/**			ADDR2NCRUNITP(ap)->nt_tagque = (u_int)value;
/**			return (TRUE);
/**		} else
/**			return (FALSE);
/**
/**	case SCSI_CAP_ARQ:
/**
/**		/* defined only per-target */
/**		if (tgtonly) {
/**			ADDR2NCRUNITP(ap)->nt_arq = (u_int)value;
/**			return (TRUE);
/**		} else
/**			return (FALSE);
/**
/******/
/******/
/******/
	case SCSI_CAP_SECTOR_SIZE:

		/* Sector size affects nt_dma_lim structure */
		ADDR2NCRUNITP(ap)->nt_dma_lim.dlim_granular = value;
		return (TRUE);


	case SCSI_CAP_TOTAL_SECTORS:
		ADDR2NCRUNITP(ap)->nt_total_sectors = value;
		return (TRUE);

	case SCSI_CAP_GEOMETRY:
		return (TRUE);

	default:
		return (UNDEFINED);
	}
}

/*
 * (*tran_pktalloc).  Allocate memory for packet (excluding DMA resources)
 * according to lengths passed; use callback and arg if resources not
 * available.
 */

struct scsi_pkt *
ncr_pktalloc(	struct scsi_address *ap,
		int		cmdlen,
		int		statuslen,
		int		tgtlen,
		int		(*callback)(),
		caddr_t		arg )
{
	struct scsi_cmd	*cmdp;
	nccb_t		*nccbp;
	u_int		 statbuflen;
	int		 kf;
	caddr_t		 tgt;

	NDBG14(("ncr_pktalloc\n"));

	/* make kmem_zalloc flag based on what pktalloc's caller wants */
	kf = HBA_KMFLAG(callback);

	/*
	 * Allocate target-private data, if necessary
	 */
	if (tgtlen > PKT_PRIV_LEN) {
		tgt = kmem_zalloc(tgtlen, kf);
		if (!tgt) {
			ASSERT(callback != SLEEP_FUNC);
			if (callback != NULL_FUNC)
				ddi_set_callback(callback, arg, &ncr_cb_id);
			return ((struct scsi_pkt *)NULL);
		}
	} else {
		tgt = NULL;
	}

	nccbp = (nccb_t *)kmem_zalloc(sizeof(*nccbp), kf);
	if (nccbp == NULL) {
		/*
 		 * Failed allocation; set up to call callback,
		 * if appropriate
		 */
		if (tgt)
			kmem_free(tgt, tgtlen);
		ASSERT(callback != SLEEP_FUNC);
		if (callback != NULL_FUNC)
			ddi_set_callback(callback, arg, &ncr_cb_id);
		NDBG12(("ncr_pktalloc: no ccb buffer\n"));
		return ((struct scsi_pkt *)NULL);
	}

	/* Initialize the packet and command wrapper */

	cmdp = &nccbp->nc_cmd;
	cmdp->cmd_private = (opaque_t)nccbp;
	cmdp->cmd_pkt.pkt_cdbp = (u_char *)&nccbp->nc_cdb;
#if 1
	cmdp->cmd_privlen = tgtlen;
	if (tgtlen > PKT_PRIV_LEN) {
		cmdp->cmd_pkt.pkt_private = tgt;
	} else if (tgtlen > 0) {
		cmdp->cmd_pkt.pkt_private = cmdp->cmd_pkt_private;
	}	
#else
	cmdp->cmd_pkt.pkt_private =
	    (opaque_t)ADDR2NCRUNITP(ap);
#endif
	nccbp->nc_cdblen = (unchar)cmdlen;

	/* The allocated cmd buffer might be larger than actual command.
	 * So the caller stores actual command length into my
	 * struct using this pointer I pass back here.
	 */
	cmdp->cmd_cdblen = nccbp->nc_cdblen;

	/*
 	 * If auto-rqsense, allocate and hook in a scsi_arq_status struct
	 * at pkt_scbp (with room for the extended sense data).  Otherwise,
	 * set up a STATUS_SIZE block, which is a sort of "max known to
	 * be required" size thing.
	 */

	if (ADDR2NCRUNITP(ap)->nt_arq)
		statbuflen = sizeof(struct scsi_arq_status);
	else
		statbuflen = STATUS_SIZE;

	if (statuslen > statbuflen)
		statbuflen = statuslen;

	cmdp->cmd_scblen = statbuflen;
	cmdp->cmd_pkt.pkt_scbp = kmem_alloc(statbuflen, kf);

	if (cmdp->cmd_pkt.pkt_scbp == NULL) {
		if (callback != NULL_FUNC && callback != SLEEP_FUNC)
			ddi_set_callback(callback, arg, &ncr_cb_id);
		kmem_free(nccbp, sizeof(*nccbp));
		NDBG12(("ncr_pktalloc: no status buffer\n"));
		return ((struct scsi_pkt *)NULL);
	}

	/* Set in address, which includes the transport structure, tgt, lun */
	cmdp->cmd_pkt.pkt_address = *ap;

	NDBG12(("ncr_pktalloc: okay nccbp=0x%x\n", nccbp));
	return ((struct scsi_pkt *)cmdp);
}

/*
 * (*tran_pktfree).  Free memory associated with the scsi_pkt (except
 * DMA resources...see ncr_dmafree).
 */

void
ncr_pktfree(struct scsi_pkt *pktp)
{
	nccb_t		*nccbp = PKTP2NCCBP(pktp);
	struct scsi_cmd	*cmdp = SCMD_PKTP(pktp);

	NDBG15(("ncr_pktfree: nccbp=0x%x\n", nccbp));
#if XXXPPC
	ASSERT(!(cmdp->cmd_flags & CFLAG_FREE));
#endif
	if (cmdp->cmd_privlen > PKT_PRIV_LEN) {
		kmem_free(pktp->pkt_private, cmdp->cmd_privlen);
	}
	kmem_free(pktp->pkt_scbp, cmdp->cmd_scblen);
	kmem_free(nccbp, sizeof(*nccbp));

	/*
 	 * If a callback is set, now's a good time to run it;
	 * the allocation it retries will probably succeed now.
 	 * The system will set ncr_cb_id to NULL when the callbacks
	 * are exhausted.
 	 */
	if (ncr_cb_id)
		ddi_run_callback(&ncr_cb_id);
}

/*
 * (*tran_dmaget).  DMA resource allocation.  This version assumes your
 * HBA has some sort of bus-mastering or onboard DMA capability, with a
 * scatter-gather list of length NCR_MAX_DMA_SEGS, as given in the
 * ddi_dma_lim_t structure defined in busops.c and passed to scsi_impl_dmaget.
 */

static struct scsi_pkt *
ncr_dmaget(	struct scsi_pkt	 *pktp,
		opaque_t	  dmatoken,
		int		(*callback)(),
		caddr_t		  arg )
{
	struct buf		*bp = (struct buf *)dmatoken;
	struct scsi_cmd		*cmdp = SCMD_PKTP(pktp);
	ddi_dma_cookie_t	 dmac;
	nccb_t			*nccbp;
	int			 cnt;
	int			 total_bytes;

	ncrti_t		 	*dmap;		/* ptr to the S/G list */

	NDBG26(("ncr_dmaget: cmdp=0x%x\n", cmdp));

	/*
	 * If your HBA hardware has flags for "data connected to this
	 * command" or "no data for this command", here's the
	 * appropriate place to set them, based on bp->b_bcount.
	 */

	if (!bp->b_bcount) {
		/* set for target examination */
		cmdp->cmd_pkt.pkt_resid = 0;
		NDBG26(("ncr_dmaget: zero\n"));
		return (pktp);
	}

	/*
	 * Record the direction of the data transfer, so that it
	 * can be correctly sync'd in tran_sync_pkt()
	 */
	if (bp->b_flags & B_READ)
		cmdp->cmd_cflags &= ~CFLAG_DMASEND;
	else
		cmdp->cmd_cflags |= CFLAG_DMASEND;

	/*
 	 * Set up DMA memory and position to the next DMA segment.
	 * Information will be in scsi_cmd on return; most usefully,
	 * in cmdp->cmd_dmaseg.
	 */

	if (!scsi_impl_dmaget(pktp, (opaque_t)bp, callback, arg,
		&(PKT2NCRUNITP(pktp)->nt_dma_lim))) {
		NDBG26(("ncr_dmaget: dmaget failed\n"));
		return (NULL);
	}

	/* Always use scatter-gather transfer */

	/*
	 * Use the loop below to store physical addresses of
	 * DMA segments, from the DMA cookies, into your HBA's
	 * scatter-gather list.  dmap is not defined, but should
	 * be an appropriate type to get into your HBA's data
	 * structures.
	 */

	/* point to the beginning of the HBA's scatter/gather list */
	nccbp = SCMDP2NCCBP(cmdp);
	dmap = &nccbp->nc_sg[0];

	for (total_bytes = cmdp->cmd_totxfer, cnt = 1; ; cnt++, dmap++) {
		off_t	offset;
		off_t	len;

		if (ddi_dma_segtocookie(cmdp->cmd_dmaseg, &offset, &len, &dmac)
		== DDI_FAILURE) {
			cmn_err(CE_CONT, "?ncr_dmaget: segtocookie failed\n");
			return (NULL);
		}

		total_bytes += dmac.dmac_size;

		/* store the segment parms into the S/G list */
		dmap->count = (ulong) dmac.dmac_size;
		dmap->address = (ulong) dmac.dmac_address;

		/* Check for end of list condition */
		if (bp->b_bcount <= total_bytes)
			break;

		/* Check for end of scatter-gather list */
		if (cnt >= NCR_MAX_DMA_SEGS)
			break;

		/* Get next DMA segment and cookie */
		if (ddi_dma_nextseg(cmdp->cmd_dmawin, cmdp->cmd_dmaseg,
				    &cmdp->cmd_dmaseg) != DDI_SUCCESS)
			break;
	}

	/* save the number of filled entries in S/G table */
	nccbp->nc_num = cnt;

	/* Inform target driver of length represented by resources */
	cmdp->cmd_totxfer = total_bytes;
	pktp->pkt_resid = bp->b_bcount - total_bytes;

	NDBG26(("ncr_dmaget: nccbp=0x%x num=%d bytes=%d %d\n"
			, nccbp, cnt, total_bytes, bp->b_bcount));

	return (pktp);
}


/*
 * (*tran_dmafree).  Free DMA resources associated with a scsi_pkt.
 * Memory for the packet itself is freed in ncr_pktfree.
 */

void
ncr_dmafree(	struct	scsi_address	*ap,
		struct	scsi_pkt	*pktp )
{
	struct scsi_cmd *cmd = SCMD_PKTP(pktp);
	NDBG13(("ncr_dmafree: pktp=0x%x\n", pktp));

	/* Free the mapping. */
	if (cmd->cmd_dmahandle) {
		ddi_dma_free(cmd->cmd_dmahandle);
		cmd->cmd_dmahandle = NULL;
	}
	PKTP2NCCBP(pktp)->nc_num = 0;
}

/*
 * (*tran_init_pkt).  This entry point merges the functionality of the  old
 * "tran_pktalloc()" and "tran_dmaget()" functions.
 */

struct scsi_pkt *
ncr_tran_init_pkt(	struct	scsi_address	*ap,
			struct	scsi_pkt	*pktp,
			struct	buf		*bp,
			int			 cmdlen,
			int			 statuslen,
			int			 tgtlen,
			int			 flags,
			int			 (*callback)(),
			caddr_t			 arg )
{
	struct scsi_pkt	*new_pkt = NULL;

	/*
	 * Allocate the new packet.
	 */
	if (!pktp) {
		pktp = ncr_pktalloc(ap, cmdlen, statuslen, tgtlen, callback
				      , arg);
		if (pktp == NULL)
			return (NULL);

		(SCMD_PKTP(pktp))->cmd_flags = flags;
		new_pkt = pktp;

	} else {
		new_pkt = NULL;
	}

	/*
	 * Set up dma info
	 */
	if (bp) {
		if (ncr_dmaget(pktp, (opaque_t)bp, callback, arg) == NULL) {
			if (new_pkt)
				ncr_pktfree(new_pkt);
			return (NULL);
		}
	}
	return (pktp);
}

/*
 * (*tran_sync_pkt).
 */
/*ARGSUSED*/
void
ncr_sync_pkt(	struct	scsi_address	*ap,
		struct	scsi_pkt	*pktp )
{
	int		 i;
	struct scsi_cmd	*cmd = SCMD_PKTP(pktp);

	if (cmd->cmd_dmahandle) {
		i = ddi_dma_sync(cmd->cmd_dmahandle, 0, 0,
		    (cmd->cmd_cflags & CFLAG_DMASEND) ?
		    DDI_DMA_SYNC_FORDEV : DDI_DMA_SYNC_FORCPU);
		if (i != DDI_SUCCESS) {
			cmn_err(CE_WARN, "ncr: sync pkt failed\n");
		}
	}
}

/*
 * (*tran_destroy_pkt).
 */

void
ncr_tran_destroy_pkt(	struct scsi_address	*ap,
			struct scsi_pkt		*pkt )
{
	ncr_dmafree(ap, pkt);
	ncr_pktfree(pkt);
}

/*
 * (*tran_tgt_init).
 */

int
ncr_tran_tgt_init(	dev_info_t 		*hba_dip,
			dev_info_t		*tgt_dip,
			scsi_hba_tran_t		*hba_tran,
			struct scsi_device	*sd )
{
	/*
	 * At this point, the scsi_device structure already exists
	 * and has been initialized.
	 *
	 * Use this function to allocate target-private data structures,
	 * if needed by this HBA.  Add revised flow-control and queue
 	 * properties for child here, if desired and if you can tell they
	 * support tagged queueing by now.
 	 */

	ncr_t		*ncrp;
	npt_t		*nptp;
	struct scsi_device *devp;
	int		len;
	int	 	targ;
	int	 	lun;
	char	 	name[MAXNAMELEN];
	char	 	mutexname[128];

	NDBG7(("ncr_tran_tgt_init\n"));
	ncrp = (ncr_t *)NCR_BLKP(SDEV2NCR(sd));

	targ	= sd->sd_address.a_target;
	lun	= sd->sd_address.a_lun;

#if defined(NCR_DEBUG)
	cmn_err(CE_CONT, "%s%d: %s%d <%d,%d>\n",
		ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
		ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
		targ, lun);
#endif	/* defined(NCR_DEBUG) */

	if (targ < 0 || targ > 7 || lun < 0 || lun > 7) {
		cmn_err(CE_WARN, "%s%d: %s%d bad address <%d,%d>\n",
			ddi_get_name(hba_dip), ddi_get_instance(hba_dip),
			ddi_get_name(tgt_dip), ddi_get_instance(tgt_dip),
			targ, lun);
		return (DDI_FAILURE);
	}

	/* copy per-target DMA limit structure from the HBA's array */
	nptp = NTL2UNITP(ncrp, targ, lun);
	nptp->nt_dma_lim = ncr_dma_lim;

	hba_tran->tran_tgt_private = nptp;

	/* 
	 * If you can do Inquiry commands to the child to find out
	 * that it's RDF_SCSI2 and it supports tagged queueing,
	 * (inq_cmdque bit is set), you can call ncr_childprop
	 * now to set up the new properties.
	 */

	/***/
	/*** ncr_childprop(dip, cdip);
	/***/

	NDBG7(("ncr_tran_tgt_init okay\n"));
 
	return(DDI_SUCCESS);
}

/*
 * (*tran_tgt_probe).
 */
/*ARGSUSED*/
int
ncr_tran_tgt_probe(	struct	scsi_device	*sd,
			int			(*callback)())
{
	int rval;
	
	rval = scsi_hba_probe(sd, callback);

#if defined(NCR_DEBUG)
	{
		char *s;
		struct ncr *ncr = SDEV2NCR(sd);

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
		cmn_err(CE_CONT, "ncr%d: %s target %d lun %d %s\n",
		    ddi_get_instance(NCR_DIP(ncr)), ddi_get_name(sd->sd_dev),
		    sd->sd_address.a_target, sd->sd_address.a_lun, s);
	}
#endif	/* defined(NCR_DEBUG) */

	return (rval);
}

/*
 * (*tran_tgt_free).  Undo initialization done in ncr_tran_tgt_init().
 */

void
ncr_tran_tgt_free(dev_info_t *hba_dip, dev_info_t *tgt_dip,
    scsi_hba_tran_t *hba_tran, struct scsi_device *sd)
{
	NDBG7(("ncr_tran_tgt_free okay\n"));
}
