/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)eth.c	1.4	93/12/01 SMI"

void PE2_Media_Disable_Int();
void PE2_Media_Enable_Int();
void PE2_Media_Reset();
static void Service_Xmt_Error();
static void Service_Rcv_Packet();
static void Service_Rcv_Error();
static void Service_Buffer_Full();
static void Service_Xmt_Complete();

/* eth.inc --- Media Control Module for the 8390 (c) 1992 Xircom */

/****************************************************************************************************************; */
/* Revision Information												; */
/*														; */
/*	Author: 	Eric Henderson										; */
/*	Started:	Jan 1992										; */
/*	Code From:	Pocket Ethernet Novell Shell Driver (Dirk Gates, 6/01/89)				; */
/*														; */
/*	Language:	MASM 5.1										; */
/*	Build:		this file is included by the adapter control module					; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Release 	Date	Who		Why									; */
/* -------	----	---		--------------------------------------------------------		; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Notes:													; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
/* History:													; */
/*														; */
/* Date	 Who    What												; */
/* ----	 ---	-------------------------------------------------------------------------------------------	; */
/*---------------------------------------------------------------------------------------------------------------; */

#include "8390nicdef.h"

/****************************************************************************************************************; */
/* Constant Definitions												; */
/*---------------------------------------------------------------------------------------------------------------; */
#define M_FALSE			0
#define M_TRUE			1
#define MAX_DATA_PACKET		1514				/* maximum data packet */
#define POLYL			0x1db6				/* used in the Multi Cast routine */
#define POLYH			0x04c1
#define ADDR_SIZE		6				/* used in the multicast routines */
#define INTERRUPT_MASK		PRX+PTX+RXE+TXE+OVW+CNT

/*// Packet Memory */
#define XMT_BUFFS_8K			2
#define XMT_BUFFS_32K			2
#define RCV_BEG_PAGE_32K		XMT_BEG_PAGE + 6*XMT_BUFFS_32K

#define MEM_BEG_PAGE			0x20					/* starting memory page */
#define MEM_PAGES			32768/256				/* memory size in pages */
#define XMT_BEG_PAGE			MEM_BEG_PAGE				/* start of transmit buffer area */

#define NUM_NIC_ENTRIES		25
#define NIC_TABLE_RXCR		3
#define NIC_TABLE_BNDRY		5
#define NIC_TABLE_PSTART	6
#define NIC_TABLE_PSTOP		7
#define NIC_TABLE_PAR0		10
#define NIC_TABLE_MAR0		16
#define NIC_TABLE_CURR		24


/****************************************************************************************************************; */
/* Structure Definitions												; */
/*---------------------------------------------------------------------------------------------------------------; */
/*// Receive Buffer Header Structure */
#ifdef notdef
struct Receive_Buffer_Header {
unsigned char RCV_NIC_Status;
unsigned char RCV_NIC_Next_Packet;
unsigned short  RCV_NIC_Byte_Count;
};

#endif
/*// Fragment Descriptor Structure */
struct Fragment_Descriptor {
char far *Fragment_Pointer;
int	  Fragment_Length;
};

/*// 8390 Initialization Table Structure */
struct NIC_Table_Entry {
unsigned char	NIC_Register_Number;
unsigned char	NIC_Register_Value;
unsigned char	NIC_Register_Mask;
unsigned char	NIC_Read_Page;
unsigned char	NIC_Write_Page;
};

/*******************************************************************************; */
/* Resident Data								; */
/*------------------------------------------------------------------------------; */
#ifdef notdef
int	(*User_ISR)();							/* address of user interrupt service routine */
int	Media_Status		= 0,
	MCP_Leftovers		= 0,
	MCP_Fragment_Count	= 0,
	MCP_Fragment_Index	= 0,
	MS_Reenable_Ints	= 0,
	Tx_Page			= XMT_BEG_PAGE,
	Tx_Length		= 0,
	Interrupt_Status	= 0,
	SBF_Resend		= 0,
	Rx_Leftovers		= M_FALSE;

int     MEM_END_PAGE		=	MEM_BEG_PAGE + 32768/256;		/* ending memory page */
int	XMT_BUFFERS		=	XMT_BUFFS_32K,
	RCV_BEG_PAGE		=	RCV_BEG_PAGE_32K,			/* start of receive buffer area */
	RCV_PAGES		=	32768/256 - 6*XMT_BUFFS_32K,		/* number of pages for receive buffer area */
	RCV_END_PAGE		=	MEM_BEG_PAGE + 32768/256;		/* end of receive buffer area */

/*// Miscellaneous Storage */
int	Next_Page		=	RCV_BEG_PAGE_32K + 1,			/* next page pointer */
	Boundary		=	RCV_BEG_PAGE_32K;			/* used for diagnostic purposes */

/*// NIC Configuration Table */
#endif

struct NIC_Table_Entry PE2_NIC_Configuration_Table[NUM_NIC_ENTRIES] = {
{DCR,	FT_4+NO_LB,		DCR_MASK,	DCR_RD_PG,	DCR_WR_PG   },
{RBCR0,	0,			RBCR0_MASK,	0xff,		RBCR0_WR_PG },
{RBCR1,	0,			RBCR1_MASK,	0xff,		RBCR1_WR_PG },
{RXCR,	0,			RXCR_MASK,	RXCR_RD_PG,	RXCR_WR_PG  },
{TXCR,	LB_INT,			TXCR_MASK,	TXCR_RD_PG,	TXCR_WR_PG  },
{BNDRY,	RCV_BEG_PAGE_32K,	BNDRY_MASK,	BNDRY_RD_PG,	BNDRY_WR_PG },
{PSTART,RCV_BEG_PAGE_32K,	PSTART_MASK,	PSTART_RD_PG,	PSTART_WR_PG},
{PSTOP,	0x20 + 32768/256,	PSTOP_MASK,	PSTOP_RD_PG,	PSTOP_WR_PG },
{ISR,	0xff,			ISR_MASK,	0xff,		ISR_WR_PG   },
{IMR,	0,			IMR_MASK,	IMR_RD_PG,	IMR_WR_PG   },
{PAR0,	0xff,			PAR0_MASK,	PAR0_RD_PG,	PAR0_WR_PG  },
{PAR1,	0x80,			PAR1_MASK,	PAR1_RD_PG,	PAR1_WR_PG  },
{PAR2,	0xc7,			PAR2_MASK,	PAR2_RD_PG,	PAR2_WR_PG  },
{PAR3,	0x00,			PAR3_MASK,	PAR3_RD_PG,	PAR3_WR_PG  },
{PAR4,	0x00,			PAR4_MASK,	PAR4_RD_PG,	PAR4_WR_PG  },
{PAR5,	0x00,			PAR5_MASK,	PAR5_RD_PG,	PAR5_WR_PG  },
{MAR0,	0,			MAR0_MASK,	MAR0_RD_PG,	MAR0_WR_PG  },
{MAR1,	0,			MAR1_MASK,	MAR1_RD_PG,	MAR1_WR_PG  },
{MAR2,	0,			MAR2_MASK,	MAR2_RD_PG,	MAR2_WR_PG  },
{MAR3,	0,			MAR3_MASK,	MAR3_RD_PG,	MAR3_WR_PG  },
{MAR4,	0,			MAR4_MASK,	MAR4_RD_PG,	MAR4_WR_PG  },
{MAR5,	0,			MAR5_MASK,	MAR5_RD_PG,	MAR5_WR_PG  },
{MAR6,	0,			MAR6_MASK,	MAR6_RD_PG,	MAR6_WR_PG  },
{MAR7,	0,			MAR7_MASK,	MAR7_RD_PG,	MAR7_WR_PG  },
{CURR, 	RCV_BEG_PAGE_32K + 1,	CURR_MASK,	CURR_RD_PG,	CURR_WR_PG  }
};

#ifdef notdef
struct Receive_Buffer_Header Receive_Status;
#endif

/* int Media_Send(h, Tmt_Length, Copy_Count, Frag_Count, Frag_List)          */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* 	int Tmt_Length = the number of bytes to Transmit                     */
/* 		int Copy_Count = the number of bytes to copy (not including  */
/* 		the Send_Header)                                             */
/* 	int Frag_Count = the number of fragments in the fragment list        */
/*                                                                           */
/* 	struct Fragment_Descriptor far *Frag_List = points to a              */
/* 	fragment list if Frag_Count != 0                                     */
/*                                                                           */
/* 	struct Fragment_Descriptor far *Frag_List = points to a              */
/* 	contiguous data buffer if Frag_Count == 0                            */
/*                                                                           */
/* 	Send_Header contains a pointer to a buffer that has been filled      */
/* 	in with the correct network header (if applicable)                   */
/*                                                                           */
/* 	Send_Header_Size reflects the number of bytes in                     */
/* 	Send_Header (0  (zero) is allowed)                                   */
/*                                                                           */
/* 	Returns:	0 if send was completed successfully                 */
/* 			non zero if an error occurred                        */


PE2_Media_Send(h, Tmt_Length, Copy_Count, Frag_Count, Frag_List)
int	Tmt_Length, Copy_Count, Frag_Count;
struct	Fragment_Descriptor far *Frag_List;
{
	int	Reenable = M_FALSE, Block_Count, Value;
	struct	Fragment_Descriptor far *Curr_Frag;

	if ( (xcb[h].Hardware_Status & HARDWARE_UNAVAILABLE) ||
		(xcb[h].Media_Status & MEDIA_IN_SEND) ) return (XM_UNAVAILABLE);
	if (Check_Driver_Shutdown(h)) return (XM_DRIVER_SHUTDOWN);
	xcb[h].Media_Status |= MEDIA_IN_SEND;
	if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) ) {
		PE2_Media_Disable_Int(h);
		Reenable = M_TRUE;
	}
/* sti */
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
	PUT_REGISTER(h, ISR, RDC);

#ifdef	PRQ_FIX
	PUT_REGISTER(h, RBCR0, 0xf				/* bug fix for 8390) */
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_RD		/* (forces PRQ high)) */
#endif
	if (Copy_Count + xcb[h].Send_Header_Size) {
		PUT_REGISTER(h, RBCR0,  (Copy_Count + xcb[h].Send_Header_Size) & 0xff);
		PUT_REGISTER(h, RBCR1, ((Copy_Count + xcb[h].Send_Header_Size) >> 8) & 0xff);
		PUT_REGISTER(h, RSAR0, 0);				/* write starting address for */
		PUT_REGISTER(h, RSAR1, xcb[h].Tx_Page);			/* remote dma */
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_WR);
		if (xcb[h].Send_Header_Size)
			BLOCK_WRITE(h, (char far *) xcb[h].Send_Header, xcb[h].Send_Header_Size);
		if (Frag_Count) {
			for (Curr_Frag = Frag_List; Frag_Count && Copy_Count; Frag_Count--, Curr_Frag++) {
				if (!Curr_Frag->Fragment_Length) continue;
				if (Curr_Frag->Fragment_Length > Copy_Count)
					Block_Count = Copy_Count;
				else	Block_Count = Curr_Frag->Fragment_Length;
				BLOCK_WRITE(h, (char far *) Curr_Frag->Fragment_Pointer, Block_Count);
				Copy_Count -= Block_Count;
			}
		} else {					/* Contiguous copy */
			if (Copy_Count)
				BLOCK_WRITE(h, (char far *)Frag_List, Copy_Count);
		}
	}
	for (Block_Count = 0; Block_Count < MAX_DATA_PACKET; Block_Count++)
		if (GET_REGISTER(h, ISR) & RDC) break;
	if (Block_Count >= MAX_DATA_PACKET) Value = XM_DATA_ERR;
	else {
		for (Block_Count = 0; Block_Count < MAX_DATA_PACKET; Block_Count++)
			if (!(GET_REGISTER(h, CMD) & TXP)) break;
	}
	if (Block_Count >= MAX_DATA_PACKET) Value = XM_NO_CABLE;
	else {
		PUT_REGISTER(h, TPSR, xcb[h].Tx_Page);
		PUT_REGISTER(h, TBCR0, Tmt_Length & 0xff);		/* write transmit length) */
		PUT_REGISTER(h, TBCR1, ((Tmt_Length >> 8) & 0xff));
		xcb[h].Tx_Page = (xcb[h].Tx_Page == XMT_BEG_PAGE ? XMT_BEG_PAGE+6 : XMT_BEG_PAGE);
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT+TXP);	/* initiate transmission */
		xcb[h].Tx_Length = Tmt_Length;
		Value = 0;
	}
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
/* cli */
	xcb[h].Media_Status &= ~MEDIA_IN_SEND;
	if (Reenable) PE2_Media_Enable_Int(h);
	return (Value);
}

/* int Media_Poll(h)                                                         */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	0 if Media_Poll was able to poll the adapter                 */
/* 		-1 if Media_Poll was unable to poll the adapter              */
/*                                                                           */

PE2_Media_Poll(h)
{
/* cli						// I don't trust them */
	if (xcb[h].Hardware_Status & HARDWARE_UNAVAILABLE) return (-1);
	if (xcb[h].Media_Status & (MEDIA_IN_ISR+INTERRUPTS_DISABLED+MEDIA_IN_SEND) )
		return (-1);
	if (GET_REGISTER(h, ISR) & (PRX+PTX+RXE+TXE+OVW+CNT) )
		PE2_Media_ISR(h);
	return (0);
}

/****************************************************************************************************************; */
/* 8390 Event Service Routines											; */
/*---------------------------------------------------------------------------------------------------------------; */

/* PE2_Media_ISR(h)                                                          */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	Services Media interrupts */

PE2_Media_ISR(h)
{
	int	Interrupt_Status;

	if (Check_Driver_Shutdown(h) || (xcb[h].Media_Status & MEDIA_IN_ISR)) return (-1);
	xcb[h].Media_Status |= MEDIA_IN_ISR;
	PE2_Media_Disable_Int(h);
	while (M_TRUE) {
		while ( (Interrupt_Status = GET_REGISTER(h, ISR)) & (PRX+PTX+RXE+TXE+OVW+CNT)) {		/* get the status bits */
/*sti */
			if (Interrupt_Status & OVW)	Service_Buffer_Full(h);
			if (Interrupt_Status & RXE+CNT) Service_Rcv_Error(h);
			if (Interrupt_Status & PRX)	Service_Rcv_Packet(h);
			if (Interrupt_Status & TXE)	Service_Xmt_Error(h);
			if (Interrupt_Status & PTX)	Service_Xmt_Complete(h);
		}
/*cli */
		PE2_Media_Enable_Int(h);
		xcb[h].Media_Status &= ~MEDIA_IN_ISR;
#ifdef NEEDSWORK
		if (*User_ISR) (*User_ISR)(h);
#endif
/*cli */
		if (xcb[h].Rx_Leftovers != M_FALSE) {
			xcb[h].Media_Status |= MEDIA_IN_ISR;
			PE2_Media_Disable_Int(h);
			Service_Rcv_Packet(h);
		} else break;
	}
	return 0;
}

/* Service Transmit Complete
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	Description: called by DriverISR to service a completed transmission */

static void Service_Xmt_Complete(h)
{
/* cli */
	PUT_REGISTER(h, ISR, PTX);
	Link_Transmit_Complete(h, xcb[h].Tx_Length, xcb[h].Last_Tx_Status,
		xcb[h].XCB_Link_Pointer);
}

/* Service Transmit Error */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	called by DriverISR to service a transmission error condition. */

static void Service_Xmt_Error(h)
{
/* cli */
	PUT_REGISTER(h, ISR, TXE);				/* clear the status */
	Link_Transmit_Complete(h, GET_REGISTER(h, TSR), 0);
}

/* Service Received Packet */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	called by DriverISR to service a received packet. */

static void Service_Rcv_Packet(h)
{
	int	 	Current;
	unsigned short	Value;
/* cli */
	while (1) {
		PUT_REGISTER(h, CMD, PAGE_1+STA+RDC_ABT);			/* check for another packet */
		Current = GET_REGISTER(h, CURR);
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
		if (Current == xcb[h].Next_Page) break;
		PUT_REGISTER(h, RBCR0, sizeof(struct Receive_Buffer_Header));		/* read header */
		PUT_REGISTER(h, RBCR1, 0);
		PUT_REGISTER(h, RSAR0, 0);
		PUT_REGISTER(h, RSAR1, xcb[h].Next_Page);
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_RD);
		BLOCK_READ(h, (char far *)&xcb[h].Receive_Status, sizeof(struct Receive_Buffer_Header));
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);;
		if (xcb[h].Receive_Status.RCV_NIC_Status & DIS+FFO+FAE+CRC) {
			NIC_Initialize(h);				/* reset all the buffers and stuff */
			PUT_REGISTER(h, TXCR, 0);				/* take NIC out of loopback */
			PUT_REGISTER(h, IMR, INTERRUPT_MASK);
		} else {
			Value = (xcb[h].Receive_Status.RCV_NIC_Byte_Count + (4-1+0x100)) / 256 + xcb[h].Next_Page;
			if (Value >= xcb[h].RCV_END_PAGE) Current -= xcb[h].RCV_PAGES;
			if  ( Value != xcb[h].Receive_Status.RCV_NIC_Next_Packet) {	/* check more paging errors */
				NIC_Initialize(h);				/* reset all the buffers and stuff */
				PUT_REGISTER(h, TXCR, 0);				/* take NIC out of loopback */
				PUT_REGISTER(h, IMR, INTERRUPT_MASK);
			} else {
				if (xcb[h].Receive_Header_Size) {
					Value = xcb[h].Receive_Status.RCV_NIC_Byte_Count - 4;
					if (xcb[h].Receive_Header_Size < Value)
						Value = xcb[h].Receive_Header_Size;
					PUT_REGISTER(h, RBCR0, Value & 0xff);	/* read header */
					PUT_REGISTER(h, RBCR1, (Value >> 8) & 0xff);
					PUT_REGISTER(h, RSAR0, sizeof(struct Receive_Buffer_Header));
					PUT_REGISTER(h, RSAR1, xcb[h].Next_Page);
					PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_RD);
					BLOCK_READ(h, (char far *) xcb[h].Receive_Header, (int) Value);
					PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
					Value = Link_Receive_Packet(h, (xcb[h].Receive_Status.RCV_NIC_Byte_Count - 4), xcb[h].Receive_Status.RCV_NIC_Status, xcb[h].XCB_Link_Pointer);
				}
				xcb[h].Next_Page = xcb[h].Receive_Status.RCV_NIC_Next_Packet;
				xcb[h].Boundary  = (xcb[h].Next_Page - 1 >= xcb[h].RCV_BEG_PAGE ? xcb[h].Next_Page - 1 : xcb[h].RCV_END_PAGE - 1);
				PUT_REGISTER(h, BNDRY, xcb[h].Boundary);		/* set the boundary pointer */
				if (Value == 0) {			/* If Link sets CX=0, they want no more */
					xcb[h].Rx_Leftovers = M_TRUE;
					break;
				}
				if (GET_REGISTER(h, ISR) & OVW) break;	/* Make sure we're not in a bad loop */
			}
		}
	}
	PUT_REGISTER(h, ISR, PRX);						/* reset the interrupt bit */
#ifdef	EXTERNAL
	No_Event_Counter = 0;
	xcb[h].Media_Select &= ~MEDFLT_LED;
#endif
}

/* int Media_Copy_Packet(h, Offset, Count, Frag_Count, Frag_List)            */
/*                                                                           */
/* Passed:	                                                             */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* 	int Offset = the desired offset into packet                          */
/*                                                                           */
/* 	int Count = the desired amount of data                               */
/*                                                                           */
/* 	int Frag_Count = the number of fragments in fragment list            */
/*                                                                           */
/* 	struct Fragment_Descriptor far *Frag_List = points to a              */
/*                                                                           */
/* 	fragment list if Frag_Count != 0                                     */
/*                                                                           */
/* 	struct Fragment_Descriptor far *Frag_List = points to a              */
/* 	contiguous data buffer if Frag_Count == 0                            */
/*                                                                           */
/* Returns:	NIC reported packet length in bytes                          */
/*                                                                           */


PE2_Media_Copy_Packet(h, Offset, Count, Frag_Count, Frag_List)
int	Offset, Count, Frag_Count;
struct	Fragment_Descriptor far *Frag_List;
{
/* cli */
	int	  Leftovers = 0, Buffer_Size; 
	unsigned short Temp; /*KBD*/
	char far *p, far *q = (char far *) Frag_List->Fragment_Pointer;

	if (Frag_Count)
		if ( (Buffer_Size = SizeOfTDBuffer(h, Frag_Count, Frag_List)) < Count)
			Count = Buffer_Size;
	Buffer_Size = xcb[h].Receive_Status.RCV_NIC_Byte_Count - sizeof(struct Receive_Buffer_Header);
	if ( (Count + Offset) > Buffer_Size)
		Count = Buffer_Size - Offset;

	if (Offset < xcb[h].Receive_Header_Size) {
		Buffer_Size = xcb[h].Receive_Header_Size - Offset;		/* Number of bytes to read from xcb[h].Receive_Header */
		if (Buffer_Size > Count) Buffer_Size = Count;
		Count -= Buffer_Size;
		p = (char far *) (xcb[h].Receive_Header+Offset);
		q = (char far *) Frag_List->Fragment_Pointer;
		if (Frag_Count == 0) {
			for ( ; Buffer_Size; Buffer_Size--, p++, q++)
				*q = *p;
		} else {
			while (Buffer_Size) {
				if ( (Temp = Frag_List->Fragment_Length) !=0 ) {
					if (Temp > Buffer_Size) {
						Leftovers = Temp - Buffer_Size;
						Temp = Buffer_Size;
					}
					q = (char far *) Frag_List->Fragment_Pointer;
					Buffer_Size -= Temp;
					for ( ; Temp; Temp--, p++, q++)
						*q = *p;
				}
				if (Leftovers == 0) Frag_List++;
			}
		}
	}
	if (Count) {
		Temp = (xcb[h].Receive_Header_Size > Offset ? xcb[h].Receive_Header_Size : Offset);
		Temp += (xcb[h].Next_Page << 8) + sizeof(struct Receive_Buffer_Header); /*KBD*/
		if (Temp >= (xcb[h].RCV_END_PAGE<<8)) Temp -= (xcb[h].RCV_PAGES<<8);/*KBD*/
		PUT_REGISTER(h, RSAR0, Temp & 0xff);
		PUT_REGISTER(h, RSAR1, (Temp >> 8) & 0xff);

		PUT_REGISTER(h, RBCR0, Count & 0xff);
		PUT_REGISTER(h, RBCR1, (Count >> 8) & 0xff);
		PUT_REGISTER(h, ISR, RDC);
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_RD);
		if (Frag_Count) {
			if (Leftovers) {
				if (Leftovers > Count) Leftovers = Count;
				Count -= Leftovers;
				BLOCK_READ(h, q, Leftovers);
				Frag_List++;
			}
			while (Count) {
				if ( (Temp = Frag_List->Fragment_Length) !=0 ) {
					if (Temp > Count) Temp = Count;
					Count -= Temp;
					BLOCK_READ(h, Frag_List->Fragment_Pointer, Temp);
				}
				Frag_List++;
			}
		} else BLOCK_READ(h, q, Count);
	}
	for (Temp = 0; Temp < MAX_DATA_PACKET; Temp++)
		if (GET_REGISTER(h, ISR) & RDC) break;
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
	if (Temp >= MAX_DATA_PACKET) return (XM_DATA_ERR);
	else return(0);
}

static SizeOfTDBuffer(h, Frag_Count, Frag_List)
int	Frag_Count;
struct	Fragment_Descriptor far *Frag_List;
{
	int	Size = 0;

	while (Frag_Count) {
		Size += Frag_List->Fragment_Length;
		Frag_List++;
		Frag_Count--;
	}
	return (Size);
}

/* Service Receive Error */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	called by DriverISR to service a receive error. */

static void Service_Rcv_Error(h)
{
	int	FAE_Errs, Missed, CRC_Errs;
/* cli */
	PUT_REGISTER(h, ISR, RXE+CNT);
	FAE_Errs = GET_REGISTER(h, CNTR0);
	Missed = GET_REGISTER(h, CNTR1);
	CRC_Errs = GET_REGISTER(h, CNTR2);
	Link_Receive_Error(h, FAE_Errs, Missed, CRC_Errs, xcb[h].XCB_Link_Pointer);
}


/* Service Buffer Full */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	called by DriverISR to service buffer full. */

static void Service_Buffer_Full(h)
{
	int	Resend;

/* cli */
	Resend = GET_REGISTER(h, CMD);
	if (NIC_Stop(h) == 0) {					/* stop the card! */
		PUT_REGISTER(h, RBCR0, 0);
		PUT_REGISTER(h, RBCR1, 0);
		if ( !(GET_REGISTER(h, ISR) & PTX+TXE) ) Resend = 0;
		PUT_REGISTER(h, TXCR, LB_INT);			/* restart the NIC in LoopBack */
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);		/* it will be reset upon exiting this routine */
		PUT_REGISTER(h, ISR, OVW);				/* clear the buffer overflow bit */
		Service_Rcv_Packet(h);				/* go empty the receive ring */
		PUT_REGISTER(h, TXCR, 0);				/* take NIC out of loopback */
		PUT_REGISTER(h, IMR, INTERRUPT_MASK);
		if (Resend & TXP)
			PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT+TXP);	/* initiate transmission */
	} else {
		NIC_Initialize(h);
		PUT_REGISTER(h, TXCR, 0);					/* take NIC out of loopback) */
		PUT_REGISTER(h, IMR, INTERRUPT_MASK);
	}
}

/* Check Driver Shut Down */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	checks if the NIC is still functioning.  */
/*      If not, unhooks the hardware and calls Link_Driver_Shutdown */

static Check_Driver_Shutdown(h)
{
	xcb[h].Media_Status |= DRIVER_ENABLED;					/* innocent until proven guilty */
/* cli */
	if ( (GET_REGISTER(h, CMD) & ~(TXP+RDC_MASK)) != PAGE_0+STA) {	/* make sure the command register contains what we wrote to it */
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);			/* write it first and then */
		if ( (GET_REGISTER(h, CMD) & ~(TXP+RDC_MASK)) != PAGE_0+STA) {
			PE2_Media_Reset(h);
			PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
			if ( (GET_REGISTER(h, CMD) & ~(TXP+RDC_MASK)) != PAGE_0+STA)
				xcb[h].Media_Status &= ~DRIVER_ENABLED;
		}
	}
	return ( !(xcb[h].Media_Status & DRIVER_ENABLED) );
}

/* Initialize and Test Network Interface Controller (8390) */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	Returns:	number of mismatches during readback (0 == success) */
/*                                                                           */
/*	writes the NIC configuration table to the NIC and then reads it back */
/*	returning the number of mismatches in cx.  if the initialization fails*/
/*	during run-time (not during installation),Driver_Shutdown is called to*/
/*	terminate the shell.  						*/
/*	NOTE: the NIC is in loopback mode after being configured.	*/

static NIC_Initialize(h)
{
	struct NIC_Table_Entry *p = PE2_NIC_Configuration_Table;
	int	i,
		Current_Page = PAGE_0,
		Errors = 0;

	Adapter_Reset(h);				/* reset 8390 */
	if (NIC_Stop(h) != 0) return (-1);
	for (i = 0; i < NUM_NIC_ENTRIES; i++, p++) {
		if (p->NIC_Write_Page != Current_Page) {
			Current_Page = p->NIC_Write_Page;
			PUT_REGISTER(h, CMD, Current_Page | (STP+RDC_ABT));	/* write value to register */
		}
		PUT_REGISTER(h, p->NIC_Register_Number, p->NIC_Register_Value);
	}
	xcb[h].Next_Page = xcb[h].RCV_BEG_PAGE+1;
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);			/* loop through configuration */
	Current_Page = PAGE_0;					/* table, reading registers */
	for (i = 0, p = PE2_NIC_Configuration_Table; i < NUM_NIC_ENTRIES; i++, p++) {
		if (p->NIC_Read_Page == 0xff) continue;
		if (p->NIC_Read_Page != Current_Page) {
			Current_Page = p->NIC_Read_Page;
			PUT_REGISTER(h, CMD, Current_Page | (STA+RDC_ABT));	/* write value to register */
		}
		if ( (GET_REGISTER(h, p->NIC_Register_Number) & p->NIC_Register_Mask)
			!= p->NIC_Register_Value) Errors++;
	}
	if (Errors) Link_Driver_Shutdown(h);
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
	return (Errors);
}

/* void Media_Reset(h)                                                       */
/*                                                                           */
/* Passed:                                                                   */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	Nothing                                                      */
/*                                                                           */
/* Resets the adapter, and leaves the media operational.                     */
/*                                                                           */

void PE2_Media_Reset(h)
{
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		NIC_Initialize(h);
		PUT_REGISTER(h, TXCR, 0);
		PUT_REGISTER(h, IMR, INTERRUPT_MASK);
	}
}

/* void Media_Enable_Int(h)                                                   */
/*                                                                           */
/* Passed:                                                                   */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	Nothing                                                      */
/*                                                                           */
/* Enables the interrupt capability of the adapter.                          */
/*                                                                           */

void PE2_Media_Enable_Int(h)
{
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		xcb[h].Media_Status &= ~INTERRUPTS_DISABLED;
		Hardware_Enable_Int(h);
		xcb[h].Adapter_Pulse_Int(h);
	}
}

/* void Media_Disable_Int(h)                                                  */
/*                                                                           */
/* Passed:                                                                   */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	Nothing                                                      */
/*                                                                           */
/* Disables the interrupt capability of the adapter.                         */
/*                                                                           */

void PE2_Media_Disable_Int(h)
{
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		xcb[h].Media_Status |= INTERRUPTS_DISABLED;
		xcb[h].Adapter_Disable_Int(h);
	}
}

/* void Media_Force_Int(h)                                                    */
/*                                                                           */
/* Passed:                                                                   */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	Nothing                                                      */
/*                                                                           */
/* Causes the adapter to generate an interrupt.                              */
/*                                                                           */

void PE2_Media_Force_Int(h)
{
	xcb[h].Adapter_Force_Int(h);
}

/* Media Pulse Int */
/*                                                                           */
/*	Provided for the HHC Module to create an edge on the interrupt line  */
/*                                                                           */

#ifdef EXTERNAL

Media_Timer_ESR()
{
	Adapter_Timer_ESR()
}

#else

void (*Media_Timer_ESR)() = 0;

#endif

/* Stop Network Interface Controller (8390)
/*                                                                           */
/* Passed:                                                                   */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	Nothing                                                      */
/*                                                                           */
/*	Returns:	number of loops left after RST bit was set (0==failed)*/
/*                                                                           */
/*	halts the NIC and waits for it to acknowledge it has stopped */

static NIC_Stop(h)
{
	int	i;

	PUT_REGISTER(h, CMD, PAGE_0+STP+RDC_ABT);
	for (i = 0; i < MAX_DATA_PACKET; i++)
		if (GET_REGISTER(h, ISR) & RST) break;
	if (i < MAX_DATA_PACKET) return (0);
	else return (-1);
}

/* void Set_Receive_Mode(h,Mode)                                             */
/*                                                                           */
/*                                                                           */
/* Passed:	                                                             */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* 	int Mode = the receive configuration mask                            */
/*                                                                           */
/* 	Returns:	nothing                                              */
/*                                                                           */
/* Configures the media to receive packets as dictated by the mask in Mode.  */
/*                                                                           */

void PE2_Set_Receive_Mode(h, Mode)
int	Mode;
{
	int	 i;
	struct NIC_Table_Entry *p = PE2_NIC_Configuration_Table;

	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CMD, PAGE_1+STA+RDC_ABT);			/* Select Page 1) */
		if (Mode & PRO) {
			Mode |= ABP+AMP;
			for (i = 0; i < 8; i++)
				PUT_REGISTER(h, MAR0+i, -1);
		} else {
			for (i = 0, p = PE2_NIC_Configuration_Table+NIC_TABLE_MAR0; i < 8; i++, p++)
				PUT_REGISTER(h, p->NIC_Register_Number, p->NIC_Register_Value);
		}
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
	}
	PE2_NIC_Configuration_Table[NIC_TABLE_RXCR].NIC_Register_Value = Mode & RXCR_MASK;
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, RXCR, Mode & RXCR_MASK);
		if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) )
			xcb[h].Adapter_Pulse_Int(h);			/* any status port read could */
								/*  clobber a pending INT */
	}
}

/* int Get_Receive_Mode(h)                                                    */
/*                                                                           */
/* Passed:		Nothing                                              */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	the receive configuration mask (see Set_Receive_Mode)        */
/*                                                                           */

PE2_Get_Receive_Mode(h)
{
	int	Mode;

	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CMD, PAGE_2+STA+RDC_ABT);
		Mode = GET_REGISTER(h, RXCR);
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);
		xcb[h].Adapter_Pulse_Int(h);				/* any status port read could */
								/*  clobber a pending INT */
	} else Mode = PE2_NIC_Configuration_Table[NIC_TABLE_RXCR].NIC_Register_Value;
	return (Mode);
}

/* void MCast_Change_Address(h,Set, Address)                                 */
/*                                                                           */
/* Passed                                                                    */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* 	int Set ==  0 if the multicast address is to be disabled             */
/*                                                                           */
/* 	int Set != 0 if the multicast address is to be enabled               */
/*                                                                           */
/* 	char far *Address points to multicast address                        */
/*                                                                           */
/* Returns:	nothing                                                      */
/* Either enables or disables the given multicast address.                   */
/*                                                                           */
/*                                                                           */

void PE2_MCast_Change_Address(h, Set, Address)
int	  Set;
char far *Address;
{
	int	i, j, carry;
	char	Byte;
	unsigned long	crc = 0xffffffff;

	if (xcb[h].Media_Status & MEDIA_INITIALIZED)
		PUT_REGISTER(h, CMD, PAGE_1+STA+RDC_ABT);			/* Select Page 1) */

	for (i = 0; i < 6; i++) {
		Byte = Address[i];
		for (j = 0; j < 8; j++) {
			carry = ((crc & (unsigned long) 0x80000000) ? 1 : 0) ^ (Byte & 0x01);
			crc <<= 1;
			Byte >>= 1;
			if (carry) crc = (crc ^ 0x04c11db6) | carry;
		}
	}
	i = (crc >> 26) & 7;
	j = (crc >> 29);
	if (Set)
		PE2_NIC_Configuration_Table[NIC_TABLE_MAR0+j].NIC_Register_Value |= 1 << i;
	else	PE2_NIC_Configuration_Table[NIC_TABLE_MAR0+j].NIC_Register_Value &= ~(1 << i);

	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, MAR0+j, PE2_NIC_Configuration_Table[NIC_TABLE_MAR0+j].NIC_Register_Value);
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);			/* Select Page 0 */
		if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) )
			xcb[h].Adapter_Pulse_Int(h);				/* any status port read could */
									/*  clobber a pending INT */
	}
}

/* void MCast_Change_All(h,Set)                                              */
/*                                                                           */
/* Passed:	                                                             */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* 	int Set == 0 if all addresses are to be disabled                     */
/* 	int Set != 0 if all addresses are to be enabled                      */
/*                                                                           */
/* Returns:	nothing                                                      */
/*                                                                           */
/* Either enables or disables all multicast addresses.                       */
/*                                                                           */

void PE2_MCast_Change_All(h, Set)
int	Set;
{
	int	i;

	if (xcb[h].Media_Status & MEDIA_INITIALIZED)
		PUT_REGISTER(h, CMD, PAGE_1+STA+RDC_ABT);			/* Select Page 1 */
	for (i = 0; i < 8; i++) {
		PE2_NIC_Configuration_Table[NIC_TABLE_MAR0+i].NIC_Register_Value =
			(Set ? -1 : 0);
		if (xcb[h].Media_Status & MEDIA_INITIALIZED)
			PUT_REGISTER(h, MAR0+i, (Set ? -1 : 0));
	}
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);			/* Select Page 0 */
		if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) )
			xcb[h].Adapter_Pulse_Int(h);				/* any status port read could */
									/*  clobber a pending INT */
	}
}

/* void Set_Physical_Address(h,Address)                                      */
/*                                                                           */
/* Passed:	                                                             */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* 	char far *Address = points to an address to write to the NIC         */
/*                                                                           */
/* Returns:	nothing                                                      */
/*                                                                           */
/* This routine sets the NIC's network address.                              */
/*                                                                           */
/*                                                                           */

void PE2_Set_Physical_Address(h, Address)
char far *Address;
{
	int	i;

	if (xcb[h].Media_Status & MEDIA_INITIALIZED)
		PUT_REGISTER(h, CMD, PAGE_1+STA+RDC_ABT);
	for (i = 0; i < 6; i++, Address++) {
		PE2_NIC_Configuration_Table[NIC_TABLE_PAR0+i].NIC_Register_Value = *Address;
		if (xcb[h].Media_Status & MEDIA_INITIALIZED)
			PUT_REGISTER(h, PAR0+i, *Address);
	}
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);			/* Select Page 0 */
		if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) )
			xcb[h].Adapter_Pulse_Int(h);				/* any status port read could */
									/*  clobber a pending INT */
	}
}

/* void Get_Physical_Address(h,Buffer)                                       */
/*                                                                           */
/* Passed:	                                                             */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	char far *Buffer = points to a 6-byte buffer                         */
/*                                                                           */
/* Returns:	The Physical address of the NIC in the passed buffer         */
/*                                                                           */
/* Copies the address currently being used by the NIC into the buffer */

void PE2_Get_Physical_Address(h, Buffer)
char far *Buffer;
{
	int	i;

	for (i = 0; i < 6; i++, Buffer)
		*Buffer++ = PE2_NIC_Configuration_Table[NIC_TABLE_PAR0+i].NIC_Register_Value;
}

/* void Media_Unhook(h)                                                       */
/*                                                                           */
/* Passed:	                                                             */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	Nothing                                                      */
/*                                                                           */
/* Unhooks the adapter completely from the host.                             */
/*                                                                           */
/*                                                                           */

void PE2_Media_Unhook(h)
{
	Adapter_Unhook(h);
#ifdef SCPA
	scpa_close(xcb[h].cookie);
#endif
	xcb_Free(h);
}

/****************************************************************************************************************; */
/* Transient Data Definitions											; */
/*---------------------------------------------------------------------------------------------------------------; */

char Memory_Test_Pattern;
char MSG_Init_Failed[]		= ADAPTER_NAME " failed initialization";
char MSG_Memory_Test_Failed[]	= ADAPTER_NAME " failed memory test";

/* Media_Initialize(h,User_Service_Routine, Node_Address, String)            */
/*                                                                           */
/* Passed:                                                                   */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	void (*User_Service_Routine)() == Address of user interrupt          */
/* 		service routine (NULL for none)                              */
/*                                                                           */
/* 		unsigned char far *Node_Address == place to store node       */
/* 		address or buffer with node address override if              */
/* 		*Node_Addres==0xFF                                           */
/*                                                                           */
/* Returns:	Error code or 0 if no error                                  */
/* 		char **String == offset in Data Group of error string        */
/* This routine is called to initialize the hardware                         */
/*                                                                           */

PE2_Media_Initialize(Handle, String, mip)
int *Handle;
char **String;
struct Media_Initialize_Params *mip;
{
	int	Value, i, h, rval;
	char	*Temp;

	h = *Handle = xcb_Alloc();

	if (h < 0) {
		*String = "Too many devices open";
		return 1;
	}

	xcb[h].Media_IO_Address = mip->Media_IO_Address;
	xcb[h].Media_IRQ_Number = mip->Media_IRQ;

	xcb[h].Send_Header = (unsigned char far *) mip->Send_Header;
	xcb[h].Receive_Header = (unsigned char far *) mip->Receive_Header;
	xcb[h].Send_Header_Size = mip->Send_Header_Size;
	xcb[h].Receive_Header_Size = mip->Receive_Header_Size;
	xcb[h].Node_Address = mip->Node_Address;
	xcb[h].XCB_Link_Pointer = mip->MIP_Link_Pointer;

#ifdef SCPA
	xcb[h].cookie = scpa_open(mip->Media_IO_Address);

	if (!xcb[h].cookie) {
		*String = "Too many devices open";
		rval = 1;
		goto free_xcb;
	}
#endif

	xcb[h].User_ISR =  mip->User_Service_Routine;
	Value = PE2_Adapter_Initialize(h, String);
	if (Value != 0) {
		rval = Value;
		goto free_all;
	}

	if (*xcb[h].Node_Address == 0xff)
		for (i = 0; i < 6; i++)
			xcb[h].Node_Address[i] = xcb[h].PE2_PX_EEPROM_Buffer.EE_Network_Address[i];
	for (i = 0; i < 6; i++)
		PE2_NIC_Configuration_Table[NIC_TABLE_PAR0+i].NIC_Register_Value =
			xcb[h].Node_Address[i];
#ifdef CLASSIC
	if (xcb[h].PE2_PX_EEPROM_Buffer.EE_Model_Number + 7 == '8') {		/* check for 8K adapter */
		MEM_PAGES	=  8192/256;			/* update memory size in pages */
		xcb[h].RCV_PAGES	=  8192/256 - 6*XMT_BUFFS_32K;	/* update number of pages for receive buffer */
		xcb[h].RCV_END_PAGE	= MEM_BEG_PAGE + 8192/256;	/* update end of receive buffer area */
		xcb[h].MEM_END_PAGE	= MEM_BEG_PAGE + 8192/256;	/* update ending memory page */
		PE2_NIC_Configuration_Table[ NIC_TABLE_PSTOP].NIC_Register_Value	= MEM_BEG_PAGE + 8192/256;
		PE2_NIC_Configuration_Table[ NIC_TABLE_PSTART].NIC_Register_Value	= RCV_BEG_PAGE_32K;
		PE2_NIC_Configuration_Table[ NIC_TABLE_BNDRY].NIC_Register_Value	= RCV_BEG_PAGE_32K;
		PE2_NIC_Configuration_Table[ NIC_TABLE_CURR].NIC_Register_Value	= RCV_BEG_PAGE_32K + 1;
		xcb[h].Boundary	= RCV_BEG_PAGE_32K;
		xcb[h].Next_Page	= RCV_BEG_PAGE_32K + 1;
	}
#endif
	if (NIC_Initialize(h) != 0) {				/* initialize the controller */
		Adapter_Unhook(h);
		*String = MSG_Init_Failed;
		rval = ERR_FAILED_INITIALIZATION;
		goto free_all;
	}
	if (Test_Adapter_Memory(h) != 0) {
		Adapter_Unhook(h);
		*String = MSG_Memory_Test_Failed;
		rval = ERR_FAILED_MEMORY_TEST;
		goto free_all;
	}
	xcb[h].Media_Status |= MEDIA_INITIALIZED+DRIVER_ENABLED;
/* cli */
	if (xcb[h].Media_IRQ_Number != -2) {
		if (xcb[h].Media_IRQ_Number == -1) xcb[h].Media_IRQ_Number = xcb[h].Hardware_IRQ_Number;
		Hardware_Setup_ISR(h);
	}
	PUT_REGISTER(h, TXCR, 0);					/* take NIC out of loopback */

#ifdef EXTERNAL
	Find_Media(h);
#endif
	PUT_REGISTER(h, IMR, INTERRUPT_MASK);
	PE2_Media_Enable_Int(h);
/* sti */
	Media_Config_Text(h, String, &Temp, &Temp);
	return (0);

free_all:
	;
#ifdef SCPA
	scpa_close(xcb[h].cookie);
#endif
free_xcb:
	xcb_Free(h);

	return rval;
}

/* Test Adapter Memory Routine */
/*                                                                           */
/* Passed:                                                                   */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	Nothing                                                      */
/*                                                                           */
/*	Returns:	number of errors (0 == success) */
/*                                                                           */
/*	performs three passes writing all of adapter memory  */

static Test_Adapter_Memory(h)
{
	int	i;

	for (i = 0; i < 3; i++)
		if (Memory_Test(h, xcb[h].PE2_PX_EEPROM_Buffer.EE_Network_Address[i*2]))	/* use our address for pattern generation */
			return(-1);
	return(0);
}

/* Adapter Memory Test Routine */
/* Passed:                                                                   */
/*                                                                           */
/*	int h = The handle returned from Media_Initialize                    */
/*
/*	int Seed = seed for the memory test                                  */
/*
/* Returns: */
/*	0 if ok */
/*	non 0 of error */
/*                                                                           */
/*	Tests the memory using the given seed */
/*                                                                           */

static Memory_Test(h, Seed)
int	Seed;
{
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);		/* set up for remote dma */
#ifdef	PRQ_FIX
	PUT_REGISTER(h, RBCR0, 0fh);			/* bug fix for 8390 */
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_RD);		/* (forces PRQ high) */
#endif
	PUT_REGISTER(h, RBCR1, MEM_PAGES);			/* now set up for real */
	PUT_REGISTER(h, RBCR0, 0);				/* transfer */
	PUT_REGISTER(h, RSAR1, MEM_BEG_PAGE);
	PUT_REGISTER(h, RSAR0, 0);
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_WR);

	if (xcb[h].Write_Test_Pattern(h, Seed, MEM_PAGES*256) != 0)
		return(-1);

	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);		/* terminate the dma */
	PUT_REGISTER(h, RBCR1, MEM_PAGES);			/* set up for check */
	PUT_REGISTER(h, RBCR0, 0);
	PUT_REGISTER(h, RSAR1, MEM_BEG_PAGE);
	PUT_REGISTER(h, RSAR0, 0);
	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_RD);

	if (xcb[h].Check_Test_Pattern(h, Seed, MEM_PAGES*256) != 0)
		return(-1);

	PUT_REGISTER(h, CMD, PAGE_0+STA+RDC_ABT);		/* terminate the dma */
	PUT_REGISTER(h, RBCR1, 0);
	PUT_REGISTER(h, RBCR0, 0);
	return (0);
}
