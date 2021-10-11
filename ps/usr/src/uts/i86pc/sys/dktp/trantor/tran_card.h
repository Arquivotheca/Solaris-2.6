/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_TRAN_CARD_H
#define	_SYS_DKTP_TRAN_CARD_H

#pragma ident	"@(#)tran_card.h	1.4	94/11/17 SMI"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	tran_card.h
 *
 *	T348 Adapter Definitions File
 *
 */

/* all 5380 type cards use the scsifnc module */
#include <sys/dktp/trantor/tran_scsi.h>
#include <sys/dktp/trantor/tran_pp.h>
#include <sys/dktp/trantor/tran_p3c.h>

/* */
/* Definitions */
/* */

/* the maximum transfer size */
/* by decreasing this we can get better system performace since */
/* the data transfer occurs with interrupts disabled, this might be */
/* decreased for our smaller cards */

#define	CARD_MAX_TRANSFER_SIZE (2*1024)


/* the t348 is IO mapped */

#define	CARD_ADDRESS_RANGE_IN_IOSPACE TRUE

/* we use 3	addresses in IO space */

#define	CARD_ADDRESS_RANGE_LENGTH 3


/* the t348 does not use interrupts */

#define	CARD_SUPPORT_INTERRUPTS FALSE

/* for now, must choose an interupt that doesn't conflict */
/* microsoft: jeff said later they will have a method */

#define	CARD_DEFAULT_INTERRUPT_LEVEL 15


/* */
/* Redefined routines */
/* */
/* These are card specific routines, but since this card has a 5380, we */
/* will redefine these to the generic n5380 or other routines. */
/* */

#define	CardDoCommand CardStartCommandInterrupt
#define	CardWriteBytesCommand ScsiWriteBytesSlow

/* the following routines are not used when interrupts are not used */

#define	CardFinishCommandInterrupt ScsiFinishCommandInterrupt
#define	CardInterrupt(x) FALSE

/* */
/* Routines in card.c */
/* */

static BOOLEAN CardCheckAdapter(PBASE_REGISTER baseIoAddress, int quick);
static void CardDisableInterrupt(PBASE_REGISTER baseIoAddress);
static int CardStartCommandInterrupt(PBASE_REGISTER baseIoAddress, UCHAR target,
    UCHAR lun, UCHAR *pcmd, UCHAR cmdlen, BOOLEAN dir,
    UCHAR *pbfr, ULONG datalen, UCHAR *pstatus);
static void CardResetBus(PBASE_REGISTER baseIoAddress);
static void CardN5380Put(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte);
static void CardN5380Get(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR *byte);
static int CardWriteBytesFast(PBASE_REGISTER baseIoAddress, UCHAR *pbytes,
    ULONG len, UCHAR phase);
static int CardReadBytesFast(PBASE_REGISTER baseIoAddress, UCHAR *pbytes,
    ULONG len, UCHAR phase);
static void CardResetBus(PBASE_REGISTER baseIoAddress);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_DKTP_TRAN_CARD_H */
