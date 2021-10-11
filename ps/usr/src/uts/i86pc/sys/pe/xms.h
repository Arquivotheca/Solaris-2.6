/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)xms.h	1.1	93/10/29 SMI"

/****************************************************************************************************************; */
/* Exports to the Network Link Control Module									; */ 
/*---------------------------------------------------------------------------------------------------------------; */
/*	Media_Initialize */
/*	Media_Config_Text */
/*	Media_Unhook */
/*	Media_Send */
/*	Media_Reset */
/*	Media_Enable_Int */
/*	Media_Disable_Int */
/*	Media_Force_Int */
/*	Media_Copy_Packet */

/*	Get_Physical_Address */
/*	Set_Physical_Address */
/*	Get_Receive_Mode */
/*	Set_Receive_Mode */
/*	Adapter_Get_Data */

/*	MCast_Change_Address */
/*	MCast_Change_All */

/****************************************************************************************************************; */
/* Data Exports to the Network Link Control Module								; */
/*---------------------------------------------------------------------------------------------------------------; */
/*	Media_Configuration */
/*	Media_Status */

/*	Media_Keywords */
/*	Media_Keyword_Text */
/*	Media_Keyword_Length */
/*	Media_Keyword_Function */

/****************************************************************************************************************; */
/* Exports to the Host Hardware Control Module									; */
/*---------------------------------------------------------------------------------------------------------------; */
/*	Media_ISR */
/*	Media_Poll */
/*	Media_Timer_ESR */

/****************************************************************************************************************; */
/* Data Exports to the Host Hardware Control Module								; */
/*---------------------------------------------------------------------------------------------------------------; */
/*	Media_IRQ_Number */
/*	Media_IO_Address */
/*	Media_Memory_Address */

/****************************************************************************************************************; */
/* Imports from Link Control Module										; */
/*---------------------------------------------------------------------------------------------------------------; */
/* Link_Receive_Packet */
/* Link_Receive_Error */
/* Link_Transmit_Complete */
/* Link_Driver_Shutdown */

/****************************************************************************************************************;
/* Data Imports from Link Control Module										; */
/*---------------------------------------------------------------------------------------------------------------; */
extern 	char far *Receive_Header;
extern 	Receive_Header_Size;
extern 	char far *Send_Header;
extern 	Send_Header_Size;

/****************************************************************************************************************; */
/* Data Imports from Hardware Control Module									; */
/*---------------------------------------------------------------------------------------------------------------; */
extern	Hardware_Configuration,
	Hardware_Status,
	Hardware_Memory_Address,
	Hardware_IRQ_Number;

/****************************************************************************************************************; */
/* Media equates used by Link Layer										; */
/*---------------------------------------------------------------------------------------------------------------; */

/*// Media Status Bit Map */
#define MEDIA_INITIALIZED	0x01
#define DRIVER_ENABLED		0x02
#define INTERRUPTS_DISABLED	0x04
#define MEDIA_IN_ISR		0x08
#define MEDIA_IN_SEND		0x10
#define MEDIA_TX_PENDING	0x20

/*// Receive Configuration Options */
#define SEP			0x01				/* save errored packets */
#define ARP			0x02				/* accept runt packets */
#define ABP			0x04				/* accept broadcast packets */
#define AMP			0x08				/* accept multicast packets */
#define PRO			0x10				/* promiscuous mode */
#define MON			0x20				/* monitor mode */
#define ADP			0x80				/* Directed packets */

/*;; Receive Status Equates */
#define RX_PACKET_PRX		0x01
#define RX_PACKET_CRC		0x02
#define RX_PACKET_FAE		0x04
#define RX_PACKET_FO		0x08
#define RX_PACKET_MPA		0x10
#define RX_PACKET_MBAM		0x20
#define RX_PACKET_DIS		0x40
#define RX_PACKET_DFR		0x80

/*;; Transmit Status Equates */
#define TX_PACKET_OK		0x01					/* packet transmitted ok */
#define TX_PACKET_COL		0x04					/* transmit collided */
#define TX_PACKET_ABT		0x08					/* transmit aborted */
#define TX_PACKET_CRS		0x10					/* carrier sense lost */
#define TX_PACKET_FFU		0x20					/* fifo underrun */
#define TX_PACKET_CDH		0x40					/* collision detect heartbeat */
#define TX_PACKET_OWC		0x80					/* out of window collision */

