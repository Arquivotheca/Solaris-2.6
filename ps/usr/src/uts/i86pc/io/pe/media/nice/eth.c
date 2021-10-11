/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)eth.c	1.6	94/04/19 SMI"

/* ETH.INC Media Controller module for Fujitsu NICE    Copyright (C) 1992 Xircom */

/****************************************************************************************************************; */
/* Revision Information                                                                                          ; */
/*                                                                                                               ; */
/*       Author:         Eric Henderson	                                                                        ; */
/*       Started:        9 Sep 92		                                                                ; */
/*       Code From:      8390 Media Controller Module (E Henderson)	                                        ; */
/*			BETA NICE Media Controller Module (M Knudstrup)	                                        ; */
/*                                                                                                               ; */
/*       Language:       MASM 5.1                                                                                ; */
/*       Build:          (this file is included in the MAC XCS layer)		                                ; */
/*                                                                                                               ; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Release       Date    Who             Why                                                                     ; */
/* -------       ----    ---             --------------------------------------------------------                ; */
/*                                                                                                               ; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Notes:                                                                                                        ; */
/*                                                                                                               ; */
/*---------------------------------------------------------------------------------------------------------------; */
/* History:                                                                                                      ; */
/*                                                                                                               ; */
/* Date   Who    What                                                                                            ; */
/* ----   ---    -------------------------------------------------------------------------------------------     ; */
/*---------------------------------------------------------------------------------------------------------------; */

#include "nicenicdef.h"

/****************************************************************************************************************; */
/* Constant Definitions												; */
/*---------------------------------------------------------------------------------------------------------------; */
#define MAX_DATA_PACKET		1514				/* maximum data packet */
#define POLYL			0x1db6				/* used in the Multi Cast routine */
#define POLYH			0x04c1
#define MULTICAST_BIT		0x1

#define LB_TEST_SIZE 		256				/* Loop back test variables */
#define LB_TEST_WAIT		LB_TEST_SIZE

/****************************************************************************************************************; */
/* Structure Definitions												; */
/*---------------------------------------------------------------------------------------------------------------; */
#ifdef notdef
/*// Receive Buffer Header Structure */
struct Receive_Buffer_Header {
unsigned char RCV_NIC_Status;
unsigned char RCV_NIC_Unused;
unsigned short  RCV_NIC_Byte_Count;
};
#endif

static void Service_Xmt_Complete();
static void Service_Rcv_Packet();
static void Service_Rcv_Error();
void PE3_Media_Disable_Int();

/*// Fragment Descriptor Structure */
struct Fragment_Descriptor {
char far *Fragment_Pointer;
int	  Fragment_Length;
};

/*// 8390 Initialization Table Structure */
struct NIC_Table_Entry {
unsigned char	NIC_Register_Number,
		NIC_Register_Value,
		NIC_Register_Bank;
};

/*******************************************************************************; */
/* Resident Data								; */
/*------------------------------------------------------------------------------; */
#ifdef notdef
static int	(*User_ISR)();							/* address of user interrupt service routine */
static int	Media_Status		= 0,
	MCP_Leftovers		= 0,
	MCP_Fragment_Count	= 0,
	MCP_Fragment_Index	= 0,
	MS_Reenable_Ints	= 0,
	Tx_Length		= 0,

	SBF_Resend		= 0,
	Rx_Leftovers		= M_FALSE,
	Rx_Bytes_Left		= 0,
	Link_Receive_Status	= 0,
	Link_Integrity		= M_TRUE,
	Receive_Mode		= 0,
	Last_Tx_Status		= 0;

static struct	Receive_Buffer_Header Receive_Status;
#endif


/*// NIC Configuration Table */
#define NUM_NIC_ENTRIES		25
#define NIC_TABLE_RXCR		6
#define NIC_TABLE_PAR0		8
#define NIC_TABLE_MAR0		14
#define NIC_LI			23

static struct  NIC_Table_Entry NIC_Configuration_Table[NUM_NIC_ENTRIES] = {
{ CONTROL_REG1,	(CTRL_REG1+DIS_NICE),	ID_BNK },	/* DISABLE NICE */
{ TX_STATUS,	TX_MASK,		ID_BNK },
{ RX_STATUS,	RX_MASK,		ID_BNK },
{ TX_INT_MASK,	TX_MASK,		ID_BNK },
{ RX_INT_MASK,	RX_MASK,		ID_BNK },
{ TX_MODE_REG,	TX_MODE,		ID_BNK },
{ RX_MODE_REG,	0x00,			ID_BNK },
{ CONTROL_REG2,	NOT_STBY,		ID_BNK },
{ IDR8,		0xff,			ID_BNK },
{ IDR9,		0x80,			ID_BNK },
{ IDR10,	0xc7,			ID_BNK },
{ IDR11,	0x00,			ID_BNK },
{ IDR12,	0x00,			ID_BNK },
{ IDR13,	0x00,			ID_BNK },
{ HTR8,		0x00,			HT_BNK },
{ HTR9,		0x00,			HT_BNK },
{ HTR10,	0x00,			HT_BNK },
{ HTR11,	0x00,			HT_BNK },
{ HTR12,	0x00,			HT_BNK },
{ HTR13,	0x00,			HT_BNK },
{ HTR14,	0x00,			HT_BNK },
{ HTR15,	0x00,			HT_BNK },
{ COL_CTRL,	COL_CTRL_VAL,		BM_BNK },
{ BMPR13,	0x00,			BM_BNK },
{ CONTROL_REG1,	CTRL_REG1,		BM_BNK }		/* ENABLE NICE */
};

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




PE3_Media_Send(h, Tmt_Length, Copy_Count, Frag_Count, Frag_List)
int	Tmt_Length, Copy_Count, Frag_Count;
struct	Fragment_Descriptor far *Frag_List;
{
	int	Reenable = M_FALSE, Block_Count;
	struct	Fragment_Descriptor far *Curr_Frag;

	if ( (xcb[h].Hardware_Status & HARDWARE_UNAVAILABLE) ||
		(xcb[h].Media_Status & MEDIA_IN_SEND) ) return (XM_UNAVAILABLE);
	if (Check_Driver_Shutdown(h)) return (XM_DRIVER_SHUTDOWN);
	xcb[h].Media_Status |= MEDIA_IN_SEND;
	if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) ) {
		PE3_Media_Disable_Int(h);
		Reenable = M_TRUE;
	}
/* sti */
	PUT_REGISTER(h, CONTROL_REG2, BM_BNK);
	PUT_REGISTER(h, DATAPORT, Tmt_Length & 0xff);		/* write transmit length */
	PUT_REGISTER(h, DATAPORT, ((Tmt_Length >> 8) & 0xff));

	if (Copy_Count + xcb[h].Send_Header_Size) {
		SETUP_BLOCK_WRITE(h);
		if (xcb[h].Send_Header_Size)
			BLOCK_WRITE(h, (char far *) xcb[h].Send_Header, (int) xcb[h].Send_Header_Size);
		if (Frag_Count) {
			for (Curr_Frag = Frag_List; Frag_Count && Copy_Count; Frag_Count--, Curr_Frag++) {
				if (!Curr_Frag->Fragment_Length) continue;
				if (Curr_Frag->Fragment_Length > Copy_Count)
					Block_Count = Copy_Count;
				else	Block_Count = Curr_Frag->Fragment_Length;
				BLOCK_WRITE(h, Curr_Frag->Fragment_Pointer, Block_Count);
				Copy_Count -= Block_Count;
			}
		} else {					/* Contiguous copy */
			if (Copy_Count)
				BLOCK_WRITE(h, (char *)Frag_List, Copy_Count);
		}
		FINISH_BLOCK_WRITE(h);
	}
	PUT_REGISTER(h, TX_CTRL, (TX_START + 1));
	xcb[h].Tx_Length = Tmt_Length;
/* cli */
	xcb[h].Media_Status &= ~MEDIA_IN_SEND;
	if (Reenable) PE3_Media_Enable_Int(h);
	return (0);
}

/* int Media_Poll(h)                                                         */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/* Returns:	0 if Media_Poll was able to poll the adapter                 */
/* 		-1 if Media_Poll was unable to poll the adapter              */
/*                                                                           */

PE3_Media_Poll(h)
{
/* cli						// I don't trust them */
	if (xcb[h].Hardware_Status & HARDWARE_UNAVAILABLE) return (-1);
	if (xcb[h].Media_Status & (MEDIA_IN_ISR+INTERRUPTS_DISABLED+MEDIA_IN_SEND) )
		return (-1);
	if (GET_REGISTER(h, RX_MODE_REG) & RX_BUF_EMPTY) {
		xcb[h].Media_Status |= MEDIA_IN_ISR;
		PE3_Media_Disable_Int(h);
		Service_Rcv_Packet(h);
		xcb[h].Media_Status &= ~MEDIA_IN_ISR;
	}
	PE3_Media_ISR(h);
	return (0);
}

/***************************************************************************************************************; */
/* NICE Event Service Routines											; */
/*--------------------------------------------------------------------------------------------------------------; */

/* PE3_Media_ISR(h)                                                          */
/*                                                                           */
/* Passed:	                                                             */
/*	int h = The handle returned from Media_Initialize                    */
/*                                                                           */
/*	Returns:	nothing */
/*                                                                           */
/*	Services Media interrupts */

PE3_Media_ISR(h)
{
	int i = 1000;

	if ((xcb[h].Media_Status & MEDIA_IN_ISR)) return (-1);
	xcb[h].Media_Status |= MEDIA_IN_ISR;
	PE3_Media_Disable_Int(h);
	while (i > 0) {
		while ( i-- > 0 && ((xcb[h].Interrupt_Status = (
				(GET_REGISTER(h, RX_STATUS) << 8) |
				((unsigned char)GET_REGISTER(h, TX_STATUS) &
				((RX_MASK*256)+TX_MASK)) )) !=0) ) {		/* get the status bits */
/*sti */
			if (xcb[h].Interrupt_Status == -1) {
				Check_Driver_Shutdown(h);
				break;
			}
			if (xcb[h].Interrupt_Status & ((PKT_RDY+OVR_FLO) << 8))/*KBD*/
				Service_Rcv_Packet(h);
			if (xcb[h].Interrupt_Status & (RX_MASK & ~(PKT_RDY+OVR_FLO))) {
				PUT_REGISTER(h,  RX_STATUS, (xcb[h].Interrupt_Status >> 8) &
					(~(PKT_RDY+OVR_FLO)) );
				Service_Rcv_Error(h);
			}
			if (xcb[h].Interrupt_Status & TX_MASK) {
				PUT_REGISTER(h,  TX_STATUS, xcb[h].Interrupt_Status & 0xff);
				Service_Xmt_Complete(h);
			}
		}
/*cli */
		xcb[h].Media_Status &= ~MEDIA_IN_ISR;
		PE3_Media_Enable_Int(h);
#ifdef NEEDSWORK
		if (*User_ISR) (*User_ISR)();
#endif
/*cli */
		if (xcb[h].Rx_Leftovers != M_FALSE) {
			xcb[h].Media_Status |= MEDIA_IN_ISR;
			PE3_Media_Disable_Int(h);
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
	xcb[h].Last_Tx_Status = 0;
	if (xcb[h].Interrupt_Status & TX_OK)	xcb[h].Last_Tx_Status |= TX_PACKET_OK;
	if (xcb[h].Interrupt_Status & COL)	xcb[h].Last_Tx_Status |= TX_PACKET_COL;
	if (xcb[h].Interrupt_Status & COL_16)	{
		xcb[h].Last_Tx_Status |= TX_PACKET_ABT;
		xcb[h].Last_Tx_Status &= ~TX_PACKET_ABT;
		xcb[h].Tx_Length = 0;
	}
	Link_Transmit_Complete(h, xcb[h].Tx_Length, xcb[h].Last_Tx_Status,
		xcb[h].XCB_Link_Pointer);
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
	int	i, Value, Accept;
/* cli */
	while (1) {
		PUT_REGISTER(h, RX_STATUS, PKT_RDY+OVR_FLO);
		if ( GET_REGISTER(h, RX_MODE_REG) & RX_BUF_EMPTY) {
			xcb[h].Rx_Leftovers = M_FALSE;
			break;
		}
		SETUP_BLOCK_READ(h);
		BLOCK_READ(h, (char far *)&xcb[h].Receive_Status, sizeof(struct Receive_Buffer_Header));
		xcb[h].Rx_Bytes_Left = xcb[h].Receive_Status.RCV_NIC_Byte_Count;
		if (xcb[h].Receive_Header_Size) {
			Value = xcb[h].Receive_Status.RCV_NIC_Byte_Count;
			if (xcb[h].Receive_Header_Size < Value)
				Value = xcb[h].Receive_Header_Size;
			BLOCK_READ(h, (char far *)xcb[h].Receive_Header, Value);
			xcb[h].Rx_Bytes_Left -= Value;
			FINISH_BLOCK_READ(h);
		}
		xcb[h].Link_Receive_Status = 0;
		if (xcb[h].Receive_Status.RCV_NIC_Status & CRC_ERR) xcb[h].Link_Receive_Status |= RX_PACKET_CRC;
		if (xcb[h].Receive_Status.RCV_NIC_Status & ALG_ERR) xcb[h].Link_Receive_Status |= RX_PACKET_FAE;
		Accept = M_TRUE;
		if (xcb[h].Receive_Header_Size >= 6) {
			if (xcb[h].Receive_Header[0] & MULTICAST_BIT) xcb[h].Link_Receive_Status |= RX_PACKET_MBAM;
			for (i = 0; i < 6; i++)
				if (xcb[h].Receive_Header[i] != 0xff) break;
			if (i == 6) {
				xcb[h].Link_Receive_Status |= RX_PACKET_MBAM;
				if ( !(xcb[h].Receive_Mode & (ABP | PRO)) )
					Accept = M_FALSE;
			}
			if ( !(xcb[h].Receive_Mode & (ADP | PRO)) ) {
				for (i = 0; i < 6; i++) {
					if ((unsigned char)xcb[h].Receive_Header[i] !=
						NIC_Configuration_Table[NIC_TABLE_PAR0+i].NIC_Register_Value)
						break;
				}
				if (i == 6) Accept = M_FALSE;
			}
			for (i = 0; i < 6; i++) {
				if ((unsigned char)xcb[h].Receive_Header[6+i] !=
					NIC_Configuration_Table[NIC_TABLE_PAR0+i].NIC_Register_Value)
					break;
			}
			if (i == 6) Accept = M_FALSE;
		}
		if (Accept)
			Value = Link_Receive_Packet(h,
				xcb[h].Receive_Status.RCV_NIC_Byte_Count, 
				xcb[h].Link_Receive_Status,
				xcb[h].XCB_Link_Pointer);
		else	Value = -1;
		while (xcb[h].Rx_Bytes_Left--) /* KBD */
			GET_REGISTER(h, DATAPORT);
		if (Value == 0) {			/* If Link sets CX=0, they want no more */
			xcb[h].Rx_Leftovers = M_TRUE;
			break;
		}
		if (GET_REGISTER(h, RX_STATUS) & OVR_FLO) break;	/* Make sure we're not in a bad loop */
	}
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

PE3_Media_Copy_Packet(h, Offset, Count, Frag_Count, Frag_List)
int	Offset, Count, Frag_Count;
struct	Fragment_Descriptor far *Frag_List;
{
/* cli */
	int	  Leftovers = 0, Buffer_Size, Temp;
	char far *p, far *q = (char far *) Frag_List->Fragment_Pointer;

	if (Frag_Count)
		if ( (Buffer_Size = SizeOfTDBuffer(h,Frag_Count, Frag_List)) < Count)
			Count = Buffer_Size;
	Buffer_Size = xcb[h].Receive_Status.RCV_NIC_Byte_Count;
	if ( (Count + Offset) > Buffer_Size)
		Count = Buffer_Size - Offset;

	if (Offset < xcb[h].Receive_Header_Size) {
		Buffer_Size = xcb[h].Receive_Header_Size - Offset;		/* Number of bytes to read from xcb[h].Receive_Header */
		if (Buffer_Size > Count) Buffer_Size = Count;
		Count -= Buffer_Size;
		p = (char far *) (xcb[h].Receive_Header+Offset);
		if (Frag_Count == 0) {
			for ( ; Buffer_Size; Buffer_Size--, p++, q++)
				*q = *p;
		} else {
			while (Buffer_Size) {
				if ( (Temp = Frag_List->Fragment_Length) != 0) {
					if (Temp > Buffer_Size) {
						Leftovers = Temp - Buffer_Size;
						Temp = Buffer_Size;
					}
					q = (char far *)Frag_List->Fragment_Pointer;
					Buffer_Size -= Temp;
					for ( ; Temp; Temp--, p++, q++)
						*q = *p;
				}
				if (Leftovers == 0) Frag_List++;
			}
		}
	}
	if (Count) {
		if (Offset > xcb[h].Receive_Header_Size) {
			Temp = Offset - xcb[h].Receive_Header_Size;
			while (Temp) {
				GET_REGISTER(h, DATAPORT);
				xcb[h].Rx_Bytes_Left--;
				Temp--;
			}
		}
		if (Count > xcb[h].Rx_Bytes_Left) Count = xcb[h].Rx_Bytes_Left;
		SETUP_BLOCK_READ(h);
		if (Frag_Count) {
			if (Leftovers) {
				if (Leftovers > Count) Leftovers = Count;
				Count -= Leftovers;
				xcb[h].Rx_Bytes_Left -= Leftovers;
				BLOCK_READ(h, (char far *)q, Leftovers);
				Frag_List++;
			}
			while (Count) {
				if ( (Temp = Frag_List->Fragment_Length) != 0) {
					if (Temp > Count) Temp = Count;
					Count -= Temp;
					xcb[h].Rx_Bytes_Left -= Temp;
					BLOCK_READ(h, (char far *)Frag_List->Fragment_Pointer, Temp);
				}
				Frag_List++;
			}
		} else {
			xcb[h].Rx_Bytes_Left -= Count;
			BLOCK_READ(h, (char far *) q, Count);
		}
	}
	FINISH_BLOCK_READ(h);
	return(0);
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
/*	called by DriverISR to service a receive error.                      */


static void Service_Rcv_Error(h)
{
	int	FAE_Errs = 0,
		Missed   = 0,
		CRC_Errs = 0;
/* cli */
	PUT_REGISTER(h, RX_STATUS, xcb[h].Interrupt_Status & ~(PKT_RDY | OVR_FLO));
	if (xcb[h].Interrupt_Status & CRC_ERR) CRC_Errs = 1;
	if (xcb[h].Interrupt_Status & ALG_ERR) FAE_Errs = 1;
	Link_Receive_Error(h, FAE_Errs, Missed, CRC_Errs, xcb[h].XCB_Link_Pointer);
}

static void NIC_Disable_Interrupts(h)
{
	PUT_REGISTER(h, TX_INT_MASK, 0);
	PUT_REGISTER(h, RX_INT_MASK, 0);
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
	if (GET_REGISTER(h, CONTROL_REG1) != CTRL_REG1) {		/* make sure the command register contains what we wrote to it */
		PUT_REGISTER(h, CONTROL_REG1, CTRL_REG1);			/* write it first and then */
		if (GET_REGISTER(h, CONTROL_REG1) != CTRL_REG1) {
			PE3_Media_Reset(h);
			PUT_REGISTER(h, CONTROL_REG1, CTRL_REG1);
			if (GET_REGISTER(h, CONTROL_REG1) != CTRL_REG1)
				xcb[h].Media_Status &= ~DRIVER_ENABLED;
		}
	}
	return ( !(xcb[h].Media_Status & DRIVER_ENABLED) );
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

PE3_Media_Reset(h)
{
	struct NIC_Table_Entry *p = NIC_Configuration_Table;
	int	i,
		Current_Bank = ID_BNK;

	for (i = 0; i < NUM_NIC_ENTRIES; i++, p++) {
		if (p->NIC_Register_Bank != Current_Bank) {
			Current_Bank = p->NIC_Register_Bank;
			PUT_REGISTER(h, CONTROL_REG2, Current_Bank);	/* write value to register */
		}
		PUT_REGISTER(h, p->NIC_Register_Number, p->NIC_Register_Value);
	}
	return (0);
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


PE3_Media_Enable_Int(h)
{
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		if (xcb[h].Media_Status & (MEDIA_IN_ISR | MEDIA_IN_SEND) ) return (-1);
		xcb[h].Media_Status &= ~INTERRUPTS_DISABLED;
		Hardware_Enable_Int(h);
		Adapter_Pulse_Int(h);
	}
	return (0);
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


void PE3_Media_Disable_Int(h)
{
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		xcb[h].Media_Status |= INTERRUPTS_DISABLED;
		Adapter_Disable_Int(h);
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


void PE3_Media_Force_Int(h)
{
	Adapter_Force_Int(h);
}

static void (*Media_Timer_ESR)() = 0;

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


void PE3_Set_Receive_Mode(h, Mode)
int	Mode;
{
	if (Mode & ABP)
		NIC_Configuration_Table[NIC_TABLE_RXCR].NIC_Register_Value = AM0;
	if (Mode & AMP)
		NIC_Configuration_Table[NIC_TABLE_RXCR].NIC_Register_Value = AM1;
	if (Mode & PRO)
		NIC_Configuration_Table[NIC_TABLE_RXCR].NIC_Register_Value = AM0 | AM1;
	xcb[h].Receive_Mode = Mode;
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, RX_MODE_REG, NIC_Configuration_Table[NIC_TABLE_RXCR].NIC_Register_Value);
		if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) )
			Adapter_Pulse_Int(h);			/* any status port read could */
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


PE3_Get_Receive_Mode(h)
{
	return (xcb[h].Receive_Mode);
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


void PE3_MCast_Change_Address(h, Set, Address)
int	  Set;
char far *Address;
{
	int	i, j, carry;
	char	Byte;
	unsigned long	crc = -1;

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
		NIC_Configuration_Table[NIC_TABLE_MAR0+j].NIC_Register_Value |= 1 << i;
	else	NIC_Configuration_Table[NIC_TABLE_MAR0+j].NIC_Register_Value &= ~(1 << i);

	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CONTROL_REG1, (CTRL_REG1+DIS_NICE));
		PUT_REGISTER(h, CONTROL_REG2, HT_BNK);
		PUT_REGISTER(h, HTR8+j, NIC_Configuration_Table[NIC_TABLE_MAR0+j].NIC_Register_Value);
		PUT_REGISTER(h, CONTROL_REG1, CTRL_REG1);
		PUT_REGISTER(h, CONTROL_REG2, BM_BNK);
		if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) )
			Adapter_Pulse_Int(h);				/* any status port read could */
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

void PE3_MCast_Change_All(h, Set)
int	Set;
{
	int	i;

	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CONTROL_REG1, (CTRL_REG1+DIS_NICE));
		PUT_REGISTER(h, CONTROL_REG2, HT_BNK);
	}
	for (i = 0; i < 8; i++) {
		NIC_Configuration_Table[NIC_TABLE_MAR0+i].NIC_Register_Value =
			(Set ? -1 : 0);
		if (xcb[h].Media_Status & MEDIA_INITIALIZED)
			PUT_REGISTER(h, HTR8+i, (Set ? -1 : 0));
	}
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CONTROL_REG1, CTRL_REG1);
		PUT_REGISTER(h, CONTROL_REG2, BM_BNK);
		if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) )
			Adapter_Pulse_Int(h);				/* any status port read could */
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

void PE3_Set_Physical_Address(h, Address)
char far *Address;
{
	int	i;

	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CONTROL_REG1, (CTRL_REG1+DIS_NICE));
		PUT_REGISTER(h, CONTROL_REG2, ID_BNK);
	}
	for (i = 0; i < 6; i++, Address++) {
		NIC_Configuration_Table[NIC_TABLE_PAR0+i].NIC_Register_Value = *Address;
		if (xcb[h].Media_Status & MEDIA_INITIALIZED)
			PUT_REGISTER(h, IDR8+i, *Address);
	}
	if (xcb[h].Media_Status & MEDIA_INITIALIZED) {
		PUT_REGISTER(h, CONTROL_REG1, CTRL_REG1);
		PUT_REGISTER(h, CONTROL_REG2, BM_BNK);
		if ( !(xcb[h].Media_Status & INTERRUPTS_DISABLED) )
			Adapter_Pulse_Int(h);				/* any status port read could */
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

void PE3_Get_Physical_Address(h, Buffer)
char far *Buffer;
{
	int	i;

	for (i = 0; i < 6; i++, Buffer)
		*Buffer++ = NIC_Configuration_Table[NIC_TABLE_PAR0+i].NIC_Register_Value;
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


void PE3_Media_Unhook(h)
{
	PUT_REGISTER(h, CONTROL_REG1, (CTRL_REG1+DIS_NICE));
	PUT_REGISTER(h, CONTROL_REG2, 0);
	Adapter_Unhook(h);

#ifdef SCPA
	scpa_close(xcb[h].cookie);
#endif
	xcb_Free(h);
}

/****************************************************************************************************************; */
/* Transient Data Definitions											; */
/*---------------------------------------------------------------------------------------------------------------; */

char LB_Test_Packet[LB_TEST_SIZE];
#ifdef NEEDSWORK
char MSG_Init_Failed[]		= ADAPTER_NAME " failed initialization";
char MSG_Memory_Test_Failed[]	= ADAPTER_NAME " failed memory test";
#else
static char MSG_Init_Failed[]		= " failed initialization";
static char MSG_Memory_Test_Failed[]	= " failed memory test";
#endif

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


PE3_Media_Initialize(Handle, String, mip)
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
	Value = Adapter_Initialize(h, String);
	if (Value != 0) {
		rval = Value;
		goto free_all;
	}
	if (*mip->Node_Address == 0xff) {
		for (i = 0; i < 3; i++)
			mip->Node_Address[i] = xcb[h].PX_EEPROM_Buffer.EE_Address_Prefix[i];
#ifdef notdef
		for (i = 0; i < 3; i++)
			mip->Node_Address[2+i] = 
				((char *)&PX_EEPROM_Buffer.EE_Serial_Number)[i];
#endif
		mip->Node_Address[3]=((char *)&xcb[h].PX_EEPROM_Buffer.EE_Serial_Number)[2];
		mip->Node_Address[4]=((char *)&xcb[h].PX_EEPROM_Buffer.EE_Serial_Number)[1];
		mip->Node_Address[5]=((char *)&xcb[h].PX_EEPROM_Buffer.EE_Serial_Number)[0];
	}
	for (i = 0; i < 6; i++)
		NIC_Configuration_Table[NIC_TABLE_PAR0+i].NIC_Register_Value =
			mip->Node_Address[i];
	if (!xcb[h].Link_Integrity)
		NIC_Configuration_Table[NIC_LI].NIC_Register_Value |= LINK_DISABLE;
#ifdef notdef
	if (xcb[h].PX_EEPROM_Buffer.EE_Model_Number[2] != 'C') {
		if (xcb[h].PX_EEPROM_Buffer.EE_Model_Number[2] != 'T')
			NIC_Configuration_Table[NIC_LI].NIC_Register_Value |= PS_TP;
		else	NIC_Configuration_Table[NIC_LI].NIC_Register_Value |= PS_AUI;
	}
#endif
	PE3_Media_Reset(h);
#ifdef notdef
	if (NIC_Loopback_Test(h) != 0) {
		Adapter_Unhook(h);
		*String = MSG_Init_Failed;
		rval = ERR_FAILED_INITIALIZATION;
		goto free_all;
	}
#endif
	xcb[h].Media_Status |= MEDIA_INITIALIZED+DRIVER_ENABLED;
/* cli */
	if (xcb[h].Media_IRQ_Number != -2) {
		if (xcb[h].Media_IRQ_Number == -1) xcb[h].Media_IRQ_Number = xcb[h].Hardware_IRQ_Number;
		Hardware_Setup_ISR(h);
	}
	PE3_Media_Enable_Int(h);
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

static NIC_Loopback_Test(h)
{
	int	i;
	char	*p;
	char far *q;

	PUT_REGISTER(h, DLCR4, LOOPBACK);
	PUT_REGISTER(h, RX_MODE_REG, AM0+AM1);
	PUT_REGISTER(h, DATAPORT, LB_TEST_SIZE % 256);
	PUT_REGISTER(h, DATAPORT, LB_TEST_SIZE / 256);
	SETUP_BLOCK_WRITE(h);
	BLOCK_WRITE(h, (char far *)PE3_Media_Send, LB_TEST_SIZE);
	PUT_REGISTER(h, TX_CTRL, (TX_START+1)); 				/* initiate transmission */
	for (i = 0; i < LB_TEST_WAIT; i++)
		if (GET_REGISTER(h, TX_STATUS) & TX_OK) break;
	if (i < LB_TEST_WAIT) {
		PUT_REGISTER(h, TX_STATUS, 0xff);
		PUT_REGISTER(h, RX_STATUS, 0xff);
		PUT_REGISTER(h, RX_MODE_REG, NIC_Configuration_Table[NIC_TABLE_RXCR].NIC_Register_Value);
		SETUP_BLOCK_READ(h);
		BLOCK_READ(h, (char far *)LB_Test_Packet, 4);
		if (*(int *)(LB_Test_Packet+2) == LB_TEST_SIZE) {
			BLOCK_READ(h, (char far *)LB_Test_Packet, LB_TEST_SIZE);
			FINISH_BLOCK_READ(h);
			for (i = LB_TEST_SIZE, p = LB_Test_Packet, q = (char far *)PE3_Media_Send;
				i; i--, p++, q++)
				if (*p != *q) break;
		} else i = -1;
	}
	PUT_REGISTER(h, DLCR4, NOT_LPK);
	return (i);
}
