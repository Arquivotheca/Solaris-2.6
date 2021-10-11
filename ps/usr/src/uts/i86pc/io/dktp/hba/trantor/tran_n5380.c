/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ident "@(#)tran_n5380.c	1.4	94/11/17 SMI"

/*
 *
 *	Trantor T348 N5380 access file.
 *
 */

#ifdef TRANTOR_DEBUG
/*
 *	trantor_debug is used to turn on/off specific debug statements, it is a
 *	bit mapped value as follows:
 */
#define	DENTRY		0x01			/* data entry		*/
#define	DIO		0x02			/* io processing	*/
#define	LOW_LEVEL	0x04			/* low level debugging	*/

extern	int	trantor_debug;
#endif  /* TRANTOR_DEBUG */

/*
 *	N5380CheckAdapter
 *
 *	This routine checks for the presense of a 5380.
 */
static BOOLEAN
N5380CheckAdapter(PBASE_REGISTER baseIoAddress)
{
	UCHAR tmp;

	/* set the phase to NULL  */
	if (N5380SetPhase(baseIoAddress, PHASE_NULL)) {
		return (FALSE);
	}

	/*	check to see that the 5380 data register behaves as expected */

	CardN5380Put(baseIoAddress, N5380_INITIATOR_CMD, IC_DATA);

	/* check for 0x55 write/read in data register */

	CardN5380Put(baseIoAddress, N5380_OUTDATA, 0x55);
	ScsiPortStallExecution(100);
	CardN5380Get(baseIoAddress, N5380_CURDATA, &tmp);
	if (tmp != 0x55) {
		return (FALSE);
	}

	/* check for 0xaa write/read in data register */

	CardN5380Put(baseIoAddress, N5380_OUTDATA, 0xaa);
	ScsiPortStallExecution(1);
	CardN5380Get(baseIoAddress, N5380_CURDATA, &tmp);

	if (tmp != 0xaa) {
		return (FALSE);
	}

	CardN5380Put(baseIoAddress, N5380_INITIATOR_CMD, 0);
	ScsiPortStallExecution(1);
	CardN5380Get(baseIoAddress, N5380_CURDATA, &tmp);

	/* data now should not match .... */
	if (tmp == 0xaa) {
		return (FALSE);
	}

	return (TRUE);
}

/*
 *	N5380Select
 *
 *	This routine selects a device through the 5380.
 */
/*ARGSUSED*/
static int
N5380Select(PBASE_REGISTER baseIoAddress, UCHAR target, UCHAR lun)
{
	int rval;

	/* set the phase to NULL */
	if (rval = N5380SetPhase(baseIoAddress, PHASE_NULL)) {
		return (rval);
	}

	/* wait for bsy to go away if someone else is using bus */
	if (rval = N5380WaitNoBusy(baseIoAddress, TIMEOUT_BUSY)) {
		return (rval);
	}

	/* assert our id and the target id on the bus */
	CardN5380Put(baseIoAddress, N5380_OUTDATA,
	    (UCHAR)((1<<HOST_ID) + (1<<target)));

	/* assert the data on the bus and assert select */
	N5380Set(baseIoAddress, N5380_INITIATOR_CMD, IC_SELECT+IC_DATA);

	/* wait for bsy to be asserted */
	if (rval = N5380WaitBusy(baseIoAddress, 250)) {
		/* clear the data bus */
		CardN5380Put(baseIoAddress, N5380_OUTDATA, 0);

		/* clear select and IC_DATA */
		N5380Clear(baseIoAddress, N5380_INITIATOR_CMD,
		    IC_SELECT+IC_DATA);

		return (HOST_SEL_TO);
	}

	/* clear the data bus */
	CardN5380Put(baseIoAddress, N5380_OUTDATA, 0);

	/* assert the data on the bus, clear select , IC_DATA already set */
	N5380Clear(baseIoAddress, N5380_INITIATOR_CMD, IC_SELECT);

	return (0);
}

/*
 *	N5380WaitBusy
 *
 *	This routine waits for the busy line to be asserted.
 *
 *	The wait is a busy wait for up to one clock tick and thereafter
 *	switches to a sleep with periodic polling.
 */
static int
N5380WaitBusy(PBASE_REGISTER baseIoAddress, ULONG usec)
{
	ULONG i;
	clock_t tick_len = drv_hztousec((clock_t)1);

	/* see if the flag comes back quickly */
	for (i = 0; i < tick_len; i += 10) {
		if (N5380Test(baseIoAddress, N5380_CURSTAT, CS_BUSY)) {
			return (0);
		}
		ScsiPortStallExecution(10);
	}

	/* ok, it did not come back quickly, we will yield to other processes */
	for (; i < usec; i += tick_len) {
		if (N5380Test(baseIoAddress, N5380_CURSTAT, CS_BUSY)) {
			return (0);
		}
		delay(1);
	}

	/* return with an error, non-zero indicates timeout  */
	return (HOST_BUS_FREE);
}

/*
 *	N5380WaitNoBusy
 *
 *	This routine waits for the Busy line to be deasserted.
 *
 *	The wait is a busy wait for up to one clock tick and thereafter
 *	switches to a sleep with periodic polling.
 */
static int
N5380WaitNoBusy(PBASE_REGISTER baseIoAddress, ULONG usec)
{
	ULONG i;
	clock_t tick_len = drv_hztousec((clock_t)1);

	/* see if the flag comes back quickly */
	for (i = 0; i < tick_len; i += 10) {
		if (!N5380Test(baseIoAddress, N5380_CURSTAT, CS_BUSY)) {
			return (0);
		}
		ScsiPortStallExecution(10);
	}

	/* ok, it did not come back quickly, we will yield to other processes */
	for (; i < usec; i += tick_len) {
		if (!N5380Test(baseIoAddress, N5380_CURSTAT, CS_BUSY)) {
			return (0);
		}
		delay(1);
	}

	/* return with an error, non-zero indicates timeout  */
	return (HOST_BUS_BUSY);
}

/*
 *	N5380WaitRequest
 *
 *	This routine waits for request to be asserted.
 *
 *	The wait is a busy wait for up to one clock tick and thereafter
 *	switches to a sleep with periodic polling.
 */
/*ARGSUSED*/
static int
N5380WaitRequest(PBASE_REGISTER baseIoAddress, ULONG usec)
{
	ULONG i;
	clock_t tick_len = drv_hztousec((clock_t)1);

	/* see if the flag comes back quickly */
	for (i = 0; i < tick_len; i += 10) {
		if (N5380Test(baseIoAddress, N5380_CURSTAT, CS_REQ)) {
			return (0);
		}
		ScsiPortStallExecution(10);
	}

	/* ok, it did not come back quickly, we will yield to other processes */
#ifdef TIMEOUT_ON_WAIT_REQUEST
	for (; i < usec; i += tick_len)
#else
	for (;;)
#endif
	{
		if (N5380Test(baseIoAddress, N5380_CURSTAT, CS_REQ)) {
			return (0);
		}
		if (!N5380Test(baseIoAddress, N5380_CURSTAT, CS_BUSY)) {
			return (HOST_BUS_FREE);
		}
		delay(1);
	}

#ifdef TIMEOUT_ON_WAIT_REQUEST
	/* return with an error, non-zero indicates timeout  */
	return (HOST_REQ_FAILED);
#endif
}

/*
 *	N5380WaitNoRequest
 *
 *	This routine waits for request to be deasserted.
 *
 *	The wait is a busy wait for up to one clock tick and thereafter
 *	switches to a sleep with periodic polling.
 */
static int
N5380WaitNoRequest(PBASE_REGISTER baseIoAddress, ULONG usec)
{
	ULONG i;
	clock_t tick_len = drv_hztousec((clock_t)1);

	/* see if the flag comes back quickly */
	for (i = 0; i < tick_len; i += 10) {
		if (!N5380Test(baseIoAddress, N5380_CURSTAT, CS_REQ)) {
			return (0);
		}
		ScsiPortStallExecution(10);
	}

	/* ok, it did not come back quickly, we will yield to other processes */
	for (; i < usec; i += tick_len) {
		if (!N5380Test(baseIoAddress, N5380_CURSTAT, CS_REQ)) {
			return (0);
		}
		delay(1);
	}

	/* return with an error, non-zero indicates timeout  */
	return (HOST_DEREQ_FAILED);
}

/*
 *	N5380GetPhase
 *
 *	This routine returns the current scsi bus phase.
 */
static int
N5380GetPhase(PBASE_REGISTER baseIoAddress, UCHAR *phase)
{
	UCHAR tmp;
	int rval;

	/* wait for request to be asserted */
	if (rval = N5380WaitRequest(baseIoAddress, TIMEOUT_REQUEST)) {
		return (rval);
	}

	/* get current phase */
	CardN5380Get(baseIoAddress, N5380_CURSTAT, &tmp);

	/* return the phase */
	*phase = (tmp >> 2) & 0x7;

	return (0);
}

/*
 *	N5380SetPhase
 *
 *	This routine sets the 5380's expected bus phase in the target command
 *	register.
 */
static int
N5380SetPhase(PBASE_REGISTER baseIoAddress, UCHAR phase)
{
	UCHAR tmp;

	/* phase must correspond the the bits of the target command register */
	CardN5380Put(baseIoAddress, N5380_TARGET_CMD, phase);

	CardN5380Get(baseIoAddress, N5380_MODE, &tmp);

	/* set the assert data bus bit to the right direction  */
	if (phase & TC_IO) {
		/* IO is set */
		if (tmp & MR_TARGET) {
			/*
			 * we are in target mode always set the assert data bit
			 */
			N5380Set(baseIoAddress, N5380_INITIATOR_CMD, IC_DATA);
		} else {
			/* we are in initiator mode clear the data enable bit */
			N5380Clear(baseIoAddress, N5380_INITIATOR_CMD, IC_DATA);
		}
	} else {
		/* IO is not set */
		if (tmp & MR_TARGET) {
			/*
			 * we are in initiator mode always set the assert
			 * data bit
			 */
			N5380Clear(baseIoAddress, N5380_INITIATOR_CMD, IC_DATA);
		} else {
			/* we are in target mode clear the data assert bit */
			N5380Set(baseIoAddress, N5380_INITIATOR_CMD, IC_DATA);
		}
	}

	/* no errors can occur from this function */
	return (0);
}

/*
 *	N5380PutByte
 *
 * This routine writes a byte to the scsi bus using the req/ack protocol.
 * To use this routine the phase should be set correctly using N5380SetPhase.
 */
static int
N5380PutByte(PBASE_REGISTER baseIoAddress, ULONG usec, UCHAR byte)
{
	int rval;

	/* put data byte to data register */
	CardN5380Put(baseIoAddress, N5380_OUTDATA, byte);

	/* wait for request to be asserted */
	if (rval = N5380ToggleAck(baseIoAddress, usec)) {
		return (rval);
	}
	return (0);
}

/*
 *	N5380GetByte
 *
 * This routine reads a byte from the scsi bus using the req/ack protocol.
 * To use this routine the phase should be set correctly using N5380SetPhase.
 */
static int
N5380GetByte(PBASE_REGISTER baseIoAddress, ULONG usec, UCHAR *byte)
{
	int rval;

	/* get data byte from data register */
	CardN5380Get(baseIoAddress, N5380_CURDATA, byte);

	/* wait for request to be asserted */
	if (rval = N5380ToggleAck(baseIoAddress, usec)) {
		return (rval);
	}
	return (0);
}

/*
 *	N5380ToggleAck
 *
 *	This routine performs the req/ack handshake.  It asserted ack, waits
 *	for request to be deasserted and then clears ack.
 */
static int
N5380ToggleAck(PBASE_REGISTER baseIoAddress, ULONG usec)
{
	int rval;
	UCHAR tmp;

	/* assert ack */
	CardN5380Get(baseIoAddress, N5380_INITIATOR_CMD, &tmp);
	tmp = tmp | IC_ACK;
	CardN5380Put(baseIoAddress, N5380_INITIATOR_CMD, tmp);

	/* wait for request to be disappear */
	if (rval = N5380WaitNoRequest(baseIoAddress, usec)) {
		return (rval);
	}

	/* clear ack */
	CardN5380Get(baseIoAddress, N5380_INITIATOR_CMD, &tmp);
	tmp = tmp & (IC_ACK^0xff);
	CardN5380Put(baseIoAddress, N5380_INITIATOR_CMD, tmp);

	return (0);
}

/*
 *	N5380ResetBus
 *
 *	This routine performs a Scsi Bus reset.
 */
static void
N5380ResetBus(PBASE_REGISTER baseIoAddress)
{
	/* reset the scsi bus */
	CardN5380Put(baseIoAddress, N5380_INITIATOR_CMD, IC_RESET);

	/* leave signal asserted for a little while... */
	delay(drv_usectohz(SCSI_RESET_TIME));

	/* Clear reset */
	CardN5380Put(baseIoAddress, N5380_INITIATOR_CMD, 0);
}

/*
 *	N5380EnableDmaWrite
 *
 *	This routine does the needed 5380 setup and initiates a dma write.
 */
#ifdef NOT_USED_IN_SOLARIS
static void
N5380EnableDmaWrite(PBASE_REGISTER baseIoAddress)
{
	UCHAR tmp;

	/* clear any interrupt condition on the 5380 */
	CardN5380Get(baseIoAddress, N5380_RESET_PARITY, &tmp);

	/* set the dma bit of 5380 */
	N5380Set(baseIoAddress, N5380_MODE, MR_DMA);

	/* start the dma on the 5380 */
	CardN5380Put(baseIoAddress, N5380_DMA_SEND, 1);
}
#endif /* NOT_USED_IN_SOLARIS */

/*
 *	N5380EnableDmaRead
 *
 *	This routine does the needed 5380 setup and initiates a dma read.
 */
#ifdef NOT_USED_IN_SOLARIS
static void
N5380EnableDmaRead(PBASE_REGISTER baseIoAddress)
{
	UCHAR tmp;

	/* clear any interrupt condition on the 5380 */
	CardN5380Get(baseIoAddress, N5380_RESET_PARITY, &tmp);

	/* set the dma bit of 5380 */
	N5380Set(baseIoAddress, N5380_MODE, MR_DMA);

	/* start the dma on the 5380 */
	CardN5380Put(baseIoAddress, N5380_INITIATOR_RECV, 1);
}
#endif /* NOT_USED_IN_SOLARIS */

/*
 *	N5380DisableDma
 *
 *	This routine does the needed 5380 setup and initiates a dma read.
 */
#ifdef NOT_USED_IN_SOLARIS
static void
N5380DisableDma(PBASE_REGISTER baseIoAddress)
{
	/* Clear the dma bit of 5380 */
	N5380Clear(baseIoAddress, N5380_MODE, MR_DMA);
}
#endif /* NOT_USED_IN_SOLARIS */

/*
 *	N5380Interrupt
 *
 *	This routine checks to see if the 5380 has asserted its interrupt line.
 */
#ifdef NOT_USED_IN_SOLARIS
static BOOLEAN
N5380Interrupt(PBASE_REGISTER baseIoAddress)
{
	return (N5380Test(baseIoAddress, N5380_BUS_STAT, BS_IRQ));
}
#endif /* NOT_USED_IN_SOLARIS */

/*
 *	N5380DisableInterrupt
 *
 *	This routine clears any pending 5380 interrupt condition.
 */
static void
N5380DisableInterrupt(PBASE_REGISTER baseIoAddress)
{
	UCHAR tmp;

	/* clear DMA mode */
	N5380Clear(baseIoAddress, N5380_MODE, MR_DMA);

	/* clear any interrupt condition on the 5380 */
	CardN5380Get(baseIoAddress, N5380_RESET_PARITY, &tmp);
}

/*
 *	N5380Set
 *
 *	Sets a mask in a 5380 register.
 */
static void
N5380Set(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte)
{
	UCHAR tmp;

	CardN5380Get(baseIoAddress, reg, &tmp);
	tmp |= byte;
	CardN5380Put(baseIoAddress, reg, tmp);
}

/*
 *	N5380Clear
 *
 *	Clears the given bit mask in a 5380 register.
 */
static void
N5380Clear(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte)
{
	UCHAR tmp;

	CardN5380Get(baseIoAddress, reg, &tmp);
	tmp &= (byte^0xff);
	CardN5380Put(baseIoAddress, reg, tmp);
}

/*
 *	N5380Test
 *
 *	Tests a bit mask in a 5380 register.
 */
static BOOLEAN
N5380Test(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR mask)
{
	UCHAR tmp;

	CardN5380Get(baseIoAddress, reg, &tmp);
	return (tmp & mask);
}

/*
 *	N5380DebugDump
 *
 *	Dumps registers 0-5 to the debug terminal.
 */
#ifdef NOT_USED_IN_SOLARIS
static void
N5380DebugDump(PBASE_REGISTER baseIoAddress)
{
	UCHAR tmp;
	int i;

	DebugPrint((0, "5380 registers:"));
	for (i = 0; i < 6; i++) {
		CardN5380Get(baseIoAddress, (UCHAR)i, &tmp);
		DebugPrint((0, " %02x", tmp));
	}
	DebugPrint((0, "\n"));
}
#endif /* NOT_USED_IN_SOLARIS */
