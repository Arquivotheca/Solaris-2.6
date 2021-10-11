/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ident "@(#)tran_card.c	1.6	95/02/16 SMI"

/*
 *	tran_card.c
 *
 *	Trantor T348 Adapter Specific File
 *
 *	See also tran_card.h which may redefine some functions with #defines.
 *
 *
 */


/*
 *	CardCheckAdapter
 *
 *	Full check is used for initial device detection and verifies
 *	the presence and basic operation of both the P3C chip and the
 *	N5380 chip.  Non-full check is used for determining that the
 *	device has not been removed and simply checks for the presence
 *	of the P3C.
 */
static BOOLEAN
CardCheckAdapter(PBASE_REGISTER baseIoAddress, int full)
{
	UCHAR data;
	UCHAR control;
	BOOLEAN rval;

	/* Verify that there is a P3C chip */
	if (!P3CCheckAdapter(baseIoAddress)) {
		return (FALSE);
	}

	if (!full)
		return (TRUE);

	CardResetBus(baseIoAddress);

	/* put the parallel adapter into scsi mode */
	P3CSetScsiMode(baseIoAddress, &data, &control);

	/* Verify that there is an N5380 chip */
	rval = N5380CheckAdapter(baseIoAddress);

	/* put the parallel adapter back to parallel mode */
	P3CSetPrinterMode(baseIoAddress, data, control);
	return (rval);
}


/*
 *	CardDisableInterrupt
 *
 */
static void
CardDisableInterrupt(PBASE_REGISTER baseIoAddress)
{
	UCHAR data;
	UCHAR control;

	/* put the parallel adapter into scsi mode */
	P3CSetScsiMode(baseIoAddress, &data, &control);

	/* Verify that there is an N5380 chip */
	N5380DisableInterrupt(baseIoAddress);

	/* put the parallel adapter back to parallel mode */
	P3CSetPrinterMode(baseIoAddress, data, control);
}


/*
 *	CardStartCommandInterrupt
 *
 */
static int
CardStartCommandInterrupt(PBASE_REGISTER baseIoAddress, UCHAR target,
    UCHAR lun,
		UCHAR *pcmd, UCHAR cmdlen, BOOLEAN dir,
		UCHAR *pbfr, ULONG datalen,
		UCHAR *pstatus)
{
	int rval;
	UCHAR data;
	UCHAR control;

	/* put the parallel adapter into scsi mode */
	P3CSetScsiMode(baseIoAddress, &data, &control);

	/* select the target and send the command bytes  */
	if (rval = ScsiSendCommand(baseIoAddress, target, lun, pcmd, cmdlen)) {
		goto done;
	}

	rval = ScsiFinishCommandInterrupt(baseIoAddress, target, lun,
			pcmd, cmdlen, dir, pbfr, datalen, pstatus);

done:
	/* put the parallel adapter back to parallel mode */
	P3CSetPrinterMode(baseIoAddress, data, control);
	return (rval);
}


/*
 *	CardResetBus
 *
 */
static void
CardResetBus(PBASE_REGISTER baseIoAddress)
{
	UCHAR data;
	UCHAR control;

	/* Put the p3c into SCSI mode to access the N5380 */
	P3CSetScsiMode(baseIoAddress, &data, &control);

	/* Reset the N5380 SCSI bus */
	N5380ResetBus(baseIoAddress);

	/* Put the p3c back into printer mode */
	P3CSetPrinterMode(baseIoAddress, data, control);
}

/*
 *	CardWriteBytesFast
 *
 *  This routine is used by the ScsiFnc routines to write bytes to the scsi
 *  bus quickly.  The ScsiFnc routines don't know how to do this quickly for
 *  a particular card, so they call this.  This routine can be mapped to the
 *  slower ScsiWriteBytesSlow routine for small transferrs or if this routine
 *  is not supported.
 */

/*ARGSUSED*/
static int
CardWriteBytesFast(PBASE_REGISTER baseIoAddress, UCHAR *pbytes,
    ULONG len, UCHAR phase)
{
	int rval = 0;
	UCHAR control;
	UCHAR tmp;

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("CardWriteBytesFast: enter, len = %d\n", len);
#endif /* TRANTOR_DEBUG */
	/* clear any interrupt condition on the 5380 */
	CardN5380Get(baseIoAddress, N5380_RESET_PARITY, &tmp);

	/* set the dma bit of 5380 */
	N5380Set(baseIoAddress, N5380_MODE, MR_DMA);

	/* start the dma on the 5380 */
	CardN5380Put(baseIoAddress, N5380_DMA_SEND, 1);

	/* put the P3C into write dma mode */
	P3CPutControl(baseIoAddress, PCCC_MODE_WDMA, 0);

	/* start control reg off zero'ed, enable dma write mode */
	control = 0;
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, 0);

	{
		ULONG i;

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("CardWriteBytesFast: len = %d\n", len);
#endif /* TRANTOR_DEBUG */
		/* write all the bytes */
		for (i = 0; i < len; i++) {

			/* wait for next byte */
			if (rval = ParallelWaitBusy(baseIoAddress,
			    TIMEOUT_FASTDATA, &tmp)) {
				goto done;
			}

			/*
			 * Allow others to get the CPU sometimes during
			 * long data transfers.  Note: we do this after
			 * waiting for the port busy signal to go away.
			 * Used to use 0x200 byte boundaries but that
			 * caused Archive tape drives to choke.  Now do
			 * it every 0x200 bytes starting at 0x205.
			 */
			if (i > 0x200 && (i & 0x1FF) == 5)
				delay(1);

			/* write the next data byte */
			ParallelPut(baseIoAddress, PARALLEL_DATA, pbytes[i]);

			/* strobe to next byte */
			control = control ^ P_AFX;
			ParallelPut(baseIoAddress, PARALLEL_CONTROL, control);

		}
	}

done:
	/* clear the dma bit of 5380 */
	N5380Clear(baseIoAddress, N5380_MODE, MR_DMA);

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("CardWriteBytesFast: exit\n");
#endif /* TRANTOR_DEBUG */
	return (rval);
}

/*
 *	CardReadBytesFast
 *
 *  This routine is used by the ScsiFnc routines to write bytes to the scsi
 *  bus quickly.  The ScsiFnc routines don't know how to do this quickly for
 *  a particular card, so they call this.  This routine can be mapped to the
 *  slower ScsiReadBytesSlow routine for small transferrs or if this routine
 *  is not supported.
 */
static int
CardReadBytesFast(PBASE_REGISTER baseIoAddress, UCHAR *pbytes,
    ULONG len, UCHAR phase)
{
	int rval = 0;
	UCHAR data;
	UCHAR tmp;

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("CardReadBytesFast: enter, len = %d\n", len);
#endif /* TRANTOR_DEBUG */
	/* use slow mode for inquiry type commands */
	if (len < 512)
		return (ScsiReadBytesSlow(baseIoAddress, pbytes, len, phase));

	/* clear any interrupt condition on the 5380 */
	CardN5380Get(baseIoAddress, N5380_RESET_PARITY, &tmp);

	/* set the dma bit of 5380 */
	N5380Set(baseIoAddress, N5380_MODE, MR_DMA);

	/* start the dma on the 5380 */
	CardN5380Put(baseIoAddress, N5380_INITIATOR_RECV, 1);

	/* put the P3C into read dma mode */
	P3CPutControl(baseIoAddress, PCCC_MODE_RDMA_NIBBLE, 0);

	/* start data reg to select high nibble */
	data = 0x80;
	ParallelPut(baseIoAddress, PARALLEL_DATA, data);
	data = 0;

	/* start control reg with P_SLC */
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, P_SLC);

	{
		ULONG i;
		UCHAR nib0, nib1;

		/* read all the bytes */
		for (i = 0; i < len; i++) {

			/* wait for next byte */
			/* reading in first nibble */
			if (rval = ParallelWaitBusy(baseIoAddress,
			    TIMEOUT_FASTDATA, &nib1)) {
				goto done;
			}

			/*
			 * Allow others to get the CPU sometimes during
			 * long data transfers.  Note: we do this after
			 * waiting for the port busy signal to go away.
			 */
			if (i && (i % 512) == 0)
				delay(1);

			/* select the low nibble */
			ParallelPut(baseIoAddress, PARALLEL_DATA, data);

			/* read low nibble */
			ParallelGet(baseIoAddress, PARALLEL_STATUS, &nib0);

			pbytes[i] = ((nib1 << 1) & 0xf0) | ((nib0 >> 3) & 0xf);

			/* prepare parallel data reg for next byte */
			data = data ^ 0x40;

			/* strobe next byte with high nibble selected */
			tmp = data | 0x80;
			ParallelPut(baseIoAddress, PARALLEL_DATA, tmp);
		}
	}

done:

	/* zero control register, disable read dma mode */
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, 0);

	/* clear the dma bit of 5380 */
	N5380Clear(baseIoAddress, N5380_MODE, MR_DMA);

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("CardReadBytesFast: exit(rval = %xh)\n", rval);
#endif /* TRANTOR_DEBUG */
	return (rval);
}

/*
 *	CardN5380Put
 *
 *  This routine is used by the N5380.C module to write byte to a 5380
 *  controller.  This allows the module to be card independent.  Other
 *  modules that assume a N5380 may also use this function.
 */
static void
CardN5380Put(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte)
{

	P3CPutControl(baseIoAddress, PCCC_MODE_WPER, reg);

	/* write the byte */
	ParallelPut(baseIoAddress, PARALLEL_DATA, byte);

	/* toggle the data_ready line */
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, P_SLC);
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, 0);
}

/*
 *	CardN5380Get
 *
 *  This routine is used by the N5380.C module to get a byte from a 5380
 *  controller.  This allows the module to be card independent.  Other
 *  modules that assume a N5380 may also use this function.
 */
static void
CardN5380Get(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR *byte)
{
	UCHAR tmp, tmp1;

	P3CPutControl(baseIoAddress, PCCC_MODE_RPER_NIBBLE, reg);

	/* assert slc */
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, P_SLC);

	/* select high nibble */
	ParallelPut(baseIoAddress, PARALLEL_DATA, 0x80);

	/* read high nibble */
	ParallelGet(baseIoAddress, PARALLEL_STATUS, &tmp);

	/* compute high nibble */
	tmp = (tmp << 1) & 0xf0;

	/* select low nibble */
	ParallelPut(baseIoAddress, PARALLEL_DATA, 0x00);

	/* read low nibble */
	ParallelGet(baseIoAddress, PARALLEL_STATUS, &tmp1);

	/* compute low nibble */
	tmp1 = (tmp1 >> 3) & 0x0f;

	/* compute and return byte */
	*byte = tmp1 | tmp;

	/* clear slc */
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, 0);
}
