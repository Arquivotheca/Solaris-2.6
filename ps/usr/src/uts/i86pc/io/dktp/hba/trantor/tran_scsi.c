/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ident "@(#)tran_scsi.c	1.4	94/08/15 SMI"

/*
 *
 *	Trantor N5380 Scsi Functions file.  Contains higher level scsi
 *	functions.
 *
 */

#ifdef TRANTOR_DEBUG
/*
 *	trantor_debug is used to turn on/off specific debug statements, it is a
 *	bit mapped value as follows:
 */
#define	DENTRY		0x01    /* data entry		*/
#define	DIO		0x02    /* io processing	*/
#define	LOW_LEVEL	0x04    /* low level debugging	*/

extern	int	trantor_debug;
#endif  /* TRANTOR_DEBUG */



/*
 *	ScsiSendCommand
 *
 *	Selects a target and sends a scsi command during command phase.
 */
static int
ScsiSendCommand(PBASE_REGISTER baseIoAddress, UCHAR target,
		UCHAR lun, UCHAR *pcmd, UCHAR cmdlen)
{
	int rval;

	/* select the target */
	if (rval = N5380Select(baseIoAddress, target, lun)) {
		return (rval);
	}

	/* set the phase to Command */
	if (rval = N5380SetPhase(baseIoAddress, PHASE_COMMAND)) {
		return (rval);
	}

	/* send the command bytes */
	if (rval = CardWriteBytesCommand(baseIoAddress, pcmd, (ULONG)cmdlen,
	    PHASE_COMMAND))
		return (rval);

	return (0);
}


/*
 *	ScsiDoCommand
 *
 *	Executes a complete scsi command: all phase sequences without using
 *	interrupts.
 */
#ifdef NOT_USED_IN_SOLARIS
static int
ScsiDoCommand(PBASE_REGISTER baseIoAddress, UCHAR target, UCHAR lun,
		UCHAR *pcmd, UCHAR cmdlen, BOOLEAN dir,
		UCHAR *pbfr, ULONG datalen,
		UCHAR *pstatus)
{
	int rval;

	/* select the target and send the command bytes  */
	if (rval = ScsiSendCommand(baseIoAddress, target, lun, pcmd, cmdlen)) {
		return (rval);
	}

	rval = ScsiFinishCommandInterrupt(baseIoAddress, target, lun,
			pcmd, cmdlen, dir, pbfr, datalen, pstatus);

	return (rval);
}
#endif /* NOT_USED_IN_SOLARIS */

/*
 *	ScsiStartCommandInterrupt
 *
 *  Executes a scsi command up to the end of command phase.  After this, the
 *  interrupt will come in and ScsiFinishCommandInterrupt should be called to
 *  complete the data, status, and message phases.
 */
#ifdef NOT_USED_IN_SOLARIS
static int
ScsiStartCommandInterrupt(PBASE_REGISTER baseIoAddress, UCHAR target,
    UCHAR lun,
		UCHAR *pcmd, UCHAR cmdlen, BOOLEAN dir,
		UCHAR *pbfr, ULONG datalen,
		UCHAR *pstatus)
{
	int rval;

	/* select the target and send the command bytes  */
	if (rval = ScsiSendCommand(baseIoAddress, target, lun, pcmd, cmdlen)) {
		return (rval);
	}

	/* enable the interrupt */
	CardEnableInterrupt(baseIoAddress);

	/*
	 * if request is already up, we may have missed the interrupt,
	 * it is done
	 */
	if (N5380Test(baseIoAddress, N5380_CURSTAT, CS_REQ)) {

		/* no need for interrupt, we are ready to proceed */
		CardDisableInterrupt(baseIoAddress);

		/* finish the command now... */
		return (CardFinishCommandInterrupt(baseIoAddress, target, lun,
				pcmd, cmdlen, dir, pbfr, datalen, pstatus));
	}

	return (0);
}
#endif /* NOT_USED_IN_SOLARIS */

/*
 *	ScsiFinishComamndInterrupt
 *
 *	Called to finish a command that has been started by
 *	ScsiStartCommandInterupt.
 *	This function completes the data, status, and message phases.
 */
/*ARGSUSED*/
static int
ScsiFinishCommandInterrupt(PBASE_REGISTER baseIoAddress, UCHAR target,
    UCHAR lun, UCHAR *pcmd, UCHAR cmdlen, BOOLEAN dir, UCHAR *pbfr,
    ULONG datalen, UCHAR *pstatus)

{
	int rval;
	UCHAR phase;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("ScsiFinishCommandInterrupt: enter\n");
#endif /* TRANTOR_DEBUG */
	/* get current phase */
	if (rval = N5380GetPhase(baseIoAddress, &phase)) {
		return (rval);
	}

	/* is there a data phase?? */
	if (datalen && phase != PHASE_STATUS) {

		/* read/write the data if there is a data phase */
		if (rval = ScsiDoIo(baseIoAddress, dir, pbfr, datalen)) {

			/*
			 * allow for the possibility of a phase
			 * change: data underrun
			 */
			if (rval != HOST_PHASE_ERR) {
				return (rval);
			}
		}
	}

	/* get the stat and message bytes */
	if (rval = ScsiGetStat(baseIoAddress, pstatus)) {
		return (rval);
	}

	/* for now, we never return pending  */
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("ScsiFinishCommandInterrupt: exit\n");
#endif /* TRANTOR_DEBUG */
	return (0);
}

/*
 *	ScsiWriteBytesSlow
 *
 *	This functions writes bytes to the scsi bus using the slow req/ack
 *	handshake.  Faster methods are generally avaiable, but they are
 *	dependent on how the card inplements the dma capabilities of the 5380.
 *	This is a sure-fire slow method that works.  It is great to bring up
 *	new cards.
 */
static int
ScsiWriteBytesSlow(PBASE_REGISTER baseIoAddress, UCHAR *pbytes, ULONG len,
    UCHAR phase)
{
	ULONG i;
	int rval;
	UCHAR tmp;

#ifdef	TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("ScsiWriteBytesSlow: enter, len = %d\n", len);
#endif /* TRANTOR_DEBUG */
	for (i = 0; i < len; i++) {

		/* wait for request to be asserted */
		if (rval = N5380GetPhase(baseIoAddress, &tmp)) {
			return (rval);
		}

		/* see if phase match */
		if (phase != tmp) {
			return (HOST_PHASE_ERR);
		}

		if (rval = N5380PutByte(baseIoAddress, TIMEOUT_REQUEST,
		    pbytes[i])) {
			return (rval);
		}
	}
#ifdef	TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("ScsiWriteBytesSlow: exit\n");
#endif /* TRANTOR_DEBUG */

	return (0);
}


/*
 *	ScsiReadBytesSlow
 *
 *	This functions reads bytes to the scsi bus using the slow req/ack
 *	handshake. Faster methods are generally avaiable, but they are dependent
 *	on how the card inplements the dma capabilities of the 5380.  This
 *	is a sure-fire slow method that works. It is great to bring up
 *	new cards.
 */
static int
ScsiReadBytesSlow(PBASE_REGISTER baseIoAddress, UCHAR *pbytes, ULONG len,
    UCHAR phase)
{
	ULONG i;
	int rval;
	UCHAR tmp;

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("ScsiReadBytesSlow: enter, len = %d\n", len);
#endif /* TRANTOR_DEBUG */
	for (i = 0; i < len; i++) {

		/* wait for request to be asserted */
		if (rval = N5380GetPhase(baseIoAddress, &tmp)) {
			return (rval);
		}

		/* see if phase match */
		if (phase != tmp) {
			return (HOST_PHASE_ERR);
		}

		if (rval = N5380GetByte(baseIoAddress, TIMEOUT_REQUEST,
		    &pbytes[i])) {
			return (rval);
		}
	}

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("ScsiReadBytesSlow: exit\n");
#endif /* TRANTOR_DEBUG */
	return (0);
}

/*
 *	ScsiDoIo
 *
 *	This function does the I/O during a data phase.
 */
static int
ScsiDoIo(PBASE_REGISTER baseIoAddress, BOOLEAN dir,	UCHAR *pbfr,
		ULONG datalen)
{
	int rval;

	if (dir) {
		/* data write */

		/* set the phase to data out */
		if (rval = N5380SetPhase(baseIoAddress, PHASE_DATAOUT)) {
			return (HOST_PHASE_ERR);
		}

		/* send the bytes */
		if (rval = CardWriteBytesFast(baseIoAddress, pbfr, datalen,
		    PHASE_DATAOUT)) {
			return (rval);
		}
	} else {
		/* data read */

		/* set the phase to data in */
		if (rval = N5380SetPhase(baseIoAddress, PHASE_DATAIN)) {
			return (HOST_PHASE_ERR);
		}

		/* read the bytes */
		if (rval = CardReadBytesFast(baseIoAddress, pbfr, datalen,
		    PHASE_DATAIN)) {
			return (rval);
		}
	}

	return (0);
}

/*
 *	ScsiGetStat
 *
 *	This function gets the status and message bytes.
 */
static int
ScsiGetStat(PBASE_REGISTER baseIoAddress, UCHAR *pstatus)
{
	UCHAR tmp;
	int rval;

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("ScsiGetStat: enter, pstatus = %xh\n", *pstatus);
#endif /* TRANTOR_DEBUG */
	/* set the phase to Status Phase */
	if (rval = N5380SetPhase(baseIoAddress, PHASE_STATUS)) {
		return (rval);
	}

	/* wait for request to be asserted */
	if (rval = N5380GetPhase(baseIoAddress, &tmp)) {
		return (rval);
	}

	/* see if phase match */
	if (PHASE_STATUS != tmp) {
		return (HOST_PHASE_ERR);
	}

	/* get the status byte */
	if (rval = N5380GetByte(baseIoAddress, TIMEOUT_REQUEST, pstatus)) {
		return (rval);
	}

	/* set the phase to Message In Phase */
	if (rval = N5380SetPhase(baseIoAddress, PHASE_MSGIN)) {
		return (rval);
	}

	/* wait for request to be asserted */
	if (rval = N5380GetPhase(baseIoAddress, &tmp)) {
		return (rval);
	}

	/* see if phase match */
	if (PHASE_MSGIN != tmp) {
		return (HOST_PHASE_ERR);
	}

	/* get the msg byte, throw it away */
	if (rval = N5380GetByte(baseIoAddress, TIMEOUT_REQUEST, &tmp)) {
		return (rval);
	}

	/* set the phase to NULL to up N5380 back to normal */
	if (rval = N5380SetPhase(baseIoAddress, PHASE_NULL)) {
		return (rval);
	}

#ifdef  TRANTOR_DEBUG
	if (trantor_debug & LOW_LEVEL)
		printf("ScsiGetStat: exit, pstatus = %xh\n", *pstatus);
#endif /* TRANTOR_DEBUG */
	/* if non-zero status byte, return error */
	if (*pstatus) {
		return (HOST_MSG_ERR);
	}
	return (HOST_CMD_OK);
}
