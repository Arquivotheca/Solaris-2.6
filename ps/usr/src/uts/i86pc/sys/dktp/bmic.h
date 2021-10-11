/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved.
 * bmic.h -- global interface to the bmic driver and isr
 *
 */
#pragma ident	"@(#)bmic.h	1.1	93/10/25 SMI"

#ifndef __BMIC_H__
#define __BMIC_H__

/*
 * The TEOP (active low) bit in the interruptStatReg
 */
#define TEOP_LOW	0x80

#define MIN_TEOP_RETRY_SIZE	64

#define BMIC_TIMEOUT_COUNT	0x1000000

#define PAGE_BOUNDARY	(SRAM_BASE + 0x20000)

/*
 * The values here are significant since bit 6 in the channel base count
 * register determines the direction of the bmic burst transfer and bit 7
 * determines whether or not to start the tarnsfer as soon as the base
 * register set is copied into the current register set (which we always want).
 */
#define BMIC_READFROMHOST 0x80	  /* transfer from host to rad */
#define BMIC_WRITETOHOST  0xc0	  /* transfer from rad to host */

#define NUM_BMIC_PRIORITIES	4
#define LOW_BMIC_PRI		3

typedef struct _bmicRequest {
    unchar      command; 	/* use #defines above			   */
    char       xferPriority;	/* priority of transfer; values of 0 to 3  */
				/* are allowed; 0 is highest priority.	   */
    char       disableXfer;	/* disable actual transfer if asserted	   */
    char       *radAddr;	/* address of rad side buffer		   */
    uint       hostAddr;	/* address of host side buffer		   */
    uint       length;		/* length of transfer in bytes		   */
    struct _bmicRequest *next;	/*					   */
    void       *requestPtr;	/* pointer to originating request struct   */
    struct _ideRequest *ideReq; /*     pointer used by completion routines */
    void       (*returnFun)();	/* The function to call when the operation */
				/* is complete. This function gets called  */
				/* with one argument which is a pointer    */
				/* to this structure.			   */
    uint	decodePhase;	/* used by decoder to handle resets	   */
} bmicRequest;


/*---------------------- the cycle types ------------------------------------*/

#define IOWR   0x20		       /*				     */
#define IORD   0x40		       /*				     */
#define IOLK   0x60		       /*				     */
#define MEMWR  0x30		       /*				     */
#define MEMRD  0x50		       /*				     */
#define MEMLK  0x70		       /*				     */

#define bmicIndexReg	(*((volatile unsigned char *)BMIC_LIR))
#define bmicDataReg	(*((volatile unsigned char *)BMIC_LDR))
#define bmicSDataReg	(*((volatile unsigned char *)BMIC_SDR))
#define bmicStatusReg	(*((volatile unsigned char *)BMIC_LSR))
#define dmaPageReg	(*((volatile unsigned char *)INT_RESET))

#define BMIC_INDEX(x)	bmicIndexReg  = (volatile)(x)
#define BMIC_DATA(x)	bmicDataReg   = (volatile)(x)
#define BMIC_SDATA(x)	bmicSDataReg  = (volatile)(x)
#define BMIC_STATUS(x)	bmicStatusReg = (volatile)(x)

#define BMIC_LIR_AUTOINC	0x80

/*
 * Bit values of the bmic local status register
 */
#define BMIC_ST_DOORBELL_INT	0x80
#define BMIC_ST_CH1_INT 	0x40
#define BMIC_ST_CH0_INT 	0x20
#define BMIC_ST_LINT_DISABLE	0x10
#define BMIC_ST_LINT_ACTIVE	0x08
#define BMIC_ST_PEEK_POKE	0x04
#define BMIC_ST_CH1_BASE_BUSY	0x02
#define BMIC_ST_CH0_BASE_BUSY	0x01

#define BMIC_TSTTC		0x01
#define BMIC_TSTET		0x02
#define BMIC_TSTIP		0x04
#define BMIC_TSTEN		0x08
#define BMIC_TSTFF		0x10
#define BMIC_TST1K		0x20

/*
 * Bmic Channel configuration register values
 */
#define BMIC_CFGEA		0x80
#define BMIC_CFGIE		0x40
#define BMIC_CFGIT		0x20
#define BMIC_CFGFF		0x10
#define BMIC_CFGBR		0x08
#define BMIC_CFGCL		0x04
#define BMIC_CFGEI		0x02
#define BMIC_CFGSU		0x01

/*
 * Index values for the various BMIC registers
 */
#define BMIC_ID_0		0x00
#define BMIC_ID_1		0x01
#define BMIC_ID_2		0x02
#define BMIC_ID_3		0x03
#define BMIC_GLOBAL_CFG 	0x08
#define BMIC_SYS_INT_ENABLE	0x09
#define BMIC_SEMAPHORE_0	0x0a
#define BMIC_SEMAPHORE_1	0x0b
#define BMIC_LOC_DOOR_EN	0x0c
#define BMIC_LOC_DOOR_INT_STAT	0x0d
#define BMIC_EISA_DOOR_EN	0x0e
#define BMIC_EISA_DOOR_INT_STAT 0x0f
#define BMIC_MBOX_0		0x10
#define BMIC_MBOX_1		0x11
#define BMIC_MBOX_2		0x12
#define BMIC_MBOX_3		0x13
#define BMIC_MBOX_4		0x14
#define BMIC_MBOX_5		0x15
#define BMIC_MBOX_6		0x16
#define BMIC_MBOX_7		0x17
#define BMIC_MBOX_8		0x18
#define BMIC_MBOX_9		0x19
#define BMIC_MBOX_10		0x1a
#define BMIC_MBOX_11		0x1b
#define BMIC_MBOX_12		0x1c
#define BMIC_MBOX_13		0x1d
#define BMIC_MBOX_14		0x1e
#define BMIC_MBOX_15		0x1f
#define BMIC_PP_DATA_0		0x30
#define BMIC_PP_DATA_1		0x31
#define BMIC_PP_DATA_2		0x32
#define BMIC_PP_DATA_3		0x33
#define BMIC_PP_ADDR_0		0x34
#define BMIC_PP_ADDR_1		0x35
#define BMIC_PP_ADDR_2		0x36
#define BMIC_PP_ADDR_3		0x37
#define BMIC_PP_CONTROL 	0x38
#define BMIC_IO_DECODE_0_BASE	0x39
#define BMIC_IO_DECODE_0_CNTL	0x3a
#define BMIC_IO_DECODE_1_BASE	0x3b
#define BMIC_IO_DECODE_1_CNTL	0x3c
#define BMIC_CH0_BASE_CNT_0	0x40
#define BMIC_CH0_BASE_CNT_1	0x41
#define BMIC_CH0_BASE_CNT_2	0x42
#define BMIC_CH0_BASE_ADDR_0	0x43
#define BMIC_CH0_BASE_ADDR_1	0x44
#define BMIC_CH0_BASE_ADDR_2	0x45
#define BMIC_CH0_BASE_ADDR_3	0x46
#define BMIC_CH0_CFG		0x48
#define BMIC_CH0_STROBE 	0x49
#define BMIC_CH0_STATUS 	0x4a
#define BMIC_CH0_TBI_BASE_0	0x4b
#define BMIC_CH0_TBI_BASE_1	0x4c
#define BMIC_CH0_CUR_CNT_0	0x50
#define BMIC_CH0_CUR_CNT_1	0x51
#define BMIC_CH0_CUR_CNT_2	0x52
#define BMIC_CH0_CUR_ADDR_0	0x53
#define BMIC_CH0_CUR_ADDR_1	0x54
#define BMIC_CH0_CUR_ADDR_2	0x55
#define BMIC_CH0_CUR_ADDR_3	0x56
#define BMIC_CH0_TBI_CUR_0	0x58
#define BMIC_CH0_TBI_CUR_1	0x59
#define BMIC_CH1_BASE_CNT_0	0x60
#define BMIC_CH1_BASE_CNT_1	0x61
#define BMIC_CH1_BASE_CNT_2	0x62
#define BMIC_CH1_BASE_ADDR_0	0x63
#define BMIC_CH1_BASE_ADDR_1	0x64
#define BMIC_CH1_BASE_ADDR_2	0x65
#define BMIC_CH1_BASE_ADDR_3	0x66
#define BMIC_CH1_CFG		0x68
#define BMIC_CH1_STROBE 	0x69
#define BMIC_CH1_STATUS 	0x6a
#define BMIC_CH1_TBI_BASE_0	0x6b
#define BMIC_CH1_TBI_BASE_1	0x6c
#define BMIC_CH1_CUR_CNT_0	0x70
#define BMIC_CH1_CUR_CNT_1	0x71
#define BMIC_CH1_CUR_CNT_2	0x72
#define BMIC_CH1_CUR_ADDR_0	0x73
#define BMIC_CH1_CUR_ADDR_1	0x74
#define BMIC_CH1_CUR_ADDR_2	0x75
#define BMIC_CH1_CUR_ADDR_3	0x76
#define BMIC_CH1_TBI_CUR_0	0x78
#define BMIC_CH1_TBI_CUR_1	0x79

#endif

