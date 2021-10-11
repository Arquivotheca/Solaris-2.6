/*
 * Copyright (c) 1995, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ifndef	_CORVETTE_CORV_H
#define	_CORVETTE_CORV_H

#pragma	ident	"@(#)corv.h	1.3	95/02/08 SMI"

#include "scb.h"
#include "pos.h"
#include "delivery.h"
#include "move.h"
#include "xmca.h"

#ifdef  __cplusplus
 extern "C" {
#endif

#define CORV_KVTOP(vaddr) (HBA_KVTOP((vaddr), PAGESHIFT, PAGEOFFSET))

#define	CORV_MCA_POSID		0x8efc	/* Micro Channel POS ID */
#define CORV_MAX_DMA_SEGS       17      /* max scatter/gather segments  */
#define CORV_MAX_EID            255	/* max entity ID supported	*/
#define	CORV_MAX_PHYSDEV	960
#define MAX_BRDS		8	/* maximum boards 		*/
#define NO_OF_TARGETS		30	/* maximum targets supported 	*/
#define NO_OF_LUNS		32	/* maximum luns/target 		*/
#define	CORV_SENSE_LEN		14	/* maximum # of sense bytes */

#define REQUEST_SENSE		0x03
#define	DEV_INQUIRY		0x12


#define CORV_CMD_port(X) 	((X))
#define CORV_CMD        	0x0
#define CORV_ATTN       	0x4
#define CORV_BCR        	0x5
#define CORV_ISR        	0x6
#define CORV_BSR        	0x7

/*      attention register                                              */
#define START_IMMEDIATE_CMD     0x10    /* start immediate command      */
#define START_SCB_CMD           0x30    /* start ccb                    */
#define START_LONG_SCB_CMD      0x40    /* start long ccb               */
#define	RESUME_DATA_TRAN	0x60	/* Resume data transfer         */
#define	MANAGEMENT_REQUEST	0x90	/* Management Request		*/
#define	MOVE_MODE_SIGNAL	0xd0	/*				*/
#define EOI                     0xe0    /* end of interrupt             */

/*      basic control register                                          */
#define BCR_SYS_RESET      	0x80    /* hardware reset		*/
#define BCR_CLR_ON_READ    	0x40    /* enable interrupt		*/
#define	BCR_RESET_EXECP_COND 	0x20  	/* reset on exception condition	*/
#define BCR_DMA_ENABLE     	0x02    /* dma enable			*/
#define BCR_INTR_ENABLE    	0x01    /* enable interrupt		*/

/*      basic status register                                           */
#define	BSR_EXCPN_CONDN		0x10	/* Exception condition		*/
#define	BSR_EXCPN_MASK		0xe0	/* exception code		*/
#define BSR_CMDFULL		0x8     /* command interface reg full	*/
#define BSR_CMDEMPTY    	0x4     /* command interface reg empty	*/
#define BSR_INTR_REQ		0x2     /* interrupt request active	*/
#define BSR_BUSY    		0x1     /* busy				*/

/*      interrupt id                                                    */
#define INTR_SCB_OK   		0x1     /* ccb completed successfully	*/
#define INTR_SCB_OK2  		0x5     /* ccb completed ok after retry	*/
#define INTR_HBA_FAIL 		0x7     /* hba hw failed		*/
#define INTR_ICMD_OK  		0xa     /* immediate cmd completed ok	*/
#define INTR_CMD_FAIL 		0xc     /* cmd completed with failure	*/
#define INTR_CMD_INV  		0xe     /* invalid command		*/
#define INTR_SEQ_ERR  		0xf     /* sw sequencing error		*/
#define INTR_UNKNOWN_ERR 	0x2  	/* for string conversion	*/

#define HBA_DEVICE      	0x0f	/* hba device			*/
#define SYSTEM_UNIT 		0x00	/* system unit			*/

/*      defines for cp_asgn_sts flag                                      */
#define ASSIGNED      		0x1  /* own's a logical device       */
#define WAITING     		0x2  /* waiting for a ld assignment  */

#define ACTIVE      		0x3  /* own's a logical device       */
#define INACTIVE     		0x4  /* waiting for a ld assignment  */

/*      defines for ld_status flag                                      */
#define CORV_OWNLD      	0x0001	/* own's a logical device       */
#define CORV_WAITLD     	0x0002	/* waiting for a ld assignment  */
#define CORV_INITLD     	0x0008	/* ld assignment valid at init  */


/*
 * Low level macros for the device control registers
 */

#define CORV_SENDEOI(X,LD)      \
        (outb((X)+CORV_ATTN, EOI | (LD)))


/* wait upto 30 seconds for status change */
#define	CORV_WAIT(X, P, M, ON, OFF)	\
	corv_wait((X), 30000000, (P), (M), (ON), (OFF))

/* wait upto 6 seconds for status change */
#define	CORV_QUICK_WAIT(X, P, M, ON, OFF)	\
	corv_wait((X), 6000000, (P), (M), (ON), (OFF))


/* wait for the busy bit to go off */
#define CORV_QBUSYWAIT(X, P)	\
	CORV_QUICK_WAIT((X), (P), BSR_BUSY, 0, BSR_BUSY)
#define CORV_BUSYWAIT(X, P)	\
	CORV_WAIT((X), (P), BSR_BUSY, 0, BSR_BUSY)


/* wait for the interrupt pending bit to go on */
#define CORV_QINTRWAIT(X, P)	\
	CORV_QUICK_WAIT((X), (P), BSR_INTR_REQ, BSR_INTR_REQ, 0)
#define CORV_INTRWAIT(X, P)        \
        CORV_WAIT((X), (P), BSR_INTR_REQ, BSR_INTR_REQ, 0)


/* wait for the command to be accepted */
#define CORV_CMDOUTWAIT(X, P)      \
        CORV_WAIT((X), (P), BSR_CMDEMPTY|BSR_BUSY, BSR_CMDEMPTY, BSR_BUSY)




#define CORV_DISABLE_INTR(blkp)	\
	outb((blkp->cb_ioaddr + CORV_BCR), (inb(blkp->cb_ioaddr+CORV_BCR) & ~BCR_INTR_ENABLE))
	
#define CORV_ENABLE_INTR(blkp)	\
	outb((blkp->cb_ioaddr + CORV_BCR), (inb(blkp->cb_ioaddr+CORV_BCR) | (BCR_INTR_ENABLE)))
	


#pragma pack(1)

struct  corv_bcr {
        unsigned        rc_eintr   : 1;	/* enable interrupt             */
        unsigned        rc_edma    : 1; /* enable dma                   */
        unsigned        rc_resv    : 3; /* reserved                     */
	unsigned	rc_rst_exp : 1;	/* reset exception condition(WO)*/
	unsigned	rc_cor	   : 1;	/* clear on read		*/
        unsigned        rc_reset   : 1; /* hw reset                     */
};

struct  corv_isr {
        unsigned        ri_ldevid : 4;   /* logical device id         */
        unsigned        ri_code   : 4;   /* interrupt id code         */
};

struct corv_bsr {
        unsigned        rs_busy     : 1;/* busy                       */
        unsigned        rs_intrhere : 1;/* interrupt request active   */
        unsigned        rs_cmd0     : 1;/* command interface reg empty  */
        unsigned        rs_cmdfull  : 1;/* command interface reg full */
        unsigned        rs_exp_cond : 1;/* exception condition          */
        unsigned        rs_exp_sts  : 3;/* exception status bits        */
};

#define CORV_bcrp(X)  	((struct corv_bcr *)(X))
#define CORV_intrp(X)  	((struct corv_isr *)(X))
#define CORV_bsrp(X)  	((struct corv_bsr *)(X))

/*
 * corv_unit - one per (target, lun) or Entity ID.
 */

typedef struct corv_unit {
	int		cu_refcnt;	/* # of linked corv_info	*/
	unchar		cu_suspended;	/* device queue is suspended	*/
	unchar 		cu_arq;		/* is ARQ supported ? 		*/
	unchar 		cu_tagque;	/* is tagged qing supported ? 	*/
	unchar 		cu_resv;	/* features in future !! 	*/
	unsigned long	cu_tot_sects;	/* total sectors supported 	*/
	EID_MGMT_REQ	cu_eidreq;	/* Assign Entity ID response	*/
	ddi_dma_lim_t 	cu_dmalim;	/* dma limit for this device 	*/
} CORV_UNIT;


/*
 * corv_info - Driver main structure one per target driver instance 
 */

typedef struct corv_info {
	scsi_hba_tran_t		*c_tranp;
	struct corv_blk		*c_blkp;
	struct corv_unit	*c_unitp;
	struct corv_scsi_bus	*c_sbp;
	unchar			 c_entity_id;
} CORV_INFO;


/*
 * corv_blk - corvette block information one per board 
 */ 

typedef struct corv_blk {
	struct corv_ccb	*cb_doneq;	/* queue of completed commands	*/
	struct corv_ccb	**cb_donetail;	/* done-queue tail ptr		*/
	kmutex_t	cb_mutex; 	/* protects this structure 	*/	
	dev_info_t 	*cb_dip;	/* device information pointer 	*/
	void		*cb_iblock;	/* arg for DDI intr functions	*/

	ushort		cb_ioaddr;	/* iobase start address 	*/
	unchar		cb_mca_slot;	/* MCA physical slot		*/
	unchar		cb_irq;		/* IRQ level used by adapter	*/

	unchar		cb_intrx;	/* interrupts property index	*/
	unchar		cb_itargetid;	/* HBA target-id, internal bus	*/
	unchar		cb_xtargetid;	/* HBA target-id, external bus	*/
        unchar          cb_intr_code;	/* interrupt code in isr	*/ 

        unchar          cb_intr_dev; 	/* interrupted device no isr	*/
	unchar		cb_exp_cond;	/* exception condition in bsr	*/
	unchar		cb_exp_sts;	/* exception status bits in bsr	*/
	unchar		cb_unused;

	ushort		cb_num_of_cmds_qd;
	PIPEDS		cb_pipep;  	/* pipe info structure 	*/

	CTRLAREA	*cb_adp_sca; 	
	CTRLAREA	*cb_sys_sca;
	caddr_t		cb_ib_pipep;
	caddr_t		cb_ob_pipep;

	CORV_UNIT	*cb_eid_map[CORV_MAX_EID];
} CORV_BLK;

/*
 * The Corvette cards support two busses, an internal bus and an
 * external bus. In this implementation there is a separate dev_info
 * node for each bus. The reg property for each dev_info node specifies
 * the boards base I/O address and the bus number. Both dev_info nodes
 * must point to the shared CORV_BLK structure which contains the state 
 * information for the adapter card.  The following structure is used to 
 * tie together the dev_info node and the CORV_BLK structures.  
 */

typedef struct corv_scsi_bus {
	ushort		 sb_ioaddr;
	ushort		 sb_bus;
	CORV_BLK	*sb_blkp;
} CORV_BUS;

#define	CORV_IOADDR_MASK	((ushort)0xfff8)
#define CORV_INTBUS		((ushort)0)
#define CORV_EXTBUS		((ushort)1)
#define CORV_MAXBUS		CORV_EXTBUS

#define	CORV_IOADDR(ioaddr)	(((ushort)(ioaddr)) & CORV_IOADDR_MASK)
#define CORV_BUS_NUM(ioaddr)	(((ushort)(ioaddr)) & ~CORV_IOADDR_MASK)
 
/*
 * Generic control element     
 */ 

#define	MAXSCBSIZE	196

typedef struct  _element { 

   ELE_HDR hdr; 
   unchar   Body[MAXSCBSIZE-sizeof(ELE_HDR)]; 

} ELEMENT; 


/*
 * SCB Command Control Block (CCB) structure
 */

typedef struct corv_ccb {
	struct corv_ccb	*ccb_linkp;		/* forward link */
	CORV_INFO	*ccb_corvp;		/* ptr to device status */
	struct scsi_cmd	*ccb_ownerp;   		/* pkt/cmd pointer */
        unchar           ccb_cdb[12];		/* scsi cmd desc. block	*/
	caddr_t		 ccb_baddr;		/* virtual buf addr */
        SG_SEGMENT       ccb_sg_list[MAX_SG_SEGS]; /* SG segs*/
        paddr_t 	 ccb_paddr;		/* physical address */

	ELEMENT		 ccb_ele; 
	ushort		 ccb_blk_cnt;
	struct scsi_arq_status	ccb_sense;
} CORV_CCB;


typedef struct corv_capacity {
	unsigned long cc_last_block;
	unsigned long cc_block_len;
} CORV_CAPACITY;


/* my macros */
#define	CORV_DIP(C)		(((C)->c_blkp)->cb_dip)

#define	TRAN2CORVINFO(tranp)	((CORV_INFO *)(tranp)->tran_tgt_private)
#define	TRAN2CORVBUSP(tranp)	((CORV_BUS *)(tranp)->tran_hba_private)
#define	TRAN2CORVBLKP(tranp)	(TRAN2CORVBUSP(tranp)->sb_blkp)
#define	TRAN2CORVUNITP(tranp)	(TRAN2CORVINFO(tranp)->c_unitp)
#define	SDEV2CORV(sd)		(TRAN2CORVINFO(SDEV2TRAN(sd)))
#define	PKT2CORVUNITP(pktp)	(TRAN2CORVUNITP(PKT2TRAN(pktp)))
#define	ADDR2CORVUNITP(ap)	(TRAN2CORVUNITP(ADDR2TRAN(ap)))
#define	ADDR2CORVBLKP(ap)	(TRAN2CORVBLKP(ADDR2TRAN(ap)))
#define	ADDR2CORVINFOP(ap)	(TRAN2CORVINFO(ADDR2TRAN(ap)))

#define	PKT2CORVINFO(pktp)	(TRAN2CORVINFO(PKT2TRAN(pktp)))

#define	CORV_BLKP(X) (((CORV_INFO *)(X))->c_blkp)

#define	CCBP2PKTP(ccbp)		(&(ccbp)->ccb_ownerp->cmd_pkt)

/* handy macros */

/*      values for tsbp->t_haerr                                       	 */
#define CORV_CMDERR_BADCCB       0x01    /* hba rejected ccb             */
#define CORV_CMDERR_BADCMD       0x03    /* command not supported        */
#define CORV_CMDERR_SYSABORT     0x04    /* host aborted cmd             */
#define CORV_CMDERR_BADLD        0x0a    /* logical device not mapped    */
#define CORV_CMDERR_END          0x0b    /* max block addr exceeded      */
#define CORV_CMDERR_END16        0x0c    /* max block addr < 16 bit card */
#define CORV_CMDERR_BADDEV       0x13    /* invalid device for command   */
#define CORV_CMDERR_TIMEOUT      0x21    /* timeout                      */
#define CORV_CMDERR_DMA          0x22    /* hba dma err                  */
#define CORV_CMDERR_BUF          0x23    /* hba buffer err               */
#define CORV_CMDERR_ABORT        0x24    /* hba aborted cmd              */
#define CORV_CMDERR_ERR          0x80    /* hba microprocessor error     */

/*      values for tsbp->t_targerr                                      */
#define CORV_TAERR_BUSRESET     0x01    /* scsi bus reset               */
#define CORV_TAERR_SELTO        0x10    /* scsi selection time out      */

#ifdef	TIMEOUT
struct t_arr {
	struct scsi_pkt *pktp;
	CORV_BLK *blkp;
	int tid;
} tarray[1000];

int tindx = 0;

#endif


/*
 * Debugging flags
 */
#ifdef	CORV_DEBUG

#define DERR  	0x0001
#define DPKT    0x0002
#define DINIT   0x0004
#define DINTR   0x0008
#define DCMD    0x0010
#define DENQ  	0x0020
#define DDEQ  	0x0040
#define DQUE  	0x0080
#define DENQERR	0x0100
#define DQDBG	0x0200
#define DERR2	0x0400

/*
**#define	DBG_PRF(fmt)	prom_printf fmt
*/
#define	DBG_PRF(fmt)	corv_err fmt
#define	DBG_FLAG_CHK(flag, fmt) if (corv_debug & (flag)) DBG_PRF(fmt)

#else

#define	DBG_FLAG_CHK(flag, fmt)

#endif

/*
 * Debugging printf macros
 */
#define	DBG_DPKT(fmt)		DBG_FLAG_CHK(DPKT, fmt)
#define	DBG_DINIT(fmt)		DBG_FLAG_CHK(DINIT, fmt)
#define	DBG_DINTR(fmt)		DBG_FLAG_CHK(DINTR, fmt)
#define	DBG_DCMD(fmt)		DBG_FLAG_CHK(DCMD, fmt)
#define	DBG_DENQ(fmt)		DBG_FLAG_CHK(DENQ, fmt)
#define	DBG_DDEQ(fmt)		DBG_FLAG_CHK(DDEQ, fmt)
#define	DBG_DQUE(fmt)		DBG_FLAG_CHK(DQUE, fmt)
#define	DBG_DENQERR(fmt)	DBG_FLAG_CHK(DENQERR, fmt)
#define	DBG_DQDBG(fmt)		DBG_FLAG_CHK(DQDBG, fmt)
#define	DBG_DERR2(fmt)		DBG_FLAG_CHK(DERR2, fmt)

#ifdef	CORV_DEBUG
#define	DBG_DERR(fmt)	DBG_FLAG_CHK(DERR, fmt)
#else
#define	DBG_DERR(fmt)	corv_err fmt
#endif

#ifdef  __cplusplus
 }
#endif

#endif	/* _CORVETTE_CORV_H */

