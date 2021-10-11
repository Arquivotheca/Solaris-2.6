/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma	ident	"@(#)csa_impl.c	1.5	95/07/12 SMI"

#include "csa.h"

/*
 * Local functions
 */

static	void	csa_complete(csa_blk_t *csa_blkp);
static	void	csa_channel_clear(csa_blk_t *csa_blkp);
static	void	csa_error(struct csa_blk *csa_blkp, struct cmpkt *pktp,
			struct intr_info intr, cmd_t *clp, rblk_t *rbp);
static	Bool_t	csa_send_ccb(csa_blk_t *csa_blkp, ccb_t *ccbp,
			csa_ccb_t *csa_ccbp);
static	void	csa_start(csa_blk_t *csa_blkp);
static	Bool_t	csa_sense_status(csa_t *csap,
			struct identify_logical_drive_status *ldstatp);


/* ******************************************************************** */
/*									*/
/* The great IDA command list described pictorially.			*/
/*									*/
/* ******************************************************************** */
/* clp -> cmd_t								*/
/*      + ----------------------------------------------------------- + */
/*	| hdr  + ------------------------------------------------- +  |	*/
/*      |      |  chdr_t (command header)			   |  | */
/*      |      |	.ldrive:   8  (logical drive)		   |  | */
/*      |      |	.priority: 8 (priority scheduling by PRI)  |  | */
/*      |      |	.control:  16 (error options control word) |  | */
/*      |      + ------------------------------------------------- +  | */
/*      | reg  + ------------------------------------------------- +  | */
/*      |      | rblk_t (request block)				   |  | */
/*      |      | .hdr + ---------------------------------------- + |  | */
/*      |      |      |   .next : 16 (offset to next request blk)| |  | */
/*	|      |      |   .cmd  : 8  (read, write, etc.)	 | |  | */
/*	|      |      |   .rcode: 8  (error code)		 | |  | */
/*      |      |      |   .blk_num: 32 (block number)		 | |  | */
/*      |      |      |   .blk_count: 16 (block count)		 | |  | */
/*      |      |      |   .cnt1 : 8  (# of scat/gat count 1)     | |  | */
/*      |      |      |   .cnt2 : 8  (# of scat/gat count 2)     | |  | */
/*      |      |      + ---------------------------------------- + |  | */
/*      |      | .sg[0]  scatter/gather				   |  | */
/*      |      |      + ---------------------------------------- + |  | */
/*      |      |      |  ulong   .size				 | |  | */
/*      |      |      |  ulong   .address			 | |  | */
/*      |      |      + ---------------------------------------- + |  | */
/*	|      | ...						   |  | */
/*      |      | .sg[n]  scatter/gather				   |  | */
/*      |      + ------------------------------------------------- +  | */
/*      + ----------------------------------------------------------- + */
/* ******************************************************************** */



/*
 * Make certain the Standard Interface (i.e., the ATA interface
 * is disabled on the IDA and IDA-2 boards
 */

Bool_t
csa_bmic_mode(ushort ioaddr)
{
	unchar	version;
	unchar	config_reg = inb(ioaddr + CREG_CONFIG);

	/* check the IDA revision digit */
	version = inb(ioaddr + EISA_CFG3);

	/* only IDA and IDA-2 have Standard Interface */
	if (version == 0x01 || version == 0x02) {
		if (config_reg & CBIT_CONFIG_SIEN) {
			cmn_err(CE_WARN,
			    "csa(0x%x): Standard Interface is not disabled\n",
				ioaddr);
			return (FALSE);
		}
	}
	if (config_reg & CBIT_CONFIG_BMEN) {
		cmn_err(CE_WARN,
			"csa(0x%x): Bus Master Interface is not enabled\n",
			ioaddr);
		return (FALSE);
	}
	return (TRUE);
}



/* ******************************************************************** */
/*									*/
/*			csa_get_irq()					*/
/*									*/
/* Description:								*/
/*									*/
/* Returns    : Bool_t							*/
/*									*/
/* ******************************************************************** */

Bool_t
csa_get_irq(
	ushort	ioaddr,
	unchar	*irqp
)
{
	unchar	config = inb(ioaddr + CREG_CONFIG);

	if (config & CBIT_CONFIG_BMEN)
		return (FALSE);

	if (config & CBIT_CONFIG_BIE3)
		*irqp = 11;
	else if (config & CBIT_CONFIG_BIE2)
		*irqp = 10;
	else if (config & CBIT_CONFIG_BIE1)
		*irqp = 14;
	else if (config & CBIT_CONFIG_BIE0)
		*irqp = 15;
	else
		return (FALSE);
	return (TRUE);
}



/*
 * Name:	csa_dev_fini
 *
 * Description: disable the controller
 */

/*ARGSUSED*/
void
csa_dev_fini(
	csa_blk_t	*csa_blkp,
	ushort		 ioaddr
)
{
	/* turn off the device interrupts */
	CSA_BMIC_DISABLE(csa_blkp, ioaddr);

	/* mask the system doorbell interrupts */
	CSA_BMIC_MASK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);
	CSA_BMIC_MASK_COMMAND_COMPLETE(csa_blkp, ioaddr);

	/* clear any bogus pending interupts */
	CSA_BMIC_ACK_LIST_COMPLETE(csa_blkp, ioaddr);

	/* set controller completion-channel to all clear */
	CSA_BMIC_SET_COMPLETE_CHNL_CLEAR(csa_blkp, ioaddr);
	return;
}



/*
 * Name:	csa_dev_init
 *
 * Description: initialize the array controller by writing to the Bus
 *		Master Interface Control (BMIC) registers.
 */

/*ARGSUSED*/
void
csa_dev_init(
	csa_blk_t	*csa_blkp,
	ushort		 ioaddr
)
{
	/* clear any bogus pending interupts */
	CSA_BMIC_ACK_LIST_COMPLETE(csa_blkp, ioaddr);

	/* leave chan clear interrupt masked */
	CSA_BMIC_MASK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);

	/* unmask command complete notification interrupt */
	CSA_BMIC_UNMASK_COMMAND_COMPLETE(csa_blkp, ioaddr);

	/* set controller completion-channel to all clear */
	CSA_BMIC_SET_COMPLETE_CHNL_CLEAR(csa_blkp, ioaddr);

	/* enable interrupts on the controller */
	CSA_BMIC_ENABLE(csa_blkp, ioaddr);
	return;
}





/* ******************************************************************** */
/*									*/
/*			csa_send_readwrite				*/
/*									*/
/* Description: set up the command list and initiate the i/o.		*/
/*									*/
/* Called by  : csa_cmd()						*/
/*									*/
/* Returns    : none							*/
/*									*/
/* ******************************************************************** */

int
csa_send_readwrite(
	csa_t		*csap,
	struct cmpkt	*pktp,
	csa_blk_t	*csa_blkp,
	ccb_t		*ccbp
)
{
	csa_ccb_t	*csa_ccbp = CCBP2CSACCBP(ccbp);
	cmd_t	*clp;		/* pointer to command list */
	rblk_t	*rbp;		/* pointer to request block */

	clp = csa_ccbp->csa_ccb_clp;
	rbp = &clp->cl_req;

#if _SOLARIS_PS_RELEASE >= 250
	if (pktp->cp_passthru) {
		struct dadkio_rwcmd *rwcmdp;
		rwcmdp = (struct dadkio_rwcmd *)(pktp->cp_bp->b_back);

		switch (rwcmdp->cmd) {
		case DADKIO_RWCMD_READ:
			rbp->rb_hdr.rh_cmd = CSA_READ_SECTOR;
			break;
		case DADKIO_RWCMD_WRITE:
			rbp->rb_hdr.rh_cmd = CSA_WRITE_SECTOR;
			break;
		default:
			CDBG_RW(("csa_readwrite: pt: csa_ccbp=0x%x cdb=0x%x\n",
				 csa_ccbp, ccbp->ccb_cdb));
			return (FALSE);
		}
	} else
#endif
	{
		switch (ccbp->ccb_cdb) {
		case DCMD_READ:
			rbp->rb_hdr.rh_cmd = CSA_READ_SECTOR;
			break;
		case DCMD_WRITE:
			rbp->rb_hdr.rh_cmd = CSA_WRITE_SECTOR;
			break;
		default:
			CDBG_RW(("csa_readwrite: csa_ccbp=0x%x cdb=0x%x\n",
				 csa_ccbp, ccbp->ccb_cdb));
			return (FALSE);
		}
	}

	CDBG_RW(("csa_readwrite: drive=0x%x %s csa_blkp=0x%x csa_ccbp=0x%x\n",
		 csap->c_drive, (ccbp->ccb_cdb == DCMD_READ)
						? "READ " : "WRITE ",
		 csa_blkp, csa_ccbp));

	/*
	 * fill in the command block header
	 */

	/* all requests priority zero */
	clp->cl_hdr.priority = 0;

	/* get logical drive number */
	clp->cl_hdr.ldrive = csap->c_drive;

	/* set control word flags */
	clp->cl_hdr.control = (CTRL_ERRREQUEST | CTRL_ERRABORT);

	/*
	 * fill in the request block
	 */
	rbp = &clp->cl_req;		/* get request block ptr */
	rbp->rb_hdr.rh_blk_num = pktp->cp_srtsec; /* start sector number */
	rbp->rb_hdr.rh_next = 0;	/* mark last req block */
	rbp->rb_hdr.rh_status = 0;	/* return error code */
	rbp->rb_hdr.rh_cnt2 = 0;	/* scatter/gather count #2 */

	/*
	 * Queue the ccb and/or start the controller.
	 */
	if (!csa_send_ccb(csa_blkp, ccbp, csa_ccbp)) {
		return (FALSE);
	}

	CDBG_RW(("csa_readwrite: Exiting\n"));
	return (TRUE);
}



/* ******************************************************************** */
/*									*/
/*			csa_scatter_gather()				*/
/*									*/
/* Description: scatter gather DMA supported				*/
/* Called by  : csa_readwrite()						*/
/* Returns    : none							*/
/*									*/
/* ******************************************************************** */

/*ARGSUSED*/
void
csa_sg_func(
	struct cmpkt	*pktp,
	ccb_t		*ccbp,
	ddi_dma_cookie_t *cookiep,
	int		segno,
	opaque_t	arg
)
{
	rblk_t	*rbp = (rblk_t *)arg;	/* request block pointer */


	/* set the next S/G descriptor */
	rbp->rb_sg[segno].sg_size = cookiep->dmac_size;
	rbp->rb_sg[segno].sg_addr = cookiep->dmac_address;

	/* update the number of scatter/gathers */
	rbp->rb_hdr.rh_cnt1 = segno + 1;
	return;
}



/* ******************************************************************** */
/*									*/
/*			csa_intr_status()				*/
/*									*/
/* ******************************************************************** */

int
csa_intr_status(
	opaque_t	 arg1,
	opaque_t	*arg2
)
{
	csa_blk_t	*csa_blkp = (csa_blk_t *)arg1;
	ulong		*statp = (ulong *)arg2;
	unchar		 sys_doorbell;

	sys_doorbell = inb(csa_blkp->cb_ioaddr + CREG_SYS_DOORBELL);

	/* check for pending interrupts */
	if ((sys_doorbell & (CBIT_SDIR_READY | CBIT_SDIR_CLEAR)) == 0) {
		/*
		 * This is OK since two IDAs may be on the same interrupt
		 * this is how we tell which one is ready.
		 */
		return (FALSE);
	}
	*statp = sys_doorbell;
	return (TRUE);
}


/*ARGSUSED*/
void
csa_process_intr(
	opaque_t	arg,
	opaque_t	status
)
{
	csa_blk_t	*csa_blkp = (csa_blk_t *)arg;
	ushort		 ioaddr = csa_blkp->cb_ioaddr;
	unchar		 sys_doorbell;

	sys_doorbell = inb(ioaddr + CREG_SYS_DOORBELL);

	CDBG_INTR(("csa_intr: sys_doorbell = 0x%x \n", sys_doorbell));

	if (sys_doorbell & CBIT_SDIR_READY) {
#ifdef CSA_DEBUG
		csa_blkp->cb_ncomplete++;
#endif
		csa_complete(csa_blkp);
	}

	if (sys_doorbell & CBIT_SDIR_CLEAR)  {
		if (CSA_BMIC_STATUS_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr)) {
#ifdef CSA_DEBUG
			csa_blkp->cb_nclear++;
#endif
			csa_channel_clear(csa_blkp);
		} else {
#ifdef CSA_DEBUG
			cmn_err(CE_CONT, "csa(0x%x): not clear\n", ioaddr);
			csa_blkp->cb_nclear_not++;
#endif
			/* ack the interrupt */
			CSA_BMIC_ACK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);
		}
	}
	return;
}



/* ******************************************************************** */
/*									*/
/*				csa_complete()				*/
/*									*/
/* Description: BMIC_DATA_READY interrupt				*/
/*			1) successful completion of a transfer		*/
/*			2) error detected during a transfer and aborted */
/*									*/
/* Called by : csa_intr()						*/
/*									*/
/* Returns   : struct intr info or NULL for error			*/
/*									*/
/* ******************************************************************** */

static void
csa_complete(csa_blk_t *csa_blkp)
{
	ccb_t		*ccbp;
	csa_ccb_t	*csa_ccbp;
	struct cmpkt	*pktp;
	cmd_t		*clp;
	rblk_t		*rbp;
	struct intr_info intr;
	ushort		 ioaddr = csa_blkp->cb_ioaddr;

	/*
	 * if disk command complete is set then save cmd list addr,
	 * request offset, tag id, and list status
	 */
	intr.cl_paddr = inl(ioaddr + CREG_COMPLETE_ADDR);
	intr.offset   = inw(ioaddr + CREG_COMPLETE_OFFSET);
	intr.status   = inb(ioaddr + CREG_COMPLETE_STATUS);
	intr.tagid    = inb(ioaddr + CREG_COMPLETE_TAGID);

#ifdef CSA_DEBUG
if (intr.tagid == 0 || intr.tagid == 0xff) debug_enter("\n\ncsa_complete\n");
#endif

	/* reset command list complete interrupt sticky bit */
	CSA_BMIC_ACK_LIST_COMPLETE(csa_blkp, ioaddr);

	/* notify controller that completion channel is clear */
	CSA_BMIC_SET_COMPLETE_CHNL_CLEAR(csa_blkp, ioaddr);

	CDBG_INTR(("csa_complete(0x%x): command list = 0x%x\n", ioaddr,
		   intr.cl_paddr));

	ccbp = TAGID2CCBP(csa_blkp, intr.tagid);
	csa_ccbp = CCBP2CSACCBP(ccbp);

	CDBG_INTR(("csa_complete(0x%x): ccbp = 0x%x\n", ioaddr, csa_ccbp));

#ifdef CSA_DEBUG
	/*
	 * If these don't match my command lists and the controllers
	 * command lists are out of sync.
	 */
	if (csa_ccbp->csa_ccb_tagid != intr.tagid) {
		CDBG_ERROR(("csa_complete(0x%x): tag mismatch "
			    "0x%x 0x%x 0x%x\n", ioaddr,
			    csa_ccbp->csa_ccb_tagid, &intr, csa_ccbp));
		debug_enter("\n\ncsa_complete tag\n");
	}
	if (csa_ccbp->csa_ccb_paddr != intr.cl_paddr) {
		CDBG_ERROR(("csa_complete(0x%x): paddr mismatch "
			    "0x%x 0x%x 0x%x\n", ioaddr,
			    csa_ccbp->csa_ccb_paddr, &intr, csa_ccbp));
		debug_enter("\n\ncsa_complete paddr\n");
	}
#endif

	/* leave if not our interrupt */
	if (csa_ccbp->csa_ccb_status != CCB_CSENT) {
		CDBG_INTR(("csa_complete(0x%x): spurious interrupt? "
			   "cmd=0x%lx off=0x%x stat=0x%x tagid=0x%x\n",
			   ioaddr, intr.cl_paddr, intr.offset, intr.status,
			   intr.tagid));
		return;
	}
	CSA_CCB_ST_DONE(csa_blkp, csa_ccbp);

	/* get the request block pointer to check for errors */
	clp = csa_ccbp->csa_ccb_clp;
	rbp = &clp->cl_req;

#ifdef CSA_DEBUG
	/* the csa is supposed to set both status fields to the same value */
	if (rbp->rb_hdr.rh_status != (intr.status & RB_ANY_REQ_ERR)) {
		CDBG_ERROR(("csa_complete(0x%x): status mismatch "
			    "0x%x 0x%x 0x%x\n", ioaddr,
			    rbp->rb_hdr.rh_status, &intr, csa_ccbp));
		debug_enter("\n\ncsa_complete status\n");
	}
#endif

	/* check for aborted and fatal errors */
	pktp = CCBP2PKTP(ccbp);
	if (pktp != NULL) {
		if ((intr.status & RB_ANY_ERR) != 0) {
			csa_error(csa_blkp, pktp, intr, clp, rbp);
		} else {
			pktp->cp_reason = CPS_SUCCESS;
			pktp->cp_resid =  0;
		}
	}
	QueueAdd(&csa_blkp->cb_doneq, &ccbp->ccb_q, ccbp);

	return;
}



/* ******************************************************************** */
/*									*/
/*			csa_channel_clear()				*/
/*									*/
/* Description: BMIC_CHAN_CLEAR interrupt				*/
/*		ready to dequeue some command list that was waiting to  */
/*		be processed						*/
/*									*/
/* Called by : csa_intr()						*/
/*									*/
/* Returns   : none							*/
/*									*/
/* ******************************************************************** */

static void
csa_channel_clear(csa_blk_t *csa_blkp)
{
	ushort	ioaddr = csa_blkp->cb_ioaddr;

	/* mask the interrupt in case there's nothing waiting to be sent */
	CSA_BMIC_MASK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);

	/* ack the interrupt to clear it's interrupt status bit */
	CSA_BMIC_ACK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);

	/* send the next pending command list to the controller */
	csa_start(csa_blkp);

	CDBG_PEND_INTR(("csa_channel_clear: 0x%x\n", csa_blkp));
	return;

}



/* ******************************************************************** */
/*									*/
/*				csa_send_ccb()				*/
/*									*/
/* Description: Check if controller is busy if not then send a command	*/
/*		list to BMIC disk controller.				*/
/*									*/
/* Called by  : csa_readwrite()						*/
/*									*/
/* Return     : none							*/
/*									*/
/* ******************************************************************** */

static Bool_t
csa_send_ccb(
	csa_blk_t	*csa_blkp,
	ccb_t		*ccbp,
	csa_ccb_t	*csa_ccbp
)
{
	ushort	ioaddr = csa_blkp->cb_ioaddr;

	/* verify the state of this ccb */
	if (csa_ccbp->csa_ccb_status != CCB_CBUSY) {
		CDBG_SEND(("csa_send_ccb: wrong state: %c\n",
				 csa_ccbp->csa_ccb_status));
		return (FALSE);
	}

	/* mark ccb and command list as queued */
	CSA_CCB_ST_QUEUED(csa_blkp, csa_ccbp);
	QueueAdd(&csa_blkp->cb_waitq, &ccbp->ccb_q, ccbp);

	/*  if channel is not CLEAR, then queue request and */
	/*  enable channel clear interrupt */
	if (!CSA_BMIC_STATUS_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr)) {
		CSA_BMIC_UNMASK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);

		CDBG_SEND(("csa_send_ccb: channel is not CLEAR, "
			   "ccb queued 0x%x\n", csa_ccbp));
		return (TRUE);
	}

	csa_start(csa_blkp);	/* send command list to controller */
	CDBG_SEND(("csa_send_ccb: request sent to controller.\n"));
	return (TRUE);
}



/* ******************************************************************** */
/*									*/
/*				csa_start()				*/
/*									*/
/* Description: Send command list address and size to IDA controller.	*/
/*									*/
/* Called by  : csa_start(), pending_intr()				*/
/*									*/
/* Returns    : nothing							*/
/*									*/
/* ******************************************************************** */

static void
csa_start(csa_blk_t *csa_blkp)
{
	ccb_t		*ccbp;
	csa_ccb_t	*csa_ccbp;
	cmd_t		*clp;
	ulong		 cl_size;
	ushort		 ioaddr = csa_blkp->cb_ioaddr;

	if (QEMPTY(&csa_blkp->cb_waitq)) {
		return;
	}

	if ((ccbp = (ccb_t *)QueueRemove(&csa_blkp->cb_waitq)) == NULL) {
		CDBG_START(("csa_start: this shouldn't happen\n"));
		return;
	}
	/* get ptr to the csa portion of the ccb */
	csa_ccbp = CCBP2CSACCBP(ccbp);

	/* make certain it's in the right state */
	if (csa_ccbp->csa_ccb_status != CCB_CQUEUED) {
		CDBG_START(("csa_start: bogus ccb\n"));
		return;
	}

	/* mark cmd list as sent */
	CSA_CCB_ST_SENT(csa_blkp, csa_ccbp);

	/* set the physical addr */
	outl(ioaddr + CREG_SUBMIT_ADDR, csa_ccbp->csa_ccb_paddr);

	/* compute and set the size */
	clp = csa_ccbp->csa_ccb_clp;
	cl_size = sizeof (chdr_t) + sizeof (rhdr_t)
		+ (sizeof (sg_t) * clp->cl_req.rb_hdr.rh_cnt1)
		+ (sizeof (sg_t) * clp->cl_req.rb_hdr.rh_cnt2);
	outw(ioaddr + CREG_SUBMIT_LENGTH, cl_size);

	/* set the tag id */
	outb(ioaddr + CREG_SUBMIT_TAGID, csa_ccbp->csa_ccb_tagid);

	/*
	 * The command list is all set up and ready to start but
	 * don't start it until we decide whether we need to enable
	 * the submit-channel clear interrupt. I think if we were
	 * to reverse the order of the start and unmask steps there
	 * might be a window where we could miss getting the channel
	 * clear interrupt if the channel clear between those two
	 * steps.
	 */

	/*
	 * Reset channel CLEAR sticky bit in case it was previously set
	 * while the interrupt was masked off.
	 */
	CSA_BMIC_ACK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);

	/*
	 * Unmask the interrupt if more ccbs are in the queue. Do this
	 * before starting the current command to avoid opening a
	 * missed interrupt window (which might only exist if for some
	 * silly reason the controller is set to edge-triggered mode.)
	 */
	if (QEMPTY(&csa_blkp->cb_waitq)) {
		CSA_BMIC_MASK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);
	} else {
		CSA_BMIC_UNMASK_SUBMIT_CHNL_CLEAR(csa_blkp, ioaddr);
	}

	/* notify BMIC that command list is ready */
	CSA_BMIC_COMMAND_SUBMIT(csa_blkp, ioaddr);
	CDBG_START(("csa_start: command at 0x%x\n", csa_ccbp->csa_ccb_paddr));
#ifdef CSA_DEBUG
	csa_blkp->cb_nsent++;
#endif
	return;
}



/* ******************************************************************** */
/*									*/
/*				csa_ccballoc()				*/
/*									*/
/* Description: Allocate a free command block.  Must have only one	*/
/*		process to enter this function at a time.		*/
/*									*/
/* Called by  : csa_readwrite()						*/
/*									*/
/* Returns    : free command block descriptor				*/
/*									*/
/* ******************************************************************** */

ccb_t *
csa_ccballoc(opaque_t arg)
{
	csa_blk_t	*csa_blkp = (csa_blk_t *)arg;
	ccb_t		*ccbp;
	csa_ccb_t	*csa_ccbp;

#ifdef CSA_DEBUG
	if (!mutex_owned(&csa_blkp->cb_rmutex)) {
debug_enter("\ncsa_ccballoc()\n");
	}
#endif

	if (!(ccbp = (ccb_t *)QueueRemove(&csa_blkp->cb_free_ccbs))) {
		/* all the ccb's are allocated */
#ifdef CSA_DEBUG
		CDBG_ERROR(("csa_ccballoc(0x%x): empty\n",
			    csa_blkp->cb_ioaddr));
#endif
		return (NULL);
	}

	/* get ptr to csa private portion of ccb */
	csa_ccbp = CCBP2CSACCBP(ccbp);

	ASSERT(csa_ccbp->csa_ccb_status == CCB_CFREE);

	/* initialize the common-ccb fields */
	bzero((caddr_t)&csa_ccbp->csa_ccb_common, sizeof (ccb_t));
	csa_ccbp->csa_ccb_common.ccb_private = csa_ccbp;
	csa_ccbp->csa_ccb_common.ccb_pktp = &csa_ccbp->csa_ccb_cmpkt;

	/* mark buffer busy and return ptr to common-ccb to caller */
	CSA_CCB_ST_BUSY(csa_blkp, csa_ccbp);
	return (ccbp);
}



/* ******************************************************************** */
/*									*/
/*				csa_ccbfree()				*/
/*									*/
/* Description: Free a command list allocated by csa_alloc().		*/
/*									*/
/* Called by  : csa_intr()						*/
/*									*/
/* Returns    : none							*/
/*									*/
/* ******************************************************************** */

void
csa_ccbfree(
	opaque_t	arg,
	ccb_t		*ccbp
)
{
	csa_blk_t	*csa_blkp = (csa_blk_t *)arg;
	csa_ccb_t	*csa_ccbp = CCBP2CSACCBP(ccbp);

#ifdef CSA_DEBUG
	if (csa_ccbp->csa_ccb_status != CCB_CBUSY
	&&  csa_ccbp->csa_ccb_status != CCB_CDONE) {
		CDBG_ERROR(("csa_ccbfree(): 0x%x 0x%x\n", csa_blkp, csa_ccbp));
debug_enter("\n\ncsa_ccbfree\n");
	}

	if (!mutex_owned(&csa_blkp->cb_rmutex)) {
debug_enter("\ncsa_ccbfree()\n");
	}
#endif

	/* mark the command block free */
	CSA_CCB_ST_FREE(csa_blkp, csa_ccbp);

	/* add it to the free list */
	QueueAdd(&csa_blkp->cb_free_ccbs, &ccbp->ccb_q, ccbp);

	return;
}



/*
 *      The buffer for the cmd_t, Command List, structure is allocated
 *      separately from the csa_ccb_t structure.  Each cmd_t buffer has
 *      to be in a single page or has to be in two physically contiguous
 *      pages.  If the cmd_t ever grows to larger than two pages then
 *	this routine has to be fixed.
 */

Bool_t
csa_ccbinit(csa_blk_t *csa_blkp)
{
	caddr_t		 bufp;
	cmd_t		*clp;
	csa_ccb_t	*csa_ccbp;
	ccb_t		*ccbp;
	int		 nccbs;
	Que_t		 good_bufq;
	Que_t		 bad_bufq;
	int		 nbad_bufs = 0;
	unchar		 tag_id = 0;

	QUEUE_INIT(&good_bufq);
	QUEUE_INIT(&bad_bufq);
	/*
	 * Sanity check the size. This routine can't handle more than
	 * two physically pages per cmd_t. It should never be this large but
	 * play it safe by varifyin this assumption.
	 */
	if (sizeof (cmd_t) > ptob(2)) {
		return (FALSE);
	}

	/* Sanity check the nccbs parameter */
	if ((nccbs = csa_blkp->cb_nccbs) <= 16)
		nccbs = 16;
	else if (nccbs > CSA_MAX_CMDS) {
		nccbs = CSA_MAX_CMDS;
	}

	cmn_err(CE_CONT, "?csa(0x%x): allocating %d CCBs.\n",
		csa_blkp->cb_ioaddr, nccbs);

	/* allow upto 3 times as many "bad" buffers to be skipped */
	nbad_bufs = nccbs * 3;

	/*
	 * Allocate all the Command List buffers first so that
	 * they'll tend to be contiguous and minimize the number
	 * of "bad" page breaks.
	 */

	while (nccbs) {
		u_int	pnum;
		u_int	pnum_end;

		/* allocate the cmd_t buffer but don't clear it until later */
		if (!(bufp = kmem_alloc(sizeof (cmd_t), KM_SLEEP))) {
			goto failed;
		}

		/*
		 * get the page frame number of the base of the buffer
		 * and the page frame number of the end of the buffer
		 */
		pnum = hat_getkpfnum(bufp);
		pnum_end = hat_getkpfnum(bufp + sizeof (cmd_t) - 1);

		/*
		 * If it fits in a single physical page or if the
		 * pages are physically contiguous page frames, then ...
		 */
		if (pnum == pnum_end
		|| (pnum + 1) == pnum_end) {
			/* ... it's a good one, save it for later. */
			QueueAdd(&good_bufq, (Qel_t *)bufp, bufp);
			nccbs--;
			continue;
		}

		/* skip this one and try another kmem_alloc() */
		QueueAdd(&bad_bufq, (Qel_t *)bufp, bufp);
		if (nbad_bufs-- < 0) {
			/* we've skipped too many bufs, give up */
			goto failed;
		}
	}

	/* now link all the good Command List buffers to csa_ccb_t's */
	while (bufp = (caddr_t)QueueRemove(&good_bufq)) {
		/* now clear the cmd_t, the Qinsert routine modified it */
		bzero(bufp, sizeof (cmd_t));

		/* allocate a csa_ccb_t structure and link to cmd_t */
		if (!(csa_ccbp = kmem_zalloc(sizeof (csa_ccb_t), KM_SLEEP))) {
			kmem_free(bufp, sizeof (cmd_t));
			goto failed;
		}

		clp = (cmd_t *)bufp;

		/* link the two buffers together */
		clp->cl_csa_ccbp = csa_ccbp;
		csa_ccbp->csa_ccb_clp = clp;

		csa_ccbp->csa_ccb_paddr = CSA_KVTOP(bufp);
		csa_ccbp->csa_ccb_tagid = tag_id;

		/* link the common-ccb and common-pkt portions */
		ccbp = &csa_ccbp->csa_ccb_common;
		ccbp->ccb_private = csa_ccbp;
		ccbp->ccb_pktp = &csa_ccbp->csa_ccb_cmpkt;
		ccbp->ccb_pktp->cp_ctl_private = ccbp;

		/* insert it into the tag id lookup table */
		TAGID2CCBP(csa_blkp, tag_id) = ccbp;

		/*
		 * the first ccb is reserved for csa_cache_flush()
		 * don't put it on the free list.
		 */
		CSA_CCB_ST_BUSY(csa_blkp, csa_ccbp);
		if (tag_id != 0) {
			/* add it to the free list */
			csa_ccbfree((opaque_t)csa_blkp, ccbp);
		}
		tag_id++;
	}

	/* free any buffers which had bad page breaks */
	while (bufp = (caddr_t)QueueRemove(&bad_bufq))
		kmem_free(bufp, sizeof (cmd_t));
	return (TRUE);


failed:
	/* free the buffers with bad page breaks */
	while (bufp = (caddr_t)QueueRemove(&bad_bufq))
		kmem_free(bufp, sizeof (cmd_t));

	/* free the good ones also */
	while (bufp = (caddr_t)QueueRemove(&good_bufq))
		kmem_free(bufp, sizeof (cmd_t));

	/* and free any that were inserted in the free list */
	while (ccbp = (ccb_t *)QueueRemove(&csa_blkp->cb_free_ccbs)) {
		csa_ccbp = CCBP2CSACCBP(ccbp);
		kmem_free(csa_ccbp->csa_ccb_clp, sizeof (cmd_t));
		kmem_free(csa_ccbp, sizeof (csa_ccb_t));
	}
	return (FALSE);

}



/* ******************************************************************** */
/*									*/
/*			csa_error()					*/
/*									*/
/* Description: Prints error messages and return proper error code	*/
/*									*/
/* Called by  : csa_intr()						*/
/*									*/
/* Returns    : Error code						*/
/*									*/
/* ******************************************************************** */

void
csa_error(
	csa_blk_t	*csa_blkp,
	struct cmpkt	*pktp,
	struct intr_info intr,
	cmd_t		*clp,	/* ptr to command list */
	rblk_t		*rbp	/* ptr to request block that caused the error */
)
{
	unchar		 rc;
	char		*errmsg;

	/*
	 * csa controller doesn't give back a residual byte count so
	 * just claim nothing transferred.
	 */
	pktp->cp_resid = pktp->cp_bytexfer;

	pktp->cp_reason = CPS_CHKERR;

#if 0
this doesn't look right to me, leave it out
	/* ??? force no retry ??? */
	pktp->cp_retry = 1;
#endif


	if (intr.status & RB_CMDLIST_ERR) {
		/*
		 * bad command list: incorrectly filled or corrupted
		 * command list
		 */
		rbp->rb_hdr.rh_status |= RB_CMDLIST_ERR;
		errmsg = "bad cmd list";
		rc = DERR_INVCDB;

	} else if (rbp->rb_hdr.rh_status & RB_RECOV_ERR) {
		/*
		 * recoverable error, possibly slavaged by retry or
		 * fault tolerance
		 */
		errmsg = "recoverable error";
		rc = DERR_SUCCESS;

	} else if (rbp->rb_hdr.rh_status & RB_FATAL_ERR) {
		/* fatal error */
		errmsg = "fatal error";
		rc = DERR_HARD;

	} else if (rbp->rb_hdr.rh_status & RB_ABORT_ERR) {
		/*
		 * req blk is aborted possibly due to other req blk
		 * of same cmd list
		 */
		errmsg = "error aborted";
		rc = DERR_ABORT;

	} else if (rbp->rb_hdr.rh_status & RB_REQ_ERR) {
		/*
		 * bad request block: incorrectly filled or corrupted
		 * request block
		 */
		errmsg = "bad req blk";
		rc = DERR_INVCDB;

	} else {
		/* otherwise it's an unknown error */
		errmsg = "unknown";
		rc = DERR_HARD;
	}

	CDBG_ERROR(("?csa_error(0x%x): error=0x%x, %s, drive=0x%x blk=0x%x\n",
		    csa_blkp->cb_ioaddr,
		    rbp->rb_hdr.rh_status, errmsg, clp->cl_hdr.ldrive,
		    rbp->rb_hdr.rh_blk_num));

	if (pktp->cp_scbp)
		*((unchar *)(pktp->cp_scbp)) = rc;
	return;
}


static struct cmpkt *
csa_pktsetup(
	csa_t	*csap,
	int	 rw,
	caddr_t	 bufferp,
	ulong	 bufsize

)
{
	struct	buf	*bp;
	struct cmpkt	*pktp;

	/* allocate a buf structure */
	if (!(bp = getrbuf(KM_SLEEP))) {
		CDBG_ERROR(("csa_pktsetup: getrbuf() failed\n"));
		goto bailout1;
	}

	/* allocate a packet and ccb to handle this request */
	if (!(pktp = csa_pktalloc(csap, NULL, NULL))) {
		CDBG_ERROR(("csa_pktsetup: pktalloc() failed\n"));
		goto bailout2;
	}

	/* initialize the buf header */
	bp->b_flags = B_KERNBUF | B_PHYS | B_BUSY;
	bp->b_un.b_addr = bufferp;
	bp->b_bcount = bufsize;
	if (rw == B_READ) {
		bp->b_flags |= B_READ;
		*((unchar *)pktp->cp_cdbp) = DCMD_READ;
	} else {
		*((unchar *)pktp->cp_cdbp) = DCMD_WRITE;
	}

	/* link the buf and pkt */
	pktp->cp_bp = bp;
	pktp->cp_bytexfer = bufsize;

	if (!csa_memsetup(csap, pktp, bp, NULL, NULL)) {
		CDBG_ERROR(("csa_pktsetup: memsetup() failed\n"));
		goto bailout3;
	}


	if (!csa_iosetup(csap, pktp)) {
		CDBG_ERROR(("csa_pktsetup: iosetup() failed\n"));
		goto bailout4;
	}
	return (pktp);

bailout4:
	csa_memfree(csap, pktp);
bailout3:
	csa_pktfree(csap, pktp);
bailout2:
	freerbuf(bp);
bailout1:
	return (NULL);

}


static Bool_t
csa_pktsend(
	csa_t		*csap,
	struct cmpkt	*pktp
)
{
	csa_blk_t	*csa_blkp;
	ccb_t		*ccbp;
	csa_ccb_t	*csa_ccbp;
	cmd_t		*clp;

	csa_blkp = CSAP2CSABLKP(csap);
	ccbp = PKTP2CCBP(pktp);
	csa_ccbp = CCBP2CSACCBP(ccbp);

	/*
	 * Queue the ccb and/or start the controller.
	 */
	pktp->cp_callback = NULL;
	if (!csa_send_ccb(csa_blkp, ccbp, csa_ccbp)) {
		CDBG_ERROR(("csa_pktsend: failed #1\n"));
		return (FALSE);
	}

	/* wait for the command to complete */
	if (!csa_pollret(csa_blkp, ccbp)) {
		CDBG_ERROR(("csa_pktsend: failed #2\n"));
		return (FALSE);
	}

	clp = csa_ccbp->csa_ccb_clp;
	if (clp->cl_req.rb_hdr.rh_status & RB_ANY_ERR) {
		CDBG_ERROR(("csa_pktsend: status=0x%x\n",
			    clp->cl_req.rb_hdr.rh_status));
		return (FALSE);
	}
	return (TRUE);
}



static void
csa_pktfini(
	csa_t		*csap,
	struct cmpkt	*pktp
)
{
	struct	buf	*bp = pktp->cp_bp;

	pktp->cp_bp = NULL;
	csa_memfree(csap, pktp);
	csa_pktfree(csap, pktp);
	freerbuf(bp);
	return;
}


static Bool_t
csa_sense_status(
	csa_t					*csap,
	struct identify_logical_drive_status	*ldstatp
)
{
	struct cmpkt	*pktp;
	cmd_t		*clp;
	Bool_t		 rc;

	/*
	 * set up the buf and packet and map it for DMA S/G
	 */
	pktp = csa_pktsetup(csap, B_READ, (caddr_t)ldstatp, sizeof (*ldstatp));
	if (!pktp) {
		CDBG_ERROR(("csa_sense_status: failed #1\n"));
		return (FALSE);
	}

	/* setup the command list header */
	clp = CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_clp;
	clp->cl_hdr.priority = 0;
	clp->cl_hdr.ldrive = csap->c_drive;
	clp->cl_hdr.control = (CTRL_ERRREQUEST | CTRL_ERRABORT);

	/* setup the request block */
	clp->cl_req.rb_hdr.rh_cmd = CSA_ID_LDSTATUS;
	clp->cl_req.rb_hdr.rh_blk_cnt = sizeof (*ldstatp) / 512;

	/* send it and wait for the response */
	rc = csa_pktsend(csap, pktp);

	/* free all the DMA resources and return */
	csa_pktfini(csap, pktp);
	return (rc);
}




Bool_t
csa_id_ldrive(
	csa_t				*csap,
	struct identify_logical_drive	*id_ldrivep
)
{
	struct cmpkt	*pktp;
	cmd_t		*clp;
	Bool_t		 rc;

	/*
	 * set up the buf and packet and map it for DMA S/G
	 */
	pktp = csa_pktsetup(csap, B_READ, (caddr_t)id_ldrivep,
			   sizeof (*id_ldrivep));
	if (!pktp) {
		CDBG_ERROR(("csa_id_ldrive: failed #1\n"));
		return (FALSE);
	}

	/* setup the command list header */
	clp = CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_clp;
	clp->cl_hdr.priority = 0;
	clp->cl_hdr.ldrive = csap->c_drive;
	clp->cl_hdr.control = (CTRL_ERRREQUEST | CTRL_ERRABORT);

	/* setup the request block */
	clp->cl_req.rb_hdr.rh_cmd = CSA_ID_LDRIVE;
	clp->cl_req.rb_hdr.rh_blk_cnt = sizeof (*id_ldrivep) / 512;

	/* send it and wait for the response */
	rc = csa_pktsend(csap, pktp);

	/* free all the DMA resources and return */
	csa_pktfini(csap, pktp);
	return (rc);
}



Bool_t
csa_id_ctlr(
	csa_t				*csap,
	struct identify_controller	*id_ctlrp
)
{
	struct cmpkt	*pktp;
	cmd_t		*clp;
	Bool_t		 rc;

	/*
	 * set up the buf and packet and map it for DMA S/G
	 */
	pktp = csa_pktsetup(csap, B_READ, (caddr_t)id_ctlrp,
			   sizeof (*id_ctlrp));
	if (!pktp) {
		CDBG_ERROR(("csa_id_ctlr: failed #1\n"));
		return (FALSE);
	}

	/* setup the command list header */
	clp = CCBP2CSACCBP(PKTP2CCBP(pktp))->csa_ccb_clp;
	clp->cl_hdr.priority = 0;
	clp->cl_hdr.ldrive = csap->c_drive;
	clp->cl_hdr.control = (CTRL_ERRREQUEST | CTRL_ERRABORT);

	/* setup the request block */
	clp->cl_req.rb_hdr.rh_cmd = CSA_ID_CTLR;
	clp->cl_req.rb_hdr.rh_blk_cnt = sizeof (*id_ctlrp) / 512;

	/* send it and wait for the response */
	rc = csa_pktsend(csap, pktp);

	/* free all the DMA resources and return */
	csa_pktfini(csap, pktp);
	return (rc);
}


/*ARGSUSED*/
int
csa_inquiry(
	dev_info_t	*mdip,
	dev_info_t	*cdip,
	csa_t		*csap
)
{
	union buff {
		struct identify_logical_drive_status	ldstat;
		struct identify_logical_drive		id_ldrive;
		struct identify_controller		id_ctlr;
	};
	struct scsi_device	*sdp;
	struct scsi_inquiry	*inqp;
	union buff		*buffp;
	ulong			 block_size;

	/*
	 * allocate a kernel buffer big enough for any of the three responses
	 */
	buffp = (union buff *)kmem_alloc(sizeof (union buff), KM_SLEEP);
	if (!buffp) {
		CDBG_SEND(("csa_inquiry: kmem_alloc() failed\n"));
		goto bailout1;
	}

	/*
	 * First get the drive status
	 */
	if (!csa_sense_status(csap, &buffp->ldstat)) {
		/* no logical drive defined */
		CDBG_SEND(("csa_inquiry: invalid logical drive\n"));
		goto bailout1;
	}

	if (buffp->ldstat.unit_status != LDR_OPER) {
		/* drive is defined but not operational */
		CDBG_SEND(("csa_inquiry: not operational\n"));
		goto bailout1;
	}

	/* send Identify Logical Drive to get the sector size */
	if (!csa_id_ldrive(csap, &buffp->id_ldrive)) {
		CDBG_SEND(("csa_inquiry: failed #3\n"));
		goto bailout1;
	}

	/* adjust DMA limits for the true block size of the Logical Drive */
	block_size = buffp->id_ldrive.block_size_in_bytes;
	csap->c_unitp->cu_dmalim.dlim_granular = block_size;
	csap->c_unitp->cu_dmalim.dlim_reqsize = block_size * ((1 << 16) - 1);

	if (!(inqp = kmem_alloc(sizeof (*inqp), KM_NOSLEEP))) {
		CDBG_SEND(("csa_inquiry: failed #4\n"));
		goto bailout1;
	}

	bzero((caddr_t)inqp, sizeof (*inqp));
	inqp->inq_dtype = DTYPE_DIRECT;
	strcpy(inqp->inq_vid, "Compaq");	/* vendor ID */
	strcpy(inqp->inq_pid, "SMART");		/* product ID */

	/* send Identify Controller */
	if (csa_id_ctlr(csap, &buffp->id_ctlr)) {
		strncpy(inqp->inq_revision,
			(char *)buffp->id_ctlr.ascii_firmware_revision, 4);
		CDBG_SEND(("csa_inquiry: rev=%s\n",
			(char *)buffp->id_ctlr.ascii_firmware_revision));
	}

	sdp = (struct scsi_device *)ddi_get_driver_private(cdip);
	sdp->sd_inq = inqp;
	kmem_free((caddr_t)buffp, sizeof (union buff));
	return (TRUE);

bailout1:
	kmem_free((caddr_t)buffp, sizeof (union buff));
bailout:
	return (FALSE);
}



Bool_t
csa_get_ldgeom(
	csa_t	*csap,
	struct tgdk_geom *tg
)
{
	csa_blk_t	*csa_blkp = CSAP2CSABLKP(csap);
	struct identify_logical_drive	*ldp;
	struct logical_parameter_table	*lpp;

	ldp = kmem_alloc(sizeof (*ldp), KM_SLEEP);
	if (!ldp) {
		CDBG_SEND(("csa_get_ldgeom: failed #1\n"));
		goto bailout1;
	}

	mutex_enter(&csa_blkp->cb_mutex);

	if (!csa_id_ldrive(csap, ldp)) {
		CDBG_SEND(("csa_get_ldgeom: failed #2\n"));
		goto bailout2;
	}

	lpp = (struct logical_parameter_table *)
		&ldp->logical_drive_parameter_table[0];

	tg->g_cyl	= lpp->cylinders;
	tg->g_acyl	= 0;
	tg->g_head	= lpp->heads;
	tg->g_sec	= lpp->sectors_per_track;
	tg->g_secsiz	= ldp->block_size_in_bytes;
	tg->g_cap	= ldp->blocks_available;

	ASSERT(tg->g_cap == tg->g_cyl * tg->g_head * tg->g_sec);

	mutex_exit(&csa_blkp->cb_mutex);

	kmem_free((caddr_t)ldp, sizeof (*ldp));
	return (TRUE);

bailout2:
	mutex_exit(&csa_blkp->cb_mutex);
	kmem_free((caddr_t)ldp, sizeof (*ldp));
bailout1:
	return (FALSE);
}


/*
 *	Called when the system is being halted to disable all hardware
 *	interrupts.
 *
 */

/*ARGSUSED*/
int
csa_flush_cache(dev_info_t *dip, ddi_reset_cmd_t cmd)
{
	csa_ccb_t		*csa_ccbp;
	ccb_t			*ccbp;
	csa_blk_t		*csa_blkp;
	cmd_t			*clp;
	rblk_t			*rbp;
	struct flush_disable	*fdp;
	struct flush_disable	 fd_cmd[2];

	csa_blkp = (csa_blk_t *)ddi_get_driver_private(dip);
	mutex_enter(&csa_blkp->cb_mutex);

	/* the first ccb is reserved for csa_flush_disable() */
	ccbp = TAGID2CCBP(csa_blkp, 0);
	csa_ccbp = CCBP2CSACCBP(ccbp);
	CSA_CCB_ST_BUSY(csa_blkp, csa_ccbp);

	clp = csa_ccbp->csa_ccb_clp;
	rbp = &clp->cl_req;

	/* setup the request block */
	bzero((caddr_t)clp, sizeof (*clp));
	rbp->rb_hdr.rh_cmd = CSA_FLUSH_CACHE;
	rbp->rb_hdr.rh_status = 0;
	rbp->rb_hdr.rh_blk_cnt = 1;
	rbp->rb_hdr.rh_cnt1 = 1;

	/*
	 * don't let the buffer for the flush command cross a page boundary
	 */
	fdp = fd_cmd;
	if (CSA_KVTOP(fdp) != CSA_KVTOP(fdp + 1)) {
		fdp++;
	}
	rbp->rb_sg[0].sg_addr = CSA_KVTOP(fdp);
	rbp->rb_sg[0].sg_size = sizeof fd_cmd;

	/*
	 * setup the command to flush and then reenable the cache
	 */
	bzero((caddr_t)fdp, sizeof (struct flush_disable));
	fdp->disable_flag = CSA_FLUSH_N_ENABLE;


	/*
	 * Queue the ccb and/or start the controller.
	 */
	if (!csa_send_ccb(csa_blkp, ccbp, csa_ccbp)) {
		CDBG_ERROR(("csa_flush_cache: failed #1\n"));
		goto bailout;
	}

	/*
	 * wait for the command to complete
	 */
	if (!csa_pollret(csa_blkp, ccbp)) {
		CDBG_ERROR(("csa_flush_cache: failed #2\n"));
		goto bailout;
	}

	if (clp->cl_req.rb_hdr.rh_status & RB_ANY_ERR) {
		CDBG_ERROR(("csa_flush_cache: status=0x%x\n",
			    clp->cl_req.rb_hdr.rh_status));
		goto bailout;
	}

	CSA_CCB_ST_FREE(csa_blkp, csa_ccbp);
	mutex_exit(&csa_blkp->cb_mutex);
	return (0);

bailout:
	CSA_CCB_ST_FREE(csa_blkp, csa_ccbp);
	mutex_exit(&csa_blkp->cb_mutex);
	return (1);
}
