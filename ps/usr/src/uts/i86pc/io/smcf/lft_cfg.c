/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)lft_cfg.c 1.3	95/08/16 SMI"

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
* File:         lft_cfg.c
*
* Description:  LMAC module - LM_GetCnfg for EISA FAST Ethernet SMC9232 adapter.
*
*
*********************
** Revision History *
*********************
*
*
* Written by:     Paul Brant
* Date:           11/23/93
*
* By         Date     Ver.   Modification Description
* ------------------- -----  --------------------------------------
* Paul_b     12/17/94 2.00   Update driver to support the LMAC SPEC
*                            more precisely.
* Scot_h     05/17/94 1.01   Consolidate the driver
* Paul_b     04/18/94 1.00   First working ethernet version
*
*
*
* $Log:   G:\sweng\src\lm9232\c_lmac\vcs\lft_cfg.cv  $
 * 
 * Changes: Make this file independent (not included by lft9232.c)
 *          Make use of lft_macr.h
 *          Change the cast type from BYTE to WORD for setting the 
 *             Full Step bit
 *
 *    Rev 1.0   23 Jan 1995 14:49:16   WATANABE
 * Initial release.
*
*
;+!/? **********************************************************************/
                                      

#ident "@(#)lft9232.c	2.00 - 95/1/17"

/* File:         lft_cfg.c
 *
 * Description:  LM_GetCnfg for 574(EFI- EISA FEAST interface)
 *
 *
 */

/*
 *
 *  GetLANAddress: read the scec register and get the 6 byte
 *                 ethernet address
 *
 *    Input:  pAS  Pointer to adapter structure
 *    output: fill in the adapter structure 6 byte physical
 *				  ethernet address (msb first)
 *
*/

#ifdef REALMODE
#include <sys/types.h>
#include "common.h"
#endif

#if ((UNIX & 0xff00) == 0x400)	/* Solaris 2.x */
#include <sys/types.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#endif /* -Solaris */

#include "lft_macr.h"
#include "lmstruct.h"
#include "lft_eq.h"
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))	/* Solaris 2.x */
#include "lft_dma.h"
#endif

void 
GetLANAddress (pAS)
AdapterStructure *pAS;
{
   SelectBank(0x10);

	/*
   read the one on the SCEC and copy it to the adapter structure
   Setup to read the SCEC's node address.  
	*/

	SMC_INPW(pAS->IA0_1, pAS->node_address);
	SMC_INPW(pAS->IA2_3, (pAS->node_address+2));
	SMC_INPW(pAS->IA4_5, (pAS->node_address+4));
}

/*
 *  Get configuration
 *
 *
 *  Functional Description:
 *  This routine get the configuration of this LMAC
 *
 *  Entry:
 *		Ptr_Adapter_Struc 	pAs		Pointer to adapter structure
 *
 *  Exit:
 *	 completion return code
 *
 */

int 
LM_GetCnfg ( pAS )
AdapterStructure *pAS;
{
	WORD slot;
	WORD found;
	WORD option_val;
	WORD InitBank;
	int StatusToReturn;
	int i;
	WORD manufid, brdid;
	BYTE regval; 
	WORD regvalw;
	
		/*
      ; get eisa io base
		*/
      pAS->io_base = (pAS->slot_num << 12) + 0x0c80;

		/*
		; read manufacturing Id at IO address (0zc80 - 0zc83)
		*/

		SMC_INP(pAS->io_base, &manufid);
		SMC_INP(pAS->io_base+1, &regval);
		manufid = (manufid << 8) | regval;

		SMC_INP(pAS->io_base+2, &brdid);
		SMC_INP(pAS->io_base+3, &regval);
		brdid = (brdid << 8) | regval;

		SMC_INP(pAS->io_base+4, &regval);

		if((manufid == (WORD) EISA_MANUF_ID) && 	/* encoded SMC ID */
			(brdid == (WORD) EISA_BRD_9232) && 		 
			((regval & B574_EBC_ENB) == (BYTE) B574_EBC_ENB)) {
		  /*
        ; set ram size, since this is an I/O based card :
        ; ram_usable and ram_base field are not initialized
		  */
			pAS->ram_size = CNFG_SIZE_128kb;
			pAS->ram_base = pAS->ram_usable = 0;

			/*
         ; get dma channel selection
			*/
			SMC_INP(pAS->io_base + B574_DMA, &regval);
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
			pAS->dma_channel = (WORD) (regval & 0x07);

			/*
			; Initialize DMA IO address lookup tables
			*/

			pAS->base_addr_dat[0] = DMA_BASE0;
			pAS->base_addr_dat[1] = DMA_BASE1;
			pAS->base_addr_dat[2] = DMA_BASE2;
			pAS->base_addr_dat[3] = DMA_BASE3;
			pAS->base_addr_dat[4] = DMA_DUMMY;
			pAS->base_addr_dat[5] = DMA_BASE5;
			pAS->base_addr_dat[6] = DMA_BASE6;
			pAS->base_addr_dat[7] = DMA_BASE7;

			pAS->DMA_base_addr_dat = pAS->base_addr_dat[pAS->dma_channel];

			pAS->base_addr_dat_L[0] = DMA_LP0;
			pAS->base_addr_dat_L[1] = DMA_LP1;
			pAS->base_addr_dat_L[2] = DMA_LP2;
			pAS->base_addr_dat_L[3] = DMA_LP3;
			pAS->base_addr_dat_L[4] = DMA_DUMMY;
			pAS->base_addr_dat_L[5] = DMA_LP5;
			pAS->base_addr_dat_L[6] = DMA_LP6;
			pAS->base_addr_dat_L[7] = DMA_LP7;

			pAS->DMA_base_addr_dat_L = pAS->base_addr_dat_L[pAS->dma_channel];

			pAS->byte_count_dat[0] = DMA_CNT0;
			pAS->byte_count_dat[1] = DMA_CNT1;
			pAS->byte_count_dat[2] = DMA_CNT2;
			pAS->byte_count_dat[3] = DMA_CNT3;
			pAS->byte_count_dat[4] = DMA_DUMMY;
			pAS->byte_count_dat[5] = DMA_CNT5;
			pAS->byte_count_dat[6] = DMA_CNT6;
			pAS->byte_count_dat[7] = DMA_CNT7;

			pAS->DMA_byte_count_dat = pAS->byte_count_dat[pAS->dma_channel];

			if (pAS->dma_channel == 4)
				pAS->dma_channel = 0xFF;
#endif /* -REALMODE && -Solaris */

			/*
			; get irq value
			*/

#ifdef BADLINT
			option_val = (regval & 0x38) >> 3;
#else
			option_val = (BYTE)(regval & 0x38) >> 3;
#endif
			switch(option_val) {
	    		case 0:
				pAS->irq_value = 0;
				break;
	    		case 1:
				pAS->irq_value = 9;
				break;
	    		case 2:
				pAS->irq_value = 3;
				break;
	    		case 3:
				pAS->irq_value = 5;
				break;
	    		case 4:
				pAS->irq_value = 7;
				break;
	    		case 5:
				pAS->irq_value = 10;
				break;
	    		case 6:
				pAS->irq_value = 11;
				break;
	    		case 7:
				pAS->irq_value = 15;
				break;
			}

			/*
         ; get rom base, size
			*/

		SMC_INP((pAS->io_base + B574_DMA + 1), &regval);
		pAS->rom_base = (DWORD)(((regval & 0xf0) << 9) | 0x0c00);

			/*
			EFI_DMA register
			*/

			SMC_INP((DWORD)pAS->io_base + B574_DMA, &regval); 
			if	((regval & B574_DMA_ROMEN) == B574_DMA_ROMEN) /* see if ROM is enabled */
				pAS->rom_size = CNFG_SIZE_64kb;
			else
				pAS->rom_size = 0;

			/* Bank 0 */
			pAS->TCR 				= pAS->io_base + 0x10 + 0;
			pAS->StatusRegister 	= pAS->io_base + 0x10 + 2;
			pAS->RCRRegister 		= pAS->io_base + 0x10 + 4;
			pAS->Counter 			= pAS->io_base + 0x10 + 6;
			pAS->MIR 				= pAS->io_base + 0x10 + 8;
			pAS->MCR 				= pAS->io_base + 0x10 + 10;
			pAS->NotUsed1        = pAS->io_base + 0x10 + 12;
			pAS->BankSelect 		= pAS->io_base + 0x10 + 14;
     
			/* Bank 1 */
			pAS->Configuration 	= pAS->io_base + 0x10 + 0;
			pAS->BaseAddress 		= pAS->io_base + 0x10 + 2;
			pAS->IA0_1 				= pAS->io_base + 0x10 + 4;
			pAS->IA2_3 				= pAS->io_base + 0x10 + 6;
			pAS->IA4_5 				= pAS->io_base + 0x10 + 8;
			pAS->General 			= pAS->io_base + 0x10 + 10;
			pAS->Control 			= pAS->io_base + 0x10 + 12;
			pAS->NotUsed2 			= pAS->io_base + 0x10 + 14;

			/* Bank 2 */
			pAS->MMUCommand 		= pAS->io_base + 0x10 + 0;    
			pAS->PNR_ARR 			= pAS->io_base + 0x10 + 2;
			pAS->FifoPorts 		= pAS->io_base + 0x10 + 4;
			pAS->Pointer 			= pAS->io_base + 0x10 + 6;
			pAS->Data0_1 			= pAS->io_base + 0x10 + 8;
			pAS->Data2_3 			= pAS->io_base + 0x10 + 10;
			pAS->Interrupt 		= pAS->io_base + 0x10 + 12;
			pAS->NotUsed3 			= pAS->io_base + 0x10 + 14;

			/* Bank 3 */
			pAS->MT0_1 				= pAS->io_base + 0x10 + 0;           
			pAS->MT2_3 				= pAS->io_base + 0x10 + 2;
			pAS->MT4_5 				= pAS->io_base + 0x10 + 4;
			pAS->MT6_7 				= pAS->io_base + 0x10 + 6;
			pAS->NotUsed4 			= pAS->io_base + 0x10 + 8;
			pAS->Revision 			= pAS->io_base + 0x10 + 10;
			pAS->ERCV 				= pAS->io_base + 0x10 + 12;

		  /*
        ;
        ; get media type at bank 1 reg 0
        ;
		  */

		SelectBank(0x10);

			SMC_INPW(pAS->Configuration, &regvalw);
			if ((regvalw & 0x8000) == 0x8000)	/* MIISel set ? */
				pAS->media_type = MEDIA_STP100_UTP100;	/* 100 Mbps */
			else
			  {	
				pAS->media_type = MEDIA_UTP;		/* 10 Mbps */
				SMC_OUTPW(pAS->Configuration, (WORD) (regvalw | 0x0400));
										/* req'd by board design
											set FULL-STEP bit to 1 for 10Mbps */
			  }

		  /*
        ; switch back to original bank
		  */
		SelectBank(0);

		  /*
        ;
        ; set adapter_bus = SLAVE mode
        ;     adapter_flags ...
        ;
		  */

        pAS->adapter_bus = BUS_EISA32S_TYPE;
        pAS->adapter_flags |= IO_MAPPED;
		  if (pAS->dma_channel != 0xff)
           pAS->adapter_flags |= USES_PHYSICAL_ADDR;

		 /*
       ; 
       ; set board_id, board_id2, extra_info, extra_info2
       ;     media_menu(EZSTART).
       ; 
       ; Currently, hardcoded it.  
       ; In the future could change to reading from EEROM.
       ; 
		 */

       pAS->board_id = 0xC133;
       pAS->board_id2 = 0x1230;
       pAS->extra_info = 0x8006;
       pAS->extra_info2 = 0x0;

       pAS->media_menu = 0x06;

		 /*
        ;
        ; load name	
        ;
		 */

		 pAS->adapter_name[0]='9';
		 pAS->adapter_name[1]='2';
		 pAS->adapter_name[2]='3';
		 pAS->adapter_name[3]='2';
		 pAS->adapter_name[4]='D';
		 pAS->adapter_name[5]='S';
		 pAS->adapter_name[6]='T';
		 pAS->adapter_name[7]=' ';
		 pAS->adapter_name[8]=0;
		 pAS->adapter_name[9]=0;
		 pAS->adapter_name[10]='$';
		 pAS->adapter_name[11]=0;

		 /*
        ; set up board info
		 */

       pAS->nic_type = NIC_C100_CHIP;
       pAS->bic_type = BIC_574_CHIP;
        
       GetLANAddress (pAS);

       return (ADAPTER_AND_CONFIG);
		}
#ifndef REALMODE
		else return (ADAPTER_NOT_FOUND);
#else
		else return (SM_ADAPTER_NOT_FOUND);
#endif
}

