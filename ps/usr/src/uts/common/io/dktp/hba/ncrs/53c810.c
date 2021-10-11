/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)53c810.c	1.16	96/02/12 SMI"

#include <sys/dktp/ncrs/ncr.h>
#include <sys/dktp/ncrs/53c8xx.h>
#include <sys/dktp/ncrs/script.h>
#undef static
#include <sys/pci.h>

/*
 * Save the following bits in the specified registers
 * during a chip reset. The bits are established by the
 * HBA's POST BIOS and are very hardware dependent.
 */
static	nrs_t	ncr53c810_reg_save[] = {
	{ NREG_SCNTL3,	(NB_SCNTL3_SCF | NB_SCNTL3_CCF) },
	{ NREG_GPREG,	0xff },
	{ NREG_CTEST4,	NB_CTEST4_BDIS },
	{ NREG_DMODE,	NB_DMODE_BL },
	{ NREG_DCNTL,	NB_DCNTL_IRQM },
	{ NREG_STEST1,	NB_STEST1_SCLK },
};

#define	NRegSave	(sizeof ncr53c810_reg_save / sizeof (nrs_t))

static	char	*dstatbits = "\
\020\
\010DMA-FIFO-empty\
\07master-data-parity-error\
\06bus-fault\
\05aborted\
\04single-step-interrupt\
\03SCRIPTS-interrupt-instruction\
\02reserved\
\01illegal-instruction";


static bool_t
ncr53c810_probe(
			dev_info_t	*dip,
			int		*regp,
			int		 len,
			int		*pidp,
			int		 pidlen,
			bus_t		 bus_type,
			bool_t		 probing)
{
	unsigned short	vendorid;
	unsigned short	deviceid;
	unsigned short	command;
	unsigned char	iline;
	ddi_acc_handle_t	handle;

	if (bus_type != BUS_TYPE_PCI)
		return (FALSE);

#ifdef	PCI_DDI_EMULATION
	/*
	 * XXX 2.4 limitations on the reg property as implemented
	 * for ncrs resulted in incompatible code.
	 * This information is modified here to allow pci config operations
	 * to succeed.
	 */
	if (probing) {
		/* properly shift reg property */
		*regp = *regp << 3;
		if (e_ddi_prop_modify(DDI_DEV_T_NONE, dip, 0, "reg",
		    (caddr_t)regp, len) != DDI_PROP_SUCCESS)
			return (FALSE);
	}
#endif

	if (pci_config_setup(dip, &handle) != DDI_SUCCESS)
		return (FALSE);

	vendorid = pci_config_getw(handle, PCI_CONF_VENID);
	deviceid = pci_config_getw(handle, PCI_CONF_DEVID);
	command = pci_config_getw(handle, PCI_CONF_COMM);
	iline = pci_config_getb(handle, PCI_CONF_ILINE);

	pci_config_teardown(&handle);

	switch (vendorid) {
	case 0x1000:
		/* check for 53c810, 53c820, 53c825 and 53c815 controllers */
		switch (deviceid) {
		case 0x0001:
		case 0x0002:
		case 0x0003:
		case 0x0004:
			goto found_it;
		}
	}

	NDBG4(("ncr53c810_probe: not found\n"));
	return (FALSE);


found_it:
	/*
	 * Some BIOSes have features allowing the user to disable the NCR
	 * chip.  Unfortunately, there's no standard PCI way to indicate
	 * an "administratively disabled" function, and so the framework
	 * can't know that it shouldn't call us.  (The I/O and memory
	 * disable bits come close, but note that it is possible to
	 * have a device with registers only in config space, with no
	 * I/O or memory-based registers.)
	 *
	 * Observed characteristics of a disabled chip:
	 *   Garbage I/O base register (e.g. 0xfc510001)
	 *	(PCI-wise, this is legal.  x86-wise, it's not.)
	 *   Zero interrupt line register
	 *	(This is strictly legal, but we know it conflicts
	 *	with the clock.)
	 *   I/O and memory access disabled
	 *
	 * We know that we're supposed to have I/O registers, so
	 * we'll try checking for enabled I/O.
	 *
	 * No, I'm not happy with this answer.  Hopefully we can
	 * figure out some generic rules so the framework can implement them.
	 */
	if ((command & PCI_COMM_IO) == 0 ||
	    (iline < 1 || iline > 15)) {
		cmn_err(CE_CONT,
		    "?ncr53c810_probe: disabled\n");
		return (FALSE);
	}

	if (probing) {
		cmn_err(CE_CONT,
		    "?ncr53c810_probe() vendor=0x%x device=0x%x\n",
		    vendorid, deviceid);
	}

	NDBG4(("ncr53c810_probe: okay\n"));

	return (TRUE);
}

void
ncr53c810_reset(ncr_t *ncrp)
{
	unchar	regbuf[NRegSave];

	NDBG10(("ncr53c810_reset: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	ncr_save_regs(ncrp, ncr53c810_reg_save, regbuf, sizeof (regbuf));

	/* Reset the 53c810 chip */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT, NB_ISTAT_SRST);

	/* wait a tick and then turn the reset bit off */
	drv_usecwait(100);
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT, 0);

	/* clear any pending SCSI interrupts */
	(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SIST0);

	/* need short delay before reading SIST1 */
	(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);
	(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);

	(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SIST1);

	/* need short delay before reading DSTAT */
	(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);
	(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);

	/* clear any pending DMA interrupts */
	(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DSTAT);

	ncr_restore_regs(ncrp, ncr53c810_reg_save, regbuf, sizeof (regbuf));

	NDBG1(("NCR53c810: Software reset completed\n"));
}

void
ncr53c810_init(ncr_t *ncrp)
{
	int	clock;

	NDBG1(("ncr53c810_init: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	/* set script to 53c8xx mode */
	ClrSetBits(ncrp, NREG_SCRATCHA0, NBIT_IS710, 0);

	/* Enable Parity checking and generation */
	ClrSetBits(ncrp, NREG_SCNTL0, 0, NB_SCNTL0_EPC);

	/* disable extra clock cycle of data setup so that */
	/* the hba can do 10MB/sec fast scsi */
	ClrSetBits(ncrp, NREG_SCNTL1, NB_SCNTL1_EXC, 0);

	/* Set the HBA's SCSI id, and enable reselects */
	ClrSetBits(ncrp, NREG_SCID, NB_SCID_ENC,
			((ncrp->n_initiatorid & NB_SCID_ENC)
					| NB_SCID_RRE));

	/* Disable auto switching */
	ClrSetBits(ncrp, NREG_DCNTL, 0, NB_DCNTL_COM);

#if defined(__ppc)
	/* Enable totem pole interrupt mode */
	/* PowerPC reference platform explicitly requires this init. */
	ClrSetBits(ncrp, NREG_DCNTL, 0, NB_DCNTL_IRQM);
#endif

	/* set the selection time-out value to next value above 250 msec. */
	ClrSetBits(ncrp, NREG_STIME0, NB_STIME0_SEL, NB_STIME0_409);

	/* Set the scsi id bit to match the HBA's idmask */
	ddi_io_putw(ncrp->n_handle, ncrp->n_reg + NREG_RESPID, ncrp->n_idmask);

	/* disable SCSI-1 single initiator option */
	/* enable TolerANT (active negation) */
	ClrSetBits(ncrp, NREG_STEST3, 0, (NB_STEST3_TE | NB_STEST3_DSI));

	/* setup the minimum transfer period (i.e. max transfer rate) */
	/* for synchronous i/o for each of the targets */
	ncr_max_sync_rate_init(ncrp, FALSE);

	/* set the scsi core divisor */
	if ((clock = ncrp->n_sclock) <= 25) {
		ncrp->n_scntl3 = NB_SCNTL3_CCF1;

	} else if (clock < 38) {
		ncrp->n_scntl3 = NB_SCNTL3_CCF15;

	} else if (clock <= 50) {
		ncrp->n_scntl3 = NB_SCNTL3_CCF2;

	} else  {
		ncrp->n_scntl3 = NB_SCNTL3_CCF3;
	}
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SCNTL3, ncrp->n_scntl3);


	NDBG1(("ncr53c810_init: instance=%d: completed\n",
		ddi_get_instance(ncrp->n_dip)));
}

void
ncr53c810_enable(ncr_t *ncrp)
{
	/* enable all fatal interrupts, disable all non-fatal interrupts */
	ClrSetBits(ncrp, NREG_SIEN0,
			(NB_SIEN0_CMP | NB_SIEN0_SEL | NB_SIEN0_RSL),
			(NB_SIEN0_MA | NB_SIEN0_SGE | NB_SIEN0_UDC
			|  NB_SIEN0_RST | NB_SIEN0_PAR));

	/* enable all fatal interrupts, disable all non-fatal interrupts */
	ClrSetBits(ncrp, NREG_SIEN1,
			(NB_SIEN1_GEN | NB_SIEN1_HTH),
			NB_SIEN1_STO);


	/* enable all valid except SCRIPT Step Interrupt */
	ClrSetBits(ncrp, NREG_DIEN, NB_DIEN_SSI,
			(NB_DIEN_MDPE | NB_DIEN_BF | NB_DIEN_ABRT
			|  NB_DIEN_SIR | NB_DIEN_IID));

	/* enable master parity error detection logic */
	ClrSetBits(ncrp, NREG_CTEST4, 0, NB_CTEST4_MPEE);
}

void
ncr53c810_disable(ncr_t *ncrp)
{
	/* disable all SCSI interrrupts */
	ClrSetBits(ncrp, NREG_SIEN0,
			(NB_SIEN0_MA | NB_SIEN0_CMP | NB_SIEN0_SEL
			|  NB_SIEN0_RSL | NB_SIEN0_SGE | NB_SIEN0_UDC
			|  NB_SIEN0_RST | NB_SIEN0_PAR), 0);

	ClrSetBits(ncrp, NREG_SIEN1,
			(NB_SIEN1_GEN | NB_SIEN1_HTH | NB_SIEN1_STO), 0);

	/* disable all DMA interrupts */
	ClrSetBits(ncrp, NREG_DIEN,
			(NB_DIEN_MDPE | NB_DIEN_BF | NB_DIEN_ABRT
			|  NB_DIEN_SSI | NB_DIEN_SIR | NB_DIEN_IID), 0);

	/* disable master parity error detection */
	ClrSetBits(ncrp, NREG_CTEST4, NB_CTEST4_MPEE, 0);
}

unchar
ncr53c810_get_istat(ncr_t *ncrp)
{
	return (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_ISTAT));
}

void
ncr53c810_halt(ncr_t *ncrp)
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
		NDBG10(("ncr53c810_halt: instance=%d: can't halt\n",
			ddi_get_instance(ncrp->n_dip)));
		return;

	got_it:
		/* if there's a SCSI interrupt pending clear it and loop */
		if (istat & NB_ISTAT_SIP) {
			/* reset the sip latch registers */
			(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SIST0);

			/* need short delay before reading SIST1 */
			(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);
			(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);

			(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SIST1);
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
			NDBG10(("ncr53c810_halt: instance=%d: okay\n",
				ddi_get_instance(ncrp->n_dip)));
			return;
		}
		/* must have been some other pending interrupt */
		drv_usecwait(1000);
	}
}


void
ncr53c810_set_sigp(ncr_t *ncrp)
{
	ClrSetBits(ncrp, NREG_ISTAT, 0, NB_ISTAT_SIGP);
}

void
ncr53c810_reset_sigp(ncr_t *ncrp)
{
	(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_CTEST2);
}

ulong
ncr53c810_get_intcode(ncr_t *ncrp)
{
	return (ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_DSPS));
}

/*
 * Utility routine; check for error in execution of command in ccb,
 * handle it.
 */

void
ncr53c810_check_error(
			npt_t		*nptp,
			struct scsi_pkt	*pktp)
{

	/* Get status from target relating to pktp, store in status */
	*pktp->pkt_scbp  = nptp->nt_statbuf[0];
	NDBG17(("ncr53c810_check_error: scb=0x%x\n", nptp->nt_statbuf[0]));

	/* store the default error results in packet */
	pktp->pkt_state |= STATE_GOT_BUS;

	if (nptp->nt_scsi_status0 == 0 &&
	    nptp->nt_scsi_status1 == 0 &&
	    nptp->nt_dma_status == 0) {
		NDBG17(("ncr53c810_check_error: A\n"));
		pktp->pkt_statistics |= STAT_BUS_RESET;
		pktp->pkt_reason = CMD_RESET;
		return;
	}

	if (nptp->nt_scsi_status1 & NB_SIST1_STO) {
		NDBG17(("ncr53c810_check_error: B\n"));
		pktp->pkt_statistics |= STAT_TIMEOUT;
	}
	if (nptp->nt_scsi_status0 & NB_SIST0_UDC) {
		NDBG17(("ncr53c810_check_error: C\n"));
		pktp->pkt_state  |= STATE_GOT_BUS | STATE_GOT_TARGET;
	}
	if (nptp->nt_scsi_status0 & NB_SIST0_RST) {
		NDBG17(("ncr53c810_check_error: D\n"));
		pktp->pkt_state  |= STATE_GOT_BUS;
		pktp->pkt_statistics |= STAT_BUS_RESET;
	}
	if (nptp->nt_scsi_status0 & NB_SIST0_PAR) {
		NDBG17(("ncr53c810_check_error: E\n"));
		pktp->pkt_statistics |= STAT_PERR;
	}
	if (nptp->nt_dma_status & NB_DSTAT_ABRT) {
		NDBG17(("ncr53c810_check_error: F\n"));
		pktp->pkt_statistics |= STAT_ABORTED;
	}


	/* Determine the appropriate error reason */

	/* watch out, on the 8xx chip the STO bit was moved */
	if (nptp->nt_scsi_status1 & NB_SIST1_STO) {
		NDBG17(("ncr53c810_check_error: G\n"));
		pktp->pkt_reason = CMD_TIMEOUT;

	} else if (nptp->nt_scsi_status0 & NB_SIST0_UDC) {
		NDBG17(("ncr53c810_check_error: H\n"));
		pktp->pkt_reason = CMD_INCOMPLETE;

	} else if (nptp->nt_scsi_status0 & NB_SIST0_RST) {
		NDBG17(("ncr53c810_check_error: I\n"));
		pktp->pkt_reason = CMD_RESET;

	} else {
		NDBG17(("ncr53c810_check_error: J\n"));
		pktp->pkt_reason = CMD_INCOMPLETE;
	}
}

/*
 * for SCSI or DMA errors I need to figure out reasonable error
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
ncr53c810_dma_status(ncr_t *ncrp)
{
	npt_t	*nptp;
	ulong	 action = 0;
	unchar	 dstat;

	/* read DMA interrupt status register, and clear the register */
	dstat = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DSTAT);

	NDBG21(("ncr53c810_dma_status: instance=%d: dstat=0x%x\n",
		ddi_get_instance(ncrp->n_dip), dstat));

	/*
	 * DMA errors leave the HBA connected to the SCSI bus.
	 * Need to clear the bus and reset the chip.
	 */
	switch (ncrp->n_state) {
	case NSTATE_IDLE:
		/* this shouldn't happen */
		cmn_err(CE_WARN, "ncr53c810_dma_status: IDLE: error %b\n",
			(ulong)dstat, dstatbits);
		action = NACTION_SIOP_REINIT;
		break;

	case NSTATE_ACTIVE:
		nptp = ncrp->n_current;
		nptp->nt_dma_status |= dstat;
		if (dstat & NB_DSTAT_ERRORS) {
			cmn_err(CE_WARN,
				"ncr53c810_dma_status: ACTIVE: error %b\n",
				(ulong)dstat, dstatbits);
			action = NACTION_SIOP_REINIT | NACTION_DO_BUS_RESET
				| NACTION_ERR;

		} else if (dstat & NB_DSTAT_SIR) {
			/* SCRIPT software interrupt */
			if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA1) == 0) {
				/* the dma list has completed */
				nptp->nt_curdp.nd_left = 0;
				nptp->nt_savedp.nd_left = 0;
			}
			action |= NACTION_CHK_INTCODE;
		}
		break;

	case NSTATE_WAIT_RESEL:
		if (dstat & NB_DSTAT_ERRORS) {
			cmn_err(CE_WARN,
				"ncr53c810_dma_status: WAIT: error %b\n",
				(ulong)dstat, dstatbits);
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
ncr53c810_scsi_status(ncr_t *ncrp)
{
	npt_t	*nptp;
	ulong	 action = 0;
	unchar	 sist0;
	unchar	 sist1;

	/* read SCSI interrupt status register, and clear the register */
	sist0 = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SIST0);
	sist1 = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SIST1);

	NDBG21(("ncr53c810_scsi_status: instance=%d: sist0=0x%x sist1=0x%x\n",
		ddi_get_instance(ncrp->n_dip),
		sist0, sist1));

	/*
	 * the scsi timeout, unexpected disconnect, and bus reset
	 * interrupts leave the bus in the bus free state ???
	 *
	 * the scsi gross error and parity error interrupts leave
	 * the bus still connected ???
	 */
	switch (ncrp->n_state) {
	case NSTATE_IDLE:
		if ((sist0 & (NB_SIST0_SGE | NB_SIST0_PAR | NB_SIST0_UDC)) ||
		    (sist1 & NB_SIST1_STO)) {
			/* shouldn't happen, do chip and bus reset */
			action = NACTION_SIOP_REINIT;
		}

		if (sist0 & NB_SIST0_RST) {
#ifdef NCR_DEBUG
if (ncrp->n_disc_num != 0) debug_enter("invalid n_disc_num\n");
#endif
			/* set all targets on this bus to renegotiate syncio */
			ncr_syncio_reset(ncrp, NULL);
			action = 0;
			drv_usecwait(500000);
		}
		break;

	case NSTATE_ACTIVE:
		nptp = ncrp->n_current;
		nptp->nt_scsi_status0 |= sist0;
		nptp->nt_scsi_status1 |= sist1;

		/*
		 * If phase mismatch then figure out the residual for
		 * the current dma scatter/gather segment
		 */
		if (sist0 & NB_SIST0_MA) {
			if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA1) == 0) {
				/* the dma list has completed */
				nptp->nt_curdp.nd_left = 0;
				nptp->nt_savedp.nd_left = 0;
			}
			action = NACTION_SAVE_BCNT;
		}

		if (sist0 & (NB_SIST0_PAR | NB_SIST0_SGE)) {
			/* attempt recovery if selection done and connected */
			if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SCNTL1) & NB_SCNTL1_CON) {
				unchar phase = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT1)
						& NB_SSTAT1_PHASE;
				action = ncr_parity_check(phase);
			} else {
				action = NACTION_ERR;
			}
		}

		if ((sist0 & NB_SIST0_UDC) || (sist1 & NB_SIST1_STO)) {
			/* bus is now idle */
			action = NACTION_SAVE_BCNT | NACTION_ERR;
		}

		if (sist0 & NB_SIST0_RST) {
			/* set all targets on this bus to renegotiate syncio */
			ncr_syncio_reset(ncrp, NULL);

			/* bus is now idle */
			action = NACTION_GOT_BUS_RESET | NACTION_ERR;
			drv_usecwait(500000);
		}
		break;

	case NSTATE_WAIT_RESEL:
		if (sist0 & NB_SIST0_PAR) {
			/* attempt recovery if reconnected */
			if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SCNTL1) & NB_SCNTL1_CON) {
				action = NACTION_MSG_PARITY;
			} else {
				/* don't respond */
				action = NACTION_BUS_FREE;
			}
		}

		if (sist0 & NB_SIST0_UDC) {
			/* target reselected then disconnected, ignore it */
			action = NACTION_BUS_FREE;
		}

		if ((sist0 & NB_SIST0_SGE) || (sist1 & NB_SIST1_STO)) {
			/* shouldn't happen, do chip and bus reset */
			action = NACTION_SIOP_REINIT | NACTION_DO_BUS_RESET;
		}

		if (sist0 & NB_SIST0_RST) {
			/* set all targets on this bus to renegotiate syncio */
			ncr_syncio_reset(ncrp, NULL);

			/* got bus reset, start something new */
			action = NACTION_GOT_BUS_RESET | NACTION_BUS_FREE;
			drv_usecwait(500000);
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
ncr53c810_save_byte_count(
				ncr_t	*ncrp,
				npt_t	*nptp)
{
	paddr_t	dsp;
	int	index;
	ulong	remain;
	unchar	opcode;
	unchar	tmp;
	unchar	sstat0;
	bool_t	rc;

	NDBG17(("ncr53c810_save_byte_count: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	/*
	 * Only need to do this for phase mismatch interrupts
	 * during actual data in or data out move instructions.
	 */
	if (nptp->nt_curdp.nd_num == 0) {
		/* since no data requested must not be S/G dma */
		rc = FALSE;
		goto out;
	}

	/*
	 * fetch the instruction pointer and back it up
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
		NDBG17(("ncr53c810_save_byte_count: "
			"instance=%d: 0x%x not move\n",
			ddi_get_instance(ncrp->n_dip), opcode));
		rc = FALSE;
		goto out;
	}

	/* convert byte index into S/G DMA list index */
	index /= 8;

	if (index < 0 || index >= NCR_MAX_DMA_SEGS) {
		/* it's out of dma list range, must have been some other move */
		NDBG17(("ncr53c810_save_byte_count: "
			"instance=%d: 0x%x not dma\n",
			ddi_get_instance(ncrp->n_dip), index));
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
	if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_CTEST2) & NB_CTEST2_DDIR) {
		/* transfer was receive */
		if (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT0) & NB_SSTAT0_ILF)
			remain++;	/* Add 1 if SIDL reg is full */

		/* check for synchronous i/o */
		if (nptp->nt_selectparm.nt_sxfer != 0)
			remain += (ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT1) >> 4) & 0xf;
	} else {
		/* transfer was send */
		sstat0 = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSTAT0);
		if (sstat0 & NB_SSTAT0_OLF)
			remain++;	/* Add 1 if data is in SODL reg. */

		/* check for synchronous i/o */
		if ((nptp->nt_selectparm.nt_sxfer != 0) &&
		    (sstat0 & NB_SSTAT0_ORF))
			remain++;	/* Add 1 if data is in SODR */
	}

	/* update the S/G pointers and indexes */
	ncr_sg_update(ncrp, nptp, index, remain);
	rc = TRUE;


out:
	/* Clear the DMA FIFO pointers */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_CTEST3, ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_CTEST3) | NB_CTEST3_CLF);

	/* Clear the SCSI FIFO pointers */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_STEST3, ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_STEST3) | NB_STEST3_CSF);

	NDBG17(("ncr53c810_save_byte_count: instance=%d: index=%d remain=%d\n",
		ddi_get_instance(ncrp->n_dip),
		index, remain));
	return (rc);
}

bool_t
ncr53c810_get_target(
			ncr_t	*ncrp,
			unchar	*tp)
{
	unchar	id;

	/* get the id byte received from the reselecting target */
	id = ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SSID);

	NDBG20(("ncr53c810_get_target: instance=%d: lcrc=0x%x\n",
		ddi_get_instance(ncrp->n_dip), id));


	/* is it valid? */
	if (id & NB_SSID_VAL) {
		/* mask off extraneous bits */
		id &= NB_SSID_ENCID;
		NDBG1(("ncr53c810_get_target: ID %d reselected\n", id));
		*tp = id;
		return (TRUE);
	}
	cmn_err(CE_WARN, "ncr53c810_get_target: invalid reselect id %d\n", id);
	return (FALSE);
}

/*
 * The 8xx chips don't require the target number to be encoded
 * as a bit value. Just return the numeric target number.
 */
unchar
ncr53c810_encode_id(unchar id)
{
	return (id);
}

void
ncr53c810_set_syncio(
			ncr_t	*ncrp,
			npt_t	*nptp)
{
	unchar	sxfer = nptp->nt_selectparm.nt_sxfer;
	unchar	scntl3;

	/* Set SXFER register */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SXFER, sxfer);

	/* Set sync i/o clock divisor in SCNTL3 registers */
	if (sxfer != 0) {
		switch (nptp->nt_sscfX10) {
		case 10:
			scntl3 = NB_SCNTL3_SCF1 | ncrp->n_scntl3;
			break;

		case 15:
			scntl3 = NB_SCNTL3_SCF15 | ncrp->n_scntl3;
			break;

		case 20:
			scntl3 = NB_SCNTL3_SCF2 | ncrp->n_scntl3;
			break;

		case 30:
			scntl3 = NB_SCNTL3_SCF3 | ncrp->n_scntl3;
			break;
		}
		nptp->nt_selectparm.nt_scntl3 = scntl3;
		ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SCNTL3, scntl3);
	}

	/* set extended filtering if not Fast-SCSI (i.e., < 5BM/sec) */
	if (sxfer == 0 || nptp->nt_fastscsi == FALSE)
		ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_STEST2, NB_STEST2_EXT);
	else
		ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_STEST2, 0);
}

void
ncr53c810_setup_script(
			ncr_t	*ncrp,
			npt_t	*nptp)
{
	unchar nleft;

	NDBG18(("ncr53c810_setup_script: instance=%d\n",
		ddi_get_instance(ncrp->n_dip)));

	/* Set the Data Structure address register to */
	/* the physical address of the active table */
	ddi_io_putl(ncrp->n_handle, ncrp->n_reg + NREG_DSA, nptp->nt_dsa_physaddr);

	/* Set syncio clock divisors and offset registers in case */
	/* reconnecting after target reselected */
	ncr53c810_set_syncio(ncrp, nptp);

	/* Setup scratcha1 as the number of segments left to do */
	ddi_io_putb(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA1, nleft = nptp->nt_curdp.nd_left);

#ifdef	notdef
if (ncrp->n_state == NSTATE_ACTIVE && nleft > NCR_MAX_DMA_SEGS)
	debug_enter("\n\nBAD NLEFT\n\n");
#endif

	NDBG18(("ncr53c810_setup_script: instance=%d: okay\n",
		ddi_get_instance(ncrp->n_dip)));
}

void
ncr53c810_start_script(
			ncr_t	*ncrp,
			int	 script)
{
	/* Send the SCRIPT entry point address to the ncr chip */
	ddi_io_putl(ncrp->n_handle, ncrp->n_reg + NREG_DSP, ncr_scripts[script]);

	NDBG18(("ncr53c810_start_script: instance=%d: script=%d\n",
		ddi_get_instance(ncrp->n_dip), script));
}

void
ncr53c810_bus_reset(ncr_t *ncrp)
{
	NDBG1(("ncr53c810_bus_reset: instance=%d\n",
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
	(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SIST0);

	/* need short delay before clearing SIST1 */
	(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);
	(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);

	(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_SIST1);

	/* need short delay before reading DSTAT */
	(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);
	(void) ddi_io_getl(ncrp->n_handle, ncrp->n_reg + NREG_SCRATCHA0);

	/* clear any pending DMA interrupts */
	(void) ddi_io_getb(ncrp->n_handle, ncrp->n_reg + NREG_DSTAT);
}

/*
 * The NCR SDMS BIOS and the Siemens Nixdorf PCE-5S CAM BIOS both
 * use the CAM algorithm described in Annex A of the SCSI CAM (Common
 * Access Method) spec.
 */

int
ncr53c810_geometry(
			ncr_t			*ncrp,
			struct	scsi_address	*ap)
{
	ulong	totalsectors;
	ulong	cyls;
	ulong	heads;
	ulong	sectors;

	totalsectors = ADDR2NCRUNITP(ap)->nt_total_sectors;

	/* it's a 53c8xx controller, use CAM routine */
	if (CAMsetsize(totalsectors, &cyls, &heads, &sectors) != 0) {
		/* got an error from CAMsetsize() use default */
cmn_err(CE_CONT, "?ncr: CAMsetsize() error, using default geometry\n");
		heads = 255;
		sectors = 63;
	}

cmn_err(CE_CONT, "?ncr(%d,%d): instance=%d: sectors=%d heads=%d sectors=%d\n",
		ap->a_target, ap->a_lun,
		ddi_get_instance(ncrp->n_dip),
		totalsectors, heads, sectors);
	return (HBA_SETGEOM(heads, sectors));
}

nops_t	ncr53c810_nops = {
	"53c810",
	ncr_script_init,
	ncr_script_fini,
	ncr53c810_probe,
	ncr_get_irq_pci,
#ifdef	PCI_DDI_EMULATION
	ncr_xlate_irq_no_sid,
	ncr_get_ioaddr_pci,
#else
	ncr_xlate_irq_sid,
	NCR_PCI_RNUMBER,
#endif
	ncr53c810_reset,
	ncr53c810_init,
	ncr53c810_enable,
	ncr53c810_disable,
	ncr53c810_get_istat,
	ncr53c810_halt,
	ncr53c810_set_sigp,
	ncr53c810_reset_sigp,
	ncr53c810_get_intcode,
	ncr53c810_check_error,
	ncr53c810_dma_status,
	ncr53c810_scsi_status,
	ncr53c810_save_byte_count,
	ncr53c810_get_target,
	ncr53c810_encode_id,
	ncr53c810_setup_script,
	ncr53c810_start_script,
	ncr53c810_set_syncio,
	ncr53c810_bus_reset,
	ncr53c810_geometry
};
