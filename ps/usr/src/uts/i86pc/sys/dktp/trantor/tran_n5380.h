/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_TRAN_N5380_H
#define	_SYS_DKTP_TRAN_N5380_H

#pragma ident	"@(#)tran_n5380.h	1.3	94/08/15 SMI"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__)

/*
 *	FILE: tran_n5380.h
 *
 *	Trantor N5380 definitions
 */

/* scsi reset time in usec */
#define	SCSI_RESET_TIME 10

/* Define the scsi phases */

#define	PHASE_NULL		0
#define	PHASE_DATAOUT	0
#define	PHASE_DATAIN	1
#define	PHASE_COMMAND	2
#define	PHASE_STATUS	3
#define	PHASE_MSGOUT	6
#define	PHASE_MSGIN		7

/* Define n5380 registers */

#define	N5380_CURDATA			0
#define	N5380_OUTDATA			0
#define	N5380_INITIATOR_CMD		1
#define	N5380_MODE				2
#define	N5380_TARGET_CMD		3
#define	N5380_CURSTAT			4
#define	N5380_ENBSEL			4
#define	N5380_BUS_STAT			5
#define	N5380_DMA_SEND			5
#define	N5380_INPDATA			6
#define	N5380_RESET_PARITY		7
#define	N5380_INITIATOR_RECV	7
#define	N5380_EMR				7

/* Define 5380 register bit assignments */

/* Initiator Command */

#define	IC_RESET 0x80
#define	IC_AIP 0x40
#define	IC_LA 0x20
#define	IC_ACK 0x10
#define	IC_BUSY 0x8
#define	IC_SELECT 0x4
#define	IC_ATN	0x2
#define	IC_DATA 0x1

/* Mode Register */

#define	MR_BLOCK 0x80
#define	MR_TARGET 0x40
#define	MR_ENBPAR_CHK 0x20
#define	MR_ENBPAR_INT 0x10
#define	MR_ENBEOP_INT 0x8
#define	MR_MTRBUSY 0x4
#define	MR_DMA 0x2
#define	MR_ARB 0x1

/* Target Command Register */

#define	TC_LAST_BYTE 0x80
#define	TC_REQ 0x8
#define	TC_MSG 0x4
#define	TC_CD 0x2
#define	TC_IO 0x1

/* Current SCSI Bus Status */

#define	CS_RST 0x80
#define	CS_BUSY 0x40
#define	CS_REQ 0x20
#define	CS_MSG 0x10
#define	CS_CD 0x8
#define	CS_IO 0x4
#define	CS_SEL 0x2
#define	CS_DBP 0x1

/* Bus and Status Register */

#define	BS_DMAEND 0x80
#define	BS_DRQ 0x40
#define	BS_PARERR 0x20
#define	BS_IRQ 0x10
#define	BS_PHASEMATCH 0x8
#define	BS_BUSYERR 0x4
#define	BS_ATN 0x2
#define	BS_ACK 0x1

/* Public Routines Definitions */

#define	N5380EnableInterrupt(baseIoAddress) \
    N5380Set(baseIoAddress, N5380_MODE, MR_DMA)

/* Public Routines */
static BOOLEAN N5380Test(PBASE_REGISTER baseIoAddress, UCHAR regx, UCHAR mask);
static void N5380Clear(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte);
static void N5380Set(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte);
static void N5380DisableInterrupt(PBASE_REGISTER baseIoAddress);
static int N5380ToggleAck(PBASE_REGISTER baseIoAddress, ULONG usec);
static int N5380GetByte(PBASE_REGISTER baseIoAddress, ULONG usec, UCHAR *byte);
static int N5380PutByte(PBASE_REGISTER baseIoAddress, ULONG usec, UCHAR byte);
static int N5380GetPhase(PBASE_REGISTER baseIoAddress, UCHAR *phase);
static int N5380SetPhase(PBASE_REGISTER baseIoAddress, UCHAR phase);
static int N5380WaitNoRequest(PBASE_REGISTER baseIoAddress, ULONG usec);
static int N5380WaitRequest(PBASE_REGISTER baseIoAddress, ULONG usec);
static int N5380WaitNoBusy(PBASE_REGISTER baseIoAddress, ULONG usec);
static int N5380WaitBusy(PBASE_REGISTER baseIoAddress, ULONG usec);
static int N5380Select(PBASE_REGISTER baseIoAddress, UCHAR target, UCHAR lun);
static void N5380ResetBus(PBASE_REGISTER baseIoAddress);
static BOOLEAN N5380CheckAdapter(PBASE_REGISTER baseIoAddress);
#ifdef NOT_USED_IN_SOLARIS
static void N5380Get(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR *byte);
static void N5380Put(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte);
static BOOLEAN N5380Interrupt(PBASE_REGISTER baseIoAddress);
static void N5380DebugDump(PBASE_REGISTER baseIoAddress);
static void N5380EnableDmaWrite(PBASE_REGISTER baseIoAddress);
static void N5380EnableDmaRead(PBASE_REGISTER baseIoAddress);
static void N5380DisableDma(PBASE_REGISTER baseIoAddress);
#endif /* NOT_USED_IN_SOLARIS */
#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* _SYS_DKTP_TRAN_N5380_H */
