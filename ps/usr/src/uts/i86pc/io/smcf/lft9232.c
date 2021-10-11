/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)lft9232.c 1.2	95/07/18 SMI"

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
* File:         lft9232.c
*
* Description:  LMAC module for EISA FAST Ethernet SMC9232 LAN Adapter Series.
*
*
*********************
** Revision History *
*********************
*
* Written by:     Paul Brant
* Date:           11/23/93
*
* By         Date     Ver.   Modification Description
* ------------------- -----  --------------------------------------
* Ronnie_k   01/20/95 2.01   Current version compiles with ODI ASM UMAC	  
* Paul_b     12/17/94 2.00   Update driver to support the LMAC SPEC
*                            more precisely.
* Scot_h     05/17/94 1.01   Consolidate the driver
* Paul_b     04/18/94 1.00   First working ethernet version
*
*
*
* $Log:   G:\sweng\src\lm9232\c_lmac\vcs\lft9232.cv  $
 * 
 * Changes: Take out include "lft_cfg.c"
 *          Make use of lft_macr.h
 *			conform to ASM version 
 *
 *    Rev 1.0   23 Jan 1995 14:49:16   WATANABE
 * Initial release.
*
*
;+!/? **********************************************************************/


#ident "@(#)lft9232.c	2.00 - 95/1/17"

/*
 * Streams driver for SMC9000 Ethernet controller
 * Implements an extended version of the AT&T Link Interface
 * IEEE 802.2 Class 1 type 1 protocol is implemented and supports
 * receipt and response to XID and TEST but does not generate them
 * otherwise.
 * Ethernet encapsulation is also supported by binding to a SAP greater
 * than 0xFF.
 */

#ifdef REALMODE
#include <sys/types.h>
#include "common.h"
#endif

#if ((UNIX & 0xff00) == 0x400)	/* Solaris 2.x */
#include <sys/types.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#endif	/* Solaris */

#ifdef NDIS3X
#include <ndis.h>
#endif

#include "lft_macr.h"
#include "lmstruct.h"
#include "lft_eq.h"

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
#include "lft_dma.h"
#endif

#ifdef UNIX
STATIC int DMAxfer ();
#ifdef BADLINT
STATIC CopyTxData();
#else
STATIC int CopyTxData();
#endif
STATIC int CheckMultiAdd();
STATIC int	RxInt();
STATIC int	TxInt();
#ifdef BADLINT
STATIC EnableSCECInterrupts();
#else
STATIC int EnableSCECInterrupts();
#endif
STATIC int	EPHIntr();
#ifdef BADLINT
STATIC WaitForMMUReady();
STATIC SCECDriverInit();
#else
STATIC int WaitForMMUReady();
STATIC int SCECDriverInit();
#endif
STATIC int	Allocate();
STATIC int xfer ();
STATIC int	AllocIntr();
STATIC int	RxOvrnIntr();
extern void GetLANAddress();
#else
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
STATIC int 
DMAxfer (AdapterStructure *pAS,
     DWORD buf, 
	 DWORD length,
     WORD length,  
     WORD dma_read_write
     );
#endif /* -REALMODE && -Solaris */


STATIC
CopyTxData(
    AdapterStructure *pAS,
    #ifdef CODE_386
         DataBufferStructure *mb,
    #else
         DataBufferStructure _far *mb,
    #endif
    BYTE	pktnum,
    int 	length
            );


STATIC int
CheckMultiAdd(
    AdapterStructure *pAS
            );

STATIC int	
RxInt(
    AdapterStructure *pAS
    );

STATIC int	
TxInt(
    AdapterStructure *pAS
    );

STATIC 
EnableSCECInterrupts(
    AdapterStructure *pAS
    );

STATIC int	
EPHIntr(
    AdapterStructure *pAS
    );

STATIC 
WaitForMMUReady(
    AdapterStructure *pAS
    );

STATIC
SCECDriverInit(
    AdapterStructure *pAS
    );

STATIC int	
Allocate(
    AdapterStructure *pAS,
    #ifdef CODE_386
         DataBufferStructure *mb,
    #else
         DataBufferStructure _far *mb,
    #endif
    int 	Length
    );


STATIC int 
xfer (
    AdapterStructure *pAS,
    DWORD	buf,
    DWORD length,
    WORD direction
    );

STATIC int	
AllocIntr(
    AdapterStructure *pAS
    );


STATIC int	
RxOvrnIntr(
    AdapterStructure *pAS
    );

extern void GetLANAddress(
    AdapterStructure *pAS
    );
#endif

/*
 *
 *      ResetSCEC
 *
 *      Routine to totally reinitialize the SCEC.
 *
 *      entry:          pAS	pointer to the adapter structure
 *
 */

STATIC void
ResetSCEC(pAS)
AdapterStructure *pAS;
{
	WORD regvalw;

	pAS->TxPacketsInsideSCEC = 0;     	/* No Tx Pckts inside after RST */
	pAS->SCECMaxPagesForTx = pAS->SCECNumberOfPages - pAS->SCEC1518InSCECPages;
													/* Set max Tx mem to default */

	pAS->MaxPercentageOnTx[0] = (100 * pAS->SCECMaxPagesForTx) / pAS->SCECNumberOfPages;

   /* Set the Control Register */

	SelectBank(0x10);

	SMC_INPW(pAS->Control, &regvalw);
	SMC_OUTPW(pAS->Control, (regvalw & RcvBadMask) | ControlResetValue);

	SelectBank(0x20);
	EnableSCECInterrupts(pAS);

	EPHIntr(pAS);		/* update of AUI bit in Config */

	SelectBank(0);

	SMC_OUTPW(pAS->TCR, (WORD) TCRResetValue);
	SMC_INPW(pAS->RCRRegister, &regvalw);
	SMC_OUTPW(pAS->RCRRegister, (regvalw & PRMSBitMask) | RCRResetValue);

}

/*
 *
 *		DoRAMTest
 *
 *		Routine to test out the SCEC's RAM
 *
 *		entry:		pAS	Pointer to adapter structure
 *					
 *		return:		0 		no error
 *						1		error
 *
 */

STATIC
DoRAMTest(pAS)
AdapterStructure *pAS;
{
	int retcod=0;
	WORD regvalw;
	BYTE mem;
	WORD i, testmem=0;

	SelectBank(0);
	SMC_INPW(pAS->MIR, &regvalw);

	while((regvalw &= 0xff00) != 0) { /* highbyte is freememory */

			if((mem = (BYTE)(regvalw >> 8)) >=8 ) 
			    mem=7;   /* gz was 8 */
			else mem--;

#ifdef BUG
			SelectBank(0x20)
#else
			SelectBank(0x20);
#endif

			SMC_OUTPW(pAS->MMUCommand,(WORD) ( mem | AllocateTx));

			for(i=0;i<15;i++) {		/* Poll for memory allocation */
				SMC_INPW(pAS->Interrupt, &regvalw);
				if(regvalw & AllocInt) break;
			}

			if(i>=15) {		/* fail in allocation */
				retcod=1;
				break;
			}

			SMC_INPW(pAS->PNR_ARR, &regvalw);

            /* Copy allocated pktnum to pnr */
			SMC_OUTP(pAS->PNR_ARR, (BYTE) (regvalw >> 8)); 

            /* set address=0,autoinc,tx area, write */

			SMC_OUTPW(pAS->Pointer, AutoIncrement);
			
			testmem = (WORD) ( ++mem << 7);

			for(i=0; i< testmem; i++) { 
/*
				SMC_OUTPW(pAS->Data0_1, 0xaa55);
*/
				SMC_OUTPW(pAS->Data0_1, testmem -i);
			}


			PreventViolatingHardwareTimings();

            /* now read pattern */

			SMC_OUTPW(pAS->Pointer, AutoIncrement | ReadMode);
			
			for(i=0; i<testmem; i++) {
				SMC_INPW(pAS->Data0_1, &regvalw);
/*
				if(regvalw != 0xaa55) break;
*/
				if(regvalw != testmem -i) break;
			}

			if(i<testmem) {
				retcod=1;		/* fail the comparsion */
				break;
			}

			SelectBank(0);	

			SMC_INPW(pAS->MIR, &regvalw);

		}

	SelectBank(0x20);
	SMC_OUTPW(pAS->MMUCommand, (WORD)ResetMMU);	/* Free Allocated mem if any */
	WaitForMMUReady(pAS);

    if (testmem == 0) /*  gets 0 before loop & if loop is never entered */
        retcod = 1;

	return(retcod);
}

/*
 *	SCECDriverInit
 *
 *	This routine is called to intialize and bring the SCEC Board
 *	into operation.
 *
 *		entry:		pAS	Pointer to adapter structure
 *
 */

STATIC
SCECDriverInit(pAS)
AdapterStructure *pAS;
{
	unsigned int i,value1;
	WORD regbase;
	BYTE regval;
	WORD regvalw;

 	pAS->InScecDriverInitFlag = 1;         /* Set flag -> inside ScecDriverInit */
	regbase=pAS->io_base;

 	if (pAS->slot_num != 0)
		regbase+=0x10;

	 /* Bank 0 */
	 pAS->TCR 					= regbase + 0;
	 pAS->StatusRegister 	= regbase+ 2;
	 pAS->RCRRegister 		= regbase+ 4;
	 pAS->Counter 				= regbase+ 6;
	 pAS->MIR 					= regbase+ 8;
	 pAS->MCR 					= regbase+ 10;
	 pAS->NotUsed1        	= regbase+ 12;
	 pAS->BankSelect 			= regbase+ 14;
	 
	 /* Bank 1 */
	 pAS->Configuration 		= regbase+ 0;   
	 pAS->BaseAddress 		= regbase+ 2;   
	 pAS->IA0_1 				= regbase+ 4;           
	 pAS->IA2_3 				= regbase+ 6;           
	 pAS->IA4_5 				= regbase+ 8;           
	 pAS->General 				= regbase+ 10;         
	 pAS->Control 				= regbase+ 12;       
	 pAS->NotUsed2 			= regbase+ 14;      
	  
	 /* Bank 2 */
	 pAS->MMUCommand 			= regbase+ 0;    
	 pAS->PNR_ARR 				= regbase+ 2;         
	 pAS->FifoPorts 			= regbase+ 4;     
	 pAS->Pointer 				= regbase+ 6;         
	 pAS->Data0_1 				= regbase+ 8;         
	 pAS->Data2_3 				= regbase+ 10;         
	 pAS->Interrupt 			= regbase+ 12;     
	 pAS->NotUsed3 			= regbase+ 14;      
	 
	 /* Bank 3 */
	 pAS->MT0_1 				= regbase+ 0;           
	 pAS->MT2_3 				= regbase+ 2;           
	 pAS->MT4_5 				= regbase+ 4;           
	 pAS->MT6_7 				= regbase+ 6;           
	 pAS->NotUsed4 			= regbase+ 8;      
	 pAS->Revision 			= regbase+ 10;      
	 pAS->ERCV 					= regbase+ 12;          

	SMC_INP(pAS->BankSelect+1, &regval);	/* ASM - read word */
	if	(regval!= SMC9000Pattern)
		{
	 	 pAS->InScecDriverInitFlag = 0;				/* We are leaving */
	 	 return(FAILED);
		}
	
	/* Issue Software Reset */

	SelectBank(0);

	SMC_OUTPW(pAS->RCRRegister, (WORD)IssueResetMask); /* Set EPH_RST */
	SMC_INPW(pAS->RCRRegister, &regvalw);	/*  Wait  */
	SMC_INPW(pAS->RCRRegister, &regvalw);	/*  Wait  */
	SMC_INPW(pAS->RCRRegister, &regvalw);	/*  Wait  */
	SMC_OUTPW(pAS->RCRRegister, (WORD)FlushResetMask);	/* Clear EPH_RST */

   /* See if SCEC is alive */

	SelectBank(0x10);

	SMC_INPW(pAS->BankSelect, &regvalw);
	if ((regvalw & SelectBankMask) != 0x0010)
	{
	 pAS->InScecDriverInitFlag = 0;					/* We are leaving */
	 return(FAILED);
	}
	for (i=0;i<10000;i++) {
		  SMC_INPW(pAS->Control, &regvalw);
	 	  if ((regvalw & Reload_StoreMask) == 0) break;
	}
	if (i>=10000) 
	{
	 pAS->InScecDriverInitFlag = 0;					/* We are leaving */
	 return(FAILED);
	}

   /* Now get the M (MIR multiplier) from the MCR and set the 
      MIRShlTo256ByteUnits variable to the proper value (M-1)
	*/

	SelectBank(0);
	SMC_INP(pAS->MCR+1, &regval);		/* ASM - read word */
#ifdef BADLINT
	pAS->MIRShlTo256ByteUnits = (((regval & MIRMultiplierMask) >> 1) - 1);
#else
	pAS->MIRShlTo256ByteUnits = (((BYTE)(regval & MIRMultiplierMask) >> 1) - 1);
#endif

   /* This part of the initialization fills SCECPageSize variable with
      the SCEC page size in 256 byte units and the SCECShrForPageSize
      var with the power of 2 which the latter number is, it asumes
      the page size is always a "power of two" multiple of 256. It
      also sets the strings that show the page size at load time
      and in the statistic table for the MEM_UTIL version.
	*/

	SelectBank(0x20);
	SMC_OUTPW(pAS->MMUCommand, (WORD)ResetMMU);	/* Free Allocated mem if any */
	WaitForMMUReady(pAS);

	SelectBank(0);
	SMC_INPW(pAS->MIR, &regvalw);
	pAS->SCECRAMSize = regvalw & 0x0ff;	/* Get mem size */
	if (pAS->SCECRAMSize == 0x0ff) pAS->SCECRAMSize++;
	pAS->SCECRAMSize = (pAS->SCECRAMSize << pAS->MIRShlTo256ByteUnits);

	SelectBank(0x20);
	SMC_OUTPW(pAS->MMUCommand, (WORD)AllocateTx);		/* Ask for 256 bytes */
	WaitForMMUReady(pAS);

	SelectBank(0);
	SMC_INP(pAS->MIR+1, &regval);		/* ASM --- read word */
	value1 = pAS->SCECPageSize = pAS->SCECRAMSize -
				(unsigned int) ((regval & 0x0ff) << pAS->MIRShlTo256ByteUnits);

	i=0;
	value1 >>= 1;			/* ASM --- just use the lower byte */
	while ( value1 != 0)
	{
	 i++;
	 value1 >>= 1;
	}
   pAS->SCECShrForPageSize = (unsigned char) i;
	pAS->SCECMaxPagesForTx = pAS->SCECNumberOfPages =
									  (pAS->SCECRAMSize / pAS->SCECPageSize);

	/* Fill Ceiling of 1518 bytes in 256 byte units */
									
	pAS->SCEC1518InSCECPages = (6/pAS->SCECPageSize);
	if((6 % pAS->SCECPageSize) != 0) pAS->SCEC1518InSCECPages++;
	pAS->SCECMaxPagesForTx -= pAS->SCEC1518InSCECPages;

	if (DoRAMTest(pAS)!=SUCCESS)
	{
	 pAS->InScecDriverInitFlag = 0;					/* We are leaving */
	 return(FAILED);
	}
	
	ResetSCEC(pAS);				/* Call common init routine */
	pAS->InScecDriverInitFlag = 0;     				/* We are leaving */

	return(SUCCESS);
}


#ifndef REALMODE
/*
 *  LM_Add_Multi_Address
 *
 *
 *  Functional Description:
 *  This function adds ethernet addresses to hardware
 *	 multicast hash table.
 *	Assume it is called under interrupt disable
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *		pAS->multi_address Pointer to 6 byte ethernet address to add to 
 *									multicast table.
 *
 *  Exit:
 *	 Return status
 *
 */

int 
LM_Add_Multi_Address ( pAS )
AdapterStructure *pAS;
{
	WORD i,j;

	for(i=0; i<MC_TABLE_ENTRIES; i++) {
		for(j=0;j<6;j++) {
			if((pAS->multi_address)[j] != (pAS->mc_table)[i].address[j])
				break;
		}
		if(j<6) continue;
	
  		/* Found our address. Now try to inc counter */

		if((pAS->mc_table)[i].instance_count == 0xff)
			return(OUT_OF_RESOURCES);
		else {
			((pAS->mc_table)[i].instance_count)++;
			if((pAS->mc_table)[i].instance_count==1)
				(pAS->mc_count)++;
  			return(SUCCESS);
		}
 	}

 /* Didn't find the address look for the first unsed entry and put this address */

	for(i=0; i<MC_TABLE_ENTRIES; i++) {
		if((pAS->mc_table)[i].instance_count == 0)
			break;
	}
	
	if(i>=MC_TABLE_ENTRIES) 
		return(OUT_OF_RESOURCES);

  /* Found one free */

	for (j=0;j<6;j++)
		(pAS->mc_table)[i].address[j] = (pAS->multi_address)[j];

	((pAS->mc_table)[i].instance_count)++;
	(pAS->mc_count)++;

  	return(SUCCESS);
}

/*
 *  LM_Delete_Multi_Address
 *
 *
 *  Functional Description:
 *  This function changes ethernet addresses to hardware
 *	 multicast hash table.
 *	Assume it is called under interrupt disable
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *		pAS->multi_address Pointer to 6 byte ethernet address to add to 
 *									multicast table.
 *
 *  Exit:
 *	 Return status
 *
 */

int 
LM_Delete_Multi_Address ( pAS )
AdapterStructure *pAS;
{
	WORD i,j;

	for(i=0; i<MC_TABLE_ENTRIES; i++) {
		for(j=0;j<6;j++) {
			if((pAS->multi_address)[j] != (pAS->mc_table)[i].address[j])
				break;
		}
		if(j<6) continue;
		
		if((pAS->mc_table)[i].instance_count==0)
			return(OUT_OF_RESOURCES);
		else {
			((pAS->mc_table)[i].instance_count)--;
			if((pAS->mc_table)[i].instance_count==0)
				(pAS->mc_count)--;
			return(SUCCESS);
		}
 	}

 	/* Did not find our address, return status */

 	return(OUT_OF_RESOURCES);
}

/*
 *  LM_Change_Receive_Mask
 *
 *
 *  Functional Description:
 *  changes the receive addressing requirements for receiving packets
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *	 Return status
 *
 */

int 
LM_Change_Receive_Mask ( pAS )
AdapterStructure *pAS;
{

	unsigned char	bank;
	WORD	RecvReg,Ctrl_Reg;

	if ( pAS->adapter_status == NOT_INITIALIZED)
   	return (ADAPTER_NOT_INITIALIZED);

   /* Save SMC9000 context, set to bank 0 */

	SMC_INP(pAS->BankSelect, &bank);
	bank &= (unsigned char) SelectBankMask;

	SelectBank(0);

	/* Set RCR Register bits (almul & prms) if necessary */

	SMC_INPW(pAS->RCRRegister, &RecvReg);	/* get current receive mode */

/* RONNIE */
/* Why set ALMUL upon the broadcast bit, its a waste of scec's resources !!
	if ( pAS->receive_mask & (ACCEPT_MULTICAST | ACCEPT_MULTI_PROM ))
*/
	if ( pAS->receive_mask & 
			(ACCEPT_MULTICAST | ACCEPT_MULTI_PROM | ACCEPT_BROADCAST))
		RecvReg |= Rcr_AlMul;
	else 	
		RecvReg &= ~Rcr_AlMul;

	if ( pAS->receive_mask & PROMISCUOUS_MODE )
		RecvReg |= PromiscuousSetBit;	/* Rcr_Prms */
	else 	
		RecvReg &= PromiscuousClrBit;	/* ~Rcr_Prms */

   SMC_OUTPW(pAS->RCRRegister,(WORD)RecvReg);

	SelectBank(0x10);

	/* Set Rcv Bad if needed */

	SMC_INPW(pAS->Control, &Ctrl_Reg);
	Ctrl_Reg = Ctrl_Reg & RcvBadClrBit;
	if (pAS->receive_mask & ACCEPT_ERR_PACKETS)
		Ctrl_Reg |= RcvBadSetBit;
	SMC_OUTPW(pAS->Control,(WORD)Ctrl_Reg);

   	SMC_OUTP(pAS->BankSelect, bank);			/* set to original bank */

   return (SUCCESS);
}

/*
 *  LM_Set_Group_address (Stub)
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *  Return:
 *    SUCCESS
 *
 */

int 
LM_Set_Group_Address(pAS)
	  AdapterStructure *pAS;
{
   return (INVALID_FUNCTION);
}


/*
 *  LM_Delete_Group_address (Stub)
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *  Return:
 *    SUCCESS
 *
 */

int 
LM_Delete_Group_Address(pAS)
	  AdapterStructure *pAS;
{
   return (INVALID_FUNCTION);
}


/*  
 *	 LM_Interrupt_Req(pAS)
 *
 *	 Routine Description:
 *
 *  This routine will generate the hardware interrupt.
 *	 But this function is not supported by this adapter.
 *  Use Allocation Interrupt to generate int for this test.
 *
 *	 Arguments:
 *
 *  Adapt - A pointer to an LMB adapter structure.
 *
 *
 *	 Return:
 *
 *	 INVALID_FUNCTION
 */

int
LM_Interrupt_Req(pAS)
AdapterStructure *pAS;
{
	unsigned char bank;
	unsigned char regval;

	SMC_INP(pAS->BankSelect, &bank);
	SelectBank(0x20);

	pAS->AllocationRequestedFlag=1;

	SMC_INP(pAS->Interrupt+1, &regval);
	
	SMC_OUTP(pAS->Interrupt+1, regval | AllocationIntEna);

	SMC_OUTP(pAS->MMUCommand, AllocateTx);

	SMC_OUTP(pAS->BankSelect, bank);
	return(SUCCESS);
}
#endif /* -REALMODE */

/*
 *  LM_Enable_Adapter 
 *
 *
 *  Functional Description:
 *  This function enables receive and transmit interrupts and board interrupts
 *  Assume it is called while interrupt disable
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *	 Return status
 *
 */

int 
LM_Enable_Adapter ( pAS )
AdapterStructure *pAS;
{
	
	unsigned char bank;

	if(pAS->adapter_status!=CLOSED) {
		SMC_INP(pAS->BankSelect, &bank);	/* Save bank */
		SelectBank(0x20);

		EnableSCECInterrupts(pAS);

   		SMC_OUTP(pAS->BankSelect, bank);		/* set to original bank */

		pAS->adapter_flags &= ~ADAPTER_DISABLED;
	}

	return (SUCCESS);
}

/*
 *  LM_Disable_Adapter 
 *
 *
 *  Functional Description:
 *  This function disables receive and transmit interrupts and board interrupts
 *  Assume it is called while interrupt disable
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *	 Return status
 *
 */

int 
LM_Disable_Adapter ( pAS )
AdapterStructure *pAS;
{
	unsigned char bank;

	SMC_INP(pAS->BankSelect, &bank);	/* Save bank */
	SelectBank(0x20);

   	SMC_OUTP(pAS->Interrupt+1, SCECMaskAllByte); /* disable board interrupts */

	SMC_OUTP(pAS->BankSelect, bank);	/* set to original bank */
	pAS->adapter_flags |= ADAPTER_DISABLED;
	return ( SUCCESS );
}

/*
 *  LM_Set_Funct_address (Stub)
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *  Return:
 *    SUCCESS
 *
 */

int 
LM_Set_Funct_Address(pAS)
	  AdapterStructure *pAS;
{
   return (INVALID_FUNCTION);
}

/*
 *	 LM_Initialize_Adapter
 *
 *  Initialize the hardware
 *
 *  Functional Description:
 *  This routine copy's the LMAC receive data to the UMAC
 * 	Assume it is called under interrupt disable
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *	 return code
 *
 */

int 
LM_Initialize_Adapter ( pAS )
AdapterStructure *pAS;
{
	int	i;
	BYTE	value;

	if ((pAS->rx_lookahead_size) == 0)		/* is it not defined yet? */
		pAS->rx_lookahead_size = 4;

	if (pAS->num_of_tx_buffs > 2) 
		pAS->num_of_tx_buffs = 2;

   pAS->adapter_flags |= RX_VALID_LOOKAHEAD;

	if (SCECDriverInit(pAS) == SUCCESS)
		{
		 value=0;
		 for (i=0;i<6;i++) value |= pAS->node_address[i];
		 if (value == 0)
			 GetLANAddress(pAS);
		 else {
			SelectBank(0x10);
			for (i=0;i<6;i++) 
				SMC_OUTP(pAS->IA0_1+i, (BYTE) pAS->node_address[i]);
		 }
		 pAS->tx_count = 0;
         pAS->hdw_int = 0;

		 InitErrorCounters(pAS);

		 pAS->adapter_status = INITIALIZED;

		 UM_Status_Change(pAS);

		 return (SUCCESS);
		}
	else return (SELF_TEST_FAILED);
}

/*
 *  LM_Open_Adapter 
 *
 *	 Routine Description:
 *
 *  This routine will open the adapter, initializing it if necessary.
 *
 *	 Arguments:
 *
 *  pAS - A pointer to an LM adapter structure.
 *
 *
 *	 Return:
 *
 *  SUCCESS				  
 *  OPEN_FAILED
 *  ADAPTER_HARDWARE_FAILED
 *
 */

int 
LM_Open_Adapter ( pAS )
AdapterStructure *pAS;
{
	 pAS->adapter_status = OPEN;
	 LM_Enable_Adapter(pAS);
     UM_Status_Change(pAS);
	 return(SUCCESS);
}


/*
 *  LM_Close_Adapter 
 *
 *
 *	 Routine Description:
 *
 *  This routine will close the adapter, stopping the card.
 *
 *	 Arguments:
 *
 *  pAS - A pointer to an LM adapter structure.
 *
 *
 *	 Return:
 *
 *  SUCCESS
 *  CLOSE_FAILED
 *  ADAPTER_HARDWARE_FAILED
 *
 */

int 
LM_Close_Adapter ( pAS )
AdapterStructure *pAS;
{
   pAS->adapter_status = CLOSED;
   LM_Disable_Adapter(pAS);
   UM_Status_Change(pAS);
   return(SUCCESS);
}

/*
 *  LM_Send (transmit a packet)
 *
 *
 *  Functional Description:
 *  LM_Send is called when a packet is ready to be transmitted. 
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *		int						length   Number of bytes to copy
 *		struct					DataBuffStructure *mb 
 *		struct					Ptr_Adapter_Struc pAS Adapter structure
 *
 *  Exit:
 *  Return:
 *    SUCCESS
 *  OUT_OF_RESOURCES
 *	 None.
 *
 */

int 
LM_Send(mb, pAS,length)
#ifdef CODE_386
     DataBufferStructure *mb;	/* ptr to block containing data */
#else
/* RONNIE Added _far to adjust for mb = es:si received from lc_uasm*/ 
     DataBufferStructure _far *mb;	/* ptr to block containing data */
#endif
	  AdapterStructure *pAS;
     unsigned int length; /* total length of packet */
{
	/* SelectBank(0x20); --- taking care by Allocate() */
	return(Allocate(pAS, mb, length));
}

/*
 *  Allocation Routine.
 *
 *  Functional Description.
 *	This routine is invoked when a packet is ready for output.  We
 *	try to allocate memory on the card to hold the packet.  If the
 *	memory is immediately available the write can proceed.  Otherwise
 *	we have to wait until an allocation interrupt occurs.  This
 *	routine may be called from both interrupt level and task level.
 *
 *  Entry:
 *	IN Ptr_Adapter_Struc pAS
 *	IN Pointer to DataStructBuffer
 *	IN UINT Length
 *
 *  Exit:
 *	 SUCCESS
 *   OUT_OF_RESOURCE
 *
 */

STATIC int	
Allocate(pAS, mb, Length )
AdapterStructure *pAS;
#ifdef CODE_386
     DataBufferStructure *mb;	/* ptr to block containing data */
#else
     DataBufferStructure _far *mb;	/* ptr to block containing data */
#endif
int 	Length;
{
	int i;
	BYTE regval;
	int	AllocSize;

	AllocSize=pAS->HardwareFrameLength=Length;
	
	if(!(AllocSize & 0x1)) AllocSize++; 
	AllocSize+=4;	 /* Add Space for StatusWord, Count & Control, minus 1
	 					to adjust to MMUCommand AllocReq */

	SelectBank(0x20);
	SMC_OUTP(pAS->MMUCommand, AllocateTx);

	for(i=0; i< 15; i++) {	/* Poll for allocation */
		SMC_INP(pAS->Interrupt, &regval);
		if(regval & AllocInt) break;
	}	
	
	if(i>=15) 
		return(OUT_OF_RESOURCES);
	
	SMC_INP(pAS->PNR_ARR+1, &regval);

	/* pass the packet number and the actual size (+header) */
	return(CopyTxData(pAS, mb, regval, AllocSize+1));

}

STATIC
CopyTxData(pAS, mb, pktnum, length)
AdapterStructure *pAS;
#ifdef CODE_386
     DataBufferStructure *mb; 
#else
     DataBufferStructure _far *mb;
#endif
BYTE	pktnum;
int 	length;
/*
   DESCRIPTION: Copy the tx data into the Controller.

   Called by:   Allocate, AllocIntr
 
   PARAMETERS:  AdapterStructure *pAS;
                DataBufferStructure *mb;	 
                    ptr to block containing data-RONNIE Added _far to adjust
                    for mb = es:si received from lc_uasm.
                pktnum  - allocated packet number.
                length  - allocated space needed for packet.

   RETURNS:     zip
 
*/

{
	WORD regvalw;
	unsigned int i=0;

    /* Set tx area and auto inc */

	SMC_OUTP(pAS->PNR_ARR, pktnum);
	SMC_OUTPW(pAS->Pointer, (length - 2) | AutoIncrement); 

    /* Set/Clear odd bit in control byte */

	if(pAS->HardwareFrameLength & 0x1)
		regvalw=SetOddBitControlByte;
	else
		regvalw=ClrOddBitControlByte;

	SMC_OUTPW(pAS->Data0_1, regvalw);

	PreventViolatingHardwareTimings();

    /* Copy status word and byte count */

	SMC_OUTPW(pAS->Pointer, (WORD) AutoIncrement);

#ifdef	BUG
	SMC_OUTPD(pAS->Data0_1, (DWORD) (length << 16));
#else
	SMC_OUTPD(pAS->Data0_1, (DWORD)length << 16);
#endif

    /* Write out the data fragments to the controller */

	for(i=0; i<mb->fragment_count; i++) {
		xfer (pAS,(DWORD)mb->fragment_list[i].fragment_ptr, 
			 (DWORD)mb->fragment_list[i].fragment_length, OS_TO_BOARD);
	}

   SMC_OUTPW(pAS->MMUCommand,(WORD) EnqueueIntoTXFifo);

   return(SUCCESS);
}

/*
 *  CheckMultiAdd
 *
 *
 *  Functional Description:
 *  
 *	 This routine checks if the address in pAS->look_ahead_buf is an active
 *	 address in the multicast and broadcast table.
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *	 SUCCESS					found address
 *	 OUT_OF_RESOURCES		did not find address
 */


STATIC int
CheckMultiAdd(pAS)
AdapterStructure *pAS;
{
	int i,j;
	McTable *PtrMCTable;

	if(pAS->receive_mask & ACCEPT_BROADCAST) { 
		/* check whether broadcast addr */
		/* this should be corresponding to the value in bc_add (lmstruct.inc) */
		for(j=0;j<6; j++) 
			if(*(pAS->look_ahead_buf+j+4) != 0xff) break;

		if(j==6) /* is broadcast addr (instance count is initialized to 1) */
			return(SUCCESS);
	}

	PtrMCTable = &(pAS->mc_table[0]);
	for(i=0;i<MC_TABLE_ENTRIES;i++,PtrMCTable++) {
		for (j=0;j<6;j++) 	/* +4 to bypass status word and byte count */
			if(*(pAS->look_ahead_buf+j+4) != PtrMCTable->address[j]) break;

 		if (j<6) continue;

		if (PtrMCTable->instance_count == 0 ) 			
  	  		return(OUT_OF_RESOURCES);				/* Found but inactive */
  		else 
  	  		return(SUCCESS);							/* Found and active */

 	}

 	return(OUT_OF_RESOURCES);						/* Did not find */

}



/*
 *  LM_Service_Events
 *
 *  Functional Description:
 *
 *  This routine is called by the UMAC to service a hardware interrupt
 *  Functional Description.
 *	 This routine is invoked by the OS when an interrupt occurs
 *	 on our IRQ.  We check to make sure our adapter is the
 *	 interrupting device.  It shouldn't be anything else!  And
 *	 if so, we clear the interrupt mask so that the Interupt process routine
 *  can loop and process all pending events on one pass.
 *
 *  Entry:
 *	 Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *
 *  Exit:
 *	 None.
 *
 */

int 
LM_Service_Events(pAS)
AdapterStructure *pAS; 
{
    int				ReturnValue;
    unsigned char	SaveBank;
    unsigned char   IntStatusReg;
    unsigned short  SavePtr;
	WORD 			regvalw;
    
    
   /*
    * Check if we are already here.
    */

	if (pAS->hdw_int == 0)

    	 pAS->hdw_int = 1;
	else
    	 return (SUCCESS);

	SMC_INP(pAS->BankSelect, &SaveBank);     /* Save BankSelect Register */
	SaveBank &= (unsigned char) SelectBankMask; 

	SelectBank(0);
	SMC_INPW(pAS->Counter, &regvalw);	    /* Clear counters */

	SelectBank(0x20);
	SMC_INPW(pAS->Pointer, &SavePtr);       /* Save Pointer Register */

   /*
    *  Check interrupt mask against status.
    */

	SMC_INP(pAS->Interrupt, &IntStatusReg);
    if (IntStatusReg & ValidInts)  {
    	while (1) {
			SMC_INP(pAS->Interrupt, &IntStatusReg);

        	if (ReceiveInt & IntStatusReg){
            	if(RxInt(pAS)==SUCCESS) continue; 
				else break;		/* ReturnValue already set to SUCCESS */
			}

        	if (TransmissionInt & IntStatusReg)  {
            	if(TxInt(pAS)==SUCCESS) continue;
				else break;		/* ReturnValue already set to SUCCESS */
        	}

        	if (AllocInt & IntStatusReg)  {
				if(pAS->AllocationRequestedFlag == 1) {
            		AllocIntr(pAS);	/* Process allocation interrupts */
            		continue;
				}
        	}
    
        	if (RxOverrunInt & IntStatusReg)  {
            	RxOvrnIntr(pAS);	/* Process receive overrun interrupts */
            	continue;
        	}

        	if (EPHInt & IntStatusReg)  {
            	EPHIntr(pAS);	/* Process eph interrupts */
            	continue;
        	}
			break;	/* get here with system interrupts disabled */
    	}
		ReturnValue=SUCCESS;
	} else {
		ReturnValue=NOT_MY_INTERRUPT;
	}

   /*
    * Restore context.
    */
	SelectBank(0x20);
    SMC_OUTPW(pAS->Pointer,(WORD)SavePtr);          /* Restore Pointer */
    SMC_OUTP(pAS->BankSelect, SaveBank);		/* Restore Bank */
    pAS->hdw_int = 0;                               

    return (ReturnValue);
}

 
/*
 *  Receive Complete Interrupt.
 *
 *  Functional Description:
 *	Process incoming frame.  Bank Select set to Bank 2 on entry.
 *
 *  Entry:
 *	IN    Ptr_Adapter_Struc    pAS    pointer to adapter structure.
 *
 *  Exit:
 *	None.
 *	Interrupt is Ack'ed.
 *
 */

STATIC int	
RxInt(pAS)
AdapterStructure *pAS;
{
    unsigned int		TempPort;
    unsigned int		TempRange;
    unsigned char		*ReadBuff;
    unsigned int		PacketStatus;
    unsigned int		ByteCount;
    DWORD 				rx_status =0;
	WORD				*tmpw;

#ifdef CODE_386
    BYTE    *FarPointerToLookaheadBuffer;
#else 
	BYTE    _far *FarPointerToLookaheadBuffer;
#endif

    /* Write pointer register for rx read */

    SMC_OUTPW(pAS->Pointer,(WORD)(ReceiveArea|AutoIncrement|ReadMode));

    PreventViolatingHardwareTimings();

	/* follows the ASM ---> copying the data to look_ahead_buf */
	FarPointerToLookaheadBuffer = pAS->look_ahead_buf;

	xfer ( pAS, (DWORD)FarPointerToLookaheadBuffer, 
			(DWORD)(pAS->rx_lookahead_size << 4), (WORD)BOARD_TO_OS); 

    /* Check for errors before copying data */
	tmpw = (WORD *) FarPointerToLookaheadBuffer;
	PacketStatus = (unsigned int) *tmpw;

    if (PacketStatus & SCEC_RXErrorBits)  {
        
		if (!(pAS->receive_mask & ACCEPT_ERR_PACKETS)) {
            /* I should probably inc a counter here too*/
			SelectBank(0x20);
        	SMC_OUTPW(pAS->MMUCommand,(WORD) RemoveReleaseTopRXFifo);
		    WaitForMMUReady(pAS);
			return(0);
		}

        /* Count for statistics. */
        if (PacketStatus & SCEC_BadRxCRC)  {
		   IncUMACCounter(ptr_rx_CRC_errors);
	       rx_status |= CRCErrorBit;
        }

        if (PacketStatus & SCEC_TooLongRXFrame) {
		   IncUMACCounter(ptr_rx_too_big);
		}
		
        if (PacketStatus & SCEC_AlignmentError) {
       	   IncUMACCounter(ptr_rx_align_errors);     
	       rx_status |= CRCAlignErrorBit;
        }
    }

	tmpw++;
	ByteCount = (unsigned int) *tmpw;
    ByteCount &= ByteCountMask;     /* Zero reserved high bits */
    ByteCount -= 5;                 /* status(2) length(2) control(1) */

	/* In ASM, it uses OddFrameBitMask which is the upper byte of Rfs_Odd */
    if (!(PacketStatus & Rfs_Odd))  {   /* If evenpacket, get rid of odd byte */
        --ByteCount;
    }

	FarPointerToLookaheadBuffer += 4;

    /* Do multicast filtering */

	if (PacketStatus & SCEC_RxMulticastBit) {

	   if (!(pAS->receive_mask & (PROMISCUOUS_MODE | ACCEPT_MULTI_PROM)))  {
	   
			 if ( CheckMultiAdd(pAS) != SUCCESS ) {
			    
			     /* Didn't pass filtering, discard */
				  SelectBank(0x20);
				  SMC_OUTPW(pAS->MMUCommand,(WORD) RemoveReleaseTopRXFifo);
				  WaitForMMUReady(pAS);
				  return(0);
			}
       }
	}

    /* Um_Receive_Packet will not have to call for a lookahead if it's already done */

	UM_Receive_Packet ( FarPointerToLookaheadBuffer, ByteCount, 
							pAS, rx_status);

   /* Release the frame.  This acknowledges the interrupt. */
	SelectBank(0x20);
	SMC_OUTPW(pAS->MMUCommand,(WORD) RemoveReleaseTopRXFifo);
	WaitForMMUReady(pAS);

   return(SUCCESS);
}

/*
 *  Transmit Complete Interrupt.
 *
 *  Functional Description:
 *	  Process transmit complete interrupt.  Count frame for statistics.
 *	  Bank Select set to Bank 2.
 *
 *  Entry:
 *	  Ptr_Adapter_Struc    pAS    pointer to adapter structure.
 *	  Bank Select set to Bank 2 on entry.
 *
 *  Exit:
 *	  None.
 *	  Interrupt is Ack'ed.
 *
 *
 */

STATIC int	
TxInt(pAS)
AdapterStructure *pAS;
{
    unsigned char		PacketNumber;
    unsigned char		SavePacketNum;
    unsigned int		PacketStatus;
    unsigned int		ByteCount;
    unsigned int		UStatus;

    /*
     *  Save Packet num and put Tx packet number into reg.
     */
		
    SMC_INP(pAS->PNR_ARR, &SavePacketNum);

	SMC_INP(pAS->FifoPorts, &PacketNumber);


    do {
        
        SMC_OUTP(pAS->PNR_ARR,(BYTE) PacketNumber);

        /*
         *  Set packet pointer with autoinc and read. 
         */

        SMC_OUTPW(pAS->Pointer,(WORD) (AutoIncrement|ReadMode));

        PreventViolatingHardwareTimings();

        /* Get status */

	    SMC_INPW(pAS->Data0_1, &PacketStatus);

        if(PacketStatus & SCEC_StatisticsBits)
        {
         if (PacketStatus & SCEC_MultipleCollision) {
			IncUMACCounter(ptr_tx_mult_collisions);
		 }

         if (PacketStatus & SCEC_SingleCollision) {
	        IncUMACCounter(ptr_tx_one_collision);
	     }
		}

	    SMC_INPW(pAS->Data0_1, &ByteCount);

        /*
         *  See how much we should dec the number of packets inside the chip 
         *  to update TxPacketsInsideSCEC.
         */

/*        ((((ByteCount) &= ByteCountMask) += 255) >>= 8); */

        ByteCount &= ByteCountMask;
        ByteCount += 255;
        ByteCount >>= 8;
        ByteCount += pAS->SCECPageSize;
        ByteCount -=1;

/*        ((ByteCount) += pAS->SCECPageSize) -=1) ;*/

        Convert256ByteUnitsToSCECPages(ByteCount);
        pAS->TxPacketsInsideSCEC -= ByteCount;

        SMC_OUTPW(pAS->MMUCommand,(WORD) ReleaseSpecificPacket);

        WaitForMMUReady(pAS);

        /* Acknowledge interrupt */

        SMC_OUTP(pAS->Interrupt,(BYTE) TransmissionInt);

        /*
         *  Update statistics.
         */

	    UStatus = SUCCESS;

	    if (PacketStatus & SCEC_16Collision)

	        UStatus = MAX_COLLISIONS;

	    if (PacketStatus & SCEC_TxUnderrun) 

	        UStatus |= FIFO_UNDERRUN;


	    UStatus = UM_Send_Complete(UStatus,pAS);

        if(UStatus == EVENTS_DISABLED)  {

            SMC_OUTP(pAS->PNR_ARR,(BYTE) SavePacketNum);

            return(UStatus);
            
        }

        /*
         * Read fifo ports register to see if we should do other packets.
         */

	    SMC_INP(pAS->FifoPorts, &PacketNumber);

    }
    while(!(PacketNumber & TEmptyBitMask));


    /* Restore packet num regiter */

    SMC_OUTP(pAS->PNR_ARR,(BYTE) SavePacketNum);


    return(UStatus);

}

/*
 *
 *  Memory Allocation Interrupt.
 *
 *  Functional Description:
 *	Process memory allocation.  Start a new transmit.  Bank Select
 *	set to Bank 2.
 *
 *
 *  Entry:
 *	IN    Ptr_Adapter_Struc    pAS    pointer to adapter structure.
 *	Bank Select set to Bank 2 on entry.
 *
 *  Exit:
 *	None.
 *	Interrupt is Ack'ed.
 *
 *
 */

STATIC int	
AllocIntr(pAS)
AdapterStructure *pAS;
{
    unsigned char	PacketNumber;
	unsigned char   CurPkNum;
	BYTE	regval;
	int	i=0;

    /*
     *  Retrieve allocated packet number and discard it.
     */

    SMC_INP(pAS->PNR_ARR + 1, &PacketNumber);

    if ((PacketNumber & 0x80)==0){ 

 		SMC_INP(pAS->PNR_ARR, &CurPkNum);	/* Save current packet num */

    	SMC_OUTP(pAS->PNR_ARR, (BYTE) PacketNumber);  /* Set allocated packet */
    	SMC_OUTP(pAS->MMUCommand, ReleaseSpecificPacket);
	
		/* Poll for completion */
		do {
			SMC_INP(pAS->MMUCommand, &regval);	
		} while(regval & 1);
		
    	SMC_OUTP(pAS->PNR_ARR, (BYTE) CurPkNum);   /* Restore Current packet */
	}

	/* Mask off allocation interrupt */

    SMC_INP(pAS->Interrupt+1, &CurPkNum);            /* Sorry-didn't want more vars */
    
    SMC_OUTP(pAS->Interrupt+1,(BYTE) (CurPkNum & AllocationIntDis));

    pAS->AllocationRequestedFlag = 0;

    UM_Interrupt(pAS);
						
    return(0);
}


/*
 *
 *  Receive OverRun Interrupt
 *
 *  Functional Description:
 *	Count in statistics.  Bank Select set to Bank 2 on entry.
 *
 *
 *  Entry:
 *	IN    Ptr_Adapter_Struc    pAS    pointer to adapter structure.
 *	Bank Select set to Bank 2.
 *
 *  Exit:
 *	None.
 *	Interrupt is Ack'ed.
 *
 *
 */

STATIC int	
RxOvrnIntr(pAS)
AdapterStructure *pAS;
{


    /*
     * Update counters and acknowledge interrupt.
     */

	IncUMACCounter(ptr_rx_lost_pkts);

    SMC_OUTP(pAS->Interrupt,(BYTE) RxOverrunInt);


    /*
     * Decrement max pages for tx to appropriate value.
     */

    if((pAS->SCECMaxPagesForTx - pAS->SCEC1518InSCECPages) > pAS->SCEC1518InSCECPages)  {
        
        
        pAS->SCECMaxPagesForTx -= pAS->SCEC1518InSCECPages;
    
    } else {

        pAS->SCECMaxPagesForTx = pAS->SCEC1518InSCECPages;
        
    }

        
    *pAS->MaxPercentageOnTx =(unsigned short) (((pAS->SCECMaxPagesForTx)*100)/pAS->SCECNumberOfPages);



   return(0);
}

/*
 *
 *  EPH Interrupt.
 *
 *  Functional Description:
 *	Secondary status from the Ethernet Protocol Handler.  This
 *	interrupt is enabled to process transmitter disabled due
 *	to max collisions.  Bank Select set to Bank 2 on entry.
 *
 *
 *  Entry:
 *	Ptr_Adapter_Struc    pAS    pointer to adapter structure.
 *	Bank Select set to Bank 2.
 *
 *  Exit:
 *	None.
 *	Interrupt is Ack'ed.
 *
 *
 */

STATIC int	
EPHIntr(pAS)
AdapterStructure *pAS;
{
    unsigned short RegValue;

	SelectBank(0);

     /* 
      * Determine if there was a fatal error 
      */


	 SMC_INPW(pAS->StatusRegister, &RegValue);

    if(!(RegValue & SCEC_FatalErrors)) {

        goto EphExit;
        
    }

    if(RegValue & SCEC_CarrierSenseLost) {
        IncUMACCounter(ptr_tx_carrier_lost);
	}

    if(RegValue & SCEC_TxUnderrun) {
        IncUMACCounter(ptr_tx_underruns);
	}

    if(RegValue & SCEC_16Collision) {
        IncUMACCounter(ptr_tx_max_collisions);
	}

    if(RegValue & SCEC_LateCollision) {
        IncUMACCounter(ptr_tx_ow_collision);
	}

    /* Enable Tx and flush error bits */

	SMC_INPW(pAS->TCR, &RegValue);

	SMC_OUTPW(pAS->TCR,(WORD)(RegValue|TXENA_SetMask));

    EphExit:

    /*
     * Has to clear previous link interrupt condition.
     */
	
	SelectBank(0x10);

    SMC_INPW(pAS->Control, &RegValue);

    SMC_OUTPW(pAS->Control, RegValue & ControlClrLEMask);

    SMC_OUTPW(pAS->Control, RegValue | ControlSetLEMask);

	SelectBank(0x20);
 
    return(0);
}

/*
 *  Receive copy data from the Ethernet controller to UMAC
 *
 *
 *  Functional Description:
 *  This routine copy's the LMAC receive data to the UMAC
 *  Assume it is called under interrupt disable
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAS		Pointer to adapter structure
 *		int		length   Number of bytes to copy
 *		long 		offset   offset into data packet
 *		struct DataBuffStructure *dbuf 
 *		struct Ptr_Adapter_Struc pAS Adapter structure
 *
 *  Exit:
 *	 None.
 *
 */


int 
LM_Receive_Copy (length, offset, dbuf, pAS, mode )
DWORD                length; 
WORD                offset; 
#ifdef CODE_386
DataBufferStructure *dbuf;  
#else              
DataBufferStructure _far     *dbuf;
#endif             
AdapterStructure    *pAS;   
WORD                mode;   

{

   unsigned int x;

   mode = 0;

	SelectBank(0x20);

	/* set to point to the receive area, Autoinc and Read */
	/* and pass the media area of 4 bytes SW & BC         */

	SMC_OUTPW(pAS->Pointer,(WORD) (ReceiveArea|AutoIncrement|ReadMode)+ offset + 4);

	PreventViolatingHardwareTimings();

   /*
    *  Loop through buffer chain.
    */

	for (x=0; x < dbuf->fragment_count; x++) 
	  {
		if ( (dbuf->fragment_list[x].fragment_length & (~PHYSICAL_ADDR)) == 0)
		 continue;

		if ( (dbuf->fragment_list[x].fragment_length & (~PHYSICAL_ADDR)) >= length)
		 {
		  if (dbuf->fragment_list[x].fragment_length & PHYSICAL_ADDR)
		   xfer ( pAS, (DWORD)dbuf->fragment_list[x].fragment_ptr, 
	                              (DWORD)(length | PHYSICAL_ADDR), (WORD)BOARD_TO_OS); 
		  else
		   xfer (pAS,(DWORD)dbuf->fragment_list[x].fragment_ptr, length, BOARD_TO_OS); 

		  length = 0;
		 }
		else
		 {
		  xfer ( pAS, (DWORD)dbuf->fragment_list[x].fragment_ptr, 
		   (DWORD)	dbuf->fragment_list[x].fragment_length, BOARD_TO_OS); 
		  length -= (DWORD)((dbuf->fragment_list[x].fragment_length & (~PHYSICAL_ADDR)));
		 }
		if (length == 0) break;
	  }
	return(SUCCESS);
}

/*
 *
 *	InitErrorCounters : 
 *     Initializes error counters to point to pAS->dummy_vector if they
 *     are not initialized by UMAC.
 *                    
 */

STATIC
InitErrorCounters(pAS)
AdapterStructure *pAS;
{

  /* to avoid testing for nulll statistic counter pointers       */
  /* in the interrupt handler we initialize the null pointers to */
  /* pAS->dummy_vector                                       */

   if (!pAS->ptr_rx_lost_pkts)
	    pAS->ptr_rx_lost_pkts = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_rx_overruns)
	    pAS->ptr_rx_overruns = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_rx_CRC_errors)
	    pAS->ptr_rx_CRC_errors = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_rx_align_errors)
	    pAS->ptr_rx_align_errors = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_rx_too_big)
	    pAS->ptr_rx_too_big = (DWORD *)&pAS->dummy_vector;


   if (!pAS->ptr_tx_deferred)
	    pAS->ptr_tx_deferred = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_tx_CD_heartbeat)
	    pAS->ptr_tx_CD_heartbeat = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_tx_one_collision)
	    pAS->ptr_tx_one_collision = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_tx_mult_collisions)
	    pAS->ptr_tx_mult_collisions = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_tx_total_collisions)
	    pAS->ptr_tx_total_collisions = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_tx_max_collisions)
	    pAS->ptr_tx_max_collisions = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_tx_underruns)
	    pAS->ptr_tx_underruns = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_tx_carrier_lost)
	    pAS->ptr_tx_carrier_lost = (DWORD *)&pAS->dummy_vector;

   if (!pAS->ptr_tx_ow_collision)
	    pAS->ptr_tx_ow_collision = (DWORD *)&pAS->dummy_vector;

}

/*
 *  XFER
 *
 *
 *  Functional Description.
 *		This routine copies the data to or from the SCEC chip
 *
 *
 */

STATIC int 
xfer ( pAS, buf, length, direction)
AdapterStructure *pAS;
DWORD	buf;				/* buf is really a far pointer, but we 
								use a DWORD to defeat pointer aritmetics */
DWORD length; 
WORD direction;
{
int	x;
#ifdef CODE_386

    BYTE  *pbuf;
#else

    BYTE _far *pbuf;
#endif

	SelectBank(0x20);

	if (direction == OS_TO_BOARD) {

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
		if (length & PHYSICAL_ADDR) {	/* IsDMATx */

			/* length should be dword aligned */
            DMAxfer(pAS, buf, length, DMA_READ_TRANSFER);

        }else {                         /* IsSlaveTx */
#endif
            
    	  	SMC_repoutsw(pAS, pAS->Data0_1, buf, length >> 1);
    		pbuf = (BYTE *)buf;
    		if (length & 0x0001) 
            	SMC_OUTP(pAS->Data0_1,*(pbuf+length-1));
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
        }
#endif
	}else { /* Board to OS */
	    
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
        if (length & PHYSICAL_ADDR) {   /* DMA in */
        	    
			/* length should be dword aligned */
            DMAxfer(pAS, buf, length, DMA_WRITE_TRANSFER);

	    }else{                         /* Slave in */
#endif

            SMC_repinsw (pAS, pAS->Data0_1, buf, length >> 1);
    		pbuf =(BYTE *)buf;
    		if (length & 0x0001) 
    			SMC_INP(pAS->Data0_1, (pbuf+length-1));
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
    	}
#endif
    }

	return (0);
}



STATIC
WaitForMMUReady(pAS)
AdapterStructure *pAS;
{
	unsigned int i;
	BYTE regval;

	for (i=0; i<9;i++) {
		SMC_INP(pAS->MMUCommand, &regval);
		if((regval & MMUBusyMask) == 0) break;
	}

#ifdef DEBUG
        if (i=10) (*(long*)pAS->MMUTimeoutCount)++;
#endif
}


STATIC
EnableSCECInterrupts(pAS)
AdapterStructure *pAS;
{
	unsigned char mask;

	mask = SCECUnmaskByte;
	if (pAS->AllocationRequestedFlag)
	   mask |= AllocationIntEna;
	SMC_OUTP(pAS->Interrupt+1, (BYTE)mask);

    
}

/*
 *      PreventViolatingHardwareTimings
 *
 *      This macro does a read operation to the Pointer Register
 *      to prevent violations of the hardware timings. It is used
 *      in the following cases:
 *
 *      1) Between setting the Pointer for a Rd. operation
 *         and reading from Data Register
 *      2) Between writing to the Data Register and reloading
 *         the Pointer
 */

STATIC void
PreventViolatingHardwareTimings()
{
	BYTE regval;
	
	SMC_INP(0x61, &regval);
}


#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
STATIC int 
DMAxfer (pAS, buf, length, dma_read_write)
AdapterStructure *pAS; 
DWORD buf;
DWORD length;
WORD dma_read_write;
/*++

Routine Description:
                    Do a DMA transfer either host to board or board to host
                    depending on the direction given.

                    Note: Assumes bank 2.


Arguments:          *pas - pointer to lmstruct.
                    buf  - pointer to SMC buffer descriptor.
                    length - length of
                    dma_read_write - direction flag of dma:
                                     DMA_READ_TRANSFER   0x08 
                                     DMA_WRITE_TRANSFER  0x04 
Return Value:       SUCCESS
                    FAILURE


--*/
{

    int i=1;

#ifndef NDIS3X
    char regval;


	length &= ~PHYSICAL_ADDR;	/* Clear phys. bit */

    /* Reset DMA */
	SMC_OUTP(DMA_B_CLEAR0, (BYTE) 0);		/*	reset DMA 0-3 */
	SMC_OUTP(DMA_B_CLEAR1, (BYTE) 0);		/*	reset DMA 4-7 */
	SMC_OUTP(DMA_MODE1, (BYTE) 0xc0);		/*	place channel 4 to
													 cascade mode */
														
	/* Enable DMA chips */
	SMC_OUTP(DMA_COMM0, (BYTE) 0x10);		/*	use rotating priority */
	SMC_OUTP(DMA_COMM1, (BYTE) 0x10);		/*	use rotating priority */

	/* Init base address */
	SMC_OUTP(pAS->DMA_base_addr_dat, (BYTE)  (buf & 0x000000ff));
	SMC_OUTP(pAS->DMA_base_addr_dat, (BYTE) ((buf&0x0000ff00)>>8));
	SMC_OUTP(pAS->DMA_base_addr_dat_L, (BYTE) ((buf&0x00ff0000)>>16));
	SMC_OUTP(pAS->DMA_base_addr_dat_L+0x0400, 
				(BYTE)((buf&0xff000000)>> 24) );

	/* Init byte count */
	SMC_OUTP(pAS->DMA_byte_count_dat, (BYTE) ((length-1) & 0x00ff) );
	SMC_OUTP(pAS->DMA_byte_count_dat, (BYTE) (((length-1)&0xff00)>>8) );
	
    /* What channel do I use? */

	if(pAS->dma_channel < 4) {  /* Mode0 - default channel 0-3 */

		SMC_OUTP(DMA_MODE0, (BYTE) (pAS->dma_channel + (DMA_BLOCK+
			DMA_INCREMENT+DMA_DISABLE_AUTO+dma_read_write)) );

		SMC_OUTP(DMA_MODE0+DMA_EXTEND, (BYTE) (pAS->dma_channel 
			+ (DMA_32_BY_BYTE+DMA_TYPE_C)) );

		SMC_OUTP(DMA_REQ0, (BYTE) (pAS->dma_channel | 4) ); /* start DMA */


        /* Check status */                    

        i=0xfffff;
        do {
			SMC_INP(DMA_STAT0, &regval);		
        }
        while( !( regval & (1 << (pAS->dma_channel & 3)) ) && i-- );


	} else {    /* Mode1 - channel 4-7 */

		SMC_OUTP(DMA_MODE1, (pAS->dma_channel & 3) | (DMA_BLOCK+
			DMA_INCREMENT+DMA_DISABLE_AUTO+dma_read_write));

		SMC_OUTP(DMA_MODE1+DMA_EXTEND, (BYTE) (pAS->dma_channel & 3) + 
			(DMA_32_BY_BYTE+DMA_TYPE_C) );

		SMC_OUTP(DMA_REQ1, (BYTE) (pAS->dma_channel | 4) ); /* start DMA*/

        /* Check status */                    

        i=0xfffff;
        do {
			SMC_INP(DMA_STAT1, &regval); 
        }
        while( !( regval & (1 << (pAS->dma_channel & 3)) ) && i-- );

	}

#endif

    if (i) {

        return(SUCCESS);
        
    }else {
        
        return(0xffff);
        
    }
}    
#endif /* -REALMODE */
