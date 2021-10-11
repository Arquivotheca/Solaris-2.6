/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ident "@(#)tran_p3c.c	1.3	94/11/17 SMI"

/*
 *	Trantor P3C access file.
 *
 *
 */

#include <sys/dktp/trantor/trantor.h>
#include <sys/dktp/trantor/tran_p3c.h>

/*
 *	P3CPutControl
 *
 *  This routine writes the p3c mode and the n5380 register number to the
 *  P3C.
 */
static void
P3CPutControl(PBASE_REGISTER baseIoAddress, UCHAR mode, UCHAR reg)
{

	UCHAR tmp;

	/* output the mode and 5380 register to the parallel data reg */
	tmp = (mode & (PC_ADRS ^ 0xff)) | reg;
	ParallelPut(baseIoAddress, PARALLEL_DATA, tmp);

	/*  */
	ParallelGet(baseIoAddress, PARALLEL_CONTROL, &tmp);
	tmp = tmp & (0xff ^ P_BUFEN);
	tmp = tmp | P_STB;
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);

	tmp = tmp & (0xff ^ P_STB);
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);
}

/*
 *	P3CSetPrinterMode
 *
 *  This routine sets the P3C to printer pass through mode.  This is the
 *  default mode and should be set after the brief use of scsi mode.
 */
static void
P3CSetPrinterMode(PBASE_REGISTER baseIoAddress, UCHAR data, UCHAR control)
{
	UCHAR tmp;

	/* to prevent glitching, put P3C into read sig nibble mode */
	P3CPutControl(baseIoAddress, PCCC_MODE_RSIG_NIBBLE, 0);


	/* restore data register */
	ParallelPut(baseIoAddress, PARALLEL_DATA, data);

	/* restore control register */
	/* leave p_init negated */
	tmp = control | P_INIT;
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);
}

/*
 *	P3CSetScsiMode
 *
 *	This routine sets the P3C into scsi mode.  Now the parallel port can
 *	be used to send commands the the n5380.  This mode should be set only
 *	briefly during when the scsi command is being executed.
 */
static void
P3CSetScsiMode(PBASE_REGISTER baseIoAddress, UCHAR *data, UCHAR *control)
{
	UCHAR tmp;

	/* save parallel data */
	ParallelGet(baseIoAddress, PARALLEL_DATA, data);

	/* zero data register */
	ParallelPut(baseIoAddress, PARALLEL_DATA, 0);

	/* save parallel control */
	ParallelGet(baseIoAddress, PARALLEL_CONTROL, control);
	*control = *control & (P_BUFEN ^ 0xff);

	/* if in peripheral mode, get out to avoid glitch */
	tmp = *control | P_INIT;
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);

	/* set ID pattern to data register */
	ParallelPut(baseIoAddress, PARALLEL_DATA, 0xfe);

	/* clear slc and init on control */
	tmp = tmp & ((P_SLC | P_INIT) ^0xff);
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);

	/* assert slc  */
	tmp = tmp | P_SLC;
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);

	/* clear all bits in control */
	ParallelPut(baseIoAddress, PARALLEL_CONTROL, 0);
}

/*
 *	P3CCheckAdapter
 *
 *	This routine is used to sense the presense of the P3C adapter out
 *	on the Parallel port.  It will only detect the adapter if a device
 *	is providing termination power.
 */
static BOOLEAN
P3CCheckAdapter(PBASE_REGISTER baseIoAddress)
{
	UCHAR data;
	UCHAR control;
	UCHAR tmp;
	UCHAR sig0, sig1;
	UCHAR sig_byte[3];
	int i;

	/* set scsi mode */
	P3CSetScsiMode(baseIoAddress, &data, &control);

	/* set read sig nibble mode */
	P3CPutControl(baseIoAddress, PCCC_MODE_RSIG_NIBBLE, 0);

	/* zero data reg to get max contention during read signature */
	ParallelPut(baseIoAddress, PARALLEL_DATA, 0);

	for (i = 0; i < 3; i++) {
		/* Assert SLC  */
		ParallelGet(baseIoAddress, PARALLEL_CONTROL, &tmp);
		tmp = (tmp & (P_BUFEN ^ 0xff)) | P_SLC;
		ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);

		/* read in the status reg, it has the low nibble */
		ParallelGet(baseIoAddress, PARALLEL_STATUS, &sig0);

		/* Deassert SLC  */
		ParallelGet(baseIoAddress, PARALLEL_CONTROL, &tmp);
		tmp = (tmp & (P_BUFEN ^ 0xff)) & (P_SLC ^ 0xff);
		ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);

		/* note: there must be a delay here for timing */

		/* Assert SLC  */
		ParallelGet(baseIoAddress, PARALLEL_CONTROL, &tmp);
		tmp = (tmp & (P_BUFEN ^ 0xff)) | P_SLC;
		ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);

		/* read in the status reg, it has the high nibble */
		ParallelGet(baseIoAddress, PARALLEL_STATUS, &sig1);

		/* Deassert SLC  */
		ParallelGet(baseIoAddress, PARALLEL_CONTROL, &tmp);
		tmp = (tmp & (P_BUFEN ^ 0xff)) & (P_SLC ^ 0xff);
		ParallelPut(baseIoAddress, PARALLEL_CONTROL, tmp);

		sig_byte[i] = ((sig0 >> 3) & 0xf) | ((sig1 << 1) & 0xf0);
	}

	/* set parallel port for use by printer */

	P3CSetPrinterMode(baseIoAddress, data, control);

	/* compare the signature bytes */
	if ((sig_byte[0] == 0x6c) && (sig_byte[1] == 0x55) &&
			(sig_byte[2] == 0xaa)) {
		return (TRUE);
	}

	return (FALSE);
}
