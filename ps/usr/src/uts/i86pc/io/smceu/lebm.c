/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)lebm.c 1.3	95/07/19 SMI"

/*
 * Module: LEBM (lebm.c) Lower Mac for SMC82M32 Ethernet LAN Adapters
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


#ident "@(#)lebm.c	1.07 - 94/11/14"

/*
 *
 * Revision History:
 *
 * 1.07   S_HUI 11/14/94 (i) Allow a work around for padding problem
 *                            (Novell ODI --- padlen = 1)
 *                       (ii) Add 2 more Bad motherboard id
 * 1.06   S_HUI 08/30/94 (i) Add 8 more Bad motherboard id
 *                       (ii) Add LMB_AutoMediaDetect to support AMD
 *              09/27/94 (iii) Add 2 more Bad motherboard id
 * 1.05   S_HUI 05/27/94 Add LMB_Ram_Test for DMA test
 *                       Move the DMA test to init time (only did once)
 *                       Update the DMA test as the one in ASM LMAC
 * 1.04   T_NGO	03/01/94 Add work around on Xmit DMA
 * 1.03   S_HUI	02/23/94 Add Master/Slave check
 * 1.02   T_NGO 09/29/93 Support EBM
 * 1.01   T_NGO 07/22/93 Add UMAC/LMAC Interface
 * 1.00   T_NGO 06/10/93 Created
 *
 *
 */


/*
 * Streams driver for SMC80xx/82xx Ethernet controller
 * Implements an extended version of the AT&T Link Interface
 * IEEE 802.2 Class 1 type 1 protocol is implemented and supports
 * receipt and response to XID and TEST but does not generate them
 * otherwise.
 * Ethernet encapsulation is also supported by binding to a SAP greater
 * than 0xFF.
 */

#ifdef BUG
#include "sys/types.h"
#else
#include <sys/types.h>
#endif

#ifdef REALMODE
#include "common.h"
extern void     repinsw(ushort, char far *, ushort);
extern void     repoutsw(ushort, char far *, ushort);
#endif /* REALMODE */

#if ((UNIX & 0xff00) == 0x400)	/* Solaris 2.x */
#include <sys/ddi.h>
#include <sys/sunddi.h>
#endif

#ifdef LM_STREAMS
/* Stuff needed for obsolete STREAMS handling */
#include "sys/param.h"
#include "sys/sysmacros.h"
#ifdef AT286
#include "sys/mmu.h"
#include "sys/seg.h"
#endif
#ifndef M_XENIX
#include "sys/immu.h"
#endif
#include "sys/stream.h"
#ifndef M_XENIX
#include "sys/stropts.h"
#endif
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/conf.h"
#include "sys/user.h"
#include "sys/errno.h"
#ifdef	DLPI
#include "sys/dlpi.h"
#include "sys/macstat.h"
#else
#include "sys/lihdr.h"
#endif
/* #ifndef SCO */

#ifndef M_INTR
#include "sys/inline.h"
#else 
static int splvar;
#define intr_disable() splvar=spl6();
#define intr_restore() splx(splvar);
#endif

#ifdef LAI_TCP
#include "sys/socket.h"
#include "net/if.h"
#endif
#include "sys/sebm.h"
#include "sys/board_id.h"
#include "sys/smchdw.h"
#include "sys/lmstruct.h"
#include "sys/ebm.h"


#include "sys/cmn_err.h"
#include "sys/strlog.h"
#endif /* LM_STREAMS */

#ifndef LM_STREAMS
#include "sebm.h"
#include "board_id.h"
#include "smchdw.h"
#include "lmstruct.h"
#include "ebm.h"
#endif

#ifdef DEADCODE
extern int sebm_boardcnt;			/* number of boards */
extern int sebm_multisize;		/* number of multicast addrs/board */
extern struct sebmparam sebmparams[];	/* board specific parameters */
#endif
#ifndef LM_STREAMS
extern struct sebmdev sebmdevs[];		/* queue specific parameters */
#endif
extern struct sebmstat sebmstats[];		/* board statistics */
#ifdef DLPI
extern struct mac_stats ebmacstats[];	/* Driver MAC statistics */
#endif
#ifdef	DEADCODE
extern struct sebmmaddr sebmmultiaddrs[];	/* storage for multicast addrs */
extern struct initparms	sebminitp[];	/* storage for initialization values*/
extern int call_sebmsched;


extern unsigned char sebmhash();
#endif


extern int sebm_debug;

  /*
   * initialize the necessary data structures for the driver
   * to get called.
   * the hi/low water marks will have to be tuned
   */

#ifdef LM_STREAMS
extern sebmsched();
#endif
extern UMB_Send_Complete();

/* streams dev switch table entries */

#ifdef M_XENIX
#include "sys/machdep.h"
#endif


#ifdef LAI_TCP
extern struct ifstats *ifstats;
extern struct ifstats sebms_ifstats[];
extern int sebmboardfirst[];
#endif

STATIC
char  VerLEBM[] =
{"@(#) lebm.c version 1.07a 11/14/94\nCopyright (C) Standard Microsystems Corp, 1994"};

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
/*** MSCHK ***/
int mschkcnt = 0x2000;
int mschktsz = 0x200;
int *mschkbuf;
#ifdef BUG
int mschkpttn[4] = {0x55aa55aa, 0xaa55aa55, 0x5a5a5a5a, 0xa5a5a5a5};
#else
ulong mschkpttn[4] = {0x55aa55aa, 0xaa55aa55, 0x5a5a5a5a, 0xa5a5a5a5};
#endif

#ifdef BUG
int sdmafail=0;
#else
ulong sdmafail=0;
#endif

#ifdef BUG
static int badmid[] = {0xf130d425,
#else
static ulong badmid[] = {0xf130d425,
#endif
                       0x0911a310,
                       0x05002335,	/* Micronics Pentinum EISA/PCI */
                       0x04002335,  /* Micronics 486 EISA/PCI */
                       0xc1aac94d,  /* Siemens */
                       0x0170335f,  /* Wyse 7000i */
                       0x4200ac10,  /* Dell Power Edge SP/466 */
                       0x81c0f022,  /* HP Netserver LC */
                       0x71c0f022,  /* HP Netserver LF */
                       0x1915110e,  /* Compaq ProLiant */
                       0x4800ac10,  /* Dell Omniplex */
                       0x2105110e}; /* Compaq Deskpro XL */
static int nbadmid=sizeof(badmid)/sizeof(int);
#endif /* -REALMODE && -Solaris */

#ifdef REALMODE
#define binit_msg(x) putstr((x))
#define binit_number(x) put2hex((unchar)(x))
#define binit_addr(x) putptr((char far *)(x))
#endif

#if ((UNIX & 0xff00) == 0x400)	/* Solaris 2.x */
#define binit_msg(y) printf("%s", (y))
#define binit_number(y) printf("%x", (unchar)(y))
#define binit_addr(y) printf("%x", (long)(y))
#endif

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
void
binit_msg ( str )
char *str;
{
int x;
for (x=0;str[x] != 0;x++) 
	putchar (str[x]);
}

void
binit_number ( numb )
unsigned char numb;
{
  unsigned char blah;

blah = numb>>4;
switch ( blah ) {
 case 0 : putchar ('0');
	break;
 case 1 : putchar ('1');
	break;
 case 2 : putchar ('2');
	break;
 case 3 : putchar ('3');
	break;
 case 4 : putchar ('4');
	break;
 case 5 : putchar ('5');
	break;
 case 6 : putchar ('6');
	break;
 case 7 : putchar ('7');
	break;
 case 8 : putchar ('8');
	break;
 case 9 : putchar ('9');
	break;
 case 0xa : putchar ('A');
	break;
 case 0xb : putchar ('B');
	break;
 case 0xc : putchar ('C');
	break;
 case 0xd : putchar ('D');
	break;
 case 0xe : putchar ('E');
	break;
 case 0xf : putchar ('F');
	break;
}

blah = numb & 0x0f;
switch ( blah ) {
 case 0 : putchar ('0');
	break;
 case 1 : putchar ('1');
	break;
 case 2 : putchar ('2');
	break;
 case 3 : putchar ('3');
	break;
 case 4 : putchar ('4');
	break;
 case 5 : putchar ('5');
	break;
 case 6 : putchar ('6');
	break;
 case 7 : putchar ('7');
	break;
 case 8 : putchar ('8');
	break;
 case 9 : putchar ('9');
	break;
 case 0xa : putchar ('A');
	break;
 case 0xb : putchar ('B');
	break;
 case 0xc : putchar ('C');
	break;
 case 0xd : putchar ('D');
	break;
 case 0xe : putchar ('E');
	break;
 case 0xf : putchar ('F');
	break;
}

}
void
binit_addr ( addr )
#ifdef BUG
unsigned long;
#else
unsigned long addr;
#endif
{
  binit_number ( (addr>>24));
  binit_number ( (addr>>16)&0x00ff);
  binit_number ( (addr>>8)&0x0000ff);
  binit_number ( (addr&0x000000ff));

}
#endif /* -REALMODE && -Solaris */


LM_STATUS
LMB_Enable_Adapter(pAd)
Ptr_Adapter_Struc pAd;
/*++

Routine Description:

    This routine will enable interrupts on the card.

Arguments:

    Adapt - A pointer to an LME adapter structure.


Return:

    SUCCESS

--*/

{
    ULONG BoardInterface = (pAd->board_id & INTERFACE_CHIP_MASK);
	 UCHAR RegValue;
	 
	 RegValue = inb(pAd->io_base+EISA_STAT);
/*
	 outb (pAd->io_base+EISA_STAT,RegValue | EISA_EIL | EISA_DIEN);
*/
#ifdef BADLINT
	 outb (pAd->io_base+EISA_STAT,RegValue | EISA_EIL);
#else
	 outb (pAd->io_base+EISA_STAT,(unchar)(RegValue | EISA_EIL));
#endif
	 Enable_585_irq(pAd);

	 pAd->adapter_flags &= ~ADAPTER_DISABLED;
	 return(SUCCESS);

}

LM_STATUS
LMB_Disable_Adapter(pAd)
Ptr_Adapter_Struc pAd;
/*++

Routine Description:

    This routine will disable interrupts on the card.

Arguments:

    Adapt - A pointer to an LME adapter structure.


Return:

    SUCCESS

--*/

{
    ULONG BoardInterface = (pAd->board_id & INTERFACE_CHIP_MASK);
	 UCHAR RegValue;

	 Disable_585_irq(pAd);
	 
	 RegValue = inb(pAd->io_base+EISA_STAT) & 0x7F ;

	 outb (pAd->io_base+EISA_STAT,RegValue);

	 pAd->adapter_flags |= ADAPTER_DISABLED;
	 return(SUCCESS);

}

LMB_Initialize_Adapter(pAd)
Ptr_Adapter_Struc pAd;
/*++

Routine Description:

    This routine will do a hardware reset, self test, and initialization of
    the adapter.

    The node_address field (if non-zero) is copied to the card, and if
    zero, is copied from the card.  The following fields must be set by
    the UM : base_io, ram_size, ram_access, node_address, max_packet_size,
    buffer_page_size, num_of_tx_buffs, and receive_mask.

Arguments:

    Adapt - A pointer to an LME adapter structure.


Return:

    SUCCESS/FAIL

--*/
{
   register int i,splval;
#ifndef BUG
   register long l;
#endif
   register UCHAR SavValue; /* Sat */
   register UCHAR RegValue;
   register USHORT XmitSize;
   BOOLEAN NoLoad;
   int j;

   short ctl_reg, cmd_reg;
	
   /* Non loopback + 8 bytes FIFO + 16bit Xfer */

   unsigned char init_dcr = DCR_LS|DCR_FT1|DCR_WTS;

   pAd->adapter_status = NOT_INITIALIZED;

   ctl_reg = pAd->io_base + EISA_790;
   cmd_reg = ctl_reg + REG_CMD;

#if defined(SEBMDEBUG)
   if (sebm_debug > 30)
   printf("Initializing SMC Ethernet LAN Adapter at 0x%x ...",ctl_reg);
#endif
   
   /* reset the 8003 & program the memory decode bits */
   RegValue = inb(ctl_reg) & 0x3F;
   
#ifdef BADLINT
   outb(ctl_reg, RegValue | MSR_RST);
#else
   outb(ctl_reg, (unchar)(RegValue | MSR_RST));
#endif

   for (i=0;i<0x10;i++)
      RegValue = inb(ctl_reg) & 0x3F;
   outb(ctl_reg, RegValue);

   /*
   // Enable Ram
   */

   RegValue = inb(pAd->io_base+EISA_INT);

#ifdef BADLINT
   outb(pAd->io_base+EISA_INT,RegValue | EISA_MENB);
#else
   outb(pAd->io_base+EISA_INT,(unchar)(RegValue | EISA_MENB));
#endif

   inb(pAd->io_base+EISA_INT);		/* Sat */

#ifdef BADLINT
   outb(pAd->io_base+EISA_INT,RegValue | EISA_MENB); /* Sat some pentium */
#else
   outb(pAd->io_base+EISA_INT,(unchar)(RegValue | EISA_MENB)); /* Sat some pentium */
#endif


    /*
    // Load LAN Address
    */

    NoLoad = FALSE;

    for (i=0; i < 6; i++) {

        if (pAd->node_address[i] != (UCHAR)0) {

            NoLoad = TRUE;

        }

    }
	/*
	 *	 Make LAN node addr visible
	 */
	
    RegValue = inp(ctl_reg+REG_HWR) & ~HWR_SWH;

    outp(ctl_reg + REG_HWR,RegValue);

    for (i=0; i < 6; i++) {

        /*
        // Read from IC
        */
		  pAd->perm_node_address[i]=inb(ctl_reg+REG_LAR0+i);

    }

    if (!NoLoad) {

        for (i=0; i < 6 ; i++) {

            pAd->node_address[i] = pAd->perm_node_address[i];
        }

    }

   outb(cmd_reg,CMD_PAGE0 + CMD_RD2 + CMD_STP); /* 0+0x20+0x01 */
#ifdef BUG
   for (i=0;i<0x10000;i++)
#else
   for (l=0;i<0x10000;i++)
#endif
     if(inb(ctl_reg+REG_ISR) & ISR_RST)
	break;
#ifdef BUG
   if(i==0x10000)
#else
   if(l==0x10000)
#endif
     return(INITIALIZE_FAILED);

   outb(ctl_reg + REG_DCR, init_dcr);
   outb(ctl_reg + REG_RBCR0, 0);
   outb(ctl_reg + REG_RBCR1, 0);

   LMB_Change_Receive_Mask(pAd);

   outb(ctl_reg + REG_TCR, TCR_LB1);  /* LoopBack mode2 */

	if( (pAd->num_of_tx_buffs > 2) || (pAd->num_of_tx_buffs == 0))

	 	pAd->num_of_tx_buffs = 2;


   XmitSize =(USHORT)(pAd->num_of_tx_buffs*TX_BUF_LEN) >> 8;

   outb(ctl_reg + REG_PSTART,(UCHAR)XmitSize);
   outb(ctl_reg + REG_BNRY, (UCHAR)XmitSize);

   pAd->rcv_start =(caddr_t)(pAd->ram_access + (XmitSize<<8));	/* skip past transmit buffer */
   pAd->rcv_stop =(caddr_t)(pAd->ram_access + (ULONG)(pAd->ram_size * 1024));	/* want end of memory */
   pAd->StartBuffer = (UCHAR)(XmitSize);
   pAd->LastBuffer = (UCHAR)(pAd->ram_size << 2);

#ifdef BADLINT
   outb(ctl_reg + REG_PSTOP, pAd->ram_size << 2);
#else
   outb(ctl_reg + REG_PSTOP, (unchar)(pAd->ram_size << 2));
#endif

   outb(ctl_reg + REG_ISR, 0xFF);
   outb(ctl_reg + REG_IMR, pAd->InterruptMask);

#if defined(SEBMDEBUG)
     if(sebm_debug > 50)
     printf("\nLM_Init: NIC Int mask 0x%x=%x\n",ctl_reg+REG_IMR,pAd->InterruptMask);
#endif

   RegValue = inb(cmd_reg);
#ifdef BADLINT
   outb(cmd_reg, (RegValue & PG_MSK & ~CMD_TXP) | CMD_PAGE1);
#else
   outb(cmd_reg, (unchar)((RegValue & PG_MSK & ~CMD_TXP) | CMD_PAGE1));
#endif
   for (i = 0; i < LLC_ADDR_LEN; i++)
     outb(ctl_reg + REG_PAR0 + i, pAd->node_address[i]);
   /*
    *  clear the multicast filter bits
    *  in LM_Change_Receive_Mask
    */

#ifdef BADLINT
   outb(ctl_reg + REG_CURR, XmitSize + 1);
#else
   outb(ctl_reg + REG_CURR, (unchar)(XmitSize + 1));
#endif
#if defined(SEBMDEBUG)
   if (sebm_debug > 70)
   printf("RegCurr 0x%x = 0x%x",ctl_reg+REG_CURR,inb(ctl_reg+REG_CURR));
#endif
   pAd->sebm_nxtpkt = (UCHAR)(XmitSize + 1);

   SavValue = inp(ctl_reg+REG_HWR) | HWR_SWH;	 /* Select Conf Regs */

   outp(ctl_reg + REG_HWR,SavValue); /* Sat */

   outb(ctl_reg + REG_RAR, 0);		/* Sat */



	/*
	 *	 Select IRQ 7 and 0WS on 790 
         */

   RegValue = inb(ctl_reg +REG_GCR);
#ifdef BADLINT
   outb(ctl_reg + REG_GCR, RegValue|GCR_IR2|GCR_IR1|GCR_IR0|GCR_0WS);
#else
   outb(ctl_reg + REG_GCR, (unchar)(RegValue|GCR_IR2|GCR_IR1|GCR_IR0|GCR_0WS));
#endif

   RegValue = inb(ctl_reg + REG_BPR);

#ifdef BADLINT
   outb(ctl_reg + REG_BPR, RegValue | BPR_M16EN);
#else
   outb(ctl_reg + REG_BPR, (unchar)(RegValue | BPR_M16EN));
#endif

   outb(cmd_reg, CMD_STP+CMD_RD2);	   /* Stop NIC Sel Page 0 */

   outb(ctl_reg + REG_MSR, MSR_MENB);	/* Enable share ram */
   inb(ctl_reg +REG_MSR);		/* Sat */
   outb(ctl_reg + REG_MSR, MSR_MENB);	/* Sat Enable share ram */


   outp(ctl_reg + REG_HWR,SavValue & ~HWR_SWH); /* Sat */ 

   if(pAd->rx_lookahead_size < DEFAULT_ERX_VALUE)
     pAd->rx_lookahead_size = DEFAULT_ERX_VALUE;
   
   outb(ctl_reg + REG_ERWCNT, pAd->rx_lookahead_size);

   pAd->BaseRamWindow = (UCHAR)((pAd->ram_base & 0x7E000) >> 11);
#if defined(SEBMDEBUG)
     printf("\nLM_Init: BRW 0x%x\n",pAd->BaseRamWindow);
#endif

   /*** MSCHK ***/
   for(j=0; j<3; j++) {
      SyncSetWindowPage(pAd,j);

      for(i=0;i<(ULONG)((pAd->ram_usable *1024));i++){
#ifdef CODE_386
         *(((unsigned char *)(pAd->ram_access)) +i)  = '\0';
#else
         *(((unsigned char far *)(pAd->ram_access)) +i)  = '\0';
#endif
      }
   }
   SyncSetWindowPage(pAd,0);

   pAd->Rcv_Buffer_Pending = 0;
   pAd->Xmit_DMA_Pending = 0;
   pAd->Rcv_DMA_Pending = 0;
   pAd->adapter_status = INITIALIZED;

#ifdef MULTI_TX_BUFS
   for(i=0; i < NUM_TX_BUFFERS; i++) {
        pAd->sebm_txbufaddr[i] = TX_BUF_LEN * i;
        pAd->sebm_txbufstate[i] = TX_FREE;
   }
   pAd->sebm_txbuffree = 0;
   pAd->sebm_txbufload = -1;
   pAd->sebm_txbuf_busy = -1;
#endif

   return(SUCCESS);

}

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
/*** MSCHK ***/
LMB_Ram_Test(pAd)
	Ptr_Adapter_Struc pAd;
/*++
	DMA test with loopback
	return 0 for passing the test
	       1 for failing 
--*/
{
   register int i;
   register UCHAR RegValue;
   short ctl_reg, cmd_reg;
   int mschkflg=0;
	
   /* Non loopback + 8 bytes FIFO + 16bit Xfer */

   unsigned char init_dcr = DCR_LS|DCR_FT1|DCR_WTS;
   int j;

   ctl_reg = pAd->io_base + EISA_790;
   cmd_reg = ctl_reg + REG_CMD;

   RegValue = inb(ctl_reg) & 0x3F;
   
   outb(ctl_reg, RegValue | MSR_RST);

   for (i=0;i<0x10;i++)
      RegValue = inb(ctl_reg) & 0x3F;
   outb(ctl_reg, RegValue);

   /*
   // Enable Ram
   */

   RegValue = inb(pAd->io_base+EISA_INT);

   outb(pAd->io_base+EISA_INT,RegValue | EISA_MENB);

   inb(pAd->io_base+EISA_INT);		/* Sat */

   outb(pAd->io_base+EISA_INT,RegValue | EISA_MENB); /* Sat some pentium */

   outb(cmd_reg,CMD_PAGE0 + CMD_RD2 + CMD_STP); /* 0+0x20+0x01 */
   for (i=0;i<0x10000;i++)
     if(inb(ctl_reg+REG_ISR) & ISR_RST)
	break;
   if(i==0x10000) { /* fail */
      return(1);
	}

   outb(ctl_reg + REG_DCR, init_dcr);
   outb(ctl_reg + REG_RBCR0, 0);
   outb(ctl_reg + REG_RBCR1, 0);

	/* check EISA ID */
	i=inl(0xc80);
	for(j=0;j<nbadmid;j++) 
		if(i==badmid[j]) return(1);
	
   if(inb(ctl_reg + REG_BPR)&BPR_BMSTR){
		return(1);
   } else {
      /* allocate temp memory */
      mschktsz &= 0xfffffffc;	/* 4 byte boundary */
      if(mschktsz > 0x800) mschktsz=0x800;	/* for debug */
      if((mschkbuf = (int *) sptalloc(1, PG_P, 0, 0))== NULL) {
		return(1);
	  } else {
      	outb(ctl_reg + REG_IMR, 0);	 /* mask out all intr */

	  	/* turn it on for the test */
      	RegValue = inb(ctl_reg+REG_HWR) | HWR_SWH;	 /* Select Conf Regs */
      	outb(ctl_reg + REG_HWR,RegValue); /* Sat */
      	outb(ctl_reg + REG_RAR, 0);		/* Sat */
	  	outb(ctl_reg + REG_HWR, RegValue & ~HWR_SWH);

	  	RegValue = inb(ctl_reg + REG_BPR);
 	  	outb(ctl_reg + REG_BPR, RegValue | BPR_M16EN);
	  	outb(ctl_reg + REG_MSR, MSR_MENB);	/* Enable share ram */

	  	/* set at page 0 */
	  	outb(pAd->io_base+EISA_RAM, inb(pAd->io_base+EISA_RAM) & 0xfc);

      	RegValue = inb(pAd->io_base + EISA_STAT);
      	outb(pAd->io_base + EISA_STAT, RegValue & ~(EISA_DIEN | EISA_EIL)); 
      	RegValue = inb(pAd->io_base + EISA_INT);
      	outb(pAd->io_base + EISA_INT, (RegValue | EISA_CLRD)); 

		/* accept broadcast packet */
		outb(ctl_reg + REG_RCR, (UCHAR) RCR_AB);

		/* set loopback mode */
		outb(ctl_reg + REG_TCR, TCR_LB1);

		outb(ctl_reg + REG_PSTART,(UCHAR) 0x8);
		outb(ctl_reg + REG_BNRY, (UCHAR) 0x8);
		outb(ctl_reg + REG_PSTOP, (UCHAR) 0x10);

		outb(cmd_reg, CMD_PAGE1+CMD_STP);
		outb(ctl_reg + REG_CURR, (UCHAR) 0x8);

		/* set up broadcast packet for loop back test */
		((int *) pAd->ram_access)[0] = 0xffffffff;
		((int *) pAd->ram_access)[1] = 0xffff;
		for(i=2; i<0x100; i++) 
		((int *) pAd->ram_access)[i] = 0x47474747;

    	for(i=0; i < mschktsz/4; i++) mschkbuf[i]=mschkpttn[0];

      	for(j=0; j<mschkcnt; j++) {
    	 /* DMA test */
		 mschkflg = dmatest(pAd);
	 	 if(mschkflg) break;
      	}

	  sdmafail=0;
	  while(inb(cmd_reg) & CMD_TXP) {
		sdmafail++;
		if(sdmafail>0xffff) {
			printf("SEBM(%x): SMC Ethernet Adapter Error !!!\n", pAd->io_base);
			/* reset NIC */
   			RegValue = inb(ctl_reg) & 0x3F;
   			outb(ctl_reg, RegValue | MSR_RST);
		}
	  }

	  outb(cmd_reg, CMD_STP);
				
      /* free temp memory */
      sptfree((char *) mschkbuf, 1, 1);

	  }
   }
   return(mschkflg);
}

/*static*/ int
dmatest(pAd)
	Ptr_Adapter_Struc pAd;
/*++

Routine Description:
	
	DMA test

--*/
{
	int i;
	int tfail = 0;
   	paddr_t phys_addr;
    UCHAR RegValue;

    intr_disable();
	/* clear the first 4 byte of received area */
	((int *) pAd->ram_access)[0x400] =  0;
    phys_addr = (paddr_t)svirtophys(((short *) mschkbuf) + 1);
	doloopback(pAd);
    outl(pAd->io_base+EISA_AA,(ULONG)(phys_addr));
	i = 0x10028000 | (mschktsz & 0x3fff - 2); 
    outl(pAd->io_base+EISA_AA+4,i);
    /* wait for DMA to complete */
    intr_restore();
	sdmafail=0;
	while(inp(pAd->io_base+EISA_STAT) & EISA_SDMA) {
		sdmafail++;
		inb(pAd->io_base+EISA_ID1);
		if(sdmafail>0xffff) { 
			tfail=1;		/* fail */
			break;
		}
	}
	intr_disable(); 
    RegValue = inb(pAd->io_base + EISA_INT);
    outb(pAd->io_base + EISA_INT, (RegValue | EISA_CLRD)); 
	intr_restore();
	if(tfail) return(1);

	if(((short *) pAd->ram_access)[0x800] != 0) 
		return(1);		/* fail */
	
	/* compare */
	if(((short *) mschkbuf)[1] != ((short *)pAd->ram_access)[0x801]) 
		return(1);

    for(i=1; i<mschktsz/4; i++) {
		if(*(mschkbuf+i) != ((int *)pAd->ram_access)[0x400+i]) {
			tfail=1;
			break; 
		}
	}
	if(tfail) return(1);	

	/* send other direction */
	/* clear the four bytes before */
	mschkbuf[0x200] = 0;
    phys_addr = (paddr_t)svirtophys(((short *) mschkbuf)+0x401);
	doloopback(pAd);
    outl(pAd->io_base+EISA_AA,(ULONG)(phys_addr));
	i = 0x1002C000 | (mschktsz & 0x3fff - 2); 
    outl(pAd->io_base+EISA_AA+4,i);
    /* wait for DMA to complete */
    intr_restore();
	sdmafail=0;
	while(inp(pAd->io_base+EISA_STAT) & EISA_SDMA) {
		sdmafail++;
		inb(pAd->io_base+EISA_ID1);
		if(sdmafail>0xffff) {
			tfail=1;
			break;
		}
	}
	intr_disable(); 
    RegValue = inb(pAd->io_base + EISA_INT);
    outb(pAd->io_base + EISA_INT, (RegValue | EISA_CLRD)); 
	intr_restore();
	if(tfail) return(1);

	/* compare */
	if(((short *) mschkbuf)[0x400] != 0) 
		return(1);

	if(((short *) mschkbuf)[0x401] != ((short *) mschkbuf)[1]) 
		return(1);

    for(i=1; i<mschktsz/4; i++) {
		if(*(mschkbuf+i) != *(mschkbuf+0x200+i)) {
	 		tfail=1;
			break; 
		}
	}
	if(tfail) return(1);

	return(0);
}
 
static int 
doloopback(pAd)
	Ptr_Adapter_Struc pAd;
/*++

Routine Description:

	Loop back operation for DMA test 

--*/
{
	int i;
	unsigned char intrstat, boundary;
	unsigned short ctl_reg = pAd->io_base + EISA_790;
	unsigned short cmd_reg = ctl_reg + REG_CMD;

	outb(cmd_reg, inb(cmd_reg) & PG_MSK);
	intrstat = inb(ctl_reg + REG_ISR);
	if(intrstat & (ISR_RXE | ISR_PRX | ISR_OVW | ISR_CNT)) {
		outb(ctl_reg + REG_ISR,  (ISR_RXE | ISR_PRX | ISR_OVW | ISR_CNT));
		boundary=inb(ctl_reg + REG_BNRY);
		boundary++;
		if(boundary >= 0x10) boundary=0x8;
		if(boundary==0x8)
			outb(ctl_reg + REG_BNRY, 0xf);
		else
			outb(ctl_reg + REG_BNRY, boundary - 1);
	} 	

	if(inb(ctl_reg+REG_CMD) & CMD_TXP)	/* if xmitting; exit */
		return(1);

	outb(cmd_reg, (inb(cmd_reg) & PG_MSK) | CMD_PAGE0);
	if(intrstat & (ISR_PTX | ISR_TXE | ISR_OVW)) {
		outb(ctl_reg+REG_ISR, (ISR_PTX | ISR_TXE | ISR_OVW));
	}

	outb(ctl_reg + REG_TPSR, 0);
	outb(ctl_reg + REG_TBCR0, 0);
	outb(ctl_reg + REG_TBCR1, 4);

	/* start transmit */
	outb(ctl_reg + REG_CMD, CMD_STA+CMD_TXP);

	return(0);	
}

/*** [1.06(ii)] ***/
LMB_AutoMediaDetect(pAd)
Ptr_Adapter_Struc pAd;
/*++

Routine Description:

	This routine is to sense which type of media connector is active and 
	disable or enable the BNC DC/DC power supply accordingly.

	The media_set field of Adapter Structure is updated to reflect media 
	detection. The EEPROM is not updated. This is a temporary runtime 
	feature only.

Arguments:

    pAd - A pointer to an LMB adapter structure.

Return:

	pAd->media_set is modified.

--*/
{
	unsigned char RegVal, RegVal1;
		
	pAd->media_set=0;
	
	RegVal=inp(pAd->io_base+EISA_ROM);
	if(RegVal & 0x8) { /* STARLAN */
		RegVal=inp(pAd->io_base+EISA_790+REG_HWR);
		outp(pAd->io_base+EISA_790+REG_HWR, RegVal | HWR_SWH);
		RegVal1=inp(pAd->io_base+EISA_790+REG_GCR);
		outp(pAd->io_base+EISA_790+REG_GCR, RegVal1 & ~(GCR_LITE+GCR_GPOUT));
		outp(pAd->io_base+EISA_790+REG_HWR, RegVal);
	} else {	/* AutoMediaDetect */
		GetMedia(pAd);
	}

	return(0);
}

static
GetMedia(pAd)
Ptr_Adapter_Struc pAd;
/*++

Routine Description:

	This routine is to test the Link Integrity status on a UTP connection.
	
	If the UTP connection is active, the BNC will be turned off;
	Otherwisw, the BNC will be turn on.

	If an AUI is also present and the link status is not active, the other
	media types will be tested in the "Chk_Xmit" function. 

Arguments:

    pAd - A pointer to an LME adapter structure.

Return:

	If (UTP active) media_set = MEDOPT_UTP | MEDOPT_BNC
	else { check by Xmit_test } 

++*/
{
	short ctl_reg = pAd->io_base + EISA_790;
	short cmd_reg = ctl_reg + REG_CMD;
	unsigned char RegVal;
	unsigned char SavTcr;
	unsigned char temp;

	/* Disable loopback */
	outb(cmd_reg, CMD_PAGE2);
	SavTcr=inb(ctl_reg+REG_TCR);
	outb(cmd_reg, CMD_PAGE0);
	outb(ctl_reg+REG_TCR, 0);

	/* turn BNC off */
	RegVal = inb(ctl_reg+REG_HWR);
	outb(ctl_reg+REG_HWR, (RegVal & 0xc3) | HWR_SWH);
	temp=inb(ctl_reg+REG_GCR);
	outb(ctl_reg+REG_GCR, (temp & ~GCR_GPOUT) | GCR_LITE);
	outb(ctl_reg+REG_HWR, RegVal);

	outb(cmd_reg, CMD_PAGE3);
	temp=inb(ctl_reg+REG_MANCH);
	outb(cmd_reg, CMD_PAGE0);
	
	if(temp & MANCH_LLED) {	/* UTP, turn BNC on in case of switching to BNC */ 
		pAd->media_set |= MED_OPT_UTP;
		/* turn BNC on */
		outb(ctl_reg+REG_HWR, (RegVal & 0xc3) | HWR_SWH);
		temp=inb(ctl_reg+REG_GCR);
		outb(ctl_reg+REG_GCR, temp | (GCR_GPOUT | GCR_LITE));
		outb(ctl_reg+REG_HWR, RegVal);
		pAd->media_set |= MED_OPT_BNC;
	} else {
		XmitTest(pAd);
	}		

	/* retore TCR */
	outb(cmd_reg, CMD_PAGE0);
	outb(ctl_reg+REG_TCR, SavTcr);

}

static
XmitTest(pAd)
Ptr_Adapter_Struc pAd;
/*++

Routine Description:

	This routine is to transmit data using the AUI port and determine
		if the BNC, AUI or (UTP for Starlan 10) is active 
	
Arguments:

    pAd - A pointer to an LME adapter structure.

Return:

	BNC is turned on if BNC link is active.
	BNC is turned off if the BNC is not active.

++*/
{
	static unsigned char dummy_frame[]="\0\0\300\0\0\0\0\0\300\0\0\0SMC Auto Media Detect Test Frame. Thank you for using SMC products. -WSKRTDDZPMBGSFMTJGVP-";

	int dummy_len=sizeof(dummy_frame);
	short ctl_reg = pAd->io_base + EISA_790;
	short cmd_reg = ctl_reg + REG_CMD;
	unsigned char RegVal,RegVal1;
	unsigned char temp;
	caddr_t txbuf;
	int i,j;

	/* Init EBM */
	Init_EBM_NIC(pAd);

	/* Disable loopback */
	outb(cmd_reg, CMD_PAGE2);
	inb(ctl_reg+REG_TCR);
	outb(cmd_reg, CMD_PAGE0);
	outb(ctl_reg+REG_TCR, 0);

	/* turn BNC off */
	RegVal = inb(ctl_reg+REG_HWR);
	outb(ctl_reg+REG_HWR, (RegVal & 0xc3) | HWR_SWH);
	temp=inb(ctl_reg+REG_GCR);
	outb(ctl_reg+REG_GCR, (temp & ~GCR_GPOUT) | GCR_LITE);
	outb(ctl_reg+REG_HWR, RegVal);

	/* setup xmit buffer */
	txbuf=(caddr_t)(pAd->ram_access);
   	sebmbcopy(dummy_frame, txbuf,  dummy_len-1, DEST_ALIGN);
 
	/* clear ISR */
	outb(ctl_reg+REG_ISR, 0xff);

	/* send dummy frame */
	/* Dummy frame length must be = 0x400 for accurate results */
   i = inb(cmd_reg);
   outb( cmd_reg, i & PG_MSK & ~CMD_TXP);

   outb( ctl_reg + REG_TPSR, 0);	/* xmit page start */
   outb( ctl_reg + REG_TBCR0, (unsigned char) 0);
   outb( ctl_reg + REG_TBCR1, (unsigned char) 4);

   i = inb(cmd_reg);
   outb( cmd_reg, i | CMD_STA | CMD_TXP);

	for(i=0; i<0x320000; i++) {		/* about 300ms */
		temp=inb(ctl_reg+REG_ISR);
		if(temp & (ISR_TXE|ISR_PTX)) break;
	}

	RegVal1=inb(ctl_reg+REG_TSR);

	/* clear ISR */
	outb(ctl_reg+REG_ISR, 0xff);

	/* stop the NIC */
	outb(cmd_reg, CMD_STP|CMD_RD2);

	for(i=0;i<0x640;i++) {
		for(j=0;j<12;j++) {
			temp=inb(ctl_reg+REG_ISR);
		}
		if(temp & ISR_RST) break;
	}
		
	if(!(RegVal1 & TSR_CRS)) 
		pAd->media_set |= MED_OPT_AUI;
	else { 	/* set BNC on */
		pAd->media_set |= MED_OPT_BNC;
		/* turn BNC on */
		outb(ctl_reg+REG_HWR, (RegVal & 0xc3) | HWR_SWH);
		temp=inb(ctl_reg+REG_GCR);
		outb(ctl_reg+REG_GCR, temp | (GCR_GPOUT | GCR_LITE));
		outb(ctl_reg+REG_HWR, RegVal);
	}
	return(0);
}

static
Init_EBM_NIC(pAd)
Ptr_Adapter_Struc pAd;
/*++

Routine Description:

    This routine is used to init EBM NIC for XmitTest. 

Arguments:

    Adapt - A pointer to an LME adapter structure.

Return:

    SUCCESS/FAIL

--*/
{
   register int i,j;
   register UCHAR RegValue, RegValue1, SavValue;
   short ctl_reg, cmd_reg;
   USHORT XmitSize;
   unsigned char init_dcr = DCR_LS|DCR_FT1|DCR_WTS;

   ctl_reg = pAd->io_base + EISA_790;
   cmd_reg = ctl_reg + REG_CMD;

   RegValue = inb(ctl_reg) & 0x3F;
   
   outb(ctl_reg, RegValue | MSR_RST);
   for (i=0;i<0x10;i++)
      RegValue = inb(ctl_reg) & 0x3F;
   outb(ctl_reg, RegValue);

   /*
   // Enable Ram
   */

   RegValue = inb(pAd->io_base+EISA_INT);
   outb(pAd->io_base+EISA_INT,RegValue | EISA_MENB);
   inb(pAd->io_base+EISA_INT);		/* Sat */
   outb(pAd->io_base+EISA_INT,RegValue | EISA_MENB); /* Sat some pentium */

   outb(cmd_reg,CMD_PAGE0 + CMD_RD2 + CMD_STP); /* 0+0x20+0x01 */
   for (i=0;i<0x10000;i++)
     if(inb(ctl_reg+REG_ISR) & ISR_RST)
	break;
   if(i==0x10000)
     return(INITIALIZE_FAILED);

   outb(ctl_reg + REG_DCR, init_dcr);
   outb(ctl_reg + REG_RBCR0, 0);
   outb(ctl_reg + REG_RBCR1, 0);

   outb(ctl_reg + REG_TCR, TCR_LB1);  /* LoopBack mode2 */

	if( (pAd->num_of_tx_buffs > 2) || (pAd->num_of_tx_buffs == 0))
	 	pAd->num_of_tx_buffs = 2;
   XmitSize =(USHORT)(pAd->num_of_tx_buffs*TX_BUF_LEN) >> 8;

   outb(ctl_reg + REG_PSTART,(UCHAR)XmitSize);
   outb(ctl_reg + REG_BNRY, (UCHAR)XmitSize);

   outb(ctl_reg + REG_PSTOP, pAd->ram_size << 2);

   outb(ctl_reg + REG_ISR, 0xFF);
   outb(ctl_reg + REG_IMR, pAd->InterruptMask);

   outb(ctl_reg + REG_CURR, XmitSize + 1);
   SavValue = inp(ctl_reg+REG_HWR) | HWR_SWH;	 /* Select Conf Regs */
   outp(ctl_reg + REG_HWR,SavValue); /* Sat */
   outb(ctl_reg + REG_RAR, 0);		/* Sat */

	/*
	 *	 Select IRQ 7 and 0WS on 790 
     */

   RegValue = inb(ctl_reg +REG_GCR);
   outb(ctl_reg + REG_GCR, RegValue|GCR_IR2|GCR_IR1|GCR_IR0|GCR_0WS);

   RegValue = inb(ctl_reg + REG_BPR);
   outb(ctl_reg + REG_BPR, RegValue | BPR_M16EN);
   outb(cmd_reg, CMD_STP+CMD_RD2);	   /* Stop NIC Sel Page 0 */

   outb(ctl_reg + REG_MSR, MSR_MENB);	/* Enable share ram */
   inb(ctl_reg +REG_MSR);		/* Sat */
   outb(ctl_reg + REG_MSR, MSR_MENB);	/* Sat Enable share ram */

   outp(ctl_reg + REG_HWR,SavValue & ~HWR_SWH); /* Sat */ 

   /*** MSCHK ***/
   for(j=0; j<3; j++) {
      SyncSetWindowPage(pAd,j);

      for(i=0;i<(ULONG)((pAd->ram_usable *1024));i++){
#ifdef CODE_386
         *(((unsigned char *)(pAd->ram_access)) +i)  = '\0';
#else
         *(((unsigned char far *)(pAd->ram_access)) +i)  = '\0';
#endif
      }
   }
   SyncSetWindowPage(pAd,0);

   return(SUCCESS);
}
#endif /* -REALMODE && -Solaris */


LM_STATUS
LMB_Open_Adapter(pAd)
    Ptr_Adapter_Struc pAd;
/*++

Routine Description:

    This routine will open the adapter, initializing it if necessary.

Arguments:

    pAd - A pointer to an LME adapter structure.


Return:

    SUCCESS				  
    OPEN_FAILED
    ADAPTER_HARDWARE_FAILED

--*/
{
    LM_STATUS Status;
	 USHORT Ctl_Reg;

    if (pAd->adapter_status == OPEN) {

        return(SUCCESS);

    }

    if (pAd->adapter_status == CLOSED) {

        Status = LMB_Initialize_Adapter(pAd);

        if (Status != SUCCESS) {

            return(Status);

        }

    }


    if (pAd->adapter_status != INITIALIZED) {

        return(OPEN_FAILED);

    }

    /*
     * Start the NIC
     */

    Ctl_Reg = pAd->io_base+EISA_790;

    outb(Ctl_Reg+REG_CMD,CMD_PAGE0 + CMD_RD2 + CMD_STA); /* 0+0x20+0x02 */

    if (pAd->mode_bits & MANUAL_CRC) {

        outb(Ctl_Reg+REG_TCR,TCR_CRC); /* 0+0x20+0x02 */

    } else {

        outb(Ctl_Reg+REG_TCR,0);

    }

    pAd->adapter_status = OPEN;

    /*
     * Enable Board Interrupt
     */

    LMB_Enable_Adapter(pAd);

    return(SUCCESS);

}

LM_STATUS
LMB_Close_Adapter(pAd)
    Ptr_Adapter_Struc pAd;
/*++

Routine Description:

    This routine will close the adapter, stopping the card.

Arguments:

    Adapt - A pointer to an LME adapter structure.


Return:

    SUCCESS
    CLOSE_FAILED
    ADAPTER_HARDWARE_FAILED

--*/
{

    if (pAd->adapter_status != OPEN) {

        return(CLOSE_FAILED);

    }

    LMB_Disable_Adapter(pAd);

    /*
	  * Stop the NIC
	  */

    outb(pAd->io_base+REG_CMD,CMD_RD2+CMD_STP); /* 0x20 + 01 */

    if (pAd->board_id & BOARD_16BIT) {
	   /* disable LAN16ENB and MEM16ENB */
	   outb(pAd->io_base + REG_LAAR, (char)(((long) pAd->ram_base >> 19) & 0x01));
    }

    /* disable memory */

    outb(pAd->io_base, (char)(((long) pAd->ram_base >> 13) & 0x3F));

    pAd->adapter_status = CLOSED;

    return(SUCCESS);

}

LM_STATUS
LMB_Change_Receive_Mask(pAd)
Ptr_Adapter_Struc pAd;

/*++

Routine Description:

    This routine will set the adapter to the receive mask in the filter
    package.

Arguments:

    Adapt - A pointer to an LME adapter structure.


Return:

    SUCCESS
    INVALID_FUNCTION

--*/
{
#ifdef CODE_386
	 PUCHAR RamAccess;
#else
	 unsigned char far * RamAccess;
#endif
    UCHAR RegValue = 0;
	 register int ctr_reg;
	 ctr_reg = pAd->io_base + EISA_790;

	 if(pAd->pc_bus == PCMCIA_BUS){
#ifdef CODE_386
      RamAccess = (PUCHAR)(pAd->ram_access + (long)(REG_OFFSET+REG_CMD));
#else
      RamAccess = (unsigned char far *)(pAd->ram_access + (long)(REG_OFFSET+REG_CMD));
#endif
	  *RamAccess = CMD_PAGE0;
	 }
    else{
		outb(ctr_reg+REG_CMD,CMD_PAGE0);			/* Select Page0 */
	 }
	 if(pAd->receive_mask & TRANSMIT_ONLY)
	   RegValue |= RCR_MON;
	 if(pAd->receive_mask & ACCEPT_MULTICAST)
	   RegValue |= RCR_AM;
	 if(pAd->receive_mask & ACCEPT_BROADCAST)
	   RegValue |= RCR_AB;
	 if(pAd->receive_mask & PROMISCUOUS_MODE)
		RegValue |= RCR_PRO+RCR_AM+RCR_AB;
	 if(pAd->receive_mask & ACCEPT_ERR_PACKETS)
		RegValue |= RCR_AR+RCR_SEP;
	 if(pAd->receive_mask & ACCEPT_MULTI_PROM)
		RegValue |= RCR_AM+RCR_AB;
	 if(RegValue & RCR_AM)
           SyncSetMAR(ctr_reg,0xff);	  /* set all */
	 else
           SyncSetMAR(ctr_reg,0);	  /* clear all */
	 if(pAd->board_id & NIC_790_BIT){
/*		RegValue |= RCR_SEP;      /* sav err for EarlyRx */
		;
	 }
	 if(pAd->pc_bus == PCMCIA_BUS){
#ifdef CODE_386
          RamAccess = (PUCHAR)(pAd->ram_access + (long)(REG_OFFSET+REG_RCR));
#else
          RamAccess = (unsigned char far *)(pAd->ram_access + (long)(REG_OFFSET+REG_RCR));
#endif
	  *RamAccess = RegValue;
         }
	 else{
		outb(ctr_reg+REG_RCR,RegValue);
	 }
	 pAd->adapter_flags |= ERX_DISABLED;

	 RegValue = IMR_PRXE|IMR_PTXE|IMR_RXEE|
                     IMR_TXEE|IMR_OVWE|IMR_CNTE;

    if( (pAd->board_id & NIC_790_BIT) &&
        (pAd->receive_mask & EARLY_RX_ENABLE) ){
	
         RegValue |= IMR_ERWE;
	 pAd->adapter_flags &= ~ERX_DISABLED;
    }

	 pAd->InterruptMask = RegValue;

	 if(pAd->pc_bus == PCMCIA_BUS){
#ifdef CODE_386
      RamAccess = (PUCHAR)(pAd->ram_access + (long)(REG_OFFSET+REG_IMR));
#else
      RamAccess = (unsigned char far *)(pAd->ram_access + (long)(REG_OFFSET+REG_IMR));
#endif
	  *RamAccess = RegValue;
    }
	 else{
		outb(ctr_reg+REG_IMR,RegValue);
	 }

	 return(SUCCESS);
}

LM_STATUS
#ifdef LM_STREAMS
LMB_Send(mb,pAd)
mblk_t	*mb;	/* ptr to message block containing packet */
Ptr_Adapter_Struc pAd;
#else
LMB_Send(mb,pAd,totalbytecnt)
#ifdef CODE_386
Data_Buff_Structure *mb;
#else
Data_Buff_Structure far *mb;
#endif
Ptr_Adapter_Struc pAd;
int totalbytecnt;
#endif

/*++

Routine Description:

    This routine will copy a packet to the card and start a transmit if
    possible
    This should be called under interrupt disable.

Arguments:

    Packet - The packet to put on the wire.

    Adapt - A pointer to an LMI adapter structure.


Return:

    SUCCESS
    OUT_OF_RESOURCES

--*/
{

   register unsigned int length; /* total length of packet */
   register unsigned int frag_len;
   caddr_t txbuf;            /* ptr to transmission buffer area on 8003 */
#ifdef LM_STREAMS
   register mblk_t	*mp; /* ptr to M_DATA block containing the packet */
#endif
   register int i;
#ifdef BADLINT
   int nxttxbuf;
#else
   UCHAR nxttxbuf;
#endif
   int txbufoff;
   short cmd_reg, ctl_reg;
   UCHAR SaveValue;
   UCHAR RegValue;

#ifdef M_XENIX
   int splvar;
#endif 

#ifdef MULTI_TX_BUFS
	if( pAd->sebm_txbufstate[pAd->sebm_txbuffree] != TX_FREE)
                return(OUT_OF_RESOURCES);
	nxttxbuf=pAd->sebm_txbuffree;
	pAd->sebm_txbufstate[nxttxbuf] = TX_PENDING;
	pAd->sebm_txbuffree = (nxttxbuf + 1) % pAd->num_of_tx_buffs;
#endif

   ctl_reg = pAd->io_base + EISA_790;
   cmd_reg = ctl_reg + REG_CMD;


   length = 0;

   SaveValue = inb(pAd->io_base+EISA_RAM);
   SyncSetWindowPage(pAd,0);

   /* load the packet header onto the board */

#if defined(SEBMDEBUG)
   if (sebm_debug&SEBMBOARD)
     printf("sebmsend: sebmbcopy(%x, %x, %x) base=%x\n",
	    (int) mb->b_rptr, (int) txbuf, (int)(mb->b_wptr - mb->b_rptr),
	    (int) pAd->ram_access);
#endif

#ifndef MULTI_TX_BUFS
   nxttxbuf = pAd->curtxbuf;
#endif
   txbufoff = nxttxbuf*TX_BUF_LEN;
   txbuf =(caddr_t)(pAd->ram_access) + txbufoff  ;

#ifdef SEBMDEBUG
   if (sebm_debug&10)
   printf("sebmsend: txbuf(%x)\n",txbuf);
#endif

#ifdef LM_STREAMS
   sebmbcopy((caddr_t) mb->b_rptr, txbuf, i=(int)(mb->b_wptr - mb->b_rptr),
	DEST_ALIGN);


   length += (unsigned int)i;
   mp = mb->b_cont;

   /*
    * load the rest of the packet onto the board by chaining through
    * the M_DATA blocks attached to the M_PROTO header. The list
    * of data messages ends when the pointer to the current message
    * block is NULL
    */

   while (mp != NULL) {

	frag_len = (int)(mp->b_wptr - mp->b_rptr);

#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMBOARD)
	printf("sebmsend: (2) sebmbcopy(%x, %x, %x) base=%x\n",
	       (int) mp->b_rptr, (int)(txbuf+length),
	       frag_len, (int) pAd->ram_access);
#endif
      if((pAd->adapter_bus == BUS_EISA32M_TYPE) && (frag_len > DMA_CUT_OFF)) {

        if(!pAd->Xmit_DMA_Pending && !pAd->Rcv_DMA_Pending){
#ifndef SMCDMA
	   RegValue=inp(pAd->io_base+EISA_STAT);
	   outp(pAd->io_base+EISA_STAT, RegValue & ~EISA_DIEN); /* tim */
           EBM_Xmit(pAd,(caddr_t)mp->b_rptr,txbufoff+length,frag_len,0);
	   outp(pAd->io_base+EISA_STAT, RegValue);
#else
           EBM_Xfer(pAd,(caddr_t)mp->b_rptr,txbufoff+length,frag_len,0);
#endif

        }
      }

      else {		/* Doing slave mode */

           sebmbcopy( (caddr_t) mp->b_rptr, txbuf + length,
	    frag_len, DEST_ALIGN);
      }

/****
      if(pAd->Xmit_DMA_Pending) {

        RegValue = inb(pAd->io_base + EISA_STAT);

/****
        if((RegValue & (EISA_DIEN | EISA_STATD)) == (EISA_DIEN | EISA_STATD))
*****/
/****
        if(RegValue & EISA_STATD)

            pAd->Xmit_DMA_Pending = 0;
      } 
***/


      length += frag_len;
      mp = mp->b_cont;

   } 		/* end while */

#else /* -LM_STREAMS:  Follow LMAC spec */

   /* 
    * uses the upper mac layer Data_Buff_Structure
    */ 
   for (i=length=0;i<mb->fragment_count;i++) {

      frag_len=mb->fragment_list[i].fragment_length;	
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMBOARD)
	printf("sebmsend: (2) sebmbcopy(%x, %x, %x) base=%x\n",
	       (int) mb->fragment_list[i].fragment_ptr, (int)(txbuf+length),
	       frag_len, (int) pAd->ram_access);
#endif

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
      if((pAd->adapter_bus == BUS_EISA32M_TYPE) && (frag_len > DMA_CUT_OFF)) {
        if(!pAd->Xmit_DMA_Pending && !pAd->Rcv_DMA_Pending){
#ifndef SMCDMA
	   RegValue=inp(pAd->io_base+EISA_STAT);
	   outp(pAd->io_base+EISA_STAT, RegValue & ~EISA_DIEN); /* tim */
           EBM_Xmit(pAd,(caddr_t)mb->fragment_list[i].fragment_ptr,
			txbufoff+length,frag_len,0);
	   outp(pAd->io_base+EISA_STAT, RegValue);
#else
           EBM_Xfer(pAd,(caddr_t)mb->fragment_list[i].fragment_ptr,
			txbufoff+length,frag_len,0);
#endif
        }
      } else 		/* Doing slave mode */
#endif /* -REALMODE && -Solaris */
           sebmbcopy( (caddr_t) mb->fragment_list[i].fragment_ptr, txbuf + length,
	    		frag_len, DEST_ALIGN);
      length += frag_len;
   } 		/* end for */

#endif /* -LM_STREAMS */

   outb(pAd->io_base+EISA_RAM,SaveValue);

#if defined(SEBMDEBUG)
   if (sebm_debug&SEBMSEND){
      printf("sebmsend: machdr=<");
      for (i=0; i<(sizeof(struct sebm_machdr)+4); i++)
	printf("%s%x", (i==0)?"":":", *(txbuf + i));
      printf(">\n");
   }
#endif

   /* check length field for proper value; pad if needed */
   if (length < SEBMMINSEND)
     length = SEBMMINSEND;

#if defined(SEBMDEBUG)
   if (sebm_debug&SEBMBOARD)
     printf("sebmsend: length=%d\n", length);
#endif

#ifdef	DLPI
   ebmacstats[pAd->adapter_num].mac_ooctets += length;
   ebmacstats[pAd->adapter_num].mac_frame_xmit++;
#endif

   /* packet loaded; now tell 790 to start the transmission */


   pAd->sebm_txbufstate[nxttxbuf] = TX_LOADED;
   pAd->sebm_txbuflen[nxttxbuf] = length;

#ifndef MULTI_TX_BUFS
   if(pAd->sebm_txbufstate[nxttxbuf^1] == TX_MTING) {
	return(SUCCESS);
   }
   pAd->sebm_txbufstate[nxttxbuf] = TX_MTING;
#else
   if(pAd->sebm_txbufload < 0) {
                pAd->sebm_txbufload=nxttxbuf;
   }

   if(pAd->sebm_txbuf_busy >= 0) {
                return(SUCCESS);
   }

   /* send the loaded packet according to the order */
   pAd->sebm_txbuf_busy=nxttxbuf=pAd->sebm_txbufload;
   length=pAd->sebm_txbuflen[nxttxbuf];
   txbufoff=pAd->sebm_txbufaddr[nxttxbuf];
#endif

   /* Tell the 83C790 to start the transmission. */
   i = inb(cmd_reg);
#ifdef BADLINT
   outb( cmd_reg, i & PG_MSK & ~CMD_TXP);
#else
   outb( cmd_reg, (unchar)(i & PG_MSK & ~CMD_TXP));
#endif
 
#ifdef BADLINT
   outb( ctl_reg + REG_TPSR,txbufoff>>8);	/* xmit page start */
#else
   outb( ctl_reg + REG_TPSR,(unchar)(txbufoff>>8));	/* xmit page start */
#endif
   outb( ctl_reg + REG_TBCR0, (unsigned char) length);
   outb( ctl_reg + REG_TBCR1, (unsigned char)(length >> 8));

   i = inb(cmd_reg);
#ifdef BADLINT
   outb( cmd_reg, i | CMD_TXP);
#else
   outb( cmd_reg, (unchar)(i | CMD_TXP));
#endif

#ifdef MULTI_TX_BUFS
   pAd->sebm_txbufstate[nxttxbuf] = TX_MTING;
   nxttxbuf = (nxttxbuf+1) % pAd->num_of_tx_buffs;
   if(pAd->sebm_txbufstate[nxttxbuf] == TX_LOADED) {
                pAd->sebm_txbufload=nxttxbuf;
   } else {
                pAd->sebm_txbufload=-1;
   }
#endif

    /*
    // return success
    */

    return(SUCCESS);

}


LM_STATUS
LMB_Service_Events(pAd)
Ptr_Adapter_Struc pAd;

/*++

Routine Description:

    This routine will handle all interrupts from the adapter.

Arguments:

    Adapt - A pointer to an LME adapter structure.


Return:

    SUCCESS

--*/
{
   register unsigned char int_reg;
   register UCHAR inval;
   register int i;
   unsigned char ts_reg, orig;
#ifdef LM_STREAMS
   mblk_t	*bp;
#endif
   int call_sebmsched = 0;	/* set if scheduler should be called */
   short cmd_reg, ctl_reg;
   UCHAR SaveValue = 0;
   unsigned short Page_Offset;
   unsigned short Ram_Offset;
   unsigned short Move_Len;
   unsigned short length,hwlength;
   rcv_buf_t	*rp;
#ifdef CODE_386
   rcv_buf_t	*ram_rp;
   rcv_buf_t rbuf;
#else
   rcv_buf_t far *ram_rp;
   static rcv_buf_t rbuf;	/* put in DS:, not SS: */
#endif
   int nxttbuf;
#ifndef LM_STREAMS
   int len;
   unsigned short txbufoff;
#endif

   ctl_reg = pAd->io_base+EISA_790;
   cmd_reg = ctl_reg + REG_CMD;

   /* mask out all board interrupts */
   outb(ctl_reg + REG_IMR,0);

   SaveValue = inb(pAd->io_base + EISA_STAT);
   i = 0;
/*
   printf("EISA_Stat 0x%x\n",inval);
*/

	if((SaveValue & (EISA_DIEN | EISA_STATD)) == (EISA_DIEN | EISA_STATD)){

	 i = EISA_STATD;
#ifdef BADLINT
	 outb(pAd->io_base + EISA_STAT, SaveValue & ~EISA_DIEN);
#else
	 outb(pAd->io_base + EISA_STAT, (unchar)(SaveValue & ~EISA_DIEN));
#endif

	 if(pAd->Rcv_DMA_Pending){
            pAd->Rcv_DMA_Pending = 0;
		
	 /*
	  * Call service routine
          */
	    if(pAd->Rcv_Buffer_Pending){
#ifdef LM_STREAMS
	       sebmrecv(pAd->Rcv_Buffer_Pending,&sebmdevs[pAd->sebm_firstd]);
#else
		UM_Receive_Packet(0,length, pAd, 0);
#endif
	       pAd->Rcv_Buffer_Pending = 0;
            }
	    inval = inb(cmd_reg);
#ifdef BADLINT
	    outb(cmd_reg, inval & PG_MSK & ~CMD_TXP);
#else
	    outb(cmd_reg, (unchar)(inval & PG_MSK & ~CMD_TXP));
#endif
	    if ((pAd->sebm_nxtpkt-1) < pAd->StartBuffer){
#ifdef BADLINT
	      outb(ctl_reg + REG_BNRY, pAd->LastBuffer - 1);
#else
	      outb(ctl_reg + REG_BNRY, (unchar)(pAd->LastBuffer - 1));
#endif
            }
	    else{
#ifdef BADLINT
	      outb(ctl_reg + REG_BNRY, pAd->sebm_nxtpkt-1);
#else
	      outb(ctl_reg + REG_BNRY, (unchar)(pAd->sebm_nxtpkt-1));
#endif
            }
	 }
	 if(pAd->Xmit_DMA_Pending){
	    pAd->Xmit_DMA_Pending = 0;
	 }

	}
   if(SaveValue & EISA_STATD){
	i = EISA_STATD;
        inval = inb(pAd->io_base + EISA_INT) & 0x7F;
        outb(pAd->io_base + EISA_INT,inval);

   }


   /* make sure CR is at page 0 */
   orig = inb(cmd_reg);
#ifdef BADLINT
   outb(cmd_reg, orig & PG_MSK & ~CMD_TXP);
#else
   outb(cmd_reg, (unchar)(orig & PG_MSK & ~CMD_TXP));
#endif

   if ((int_reg = inb(ctl_reg + REG_ISR)) == 0 && !i){
#ifdef BUG
         binit_msg("sebmnintr: spurious interrupt\n");
#endif
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMINTR)
         printf("sebmnintr: spurious interrupt\n");
#endif
#ifdef DLPI
       ebmacstats[pAd->adapter_num].mac_spur_intr++;
#endif
#ifdef BUG
      return;
#else
	outb(ctl_reg + REG_IMR, pAd->InterruptMask);
	return (NOT_MY_INTERRUPT);
#endif
   }

   /* mask off bits that will be handled */
   outb(ctl_reg + REG_ISR, int_reg);

#if defined(SEBMDEBUG)
   if (sebm_debug&SEBMINTR)
     printf("sebmnintr(): int_reg=%x\n", int_reg);
#endif

   if (int_reg & ISR_CNT) {
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMINTR)
	printf("CNT\n");
#endif
#ifdef	DLPI
      ebmacstats[pAd->adapter_num].mac_align += inb(ctl_reg + REG_CNTR0);
      ebmacstats[pAd->adapter_num].mac_badsum += inb(ctl_reg + REG_CNTR1);
		inval = inb(ctl_reg + REG_CNTR2);
		ebmacstats[pAd->adapter_num].mac_no_resource += inval;
		ebmacstats[pAd->adapter_num].mac_frame_recv  += inval;
#else
      sebmstats[pAd->adapter_num].sebms_align += inb(ctl_reg + REG_CNTR0);
      sebmstats[pAd->adapter_num].sebms_crc   += inb(ctl_reg + REG_CNTR1);
      sebmstats[pAd->adapter_num].sebms_lost  += inb(ctl_reg + REG_CNTR2);
#endif

   }

   if (int_reg & ISR_OVW) {
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMOVERFLOW)
	printf("OVW %d\r", sebmstats[pAd->adapter_num].sebms_ovw);
#endif
#ifdef DLPI
      ebmacstats[pAd->adapter_num].mac_baddma ++;
#else
      sebmstats[pAd->adapter_num].sebms_ovw++;
#endif

     /* Case where we got Ring Overflow and there is  */
     /* nothing in the ring.  Just rewrite boundary.			  */

     inval = inb(cmd_reg);
#ifdef BADLINT
     outb(cmd_reg, (inval & PG_MSK & ~CMD_TXP) | CMD_PAGE1);
#else
     outb(cmd_reg, (unchar)((inval & PG_MSK & ~CMD_TXP) | CMD_PAGE1));
#endif

     if (pAd->sebm_nxtpkt == (unsigned char) inb(ctl_reg + REG_CURR)) {
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMOVERFLOW)
	printf("OVW and empty ring\n");
#endif
	 /* set CR to page 0 & set BNRY to new value */
	 inval = inb(cmd_reg);
#ifdef BADLINT
	 outb(cmd_reg, inval & PG_MSK & ~CMD_TXP);
#else
	 outb(cmd_reg, (unchar)(inval & PG_MSK & ~CMD_TXP));
#endif
	 inval = inb(ctl_reg + REG_BNRY);
	 outb(ctl_reg + REG_BNRY, inval);
     }

   }

   if (int_reg & ISR_PRX) {

#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMINTR)
	printf("PRX: nxtpkt = %x\n", pAd->sebm_nxtpkt);
#endif

      inval = inb(cmd_reg);
#ifdef BADLINT
      outb(cmd_reg, (inval & PG_MSK & ~CMD_TXP) | CMD_PAGE1);
#else
      outb(cmd_reg, (unchar)((inval & PG_MSK & ~CMD_TXP) | CMD_PAGE1));
#endif

      SaveValue = inb(pAd->io_base+EISA_RAM);
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMINTR){
	printf("PRX: ReadERAM = %x\n", SaveValue);
      printf("PRX: nxtpkt = %x Curr = 0x%x\n", pAd->sebm_nxtpkt,inb(ctl_reg+REG_CURR));
	}
#endif

      while (pAd->sebm_nxtpkt != (UCHAR)inb(ctl_reg + REG_CURR)){ 
#ifdef DLPI
    ebmacstats[pAd->adapter_num].mac_frame_recv++;
#else
	 sebmstats[pAd->adapter_num].sebms_rpkts++;
#endif

#ifdef LAI_TCP
	 sebms_ifstats[pAd->adapter_num].ifs_ipackets++;
#endif
	 /*
	  * set up ptr to packet & update nxtpkt
          */

	 Page_Offset = (USHORT)(pAd->sebm_nxtpkt << 8);
#ifndef LM_STREAMS
	 pAd->Page_Offset = Page_Offset;
#endif
#ifdef CODE_386
	 ram_rp = (rcv_buf_t *)(pAd->ram_access +(Page_Offset&
				pAd->page_offset_mask));
#else
	 ram_rp = (rcv_buf_t far *)(pAd->ram_access +(Page_Offset&
				pAd->page_offset_mask));
#endif
	 pAd->Page_Num = (UCHAR)(pAd->sebm_nxtpkt >> 5);

#if defined(SEBMDEBUG)
	    if (sebm_debug&SEBMINTR){
	    printf ("\nCurrent Receive PacketRAM 0x%x Host Buff: 0x%x\n",Page_Offset,ram_rp);
    printf ("0)CurPage: 0x%x  Nxtpage: 0x%x",inb(pAd->io_base+EISA_RAM),pAd->Page_Num);
	}
#endif

		SyncSetWindowPage(pAd,pAd->Page_Num);

#ifdef SEBMDEBUG
	    if (sebm_debug&SEBMINTR){
    printf ("\nEISA_INT = 0x%x EISA_RAM = 0x%x NIC_RAM = 0x%x",inb(pAd->io_base +EISA_INT),inb(pAd->io_base+EISA_RAM),inb(ctl_reg + REG_BPR));
	}
#endif
	   sebmbcopy((caddr_t) ram_rp, (caddr_t)&rbuf, sizeof(rcv_buf_t),
			   SRC_ALIGN);

		rp = &rbuf;

#if defined(SEBMDEBUG)
	    if (sebm_debug>50){
	    printf("\nLookAhead:Curpage=%x status=%x, nxtpg=%x, datalen=%d\n",
		   pAd->sebm_nxtpkt,rp->status, rp->nxtpg, rp->datalen);
		}
#endif

	 pAd->sebm_nxtpkt = rp->nxtpg;
	 if((rp->status & 0x01) == 0){

	    if (sebm_debug>50){
	   printf("\nBadStatus:Curpage=%x status=%x, nxtpg=%x, datalen=%d\n",
		   pAd->sebm_nxtpkt,rp->status, rp->nxtpg, rp->datalen);
		}
           continue;
	 }

	 if( ( rp->nxtpg > pAd->LastBuffer) || (rp->nxtpg < pAd->StartBuffer) ){
#if defined(SEBMDEBUG)
	    if (sebm_debug>50){
	    printf("Bad nxtpkt value: nxtpkt=%x nxtpg = %x ",pAd->sebm_nxtpkt,rp->nxtpg);
	    printf("status=%x, datalen=%d, ramptr = 0x%x\n",
		   rp->status, rp->datalen,ram_rp);
	    for (i=0;i < 20;i++)
		printf ("%x ",ram_rp[i]);
	    }
#endif
            inval = inb(cmd_reg);
#ifdef BADLINT
            outb(cmd_reg, (inval & PG_MSK & ~CMD_TXP) | CMD_PAGE1);
#else
            outb(cmd_reg, (unchar)((inval & PG_MSK & ~CMD_TXP) | CMD_PAGE1));
#endif
	    pAd->sebm_nxtpkt = rp->nxtpg= inb(ctl_reg + REG_CURR);
#ifdef BADLINT
            outb(cmd_reg, inval & PG_MSK & ~CMD_TXP);
#else
            outb(cmd_reg, (unchar)(inval & PG_MSK & ~CMD_TXP));
#endif
	    if (pAd->sebm_nxtpkt > pAd->LastBuffer) {
	      pAd->sebm_nxtpkt = (pAd->sebm_nxtpkt - pAd->LastBuffer) + pAd->StartBuffer;
	    }
	    if ((pAd->sebm_nxtpkt -1) < pAd->StartBuffer){
#ifdef BADLINT
	      outb(ctl_reg + REG_BNRY, pAd->LastBuffer - 1);
#else
	      outb(ctl_reg + REG_BNRY, (unchar)(pAd->LastBuffer - 1));
#endif
            }
	    else{
#ifdef BADLINT
	      outb(ctl_reg + REG_BNRY, pAd->sebm_nxtpkt - 1);
#else
	      outb(ctl_reg + REG_BNRY, (unchar)(pAd->sebm_nxtpkt - 1));
#endif
            }
	    break;
	 }

	 hwlength = rp->datalen - 4;

	 /* get length of packet w/o CRC field */

	 length = LLC_LENGTH(&rp->pkthdr);
	 if (length > SEBMMAXPKT) {
		length = hwlength;		/* DL_ETHER */
	 } else {
		/* DL_CSMACD */
		/* rp->datalen can be wrong (hardware bug) -- use llc length */
		/* the llc length is 18 bytes shorter than datalen... */
			length += 14;
			if (length >= SEBMMINRECV) {	/* [1.07(i)] */
				if (length != hwlength && length+pAd->padlen!=hwlength)
						hwlength = 0;
				} else if (hwlength != SEBMMINRECV) {
						hwlength = 0;
			}

	 }
#if defined(SEBMDEBUG)
	if(sebm_debug > 30)
	    printf("\nIn PRX Adjusted packet_len w/o CRC =0x%x",length);
#endif

#ifdef DLPI
    ebmacstats[pAd->adapter_num].mac_ioctets += length;
	 if ( (length > SEBMMAXPKT+LLC_EHDR_SIZE) || (hwlength < SEBMMINRECV) ) {
		ebmacstats[pAd->adapter_num].mac_badlen ++;
#else
	 if ( (length > SEBMMAXPKT+LLC_EHDR_SIZE) || (hwlength < SEBMMINRECV) ) {
#endif
	     /* garbage packet? - toss it */
	     call_sebmsched++;
	     /* set CR to page 0 & set BNRY to new value */
	     inval = inb(cmd_reg);
#ifdef BADLINT
	     outb(cmd_reg, inval & PG_MSK & ~CMD_TXP);
#else
	     outb(cmd_reg, (unchar)(inval & PG_MSK & ~CMD_TXP));
#endif
	     if ((pAd->sebm_nxtpkt-1) < pAd->StartBuffer)
#ifdef BADLINT
	       outb(ctl_reg + REG_BNRY, pAd->LastBuffer-1);
#else
	       outb(ctl_reg + REG_BNRY, (unchar)(pAd->LastBuffer-1));
#endif
	     else
#ifdef BADLINT
	       outb(ctl_reg + REG_BNRY, pAd->sebm_nxtpkt-1);
#else
	       outb(ctl_reg + REG_BNRY, (unchar)(pAd->sebm_nxtpkt-1));
#endif
	     break;
	 }

#ifdef LM_STREAMS
	 /* get buffer to put packet in & move it there */
	 if ((bp = allocb(length, BPRI_MED)) != NULL ||
	     (bp = allocb(nextsize(length), BPRI_MED)) != NULL) {
	    caddr_t dp, cp;
	    unsigned cnt;

	    /* dp -> data dest; ram_rp -> llc hdr */
	    dp = (caddr_t) bp->b_wptr;
	    cp = (caddr_t) &ram_rp->pkthdr;

	    /* set new value for b_wptr */
	    bp->b_wptr = bp->b_rptr + length;

	    /*
	     * See if there is a wraparound. If there
	     * is remove the packet from its start to
	     * rcv_stop, set cp to rcv_start and remove
	     * the rest of the packet. Otherwise, re-
	     * move the entire packet from the given
	     * location.
	     */

		Page_Offset += 4;
		Ram_Offset = Page_Offset & pAd->page_offset_mask;
		Move_Len =0;
            if((pAd->adapter_bus==BUS_EISA32M_TYPE) && (length >= DMA_CUT_OFF)){

		if(!pAd->Rcv_DMA_Pending && !pAd->Xmit_DMA_Pending)
		 {
		    pAd->Rcv_Buffer_Pending = (caddr_t)bp;
		    if(Page_Offset + length >= 0x8000) {
		      Move_Len = 0x8000 - Page_Offset;
		      EBM_Xfer(pAd,dp,Page_Offset,Move_Len,EISA_SZ_DIR);
		      length -= Move_Len;
		      dp +=Move_Len;	/* host ram */
		      Page_Offset = (USHORT)((ULONG)pAd->rcv_start - pAd->ram_access); 
		    }
#ifdef SEBMDEBUG
	if(sebm_debug > 10)
		    binit_msg("r");
#endif
		    while(1){
   			inval = inb(pAd->io_base + EISA_STAT);
			if(!(inval & 0x02))
			   break;	
#ifdef SEBMDEBUG
	if(sebm_debug > 10)
			binit_msg("P");
#endif
			}
		    EBM_Xfer(pAd,dp,Page_Offset,length,EISA_SZ_DIR);
		 }
   		inval = inb(pAd->io_base + EISA_STAT);

	        if((inval & (EISA_DIEN | EISA_STATD)) == (EISA_DIEN | EISA_STATD)){
		  if(pAd->Rcv_Buffer_Pending){
	            sebmrecv(pAd->Rcv_Buffer_Pending,&sebmdevs[pAd->sebm_firstd]);
	            pAd->Rcv_Buffer_Pending = 0;
		  }
		  if(pAd->Rcv_DMA_Pending){
	            outb(pAd->io_base + EISA_STAT, inval & ~EISA_DIEN);
	            pAd->Rcv_DMA_Pending = 0;
                  }
		  else{
#ifdef SEBMDEBUG
	if(sebm_debug > 10)
			binit_msg("*");
#endif
		  }
		 }
	    }
	    else
	         {		

		if((Ram_Offset+length) >= 0x2000) {
		   Move_Len = 0x2000 - Ram_Offset;
#if defined(SEBMDEBUG)
	if(sebm_debug > 30)
    printf ("\n1)CurPage: 0x%x  Nxtpage: 0x%x MLen=0x%x",inb(pAd->io_base+EISA_RAM),pAd->Page_Num,Move_Len);
#endif
		   SyncSetWindowPage(pAd,pAd->Page_Num);
	      	   sebmbcopy( cp, dp, Move_Len, SRC_ALIGN);
		   length -= Move_Len;
		   dp +=Move_Len;	/* host ram */
		   pAd->Page_Num += 1;
		   if(pAd->Page_Num == 4)
		   {
#if defined(SEBMDEBUG)
	if(sebm_debug > 30)
			printf("\n Reaching PBound!!!");
#endif
			pAd->Page_Num =0;
		        cp = pAd->rcv_start;
		   }else{
		        cp =(caddr_t)pAd->ram_access;
#if defined(SEBMDEBUG)
	if(sebm_debug > 30)
			printf("\n Copy from here 0x%x",cp);
#endif
		   }
#if defined(SEBMDEBUG)
	if(sebm_debug > 30)
		printf("\nPO=0x%x Set_Page #0x%x\n",Page_Offset+Move_Len,pAd->Page_Num);
#endif
		SyncSetWindowPage(pAd,pAd->Page_Num);
		}
#if defined(SEBMDEBUG)
	if(sebm_debug > 30)
		printf("\nPO=0x%x Set_Page #0x%x LLeft 0x%x\n",Page_Offset+Move_Len,pAd->Page_Num,length);
#endif
	      	if(length)
		    sebmbcopy( cp, dp, length, SRC_ALIGN);
	 /*
	  * Call service routine
          */

	    sebmrecv(bp,&sebmdevs[pAd->sebm_firstd]);

	  } /* end if not DMA */

	 } else {
	    /* keep track for possible management */
#ifdef DLPI
		ebmacstats[pAd->adapter_num].mac_frame_nosr++;
#else
	    sebmstats[pAd->adapter_num].sebms_nobuffer ++;
#endif
#if defined(SEBMDEBUG)
	    if (sebm_debug&SEBMBUFFER)
	      printf("sebmnintr: no buffers (%d)\n", length);
#endif
	    call_sebmsched++;
	 }	 /* end if */

#else /* -LM_STREAMS */

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
	if((pAd->adapter_bus==BUS_EISA32M_TYPE) && (length >= DMA_CUT_OFF)) {
		if(!pAd->Rcv_DMA_Pending && !pAd->Xmit_DMA_Pending) {
			pAd->CurRcvbuf = (caddr_t) &ram_rp->pkthdr;
			UM_Receive_Packet(0,length, pAd, 0);
		}
   		inval = inb(pAd->io_base + EISA_STAT);

	        if((inval & (EISA_DIEN | EISA_STATD)) == (EISA_DIEN | EISA_STATD)){
			if(pAd->Rcv_Buffer_Pending) {
				UM_Receive_Packet(0,length, pAd, 0);
				pAd->Rcv_Buffer_Pending = 0;
			}
			if(pAd->Rcv_DMA_Pending){
				outb(pAd->io_base + EISA_STAT, inval & ~EISA_DIEN);
				pAd->Rcv_DMA_Pending = 0;
			}
		}
	} else {
#endif /* -REALMODE && -Solaris */
		pAd->CurRcvbuf = (caddr_t) &ram_rp->pkthdr;
		UM_Receive_Packet(0,length, pAd, 0);
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
	}
#endif
		
#endif /* LM_STREAMS */

	 /* set CR to page 0 & set BNRY to new value */
	 if(!pAd->Rcv_DMA_Pending){
	 inval = inb(cmd_reg);
#ifdef BADLINT
	 outb(cmd_reg, inval & PG_MSK & ~CMD_TXP);
#else
	 outb(cmd_reg, (unchar)(inval & PG_MSK & ~CMD_TXP));
#endif 
	 if ((pAd->sebm_nxtpkt-1) < pAd->StartBuffer){
#ifdef BADLINT
	   outb(ctl_reg + REG_BNRY, pAd->LastBuffer - 1);
#else
	   outb(ctl_reg + REG_BNRY, (unchar)(pAd->LastBuffer - 1));
#endif
#if defined(SEBMDEBUG)
	if(sebm_debug)
	   printf("\nBUPDAT 0x%x",(pAd->ram_size<<2) -1);
#endif
         }
	 else{
#ifdef BADLINT
	   outb(ctl_reg + REG_BNRY, pAd->sebm_nxtpkt-1);
#else
	   outb(ctl_reg + REG_BNRY, (unchar)(pAd->sebm_nxtpkt-1));
#endif
#if defined(SEBMDEBUG)
	if(sebm_debug)
	   printf("\nBUPDAT 0x%x",(pAd->sebm_nxtpkt-1));
#endif
         }
	 inval = inb(cmd_reg);
#ifdef BADLINT
	 outb(cmd_reg, (inval & PG_MSK & ~CMD_TXP) | CMD_PAGE1);
#else
	 outb(cmd_reg, (unchar)((inval & PG_MSK & ~CMD_TXP) | CMD_PAGE1));
#endif
	 }
	 else
	   break;

    } /* end while */

       outb(pAd->io_base+EISA_RAM,SaveValue);

   } /* end if PRX int */


   /* restore CR to page 0 */
   inval = inb(cmd_reg);
#ifdef BADLINT
   outb(cmd_reg, inval & PG_MSK & ~CMD_TXP);
#else
   outb(cmd_reg, (unchar)(inval & PG_MSK & ~CMD_TXP));
#endif

   if (int_reg & ISR_RXE) {
#ifdef DLPI
		unsigned char rcv_status;

      rcv_status = inb(ctl_reg + REG_RSR);
      if(rcv_status & RSR_FO)
		  ebmacstats[pAd->adapter_num].mac_baddma++;
#endif
	     
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMINTR)
	printf("RXE\n");
#endif
#ifdef DLPI
      ebmacstats[pAd->adapter_num].mac_align += inb(ctl_reg + REG_CNTR0);
      ebmacstats[pAd->adapter_num].mac_badsum += inb(ctl_reg + REG_CNTR1);
		inval = inb(ctl_reg + REG_CNTR2);
		ebmacstats[pAd->adapter_num].mac_no_resource += inval;
		ebmacstats[pAd->adapter_num].mac_frame_recv  += inval;
#else

      /* clear network tally counters */
      sebmstats[pAd->adapter_num].sebms_align += inb(ctl_reg + REG_CNTR0);
      sebmstats[pAd->adapter_num].sebms_crc   += inb(ctl_reg + REG_CNTR1);
      sebmstats[pAd->adapter_num].sebms_lost  += inb(ctl_reg + REG_CNTR2);
#endif

   }

   if (int_reg & ISR_PTX) {
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMINTR)
	printf("PTX\n");
#endif
#ifndef DLPI
      sebmstats[pAd->adapter_num].sebms_xpkts++;
#endif
#ifdef LAI_TCP
      sebms_ifstats[pAd->adapter_num].ifs_opackets++;
#endif

      /* free the transmit buffer */
#ifndef MULTI_TX_BUFS
      pAd->sebm_txbuf_busy = 0;
      nxttbuf = pAd->curtxbuf;
      if(pAd->sebm_txbufstate[nxttbuf] != TX_MTING)  
	nxttbuf ^= 1;
      pAd->sebm_txbufstate[nxttbuf] = TX_FREE;
#else
      nxttbuf=pAd->sebm_txbuf_busy;
      pAd->sebm_txbufstate[nxttbuf]=TX_FREE;
      pAd->sebm_txbuf_busy = -1;
#endif

      pAd->XmitRetry = DEFAULT_TX_RETRIES;

      ts_reg = inb(ctl_reg + REG_TSR);
      if (ts_reg&TSR_COL) {
        unsigned cnt = inb(ctl_reg + REG_NCR); /* read #  collision */

#ifdef DLPI
		ebmacstats[pAd->adapter_num].mac_collisions  += cnt;
		ebmacstats[pAd->adapter_num].mac_frame_coll  += cnt;
#endif

#ifdef LAI_TCP
        sebms_ifstats[pAd->adapter_num].ifs_collisions += cnt;
#endif
      }
      if (ts_reg & TSR_CRS)

#ifdef DLPI
        ebmacstats[pAd->adapter_num].mac_carrier++;
#endif

      UMB_Send_Complete(pAd);
      call_sebmsched++;
#ifdef MULTI_TX_BUFS

      if((nxttbuf=pAd->sebm_txbufload) >= 0) {
                txbufoff=pAd->sebm_txbufaddr[nxttbuf];
                len=pAd->sebm_txbuflen[nxttbuf];
                pAd->sebm_txbuf_busy=nxttbuf;
		
		/* Tell the 83C790 to start the transmission. */
   		i = inb(cmd_reg);
   		outb( cmd_reg, (unchar)(i & PG_MSK & ~CMD_TXP));
 
   		outb( ctl_reg + REG_TPSR,(unchar)(txbufoff>>8));       /* xmit page start */
   		outb( ctl_reg + REG_TBCR0, (unsigned char) len);
   		outb( ctl_reg + REG_TBCR1, (unsigned char)(len >> 8));
 
   		i = inb(cmd_reg);
   		outb( cmd_reg, (unchar)(i | CMD_TXP));

                pAd->sebm_txbufstate[nxttbuf] = TX_MTING;
                nxttbuf = (nxttbuf+1) % pAd->num_of_tx_buffs;
                if(pAd->sebm_txbufstate[nxttbuf] == TX_LOADED) {
                                pAd->sebm_txbufload=nxttbuf;
                } else {
                                pAd->sebm_txbufload=-1;
                }
         }
#endif /* MULTI_TX_BUFS */

   }

   if (int_reg & ISR_TXE) {
#if defined(SEBMDEBUG)
      if (sebm_debug&SEBMINTR)
	printf("TXE\n");
#endif
      
      /* free the transmit buffer */
#ifndef MULTI_TX_BUFS
      pAd->sebm_txbuf_busy = 0;
      nxttbuf = pAd->curtxbuf;
      if(pAd->sebm_txbufstate[nxttbuf] != TX_MTING)  
	nxttbuf ^= 1;
      pAd->sebm_txbufstate[nxttbuf] = TX_FREE;
#else
      nxttbuf=pAd->sebm_txbuf_busy;
      pAd->sebm_txbufstate[nxttbuf]=TX_FREE;
      pAd->sebm_txbuf_busy = -1;

#endif
      ts_reg = inb(ctl_reg + REG_TSR);
      if (ts_reg&TSR_COL) {

        unsigned cnt = inb(ctl_reg + REG_NCR); /* read #  collision */
#ifdef DLPI
		  ebmacstats[pAd->adapter_num].mac_collisions  += cnt;
		  ebmacstats[pAd->adapter_num].mac_oframe_coll++;
#endif

#ifdef LAI_TCP
        sebms_ifstats[pAd->adapter_num].ifs_collisions += cnt;
#endif
      }
      if (ts_reg&TSR_ABT)
#ifdef DLPI
		  ebmacstats[pAd->adapter_num].mac_xs_coll++;
#endif
		if (ts_reg&TSR_FU)
#ifdef DLPI
		  ebmacstats[pAd->adapter_num].mac_baddma++;
#endif
      if (ts_reg & TSR_CRS)
#ifdef DLPI
        ebmacstats[pAd->adapter_num].mac_carrier++;
#endif
			
      UMB_Send_Complete(pAd);
      call_sebmsched++;

#ifdef MULTI_TX_BUFS
      if((nxttbuf=pAd->sebm_txbufload) >= 0) {
                txbufoff=pAd->sebm_txbufaddr[nxttbuf];
                len=pAd->sebm_txbuflen[nxttbuf];
                pAd->sebm_txbuf_busy=nxttbuf;
                
                /* Tell the 83C790 to start the transmission. */ 
                i = inb(cmd_reg);
                outb( cmd_reg, (unchar)(i & PG_MSK & ~CMD_TXP)); 
  
                outb( ctl_reg + REG_TPSR,(unchar)(txbufoff>>8));       /* xmit page start */
                outb( ctl_reg + REG_TBCR0, (unsigned char) len); 
                outb( ctl_reg + REG_TBCR1, (unsigned char)(len >> 8)); 
  
                i = inb(cmd_reg); 
                outb( cmd_reg, (unchar)(i | CMD_TXP));
 
                pAd->sebm_txbufstate[nxttbuf] = TX_MTING;
                nxttbuf = (nxttbuf+1) % pAd->num_of_tx_buffs;
                if(pAd->sebm_txbufstate[nxttbuf] == TX_LOADED) {
                                pAd->sebm_txbufload=nxttbuf;
                } else {
                                pAd->sebm_txbufload=-1;
                }
         }
#endif /* MULTI_TX_BUFS */
   }

#ifdef LM_STREAMS
   if (call_sebmsched)
     sebmsched(&sebmdevs[pAd->sebm_firstd]); /* reschedule blocked writers */
#endif

   /* it should be safe to do this */
   /* here */

   outb(ctl_reg + REG_IMR, pAd->InterruptMask);
#ifdef BADLINT
   outb(cmd_reg, orig & ~CMD_TXP);	/* put things back the way they were found */
#else
   outb(cmd_reg, (unchar)(orig & ~CMD_TXP));	/* put things back the way they were found */
#endif
					/*(but don't set TXP bit)   */
	return(SUCCESS);
}

#ifdef DEADCODE
Enable_59x_irq(pAd)
Ptr_Adapter_Struc pAd;
{
		 UCHAR RegValue;

		 RegValue = inb(pAd->io_base + REG_CR);
		 RegValue |= CR_EIL;
		 outb(pAd->io_base + REG_CR,RegValue);
		 pAd->LaarEnter = pAd->LaarExit = RegValue;
}

Enable_NIC_irq(pAd)
Ptr_Adapter_Struc pAd;
{
		UCHAR RegValue;

		outb(pAd->io_base + REG_CMD,CMD_PAGE0);
	/*
	 * init InterruptMask in LM_Change_Receive_Mask
	 */
		outb(pAd->io_base	+ REG_IMR,pAd->InterruptMask); 
}
#endif /* DEADCODE */

Enable_585_irq(pAd)
Ptr_Adapter_Struc pAd;
{
#ifdef EBM
		 outb(pAd->io_base +EISA_790 + REG_INTCR,INTCR_EIL);
#else
		 outb(pAd->io_base + REG_INTCR,INTCR_EIL);
#endif
}

#ifdef DEADCODE
Disable_59x_irq(pAd)
Ptr_Adapter_Struc pAd;
{
		 UCHAR RegValue;

		 RegValue = inb(pAd->io_base + REG_CR);
		 RegValue &= ~CR_EIL;
		 outb(pAd->io_base + REG_CR,RegValue);
		 pAd->LaarEnter = pAd->LaarExit = RegValue;
}
#endif /* DEADCODE */

Disable_585_irq(pAd)
Ptr_Adapter_Struc pAd;
{
		 UCHAR RegValue;
#ifdef EBM
		 RegValue = inb(pAd->io_base + EISA_790 +REG_INTCR);
		 RegValue &= ~(INTCR_EIL + INTCR_SINT);
		 outb(pAd->io_base + REG_INTCR+EISA_790,RegValue);
#else
		 RegValue = inb(pAd->io_base + REG_INTCR);
		 RegValue &= ~(INTCR_EIL + INTCR_SINT);
		 outb(pAd->io_base + REG_INTCR,RegValue);
#endif
}

#ifdef DEADCODE
Disable_NIC_irq(pAd)
Ptr_Adapter_Struc pAd;
{
		UCHAR RegValue;

		outb(pAd->io_base + REG_CMD,CMD_PAGE0);
		outb(pAd->io_base	+ REG_IMR,0);

}
#endif /* DEADCODE */

SyncSetWindowPage(pAd,Value)
Ptr_Adapter_Struc pAd;
UCHAR Value;

/*++

Routine Description:

    This function is used to synchronize with the interrupt, switching
    to page 1 to set the mulitcast filter then switching back to page 0.

Arguments:

    Base_IO and Value to be written

Notes:

    return NONE

--*/

{
#ifdef M_XENIX
	int splvar;
#endif
	UCHAR RegValue;
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
	intr_disable();
#endif
   /*
    *	Write the page #
    */
	RegValue = pAd->BaseRamWindow;
	
#ifdef BADLINT
	outb(pAd->io_base + EISA_RAM,RegValue | Value);
#else
	outb(pAd->io_base + EISA_RAM,(unchar)(RegValue | Value));
#endif

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
	intr_restore();
#endif
}

SyncSetMAR(Base_IO,Value)
int Base_IO;
UCHAR Value;

/*++

Routine Description:

    This function is used to synchronize with the interrupt, switching
    to page 1 to set the mulitcast filter then switching back to page 0.

Arguments:

    Base_IO and Value to be written

Notes:

    return NONE

--*/

{
#ifdef M_XENIX
	int splvar;
#endif
	UCHAR RegValue;
	register int i;

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
	intr_disable();
#endif
   /*
    *	Save the page #
    */

	RegValue = inb(Base_IO+REG_CMD) & (CMD_PS0+CMD_PS1);
	
	outb(Base_IO+REG_CMD,CMD_PAGE1);

   /*
    *  Set/Clear the multicast filter bits
    */

   for (i = 0; i < 8; i++)

     outb(Base_IO + REG_MAR0 + i, Value);

   /*
    * Back to the original page 
    */

	outb(Base_IO+REG_CMD,RegValue);

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
	intr_restore();
#endif
}	

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
EBM_Xmit(pAd,dp,cp,dmalen,dir)
Ptr_Adapter_Struc pAd;
caddr_t dp;
int cp;
ushort dmalen;
ushort dir;
{
    paddr_t phys_addr;
    paddr_t paddrbnd;
    ULONG TarSize;
    UCHAR inval;
    register USHORT bcp, rbcp, rlen;
    register int dst;

    dst = (int) pAd->ram_access + cp;
    phys_addr = (paddr_t)svirtophys(dp);

    paddrbnd = (phys_addr + dmalen) & 0x3ff;
    if(paddrbnd <= 0x10) {
        sebmbcopy( dp,dst, dmalen, DEST_ALIGN);
	return(SUCCESS);
    }

    paddrbnd = phys_addr & 0x3ff;
    if(paddrbnd>=0x3e0) {		/* 1st frag <= 32bytes */
	bcp = 0x400 - paddrbnd;
	if(bcp>dmalen) bcp = dmalen;
        sebmbcopy( dp,dst,bcp, DEST_ALIGN);
	dmalen -= bcp;
	if(dmalen==0) return(SUCCESS);
	dst+=(int) bcp;
	dp+=bcp;
	cp+=bcp;
	phys_addr+=bcp;
     }

    while(dmalen)
    {
/*
 * Need to split it in smaller chunk to work around HW problem
 */ 

	paddrbnd = phys_addr & 0x3ff;
	bcp = 0x400 - paddrbnd;
	if(bcp > dmalen) bcp = dmalen;

	if (bcp > 0x10){
	      inval = inb(pAd->io_base + EISA_STAT);

#if defined(SEBMDEBUG)
	if(sebm_debug)
        printf("In DMA EISA_Stat 0x%x\n",inval);
#endif

	      if(inval & EISA_SDMA)	/* 0x02 */
	      {
                 sebmbcopy( dp,dst,bcp, DEST_ALIGN);
	      }
	      else{

#if defined(SEBMDEBUG)
	if(sebm_debug)
	printf("DMA from 0x%x to 0x%x len 0x%x\n",phys_addr,cp,bcp);
#endif
		if(paddrbnd+bcp>0x3f0) {
			rlen = paddrbnd+bcp-0x3f0;
			rbcp = bcp-rlen;
		} else {
			rbcp = bcp;
			rlen = 0;
		}

		if(rbcp < 0x20) {	/* if < 32 bytes, NO DMA */
			rlen+=rbcp;
			rbcp=0;
		}
		if(rbcp>0) {

                 TarSize = (ULONG)(cp) << 16;

	         TarSize |=((rbcp & 0x3FFF) | EISA_SZ_SDMA | dir);

	         pAd->Xmit_DMA_Pending = 1;

	         outl(pAd->io_base+EISA_AA,(ULONG)(phys_addr));

#if defined(SEBMDEBUG)
	if(sebm_debug)
        printf("Start DMA 0x%x\n",TarSize);
#endif
	         outl(pAd->io_base+EISA_AA+4,TarSize);
		/* since ebm becomes bdm! */
		 sdmafail=0;
		 while(inp(pAd->io_base+EISA_STAT) & EISA_SDMA){
			sdmafail++;
			inb(pAd->io_base+EISA_ID1);
			if(sdmafail>=0xffff) {
      				pAd->adapter_bus = BUS_EISA32S_TYPE;	
      				printf("SMC Ethernet Adapter runs under SLAVE mode. \n");
				break;
			}
		 }
		 if(sdmafail>=0xffff) {
			sebmbcopy(dp, dst, bcp, DEST_ALIGN);
			rlen=0;
		 } 
      		 inval = inb(pAd->io_base + EISA_INT);
      		 outb(pAd->io_base + EISA_INT, (inval | EISA_CLRD)); 
		}
		if(rlen>0) 
                 	sebmbcopy( dp+rbcp,dst + rbcp,rlen, DEST_ALIGN);
             }				
	}
	else {		/* case 2 ; length <= 16 after 1k */
                  sebmbcopy( dp,dst,bcp, DEST_ALIGN);
	}
	dmalen -= bcp;
	if(dmalen==0) break;
	dst += (int) bcp; 
	dp += bcp;
	cp += bcp;
	phys_addr += bcp;
   }   	          /* end while */	

	pAd->Xmit_DMA_Pending = 0;
	return(SUCCESS);
}


EBM_Xfer(pAd,dp,cp,length,dir)
Ptr_Adapter_Struc pAd;
caddr_t dp;
caddr_t cp;
ushort length;
ushort dir;
{
	paddr_t phys_addr;
	ULONG	TarSize;
	UCHAR inval;

	phys_addr = (paddr_t)svirtophys(dp);

	/*
	 *	 Load Dest Host PA
	 */
#if defined(SEBMDEBUG)
	if(sebm_debug)
	printf("DMA from 0x%x to 0x%x\n",cp,phys_addr);
#endif

	outl(pAd->io_base+EISA_AA,(ULONG)(phys_addr));

	/*
	 *	 Load Src SR PA
	 */
        TarSize = (ULONG)(cp) << 16;

	/*
	 *	 Load count
	 */

	TarSize |=((length & 0x3FFF) | EISA_SZ_SDMA | dir);

	if( dir )
	  pAd->Rcv_DMA_Pending = 1;
	else
	  pAd->Xmit_DMA_Pending = 1;

   	inval = inb(pAd->io_base + EISA_STAT);

/*
        printf("In DMA EISA_Stat 0x%x\n",inval);
*/

	outb(pAd->io_base + EISA_STAT, inval | EISA_DIEN); 

#if defined(SEBMDEBUG)
	if(sebm_debug)
        printf("Start DMA 0x%x\n",TarSize);
#endif

	outl(pAd->io_base+EISA_AA+4,TarSize);

	return(SUCCESS);
}
#endif /* -REALMODE && -Solaris */

#if ((UNIX & 0xff00) == 0x400)	/* Solaris 2.x */
LM_STATUS
LMB_Add_Multi_Address(pAd)
Ptr_Adapter_Struc pAd;
{
	/*
	 * This is not used.  This LM driver relies on the use of
	 * Change_Receive_Mask() only.
	 */
	return(SUCCESS);
}

LM_STATUS
LMB_Delete_Multi_Address(pAd)
Ptr_Adapter_Struc pAd;
{
	/*
	 * This is not used.  This LM driver relies on the use of
	 * Change_Receive_Mask() only.
	 */
	return(SUCCESS);
}
#endif /* Solaris */

#ifndef LM_STREAMS
LMB_Receive_Copy(Nbytes,Offset,pDB,pAd,CopyStat)
ushort Nbytes;
unsigned short Offset;
#ifdef CODE_386
Data_Buff_Structure *pDB;
#else
Data_Buff_Structure far *pDB;
#endif
Ptr_Adapter_Struc pAd;
unsigned short CopyStat;
{
	caddr_t cp, dp;
	int length, inval;
	unsigned short Page_Offset, Ram_Offset, Move_Len;


	cp = (caddr_t) (pAd->CurRcvbuf + Offset);

	dp = (caddr_t) pDB->fragment_list[0].fragment_ptr;
        length = pDB->fragment_list[0].fragment_length;

	Ram_Offset = (pAd->Page_Offset + 4) & pAd->page_offset_mask;
        Move_Len =0;

#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
	if((pAd->adapter_bus==BUS_EISA32M_TYPE) && (length >= DMA_CUT_OFF)){
		if(!pAd->Rcv_DMA_Pending && !pAd->Xmit_DMA_Pending) {
			pAd->Rcv_Buffer_Pending = (caddr_t)pDB;
			if(Page_Offset + length >= 0x8000) {
				Move_Len = 0x8000 - Page_Offset;
				EBM_Xfer(pAd,dp,Page_Offset,Move_Len,EISA_SZ_DIR);
				length -= Move_Len;
				dp +=Move_Len;    /* host ram */
				Page_Offset = 
				(USHORT)(pAd->rcv_start - pAd->ram_access);
			}
			while(1){
				inval = inb((ushort)(pAd->io_base + EISA_STAT));
				if(!(inval & 0x02))
					break;
			}
			EBM_Xfer(pAd,dp,Page_Offset,length,EISA_SZ_DIR);
		}
	} else {
#endif /* -REALMODE && -Solaris */
		if((Ram_Offset+length) >= 0x2000) {
			Move_Len = 0x2000 - Ram_Offset;
			SyncSetWindowPage(pAd,pAd->Page_Num);
			sebmbcopy( cp, dp, Move_Len, SRC_ALIGN);
			length -= Move_Len;
			dp +=Move_Len;       /* host ram */
			pAd->Page_Num += 1;
			if(pAd->Page_Num == 4) {
				pAd->Page_Num =0;
				cp = pAd->rcv_start;
			} else {
				cp =(caddr_t)pAd->ram_access;
			}
			SyncSetWindowPage(pAd,pAd->Page_Num);
		}
		if(length) {
			sebmbcopy( cp, dp, length, SRC_ALIGN);
		}
#if (!defined(REALMODE) && ((UNIX & 0xff00) != 0x400))
	}
#endif
	return(SUCCESS);
}
#endif -LM_STREAMS
