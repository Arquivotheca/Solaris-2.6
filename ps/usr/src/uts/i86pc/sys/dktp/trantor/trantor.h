/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 */

#ifndef _SYS_DKTP_TRANTOR_H
#define	_SYS_DKTP_TRANTOR_H

#pragma ident	"@(#)trantor.h	1.6	95/02/16 SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/scsi/scsi.h>
#include <sys/dktp/hba.h>
#define	UCHAR	unsigned char
#define	BOOLEAN	short
#define	ULONG	unsigned long
#define	CONST	const
#define	FALSE   0
#define	TRUE    1
#define	UNDEFINED	-1
#define	ScsiPortStallExecution(x) drv_usecwait(x)

#define	TRANTOR_MAX_DMA_SEGS 1	/* max number of s/g elements 		*/

#define	TRANTOR_SETGEOM(hd, sec) (((hd) << 16) | (sec))

struct trantor_ccb {			/* command control block	*/
	caddr_t		ccb_datap;	/* data or s/g list pointer	*/
	ulong		ccb_datalen;	/* data or s/g list length	*/
	u_char 		ccb_target;	/* scsi target id (to ATTN reg)	*/
	u_char 		ccb_lun;
	unchar		ccb_cdblen;	/* length in bytes of SCSI CDB	*/
	unchar		ccb_dataout;	/* 0 for read, 1 otherwise	*/
	ushort		ccb_status;
	struct scsi_pkt *ccb_ownerp;	/* pointer back to packet	*/
	struct trantor_ccb	*ccb_forw;
};

struct  trantor_blk {
	kmutex_t 		tb_mutex;
	dev_info_t 		*tb_dip;
	unchar			tb_targetid;
	char			tb_unplugged;
	char			tb_cbexit;
	ushort			tb_ioaddr;
	struct	trantor_ccb 	*tb_ccboutp;
	struct  trantor_ccb	*tb_last;
	ushort  		tb_child;
	kthread_t		*tb_cbth;	/* Callback thread */
	kcondvar_t		tb_scv;		/* Callback semaphore */
	kcondvar_t		tb_dcv;		/* Thread destroy semaphore */
	struct trantor_ccb	*tb_cbccb;	/* List of pending callbacks */
};

/*
 * Trantor requires DMA limits structure per target to record device
 * sector sizes.
 */
struct trantor_unit {
	ddi_dma_lim_t	tu_lim;
};

struct trantor {
	scsi_hba_tran_t			*t_tran;
	struct trantor_blk		*t_blkp;
	struct trantor_unit		*t_unitp;
};

#define	TRAN2HBA(hba)		((struct trantor *)(hba)->tran_hba_private)
#define	SDEV2HBA(sd)		(TRAN2HBA(SDEV2TRAN(sd)))
#define	TRAN2TRANTOR(hba)	((struct trantor *)(hba)->tran_tgt_private)
#define	TRAN2TRANTORBLKP(hba)	((TRAN2TRANTOR(hba))->t_blkp)
#define	TRAN2TRANTORUNITP(hba)	((TRAN2TRANTOR(hba))->t_unitp)
#define	SDEV2TRANTOR(sd)	(TRAN2TRANTOR(SDEV2TRAN(sd)))
#define	PKT2TRANTORUNITP(pktp)	(TRAN2TRANTORUNITP(PKT2TRAN(pktp)))
#define	PKT2TRANTORBLKP(pktp)	(TRAN2TRANTORBLKP(PKT2TRAN(pktp)))
#define	ADDR2TRANTORUNITP(ap)	(TRAN2TRANTORUNITP(ADDR2TRAN(ap)))
#define	ADDR2TRANTORBLKP(ap)	(TRAN2TRANTORBLKP(ADDR2TRAN(ap)))

#define	TRANTOR_BLKP(X) (((struct trantor *)(X))->t_blkp)

/*
 * Number of microseconds to wait for request to come back from target
 * in normal situations.
 */
#define	TIMEOUT_REQUEST 2000000

/*
 * Number of microseconds to wait for request to come back from target
 * during "fast" transfers.  This time is long because Trantor does not
 * permit disconnect and devices might pause for buffering etc.
 */
#define	TIMEOUT_FASTDATA 5000000

/* wait upto 1 sec for busy to disappear from scsi bus */
#define	TIMEOUT_BUSY 1000000

/* wait upto 250 msec for target to be selected */
#define	TIMEOUT_SELECT 250000

/* */
/* SCSI Bus id where our initiator is at */
/* */
#define	HOST_ID 7

/* a generic pointer for card registers */
typedef unsigned int PBASE_REGISTER;

#ifdef	__cplusplus
}
#endif

#endif /* _SYS_DKTP_TRANTOR_H */
