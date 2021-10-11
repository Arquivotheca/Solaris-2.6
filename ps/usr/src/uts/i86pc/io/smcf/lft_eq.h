/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)lft_eq.h 1.2	95/07/18 SMI"

/****************************************************************************
*
*       The information contained in this file is confidential and 
*       proprietary to Standard Microsystems Corporation.  No part
*       of this file may be reproduced or distributed, in any form
*       or by any means for any purpose, without the express written
*       permission of Standard Microsystems Corporation.
*
*	(c) COPYRIGHT 1995 Standard Microsystems Corporation,
*	ALL RIGHTS RESERVED.
*
* File:         lft_eq.h
*
* Description:  This file contains defines for the 83C100 FAST Ethernet chip
*               used on SMC9232 adapter.
*
*
*********************
** Revision History *
*********************
*
* Written by: Paul Brant 1/23/95
*
*
* $Log:   G:\sweng\src\lm9232\c_lmac\vcs\lft_eq.hv  $
 * 
 * Changes: move the I/O function prototype to lft_macr.h
 *          add SelectBank macro
 *
 *    Rev 1.0   23 Jan 1995 14:49:14   WATANABE
 * Initial release.
*
*
;+!/? **********************************************************************/



/*
 *
 *  Prototypes 
 *
 */

STATIC void PreventViolatingHardwareTimings();

/* 
 *  Interrupt Register Bits
 *
 */

#define EPHInt				0x20		/*  EPH type interrupt	*/
#define RxOverrunInt		0x10		/*  Receive overrun interrupt	*/
#define AllocInt			0x08		/*  Allocation interrupt	*/
#define TxEmptyInt			0x04		/*  Xmit fifo empty interrupt	*/
#define TransmissionInt		0x02		/*  Xmit complete interrupt	*/
#define ReceiveInt			0x01		/*  Receive complete interrupt	*/
#define EnabledInts			(TransmissionInt | ReceiveInt | EPHInt | RxOverrunInt)



/* 
 *  Pointer Register (Bank2_PTR)
 *
 */

#define Ptr_Rcv		0x8000		/*  Access is to receive area	*/
#define Ptr_Auto	0x4000		/*  Auto-increment on access	*/
#define Ptr_Read	0x2000		/*  =1 then read operation	*/
								/*  =0 then write operation	*/
#define PtrOffset	0x03ff		/*  Mask pointer value	*/

#define AutoIncrement	Ptr_Auto
#define ReceiveArea		Ptr_Rcv
#define	ReadMode		Ptr_Read

/*
 *		Xfer direction
 */

#define BOARD_TO_OS			1
#define OS_TO_BOARD			2

/* Miscellaneous masks */

#define SelectBankMask     0x0030 
#define IssueResetMask     0x8000 
#define FlushResetMask     0x0000 
#define Reload_StoreMask   0x0003 
#define MIRMultiplierMask  0x000e 
#define MMUBusyMask        0x0001 
#define RcvBadMask         0x4000 
#define RcvBadClrBit       0xbfff
#define RcvBadSetBit       0x4000 
#define PRMSBitMask		   0x0002
#define MMUBusyMask        0x0001 
#define SCECUnmaskByte     0x33             /* ;TXEmpty, Alloc. disabled */
#define SCECMaskAllByte	   0x0000
#define AllocationIntEna   0x08     
#define IntSelectionMask   0x0006 

#define ValidInts				0x3b
#define ByteCountMask           0x07ff
#define TEmptyBitMask           0x80     
#define AllocationIntDis        0x0f7    
#define ControlClrLEMask        0x0ff7f
#define ControlSetLEMask        0x0080


/* MMU Command bits */

#define AllocateTx         			0x0020            /* To be or-ed depending on the 
													   size to be requested */
#define ResetMMU					0x0040
#define RemoveFromTopRXFifo     	0x060
#define RemoveReleaseTopRXFifo  	0x080
#define ReleaseSpecificPacket   	0x0a0
#define EnqueueIntoTXFifo       	0x0c0
#define ResetTxFifos            	0x0e0

/*
 *  Frame send/receive control byte
 *
 */

#define CtlByteOdd	0x2000		/*  Frame ends on odd byte boundary	*/
#define CtlByteCRC	0x1000		/*  Append CRC on xmit	*/

#define SetOddBitControlByte	CtlByteOdd
#define ClrOddBitControlByte	0x0000
/* 
 *  Masks for Transmit Control Register (Bank0_TCR)
 *
 */

#define Tcr_EphLoop	0x2000		/*  Loop at EPH block	*/
#define Tcr_StpSqet	0x1000		/*  Stop xmit on SQET error	*/
#define Tcr_Fdx		0x0800		/*  Full duplex mode	*/
#define Tcr_MonCsn	0x0400		/*  Monitor carrier	*/
#define Tcr_NoCrc	0x0100		/*  Don't append CRC	*/
#define Tcr_PadEn	0x0080		/*  Pad short frames	*/
#define Tcr_ForCol	0x0004		/*  Force collision	*/
#define Tcr_Loop	0x0002		/*  Local loopback	*/
#define Tcr_TxEna	0x0001		/*  Enable transmitter	*/
#define TXENA_SetMask	Tcr_TxEna

/* 
 *  Masks for Receive Control Register (Bank0_RCR)
 *
 */

#define Rcr_Reset		0x8000		/*  Software reset	*/
#define Rcr_FiltCar		0x4000		/*  Filter carrier for 12 bits	*/
#define Rcr_Gain		0x0c00		/*  Adjust pll gain (test only)	*/
#define Rcr_StrpCrc		0x0200		/*  Strip CRC on received frames	*/
#define Rcr_RxEn		0x0100		/*  Enable receiver	*/
#define Rcr_AlMul		0x0004		/*  Accept all multicasts (no filtering)	*/
#define Rcr_Prms		0x0002		/*  Promiscuous mode	*/
#define Rcr_Abort		0x0001		/*  Frame aborted (too long)	*/

#define PromiscuousSetBit	Rcr_Prms
#define PromiscuousClrBit	(~Rcr_Prms)

/*
 *  Receive frame status word
 *
 */

#define Rfs_Align	0x8000		/*  Frame alignment error	*/
#define Rfs_Bcast	0x4000		/*  Frame was broadcast	*/
#define Rfs_CRC		0x2000		/*  Frame CRC error	*/
#define Rfs_Odd		0x1000		/*  Frame has odd byte count	*/
#define Rfs_Long	0x0800		/*  Frame was too long	*/
#define Rfs_Short	0x0400		/*  Frame was too short	*/
#define Rfs_Mcast	0x0001		/*  Frame was multicast	*/
#define Rfs_Hash	0x00fe		/*  Mask hash value (multicast)	*/
#define Rfs_Error	(Rfs_Align | Rfs_CRC | Rfs_Long | Rfs_Short)

#define OddFrameBitMask         0x10	/* upper byte of Rfs_Odd */

/*       Reception Error Bits      */

#define SCEC_RXErrorBits            0xa400  
#define SCEC_TooLongRXFrame         0x0800   
#define SCEC_TooShortRXFrame        0x0400   
#define SCEC_BadRxCRC               0x2000   
#define SCEC_AlignmentError         0x8000   

/*       Reception Statistics Bits	*/

#define SCEC_RxBroadcastBit          0x4000   
#define SCEC_RxMulticastBit          0x01

/* Transmission Fatal Errors */

#define SCEC_FatalErrors 			0x8630
#define SCEC_16Collision 			0x0010
#define SCEC_SQET        			0x0020
#define SCEC_LateCollision			0x0200
#define SCEC_CarrierSenseLost   	0x0400

/* Transmission Statatistics Bits */

#define SCEC_StatisticsBits          0x08ce   
#define SCEC_SuccessTx               0x0001   
#define SCEC_SingleCollision         0x0002   
#define SCEC_MultipleCollision       0x0004   
#define SCEC_LastTXMulticast         0x0008   
#define SCEC_LastTXBroadcast         0x0040   
#define SCEC_TXDeferred              0x0080   
#define SCEC_ExcessiveDeferral       0x0800   
#define SCEC_TxUnderrun				 0x8000


/*
 *
 *	Reset Values for some registers
 *
 */


#define ControlResetValue	  0x00a0

/*
; 000x 0xx0 100x x000       MMU not bypassed, Doesn't receive packet with bad 
; CRC, Not in PowerDown mode, Not autorelease mode, UDOE disabled, Link Error
; enabled, counter error disabled, transmit error enabled
*/

#define TCRResetValue		  0x0081

/*
; xx00 00x0 1xxx x001       No EPH Loop, Doesn't Stop Tx on SQET, No FullDpx,  
; Monitor Carr. off, CRC inserted on TX, Pad w/00 for Frames < 64 bytes, No  
; Force Collisions, No Loopback, Transmit enabled
*/

#define RCRResetValue			0x0300

/*
; 00xx 0011 xxxx x000, No reset, No filter carrier, PLL gain = 0, CRC on RX
; striped, Receiver on, Accept multicast matching only, No promiscuous mode,
; Clear RXabort
*/


/* Miscellaneous constants */

#define FRAMEOVERHEAD			6	   	/*  Overhead bytes for adapter control	*/
#define MMUWait					15	   	/*  Loop count waiting for memory allocation	*/

#define EISA_MANUF_ID          0x04da3  /* 'SMC' compressed */
#define EISA_BRD_9232          0x0A010
#define B574_DMA               0x06		
#define B574_EBC_ENB           0x0001   /* board enable bit */
#define B574_DMA_ROMEN         0x040    /* ROM enable bit */

#define CNFG_SIZE_64kb			64
#define CNFG_SIZE_128kb       	128

#define CRC_ERROR_STATUS        1
#define CRCErrorBit				CRC_ERROR_STATUS
#define CRCAlignErrorBit		2


#define SMC9000Pattern 			0x33

/***
	IncUMACCounter
	increment the counter in the UMAC data space (Adapter Structure)
***/
#define IncUMACCounter(counter) (*pAS->counter)++

/***
	SelectBank 
	Changes to the SCEC bank of registers specified in the bank parameter
***/
#define SelectBank(bank) SMC_OUTP(pAS->BankSelect, bank);

/***
	Convert256ByteUnitsToSCECPages
***/
#define Convert256ByteUnitsToSCECPages(count)	\
	if(pAS->SCECShrForPageSize) { \
		count <<= pAS->SCECShrForPageSize; \
	}
