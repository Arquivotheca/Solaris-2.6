/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)pe.h	1.1	93/10/29 SMI"

/*
 * Hardware specific driver declarations for the Xircom PE
 * driver conforming to the Generic LAN Driver model.
 */
#ifndef _PE_H
#define _PE_H 1

/* debug flags */
#define PETRACE	0x01
#define PEERRS		0x02
#define PERECV		0x04
#define PEDDI		0x08
#define PESEND		0x10
#define PEINT		0x20

#ifdef DEBUG
#define PEDEBUG 1
#endif

/* Misc */
#define PEHIWAT	32768		/* driver flow control high water */
#define PELOWAT	4096		/* driver flow control low water */
#define PEMAXPKT	1500		/* maximum media frame size */
#define PEIDNUM	0		/* should be a unique id; zero works */

/* board state */
#define PE_IDLE	0
#define PE_WAITRCV	1
#define PE_XMTBUSY	2
#define PE_ERROR	3

/* receive mode */
#define PE_RMODE_SEP	0x01	/* Save erred packets */
#define PE_RMODE_ARP	0x02	/* Accept Runt packets */
#define PE_RMODE_ABP	0x04	/* Accept Broadcast packets */
#define PE_RMODE_AMP	0x08	/* Accept Multicast packets */
#define PE_RMODE_PRO	0x10	/* Promiscuous mode */
#define PE_RMODE_MON	0x20	/* Monitor, don't save packets in memory */
#define PE_RMODE_ADP	0x80	/* Accept directed packets */

/* driver specific declarations */
struct peinstance {
	int pe_random;		/* device specific data item */
	int pe_handle;		/* xircom ddk returned handle */
	char pe_rbuf[14];		/* xircom ddk prescan receive buffer */
	int pe_initialized;

	char *pe_Send_Header;
	char *pe_Receive_Header;
	int pe_Send_Header_Size;
	int pe_Receive_Header_Size;
	int pe_receive_mode;

	int (*Media_Copy_Packet)();
	int (*Media_Send)();
	int (*Media_Poll)();
	int (*Get_Physical_Address)();
	int (*Set_Physical_Address)();
	int (*Media_Unhook)();
	int (*Set_Receive_Mode)();
	int (*Media_ISR)();
	int (*MCast_Change_Address)();
};

#endif
