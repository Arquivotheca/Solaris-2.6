/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ident "@(#)tran_pp.c	1.3	94/08/15 SMI"

/*
 *	Trantor parallel port access file.
 *
 */


/*
 * IO Register Definition
 */

typedef struct tagRegister {
	UCHAR ParallelPort[3];
} *PCARD_BASE_REGISTER;


/*
 *	ParallelGet
 *
 *	This routine gets a byte from a parallel port register.
 */
static void
ParallelGet(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR *byte)
{
	*byte = ((short)inb(baseIoAddress+reg) & 0xff);
}

/*
 *	ParallelPut
 *
 *	This routine write a byte to a parallel port register.
 */
static void
ParallelPut(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte)
{
	outb(baseIoAddress+reg, byte);
}


/*
 *	ParallelWaitBusy
 *
 *	This routine waits until the busy line is released.
 */
static int
ParallelWaitBusy(PBASE_REGISTER baseIoAddress, ULONG usec, UCHAR *data)
{
	ULONG i;
	clock_t tick_len = drv_hztousec((clock_t)1);

	/* see if the flag comes back quickly */
	for (i = 0; i < tick_len; i += 10) {
		ParallelGet(baseIoAddress, PARALLEL_STATUS, data);
		if (*data & P_BUSY) {
			return (0);
		}
		ScsiPortStallExecution(10);
	}

	/* ok, it did not come back quickly, we will yield to other processes */
	for (; i < usec; i += tick_len) {
		ParallelGet(baseIoAddress, PARALLEL_STATUS, data);
		if (*data & P_BUSY) {
			return (0);
		}
		delay(1);
	}

	/* return with an error, non-zero indicates timeout  */
	return (PARALLEL_TO);
}


/*
 *	ParallelIntEnabled
 *
 *	This routine determines whether a parallel port has interrupts
 *	enabled.
 */
static int
ParallelIntEnabled(PBASE_REGISTER baseIoAddress)
{
	UCHAR control;

	ParallelGet(baseIoAddress, PARALLEL_CONTROL, &control);
	return ((control & P_IRQEN) ? 1 : 0);
}
