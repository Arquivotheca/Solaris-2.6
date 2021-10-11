/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef _CORVETTE_SCB_H
#define _CORVETTE_SCB_H

#pragma	ident	"@(#)scb.h	1.1	95/01/28 SMI"

/*
 * scb.h: contains the SCB Locate Mode structures
 */

#pragma pack(1)

/*
 * SCB TSB (Termination Status Block) structure
 */

typedef struct  corv_tsb {      	/* termination status block	*/

        ushort          t_endstatus;    /* end status word              */
        ushort          t_rtrycnt;      /* retry count                 	*/
        long            t_resid;        /* residual byte count          */
        paddr_t         t_sg_addr; 	/* scatter gather list addr     */
        ushort          t_add_sts_len;  /* target status length = 0x000c*/

	/* SCSI status byte */
        unchar        	t_vres2    :1; 	/* vendor unique bit            */
        unchar        	t_scsistat :4; 	/* target status code           */
        unchar        	t_vres1    :2; 	/* vendor unique bit            */
        unchar        	t_resv1    :1; 	/* reserved			*/

	/* Command status */
        unchar          t_cmdstatus;    /* hba status                   */

	/* Device error code */
        unchar          t_deverror;     /* target error code            */

	/* Command error code */
        unchar          t_cmderror;     /* hba error code               */

        ushort          t_diag_err_mod; /* diagnostic error modifier    */
        unchar          t_resv2;        /* cache hit ratio(=0x00)	*/
        unchar          t_resv3;        /* cache status(=0xc0)		*/
        paddr_t         t_ele_addr;     /* last processed scb addr      */

} CORV_TSB;

/*	
 *      SCB End Status Word
 *	Values for t_endstatus - bits 0-7
 */
#define	SCB_NO_ERR	0x0001		/* SCB ended (no error)		*/
#define	SCB_SHORT_EXP	0x0020		/* short record exception	*/
#define	INVALID_CMD	0x0040		/* invalid command rejected 	*/
#define	SCB_REJECTED	0x0080		/* SCB rejected			*/
#define	SCB_SPEC_CHK	0x0010		/* SCB specification check	*/
#define	RESERVED0	0x0020
#define	SCB_HALTED	0x0040		/* SCB halted			*/
#define	SCB_INTR_QED	0x0080		/* SCB interrupt queued		*/

/*
 *      SCB End Status Word
 *	Values for t_endstatus - bits 8-15
 */
#define	ADD_STS_AVAIL	0x0100		/* additional status available  */
#define	DD_STS_AVAIL	0x0200		/* device depended status available */
#define	RESERVED1	0x0400
#define	DEV_NOT_INIT	0x0800		/* device not initialized 	*/
#define	MAJOR_EXCEPTION	0x1000		/* major exception occured 	*/

/*
 *	Values for t_rtrycnt 
 */
#define	SYS_ICHK_RETRY	0x0020		/* system interface check retry	*/
#define	ARQ_DTA_VALID	0x0040		/* automatic request sense data valid */
#define	ADP_RETRY_DIS	0x0080		/* adapter retries disabled 	*/
#define	SCSI_MSG_ERR	0x1000		/* SCSI Tag/Sync/Wide message 	*/
					/* induced error 		*/
#define	DEV_CMD_Q_SUSP	0x2000		/* device command q suspended	*/
#define	ADP_ERR_LOG_NE	0x4000		/* adapter error log not empty	*/
#define	ADP_RETRY_INVOK	0x8000		/* adpater retry invoked	*/

/*
 *	Values for t_scsistatus  4 bits SCSI status
 */
#define	GOOD_CONDITION	0x00	/* good condition (no error)		*/
#define	CHK_CONDITION	0x02	/* check condition (error)		*/	
#define	CONDITION_MET	0x04	/* check condition (error)		*/	
#define	BUSY		0x08	/* busy (command rejected 		*/

/* Linked Commands (Intermediate)					*/

#define	INTER_GOOD	0x8	/* good (no error)			*/
#define	INTER_COND_MET	0xa	/* condition met/good (no error)	*/
#define	RESV_CONFLICT	0xc	/* reservation conflict			*/

/*
 *	Values for t_cmdstatus 1byte command status
 */
#define	CMD_COMP_SUCC	0x01	/* SCB command completed with success	*/
#define	CMD_COMP_RTRY	0x05	/* command completed with success w/retries */	
#define	ADP_HW_FAILURE	0x07	/* adapter hardware failure		*/
#define	IMD_CMD_COMP	0x0a	/* immediate command complete		*/
#define	CMD_ERROR	0x0e	/* command error (invalid cmd or parameter */
#define	SW_SEQ_ERROR	0x0f	/* software sequencing error		*/

/*
 * 	device error code
 *	Values for t_deverror 
 */

#define	NO_ERROR	0x00	/* no error 				*/
#define	SCSI_BUS_RESET	0x01	/* SCSI bus reset occured or SCSI bus reset */
				/* required error still pending		*/
#define	SCSI_INTER_FLT	0x02	/* SCSI interface fault			*/	
#define	WRITE_ERROR	0x03	/* SCSI device posted write error 	*/
				/* (deferred error )			*/
#define	INT_PWR_FAILURE	0x04	/* internal SCSI bus terminator power	*/
				/* failure				*/
#define EXT_PWR_FAILURE	0x05	/* external SCSI bud terminator power	*/
				/* failure				*/
#define	INT_SNS_ERR	0x06	/* internal SCSI bus differential sense error */
#define	EXT_SNS_ERR	0x07	/* external SCSI bus differential sense error */
#define	SCSI_SEL_TO	0x10	/* SCSI selection time out 		*/
				/* (device not available)		*/
#define	BUS_FREE	0x11	/* unexpected SCSI bus free		*/
#define	MSG_REJECTED	0x12	/* mandatory SCSI message rejected	*/
#define	INVLD_SCSI_PHSE	0x13	/* invalid SCSI phase sequence		*/
#define	INT_BUS_RESET	0x14	/* internal SCSI bus reset required	*/
#define	EXT_BUS_RESET	0x15	/* external SCSI bus reset required	*/
#define	SRT_LTH_RCD_ERR	0x20	/* short length record error		*/

/* following are error codes when operating in target mode		*/

/* #define BUS_RESET_MSG	0x40 /* bus reset msg received 		*/
/* #define ABORT_MSG		0x41 /* abort msg received 		*/
/* #define TGT_CMD_OVRRUN	0x42 /* target command overrun 		*/
/* #define DTA_ALLOC_ERR	0x43 /* a data allocation error occurred*/
/* #define INITIATOR_ERR	0x44 /* a SCSI initiator error was detected*/
/* #define INITIATOR_FLT	0x45 /* a SCSI initiator fault occured	*/

/* assign rejection error codes 					*/

#define	ASGN_REJECTED_0	0x08	/* cmd on progress on device		*/
#define	ASGN_REJECTED_1	0x09	/* SCSI device already assigned		*/
#define	ASGN_REJECTED_2	0x12	/* SCSI bus number not supported	*/

#define	MGMT_PIPE_REJ	0x0d	/* suspend/resume delivery pipes 	*/
				/* request rejected - cmd in progress	*/
#define	INVALID_DEV	0x13	/* invalid device for command		*/
#define	CHKSUM_HDR_ERR	0x14	/* checksum hdr err in EPROM data	*/
#define	MEM_ERASE_ERR_E	0x15	/* memory erasure error - even byte	*/
#define MEM_ERASE_ERR_O	0x16	/* memory erasure error - odd byte	*/
#define	MEM_VFY_ERR_E	0x17	/* memory verify error - even byte	*/
#define MEM_VFY_ERR_O	0x18	/* memory verify error - odd byte	*/

/* download/microcode rejection error codes				*/

#define	DNLD_MC_REJ_0	0x19	/* sequence error (requires prepare)	*/
#define	DNLD_MC_REJ_1	0x1a	/* other commads in progress		*/
#define	INT_MC_ERR	0x80	/* internal microcode detected error	*/

#define	ADP_HW_ERR	0x20	/* adapter hardware error		*/
#define	ADP_GLOB_TO	0x21	/* adp detected global cmd time out	*/
#define	DMA_ERROR	0x22	/* dma error				*/

/* target mode error codes						*/

#define	TGT_ASGN_ERR	0x40	/* target assign error 			*/
#define WRONG_CMD	0x41	/* wrong cmd sent by host for cur mode	*/
#define	DTA_ENB_ERR	0x42	/* data enable SCB error 		*/
#define	ENB_END_ERR	0x43	/* enable/end SCB error 		*/
#define	TM_REQ_REJECTED	0x44	/* TM req rej - sent to a non-tgt entity*/

/* establish buffer pool request specific error codes			*/

#define	MORE_BUFS_EPB	0x45	/* more than 10 buffers 		*/
#define	MORE_EBP_REQS	0x46	/* more than 32 request active		*/
#define	MIN_BUF_SIZ	0x47	/* buffer size < 64 bytes 		*/

/* suspend request specific error codes					*/

#define	SUS_REQ_REJ_0	0x48	/* no disconnect allowed by initiator	*/
#define	SUS_REQ_REJ_1	0x49	/* target entity already suspended	*/
#define	RES_REQ_REJ	0x4a	/* resume request rejected -		*/


/* 
 * scatter/gather DMA segment descriptor 
 */

typedef struct corv_dma_segment {

        ulong   data_ptr;     		/* segment address		*/
        ulong   data_len;     		/* segment length		*/

} SG_SEGMENT ;


/*      
 * values for ccb_opcode - SCB commands
 */

#define ADP_CMD_RESET     0x00    /* scsi (soft) reset                  */
#define ADP_CMD_READ      0x01    /* read data                          */
#define ADP_CMD_WRITE     0x02    /* write data                         */
#define ADP_CMD_READVFY   0x03    /* read verify                        */
#define ADP_CMD_WRITEVFY  0x04    /* write verify                       */
#define ADP_CMD_ICMD      0x04    /* supplement code                    */
#define ADP_CMD_CMDSTAT   0x07    /* get command complete status        */
#define ADP_CMD_REQSEN    0x08    /* request sense                      */
#define ADP_CMD_READCAP   0x09    /* read capacity                      */
#define ADP_CMD_GETPOS    0x0a    /* get pos and adapter information    */
#define ADP_CMD_DEVINQ    0x0b    /* device inquiry                     */
#define ADP_CMD_HBACTRL   0x0c    /* feature control                    */
#define ADP_CMD_DMACTRL   0x0d    /* dma pacing control                 */
#define ADP_CMD_ASSIGN    0x0e    /* logical device assignment          */
#define ADP_CMD_ABORT     0x0f    /* command abort                      */
#define ADP_CMD_FMTUNIT   0x16    /* format unit                        */
#define ADP_CMD_FMTPREP   0x17    /* format prepare                     */
#define ADP_CMD_REASSIGN  0x18    /* reassign block                     */
#define ADP_CMD_SCB       0x1c    /* start ccb                          */
#define ADP_CMD_SNDSCSI   0x1f    /* send scsi command                  */
#define ADP_CMD_LSCB      0x24    /* start long ccb                     */
#define ADP_CMD_PREREAD   0x31    /* read prefetch                      */



/*      values for tsbp->t_cmderror                                     */

#define	CMD_NO_ERROR		0x00	/* no error			*/
#define	CORV_CMDERR_BADCCB      0x01    /* invalid parameter in SCB	*/
#define	CORV_CMDERR_BADCMD      0x03    /* command not supported        */
#define	CORV_CMDERR_SYSABORT    0x04    /* host aborted cmd             */
#define	FMT_REJECTED		0x07	/* format rejected - sequence error*/
#define	CORV_CMDERR_BADLD       0x0a    /* logical device not mapped    */
#define	CORV_CMDERR_END         0x0b    /* max block addr exceeded      */
#define	CORV_CMDERR_END16       0x0c    /* max block addr < 16 bit card */
#define	CMD_REJECTED_4		0x0f	/* suspended SCSI Qs & adapter Q full*/
#define	CORV_CMDERR_BADDEV      0x13    /* invalid device for command   */
#define	CORV_CMDERR_TIMEOUT     0x21    /* timeout                      */
#define	CORV_CMDERR_DMA         0x22    /* hba dma err                  */
#define	CORV_CMDERR_BUF         0x23    /* hba buffer err               */
#define	CORV_CMDERR_ABORT       0x24    /* hba aborted cmd              */
#define	CORV_CMDERR_ERR         0x80    /* hba microprocessor error     */


#endif	/* _CORVETTE_SCB_H */
