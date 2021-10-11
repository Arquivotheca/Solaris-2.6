/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_TRAN_SCSI_H
#define	_SYS_DKTP_TRAN_SCSI_H

#pragma ident	"@(#)tran_scsi.h	1.4	94/11/17 SMI"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	tran_scsi.h
 *
 *	Trantor SCSI definitions
 *
 */

#include <sys/dktp/trantor/tran_n5380.h>

/* */
/* Public Functions */
/* */

static int ScsiWriteBytesSlow(PBASE_REGISTER baseIoAddress, UCHAR *pbytes,
    ULONG len, UCHAR phase);
static int ScsiReadBytesSlow(PBASE_REGISTER baseIoAddress, UCHAR *pbytes,
    ULONG len, UCHAR phase);
static int ScsiSendCommand(PBASE_REGISTER baseIoAddress, UCHAR target,
    UCHAR lun, UCHAR *pcmd, UCHAR cmdlen);
static int ScsiGetStat(PBASE_REGISTER baseIoAddress, UCHAR *pstatus);
static int ScsiDoIo(PBASE_REGISTER baseIoAddress, BOOLEAN dir,	UCHAR *pbfr,
    ULONG datalen);
static int ScsiFinishCommandInterrupt(PBASE_REGISTER baseIoAddress,
    UCHAR target, UCHAR lun, UCHAR *pcmd, UCHAR cmdlen, BOOLEAN dir,
    UCHAR *pbfr, ULONG datalen, UCHAR *pstatus);
#ifdef NOT_USED_IN_SOLARIS
static int ScsiStartCommandInterrupt(PBASE_REGISTER baseIoAddress, UCHAR target,
    UCHAR lun, UCHAR *pcmd, UCHAR cmdlen, BOOLEAN dir,
    UCHAR *pbfr, ULONG datalen, UCHAR *pstatus);
static int ScsiDoCommand(PBASE_REGISTER baseIoAddress, UCHAR target, UCHAR lun,
    UCHAR *pcmd, UCHAR cmdlen, BOOLEAN dir, UCHAR *pbfr, ULONG datalen,
    UCHAR *pstatus);
#endif /* NOT_USED_IN_SOLARIS */

#ifdef __cplusplus
}
#endif

#endif /* _SYS_DKTP_TRAN_SCSI_H */
