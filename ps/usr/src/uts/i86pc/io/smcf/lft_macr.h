/*
 * This is an SMC LMAC driver.  It will periodically be updated with
 * new versions from SMC.  It is important that minimal changes be
 * made to this file, to facilitate maintenance and running diffs with
 * new versions as they arrive.  DO NOT cstyle or lint this file, or
 * make any other unnecessary changes.
 *
 * This file should be identical in the PS and Realmode source trees.
 */

#ident	"@(#)lft_macr.h 1.2	95/07/18 SMI"

/*++
Copyright (c) 1995  Standard Microsystems Corporation

Module Name:

   lft_macr

Abstract:

      Contains universal macros for LMAC

Author:

   SMC


Revision History:

$Log:   F:\sweng\src\lm9232\c_lmac\vcs\lft_macr.hv  $
 * 
 * Initial revision: Create it for Fast Ethernet C LMAC
 *


;+!/?


--*/

#ifndef STATIC
#ifndef UNIX
#define STATIC static 
#else
#define STATIC
#endif
#endif

#ifdef UNIX
#if ((UNIX & 0xff00) != 0x400)	/* Not for Solaris 2.x */
/* solving symbol name conflict */
#define LM_Add_Multi_Address sm92_LM_Add_Multi_Address
#define LM_Change_Receive_Mask sm92_LM_Change_Receive_Mask
#define LM_Close_Adapter sm92_LM_Close_Adapter
#define LM_Delete_Multi_Address sm92_Delete_Multi_Address
#define LM_Disable_Adapter sm92_LM_Disable_Adapter
#define LM_Enable_Adapter sm92_LM_Enable_Adapter
#define LM_GetCnfg sm92_LM_GetCnfg
#define LM_Initialize_Adapter sm92_LM_Initialize_Adapter
#define LM_Interrupt_Req sm92_LM_Interrupt_Req
#define LM_Open_Adapter sm92_LM_Open_Adapter
#define LM_Receive_Copy sm92_LM_Receive_Copy
#define LM_Send sm92_LM_Send
#define LM_Service_Events sm92_LM_Service_Events
#define UM_Interrupt sm92_UM_Interrupt
#define UM_Receive_Copy_Complete sm92_UM_Receive_Copy_Complete
#define UM_Receive_Packet sm92_UM_Receive_Packet
#define UM_Send_Complete sm92_UM_Send_Complete
#define UM_Status_Change sm92_UM_Status_Change
#define GetLANAddress sm92_GetLANAddress
#endif	/* -Solaris */
#endif

/* extern function prototype */
#ifdef UNIX
#if ((UNIX & 0xff00) != 0x400)	/* Not for Solaris 2.x */
/* I/O */
extern inb();
extern inw();
extern inl();
extern outb();
extern outw();
extern outl();
/* Stream I/O */
extern repinsb();
extern repinsw();
extern repinsd();
extern repoutsb();
extern repoutsw();
extern repoutsd();
/* Memory Move */
extern bcopy();
extern bzero();
#endif /* -Solaris */
/* Wait */
#if ((UNIX & 0xff00) == 0x0300)
extern drv_usecwait();
#endif
#endif

#ifdef NDIS3X
/* I/O */
extern NdisRawReadPortUchar();
extern NdisRawReadPortUshort();
extern NdisRawReadPortUlong();
extern NdisRawWritePortUchar();
extern NdisRawWritePortUshort();
extern NdisRawWritePortUlong();
/* Stream I/O */
extern NdisReadPortBufferUchar();
extern NdisReadPortBufferUshort();
extern NdisReadPortBufferUlong();
extern NdisWritePortBufferUchar();
extern NdisWritePortBufferUshort();
extern NdisWritePortBufferUlong();
/* Memory Move */
extern NdisMoveToMappedMemory();
extern NdisMoveFromMappedMemory();
/* Wait */
extern NdisStallExecution();
#endif

#ifdef GENERIC
extern g_inp();
extern g_inpw();
extern g_inpdw();
extern g_outp();
extern g_outpw();
extern g_outpdw();
extern repinsb();
extern repinsw();
extern repinsdw();
extern repoutb();
extern repoutw();
extern repoutdw();
#endif

#ifdef REALMODE
#define SMC_INP(Port, Data) *(unsigned char *)(Data)=inb((ushort)(Port))
#define SMC_INPW(Port, Data) *(unsigned short *)(Data)=inw((ushort)(Port))
#define SMC_INPD(Port, Data) *(unsigned long *)(Data)=inl((ushort)(Port))

#define SMC_OUTP(Port, Data) outb((ushort)(Port), (unchar)(Data))
#define SMC_OUTPW(Port, Data) outw((ushort)(Port), (ushort)(Data))
#define SMC_OUTPD(Port, Data) outl((ushort)(Port), (ulong)(Data))

#define SMC_repinsw(pAS, ioaddr, memaddr, len) repinsw(ioaddr, (void far *)memaddr, (unsigned int)len) 
#define SMC_repoutsw(pAS, ioaddr, memaddr, len) repoutsw(ioaddr, (void far *)memaddr, (unsigned int)len)

#endif	/* REALMODE */

/* I/O macro */
#ifdef UNIX
#define SMC_INP(Port, Data) *(unsigned char *)(Data)=inb(Port)
#define SMC_INPW(Port, Data) *(unsigned short *)(Data)=inw(Port)
#define SMC_INPD(Port, Data) *(unsigned long *)(Data)=inl(Port)

#define SMC_OUTP(Port, Data) outb(Port, Data)
#define SMC_OUTPW(Port, Data) outw(Port, Data)
#define SMC_OUTPD(Port, Data) outl(Port, Data)
#endif

#ifdef NDIS3X
#define SMC_INP(Port, Data) NdisRawReadPortUchar(Port, Data)
#define SMC_INPW(Port, Data) NdisRawReadPortUshort(Port, Data)
#define SMC_INPD(Port, Data) NdisRawReadPortUlong(Port, Data)

#define SMC_OUTP(Port, Data) NdisRawWritePortUchar(Port, Data)
#define SMC_OUTPW(Port, Data) NdisRawWritePortUshort(Port, Data)
#define SMC_OUTPD(Port, Data) NdisRawWritePortUlong(Port, Data)
#endif

#ifdef GENERIC
#define SMC_INP(Port, Data) *(unsigned char *)(Data)=g_inp(Port)
#define SMC_INPW(Port, Data) *(unsigned char *)(Data)=g_inpw(Port)
#define SMC_INPD(Port, Data) *(unsigned char *)(Data)=g_inpdw(Port)

#define SMC_OUTP(Port, Data) g_outp(Port, Data)
#define SMC_OUTPW(Port, Data) g_outpw(Port, Data)
#define SMC_OUTPD(Port, Data) g_outpdw(Port, Data)
#endif

/* Stream IO */
#ifdef UNIX
#if ((UNIX & 0xff00) != 0x400)  /* Not Solaris 2.x */
#define SMC_repinsb(pAS, ioaddr, memaddr, len) repinsb(ioaddr, memaddr, len) 
#define SMC_repinsw(pAS, ioaddr, memaddr, len) repinsw(ioaddr, memaddr, len) 
#define SMC_repinsd(pAS, ioaddr, memaddr, len) repinsd(ioaddr, memaddr, len) 
#define SMC_repoutsb(pAS, ioaddr, memaddr, len) repoutsb(ioaddr, memaddr, len) 
#define SMC_repoutsw(pAS, ioaddr, memaddr, len) repoutsw(ioaddr, memaddr, len) 
#define SMC_repoutsd(pAS, ioaddr, memaddr, len) repoutsd(ioaddr, memaddr, len) 
#else				/*Solaris 2.x */
#define SMC_repinsb(pAS, ioaddr, memaddr, len) repinsb(ioaddr, memaddr, len)
#define SMC_repinsw(pAS, ioaddr, memaddr, len) repinsw(ioaddr, (unsigned short *)memaddr, len)
#define SMC_repinsd(pAS, ioaddr, memaddr, len) repinsdw(ioaddr, memaddr, len)

#define SMC_repoutsb(pAS, ioaddr, memaddr, len) repoutsb(ioaddr, memaddr, len)
#define SMC_repoutsw(pAS, ioaddr, memaddr, len) repoutsw(ioaddr, (unsigned short *)memaddr, len)
#define SMC_repoutsd(pAS, ioaddr, memaddr, len) repoutsdw(ioaddr, memaddr, len)
#endif				/* Solaris 2.x */
#endif

#ifdef NDIS3X
#define SMC_repinsb(pAS, ioaddr, memaddr, len) \
	NdisReadPortBufferUchar((void *)pAS->NdisAdapterHandle, \
		(unsigned long) ioaddr, (char *) memaddr, (unsigned long) len)
#define SMC_repinsw(pAS, ioaddr, memaddr, len) \
	NdisReadPortBufferUshort((void *)pAS->NdisAdapterHandle, \
		(unsigned long) ioaddr, (char *) memaddr, (unsigned long) len)
#define SMC_repinsd(pAS, ioaddr, memaddr, len) \
	NdisReadPortBufferUlong((void *)pAS->NdisAdapterHandle, \
		(unsigned long) ioaddr, (char *) memaddr, (unsigned long) len)
#define SMC_repoutsb(pAS, ioaddr, memaddr, len) \
	NdisWritePortBufferUchar((void *)pAS->NdisAdapterHandle, \
		(unsigned long) ioaddr, (char *) memaddr, (unsigned long) len)
#define SMC_repoutsw(pAS, ioaddr, memaddr, len) \
	NdisWritePortBufferUshort((void *)pAS->NdisAdapterHandle, \
		(unsigned long) ioaddr, (char *) memaddr, (unsigned long) len)
#define SMC_repoutsd(pAS, ioaddr, memaddr, len) \
	NdisWritePortBufferUlong((void *)pAS->NdisAdapterHandle, \
		(unsigned long) ioaddr, (char *) memaddr, (unsigned long) len)
#endif

#ifdef GENERIC
#define SMC_repinsb(pAS, ioaddr, memaddr, len) repinsb(ioaddr, memaddr, len)
#define SMC_repinsw(pAS, ioaddr, memaddr, len) repinsw(ioaddr, memaddr, len)
#define SMC_repinsd(pAS, ioaddr, memaddr, len) repinsdw(ioaddr, memaddr, len)

#define SMC_repoutsb(pAS, ioaddr, memaddr, len) repoutsb(ioaddr, memaddr, len)
#define SMC_repoutsw(pAS, ioaddr, memaddr, len) repoutsw(ioaddr, memaddr, len)
#define SMC_repoutsd(pAS, ioaddr, memaddr, len) repoutsdw(ioaddr, memaddr, len)
#endif

/* Memory Move */
#ifdef UNIX
#define SMC_copy(dst, src, size) bcopy((char *) src, (char *) dst, (long) size)
#define SMC_copy_up(dst, src, size) bcopy((char *) src, (char *) dst, (long) size)
#define SMC_zero(dst, size) bzero((char *) dst, (int) size)
#endif

#ifdef NDIS3X
#define SMC_copy(dst, src, size) NdisMoveToMappedMemory(dst,src,size)
#define SMC_copy_up(dst, src, size) NdisMoveFromMappedMemory(dst,src,size)
#endif

/* Wait */
#ifdef UNIX
#if ((UNIX&0xff00)==0x0300)
#define SMC_Wait(nms) drv_usecwait((long) 1000*nms)
#define SMC_uWait(nus) drv_usecwait((long) nus)
#else
#define SMC_Wait(nms) {\
		int i; \
		extern microdata; \
		for(i=0; i<1000*nms*microdata; i++) ; \
	}
#define SMC_uWait(nus) {\
		int i; \
		extern microdata; \
		for(i=0; i<nus*microdata; i++) ; \
	}
#endif
#endif

#ifdef NDIS3X
#define SMC_Wait(nms) NdisStallExecution(1000*nms)
#define SMC_uWait(nus) NdisStallExecution(nus)
#endif
