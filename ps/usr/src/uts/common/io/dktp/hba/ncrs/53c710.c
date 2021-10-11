/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)53c710.c	1.12	96/06/03 SMI"

#include <sys/dktp/ncrs/ncr.h>
#include <sys/dktp/ncrs/53c710.h>
#include <sys/dktp/ncrs/script.h>

#include "ncr_sni.h"

/*
 * EISA product ids accepted by this driver for slots 0x1000 to 0xf000
 */
int	ncr53c710_eisa_board_ids[] = {
		0x1144110e	/* Compaq part #1089, rev 1 */
};

/*
 * Save the following bits in the specified registers
 * during a chip reset. The bits are established by the
 * HBA's POST BIOS and are very hardware dependent.
 */
static	nrs_t	ncr53c710_reg_save[] = {
	/* NOTE: DCNTL must always be first write after reset */
	{ NREG_DCNTL,	(NB_DCNTL_CF | NB_DCNTL_EA | NB_DCNTL_FA) },
	{ NREG_SCNTL0,	NB_SCNTL0_EPG },
	{ NREG_CTEST0,	NB_CTEST0_GRP },
	{ NREG_CTEST4,	NB_CTEST4_MUX },
	{ NREG_CTEST7,	(NB_CTEST7_CDIS | NB_CTEST7_SC | NB_CTEST7_TT1
			| NB_CTEST7_DFP | NB_CTEST7_EVP | NB_CTEST7_DIFF) },
	{ NREG_CTEST8,	(NB_CTEST8_FM | NB_CTEST8_SM) },
	{ NREG_DMODE,	(NB_DMODE_BL | NB_DMODE_FC |NB_DMODE_PD
			| NB_DMODE_FAM | NB_DMODE_U0) }
};

#define	NRegSave	(sizeof ncr53c710_reg_save / sizeof (nrs_t))

static	char	*dstatbits = "\
\020\
\010DMA-FIFO-empty\
\07reserved\
\06bus-fault\
\05aborted\
\04single-step-interrupt\
\03SCRIPTS-interrupt-instruction\
\02watchdog-timeout\
\01illegal-instruction";

static bool_t
ncr53c710_probe(	dev_info_t	*dip,
			int		*regp,
			int		 len,
			int		*pidp,
			int		 pidlen,
			bus_t		 bus_type,
			bool_t		 probing )
{
	ioadr_t	slotadr = *regp;
	int	cnt;
	

	if (bus_type != BUS_TYPE_EISA)
		return (FALSE);
	
	/* allow product-id property to override the default EISA id */
	if (pidp == NULL) {
		if (slotadr < 0x1000)
			return (FALSE);

		/* Handle normal EISA cards in a normal EISA slot */
		pidp = ncr53c710_eisa_board_ids;
		pidlen = sizeof ncr53c710_eisa_board_ids;
	}

	/* check the product id of the current slot for a match */
	for (cnt = pidlen / sizeof (int); cnt > 0; cnt--, pidp++) {
		if (eisa_probe(slotadr, *pidp)) {
			NDBG4(("ncr53c710_probe success %d %d\n", *pidp, cnt));
			return (TRUE);
		}
	}
	return (FALSE);
}

void
ncr53c710_reset( ncr_t *ncrp )
{
	unchar	regbuf[NRegSave];

	NDBG10(("ncr53c710_reset: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	ncr_save_regs(ncrp, ncr53c710_reg_save, regbuf, sizeof regbuf);

	/* Reset the 53C710 chip */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT, NB_ISTAT_RST);

	/* wait a tick and then turn the reset bit off */
	drv_usecwait(100);
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT, 0);

	/* clear any pending SCSI interrupts */
	(void)ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT0);

	/* need short delay before reading DSTAT */
	(void)ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCH);
	(void)ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCH);

	/* clear any pending DMA interrupts */
	(void)ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DSTAT);

	ncr_restore_regs(ncrp, ncr53c710_reg_save, regbuf, sizeof (regbuf));

	NDBG1(("ncr53c710_reset: Software reset completed\n"));
	return;
}

void
ncr53c710_init( ncr_t *ncrp )
{
	int	clock;

	NDBG1(("ncr53c710_init: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	/* set script to 53c710 mode */
	ClrSetBits(ncrp, NREG_SCRATCH0, 0, NBIT_IS710);

	/* Set the scsi id bit to match the HBA's idmask */
	ClrSetBits(ncrp, NREG_SCID, 0, ncrp->n_idmask);

	/* Enable Selection/Reselection */
	/* disable extra clock cycle of data setup so 100ns fast scsi works */
	ClrSetBits(ncrp, NREG_SCNTL1, NB_SCNTL1_EXC, NB_SCNTL1_ESR);

	/* Disable Byte-to-Byte timer. According to NCR errata, */
	/* if this bit is set then we get bus timeout interrupt */
	/* every 1 ms in case of a catastrophic bus failure */ 
	/* Enable Active Neagation */
	ClrSetBits(ncrp, NREG_CTEST0, 0, (NB_CTEST0_EAN | NB_CTEST0_BTD));

	/* Enable Parity checking */
	ClrSetBits(ncrp, NREG_SCNTL0, 0, NB_SCNTL0_EPC);

	/* setup the minimum transfer period (i.e. max transfer rate) */
	/* for synchronous i/o for each of the targets */
	ncr_max_sync_rate_init(ncrp, TRUE);

	/* Disable auto switching, and set the core clock divisor */
	if ((clock = ncrp->n_sclock) <= 25) {
		ClrSetBits(ncrp, NREG_DCNTL, NB_DCNTL_CF
					, (NB_DCNTL_COM | NB_DCNTL_CF1));

	} else if (clock < 38) {
		ClrSetBits(ncrp, NREG_DCNTL, NB_DCNTL_CF
					, (NB_DCNTL_COM | NB_DCNTL_CF15));

	} else if (clock <= 50) {
		ClrSetBits(ncrp, NREG_DCNTL, NB_DCNTL_CF
					, (NB_DCNTL_COM | NB_DCNTL_CF2));

	} else  {
		ClrSetBits(ncrp, NREG_DCNTL, NB_DCNTL_CF
					, (NB_DCNTL_COM | NB_DCNTL_CF3));
	}

	NDBG1(("ncr53c710_init: instance=%d completed\n",
		ddi_get_instance(ncrp->n_dip)));

	return;
}


void
ncr53c710_enable( ncr_t *ncrp )
{
	/* enable all except Function Complete and Selected interrupts */
	ClrSetBits(ncrp, NREG_SIEN, 0, ~(NB_SIEN_FCMP | NB_SIEN_SEL));

	/* enable all valid except SCRIPT Step Interrupt */
	ClrSetBits(ncrp, NREG_DIEN, 0, ~(NB_DIEN_RES7 | NB_DIEN_RES6
							 | NB_DIEN_SSI));
}

void
ncr53c710_disable( ncr_t *ncrp )
{
	/* disable all SCSI interrrupts */
	ClrSetBits(ncrp, NREG_SIEN, 0xff, 0);

	/* disable all DMA interrupts */
	ClrSetBits(ncrp, NREG_DIEN, 0xff, 0);
}

unchar
ncr53c710_get_istat( ncr_t *ncrp )
{
	return (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT));
}

void
ncr53c710_halt( ncr_t *ncrp )
{
	bool_t	first_time = TRUE;
	int	loopcnt;
	unchar	istat;
	unchar	dstat;

	/* turn on the abort bit */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT, NB_ISTAT_ABRT);

	/* wait for and clear all pending interrupts */ 
	for (;;) {

		/* wait up to 1 sec. for a DMA or SCSI interrupt */
		for (loopcnt = 0; loopcnt < 1000; loopcnt++) {
			istat = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT);
			if (istat & (NB_ISTAT_SIP | NB_ISTAT_DIP))
				goto got_it;

			/* wait 1 millisecond */
			drv_usecwait(1000);
		}
		NDBG10(("ncr53c710_halt: instance=%d: can't halt\n",
			ddi_get_instance(ncrp->n_dip)));
		return;

	got_it:
		/* if there's a SCSI interrupt pending clear it and loop */
		if (istat & NB_ISTAT_SIP) {
			/* reset the sip latch registers */
			(void)ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT0);
			continue;
		}

		if (first_time) {
			/* reset the abort bit before reading DSTAT */
			ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT, 0);
			first_time = FALSE;
		}
		/* read the DMA status register */
		dstat = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DSTAT);
		if (dstat & NB_DSTAT_ABRT) {
			/* got the abort interrupt */
			NDBG10(("ncr53c710_halt: instance=%d: okay\n",
				ddi_get_instance(ncrp->n_dip)));
			return;
		}
		/* must have been some other pending interrupt */
		drv_usecwait(1000);
	}
}

void
ncr53c710_set_sigp( ncr_t *ncrp )
{
	ClrSetBits(ncrp, NREG_ISTAT, 0, NB_ISTAT_SIGP);
}

void
ncr53c710_reset_sigp( ncr_t *ncrp )
{
	(void)ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_CTEST2);
	return;
}

ulong
ncr53c710_get_intcode( ncr_t *ncrp )
{
	return(ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_DSPS));
}

/* 
 * Utility routine; check for error in execution of command in ccb,
 * handle it.
 */

void
ncr53c710_check_error (	npt_t		*nptp,
			struct scsi_pkt	*pktp )
{

	/* Get status from target relating to pktp, store in status */
	*pktp->pkt_scbp  = nptp->nt_statbuf[0];
	NDBG17(("ncr53c710_chkerr: scb=0x%x\n", nptp->nt_statbuf[0]));

	/* store the default error results in packet */
	pktp->pkt_state |= STATE_GOT_BUS;

	if (nptp->nt_scsi_status0 == 0 && nptp->nt_dma_status == 0) {
		NDBG17(("ncr53c710_chkerr: A\n"));
		pktp->pkt_statistics |= STAT_BUS_RESET;
		pktp->pkt_reason = CMD_RESET;
		return;
	}

	if (nptp->nt_scsi_status0 & NB_SSTAT0_STO) {
		NDBG17(("ncr53c710_check_error: B\n"));
		pktp->pkt_statistics |= STAT_TIMEOUT;

	}
	if (nptp->nt_scsi_status0 & NB_SSTAT0_SGE) {
		NDBG17(("ncr53c710_check_error: C\n"));
		pktp->pkt_state  |= STATE_GOT_BUS | STATE_GOT_TARGET;
		pktp->pkt_statistics |= STAT_BUS_RESET;

	}
	if (nptp->nt_scsi_status0 & NB_SSTAT0_UDC) {
		NDBG17(("ncr53c710_check_error: D\n"));
		pktp->pkt_state  |= STATE_GOT_BUS | STATE_GOT_TARGET;

	}
	if (nptp->nt_scsi_status0 & NB_SSTAT0_RST) {
		NDBG17(("ncr53c710_check_error: E\n"));
		pktp->pkt_state  |= STATE_GOT_BUS;
		pktp->pkt_statistics |= STAT_BUS_RESET;

	}
	if (nptp->nt_scsi_status0 & NB_SSTAT0_PAR) {
		NDBG17(("ncr53c710_check_error: F\n"));
		pktp->pkt_statistics |= STAT_PERR;

	}
	if (nptp->nt_dma_status & NB_DSTAT_ABRT) {
		NDBG17(("ncr53c710_check_error: G\n"));
		pktp->pkt_statistics |= STAT_ABORTED;
	}


	/* Determine the appropriate error reason */
	if (nptp->nt_scsi_status0 & NB_SSTAT0_STO) {
		NDBG17(("ncr53c710_check_error: H\n"));
		pktp->pkt_reason = CMD_TIMEOUT;

	} else if (nptp->nt_scsi_status0 & NB_SSTAT0_SGE) {
		NDBG17(("ncr53c710_check_error: I\n"));
		pktp->pkt_reason = CMD_DATA_OVR;

	} else if (nptp->nt_scsi_status0 & NB_SSTAT0_UDC) {
		NDBG17(("ncr53c710_check_error: J\n"));
		pktp->pkt_reason = CMD_INCOMPLETE;

	} else if (nptp->nt_scsi_status0 & NB_SSTAT0_RST) {
		NDBG17(("ncr53c710_check_error: K\n"));
		pktp->pkt_reason = CMD_RESET;

	} else {
		NDBG17(("ncr53c710_check_error: L\n"));
		pktp->pkt_reason = CMD_INCOMPLETE;
	}

	return;
}

/* for SCSI or DMA errors I need to figure out reasonable error
 * recoveries for all combinations of (hba state, scsi bus state,
 * error type). The possible error recovery actions are (in the
 * order of least to most drastic):
 * 
 * 	1. send message parity error to target
 *	2. send abort
 *	3. send abort tag
 *	4. send initiator detected error
 *	5. send bus device reset
 *	6. bus reset
 */

ulong
ncr53c710_dma_status( ncr_t *ncrp )
{
	npt_t	*nptp;
	ulong	 action = 0;
	unchar	 dstat;

	/* read DMA interrupt status register, and clear the register */
	dstat = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DSTAT);

	NDBG21(("ncr53c710_dma_status: instance=%d dstat=0x%x\n", 
		ddi_get_instance(ncrp->n_dip), dstat));

	/*
	 * DMA errors leave the HBA connected to the SCSI bus.
	 * Need to clear the bus and reset the chip.
	 */
	switch (ncrp->n_state) {
	case NSTATE_IDLE:
		/* this shouldn't happen */
		cmn_err(CE_WARN, "ncr53C710_dma_status: IDLE: error %b\n"
				, (ulong)dstat, dstatbits);
		action = NACTION_SIOP_REINIT;
		break;

	case NSTATE_ACTIVE:
		nptp = ncrp->n_current;
		nptp->nt_dma_status |= dstat;
		if (dstat & NB_DSTAT_ERRORS) {
			cmn_err(CE_WARN
				, "ncr53c710_dma_status: ACTIVE: error %b\n"
				, (ulong)dstat, dstatbits);
			action = NACTION_SIOP_REINIT | NACTION_DO_BUS_RESET
						     | NACTION_ERR;

		} else if (dstat & NB_DSTAT_SIR) {
			/* SCRIPT software interrupt */
			if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCH1) == 0) {
				/* the dma list has completed */
				nptp->nt_curdp.nd_left = 0;
				nptp->nt_savedp.nd_left = 0;
			}
			action |= NACTION_CHK_INTCODE;
		}
		break;

	case NSTATE_WAIT_RESEL:
		if (dstat & NB_DSTAT_ERRORS) {
			cmn_err(CE_WARN
				, "ncr53c710_dma_status: WAIT: error %b\n"
				, (ulong)dstat, dstatbits);
			action = NACTION_SIOP_REINIT;

		} else if (dstat & NB_DSTAT_SIR) {
			/* SCRIPT software interrupt */
			action |= NACTION_CHK_INTCODE;
		}
		break;
	}
	return (action);
}

ulong
ncr53c710_scsi_status( ncr_t *ncrp )
{
	npt_t	*nptp;
	ulong	 action = 0;
	unchar	 sstat0;

	/* read SCSI interrupt status register, and clear the register */
	sstat0 = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT0);

	NDBG21(("ncr53c710_scsi_status: instance=%d sstat0=0x%x\n",
		ddi_get_instance(ncrp->n_dip), sstat0));
/***
/***	cmn_err(CE_WARN, "ncr53c710_scsi_status: error %b\n", (ulong)sstat0
/***			, sbits);
/***/

	/* the scsi timeout, unexpected disconnect, and bus reset 
	 * interrupts leave the bus in the bus free state ???
	 * 
	 * the scsi gross error and parity error interrupts leave
	 * the bus still connected ???
	 */
	switch (ncrp->n_state) {
	case NSTATE_IDLE:
		if (sstat0 & (NB_SSTAT0_STO | NB_SSTAT0_SGE
					| NB_SSTAT0_PAR | NB_SSTAT0_UDC)) {
			/* shouldn't happen, do chip and bus reset */
			action = NACTION_SIOP_REINIT;
		}
		if (sstat0 & NB_SSTAT0_RST) {
			/* set all targets on this bus to renegotiate syncio */
			ncr_syncio_reset(ncrp, NULL);
			action = 0;
		}
		break;

	case NSTATE_ACTIVE:
		nptp = ncrp->n_current;
		nptp->nt_scsi_status0 |= sstat0;

		/* If phase mismatch then figure out the residual for 
		 * the current Scatter/Gather DMA segment.
		 */
		if (sstat0 & NB_SSTAT0_MA) {
			/* check if the dma list has completed */
			if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCH1) == 0) {
				nptp->nt_curdp.nd_left = 0;
				nptp->nt_savedp.nd_left = 0;
			} 
			action = NACTION_SAVE_BCNT;
		}

		if (sstat0 & (NB_SSTAT0_PAR | NB_SSTAT0_SGE)) {
			/* attempt recovery if selection done and connected */
			if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SCNTL1) & NB_SCNTL1_CON) {
				unchar phase = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT2)
						& NB_SSTAT2_PHASE;
				action = ncr_parity_check(phase);
			} else {
				action = NACTION_ERR;
			}
		}


		if (sstat0 & (NB_SSTAT0_STO | NB_SSTAT0_UDC)) {
			/* bus is now idle */
			action = NACTION_SAVE_BCNT | NACTION_ERR;
		}

		if (sstat0 & NB_SSTAT0_RST) {
			/* set all targets on this bus to renegotiate syncio */
			ncr_syncio_reset(ncrp, NULL);

			/* bus is now idle */
			action = NACTION_GOT_BUS_RESET | NACTION_SAVE_BCNT
						       | NACTION_ERR;
		}
		break;

	case NSTATE_WAIT_RESEL:
		if (sstat0 & NB_SSTAT0_PAR) {
			/* attempt recovery if reconnected */
			if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SCNTL1) & NB_SCNTL1_CON) {
				action = NACTION_MSG_PARITY;
			} else {
				/* don't respond */
				action = 0;
			}
		}

		if (sstat0 & NB_SSTAT0_UDC) {
			/* target reselected then disconnected, ignore it */
			action = NACTION_BUS_FREE;
		}

		if (sstat0 & (NB_SSTAT0_STO | NB_SSTAT0_SGE)) {
			/* shouldn't happen, do chip and bus reset */
			action = NACTION_SIOP_REINIT | NACTION_DO_BUS_RESET;
		}

		if (sstat0 & NB_SSTAT0_RST) {
			/* set all targets on this bus to renegotiate syncio */
			ncr_syncio_reset(ncrp, NULL);

			/* got bus reset, start a new request */
			action = NACTION_GOT_BUS_RESET | NACTION_BUS_FREE;
		}
		break;
	}
	return (action);
}

/*
 * If the phase-mismatch which preceeds the Save Data Pointers
 * occurs within in a Scatter/Gather segment there's a residual
 * byte count that needs to be computed and remembered. It's 
 * possible for a Disconnect message to occur without a preceeding
 * Save Data Pointers message, so at this point we only compute
 * the residual without fixing up the S/G pointers.
 */
bool_t
ncr53c710_save_byte_count(	ncr_t	*ncrp,
				npt_t	*nptp )
{
	paddr_t	dsp;
	int	index;
	ulong	remain;
	unchar	opcode;
	unchar	tmp;
	unchar	sstat1;
	bool_t	rc;

	NDBG17(("ncr53c710_save_byte_count: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	/* Only need to do this for phase mismatch interrupts
	 * during actual move-data-in or move-data-out instructions.
	 */
	if (nptp->nt_curdp.nd_num == 0) {
		/* since no data requested must not be S/G dma */
		rc = FALSE;
		goto out;
	}

	/* fetch the instruction pointer and back it up
	 * to the actual interrupted instruction.
	 */
	dsp = ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_DSP) - 8;

	/* check for MOVE DATA_OUT or MOVE DATA_IN instruction */
	opcode = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DCMD);
	if (opcode == (NSOP_MOVE | NSOP_DATAOUT)) {
		/* move data out */
		index = dsp - ncr_do_list;

	} else if (opcode == (NSOP_MOVE | NSOP_DATAIN)) {
		/* move data in */
		index = dsp - ncr_di_list;

	} else {
		/* not doing data dma so nothing to update */
		NDBG17(("ncr53c710_save_byte_count: instance=%d"
			" 0x%x not move\n", ddi_get_instance(ncrp->n_dip),
			opcode));
		rc = FALSE;
		goto out;
	}

	/* convert byte index into S/G DMA list index */
	index /= 8;

	if (index < 0 || index >= NCR_MAX_DMA_SEGS) {
		/* it's out of dma list range, must have been some other move */
		NDBG17(("ncr53c710_save_byte_count: instance=%d"
			" 0x%x not dma\n", ddi_get_instance(ncrp->n_dip),
			index));
		rc = FALSE;
		goto out;
	}

	/* get the residual from the byte count register */
	remain = ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_DBC);
	remain &= 0xffffff;

	/* number of bytes stuck in the DMA FIFO */
	tmp = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DFIFO) & 0x7f;
	tmp -= (remain & 0x7f);

	/* actual number left untransferred */
	remain += (tmp & 0x7f);

	/* add one if there's a byte stuck in the SCSI fifo */
	if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_CTEST0) & NB_CTEST0_DDIR) {
		/* transfer was receive */
		if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT1) & NB_SSTAT1_ILF)
			remain++;	/* Add 1 if SIDL reg is full */

		/* check for synchronous i/o */
		if (nptp->nt_selectparm.nt_sxfer != 0)
			remain += (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT2) >> 4) & 0xf;
	} else {
		/* transfer was send */
		sstat1 = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT1);
		if (sstat1 & NB_SSTAT1_OLF)
			remain++;	/* Add 1 if data is in SODL reg. */

		/* check for synchronous i/o */
		if ((nptp->nt_selectparm.nt_sxfer != 0)
		&&  (sstat1 & NB_SSTAT1_ORF))
			remain++;	/* Add 1 if data is in SODR */
	}

	/* update the S/G pointers and indexes */
	ncr_sg_update(ncrp, nptp, (unchar)index, remain);
	rc = TRUE;


    out:
	/* Clear the DMA and SCSI FIFO pointers */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_CTEST8, ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_CTEST8) | NB_CTEST8_CLF);

	NDBG17(("ncr53c710_save_byte_count: instance=%d index=%d remain=%d\n",
		ddi_get_instance(ncrp->n_dip),
		index, remain));
	return (rc);
}

bool_t
ncr53c710_get_target(	ncr_t	*ncrp,
			unchar	*tp )
{
	unchar	target;
	unchar	id;
	unchar	saveid;

	/* get the id byte received from the reselecting target */
	saveid = id = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_LCRC);

	NDBG20(("ncr53c710_get_target: instance=%d lcrc=0x%x\n",
		ddi_get_instance(ncrp->n_dip), id));

	/* mask off this HBA's id bit */
	id &= ~ncrp->n_idmask;

	/* convert the remaining bit into a target number */
	for (target = 0; target < NTARGETS; target++) {
		if (id & 1) {
			NDBG1(("ncr53c710_get_target: ID %d reselected\n", id));
			*tp = target;
			return (TRUE);
		}
		id = id >> 1;
	}
	cmn_err(CE_WARN, "ncr53c710_get_target: invalid reselect id %d\n"
		       , saveid);
	return (FALSE);
}

unchar
ncr53c710_encode_id( unchar id )
{
	return (1<<id);
}

void
ncr53c710_set_syncio(	ncr_t	*ncrp,
			npt_t	*nptp )
{
	unchar	sxfer = nptp->nt_selectparm.nt_sxfer;

	/* Set sync i/o clock divisors in SXFER and SBCL registers */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SXFER, sxfer);

	if (sxfer != 0) {
		/* Set sync i/o clock divisor in SBCL registers */
		switch (nptp->nt_sscfX10) {
		case 10:
			ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SBCL, NB_SBCL_SSCF1);
			break;

		case 15:
			ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SBCL, NB_SBCL_SSCF15);
			break;

		case 20:
			ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SBCL, NB_SBCL_SSCF2);
			break;
		}
	}

	/* Only set extended filtering when not Fast-SCSI (< 5BM/sec) */
	if (sxfer == 0 || nptp->nt_fastscsi == FALSE)
		ClrSetBits(ncrp, NREG_CTEST0, 0, NB_CTEST0_ERF);
	else
		ClrSetBits(ncrp, NREG_CTEST0, NB_CTEST0_ERF, 0);

	return;
}


void
ncr53c710_setup_script(	ncr_t	*ncrp,
			npt_t	*nptp )
{
	unchar nleft;

	NDBG18(("ncr53c710_setup_script: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	/* Set the Data Structure address register to */
	/* the physical address of the active table */
	ddi_io_putl(ncrp->n_handle, ncrp->n_reg + NREG_DSA, nptp->nt_dsa_physaddr);

	/* Set syncio clock divisors and offset registers in case */
	/* reconnecting after target reselected */
	ncr53c710_set_syncio(ncrp, nptp);

	/* Setup scratch1 as the number of segments left to do */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCH1, nleft = nptp->nt_curdp.nd_left);

/***/
/***if (ncrp->n_state == NSTATE_ACTIVE && nleft > NCR_MAX_DMA_SEGS)
/***	debug_enter("\n\nBAD NLEFT\n\n");
/***/

	NDBG18(("ncr53c710_setup_script: instance=%d okay\n",
		ddi_get_instance(ncrp->n_dip)));
	return;
}

void
ncr53c710_start_script(	ncr_t	*ncrp,
			int	 script )
{
	/* Send the SCRIPT entry point address to the ncr chip */
	ddi_io_putl(ncrp->n_handle, ncrp->n_reg + NREG_DSP, ncr_scripts[script]);

	NDBG18(("ncr53c710_start_script: instance=%d script=%d\n",
		ddi_get_instance(ncrp->n_dip), script));
	return;
}

void
ncr53c710_bus_reset( ncr_t *ncrp )
{
	NDBG1(("ncr53c710_bus_reset: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	/* Reset the scsi bus */
	ClrSetBits(ncrp, NREG_SCNTL1, 0, NB_SCNTL1_RST);

	/* Wait at least 25 microsecond */
	drv_usecwait(25);

	/* Turn off the bit to complete the reset */
	ClrSetBits(ncrp, NREG_SCNTL1, NB_SCNTL1_RST, 0);  

	/* Allow 250 milisecond bus reset recovery time */
	drv_usecwait(250000);

	/* clear any pending SCSI interrupts */
	(void)ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT0);

	/* need short delay before reading DSTAT */
	(void)ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCH);
	(void)ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCH);

	/* clear any pending DMA interrupts */
	(void)ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DSTAT);
	return;
}

int
ncr53c710_geometry(	ncr_t			*ncrp,
			struct	scsi_address	*ap )
{
	ulong	totalsectors;
	ulong	heads;
	ulong	sectors;

	totalsectors = ADDR2NCRUNITP(ap)->nt_total_sectors;

	/* use Compaq's geometry on the 53c710 */
	if (totalsectors <= 1024 * 64 * 32) {
		heads = 64;
		sectors = 32;
	} else { /* more than 1024 cylinders */
		heads = 255;
		sectors = 63;
	}

cmn_err(CE_CONT, "?ncr(%d,%d): instance=%d sectors=%d heads=%d sectors=%d\n",
		ap->a_target, ap->a_lun,
		ddi_get_instance(ncrp->n_dip),
		totalsectors, heads, sectors);
	return (HBA_SETGEOM(heads, sectors));
}


nops_t	ncr53c710_nops = {
	"53c710",
	ncr_script_init,
	ncr_script_fini,
	ncr53c710_probe,
	ncr_get_irq_eisa,
	ncr_xlate_irq_no_sid,
#ifdef	PCI_DDI_EMULATION
	ncr_get_ioaddr_eisa,
#else
	NCR_EISA_RNUMBER,
#endif
	ncr53c710_reset,
	ncr53c710_init,
	ncr53c710_enable,
	ncr53c710_disable,
	ncr53c710_get_istat,
	ncr53c710_halt,
	ncr53c710_set_sigp,
	ncr53c710_reset_sigp,
	ncr53c710_get_intcode,
	ncr53c710_check_error,
	ncr53c710_dma_status,
	ncr53c710_scsi_status,
	ncr53c710_save_byte_count,
	ncr53c710_get_target,
	ncr53c710_encode_id,
	ncr53c710_setup_script,
	ncr53c710_start_script,
	ncr53c710_set_syncio,
	ncr53c710_bus_reset,
	ncr53c710_geometry
};


/*
 * The Siemens Nixdorf, Inc. PCE-5S system requires special ncr_probe
 * and ncr_get_irq functions for its two embedded 710 controllers.
 */
extern	bool_t	ncr53c710_probe_SNI(dev_info_t *, int *, int, int *, int, bus_t, bool_t);
bool_t	ncr_get_irq_eisa_SNI(ncr_t *, int *, int);
int	ncr53c810_geometry(ncr_t *ncrp, struct scsi_address *ap);

nops_t	ncr53c710_nops_SNI = {
	"53c710",
	ncr_script_init,
	ncr_script_fini,
	ncr53c710_probe_SNI,
	ncr_get_irq_eisa_SNI,
	ncr_xlate_irq_no_sid,
#ifdef	PCI_DDI_EMULATION
	ncr_get_ioaddr_eisa,
#else
	NCR_EISA_RNUMBER,
#endif
	ncr53c710_reset,
	ncr53c710_init,
	ncr53c710_enable,
	ncr53c710_disable,
	ncr53c710_get_istat,
	ncr53c710_halt,
	ncr53c710_set_sigp,
	ncr53c710_reset_sigp,
	ncr53c710_get_intcode,
	ncr53c710_check_error,
	ncr53c710_dma_status,
	ncr53c710_scsi_status,
	ncr53c710_save_byte_count,
	ncr53c710_get_target,
	ncr53c710_encode_id,
	ncr53c710_setup_script,
	ncr53c710_start_script,
	ncr53c710_set_syncio,
	ncr53c710_bus_reset,

	/* the Siemens Nixdorf PCE-5S BIOS uses the same CAM algorithm
	 * as the NCR 53c810 SDMS BIOS
	 */
	ncr53c810_geometry
};
