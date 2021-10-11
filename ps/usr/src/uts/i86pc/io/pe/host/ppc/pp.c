/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)pp.c	1.3	93/12/01 SMI"

void Hardware_Enable_Int();
void Hardware_Disable_Int();
void Programmable_Delay();
void Toggle_Port();


/* pp.c --- Parallel Port Host Hardware Control Module	(c) 1993 Xircom */

/****************************************************************************************************************; */
/* Revision Information												; */
/*														; */
/*	Author: 	Eric Henderson										; */
/*	Started:	23 Jan 92										; */
/*	Code From:	Old PXDRIVE.INC										; */
/*														; */
/*	Language:	MASM 5.1										; */
/*	Build:		MASM pp;										; */
/*			MASM /DOS2 pp, ppos2;									; */
/*														; */
/*--------------------------------------------------------------------------------------------------------------; */
/* Release 	Date	Who		Why									; */
/* -------	----	---		--------------------------------------------------------		; */
/*														; */
/*--------------------------------------------------------------------------------------------------------------; */
/* Notes:													; */
/*														; */
/*--------------------------------------------------------------------------------------------------------------; */
/* History:													; */
/*														; */
/* Date	 Who    What												; */
/* ----	 ---	------------------------------------------------------------------------------------------------; */
/* 3/93  EKH	Converted to C from assembly									; */
/*--------------------------------------------------------------------------------------------------------------; */

#include "config.h"
#ifdef SCPA
#include "scpa.h"
#endif
#include "xcb.h"
#include "pp.h"
#include "xhs.h"

/***************************************************************************************************************; */
/* PPX Specific Definitions											; */
/*--------------------------------------------------------------------------------------------------------------; */
#define USER_PPX_PORT_SELECTED	0x80
#define PPX_SLCT_CLK		0x08
#define PPX_SLCTA		0x04

/****************************************************************************************************************; */
/* Exports to Parallel Port specific Media Modules								; */
/*---------------------------------------------------------------------------------------------------------------; */
int	
	Enable_EPP_Mode(),
	Disable_EPP_Mode();

void	Programmable_Delay();				/* Currently not used */

#ifdef NEEDSWORK /* also defined below */
int	IRQ_Ctrl_Polarity;
int	LPT_Write_Ctrl;
int	LPT_Read_Ctrl;
int	LPT_Port_Number;
#endif

/****************************************************************************************************************; */
/* Imports from Media Module											; */
/*---------------------------------------------------------------------------------------------------------------; */
extern	Media_Configuration;


/****************************************************************************************************************; */
/* Resident Data Definitions											; */
/*---------------------------------------------------------------------------------------------------------------; */
char	Signature[] = "Xircom Pocket LAN Adapter";

#ifdef notdef
int	Hardware_Memory_Address,	/* Not used in Parallel Port HHC */
	Hardware_Status,
	Hardware_Configuration	=	PPX_HARDWARE,
	Hardware_IRQ_Number,

	LPT_Write_Ctrl	= NOT_RESET,	/* Parallel port control image */
	LPT_Read_Ctrl	= DIR_CTRL+NOT_RESET, /* Parallel port control image */
	LPT_Port_Number	= -1,		/* Printer port number (0, 1, or 2) */
	IRQ_Ctrl_Polarity = 0;	/* Remembers whether IRQ is active hi or lo */
#endif

/****************************************************************************************************************; */
/* Initialization Data Section											; */
/*---------------------------------------------------------------------------------------------------------------; */
int	PPI_Check_Adapter(), 
	PE2_Check_Adapter(), 
	PE1_Check_Adapter(), EE_Check_Adapter();

void	PGA_Set_IRQ_Signal(),PGA_Profile_LPT_Port(),Download_Bit_Stream(),
	PPI_Profile_Port(), PPI_Set_IRQ_Signal();

#ifdef notdef
int	Adapter_Type		=	0;	/* Type of Xircom hardware */
struct	Adapter_Control		*PPT_Table_Ptr;
#endif
struct	Adapter_Control		
	PPT_Table[5]		=	{
	{ PE3, PPI_Check_Adapter, PPI_Profile_Port, 	PPI_Set_IRQ_Signal},
	{ PE2, PE2_Check_Adapter, PGA_Profile_LPT_Port, PGA_Set_IRQ_Signal},
	{ EE,  EE_Check_Adapter,  PGA_Profile_LPT_Port, PGA_Set_IRQ_Signal},
	{ PE1, PE1_Check_Adapter, PGA_Profile_LPT_Port, PGA_Set_IRQ_Signal},
	{0}				/* This signals the last adapter */
};

char	Compaq_Signature[]	=	"COMPAQ";
char	MSG_Not_Found[]		=	"A Pocket LAN Adapter could not be found",
	MSG_Already_Loaded[]	=	"A Pocket LAN Adapter driver is already loaded",
	MSG_Bad_Interrupt[]	=	"The selected interrupt is unavailable",
	MSG_Bad_IO_Address[]	=	"The selected I/O address is unavailable",
	MSG_Bad_LPT_Number[]	=	"The selected LPT Port is unavailable";

#include	"pga.c"
#include	"ppi.c"

/***************************************************************************************************************; */
/* Hardware_Initialize												; */
/*														; */
/*	Assumes:	The variables LPT_Port_Number, Media_IRQ_Number, and Media_IO_Address			; */
/*			have been set to their desired values (-1 for auto determination)			; */
/*			The variable Device_Help has been initialized for OS/2					; */
/*														; */
/*	Returns:	error code (defined in CONFIG.EQU):							; */
/*				0 -> initialization successful							; */
/*				1 -> Xircom LAN adapter missing, not powered, or malfunctioned			; */
/*				2 -> a driver is already loaded (a new adapter could not be found)		; */
/*				3 -> The selected Media_IO_Address is unavailable				; */
/*				4 -> The selected Media_Interrupt is unavailable				; */
/*			INT = disabled										; */
/*														; */
/*			All variables called with auto determination have been set to				; */
/*			discovered values (if any)								; */
/*														; */
/*	Description:	Called to discover the hardware configuration. If an adapter is found			; */
/*			some variables are initialized, namely the I/O port. This routine reports		; */
/*			the interrupt level found, but it does not hook the interrupt. A separate call		; */
/*			to Hardware_Setup_ISR must be made.							; */
/*														; */
/*---------------------------------------------------------------------------------------------------------------; */
Hardware_Initialize(h, Adapter, Err_String)
int	*Adapter;						/* Reports which Xircom adapter found on port */
char	**Err_String;						/* Points to error text string (if unsuccess) */
{
	int	result;
	struct	Adapter_Control *PPT_Ptr;

	if ((result = Find_Xircom_Adapter(h, Err_String)) == 0) {
		Find_IRQ_Number(h);
		(xcb[h].PPT_Table_Ptr->Profile_Port)(h);
		*Adapter = xcb[h].Adapter_Type;
		result = 0;
	}
	return (result);
}

Find_Xircom_Adapter(h, Err_String)
char	**Err_String;
{
	if (xcb[h].Media_IO_Address != -1) {
		for (xcb[h].LPT_Port_Number = 2;xcb[h].LPT_Port_Number >= 0;xcb[h].LPT_Port_Number--)
			if (LPT_Num_To_Port(h,xcb[h].LPT_Port_Number)==xcb[h].Media_IO_Address)
				break;

		if (xcb[h].LPT_Port_Number == -1) 
			/* we could not find a BIOS table entry for the IO */
			/* address specified so use this one since it's least */
			/* likely to interfere */
			xcb[h].LPT_Port_Number = 2;	

		if (Test_LPT_Port(h) == 0) return (0);
		xcb[h].LPT_Port_Number = -1;
		*Err_String = MSG_Bad_IO_Address;
		return (ERR_SELECTED_IO);
	}

	if (xcb[h].LPT_Port_Number != -1) {
		xcb[h].Media_IO_Address = LPT_Num_To_Port(h,xcb[h].LPT_Port_Number);
		if (xcb[h].Media_IO_Address & 0xff00)
			if (Test_LPT_Port(h) == 0) return (0);
		xcb[h].Media_IO_Address = -1;
		*Err_String = MSG_Bad_LPT_Number;
		return (ERR_SELECTED_LPT);
	}
	for (xcb[h].LPT_Port_Number = 2; xcb[h].LPT_Port_Number >= 0; xcb[h].LPT_Port_Number--)
		if (LPT_Num_To_Port(h,xcb[h].LPT_Port_Number) != 0) break;
	if (xcb[h].LPT_Port_Number == -1) {
		xcb[h].LPT_Port_Number		= 0;
		xcb[h].Media_IO_Address	= 0x378;
		if (Test_LPT_Port(h) == 0) return (0);
		xcb[h].Media_IO_Address	= 0x278;
		if (Test_LPT_Port(h) == 0) return (0);
		xcb[h].Media_IO_Address	= 0x3bc;
		if (Test_LPT_Port(h) == 0) return (0);
		xcb[h].LPT_Port_Number	= -1;
		xcb[h].Media_IO_Address= -1;
	}
	for (xcb[h].LPT_Port_Number = 2; xcb[h].LPT_Port_Number >= 0; xcb[h].LPT_Port_Number--)
		if ((xcb[h].Media_IO_Address = LPT_Num_To_Port(h,xcb[h].LPT_Port_Number)) & 0xff00)
			if (Test_LPT_Port(h) == 0) return (0);
	if (Check_Already_Loaded(h) != 0) {
		*Err_String = MSG_Not_Found;
		return (ERR_ADAPTER_MISSING);
	} else {
		*Err_String = MSG_Already_Loaded;
		return (ERR_DRIVER_ALREADY_LOADED);
	}
}

/***************************************************************************************************************; */
/* Test LPT Port												; */
/*														; */
/*	Description:	An adapter is investigated using the I/O values set in Find_Xircom_Adapter.		; */
/*			If an adapter is found then returns the type.						; */
/*--------------------------------------------------------------------------------------------------------------; */
Test_LPT_Port(h)
{
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET);
				/* This code ensures the parallel port is */
				/* in a known state before we use. Note */
				/* that this code relies on the delay in */
				/* the following code */
	OUTB(xcb[h].Media_IO_Address,0, 0);
				/* This code is to ensure the PPX is in a */
				/* known state before we switch it. Forcing */
				/* the data lines low will help it come out */
				/* of a latch-up condition. */
	pe_delay(h,25000);	/* 25 milliseconds */
	OUTB(xcb[h].Media_IO_Address,0, -1);
	pe_delay(h,1);		/* 1 microsecond */

	for ( xcb[h].PPT_Table_Ptr = PPT_Table; 
	    xcb[h].PPT_Table_Ptr->Adapter_Type; xcb[h].PPT_Table_Ptr++) {
		xcb[h].Adapter_Type = xcb[h].PPT_Table_Ptr->Adapter_Type;
		xcb[h].Hardware_Status |= PPX_PORT_B;
		Toggle_Port(h);
		if ( (*xcb[h].PPT_Table_Ptr->Check_Adapter)(h) != 0) {
			xcb[h].Hardware_Status &= ~PPX_PORT_B;
			Toggle_Port(h);
			if ( (*xcb[h].PPT_Table_Ptr->Check_Adapter)(h) == 0)
				 break;
		} else break;

		OUTB(xcb[h].Media_IO_Address,2, SELECT_IN+NOT_RESET);
		xcb[h].Adapter_Type = 0;
	}
	if (xcb[h].Adapter_Type != 0) {
		Toggle_Port(h);
		xcb[h].Hardware_Configuration &= ~PPX_HARDWARE;
		if ( (*xcb[h].PPT_Table_Ptr->Check_Adapter)(h) != 0) {
			xcb[h].Hardware_Configuration |= PPX_HARDWARE;
			Toggle_Port(h);
		}
	}
	return (!xcb[h].Adapter_Type);		/* 0 = success */
}



void Hardware_Enable_Int(h)
{
	if ( !(xcb[h].Media_Configuration & EPP_MODE) )
		(void) INB(xcb[h].Media_IO_Address,1);			/* Some ports need a status read to reset IRQ's	 */
	xcb[h].LPT_Write_Ctrl |= LPT_IRQ_ENB;
	xcb[h].LPT_Read_Ctrl  |= LPT_IRQ_ENB;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

void Hardware_Disable_Int(h)
{
	xcb[h].LPT_Write_Ctrl &= ~LPT_IRQ_ENB;
	xcb[h].LPT_Read_Ctrl  &= ~LPT_IRQ_ENB;
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

void 
Programmable_Delay(h) { h++; }
	

/***************************************************************************************************************; */
/* Toggle Port 													; */
/*														; */
/*	Procedure Type:	near											; */
/*														; */
/*	Assumes:	DS  = CGroup alias									; */
/*			INT disabled										; */
/*														; */
/*	Returns:	nothing											; */
/*			destroyed: AX, BX, CX, DX								; */
/*														; */
/*	Description:	see the schematic									; */
/*--------------------------------------------------------------------------------------------------------------; */
void Toggle_Port(h)
{
	OUTB(xcb[h].Media_IO_Address,0, 0);					/* set data to zero, it will */
									/* go to pga control register */
	OUTB(xcb[h].Media_IO_Address,2, NOT_RESET);				/* make sure all control lines are reset */

	OUTB(xcb[h].Media_IO_Address,0, 0x10+PPX_SLCT_CLK+PPX_SLCTA);		/* d2 hi makes FF1=0 */
	OUTB(xcb[h].Media_IO_Address,0, 0x10+PPX_SLCTA);				/* second phase of clock */
									/* strobe d3 makes first FF a 0 */

	OUTB(xcb[h].Media_IO_Address,0, 0x10+PPX_SLCT_CLK);			/* Second phase of clock, prepare d2 lo */
	OUTB(xcb[h].Media_IO_Address,0, 0x10);					/* now FF1 is hi, FF2 is lo */

	OUTB(xcb[h].Media_IO_Address,0, 0x10+PPX_SLCT_CLK);
	OUTB(xcb[h].Media_IO_Address,0, 0x10);					/* now FF1=1 FF2=1 FF3=0 */

	OUTB(xcb[h].Media_IO_Address,0, 0x10+PPX_SLCT_CLK);
	if (xcb[h].Hardware_Status & PPX_PORT_B) {
		OUTB(xcb[h].Media_IO_Address,0, 0x10+PPX_SLCT_CLK+PPX_SLCTA);
		OUTB(xcb[h].Media_IO_Address,0, 0x10+PPX_SLCTA);			/* This clocks FF4, and d2 determines port */
	} else  OUTB(xcb[h].Media_IO_Address,0, 0x10);				/* This clocks FF4, and d2 determines port */
	xcb[h].Hardware_Status ^= PPX_PORT_B;

	/*// the following is for PE3 but may clobber older pe's */
	if (xcb[h].Hardware_Status & USER_PPX_PORT_SELECTED) {
		OUTB(xcb[h].Media_IO_Address,2, NOT_RESET+PPI_RDS+PPI_STROBE);
		OUTB(xcb[h].Media_IO_Address,2, NOT_RESET);
	}
}
