/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)smceucfg.c 1.3	95/08/16 SMI"

/*
 * Module: GetCnfg for SMC Ethernet/Token LAN Adapters
 * Project: UMAC/LMAC for UNIX SVR3.2
 *
 *
 *		Copyright (c) 1993 by Standard MicroSystems Corporation
 *		All rights reserved.  Contains confidential information and
 *		trade secrets proprietary to
 *			Standard MicroSystems Corporation
 *			6 Hughes
 *			Irvine, California  92718
 *
 */


#ident "@(#)GetCnfg.c	1.05 - 94/12/09"

/*
 *
 * Revision History:
 *
 * 1.05   S_HUI 12/09/94 Add longer delay for soft reset problem
 * 1.04   S_HUI 08/30/94 Add fix for soft reset problem
 * 1.03   S_HUI 05/20/94 Clean up and make it only for EISA adapters
 * 1.02   S_HUI	02/23/94 Add Master/Slave check 
 * 1.01   T_NGO 08/10/93 Add EISA
 * 1.00   T_NGO 06/10/93 Created C version of GetCnfg.asm
 *
 *
 */

#include	<sys/types.h>

#ifdef REALMODE
#include	"common.h"
#include	"sebm.h"
#endif

#if ((UNIX & 0xff00) == 0x400)	/* Solaris 2.x */
#include	<sys/ddi.h>
#include	<sys/sunddi.h>
#include	"sebm.h"
#endif /* Solaris */

#include	"lmstruct.h"
#include	"board_id.h"
#include	"smchdw.h"
#include 	"ebm.h"

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
#ifdef LM_STREAMS
#include	<sys/stream.h>
#ifdef DLPI
#include	"sys/macstat.h"
#endif
#endif /* LM_STREAMS */
#ifdef	EBM
#include	"sys/sebm.h"
#else
#include	"sys/smc.h"
#endif
#include	"sys/lmstruct.h"
#include	"sys/board_id.h"
#include	"sys/smchdw.h"
#include 	"sys/ebm.h"
#endif /* -REALMODE && -Solaris */

STATIC
char  VerGetCnfg[] =
{"@(#) GetCnfg.c version 1.04a 08/30/94\nCopyright (C) Standard Microsystems Corp, 1993"};


static unsigned char IrqBrd8[] = {0, 0x2, 0x3, 0x4, 0x5, 0x6,0,0};
static unsigned char IrqBrd16[] = {0, 0x9, 0x3, 0x5, 0x7, 0x0A,0x0B,0x0F};

STATIC
LM_STATUS
LM_GetEISA_Config(pAd)
Ptr_Adapter_Struc pAd;

/*++

Routine Description:

    This routine will get the configuration information about the card.


Arguments:

    pAd - A pointer to an LMI adapter structure.

Return:

    ADAPTER_AND_CONFIG,
    ADAPTER_NO_CONFIG,
    ADAPTER_NOT_FOUND

--*/
{

	 UCHAR bytevalue;
	 USHORT wordvalue;


	/*
	// Get SMCs' ID and board ID and verify
	*/


	 bytevalue = inp(pAd->io_base);

	 wordvalue = bytevalue;

	 wordvalue <<= 8;

	 bytevalue = inp(pAd->io_base+EISA_ID2);

	 

	 if((wordvalue |= bytevalue) != MANUF_ID) {

#ifdef SEBMDEBUG
	 printf("\n Config Error! read_man_id: %x MAN_ID: %x",wordvalue,MANUF_ID);
#endif
	   	/*
		// Error
		*/


#ifndef REALMODE
		return ADAPTER_NOT_FOUND;
#else
		return SM_ADAPTER_NOT_FOUND;
#endif

	}


	 wordvalue = inp(pAd->io_base+EISA_ID3);

	 wordvalue <<= 8;


	 bytevalue = inp(pAd->io_base+EISA_ID4);



	 if((wordvalue |= bytevalue) !=  EISA_BRD_ID) {

	   	/*
		// Error
		*/

#ifdef SEBMDEBUG
	 printf("\n Config Error! read_brd_id: %x EISA_BID: %x",wordvalue,EISA_BRD_ID);
#endif

#ifndef REALMODE
		return ADAPTER_NOT_FOUND;
#else
		return SM_ADAPTER_NOT_FOUND;
#endif

	 }

    pAd->board_id = wordvalue | NIC_790_BIT ;

    if(!(inp(pAd->io_base+EISA_EBC) & EISA_ENB))
    { 
#ifdef SEBMDEBUG
	 printf("\n Config Error!");
#endif
#ifndef REALMODE
		return ADAPTER_NOT_FOUND;
#else
		return SM_ADAPTER_NOT_FOUND;
#endif
    }

	/*
	// Get irq value
	*/

	bytevalue = inp(pAd->io_base+EISA_INT);

	 
	pAd->irq_value = IrqBrd16[bytevalue & EISA_IRQ_MASK];


	/*
	// Get ROM info
	*/



	 bytevalue = inp(pAd->io_base+EISA_INT);

         pAd->rom_size = (bytevalue & EISA_ROMEN) ? 64 : 0; 
	 
    /*
     * Get ROM Base
     */

#ifdef BUG
	 pAd->rom_base = 0x0C0000 | ((inp(pAd->io_base+EISA_ROM) & EISA_ROM_MASK) << 9);
#else
	 pAd->rom_base = 0x0C0000 | ((ulong)(inp((ushort)(pAd->io_base+EISA_ROM)) & EISA_ROM_MASK) << 9);
#endif


	/*
	// Get RAM base
	*/


#ifdef BUG
    pAd->ram_base = 0x080000 | ((inp(pAd->io_base+EISA_RAM) & EISA_RAM_MASK) << 11);
#else
    pAd->ram_base = 0x080000 | ((ulong)(inp((ushort)(pAd->io_base+EISA_RAM)) & EISA_RAM_MASK) << 11);
#endif

    /*
    // Set defaults for LM
    */
    pAd->nic_type= NIC_790_CHIP;
    pAd->bic_type= BIC_571_CHIP;
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
    pAd->adapter_bus = BUS_EISA32M_TYPE;	/* Master */
#else
    pAd->adapter_bus = BUS_EISA32S_TYPE;    /* Solaris uses Slave mode */
#endif

    pAd->adapter_flags = 0;


/* INIT in UMAC
    	pAd->num_of_tx_buffs = NUM_TX_BUFFERS;
*/


    	pAd->xmit_buf_size = 0x600;  /* A multiple of 256 > 1514. */

	pAd->ram_size = 32;
	pAd->ram_usable = 8;
	pAd->page_offset_mask = 0x1fff;

#ifdef SEBMDEBUG
    printf("\nIo 0x%x, Irq 0x%x, RamBase 0x%x, RamSize 0x%x\n",
                 pAd->io_base,
                 pAd->irq_value,
                 pAd->ram_base,
                 pAd->ram_size
                 );
#endif

	Check790(pAd);		/* [1.04] */

	return(ADAPTER_AND_CONFIG);

}


/*** [1.04] ***/
/*
 *
 * Check790   Checks to see if the 790 got hosed and resets it if it is.
 *
 *
 * Return: 
 *      SUCCESS
 *
 */
STATIC
Check790(pAd)
Ptr_Adapter_Struc pAd;
{
	unsigned char RegValue;
	unsigned char RegValue1;
#ifdef BUG
	int i;
#else
	long i;
#endif

	RegValue = inp(pAd->io_base+EISA_790+REG_REV);
	if(RegValue==0x21) 
#ifdef BADLINT
		return;
#else
		return(1);
#endif

	RegValue = inp(pAd->io_base+EISA_STAT);
	outp(pAd->io_base+EISA_STAT, RegValue | EISA_RNIC);

#ifdef BUG
	for(i=0;i<0x0a000; i++) {	/* delay loop */
#else
	for(i=0;i<0x0a000L; i++) {	/* delay loop */
#endif
		RegValue1=inp(pAd->io_base+EISA_STAT);
	}

	outp(pAd->io_base+EISA_STAT, RegValue & ~EISA_RNIC);

	i=0;
	while(inp(pAd->io_base+EISA_790+REG_EER) & EER_RC) {
#ifdef BUG
		if(i>0x0a000) break;
#else
		if(i>0x0a000L) break;
#endif
		i++;
	}

#ifdef BUG
	for(i=0;i<0x0a000; i++) {	/* [1.05] --- delay loop */
#else
	for(i=0;i<0x0a000L; i++) {	/* [1.05] --- delay loop */
#endif
		RegValue1=inp(pAd->io_base+EISA_STAT);
	}

	/* reset EISA config defaults */
	RegValue=inp(pAd->io_base);
	RegValue=inp(pAd->io_base+1);
	outp(pAd->io_base+EISA_790+REG_BPR, BPR_M16EN+BPR_BMSTR);
	outp(pAd->io_base+EISA_790+REG_HWR, HWR_SWH+HWR_ETHER);
	outp(pAd->io_base+EISA_790+REG_GCR, 
			GCR_IR2+GCR_0WS+GCR_IR1+GCR_IR0+GCR_LITE);
	/* switch it back */
	outp(pAd->io_base+EISA_790+REG_HWR, HWR_ETHER);

#ifdef BADLINT
	return;
#else
	return(SUCCESS);
#endif
}

/*
 *
 * GetAdapterNamePtr    Get board name from its own ID
 *
 *
 * Return: 
 *      SUCCESS
 *
 */
STATIC
GetAdapterNamePtr(pAd)
Ptr_Adapter_Struc pAd;
{
	char *board_name;

	switch (pAd->board_id & STATIC_ID_MASK) {
	case S82M32:
        if(pAd->adapter_bus == BUS_EISA32S_TYPE)	/* Slave */
			board_name = "82S32   ";
		else
			board_name = "82M32   ";
		break;
	default:
		board_name = "UNKNOWN ";
		break;
	}
#ifdef SMEDEBUG
#ifndef SCO
   init_msg(board_name);
#endif
#endif
#ifndef REALMODE
#ifdef BADLINT
   bcopy(board_name, pAd->adapter_name,10);
#else
   bcopy(board_name, (caddr_t)pAd->adapter_name,10);
#endif
#else
   {
	int i;
	for (i = 0; i < 10; i++)
		pAd->adapter_name[i] = board_name[i];
   }
#endif
   return(SUCCESS); 
}

/*
 * this stores the LAN address from the adapter board into the char string
 *	if the first three bytes do not reflect that of a Western Digital
 *	adapter, it will return an error
 */

int
Get_Node_Address(address, char_ptr)
#ifdef BADLINT
int   		address;
#else
unsigned short	address;
#endif
unsigned char  *char_ptr;
{
	/* Always return the LAN address and do not check for WD node address */
	register int i;
	UCHAR	SaveValue;
#ifdef EBM
	address +=EISA_790;
#endif
	SaveValue = inb(address + REG_HWR);
#ifdef BADLINT
	outb(address + REG_HWR,SaveValue & ~HWR_SWH);
#else
	outb(address + REG_HWR,(unchar)(SaveValue & ~HWR_SWH));
#endif
	
	for(i=0;i<6;i++)
	 *char_ptr++ = inb (address+REG_LAR0+i);
        
	outb(address + REG_HWR,SaveValue);
	return (0);
}


/*
 *
 * GetCnfg    Get all configuration Information for 
 *            the Interface chips -- 583,584,593 and 594
 *
 *
 * Return: 
 *      SUCCESS, get configuration successfully.
 *      (i.e see return code in lmstruct.h).
 *
 *      All configuration information is put into
 *      the configuration structure. 
 *
 */

LM_STATUS
LMB_GetCnfg(pAd)
Ptr_Adapter_Struc pAd;

{
	LM_STATUS StatusToReturn;
	
	if(pAd->pc_bus == EISA_BUS) {
#ifndef BUG
		/* We don't know if the UM will give us z000 or zc80,
		 * so we compute it using the slot number
		 */
		if (pAd->slot_num)
			pAd->io_base = pAd->slot_num * 0x1000 + 0xc80;
#endif
		StatusToReturn = LM_GetEISA_Config(pAd);
		GetAdapterNamePtr(pAd);
		Get_Node_Address(pAd->io_base,pAd->perm_node_address);
	} else
#ifndef REALMODE
		StatusToReturn = ADAPTER_NOT_FOUND;
#else
		StatusToReturn = SM_ADAPTER_NOT_FOUND;
#endif

	return(StatusToReturn);


}
