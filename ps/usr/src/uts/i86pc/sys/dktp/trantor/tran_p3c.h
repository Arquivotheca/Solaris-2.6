/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_TRAN_P3C_H
#define	_SYS_DKTP_TRAN_P3C_H

#pragma ident	"@(#)tran_p3c.h	1.4	94/11/17 SMI"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	tran_p3c.h
 *
 *	Trantor P3C definitions
 *
 */

#include <sys/dktp/trantor/tran_port.h>
#include <sys/dktp/trantor/tran_pp.h>

/* p3c control */

#define	PC_RES 0x80
#define	PC_MODE 0x70
#define	PC_ADRS 0x0f

/* p3c modes */

#define	PCCC_MODE_RPER_BYTE		0
#define	PCCC_MODE_RPER_NIBBLE 	0x10
#define	PCCC_MODE_RDMA_BYTE		0x20
#define	PCCC_MODE_RDMA_NIBBLE 	0x30
#define	PCCC_MODE_WPER			0x40
#define	PCCC_MODE_RSIG_BYTE		0x50
#define	PCCC_MODE_WDMA			0x60
#define	PCCC_MODE_RSIG_NIBBLE 	0x70


/* */
/* Public Functions */
/* */

static void P3CPutControl(PBASE_REGISTER baseIoAddress, UCHAR mode, UCHAR reg);
static void P3CSetPrinterMode(PBASE_REGISTER baseIoAddress, UCHAR data,
	UCHAR control);
static void P3CSetScsiMode(PBASE_REGISTER baseIoAddress, UCHAR *data,
	UCHAR *control);
static BOOLEAN P3CCheckAdapter(PBASE_REGISTER baseIoAddress);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_DKTP_TRAN_P3C_H */
