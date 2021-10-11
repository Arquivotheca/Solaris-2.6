/*
 * Copyright (c) 1993, by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)pe3.c	1.3	94/04/06 SMI"

/* pe3.inc --- Pocket Ethernet Adapter III Adapter Control Module (c) 1992 Xircom */

/***************************************************************************************************************; */
/* Revision Information												; */
/*														; */
/*	Author: 	Eric Henderson										; */
/*	Started:	9 Sep 92										; */
/*	Code From:	Pocket Ethernet Adapter II ACM (Eric Henderson 1/92)					; */
/*														; */
/*	Language:	MASM 5.1										; */
/*	Build:		MASM pe3;										; */
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
/* ----	 ---	-------------------------------------------------------------------------------------------	; */
/*														; */
/*--------------------------------------------------------------------------------------------------------------; */
#include "config.h"
#ifdef SCPA
#include "scpa.h"
#endif
#include "xcb.h"
#include "xms.h"
#include "ppi.h"
#include "pp.h"

/***************************************************************************************************************; */
/* Imports From Parallel Port specific Hardware Modules								; */
/*--------------------------------------------------------------------------------------------------------------; */
int	Enable_EPP_Mode(),
	Disable_EPP_Mode(),
	Programmable_Delay();


void EEPROM_Disable();
void EEPROM_Enable();
void EEPROM_Put_Bit();
int EEPROM_Get_Bit();

#ifdef notdef
extern	IRQ_Ctrl_Polarity,
	LPT_Write_Ctrl,
	LPT_Read_Ctrl,
	LPT_Port_Number;
#endif

extern	Link_Integrity;						/* Not really external */

/***************************************************************************************************************; */
/* Equates													; */
/*--------------------------------------------------------------------------------------------------------------; */
#define M_FALSE			0
#define M_TRUE			1

#define ADAPTER_NAME		"Pocket Ethernet Adapter III"
#define ADAPTER_TYPE_NUMBER	PE3

#define XIRCOM_ADDRESS_1	0x00
#define XIRCOM_ADDRESS_2	0x80
#define XIRCOM_ADDRESS_3	0xc7

#define NOT_EWRITE		1
#define NICE_DATA_PORT		8

/***************************************************************************************************************; */
/* Structure Definitions											; */
/*--------------------------------------------------------------------------------------------------------------; */
#ifdef notdef
struct EEPROM_Structure {
	unsigned char	EE_Copyright[16];			/* Copyright message */
	long		EE_Serial_Number;			/* serial number, 32 bit long */
	unsigned char	EE_Manufacture_Date[4];			/* manufacture date (w/o hsecs) */
	unsigned char	EE_Model_Number[3];			/* model number, ascii string */
	unsigned char	EE_Address_Prefix[3];			/* First three bytes of network address */
	unsigned short	EE_Check_Sum;				/* 16 bit check sum */
};
#endif

/***************************************************************************************************************; */
/* Resident Data Definitions											; */
/*--------------------------------------------------------------------------------------------------------------; */


#ifdef notdef
static int	Media_Configuration		= -1,
		Media_Memory_Address		= 0,
		Media_IRQ_Number		= -1,
		Media_IO_Address		= -1;

static char	Setup_Block_Read_Value	= NICE_DATA_PORT+PPI_READ+PPI_FALLING_EDGES+PPI_RISING_EDGES;	/*Bi-directional */
static char	Setup_Block_Write_Value	= NICE_DATA_PORT+PPI_FALLING_EDGES+PPI_RISING_EDGES;
#endif



#ifdef notdef
void	(*Set_Ctrl_Reg)();
static int	Media_Data_Strobe		= PPI_STROBE,
		Media_Not_Data_Strobe		= ~PPI_STROBE;
#endif

/***************************************************************************************************************; */
/* Resident Code Section											; */
/*--------------------------------------------------------------------------------------------------------------; */
#ifdef notdef
static void (*PUT_REGISTER)();
static int  (*GET_REGISTER)();
static void (*Block_Read)();
static void (*Block_Write)();
#endif

static void Adapter_Enable_Int(h)
{
	int	Value = PPI_IRQ_ENABLE;

	if (xcb[h].IRQ_Ctrl_Polarity)	Value |= PPI_IRQ_INVERT;
	PUT_REGISTER(h, GENREG0, Value);
}

static void Adapter_Disable_Int(h)
{
	int	Value = 0;

	if (xcb[h].IRQ_Ctrl_Polarity)	Value |= PPI_IRQ_INVERT;
	PUT_REGISTER(h, GENREG0, Value);
}

static void Adapter_Pulse_Int(h)
{
	Adapter_Disable_Int(h);
	Adapter_Enable_Int(h);
}

static void Adapter_Force_Int(h)
{
	int	Value;

	if (xcb[h].IRQ_Ctrl_Polarity) Value = 0;
	else Value = PPI_IRQ_INVERT;
	PUT_REGISTER(h, GENREG0, Value);
}

static void Adapter_Unhook(h)
{
	if (xcb[h].Media_Configuration & EPP_MODE)
		Disable_EPP_Mode(h);
	Hardware_Unhook(h);
}

void PE3_Setup_Block_Read(h)
{
	int	Value = PPI_READ_EDGE;

	xcb[h].Set_Ctrl_Reg(h, xcb[h].Setup_Block_Read_Value);
	if (xcb[h].Media_Configuration & (BIDIRECTIONAL_MODE | EWRITE_MODE)) {
		xcb[h].Set_Ctrl_Reg(h, 0);
		if (xcb[h].Media_Data_Strobe & PPI_STROBE) Value |= PPI_USE_STROBE;
		xcb[h].Set_Ctrl_Reg(h, Value);
	}
}

void PE3_Setup_Block_Write(h)
{
	xcb[h].Set_Ctrl_Reg(h, xcb[h].Setup_Block_Write_Value);
}

void PE3_Finish_Block_Read(h)
{
	int	Value;

	if (xcb[h].Media_Configuration & (BIDIRECTIONAL_MODE | EWRITE_MODE)) {
		Value = (xcb[h].LPT_Read_Ctrl & xcb[h].Media_Data_Strobe) | (PPI_RAS | xcb[h].LPT_Write_Ctrl);
		OUTB(xcb[h].Media_IO_Address,2, Value);
		xcb[h].LPT_Read_Ctrl &= xcb[h].Media_Not_Data_Strobe;
		Value &= xcb[h].Media_Not_Data_Strobe;
		OUTB(xcb[h].Media_IO_Address,2, Value);
		OUTB(xcb[h].Media_IO_Address,0, GENREG8+PPI_FALLING_EDGES);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
		xcb[h].Set_Ctrl_Reg(h, 0);
		Value = PPI_READ_PULSE;
		if (xcb[h].Media_Data_Strobe & PPI_STROBE) Value |= PPI_USE_STROBE;
		xcb[h].Set_Ctrl_Reg(h, Value);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Media_Data_Strobe);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	}
}

void PE3_Finish_Block_Write(h)
{
	int	Value;

	if ( !(xcb[h].Media_Configuration & (EPP_MODE+EWRITE_MODE)) ) {
		Value = xcb[h].LPT_Write_Ctrl | PPI_RAS;
		OUTB(xcb[h].Media_IO_Address,2, Value);
		xcb[h].LPT_Write_Ctrl &= xcb[h].Media_Not_Data_Strobe;
		Value &= xcb[h].Media_Not_Data_Strobe;
		OUTB(xcb[h].Media_IO_Address,0, GENREG8);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Media_Data_Strobe);
		OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	}
}

/***************************************************************************************************************; */
/* Initialization Data Section											; */
/*--------------------------------------------------------------------------------------------------------------; */
static char	NonKeyWord[]		= "NON",
	LPTKeyWord[]		= "LPT",
	LinkDisKeyword[]	= "LINKDISABLE",
	SPPKeyWord[]		= "SPP";

static int	Media_Keywords		= 3;
static char	*Media_Keyword_Text[]	= {
		NonKeyWord,
		LPTKeyWord,
		LinkDisKeyword,
		SPPKeyWord
	};

void Process_Non_Keyword(), Process_LPT_Keyword(), 
	Process_LinkDis_Keyword(), Process_SPP_Keyword();

static void	(*Media_Keyword_Function[])()	= {
		Process_Non_Keyword,
		Process_LPT_Keyword,
		Process_LinkDis_Keyword,
		Process_SPP_Keyword
	};


static char	MSG_Copyright[]		= "(C) Xircom 199",			/* Make sure (C) >= 1993 */
#ifdef NEEDSWORK
	MSG_EEPROM_Failed[]	= ADAPTER_NAME " Address EEPROM unreadable",
#else
	MSG_EEPROM_Failed[]	= " Address EEPROM unreadable",
#endif
	MSG_Bad_Mode[]		= "The selected hardware configuration is not supported",
	MSG_Wrong_Adapter[]	= "This driver does not support the currently connected adapter",
	MSG_Bad_LPT_Keyword[]	= "The LPT keyword must specify 1, 2, or 3, value ignored",
	MSG_Unrecognized[]	= "ERROR: Unrecognized command line option: ",
	MSG_Keyword[25];

#ifdef notdef
static char	*Message_Ptr		= MSG_Bad_Mode;				/* sanity default */
#endif

#ifdef NEEDSWORK
char	MSG_EPP_Mode[]		= ADAPTER_NAME " running in EPP mode",
	MSG_EWrite_Mode[]	= ADAPTER_NAME " running in Enhanced write mode",
	MSG_Bidir_Mode[]	= ADAPTER_NAME " running in Bidirectional mode",
	MSG_ENon_Bidir_Mode[]	= ADAPTER_NAME " running in Enhanced Non-Bidirectional mode",
	MSG_Non_Bidir_Mode[]	= ADAPTER_NAME " running in Non-Bidirectional mode",
	MSG_COMPAQ_Mode[]	= ADAPTER_NAME " running in Compaq Bidirectional mode",
	MSG_PPX_Loaded[]	= "Parallel Port Multiplexor support loaded";

char	MSG_Configuration[70]	= "LPT1:, No IRQ, ",
	MSG_No_IRQ[]		= "No IRQ, ",
	MSG_IRQ	[]		= "IRQ7, ",
	MSG_PPX[]		= ", PPX";
#endif

static char	MSG_EPP_Mode[]		= " running in EPP mode",
	MSG_EWrite_Mode[]	= " running in Enhanced write mode",
	MSG_Bidir_Mode[]	= " running in Bidirectional mode",
	MSG_ENon_Bidir_Mode[]	= " running in Enhanced Non-Bidirectional mode",
	MSG_Non_Bidir_Mode[]	= " running in Non-Bidirectional mode",
	MSG_COMPAQ_Mode[]	= " running in Compaq Bidirectional mode",
	MSG_PPX_Loaded[]	= "Parallel Port Multiplexor support loaded";

static char	MSG_Configuration[70]	= "LPT1:, No IRQ, ",
	MSG_No_IRQ[]		= "No IRQ, ",
	MSG_IRQ	[]		= "IRQ7, ",
	MSG_PPX[]		= ", PPX";

#ifdef notdef
static int	EEPROM_Enable_Flag	= 0;
#endif


int	Not_Implemented(), DQNT_Register_Test(), WBT_Register_Test(),
	PS2_Register_Test(), HH_Register_Test();
static int	(*Setup_Table[])()	= {
		DQNT_Register_Test,
		WBT_Register_Test,
		PS2_Register_Test,
		HH_Register_Test,
		Not_Implemented,
		Not_Implemented,
		Not_Implemented,
		Not_Implemented
	};

struct xcb xcb[MAXXCB];

void xcb_Free(h)
int h;
{
	if (h >= 0 && h < MAXXCB)
		xcb[h].inuse = 0;
}

int xcb_Alloc() {
	char *cp;
	int i,ii;

	for (i = 0; i < MAXXCB; i++)
		if (!xcb[i].inuse)
			break;

	if (i >= MAXXCB)
		return -1;


#ifdef NOTPORT
	memset(xcb[i], 0, sizeof(struct xcb));
#else
	for (ii=0, cp = (char *) &xcb[i]; ii < sizeof(struct xcb); ii++)
		*cp++ = 0;
#endif

	xcb[i].inuse = 1;
	xcb[i].Message_Ptr		= MSG_Bad_Mode;

	xcb[i].Media_Configuration	= -1;
	xcb[i].Media_IRQ_Number		= -1;
	xcb[i].Media_IO_Address		= -1;

	xcb[i].Setup_Block_Read_Value	= /*Bi-directional */
		NICE_DATA_PORT+PPI_READ+PPI_FALLING_EDGES+PPI_RISING_EDGES;	
	xcb[i].Setup_Block_Write_Value	= 
		NICE_DATA_PORT+PPI_FALLING_EDGES+PPI_RISING_EDGES;

	xcb[i].Media_Data_Strobe		= PPI_STROBE;
	xcb[i].Media_Not_Data_Strobe		= ~PPI_STROBE;

	/* Set up EEPROM_* for PE3, PE2 changes these */

	xcb[i].EEPROM_Disable = EEPROM_Disable;
	xcb[i].EEPROM_Enable = EEPROM_Enable;
	xcb[i].EEPROM_Put_Bit = EEPROM_Put_Bit;
	xcb[i].EEPROM_Get_Bit = EEPROM_Get_Bit;

	xcb[i].Rx_Leftovers		= M_FALSE;
	xcb[i].Link_Integrity		= M_TRUE;

	xcb[i].Hardware_Configuration	=	PPX_HARDWARE;

	xcb[i].LPT_Write_Ctrl	= NOT_RESET;
	xcb[i].LPT_Read_Ctrl	= DIR_CTRL+NOT_RESET;
	xcb[i].LPT_Port_Number	= -1;
	xcb[i].Data_Strobe	=	PPI_RDS;

#define XMT_BUFFS_8K			2
#define XMT_BUFFS_32K			2
#define RCV_BEG_PAGE_32K		XMT_BEG_PAGE + 6*XMT_BUFFS_32K

#define MEM_BEG_PAGE			0x20					/* starting memory page */
#define MEM_PAGES			32768/256				/* memory size in pages */
#define XMT_BEG_PAGE			MEM_BEG_PAGE				/* start of transmit buffer area */

	xcb[i].Tx_Page			=	XMT_BEG_PAGE;
	xcb[i].MEM_END_PAGE		=	MEM_BEG_PAGE + 32768/256;
	xcb[i].XMT_BUFFERS		=	XMT_BUFFS_32K;
	xcb[i].RCV_BEG_PAGE		=	RCV_BEG_PAGE_32K;
	xcb[i].RCV_PAGES		=	32768/256 - 6*XMT_BUFFS_32K;
	xcb[i].RCV_END_PAGE		=	MEM_BEG_PAGE + 32768/256;

	xcb[i].Next_Page		=	RCV_BEG_PAGE_32K + 1;
	xcb[i].Boundary			=	RCV_BEG_PAGE_32K;

	return i;
}


static Adapter_Initialize(h, Err_String)
char	**Err_String;
{
	int	Adapter, Value;
	extern int rflag;

	if ( (Value = Hardware_Initialize(h, &Adapter, Err_String)) != 0 )
		return Value;

	if ( Adapter != ADAPTER_TYPE_NUMBER) {
		*Err_String = MSG_Wrong_Adapter;
		return (ERR_WRONG_ADAPTER);
	}
	if (Setup_Configuration(h, xcb[h].Hardware_Configuration & xcb[h].Media_Configuration) != 0) {
		*Err_String = MSG_Bad_Mode;
		return (ERR_SELECTED_CONFIGURATION);
	}
	if (Read_PX_EEPROM(h) != 0) {
		*Err_String = MSG_EEPROM_Failed;
		return (ERR_EEPROM_UNREADABLE);
	}
	return(0);
}

/***************************************************************************************************************; */
/* Media Config Text												; */
/*														; */
/* Type:		Near											; */
/*														; */
/* Assumes:	DS  = DATA_GROUP alias										; */
/*														; */
/* Returns:	AX  = offset in DATA_GROUP of ASCII Configuration string short form (LPT1:, IRQ7, Bidirectional); */
/*		DX  = offset in DATA_GROUP of ASCII Mode string							; */
/*		BX  = offset in DATA_GROUP of ASCII Mode string continued					; */
/*		    = 0 if no continuation									; */
/*														; */
/* Description:	Sets up the configuration strings based on the media variables.					;												; */
/*--------------------------------------------------------------------------------------------------------------; */
static void Media_Config_Text(h, Config, Mode, Mode_Continued)
char	**Config, **Mode, **Mode_Continued;
{
	char	*p, *q;

	MSG_Configuration[3] = xcb[h].LPT_Port_Number + '1';
	if (xcb[h].Media_IRQ_Number) {
		MSG_IRQ[3] = xcb[h].Media_IRQ_Number + '0';
		for ( p = MSG_IRQ, q = MSG_Configuration+7; *p; p++, q++)
			*q = *p;
	} else q = MSG_Configuration + 15;
	for (p = xcb[h].Message_Ptr; *p; q++, p++) *q = *p;
	if (xcb[h].Hardware_Configuration & PPX_HARDWARE) {
		for (p = MSG_PPX; *p; q++, p++) *q = *p;
		xcb[h].Media_Configuration |= PPX_HARDWARE;
		*Mode_Continued = MSG_PPX_Loaded;
	}
	*q = '\0';
	*Config = MSG_Configuration;
	if (xcb[h].Media_Configuration & EPP_MODE)		  *Mode = MSG_EPP_Mode;
	if (xcb[h].Media_Configuration & EWRITE_MODE)		  *Mode = MSG_EWrite_Mode;
	if (xcb[h].Media_Configuration & COMPAQ_MODE)		  *Mode = MSG_COMPAQ_Mode;
	if (xcb[h].Media_Configuration & BIDIRECTIONAL_MODE)	  *Mode = MSG_Bidir_Mode;
	if (xcb[h].Media_Configuration & NON_BIDIRECTIONAL_MODE) *Mode = MSG_Non_Bidir_Mode;
}

static Setup_Configuration(h, Mode)
int Mode;
{
	int	i;

	for (i = 7; i >= 0; i--)
		if ((Mode >> i) & 1)
			if (Setup_Table[i](h, Mode) == 0)
				return (0);
	return (ERR_SELECTED_CONFIGURATION);
}

static Not_Implemented(h)
{
	return (-1);
}

/***************************************************************************************************************; */
/* Process Keyword Functions											; */
/*														; */
/*	Procedure Type:	Near											; */
/*														; */
/*	Assumes:	interrupts disabled, remain disabled							; */
/*			AX contains value if SI == 0								; */
/*			SI  = 0 if keyword is to be set								; */
/*			SI -> ASCII value to parse								; */
/*														; */
/*	Returns:	DX  = 0 if success									; */
/*			DX -> Error message if failure								; */
/*			BX  = number of	bytes used from SI if SI != 0						; */
/*														; */
/*	Preserves:	CX											; */
/*														; */
/*	Description:	These routines are vectored into from the Media_Keyword_Function Table. If		; */
/*			the link layer determines that a keyword match has been found, it calls these		; */
/*			routines to set the appropriate values. If the link layer parses the strings, it 	; */
/*			sets SI to 0 and passes the numeric value in AX, otherwise these functions return	; */
/*			the number of bytes parsed from SI							; */
/*--------------------------------------------------------------------------------------------------------------; */
static void Process_Non_Keyword(h)
{
	xcb[h].Media_Configuration	= NON_BIDIRECTIONAL_MODE;
}

static void Process_LPT_Keyword(h)
{	/* Needs some thought */
}

static void Process_LinkDis_Keyword(h)
{
	xcb[h].Link_Integrity = 0;
}

static void Process_SPP_Keyword(h)
{}

static void Configure_PPIC(h, Reg1, Reg2)
{
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | PPI_STROBE);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
	xcb[h].Set_Ctrl_Reg(h, GENREG8+PPI_RISING_EDGES);
	xcb[h].Set_Ctrl_Reg(h, Reg1);
	xcb[h].Set_Ctrl_Reg(h, Reg2 | PPI_COWCATCHER | (xcb[h].Media_Data_Strobe & PPI_RDS ? 0 : PPI_USE_STROBE) );
	xcb[h].Set_Ctrl_Reg(h, (xcb[h].Media_Configuration & EPP_MODE ? 3 : 0) );
	OUTB(xcb[h].Media_IO_Address,0, 0);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl | xcb[h].Media_Data_Strobe);
	OUTB(xcb[h].Media_IO_Address,2, xcb[h].LPT_Write_Ctrl);
}

/***************************************************************************************************************; */
/* Read Pocket LAN EEPROM											; */
/*														; */
/* Type:		Near											; */
/*														; */
/* Assumes:	DS  = DATA_GROUP alias										; */
/*														; */
/* Returns:	CX  = checksum of EEPROM (0 == success)								; */
/* Destroys:	AX, BX, CX, DI, ES										; */
/*														; */
/* Description: 	block reads the address EEPROM into the EEPROM buffer passed in es:di.			; */
/*--------------------------------------------------------------------------------------------------------------; */
static Read_PX_EEPROM(h)
{
	char	*p, *q;

	if (Read_EEPROM(h, (char *)&xcb[h].PX_EEPROM_Buffer) != 0) return (-1);
	if (xcb[h].PX_EEPROM_Buffer.EE_Address_Prefix[0] != XIRCOM_ADDRESS_1) return (-1);
	if (xcb[h].PX_EEPROM_Buffer.EE_Address_Prefix[1] != XIRCOM_ADDRESS_2) return (-1);
	if (xcb[h].PX_EEPROM_Buffer.EE_Address_Prefix[2] != XIRCOM_ADDRESS_3) return (-1);

	for (p = MSG_Copyright, q = (char *) xcb[h].PX_EEPROM_Buffer.EE_Copyright; *p; p++, q++)
		if (*p != *q) return (-1);
	if (*q < '3') return (-1);
	return (0);
}

/***************************************************************************************************************; */
/* PE3 specific EEPROM routines called from EEPROM.INC/EEPROM.C							; */
/***************************************************************************************************************; */

/***************************************************************************************************************; */
/* EEPROM Enable												; */
/*														; */
/*														; */
/* Description: Enables the chip select on the EEPROM. All subsequent calls to EEPROM_Put_Bit/EEPROM_Get_Bit	; */
/*		will be affected.										; */
/*--------------------------------------------------------------------------------------------------------------; */
static void EEPROM_Enable(h)
{
	PUT_REGISTER(h, GENREG1, PPI_EECS);
	xcb[h].EEPROM_Enable_Flag |= PPI_EECS;
}

/***************************************************************************************************************; */
/* EEPROM Disable												; */
/* Description: Disables the chip select on the EEPROM. All subsequent calls to EEPROM_Put_Bit/EEPROM_Get_Bit	; */
/*		will be affected.										; */
/*--------------------------------------------------------------------------------------------------------------; */
static void EEPROM_Disable(h)
{
	PUT_REGISTER(h, GENREG1, 0);
	xcb[h].EEPROM_Enable_Flag =  0;
}

/***************************************************************************************************************; */
/* EEPROM Get Bit												; */
/*														; */
/*														; */
/* Description: 	inputs one bit in serial stream to EEPROM						; */
/*--------------------------------------------------------------------------------------------------------------; */
static EEPROM_Get_Bit(h)
{
	return (GET_REGISTER(h, GENREG1) & 1);
}

/***************************************************************************************************************; */
/* EEPROM Put Bit												; */
/*														; */
/* Type:		Near											; */
/*														; */
/* Assumes:	AL  = bit 0 contains bit to be output								; */
/*		DS  = DATA_GROUP										; */
/*														; */
/* Returns:	nothing												; */
/* Destroys:	AX, DX												; */
/*														; */
/* Description: 	outputs one bit in serial stream to EEPROM						; */
/*--------------------------------------------------------------------------------------------------------------; */
static void EEPROM_Put_Bit(h, Bit)
int	Bit;
{
	PUT_REGISTER(h, GENREG1, Bit | xcb[h].EEPROM_Enable_Flag);
	PUT_REGISTER(h, GENREG1, Bit | xcb[h].EEPROM_Enable_Flag | PPI_EESK);
	PUT_REGISTER(h, GENREG1, Bit | xcb[h].EEPROM_Enable_Flag);
}


#include "sh.c"
#include "dqnt.c"
#include "wbt.c"
#include "ps2.c"
#include "epp.c"
#include "compaq.c"

#include "../nice/eth.c"
#include "../eeprom.c"
