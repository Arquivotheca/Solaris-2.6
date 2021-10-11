/*
 * Copyright (c) 1992, 1993, 1994 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_TRAN_PORT_H
#define	_SYS_DKTP_TRAN_PORT_H

#pragma ident	"@(#)tran_port.h	1.2	94/05/05 SMI"

#ifdef __cplusplus
extern "C" {
#endif

#define	UCHAR		unsigned char
#define	BOOLEAN		short
#define	ULONG		unsigned long
#define	CONST		const
#define	FALSE   0
#define	TRUE    1
#define	ScsiPortStallExecution(x) drv_usecwait(x)
#define	HOST_CMD_OK	0x00    /* command success		*/
#define	HOST_NO_STATUS  0x00    /* no adapter status available  */
#define	HOST_CMD_ABORTED 0x04	/* command aborted by host	*/
#define	H_A_CMD_ABORTED	0x05	/* command aborted by hba	*/
#define	HOST_NO_FW	0x08	/* firmware not downloaded	*/
#define	HOST_NOT_INITTED 0x09   /* SCSI subsytem not initialized */
#define	HOST_BAD_TARGET	0x0A	/* target not assigned		*/
#define	HOST_SEL_TO	0x11	/* selection timeout		*/
#define	HOST_DU_DO	0x12	/* Data overrun or underrun	*/
#define	HOST_BUS_FREE   0x13	/* unexpected bus free		*/
#define	HOST_PHASE_ERR  0x14    /* target bus phase seq error	*/
#define	HOST_BAD_OPCODE 0x16	/* invalid operation code	*/
#define	HOST_BAD_PARAM  0x18    /* invalid control blk parameter */
#define	HOST_DUPLICATE  0x19    /* duplicate target ctl blk rec */
#define	HOST_BAD_SG_LIST 0x1A   /* invalid Scatter/Gather list	*/
#define	HOST_BAD_REQSENSE 0x1B  /* request sense command failed */
#define	HOST_HW_ERROR   0x20    /* Host Adapter hardware error	*/
#define	HOST_ATTN_FAILED 0x21   /* target didn't respond to attn */
#define	HOST_SCSI_RST1   0x22   /* SCSI bus reset by host adapter */
#define	HOST_SCSI_RST2   0x23   /* SCSI bus reset by other device */
#define	HOST_BUS_BUSY   0x24	/* unexpected bus busy		*/
#define	HOST_REQ_FAILED 0x25	/* bus req assert failed	*/
#define	HOST_DEREQ_FAILED 0x26	/* bus req desert failed	*/
#define	HOST_MSG_ERR    0x27    /* host get messags bytes error */
#define	PARALLEL_TO	0x28	/* Parallel port timeout	*/
#define	HOST_BAD_CHKSUM 0x80	/* program checksum failure	*/

#ifdef __cplusplus
}
#endif

#endif /* _SYS_DKTP_TRAN_PORT_H */
