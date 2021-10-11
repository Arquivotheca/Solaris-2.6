/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_TRAN_PP_H
#define	_SYS_DKTP_TRAN_PP_H

#pragma ident	"@(#)tran_pp.h	1.2	94/05/05 SMI"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	tran_pp.h
 *
 *	Trantor parallel port definitions
 *
 */

/* parallel port defs */

/* p_s */

#define	P_BUSY 0x80
#define	P_ACK 0x40
#define	P_PE 0x20
#define	P_SELECT 0x10
#define	P_ERR 0x8

/* p_c */

#define	P_BUFEN 0xE0
#define	P_IRQEN 0x10
#define	P_SLC 0x8
#define	P_INIT 0x4
#define	P_AFX 0x2
#define	P_STB 0x1

/* parallel port registers */

#define	PARALLEL_DATA 0
#define	PARALLEL_STATUS 1
#define	PARALLEL_CONTROL 2

/* */
/* Public Functions */
/* */
#ifdef NO_NEED
void ParallelPut(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR byte);
void ParallelGet(PBASE_REGISTER baseIoAddress, UCHAR reg, UCHAR *byte);
int ParallelWaitBusy(PBASE_REGISTER baseIoAddress, ULONG usec, UCHAR *data);
#endif /* NO_NEED */

#ifdef __cplusplus
}
#endif

#endif /* _SYS_DKTP_TRAN_PP_H */
